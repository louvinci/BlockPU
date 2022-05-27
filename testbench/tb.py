from msilib.schema import Error
import numpy as np
from numpy import random
import os
import torch
np.set_printoptions(threshold=1000000) #全部输出
torch.set_printoptions(threshold=np.inf)
def input_gen(height,width,channel,datatype,fdir,DEBUG=False):
    if 'float' in datatype:
        input = random.rand((height,width,channel)).astype(np.float32) # np.float32
    elif 'int16' in datatype:
        input = random.randint(0,3,(height,width,channel)).astype(np.short)
    elif 'int8' in datatype:
        input = random.randint(0,3,(height,width,channel)).astype(np.int8)
    else:
        raise Exception
    input = input - 1
    if DEBUG:
        print(input)
    input.tofile(fdir)
    return input

def pw_weight_gen(M,N,Tm,Tn,datatype,fdir,DEBUG=False):
    if 'float' in datatype:
        input = random.rand((M/Tm,N/Tn,Tm,Tn)).astype(np.float32) # np.float32
    elif 'int16' in datatype:
        input = random.randint(0,3,(M//Tm,N//Tn,Tm,Tn)).astype(np.short)
    elif 'int8' in datatype:
        input = random.randint(0,3,(M//Tm,N//Tn,Tm,Tn)).astype(np.int8)
    else:
        raise Exception
    input = input -1 
    if DEBUG:
        print(input)
    input.tofile(fdir)

# Tn packed format (N/Tn1) * K * K*Tn1
def dw_weight_gen2(K,N,pack_num,datatype,fdir,DEBUG=False):
    if 'float' in datatype:
        input = random.rand((N//pack_num,K,K,pack_num)).astype(np.float32) # np.float32
    elif 'int16' in datatype:
        input = random.randint(0,3,(N//pack_num,K,K,pack_num)).astype(np.short)
    elif 'int8' in datatype:
        input = random.randint(0,3,(N//pack_num,K,K,pack_num)).astype(np.int8)
    else:
        raise Exception
    input = input -1
    if DEBUG:
        print(input)
    input.tofile(fdir)
    #input = input.transpose(0,3,1,2).reshape(N,K,K)
    return input

def res_get2(H,W,C,pack_num,datatype,dir,DEBUG=False):
    if datatype == 'int16':
        x = np.fromfile(dir,dtype= np.short)
    elif datatype == 'int8':
        x = np.fromfile(dir,dtype=np.int8)
    else:
        raise Exception('Only support INT8 and INT16')

    x = x.reshape(H//pack_num,W,C,pack_num).transpose(0,3,1,2)
    x = x.reshape(H,W,C)
    if DEBUG:print(x)
    return x

def res_getpw(M,N,Tm,Tn,datatype,dir,DEBUG=False):
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

def res_get(H,W,C,datatype,dir,DEBUG=False):
    if datatype == 'int16':
        x = np.fromfile(dir,dtype= np.short)
    elif datatype == 'int8':
        x = np.fromfile(dir,dtype=np.int8)
    else:
        raise Exception('Only support INT8 and INT16')
    x = x.reshape(H,W,C)
    if DEBUG:print(x)
    return x



def main():
    torch.manual_seed(123)
    dir="E:/NAS_Project/VitisHLS/ThreeBPU/testbench"
    R1= 14
    C1= 14
    N1= 16
    E = 6
    K=7
    M1= E*N1
    DwTn1 = 8
    PwTn1 = 4
    Tm2 = 2 #! this version the weight is not packed

    datatype = 'int8'
    dw_wt   = dw_weight_gen2(K,N1,DwTn1,datatype,os.path.join(dir,"dw_weight.bin"),False)
    input   = input_gen(R1,C1,N1,datatype,os.path.join(dir,"input.bin"),False)
    pw1_wt  = pw_weight_gen(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw1_weight.bin"),False) # M1/Tm * N1/PwTn1 * Tm*PwTn1
    pw2_wt  = pw_weight_gen(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw2_weight.bin"),False)
    
    

    input    =  res_get(R1,C1,N1,datatype,os.path.join(dir,"input.bin"),False)
    dw_wt    =  res_get2(N1,K,K,DwTn1,datatype,os.path.join(dir,"dw_weight.bin"),False)
    pw1_wt   =  res_getpw(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw1_weight.bin"),False)
    pw2_wt   =  res_getpw(M1,N1,Tm2,PwTn1,datatype,os.path.join(dir,"pw2_weight.bin"),False)
    pw2_wt2  =  pw2_wt.transpose(0,2,1)

    input_tensor = torch.tensor(input.transpose(2,0,1),requires_grad=False).unsqueeze(0)
    
    weight_tensor = torch.tensor(dw_wt,requires_grad=False).unsqueeze(1)
    print(input,file=open(os.path.join(dir,'input_HWN.txt'),'w+'))
    print(input_tensor,file=open(os.path.join(dir,'input_NHW.txt'),'w+'))
    print(weight_tensor,file=open(os.path.join(dir,'dw_weight.txt'),'w+'))
    dwconv1=torch.nn.Conv2d(N1,N1,kernel_size=K,stride=1,padding=3,groups=N1,bias=False)
    dwconv1.weight.data=weight_tensor

    
    pw1_wt_tensor = torch.tensor(pw1_wt.transpose(1,2,0),requires_grad=False).unsqueeze(-1)
    print(pw1_wt_tensor,file=open(os.path.join(dir,'pw1_weight.txt'),'w+'))
    pwconv1 = torch.nn.Conv2d(N1,M1,kernel_size=1,stride=1,padding=0,groups=1,bias=False)
    pwconv1.weight.data= pw1_wt_tensor

    pw2_wt_tensor = torch.tensor(pw2_wt2.transpose(1,2,0),requires_grad=False).unsqueeze(-1)
    print(pw2_wt_tensor.transpose(1,0),file=open(os.path.join(dir,'pw2_weight_T.txt'),'w+'))
    pwconv2 = torch.nn.Conv2d(M1,N1,kernel_size=1,stride=1,padding=0,groups=1,bias=False)
    pwconv2.weight.data= pw2_wt_tensor
    
    with torch.no_grad():
        output_tensor= dwconv1(input_tensor)
        print(output_tensor.type(),output_tensor.shape)
        print(output_tensor,file=open(os.path.join(dir,'dw_res.txt'),'w+'))
        res= pwconv1(output_tensor) # .byte() # usinged INT8
        print(res,file=open(os.path.join(dir,'pw1_res.txt'),'w+'))
        res=pwconv2(res)

    #print(output_tensor)
    print(res.shape)
    print(res,file=open(os.path.join(dir,'hls_resNCHW.txt'),'w+'))
    # NCHW
    res_sw = res.numpy().transpose(0,2,3,1)
    #transpose
    print(res_sw,file=open(os.path.join(dir,'hls_resNHWC.txt'),'w+'))
    res_sw.tofile(os.path.join(dir,"hls_res.bin"))

if __name__ == "__main__":
    main()
