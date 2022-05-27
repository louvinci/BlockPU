#include"block.h"
#include"pw_engine.h"
#include"dw_engine.h"
#include <iostream>


template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned OutWidth,unsigned Size>
void QPW2DDR_Reduce( ap_int<Accbit> buf[Tm][Size],
			     ap_uint<OutWidth> * ddr_burst,
			 unsigned row,unsigned col,unsigned M,unsigned C)
{
	//[R1*C1*M1/Tm1], due to the completely buffer in the M dimmension, here the address is continuous in M dimmension
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	const unsigned short BLOCK = Tr*Tc;
    ap_uint<OutWidth> DATA;
    const unsigned NUM= OutWidth/Abit;
    const unsigned loops = Tm/NUM;
    static_assert(Tm % NUM == 0,"For ReduceWidth, InWidth mod OutWidth is not 0");
    unsigned offsetC = M/NUM;
    unsigned offsetR = C*M/NUM;


    PW_R:
	for(unsigned r = 0; r < Tr; r++) {
        PW_C:
		for(unsigned c = 0; c < Tc; c++) {
            PW_M:
			for(unsigned t=0;t < M/Tm; t++){//TODO Merge loop
				PW_P:
				for(unsigned tm = 0; tm < loops; tm++) {
	#pragma HLS PIPELINE II=1
					for(unsigned ii=0;ii<NUM;ii++){
	#pragma HLS UNROLL
						DATA.range( (ii+1)*Abit-1, ii*Abit)=buf[tm*NUM+ii][t*BLOCK+r*Tc+c].range(Abit-1, 0); //TODO,direct clip, plan to add quant operation
					}
					ddr_burst[(row+r)*offsetR + (col+c)*offsetC + t*loops+tm].range(OutWidth-1, 0)=DATA.range(OutWidth-1, 0);
				}
			}
		}
	}

}




void FusedDW_PW_InMode(ap_uint<InWidth1>  In_ddrsrc[R1*C1*N1/DwTn1],
		               ap_uint<DwWidth1>  Wt7x7_ddrsrc[N1/DwTn1*7*7],
		               ap_uint<Pw1Width1> Wt1x1_ddrsrc[M1*N1/PwTn1],
					   ap_uint<Pw2Width1> Wt2_ddrsrc[N1*M1/Tm1],
                       ap_uint<OutWidth1> Out_ddr[R1*C1*N1/PwTn1])
{
#pragma HLS INTERFACE m_axi depth=6272    port=In_ddrsrc	offset=slave	bundle=DWIN latency=64 max_read_burst_length=32 num_read_outstanding=16
#pragma HLS INTERFACE m_axi depth=3136	  port=Wt7x7_ddrsrc offset=slave	bundle=DWWT latency=64 max_read_burst_length=32 num_read_outstanding=16
#pragma HLS INTERFACE m_axi depth=1024    port=Wt1x1_ddrsrc offset=slave    bundle=PWWT latency=64 max_read_burst_length=32 num_read_outstanding=16
#pragma HLS INTERFACE m_axi depth=1024    port=Wt2_ddrsrc   offset=slave    bundle=PW2WT latency=64 max_read_burst_length=32 num_read_outstanding=16
#pragma HLS INTERFACE m_axi depth=6272	  port=Out_ddr      offset=slave	bundle=OUTPUT latency=64 max_write_burst_length=32 num_write_outstanding=16

#pragma HLS INTERFACE s_axilite register	port=return


ap_uint<Abit1*DwTn1> coldbuff0[SIZE1];// buffer the input
ap_int<Accbit1> PWObuff0[PwTn1][SIZE2];

ap_uint<Abit1*DwTn1> coldbuff1[SIZE1];// buffer the input
ap_int<Accbit1> PWObuff1[PwTn1][SIZE2];


unsigned loops = R1/Tr1*C1/Tc1;
unsigned row=0,pre_row=0,pre_row2=0;
unsigned col=0,pre_col=0,pre_col2=0;
bool flag=true;

    BPU_R:
    for(unsigned cnt=0; cnt<loops; cnt++){
    	if(cnt==0){
    		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit1,InWidth1,DwWidth1,SIZE1>
    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,coldbuff0,row,col,R1,C1,N1);
    	}else if(cnt == 1){

    		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit1,InWidth1,DwWidth1,SIZE1>
    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,coldbuff1,row,col,R1,C1,N1);

    	    PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);

    	}else{
    		if(flag){
        		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit1,InWidth1,DwWidth1,SIZE1>
        		    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,coldbuff0,row,col,R1,C1,N1);

        		PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff1,PWObuff1,M1,N1);
        		QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Out_ddr,pre_row2,pre_col2,N1,C1);
        		flag = !flag;
    		}else{
        		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit1,InWidth1,DwWidth1,SIZE1>
        		    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,coldbuff1,row,col,R1,C1,N1);

        		PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);
        		QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff1,Out_ddr,pre_row2,pre_col2,N1,C1);
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

    if(flag){
    	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
    	        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff1,PWObuff1,M1,N1);
    	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Out_ddr,pre_row2,pre_col2,N1,C1);
    	flag = !flag;
    }else{
    	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
    	        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);
    	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff1,Out_ddr,pre_row2,pre_col2,N1,C1);
        flag = !flag;
    }

    if(flag){
    	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Out_ddr,pre_row,pre_col,N1,C1);
    }else{
    	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff1,Out_ddr,pre_row,pre_col,N1,C1);
    }

//	BlockR:
//	for(unsigned  rr=0;rr<R1;rr+=Tr1){
//		BlockC:
//		for(unsigned cc=0;cc<C1;cc+=Tc1){
//
//			DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit1,InWidth1,DwWidth1,SIZE1>
//     		    		(In_ddrsrc,Wt7x7_ddrsrc,coldbuff0,rr,cc,R1,C1,N1);
//
//			PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
//			             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);
//
//	        QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Out_ddr,rr,cc,N1,C1);
//		}
//	}


}

