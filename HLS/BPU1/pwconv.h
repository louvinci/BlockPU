#pragma once
#include <ap_fixed.h>
#include<iostream>

ap_int<32> MUL_INT8(ap_int<8> A, ap_int<8> W0, ap_int<8> W1)
{
    //ap_int<24> W;
    //W = (W0, ap_uint<16>(0)) + ap_int<24>(W1);
    ap_int<25> W= W0;
    W <<= 16;
    W+=W1;
	
    ap_int<16> r0;
    ap_int<16> r1;

    (r0, r1) = A*W;

    r0 = r0+r1[16-1];

    return (r0,r1);
}
template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit>
void CONV1x1_OP1(ap_int<Abit> in[Tn][PRELOAD][Tr][Tc],ap_int<Wbit> weights[PRELOAD][Tm][Tn],ap_int<Accbit> out[Tm][Tr][Tc],
             unsigned short n){
//#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=2 complete
#pragma HLS array_partition variable=weights dim=3 complete
	unsigned char r=0,c=0;
	unsigned char index=0;
	const unsigned loops = PRELOAD*Tr*Tc;
    PW1_TrTc:
    for(unsigned cnt=0;cnt<loops;cnt++){
#pragma HLS DEPENDENCE false intra variable=out
#pragma HLS DEPENDENCE false inter variable=out
#pragma HLS PIPELINE II=1

            for(unsigned char tm=0; tm< Tm; tm++){
#pragma HLS UNROLL
                for(unsigned char tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
                    if(index==0 && n==0 && tn ==0 ){
                        out[tm][r][c] = in[tn][index][r][c]*weights[index][tm][tn];
                    }else{
                        out[tm][r][c] += in[tn][index][r][c]*weights[index][tm][tn];
                    }
                }
            }
        if(c == Tc-1){
        	c=0;
        	if(r == Tr-1){
        		r = 0;
        		index += 1;
        	}else{
        		r+=1;
        	}

        }else{
        	c+=1;
        }
    }


}
//TODO the partition size is larger than the need
template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit>
void CONV1x1_OP(ap_int<Abit*Tn> in[PRELOAD][Tr][Tc],ap_int<Wbit> weights[PRELOAD][Tm][Tn],ap_int<Accbit> out[Tm][Tr][Tc],
             unsigned short n){
//#pragma HLS inline off
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=2 complete
#pragma HLS array_partition variable=weights dim=3 complete

#ifdef DEBUG
	std::cout<<"=======PE:PW1 Weight========"<<std::endl;
	for(unsigned tm = 0; tm < Tm; tm++) {
		for(unsigned tn = 0; tn < PRELOAD*Tn; tn++) {
			ap_int<Wbit> tmp;
			tmp.range(Wbit-1,0) = weights[tn/Tn][tm][tn%Tn].range(Wbit-1,0);
			std::cout<<tmp<<" ";
		}
        std::cout<<std::endl;
	}
#endif

#ifdef DEBUG
	std::cout<<"=======PE:PW1 Input========"<<std::endl;
	for(unsigned tn = 0; tn < PRELOAD*Tn; tn++) {
		for(unsigned r = 0; r < Tr; r++) {
			for(unsigned c = 0; c < Tc; c++) {
				ap_int<Abit> tmp;
				tmp= in[tn%Tn][tn/Tn][r][c];
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif
	unsigned char r=0,c=0;
	unsigned char index=0;
	const unsigned loops = PRELOAD*Tr*Tc;
    PW1_TrTc:
    for(unsigned cnt=0;cnt<loops;cnt++){
#pragma HLS DEPENDENCE false intra variable=out
#pragma HLS DEPENDENCE false inter variable=out
#pragma HLS PIPELINE II=1

            for(unsigned char tm=0; tm< Tm; tm+=2){
#pragma HLS UNROLL
                for(unsigned char tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
                	ap_int<32> tmp;
                	ap_int<16> tmp1,tmp2;
                    if(index==0 && n==0 && tn ==0 ){
                    	//tmp = MUL_INT8(in[tn][index][r][c],weights[index][tm][tn],weights[index][tm+1][tn]);
						tmp = MUL_INT8(in[index][r][c].range((tn+1)*Abit-1,tn*Abit),weights[index][tm][tn],weights[index][tm+1][tn]);
                        tmp1.range(15,0) = tmp.range(31,16);
                        tmp2.range(15,0) = tmp.range(15,0);
                        out[tm][r][c]   = ap_int<Accbit>(tmp1);
                        out[tm+1][r][c] = ap_int<Accbit>(tmp2);

                    }else{
						//tmp = MUL_INT8(in[tn][index][r][c],weights[index][tm][tn],weights[index][tm+1][tn]);
						tmp = MUL_INT8(in[index][r][c].range((tn+1)*Abit-1,tn*Abit),weights[index][tm][tn],weights[index][tm+1][tn]);
						tmp1.range(15,0) = tmp.range(31,16);
						tmp2.range(15,0) = tmp.range(15,0);
						out[tm][r][c]   += ap_int<Accbit>(tmp1);
						out[tm+1][r][c] += ap_int<Accbit>(tmp2);
                    }
                }
            }
        if(c == Tc-1){
        	c=0;
        	if(r == Tr-1){
        		r = 0;
        		index += 1;
        	}else{
        		r+=1;
        	}

        }else{
        	c+=1;
        }
    }
}



template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,signed Size>
void CONV1x1_acc_OP(ap_int<Abit*Tn> in[Tr][Tc],ap_int<Wbit> weights[PRELOAD][Tm][Tn],ap_int<Accbit> out[Tm][Size],
             unsigned short n, unsigned short offset){
//#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=2 complete
#pragma HLS array_partition variable=weights dim=3 complete

#ifdef DEBUG

	std::cout<<"\n=======PE:PW2_OP Weight========"<<std::endl;
	for(unsigned tm = 0; tm < PRELOAD*Tm; tm++) {
		for(unsigned tn = 0; tn < Tn; tn++) {
			ap_int<Wbit> tmp;
			tmp.range(Wbit-1,0) = weights[tm/Tm][tm%Tm][tn].range(Wbit-1,0);
			std::cout<<tmp<<" ";
		}
        std::cout<<std::endl;
	}
#endif

#ifdef DEBUG
	std::cout<<"=======PE:PW2_OP Input========"<<std::endl;
	for(unsigned tn = 0; tn < Tn; tn++) {
		for(unsigned r = 0; r < Tr; r++) {
			for(unsigned c = 0; c < Tc; c++) {
				ap_int<Abit> tmp;
				tmp.range(Abit-1,0) = in[tn][r][c].range(Abit-1,0);
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif
	unsigned short r=0,c=0;
	unsigned char index = 0;
	const unsigned TRC = Tr*Tc;
	unsigned addr=0;
    PW_TrTc:
    for(unsigned short cnt=0;cnt<PRELOAD*TRC;cnt++){
#pragma HLS DEPENDENCE false intra variable=out
#pragma HLS DEPENDENCE false inter variable=out
#pragma HLS PIPELINE II=1
    	addr =  offset*PRELOAD + index*TRC + r*Tc+c ;
		for(unsigned short tm=0; tm< Tm; tm+=2){
#pragma HLS UNROLL
			for(unsigned char tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
            	ap_int<32> tmp;
            	ap_int<16> tmp1,tmp2;
				if(n==0 && tn ==0){
					//out[tm][addr] = in[tn][r][c]*weights[index][tm][tn];
                	tmp = MUL_INT8(in[r][c].range((tn+1)*Abit-1,tn*Abit),weights[index][tm][tn],weights[index][tm+1][tn]);
                    tmp1.range(15,0) = tmp.range(31,16);
                    tmp2.range(15,0) = tmp.range(15,0);
                    out[tm][addr]    = ap_int<32>(tmp1);
                    out[tm+1][addr]  = ap_int<32>(tmp2);
				}else{
					//out[tm][addr] += in[tn][r][c]*weights[index][tm][tn];
                	//tmp = MUL_INT8(in[tn][r][c],weights[index][tm][tn],weights[index][tm+1][tn]);
					tmp = MUL_INT8(in[r][c].range((tn+1)*Abit-1,tn*Abit),weights[index][tm][tn],weights[index][tm+1][tn]);
                    tmp1.range(15,0) = tmp.range(31,16);
                    tmp2.range(15,0) = tmp.range(15,0);
                    out[tm][addr]    += ap_int<32>(tmp1);
                    out[tm+1][addr]  += ap_int<32>(tmp2);
				}
			}
		}
        if(c==Tc-1){
    	     c = 0;
    	     if(r == Tr-1){
    	    	 r=0;
    	    	 index += 1;
    	     }else{
    	    	 r+=1;
    	     }
        }else{
        	c+=1;
        }
    }
}
