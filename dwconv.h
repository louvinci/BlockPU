#pragma once
#include <ap_fixed.h>
#include<iostream>

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned ABit,unsigned WBit,unsigned AccBit>
void  DW_CONV7x7(ap_uint<ABit>  in[Tn][Tr+6][Tc+6],
				 ap_int<AccBit> out[Tn][Tr][Tc],
			     ap_uint<WBit>  weights[Tn][7][7],
			 	 bool norm)
{
#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=1 complete

#ifdef DEBUG
	std::cout<<"=======PE:DW Input========"<<std::endl;
	for(unsigned tn = 0; tn < Tn; tn++) {
		for(unsigned r = 0; r < Tr+6; r++) {
			for(unsigned c = 0; c < Tc+6; c++) {
				ap_int<ABit> tmp = in[tn][r][c];
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif

	DW_Tr:
	for(unsigned h = 0; h < Tr; h++){
		DW_Tc:
		for(unsigned  w = 0; w < Tc; w++){
			unsigned i=0,j=0;
			DW_K:
			for(unsigned cnt=0;cnt<49;cnt++){//TODO optimization:loop merge
#pragma HLS pipeline II=1
				for(unsigned co = 0; co < Tn; co++){
#pragma HLS UNROLL
					if(cnt == 0){//optimization instead of i==0 and j==0
						out[co][h][w] = (weights[co][i][j] * in[co][h+i][w+j]);
					}else{
						out[co][h][w] += (weights[co][i][j] * in[co][h+i][w+j]);

					}

				}
				if (j==6){
					j=0;
					i+=1;
				}else{
					j+=1;
				}
			}
		}
	}

}
