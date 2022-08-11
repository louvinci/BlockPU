#pragma once
#include <ap_fixed.h>
#include<iostream>

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned ABit,unsigned WBit,unsigned AccBit>
void  DW_CONV7x7(ap_int<ABit>   in[Tn][Tr+6][Tc+6],
				 ap_int<AccBit> out[Tn][Tr][Tc],
			     ap_int<WBit>   weights[Tn][7][7])
{
//#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=1 complete

#ifdef DEBUG
	std::cout<<"=======PE:DW Input========"<<std::endl;
	for(unsigned tn = 0; tn < Tn; tn++) {
		for(unsigned r = 0; r < Tr+6; r++) {
			for(unsigned c = 0; c < Tc+6; c++) {
				ap_int<ABit> tmp;
				tmp.range(ABit-1,0)= in[tn][r][c].range(ABit-1,0);
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif


	unsigned short i=0,j=0, h=0,w=0,cnt2=0;
	DW_LP:
	for(unsigned short cnt=0;cnt<49*Tr*Tc;cnt++){
#pragma HLS pipeline II=1
		for(unsigned char co = 0; co < Tn; co++){
#pragma HLS UNROLL
			if( cnt2==0){//optimization instead of i==0 and j==0
				out[co][h][w] = (weights[co][i][j] * in[co][h+i][w+j]);
			}else{
				out[co][h][w] += (weights[co][i][j] * in[co][h+i][w+j]);

			}

		}
		if(cnt2==48){
			cnt2=0;
		}else{
			cnt2+=1;
		}
		if (j==6){
			j=0;
			if(i == 6){
				i=0;
				if(w==Tc-1){
					w=0;
					h+=1;
				}else{
					w+=1;
				}
			}else{
			    i+=1;
			}
		}else{
			j+=1;
		}
	}


}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned ABit,unsigned WBit,unsigned AccBit>
void  DW_CONV3x3(   ap_int<ABit>   in[Tn][Tr+6][Tc+6],
				    ap_int<AccBit> out[Tn][Tr][Tc],
			        ap_int<WBit>   weights[Tn][7][7])
{
//#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=1 complete

	unsigned short i=0,j=0, h=0,w=0,cnt2=0;
	DW_LP:
	for(unsigned short cnt=0;cnt<9*Tr*Tc;cnt++){
#pragma HLS pipeline II=1
		for(unsigned char co = 0; co < Tn; co++){
#pragma HLS UNROLL
			if( cnt2==0){//optimization instead of i==0 and j==0
				out[co][h][w] = (weights[co][i][j] * in[co][h+i][w+j]);
			}else{
				out[co][h][w] += (weights[co][i][j] * in[co][h+i][w+j]);

			}

		}
		if(cnt2==8){
			cnt2=0;
		}else{
			cnt2+=1;
		}
		if (j==2){
			j=0;
			if(i == 2){
				i=0;
				if(w==Tc-1){
					w=0;
					h+=1;
				}else{
					w+=1;
				}
			}else{
			    i+=1;
			}
		}else{
			j+=1;
		}
	}


}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned ABit,unsigned WBit,unsigned AccBit>
void  DW_CONV(      ap_int<ABit>   in[Tn][Tr+6][Tc+6],
				    ap_int<AccBit> out[Tn][Tr][Tc],
			        ap_int<WBit>   weights[Tn][7][7],
					unsigned Ksize
			)
{
#pragma HLS inline off
#pragma HLS array_partition variable=in dim=1 complete
#pragma HLS array_partition variable=out dim=1 complete
#pragma HLS array_partition variable=weights dim=1 complete
unsigned short kloops = Ksize*Ksize;
#ifdef DEBUG
	std::cout<<"=======PE:DW Input========"<<std::endl;
	for(unsigned tn = 0; tn < Tn; tn++) {
		for(unsigned r = 0; r < Tr+Ksize-1; r++) {
			for(unsigned c = 0; c < Tc+Ksize-1; c++) {
				ap_int<ABit> tmp;
				tmp.range(ABit-1,0)= in[tn][r][c].range(ABit-1,0);
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif
#ifdef DEBUG
	std::cout<<"=======PE:DW Weight========"<<std::endl;
	for(unsigned tn = 0; tn < Tn; tn++) {
		for(unsigned r = 0; r < Ksize; r++) {
			for(unsigned c = 0; c < Ksize; c++) {
				ap_int<ABit> tmp;
				tmp.range(ABit-1,0)= weights[tn][r][c].range(ABit-1,0);
				std::cout<<tmp<<" ";
			}
			std::cout<<std::endl;
		}
        std::cout<<std::endl;
	}
#endif
	unsigned short i=0,j=0, h=0,w=0,cnt2=0;
	DW_LP:
	for(unsigned short cnt=0;cnt<kloops*Tr*Tc;cnt++){
#pragma HLS pipeline II=1
		for(unsigned char co = 0; co < Tn; co++){
#pragma HLS UNROLL
			if( cnt2==0){//optimization instead of i==0 and j==0
				out[co][h][w] = (weights[co][i][j] * in[co][h+i][w+j]);
			}else{
				out[co][h][w] += (weights[co][i][j] * in[co][h+i][w+j]);

			}

		}
		if(cnt2==kloops-1){
			cnt2=0;
		}else{
			cnt2+=1;
		}
		if (j==Ksize-1){
			j=0;
			if(i == Ksize-1){
				i=0;
				if(w==Tc-1){
					w=0;
					h+=1;
				}else{
					w+=1;
				}
			}else{
			    i+=1;
			}
		}else{
			j+=1;
		}
	}


}

