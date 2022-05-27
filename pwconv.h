#pragma once
#include <ap_fixed.h>
#include<iostream>

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit>
void CONV1x1(ap_uint<Abit> in[Tn][Tr][Tc],ap_uint<Wbit> weights[Tm][Tn],ap_int<Accbit> out[Tm][Tr][Tc],
             unsigned n,bool relu){
#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=0 complete

#ifdef DEBUG
	std::cout<<"=======PE:PW1 Weight========"<<std::endl;
	for(unsigned tm = 0; tm < Tm; tm++) {
		for(unsigned tn = 0; tn < Tn; tn++) {
			ap_int<Wbit> tmp;
			tmp.range(Wbit-1,0) = weights[tm][tn].range(Wbit-1,0);
			std::cout<<tmp<<" ";
		}
        std::cout<<std::endl;
	}
#endif

#ifdef DEBUG
	std::cout<<"=======PE:PW1 Input========"<<std::endl;
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

    PW1_Tr:
    for(unsigned r=0;r<Tr;r++){
        PW_Tc:
        for(unsigned c=0;c<Tc;c++){
#pragma HLS DEPENDENCE false intra variable=out
#pragma HLS DEPENDENCE false inter variable=out
#pragma HLS PIPELINE II=1
            for(unsigned tm=0; tm< Tm; tm++){
#pragma HLS UNROLL
                for(unsigned tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
                    if(n==0 && tn ==0){
                        out[tm][r][c] = in[tn][r][c]*weights[tm][tn];
                    }else{
                        out[tm][r][c] += in[tn][r][c]*weights[tm][tn];
                    }
                }
            }
        }
    }
#ifdef DEBUG
	std::cout<<"=======PE:PW1 Output========"<<std::endl;
	for(unsigned tm = 0; tm< Tm; tm++) {
		for(unsigned r = 0; r < Tr; r++) {
			for(unsigned c = 0; c < Tc; c++) {
				ap_int<Abit> tmp = out[tm][r][c].range(Abit-1,0);
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif
}


template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,signed Size>
void CONV1x1_acc(ap_uint<Abit> in[Tn][Tr][Tc],ap_uint<Wbit> weights[Tm][Tn],ap_int<Accbit> out[Tm][Size],
             unsigned n, unsigned short offset){
#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=0 complete

#ifdef DEBUG

	std::cout<<"\n=======PE:PW2 Weight========"<<std::endl;
	for(unsigned tm = 0; tm < Tm; tm++) {
		for(unsigned tn = 0; tn < Tn; tn++) {
			ap_int<Wbit> tmp;
			tmp.range(Wbit-1,0) = weights[tm][tn].range(Wbit-1,0);
			std::cout<<tmp<<" ";
		}
        std::cout<<std::endl;
	}
#endif

#ifdef DEBUG
	std::cout<<"=======PE:PW2 Input========"<<std::endl;
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

    PW_Tr:
    for(unsigned r=0;r<Tr;r++){
        PW_Tc:
        for(unsigned c=0;c<Tc;c++){
#pragma HLS DEPENDENCE false intra variable=out
#pragma HLS DEPENDENCE false inter variable=out
#pragma HLS PIPELINE II=1
            for(unsigned tm=0; tm< Tm; tm++){
#pragma HLS UNROLL
                for(unsigned tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
                    if(n==0 && tn ==0){
                        out[tm][r*Tc+c+offset] = in[tn][r][c]*weights[tm][tn];
                    }else{
                        out[tm][r*Tc+c+offset] += in[tn][r][c]*weights[tm][tn];
                    }
                }
            }
        }
    }
#ifdef DEBUG
	std::cout<<"=======PE:PW2 Output========"<<std::endl;
	for(unsigned tm = 0; tm< Tm; tm++) {
		for(unsigned r = 0; r < Tr; r++) {
			for(unsigned c = 0; c < Tc; c++) {
				ap_int<Abit> tmp = out[tm][r*Tc+c+offset].range(Abit-1,0);
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif
}
