'''
    mode 0: DW + PW(coldbuffer) + PW:           equals    (fused DW+PW1 ) + PW2, coldbuffer:[PW1_TM][PW1_M*Tr*Tc]
    mode 1: DW(coldbuffer)+PW+PW(coldbuffer)    equals    (DW coldbuffer) + (fused PW1+PW2)
    BWin,BW_wt: Byte/cycle
'''
#Version1-2022-5-7
#   the coldbuffer after DW is used to keep the results(Abit). The coldbuffer after the PW core is used to keep intermediate results (Accbit), to accumate in [Tn] dimmensioin 
#Revise 2022-5-12
  #- PW_WEIGHT_Cache should decrease the N/Tn loops times, Fix the bug  when N< PW_WEIGHT_Cache*Tn
  #- Accbit in PE's out buffer,doublebuffer is the databit
#Note
# *PW_WEIGHT_Cache value influence the bandwidth much
# *the bandwith can keep the computation latency conver the in and weight latency in DW and  PW1/PW2(when total_loops>WEIGHT_CACHE)
# we don't apply the output double buffering for the pw1

#Version4 2022-5-17
#! the latency of matrix-add operation or keep the input when the pw2 is computing 
# the bandwidth of branch in to execute the Matrix-add "+=" is overlook, matrix_add is parallel between DW and PW engine,
# otherwise the bandwidth overhead is too much,
#! 5-21 , in pw1 latency,when dw_tn!= pw_tn, modified the value in_tn, and out_tn

import numpy as np
BrustDDRSetupTime=64
PW_WEIGHT_Cache=4

STAGE_lAT=4

def BRAM_CONSUM(data_bit,data_num):
    if data_num < 16:
        return 0 
    if data_bit> 36:
        bram_perdata = np.ceil(data_bit / 36)
        num = bram_perdata *np.ceil(data_num / 512)
    elif data_bit>18:
        num = np.ceil(data_num / 512)
    elif data_bit >9:
        num = np.ceil(data_num / 1024)
    else:
        num = np.ceil(data_num / 2048)
    return num

def dw_resource(tn,tr,tc,K,Dbit=8):
    inbuf  = BRAM_CONSUM(tn*Dbit,(tr+K-1)*(tc+K-1))
    wtbuf  = BRAM_CONSUM(tn*Dbit,K*K)
    outbuf = BRAM_CONSUM(tn*Dbit*2,tr*tc)
    return (inbuf+outbuf+wtbuf)*2

def pw_resouce(tm,tn,tr,tc,coldbuf=False,Dbit=8,out_db=False):
    inbuf,outbuf,wtbuf = 0,0,0
    assert (coldbuf & out_db) ==False,'coldbuf and out_db cannot be True at the same time'
    if coldbuf:
        inbuf  = BRAM_CONSUM(tn*Dbit,tr*tc) # load PW_WEIGHT_Cache times tm weights 
        outbuf = 0 # directly acculmuate in the coldbuffer
    else:
        inbuf  = BRAM_CONSUM(tn*Dbit,tr*tc*PW_WEIGHT_Cache)# load PW_WEIGHT_Cache times tn weights 
        outbuf = BRAM_CONSUM(tm*Dbit*2,tr*tc)

    wtbuf  = 0  #tm*tn complete partition 

    if out_db:
        total_bram = (inbuf+wtbuf+outbuf)*2
    else:
        total_bram = (inbuf+wtbuf)*2+outbuf

    return total_bram


def dw_latency(tn,tr,tc,dwN,K,BWin,BWwt,Dbit=8):
    in_num_cycle  = (BWin*8.0) / Dbit
    wt_num_cycle  = (BWwt*8.0) / Dbit
    loops  =  np.ceil(dwN/tn)

    in_lat   = BrustDDRSetupTime + np.ceil(tn/in_num_cycle) * (tr+K-1)*(tc+K-1)
    wt_lat   = BrustDDRSetupTime + np.ceil(tn/wt_num_cycle) * K*K
    comp_lat = tr*tc*K*K
    out_lat  = tr*tc #do quant and output to coldbuffer 

    in_wt_lat      =  max(in_lat,wt_lat)
    
    if loops>=2:
        in_wt_comp_lat =  max(in_wt_lat,comp_lat)
        loop_lat       =  max(in_wt_lat,comp_lat)+STAGE_lAT
        total_lat = in_wt_lat + in_wt_comp_lat+ (loops-2)*loop_lat + comp_lat + out_lat
    else:
        total_lat = in_wt_lat + comp_lat+out_lat
    print("dwin:{}, wt:{}, comp:{}".format(in_lat,wt_lat,comp_lat))
    return total_lat

def pw1pw2_latency(Pw1N,Pw1M,pw1_tm,pw1_tn,in_tn,tr,pw1_wt_bw,pw2_wt_bw,Dbit=8):
    pw1wt_num_cycle   = np.floor((pw1_wt_bw*8.0)  / Dbit) 
    pw2wt_num_cycle   = np.floor((pw2_wt_bw*8.0)  / Dbit) 

    pw2_tm, pw2_tn = pw1_tn, pw1_tm
    pw1_tn_loops, pw1_tm_loops = np.ceil(Pw1N / pw1_tn),  np.ceil(Pw1M/pw1_tm)
    pw2_tn_loops, pw2_tm_loops = pw1_tm_loops,pw1_tn_loops

    #*inner-pw1-tn
    if pw1_tn_loops>= PW_WEIGHT_Cache:
        pw1_tn_loops   =  np.ceil(pw1_tn_loops / PW_WEIGHT_Cache)
        pw1_lat_in     =  np.ceil(pw1_tn/in_tn)*PW_WEIGHT_Cache*tr*tr # load PW_WEIGHT_Cache times of weight along the [tn] dimmension
        pw1_lat_wt     =  BrustDDRSetupTime + np.ceil(pw1_tn/pw1wt_num_cycle)*pw1_tm*PW_WEIGHT_Cache
        pw1comp_lat    =  PW_WEIGHT_Cache*tr*tr
    else:
        pw1_lat_in     =  np.ceil(pw1_tn/in_tn)*tr*tr
        pw1_lat_wt     =  BrustDDRSetupTime + np.ceil(pw1_tn/pw1wt_num_cycle)*pw1_tm
        pw1comp_lat    =  tr*tr
    pw1_in_wt_lat  = max(pw1_lat_in,pw1_lat_wt)
    pw1_tnloop_lat = max(pw1_in_wt_lat,pw1comp_lat)+STAGE_lAT
    inner_lat = pw1_in_wt_lat + (pw1_tn_loops-1)*pw1_tnloop_lat + pw1comp_lat
    
    #*outer-pw2-tm
    pw2_lat_in = tr*tr # only load once in the tm loops
    if pw2_tm_loops >=PW_WEIGHT_Cache:
            pw2_tm_loops   =  np.ceil(pw2_tm_loops / PW_WEIGHT_Cache)
            pw2_wt_lat     =  BrustDDRSetupTime + np.ceil(pw2_tm/pw2wt_num_cycle)*pw2_tn*PW_WEIGHT_Cache
            pw2_comp_lat   =  PW_WEIGHT_Cache*tr*tr
    else:
            pw2_wt_lat     =  BrustDDRSetupTime + np.ceil(pw2_tm/pw2wt_num_cycle)*pw2_tn
            pw2_comp_lat   =  tr*tr
    outer_lat = max(pw2_wt_lat,pw2_lat_in) + (pw2_tm_loops-1)*(max(pw2_wt_lat,pw2_comp_lat)+STAGE_lAT) + pw2_comp_lat    

    #* common loop
    total_lat = inner_lat+(pw1_tm_loops-1)*(max(inner_lat,outer_lat)+STAGE_lAT) + outer_lat
    print("inner lat: {}, outner lat:{}".format(inner_lat,outer_lat))
    return total_lat

def pw1pw2_optimized(Pw1N,Pw1M,pw1_tm,pw1_tn,in_tn,tr,pw1_wt_bw,pw2_wt_bw,Dbit=8):
    pw1wt_num_cycle   = np.floor((pw1_wt_bw*8.0)  / Dbit) 
    pw2wt_num_cycle   = np.floor((pw2_wt_bw*8.0)  / Dbit) 

    pw2_tm, pw2_tn = pw1_tn, pw1_tm
    pw1_tn_loops, pw1_tm_loops = np.ceil(Pw1N / pw1_tn),  np.ceil(Pw1M/pw1_tm)

    #*innerloop pw1-tn; pw2-tm
    pw2_in_lat = tr*tr # only load once in the tm loops
    if pw1_tn_loops>= PW_WEIGHT_Cache:
        pw1_tn_loops   =  np.ceil(pw1_tn_loops / PW_WEIGHT_Cache)
        pw1_in_lat     =  np.ceil(pw1_tn/in_tn)*PW_WEIGHT_Cache*tr*tr # load PW_WEIGHT_Cache times of weight along the [tn] dimmension
        pw1_wt_lat     =  BrustDDRSetupTime + np.ceil(pw1_tn/pw1wt_num_cycle)*pw1_tm*PW_WEIGHT_Cache
        pw1_comp_lat   =  PW_WEIGHT_Cache*tr*tr
        pw2_wt_lat     =  BrustDDRSetupTime + np.ceil(pw2_tm/pw2wt_num_cycle)*pw2_tn*PW_WEIGHT_Cache
        pw2_comp_lat   =  PW_WEIGHT_Cache*tr*tr
        
    else:
        pw1_in_lat     =  np.ceil(pw1_tn/in_tn)*tr*tr
        pw1_wt_lat     =  BrustDDRSetupTime + np.ceil(pw1_tn/pw1wt_num_cycle)*pw1_tm
        pw1_comp_lat   =  tr*tr
        pw2_wt_lat     =  BrustDDRSetupTime + np.ceil(pw2_tm/pw2wt_num_cycle)*pw2_tn
        pw2_comp_lat   =  tr*tr
    # stageF1:(pw1_in&pw1_wt)
    # stageF2:(pw1_in&pw1_wt &pw1_comp)
    # stageF3:(pw1_in&pw1_wt &pw1_comp) & (pw2_in&pw2_wt)
    # stage:  (pw1_in&pw1_wt &pw1_comp) & (pw2_wt&pw2_comp) 
    # ...
    # stageL3:(pw1_comp) & (pw2_wt&pw2_comp)
    # stageL2:(pw2_wt&pw2_comp)
    # stageL1:(pw2_comp)
    pw2_wt_comp_lat = max(pw2_wt_lat, pw2_comp_lat)
    pw2_in_wt_lat   = max(pw2_in_lat, pw2_wt_lat  )
    stageF1_lat = max(pw1_in_lat,   pw1_wt_lat)
    stageF2_lat = max(stageF1_lat,  pw1_comp_lat)
    stageF3_lat = max(stageF2_lat,  pw2_in_wt_lat)
    stage_lat   = max(stageF2_lat,  pw2_wt_comp_lat) + STAGE_lAT
    stageL3_lat = max(pw1_comp_lat, pw2_wt_comp_lat)
    
    total_loops = pw1_tn_loops * pw1_tm_loops # loop merge
    if total_loops>=3:
        total_lat = stageF1_lat + stageF2_lat + stageF3_lat + (total_loops-3)*stage_lat + stageL3_lat + pw2_wt_comp_lat + pw2_comp_lat
    elif total_loops ==2:
        total_lat = stageF1_lat + stageF2_lat + stageF3_lat + max(pw1_comp_lat, pw2_in_wt_lat) + pw2_wt_comp_lat + pw2_comp_lat
    else:
        total_lat = stageF1_lat + pw1_comp_lat + pw2_in_wt_lat + pw2_comp_lat


    return total_lat

def dwpw1_latency(N,M,K,dw_tn,pw_tm,pw_tn,tr,dw_bw_in,dw_bw_wt,pw_bw_wt,Dbit=8):
    dwwt_num_cycle   =  np.floor((dw_bw_in*8.0)  / Dbit)
    dwin_num_cycle   =  np.floor((dw_bw_wt*8.0)  / Dbit)
    pwwt_num_cycle   =  np.floor((pw_bw_wt*8.0)  / Dbit)
    dw_tn_loops = np.ceil(N/dw_tn)
    pw_tn_loops = np.ceil(N/pw_tn)
    pw_tm_loops = np.ceil(M/pw_tm)
    #usually dw_tn != pw_tn
    common_loops = max(dw_tn_loops,pw_tn_loops)

    #*inner-dw-tn
    dwin_lat      =  BrustDDRSetupTime + np.ceil(dw_tn/dwin_num_cycle) * (tr+K-1)*(tr+K-1)
    dwwt_lat      =  BrustDDRSetupTime + np.ceil(dw_tn/dwwt_num_cycle) * K*K
    dw_comp_lat   =  tr*tr*K*K
    dw_in_wt_lat  =  max(dwin_lat,dwwt_lat)
    dw_inwt_comp  =  max(dw_in_wt_lat,dw_comp_lat)
    
    #*outer-pw-dm
    pwin_lat       =  np.ceil(pw_tn/dw_tn)*tr*tr # only load once and quant
    if pw_tm_loops >= PW_WEIGHT_Cache:
        pw_tm_loops   =  np.ceil(pw_tm_loops/ PW_WEIGHT_Cache)
        pw_wt_lat     =  BrustDDRSetupTime + np.ceil(pw_tm/pwwt_num_cycle)*pw_tn*PW_WEIGHT_Cache
        pw_comp_lat   =  PW_WEIGHT_Cache*tr*tr
    else:
        pw_wt_lat     =  BrustDDRSetupTime + np.ceil(pw_tm/pwwt_num_cycle)*pw_tn
        pw_comp_lat   =  tr*tr
    outer_lat = max(pwin_lat,pw_wt_lat) + (pw_tm_loops-1)*(max(pw_wt_lat,pw_comp_lat)+STAGE_lAT)+ pw_comp_lat
    print("pw1 outer latency: {}, in: {}, wt:{}, comp:{} ".format(outer_lat,pwin_lat,pw_wt_lat,pw_comp_lat))
          
    #* common loop
    stage_lat = max(dw_inwt_comp,outer_lat)+STAGE_lAT
    if common_loops >=2:
        total_lat = dw_in_wt_lat + dw_comp_lat + (common_loops-2)*stage_lat + max(outer_lat,dw_comp_lat) + outer_lat
    else:
        total_lat = dw_in_wt_lat + dw_comp_lat + outer_lat
    print("dw in {}, wt: {}, comp: {}:".format(dwin_lat,dwwt_lat,dw_comp_lat))
    
    return total_lat

def pw2_latency(N,M,tm,tn,tr,BWwt,BWout,Dbit=8):
    wt_num_cycle   =  np.floor( (BWwt*8.0)  / Dbit)
    out_num_cycle  =  np.floor( (BWout*8.0) / Dbit)
    tn_loops, tm_loops= np.ceil(N / tn) , np.ceil(M/tm)

    lat_out2ddr        =    BrustDDRSetupTime + np.ceil(tm/out_num_cycle)*tr*tr 

    if tn_loops>= PW_WEIGHT_Cache:
        tn_loops   =  np.ceil(tn_loops / PW_WEIGHT_Cache)
        wt_lat     =  BrustDDRSetupTime + np.ceil(tn/wt_num_cycle)*tm*PW_WEIGHT_Cache
        comp_lat   =  PW_WEIGHT_Cache*tr*tr
        in_lat     =  PW_WEIGHT_Cache*tr*tr
    else:
        in_lat     =  tr*tr
        wt_lat     =  BrustDDRSetupTime + np.ceil(tn/wt_num_cycle)*tm
        comp_lat   =  tr*tr      

    in_wt_lat      =  max(wt_lat,in_lat)
    inner_lat = in_wt_lat + (tn_loops-1)*in_wt_lat + comp_lat
    total_lat = inner_lat + (tm_loops-1) * (max(inner_lat,lat_out2ddr)+STAGE_lAT) + lat_out2ddr

    return total_lat
    
# the minimal bandwidth required,keep the computation latency convers
def bandwidth_alloc(block_param,K,comp_mode = 1, databit=8):
    dw_bw_in,dw_bw_wt,  pw1_bw_wt,  pw2_bw_wt,pw2_bw_out = 1,1, 1, 1,1
    tr,dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn = block_param[0:6]
    dw_in  = np.ceil( dw_tn /  ( (tr*tr*K*K - BrustDDRSetupTime) / ( (tr+K-1)*(tr+K-1))) )
    dw_wt  = np.ceil( dw_tn /  ( (tr*tr*K*K - BrustDDRSetupTime) / ( K*K              )) )
    pw1_CompSubSetup_lat =  tr*tr*PW_WEIGHT_Cache - BrustDDRSetupTime
    if pw1_CompSubSetup_lat <=0:
        pw1_wt = pw1_tn
    else:
        pw1_wt = np.ceil( pw1_tn / ( pw1_CompSubSetup_lat / (pw1_tm*PW_WEIGHT_Cache) ) ) 

    dw_bw_in  = 2**(np.ceil(np.log2(dw_in))) * (databit/8)
    dw_bw_wt  = 2**(np.ceil(np.log2(dw_wt))) * (databit/8)
    pw1_bw_wt = 2**(np.ceil(np.log2(pw1_wt))) * (databit/8)

    pw2_bw_wt = pw1_bw_wt
    
    if comp_mode == 0: # pw1+coldbuffer
        pw2_bw_out = 8 * np.floor(databit/8) #!this value is set out2ddr ap_uint<64>
    else:
        pw2_bw_out = 4 * np.floor(databit/8)
    return [dw_bw_in,dw_bw_wt,  pw1_bw_wt,  pw2_bw_wt,pw2_bw_out]

def block_resource(block_param,alloc_layers,MaxK,MaxBit,comp_mode=1):
    '''
    alloc_layers: allocated layer parameter, [ [cin,cout,hin,"k7_e4",stride,databit],...]
    block_param:  hardware parameter,        [tr,dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn]
    here set pw1_tm = pw2_tn, pw1_tn = pw2_tm
    '''
    assert len(block_param)==6,"block hardware params Error"
    coldbuf1,coldbuf2,coldbuf3=0,0,0
    dw_bram,pw1_bram,pw2_bram = 0,0,0
    Maxdw_N  = max(alloc_layers[:,0].astype(np.int))
    Maxpw1_M = int(max(list( map(lambda x:int(x[0])*int(x[3][4]),alloc_layers) )) )
    Maxpw2_M = max(alloc_layers[:,1].astype(np.int))
    
    tr,dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn = block_param[0:6]
    total_dsp = dw_tn + pw1_tm*pw1_tn + pw2_tm*pw2_tn
    dw_bram  = dw_resource(dw_tn,tr,tr,MaxK,MaxBit)
    if comp_mode==0:
        #  dw + (pw+coldbuffer2) + pw,  pw1_tm,pw1_tn = pw2_tm,pw2_tn"
        pw1_num  = np.ceil(Maxpw1_M/pw1_tm)*tr*tr
        pw1_bram = pw_resouce(pw1_tm,pw1_tn,tr,tr,coldbuf=True, Dbit=MaxBit,out_db=False)
        pw2_bram = pw_resouce(pw2_tm,pw2_tn,tr,tr,coldbuf=False,Dbit=MaxBit,out_db=True)
        coldbuf2 = BRAM_CONSUM(data_bit = pw1_tm*MaxBit*2,data_num=pw1_num)
    elif comp_mode==1:
        #(dw+coldbuffer1)+pw+(pw+coldbuff3)
        dw_num  =  np.ceil(Maxdw_N/dw_tn)*tr*tr
        pw2_num =  np.ceil(Maxpw2_M/pw2_tm)*tr*tr
        pw1_bram   =  pw_resouce(pw1_tm,pw1_tn,tr,tr,coldbuf=False,Dbit=MaxBit,out_db=True)
        pw2_bram   =  pw_resouce(pw2_tm,pw2_tn,tr,tr,coldbuf=True, Dbit=MaxBit,out_db=False)
        coldbuf1   =  BRAM_CONSUM(data_bit = dw_tn*MaxBit,    data_num=dw_num)
        coldbuf3   =  BRAM_CONSUM(data_bit = pw2_tm*MaxBit*2, data_num=pw2_num) # pw2's coldbuffer is also server as out_buffer for accmulate 
    else:
        raise Exception('Wrong Comput Mode')

    total_bram = dw_bram+pw1_bram+pw2_bram + (coldbuf1+coldbuf2+coldbuf3)*2
    #print("dw_bram:{}, pw1_bram:{}, pw2_bram:{}, coldbuf1:{},coldbuf2:{} ".format(dw_bram, pw1_bram, pw2_bram, coldbuf1,coldbuf2))
    return total_dsp,total_bram

def block_latency(block_param,bandparam,layer,comp_mode=1,MaxBit=8):
    '''
    layer: signgle layer parameter,          [cin,cout,hin,"k7_e4",stride,databit]
    block_param:  hardware parameter,        [tr,dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn]
    bandparam:    hardware parameter,        [dw_bw_in,dw_bw_wt,  pw1_bw_wt,  pw2_bw_wt,pw2_bw_out]
    here set pw1_tm = pw2_tn, pw1_tn = pw2_tm
    '''
    assert len(block_param)==6,"block hardware params Error"

    tr, dw_tn, pw1_tm, pw1_tn, pw2_tm, pw2_tn = block_param[0:6]
    cin,cout,hin,ktype,stride,databit    = int(layer[0]),int(layer[1]),int(layer[2]),layer[3],int(layer[4]),int(layer[5])
    #print(cin,cout,hin,ktype,stride,databit)
    outerloops =( np.ceil(hin/tr) )**2
    
    dw_bw_in,dw_bw_wt,  pw1_bw_wt,  pw2_bw_wt,pw2_bw_out = bandparam[0:5]
    K,e = int(ktype[1]),int(ktype[4])

    if comp_mode ==1:
        out_num_cycle  =  (pw2_bw_out*8.0) / MaxBit
        part1_lat = dw_latency(dw_tn,tr,tr,cin,K,dw_bw_in,dw_bw_wt,MaxBit)
        part2_lat = pw1pw2_optimized(Pw1N=cin, Pw1M=cin*e, pw1_tm=pw1_tm, pw1_tn=pw1_tn, in_tn=dw_tn, tr=tr,\
                                   pw1_wt_bw=pw1_bw_wt,  pw2_wt_bw=pw2_bw_wt, Dbit=MaxBit)
        part3_lat = np.ceil(pw2_tm/out_num_cycle) * np.ceil(cout/pw2_tm) *tr*tr
        if part3_lat > part2_lat:
            print("Enlarge the pw2 out ddr in mode 1")
        if outerloops >=2:
            part1_2_lat = max(part1_lat,part2_lat)
            part2_3_lat = max(part2_lat,part3_lat)
            block_lat = part1_lat + part1_2_lat+(outerloops-2)* max(part1_2_lat,part3_lat) + part2_3_lat + part3_lat
        else:
            block_lat = part1_lat + part2_lat + part3_lat
        print('dw_latency: {}; pw_pw: {}; Pw_DDR:{} '.format(part1_lat,part2_lat,part3_lat))
    else:
        part1_lat =  dwpw1_latency(N=cin,M=e*cin,K=K,dw_tn=dw_tn,pw_tm =pw1_tm,pw_tn=pw1_tn,tr=tr,\
                                   dw_bw_in=dw_bw_in,dw_bw_wt=dw_bw_wt,pw_bw_wt=pw1_bw_wt,Dbit=MaxBit)
        part2_lat =  pw2_latency(N=cin*e,M=cin,tm=pw2_tm,tn=pw2_tn,tr=tr,\
                                               BWwt=pw2_bw_wt,BWout=pw2_bw_out, Dbit=MaxBit)
        block_lat = part1_lat + (outerloops-1)*max(part1_lat,part2_lat) + part2_lat
        print('dwpw1_latency: {}; pw2: {};'.format(part1_lat,part2_lat))

    return block_lat

def BPU_modeling(block_param,alloc_layers,comp_mode=1):
    '''
    block_param: numpy.array
    alloc_layers:numpy.array
    '''
    assert comp_mode==0 or comp_mode==1, "Only support Mode 0&1"
    bpu_bram,bpu_dsp = 0,0
    layer_wise_lat = []
    MaxDbit  =  max(alloc_layers[:,-1].astype(np.int)) 
    Kmax = max(list( map(lambda x:int(x[3][1]),alloc_layers) ))
    bandparam = bandwidth_alloc(block_param,K=Kmax,comp_mode=comp_mode, databit = MaxDbit)
    #bandparam = [8,8,4,4,4]
    print("Bandwidth allocate: dw_in,dw_wt,  pw1_wt,  pw2_wt,pw2_out ", bandparam)
    bpu_bw = sum(bandparam) # bytes/cycle
    bpu_dsp,bpu_bram  = block_resource(block_param,alloc_layers,MaxK=Kmax,MaxBit=MaxDbit,comp_mode=comp_mode)
    for i in range(len(alloc_layers)):
        layer_lat = block_latency(block_param,bandparam,alloc_layers[i],comp_mode,MaxBit=MaxDbit)
        layer_wise_lat.append(layer_lat)
    
    return (bpu_dsp,bpu_bram,bpu_bw),layer_wise_lat

def Test():
    # zc706:  PS DDR3 5.3GB/s    PL DDR43: 12.8GB/s
    # zcu104:                    PL DDR4 4GB 64bit  21.3GB/s
    # zcu102: PS 21GB/s
    import time
    platform_zc706 = {"name":706,"dsp":900, "bram":1060,"bw":12.8}  # 150Mhz, 54bytes/cycle total
    platform_zc102 = {"name":102,"dsp":2520,"bram":1783,"bw":21.3}  # 200Mhz, 85bytes/cycle total
    platform_zc104 = {"name":104,"dsp":1728,"bram":2111,"bw":21}    # 200Mhz, 84bytes/cycle total
    tr = 7
    comp_mode = 1
    #dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn =  4, 2,4,4,2
    #dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn = 16,8,2,2,8
    #dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn = 32,32,8,8,32
    dw_tn,pw1_tm,pw1_tn, = 8,2,4
    pw2_tm,pw2_tn = pw1_tn, pw1_tm
    #alloc_layer = np.array( [ [256, 256, 7, 'k3_e4', 1,8], [256, 256, 7, 'k3_e4', 1,8] ] )
    #alloc_layer = np.array( [ [16,16,7,'k7_e4',2,8],[16,32,7,'k7_e6',2,8] ] )

    #alloc_layer = np.array( [ [16,16,14,'k3_e4',2,8],[16,16,14,'k3_e6',2,8] ] )
    alloc_layer = np.array( [ [32, 32, 28, 'k7_e6', 1,8] ])
    block_parameter = np.array([tr,dw_tn,pw1_tm,pw1_tn,pw2_tm,pw2_tn])
    bt = time.time()
    (bpu_dsp,bpu_bram,bpu_bw),layer_wise_lat = BPU_modeling(block_parameter,alloc_layer,comp_mode=comp_mode)
    et = time.time()
    print("Mode-{0}, consuming {1:.5f}s".format( comp_mode,et-bt ))
    print('BRAM18K: {}, DSP:{}, BW(Bytes/cycle):{}'.format(bpu_bram,bpu_dsp,bpu_bw))
    print("layer latency: (cycles)",layer_wise_lat)


if __name__=="__main__":
    Test()