from struct import pack
import numpy as np
from numpy import random
import os
import torch
np.set_printoptions(threshold=1000000) #全部输出
torch.set_printoptions(threshold=np.inf)
BIT32 = 32
INTbit32 = 10
INTbit16 = 8
INTbit8 = 3


def fixed_type(input,bit,intege_bit):
    assert intege_bit <= bit,'ERROR bit assign'
    if torch.is_tensor(input) == False:
        input = torch.tensor(input)
    intege =torch.round(input) #rounding
    frac = input - intege
    #print(intege,frac)
    frac_scale = 2**(bit-intege_bit)#to use bit shift here, so the scale is not 2**(bit-intege_bit)-1
    inte_scale = 2**(intege_bit-1)-1
    intege = torch.clamp(intege,-inte_scale-1,inte_scale)# cliping
    frac=torch.round(frac* frac_scale)/frac_scale
    res = intege+frac
    q_res = (res*frac_scale).int()
    return q_res


def input_gen(height,width,channel,DwTn,datatype,fdir,DEBUG=False):
    if 'float' in datatype:
        input = random.rand((height,width,channel)).astype(np.float32) # np.float32
    elif 'int16' in datatype:
        input = random.randint(-128,127,(height,width,channel)).astype(np.short)
    elif 'int8' in datatype:
        input = random.randint(-128,127,(height,width,channel)).astype(np.int8)
    else:
        raise Exception
    pad = channel% DwTn
    if channel > DwTn and pad!=0:
        input = np.pad(input,((0,0),(0,0),(0,pad)),"constant",constant_values = (-77))
    
    if DEBUG:
        print(input)
    input.tofile(fdir)
    return input

def get_in(H,W,C,DwTn,datatype,dir,DEBUG=False):
    if datatype == 'int16':
        x = np.fromfile(dir,dtype= np.short)
    elif datatype == 'int8':
        x = np.fromfile(dir,dtype=np.int8)
    else:
        raise Exception('Only support INT8 and INT16')
    pad = C% DwTn
    if C > DwTn and pad!=0:
        C = C+pad
        x = x.reshape(H,W,C)
        x = x[:,:,:-pad]
    else:
        x = x.reshape(H,W,C)
    if DEBUG:print(x)
    return x

def norm_wt_gen(N,datatype,fdir):
    wt   = random.randn(N).astype(np.float32)/(2**8)
    bias = random.randn(N).astype(np.float32)
    # pack wt_bias to uint64

    norm_wt =  np.vstack((wt,bias)).T.reshape(-1)
    
    if "int32" in datatype:
        q_norm_wt = fixed_type(norm_wt,32,INTbit32).numpy().astype(np.int32)
    elif 'int16' in datatype:
        q_norm_wt = fixed_type(norm_wt,16,INTbit16).numpy().astype(np.short)
    elif "int8" in datatype:
        q_norm_wt = fixed_type(norm_wt,8,INTbit8).numpy().astype(np.int8)
    else:
        raise Exception
    q_norm_wt.tofile(fdir)
    return q_norm_wt

def get_norm(N,datatype, dir):
    if "int32" in datatype:
        x = np.fromfile(dir,dtype= np.int32)
    elif 'int16' in datatype:
        x = np.fromfile(dir,dtype= np.short)
    elif  'int8' in datatype:
        x = np.fromfile(dir,dtype=np.int8)
    else:
        raise Exception('Only support INT8 and INT16')
    x = x.reshape(N,2).T.reshape(-1)
    wt = x[:N]
    bias = x[N:2*N]
    return wt,bias
    

def pw_weight_gen(M,N,Tm,Tn,datatype,fdir,DEBUG=False):

    if 'float' in datatype:
        input = random.rand((M/Tm,N/Tn,Tm,Tn)).astype(np.float32) # np.float32
    elif 'int16' in datatype:
        input = random.randint(-64,64,(M//Tm,N//Tn,Tm,Tn)).astype(np.short)
    elif 'int8' in datatype:
        input = random.randint(-64,64,(M//Tm,N//Tn,Tm,Tn)).astype(np.int8)
    else:
        raise Exception
  
    if DEBUG:
        print(input)
    input.tofile(fdir)

# Tn packed format (N/Tn1) * K * K*Tn1
def dw_weight_gen(N,K,pack_num,datatype,fdir,DEBUG=False):
    if 'float' in datatype:
        input = random.rand((N,K,K)).astype(np.float32) # np.float32
    elif 'int16' in datatype:
        input = random.randint(-64,64,(N,K,K)).astype(np.short)
    elif 'int8' in datatype:
        input = random.randint(-64,64,(N,K,K)).astype(np.int8)
    else:
        raise Exception
    if N < pack_num:
        pack_num = N
    elif N%pack_num !=0:
        pad =pack_num - (N%pack_num)
        input = np.pad(input, ( (0,pad),(0,0),(0,0) ) )
        N =N + pad
    input  = input.reshape( N//pack_num, pack_num,K , K ).transpose(0,2,3,1)
    if DEBUG:
        print(input)
    input.tofile(fdir)

    return input

def get_dw(N,K,pack_num,datatype,dir,DEBUG=False):
    #here is N,K,K, 
    pad = 0
    if datatype == 'int16':
        x = np.fromfile(dir,dtype= np.short)
    elif datatype == 'int8':
        x = np.fromfile(dir,dtype=np.int8)
    else:
        raise Exception('Only support INT8 and INT16')
    
    if N < pack_num:
        pack_num = N
    elif N%pack_num != 0:
        pad =pack_num - (N%pack_num)
        N = N + pad

    x = x.reshape(N//pack_num,K,K,pack_num).transpose(0,3,1,2)
    x = x.reshape(N,K,K) 
    if pad != 0:
        x = x[:-pad,:,:]# for the pytorch compute,ignore the pad "0"
    if DEBUG:print(x)
    return x

def get_pw(M,N,Tm,Tn,datatype,dir,DEBUG=False):
 
    if datatype == 'int16':
        x = np.fromfile(dir,dtype= np.short)
    elif datatype == 'int8':
        x = np.fromfile(dir,dtype=np.int8)
    else:
        raise Exception('Only support INT8 and INT16')
    x = x.reshape(M//Tm,N//Tn,Tm,Tn).transpose(0,2,1,3)
    x = x.reshape(1,M,N)
    if DEBUG:print(x)
    return x   





def main():
    torch.manual_seed(123)
    dir="./"
    R1= 14
    C1= 14
    N1= 128
    E = 4
    K=7
    M1= E*N1
    DwTn1 = 32
    PwTn1 = 4
    Tm2 = 64

    # DwTn1 = 8
    # PwTn1 = 2
    # Tm2 = 2

    rescale1    =  fixed_type(0.414/(2**8),32,INTbit32)
    rescale2    =  fixed_type(0.715/(2**8),32,INTbit32)
    id_rescale  =  fixed_type(1.0/(2**8),32,INTbit32)
    datatype    =  'int8'
    range_out   =  2**(8-1)
    print(rescale1,rescale2,id_rescale)
    input_gen(R1,C1,N1,DwTn1,datatype,os.path.join(dir,"input.bin"),False)
    dw_weight_gen(N1,K,DwTn1,datatype,os.path.join(dir,"dw_wt.bin"),False)
    pw_weight_gen(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw1_wt.bin"),False) # M1/Tm * N1/PwTn1 * Tm*PwTn1
    pw_weight_gen(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw2_wt.bin"),False)
    norm_wt_gen(N1,'int32',os.path.join(dir,"norm.bin")) # store the weight and bias in the fixed16 or fixed8

    

    input    =   get_in(R1,C1,N1,DwTn1,datatype,os.path.join(dir,"input.bin"),False)
    dw_wt    =   get_dw(N1,K,DwTn1,datatype,os.path.join(dir,"dw_wt.bin"),False)
    pw1_wt   =   get_pw(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw1_wt.bin"),False)
    pw2_wt   =   get_pw(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw2_wt.bin"),False)
    pw2_wt2  =   pw2_wt.transpose(0,2,1)
    norm_wt ,norm_bias  =  get_norm(N1,'int32',os.path.join(dir,"norm.bin"))
    
    print(norm_wt,file=open(os.path.join(dir,'norm_wt.txt'),'w+'))
    print(norm_bias,file=open(os.path.join(dir,'norm_bias.txt'),'w+'))

    input_tensor = torch.tensor(input.transpose(2,0,1),requires_grad=False).unsqueeze(0).long()
    weight_tensor = torch.tensor(dw_wt,requires_grad=False).unsqueeze(1).long()
    print(input,file=open(os.path.join(dir,'input_HWN.txt'),'w+'))
    print(input_tensor,file=open(os.path.join(dir,'input_NHW.txt'),'w+'))
    print(weight_tensor,file=open(os.path.join(dir,'dw_weight.txt'),'w+'))
    if K ==7 :
        pad = 3
    elif K==3:
        pad = 1
    else:
        raise Exception("Not support other K size")
        
    dwconv1=torch.nn.Conv2d(N1,N1,kernel_size=K,stride=1,padding=pad,groups=N1,bias=False)
    print("dwconv1 weight shape: ",dwconv1.weight.data.shape,"Generate Weight Shape: ", weight_tensor.shape)
    dwconv1.weight.data=weight_tensor

    
    pw1_wt_tensor = torch.tensor(pw1_wt.transpose(1,2,0),requires_grad=False).unsqueeze(-1).long()
    print(pw1_wt_tensor,file=open(os.path.join(dir,'pw1_wt.txt'),'w+'))
    pwconv1 = torch.nn.Conv2d(N1,M1,kernel_size=1,stride=1,padding=0,groups=1,bias=False)
    pwconv1.weight.data= pw1_wt_tensor

    pw2_wt_tensor = torch.tensor(pw2_wt2.transpose(1,2,0),requires_grad=False).unsqueeze(-1).long()
    print(pw2_wt_tensor.transpose(1,0),file=open(os.path.join(dir,'pw2_wt_T.txt'),'w+'))
    pwconv2 = torch.nn.Conv2d(M1,N1,kernel_size=1,stride=1,padding=0,groups=1,bias=False)
    pwconv2.weight.data= pw2_wt_tensor

    norm_wt_tensor    =  torch.tensor(norm_wt,   requires_grad=False).reshape(1,N1,1,1).long()
    norm_bias_tensor =  torch.tensor(norm_bias,requires_grad=False).reshape(1,N1,1,1).long()
    
    print("input tensor: ",input_tensor.dtype,input_tensor.shape)
    with torch.no_grad():

        output_tensor= dwconv1(input_tensor)
        output_tensor = output_tensor*norm_wt_tensor + norm_bias_tensor
        q_x =torch.div( output_tensor, 2**(BIT32-INTbit32),rounding_mode='floor')
        q_x = torch.clamp(q_x,-range_out+1,range_out-1) #! not use the -128
        
        print("dw tensor: ",q_x.dtype,q_x.shape)
        print(q_x,file=open(os.path.join(dir,'dw_res.txt'),'w+'))

        res= pwconv1(q_x) # .byte() # usinged INT8
        res = res*rescale1
        res =torch.div( res, 2**(BIT32-INTbit32),rounding_mode='floor') # not trunc
        res = torch.clamp(res,0,range_out-1)
        print(res,file=open(os.path.join(dir,'pw1_res.txt'),'w+'))
        res=pwconv2(res)
        res_tmp = res.permute(0,2,3,1)
        print(res_tmp,file=open(os.path.join(dir,'pw2_res.txt'),'w+'))
        res = res*rescale2  + input_tensor * id_rescale
        res =torch.div( res, 2**(BIT32-INTbit32),rounding_mode='floor')
        res = torch.clamp(res,-range_out+1,range_out-1)

    res = res.char() #!must transform to INT8
    print("output tensor: ", res.dtype,res.shape)
    print(res,file=open(os.path.join(dir,'sw_resNCHW.txt'),'w+'))
    # NCHW
    res_sw = res.numpy().transpose(0,2,3,1)
    #transpose
    print(res_sw,file=open(os.path.join(dir,'sw_resNHWC.txt'),'w+'))
    res_sw.tofile(os.path.join(dir,"sw_res.bin"))

if __name__ == "__main__":
    main()