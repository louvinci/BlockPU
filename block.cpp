#include"block.h"
#include"pw_engine.h"
#include"dw_engine.h"
#include <iostream>

template <	unsigned Ibit,unsigned ScaleBit,unsigned Obit>
ap_int<Obit> quant_add(  ap_int<Ibit> in1,
                             ap_int<Ibit> in2,
							 ap_int<ScaleBit> scale1,
							 ap_int<ScaleBit> scale2
                             ) {

//const unsigned D = 1 << (W_BIT - 1 + DATA_BIT + L_SHIFT);
	const int Max = (1<<Obit)-1;
	const int Min = -(1<<Obit);
	ap_int<Ibit+ScaleBit+1> t_res = in1 *scale1 + in2*scale2 ;
	ap_int<Obit> res;

	if (t_res > Max) {
		res = Max;
	} else if (t_res < Min){
		res = Min;
	} else{
		res = t_res;
	}
	return res;

}

template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned OutWidth,unsigned Size>
void QPW2DDR_Reduce_old( ap_int<Accbit> buf[Tm][Size],
		             ap_uint<OutWidth> * br_ddr_burst,
			         ap_uint<OutWidth> * ddr_burst,
			 unsigned row,unsigned col,unsigned M,unsigned C)
{
	//[R1*C1*M1/Tm1], due to the completely buffer in the M dimmension, here the address is continuous in M dimmension
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	const unsigned short BLOCK = Tr*Tc;
    ap_uint<OutWidth> DATA_o,DATA_i;
    const unsigned NUM= OutWidth/Abit;
    const unsigned loops = Tm/NUM;
    static_assert(Tm % NUM == 0,"For ReduceWidth, InWidth mod OutWidth is not 0");
    unsigned offsetC = M/NUM;
    unsigned offsetR = C*M/NUM;
    unsigned addr = 0;
    PW_R:
	for(unsigned r = 0; r < Tr; r++) {
        PW_C:
		for(unsigned c = 0; c < Tc; c++) {
            PW_M:
			for(unsigned t=0;t < M/Tm; t++){//TODO Merge loop
				PW_P:
				for(unsigned tm = 0; tm < loops; tm++) {
	#pragma HLS PIPELINE II=1
					addr = (row+r)*offsetR + (col+c)*offsetC + t*loops+tm;
					DATA_i = br_ddr_burst[addr];
					for(unsigned ii=0;ii<NUM;ii++){
	#pragma HLS UNROLL
						ap_int<Abit> tmp1 = DATA_i.range((ii+1)*Abit-1, ii*Abit);
						ap_int<Abit> tmp2 = buf[tm*NUM+ii][t*BLOCK+r*Tc+c].range(Abit-1, 0);
						ap_int<Abit> res  = quant_add<Abit,ScaleBit1,Abit>(tmp1,tmp2,1,1);
						DATA_o.range( (ii+1)*Abit-1, ii*Abit)=res.range(Abit-1, 0);
					}
					ddr_burst[addr].range(OutWidth-1, 0)=DATA_o.range(OutWidth-1, 0);
				}
			}
		}
	}

}

//the address is continuous in M dimmension
template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned OutWidth,unsigned Size>
void QPW2DDR_Reduce_old2( ap_int<Accbit> buf[Tm][Size],
		             ap_uint<OutWidth> * branch_ddr,
			         ap_uint<OutWidth> * out_ddr,
			 unsigned row,unsigned col,unsigned M,unsigned C)
{

#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	const unsigned short BLOCK = Tr*Tc;
    ap_uint<OutWidth> DATA_o,DATA_i;
    const unsigned NUM= OutWidth/Abit;
    const unsigned tile = Tm/NUM;
    unsigned loops = BLOCK*M/NUM;


    unsigned char r=0,c=0,t=0,tm=0;
    static_assert(Tm % NUM == 0,"For ReduceWidth, InWidth mod OutWidth is not 0");
    unsigned offsetC  = M/NUM;
    unsigned offsetR  = C*M/NUM;
    unsigned off_addr =  row*offsetR +col*offsetC;
    unsigned addr = 0,cnt2=0;

    PWDDR:
	for(unsigned cnt = 0; cnt < loops; cnt++) {
#pragma HLS PIPELINE II=1

		//addr = off_addr + r*offsetR + c*offsetC + t*tile+tm;
		addr = off_addr + r*offsetR;
		ap_uint<OutWidth> * branch_brust = branch_ddr + addr;
		ap_uint<OutWidth> * out_brust = out_ddr + addr;

		DATA_i = branch_brust[cnt2];
		for(unsigned ii=0;ii<NUM;ii++){
#pragma HLS UNROLL
			ap_int<Abit> tmp1 = DATA_i.range((ii+1)*Abit-1, ii*Abit);
			ap_int<Abit> tmp2 = buf[tm*NUM+ii][t*BLOCK+r*Tc+c].range(Abit-1, 0);
			ap_int<Abit> res  = quant_add<Abit,ScaleBit1,Abit>(tmp1,tmp2,1,1);
			DATA_o.range( (ii+1)*Abit-1, ii*Abit)=res.range(Abit-1, 0);
		}
		out_brust[cnt2]=DATA_o;
		if(cnt2 == Tc*offsetC-1){
			cnt2 = 0;
		}else{
			cnt2+=1;
		}

		if(tm == tile-1){
				tm = 0;
				if (t == M/Tm-1){
					t=0;
					if(c == Tc-1){
						c=0;
						if(r == Tr-1){
							r = 0;
						}else{
							r+=1;
						}
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

//the address is continuous in M dimmension
template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned OutWidth,unsigned Size>
void QPW2DDR_Reduce( ap_int<Accbit> buf[Tm][Size],
		             ap_uint<OutWidth> * branch_ddr,
			         ap_uint<OutWidth> * out_ddr,
			 unsigned row,unsigned col,unsigned M,unsigned C)
{

#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	const unsigned short BLOCK = Tr*Tc;
    ap_uint<OutWidth> DATA_o,DATA_i;
    const unsigned NUM= OutWidth/Abit;
    const unsigned tile = Tm/NUM;
    unsigned loops = BLOCK*M/NUM;


    unsigned char c=0,t=0,tm=0;
    static_assert(Tm % NUM == 0,"For ReduceWidth, InWidth mod OutWidth is not 0");
    unsigned offsetC  = M/NUM;
    unsigned offsetR  = C*M/NUM;
    unsigned off_addr =  row*offsetR +col*offsetC;
    unsigned addr = 0;

    PWDDR:
	for(unsigned char r = 0; r < Tr; r++) {
		addr = off_addr + r*offsetR;
		ap_uint<OutWidth> * branch_brust = branch_ddr + addr;
		ap_uint<OutWidth> * out_brust = out_ddr + addr;

		for(unsigned cnt2 =0;cnt2 < Tc*offsetC;cnt2++){
#pragma HLS PIPELINE II=1 rewind
			DATA_i = branch_brust[cnt2];
			for(unsigned ii=0;ii<NUM;ii++){
#pragma HLS UNROLL
				ap_int<Abit> tmp1 = DATA_i.range((ii+1)*Abit-1, ii*Abit);
				ap_int<Abit> tmp2 = buf[tm*NUM+ii][t*BLOCK+r*Tc+c].range(Abit-1, 0);
				ap_int<Abit> res  = quant_add<Abit,ScaleBit1,Abit>(tmp1,tmp2,1,1);
				DATA_o.range( (ii+1)*Abit-1, ii*Abit)=res.range(Abit-1, 0);
			}
			out_brust[cnt2]=DATA_o;
			if(tm == tile-1){
				tm = 0;
				if (t == M/Tm-1){
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


void FusedDW_PW_InMode(
						   ap_uint<InWidth1> In_ddrsrc[R1*C1*N1/DWinNum1],
						   ap_uint<DwWidth1> Wt7x7_ddrsrc[N1/DWwtNum1*7*7],
						   ap_uint<Pw1Width1> Wt1x1_ddrsrc[M1*N1/PW1wtNum1],
						   ap_uint<Pw2Width1> Wt2_ddrsrc[N1*M1/PW2wtNum1],
						   ap_uint<OutWidth1> Branch_ddr[R1*C1*N1/PW2OutNum1],
						   ap_uint<OutWidth1> Out_ddr[R1*C1*N1/PW2OutNum1]

		)
{
#pragma HLS INTERFACE m_axi depth=25088   port=In_ddrsrc	offset=slave	bundle=DWIN latency=64 max_read_burst_length=32 num_read_outstanding=16 num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=1568    port=Wt7x7_ddrsrc offset=slave	bundle=DWWT latency=64 max_read_burst_length=64 num_read_outstanding=16 num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=6144    port=Wt1x1_ddrsrc offset=slave    bundle=PWWT latency=48 max_read_burst_length=64 num_read_outstanding=32 num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=6144    port=Wt2_ddrsrc   offset=slave    bundle=PW2WT latency=48 max_read_burst_length=64 num_read_outstanding=32 num_write_outstanding=1
#pragma HLS INTERFACE m_axi depth=25088   port=Out_ddr      offset=slave	bundle=OUTPUT latency=64 max_write_burst_length=32 num_write_outstanding=16 num_read_outstanding=1
#pragma HLS INTERFACE m_axi depth=25088   port=Branch_ddr   offset=slave	bundle=BRANCH latency=64 max_read_burst_length=32 num_read_outstanding=16 num_write_outstanding=1

//#pragma HLS INTERFACE m_axi depth=25088   port=In_ddrsrc	offset=slave	bundle=DWIN latency=64 max_read_burst_length=32
//#pragma HLS INTERFACE m_axi depth=1568    port=Wt7x7_ddrsrc offset=slave	bundle=DWWT latency=64 max_read_burst_length=32
//#pragma HLS INTERFACE m_axi depth=6144    port=Wt1x1_ddrsrc offset=slave    bundle=PWWT latency=64 max_read_burst_length=32
//#pragma HLS INTERFACE m_axi depth=6144    port=Wt2_ddrsrc   offset=slave    bundle=PW2WT latency=64 max_read_burst_length=32
//#pragma HLS INTERFACE m_axi depth=25088   port=Out_ddr      offset=slave	bundle=OUTPUT latency=64 max_write_burst_length=32
//#pragma HLS INTERFACE m_axi depth=25088   port=Branch_ddr   offset=slave	bundle=BRANCH latency=64 max_read_burst_length=32

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
        		QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1);
        		flag = !flag;
    		}else{
        		DW_Engine<Tr1,Tc1,DwTn1,Abit1,Wbit1,Accbit1,InWidth1,DwWidth1,SIZE1>
        		    		     		    		(In_ddrsrc,Wt7x7_ddrsrc,coldbuff1,row,col,R1,C1,N1);

        		PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);
        		QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff1,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1);
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
        	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
        	        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff1,PWObuff1,M1,N1);
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1);
        	flag = !flag;
        }else{
        	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
        	        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff1,Branch_ddr,Out_ddr,pre_row2,pre_col2,N1,C1);
            flag = !flag;
        }

        if(flag){
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,pre_row,pre_col,N1,C1);
        }else{
        	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff1,Branch_ddr,Out_ddr,pre_row,pre_col,N1,C1);
        }
    }else{
    	PW_PW_Fused<Tr1,Tc1,DwTn1,PwTn1,Tm1,Abit1,Wbit1,Accbit1,Pw1Width1,Pw2Width1,SIZE1,SIZE2>
    	        	        		    					             (Wt1x1_ddrsrc,Wt2_ddrsrc,coldbuff0,PWObuff0,M1,N1);
    	QPW2DDR_Reduce<PwTn1,Tr1,Tc1,Abit1,Accbit1,OutWidth1,SIZE2>(PWObuff0,Branch_ddr,Out_ddr,0,0,N1,C1);
    }


}

