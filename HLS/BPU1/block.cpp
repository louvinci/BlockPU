#include"block.h"
#include"pw_engine.h"
#include"dw_engine.h"
#include <iostream>
#include"quant_node.h"

template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned Obit,unsigned OutWidth,unsigned ScaleBit,unsigned IntBit,unsigned Size>
void QPW2DDR_Reduce( ap_int<Accbit>    buf[Tm][Size],
		             ap_uint<OutWidth> * branch_ddr,
			         ap_uint<OutWidth> * out_ddr,
			         unsigned row,unsigned col,unsigned M,unsigned C,
					 ap_uint<ScaleBit> rescale, ap_uint<ScaleBit> id_rescale,unsigned short PadM)
{

//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	const unsigned short BLOCK = Tr*Tc;

    const unsigned NUM= OutWidth/Abit;
    const unsigned tile = Tm/NUM;
    unsigned short FinalM = (PadM == 0)? M:PadM;
//    std::cout<<"FinalM: "<<FinalM<<"M: "<<M<<std::endl;
    unsigned char c=0,t=0,tm=0;
    static_assert(Tm % NUM == 0,"For ReduceWidth, InWidth mod OutWidth is not 0");
    unsigned short loops = M/Tm;
    unsigned offsetC  = FinalM/NUM;
    unsigned offsetR  = C*FinalM/NUM;
    unsigned off_addr =  row*offsetR +col*offsetC;


    PWDDR:
	for(unsigned r = 0; r < Tr; r++) {
		for(unsigned cnt2 =0;cnt2 < Tc*offsetC;cnt2++){//
#pragma HLS LOOP_TRIPCOUNT max=560 min=224
#pragma HLS PIPELINE II=1 rewind
			ap_uint<OutWidth> DATA_o,DATA_i;
			ap_uint<OutWidth> * br_addr  = branch_ddr + off_addr + r*offsetR;
			ap_uint<OutWidth> * out_addr = out_ddr    + off_addr + r*offsetR;
			DATA_i = br_addr[cnt2];
			for(unsigned ii=0;ii<NUM;ii++){
#pragma HLS UNROLL
				ap_int<Abit>   identity  =  DATA_i.range((ii+1)*Abit-1, ii*Abit);
				ap_int<Accbit> pw2_acc   =  buf[tm*NUM+ii][(t%loops)*BLOCK+r*Tc+c];
				ap_int<Obit>   res       =  quant_add<Abit,Accbit,Obit,ScaleBit,IntBit>(identity,pw2_acc,rescale,id_rescale);
				DATA_o.range( (ii+1)*Obit-1, ii*Obit)=res.range(Obit-1, 0);
			}
			out_addr[cnt2]=DATA_o;
			if(tm == tile-1){
				tm = 0;
				if (t == FinalM/Tm-1){
					t=0;
					if(c == Tc-1){
						c=0;
					}else{
						c+=1;
					}
				}else{
					t+=1;
				}
			}else{
				tm+=1;
			}
		}

	}

}

void QBPU1(
						   ap_uint<InWidth1>      In_ddrsrc[MXR1*MXC1*MXN1/DWinNum1],
						   ap_uint<NormWidth1>    Normq_ddrsrc[NormSize1],
						   ap_uint<DwWidth1>      Wt7x7_ddrsrc[MXN1/DWwtNum1*7*7],
						   ap_uint<Pw1Width1>     Wt1_ddrsrc[MXM1*MXN1/PW1wtNum1],
						   ap_uint<Pw2Width1>     Wt2_ddrsrc[MXN1*MXM1/PW2wtNum1],
						   ap_uint<OutWidth1>     Branch_ddr[MXR1*MXC1*MXN1/PW2OutNum1],
						   ap_uint<OutWidth1>     Out_ddr[MXR1*MXC1*MXN1/PW2OutNum1],
						   ap_uint<ScaleBit1>     rescale1,
						   ap_uint<ScaleBit1>     rescale2,
						   ap_uint<ScaleBit1>     id_rescale,
						   unsigned   R1, unsigned C1,
						   unsigned   M1, unsigned N1, unsigned K1,unsigned PadN1

		)
{

#pragma HLS INTERFACE m_axi depth=25088   port=In_ddrsrc	    offset=slave	bundle=DWIN    latency=64     max_read_burst_length=64     num_read_outstanding=64     num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=1280    port=Normq_ddrsrc	    offset=slave	bundle=NORM    latency=64     max_read_burst_length=128    num_read_outstanding=8      num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=1568    port=Wt7x7_ddrsrc     offset=slave	bundle=DWWT    latency=64     max_read_burst_length=128    num_read_outstanding=16     num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=6144    port=Wt1_ddrsrc       offset=slave    bundle=PWWT    latency=48     max_read_burst_length=128    num_read_outstanding=16     num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=6144    port=Wt2_ddrsrc       offset=slave    bundle=PW2WT   latency=48     max_read_burst_length=128    num_read_outstanding=16     num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=25088   port=Out_ddr          offset=slave	bundle=OUTPUT  latency=64     max_write_burst_length=64    num_write_outstanding=16    num_read_outstanding=1
#pragma HLS INTERFACE m_axi depth=25088   port=Branch_ddr       offset=slave	bundle=BRANCH  latency=64     max_read_burst_length=64     num_read_outstanding=16     num_write_outstanding=1


#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=rescale1
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=rescale2
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=id_rescale
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=R1
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=C1
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=M1
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=N1
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=K1
#pragma HLS INTERFACE s_axilite  bundle=CONFIG port=PadN1
#pragma HLS INTERFACE s_axilite register port=return


ap_int<Abit1*DwTn1>  coldbuff0[SIZE1];// buffer the input
ap_int<Accbit1>      PWObuff0[PwTn1][SIZE2];

ap_int<Abit1*DwTn1>  coldbuff1[SIZE1];// buffer the input
ap_int<Accbit1>      PWObuff1[PwTn1][SIZE2];


unsigned loops = R1/Tr1*C1/Tc1;
unsigned row=0,pre_row=0,pre_row2=0;
unsigned col=0,pre_col=0,pre_col2=0;
bool flag=true;

    BPU_R:
    for(unsigned cnt=0; cnt<loops; cnt++){

    	if(cnt==0){
    		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit2,InWidth1,DwWidth1,BN_int1,SIZE1,NormWidth1,NormSize1>
    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,Normq_ddrsrc,coldbuff0,row,col,R1,C1,N1,K1,PadN1);
    	}else if(cnt == 1){

    		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit2,InWidth1,DwWidth1,BN_int1,SIZE1,NormWidth1,NormSize1>
    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,Normq_ddrsrc,coldbuff1,row,col,R1,C1,N1,K1,PadN1);

    	    PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,ScaleBit1,Scale_int1,SIZE1,SIZE2>
    					             (Wt1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1,rescale1);
    	}else{
    		if(flag){
        		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit2,InWidth1,DwWidth1,BN_int1,SIZE1,NormWidth1,NormSize1>
        		    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,Normq_ddrsrc,coldbuff0,row,col,R1,C1,N1,K1,PadN1);

        		PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,ScaleBit1,Scale_int1,SIZE1,SIZE2>
        		    					             (Wt1_ddrsrc,Wt2_ddrsrc,coldbuff1,PWObuff1,M1,N1,rescale1);
        		QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1,rescale2,id_rescale,PadN1);
        		flag = !flag;
    		}else{
        		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit2,InWidth1,DwWidth1,BN_int1,SIZE1,NormWidth1,NormSize1>
        		    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,Normq_ddrsrc,coldbuff1,row,col,R1,C1,N1,K1,PadN1);

        		PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,ScaleBit1,Scale_int1,SIZE1,SIZE2>
        		    					             (Wt1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1,rescale1);
        		QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff1,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1,rescale2,id_rescale,PadN1);
    			flag = !flag;

    		}
    	}
    	pre_row2 = pre_row;
    	pre_col2 = pre_col;
    	pre_row = row;
    	pre_col = col;
		if(col == C1-Tc1){
			col = 0;
			row+=Tr1;
		}else{
			col+=Tc1;
		}
    }

    if(loops > 1){
        if(flag){
        	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,ScaleBit1,Scale_int1,SIZE1,SIZE2>
        	        		    					             (Wt1_ddrsrc,Wt2_ddrsrc,coldbuff1,PWObuff1,M1,N1,rescale1);
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1,rescale2,id_rescale,PadN1);
        	flag = !flag;
        }else{
        	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,ScaleBit1,Scale_int1,SIZE1,SIZE2>
        	        		    					             (Wt1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1,rescale1);
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff1,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1,rescale2,id_rescale,PadN1);
            flag = !flag;
        }

        if(flag){
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,pre_row,pre_col,N1,C1,rescale2,id_rescale,PadN1);
        }else{
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff1,Branch_ddr,Out_ddr,pre_row,pre_col,N1,C1,rescale2,id_rescale,PadN1);
        }
    }else{
    	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,ScaleBit1,Scale_int1,SIZE1,SIZE2>
    	        	        		    					             (Wt1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1,rescale1);
    	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,Obit1,OutWidth1,ScaleBit1,Scale_int1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,0,0,N1,C1,rescale2,id_rescale,PadN1);
    }


}

