#include"dwconv.h"

// InWidth/Abit >= Tn,will waist some data when InWidth/Abit > Tn
template<unsigned Tn,unsigned Tr,unsigned Tc,unsigned Abit,unsigned InWidth>
void reduceload_axi( ap_uint<InWidth>  *ddr_burst,
					 ap_uint<Abit>      buf[Tn][Tr+6][Tc+6],
					unsigned ch,  unsigned row,unsigned col,
					unsigned  offsetR, unsigned offsetC,unsigned R,unsigned C)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
    //row * S - P, j < (row + Tr - 1) * S + K - P
	//row+r-3 <0 | row+r-3>=R | col +c-3<0 | col+c-3>=C
    ap_uint<InWidth> DATA;
    RA_R:
	for(unsigned r = 0; r < Tr+6; r++) {
		RA_C:
        for(unsigned c = 0; c < Tc+6; c++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1

            if (row+r<3 | row+r> R+2 | col +c<3 | col+c>C+2){
                for(int tn = 0; tn < Tn; tn++) {
                #pragma HLS UNROLL
				    buf[tn][r][c].range(Abit-1, 0) = 0;
			}
            }else{
            	//unsigned index = (row+r-3)*offsetR + (col+c-3)*offsetC + ch;
            	//std::cout<<"  index "<<index<<" ";
                DATA.range(InWidth-1, 0) = ddr_burst[(row+r-3)*offsetR + (col+c-3)*offsetC + ch].range(InWidth-1, 0);
                for(unsigned tn = 0; tn < Tn; tn++) {
                #pragma HLS UNROLL
				    buf[tn][r][c].range(Abit-1, 0) = DATA.range( (tn+1)*Abit-1, tn*Abit);
			    }
            }
            //std::cout<<std::endl;
		}
        //std::cout<<std::endl;
	}
}

template<unsigned Tn,unsigned Wbit,unsigned WtWidth>
void loadwt7x7_axi( ap_uint<WtWidth> *ddr_burst,
							 ap_uint<Wbit> buf[Tn][7][7],unsigned offset)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
    ap_uint<WtWidth> DATA;

	unsigned r=0,c=0;
    LW7_L:
	for(unsigned cnt = 0; cnt < 49; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		DATA.range(WtWidth-1, 0) = ddr_burst[offset*49+r*7+c].range(WtWidth-1, 0);
		for(unsigned tn = 0; tn < Tn; tn++) {
#pragma HLS UNROLL
			buf[tn][r][c].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);
		}
		if (c==6){
			c=0;
			r+=1;
		}else{
			c+=1;
		}

	}
}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Abit,unsigned Accbit, unsigned Size>
void Q2ColdBuffer(ap_int<Accbit> src[Tn][Tr][Tc],ap_uint<Abit*Tn> dst[Size],unsigned offset){
#pragma HLS inline off
//#pragma HLS array_partition variable=dst dim=1 complete
#pragma HLS array_partition variable=src dim=1 complete

    FillB:
    for(unsigned r=0;r<Tr;r++){
        for(unsigned c=0;c<Tc;c++){
#pragma HLS PIPELINE II=1
            for(unsigned tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
                dst[offset +r*Tc+c].range((tn+1)*Abit-1,tn*Abit) = src[tn][r][c].range(Abit-1,0);//TODO just clip,plan to add quant operation
//                ap_int<Abit> tmp =  src[tn][r][c].range(Abit-1,0);
//                std::cout<<tmp<<" ";
            }
            //std::cout<<std::endl;
        }
        //std::cout<<std::endl;
    }
}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Abit,unsigned Wbit,unsigned Accbit,
         unsigned InWidth,unsigned WtWidth,unsigned Size>
void DW_Engine(ap_uint<InWidth> *In_ddrsrc,
		       ap_uint<WtWidth> *Wt7x7_ddrsrc,
			   ap_uint<Abit*Tn> coldbuff[Size],
			   unsigned row,unsigned col,unsigned R, unsigned C,unsigned N){

#pragma HLS inline off
	ap_uint<Abit>   Abuff0[Tn][Tr+6][Tc+6];
	ap_int<Accbit>  Obuff0[Tn][Tr][Tc];
	ap_uint<Wbit>   Wbuff0[Tn][7][7];

	ap_uint<Abit>   Abuff1[Tn][Tr+6][Tc+6];
	ap_int<Accbit>  Obuff1[Tn][Tr][Tc];
	ap_uint<Wbit>   Wbuff1[Tn][7][7];
	unsigned loops=N/Tn; //TODO offset should be InWidth / Abit
	bool flag=true;
	{
	    reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff0,0,row,col,C*loops,loops,R,C);
	    loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff0,0);
	}
	{
		reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff1,1,row,col,C*loops,loops,R,C);
		loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff1,1);
		DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,0);
	}
    DWEG1:
    for(unsigned n=2; n<loops; n++){
		if(flag){
			reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff0,n,row,col,C*loops,loops,R,C);
			loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff0,n);
			DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff1,Obuff1,Wbuff1,0);
			Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff0,coldbuff,(n-2)*Tr*Tc);
			flag=!flag;
		}else{
			reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff1,n,row,col,C*loops,loops,R,C);
			loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff1,n);
			DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,0);
			Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff1,coldbuff,(n-2)*Tr*Tc);
			flag=!flag;
		}
    }

	if(flag){
		DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff1,Obuff1,Wbuff1,0);
		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff0,coldbuff,(loops-2)*Tr*Tc);
		flag=!flag;
	}else{
		DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,0);
		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff1,coldbuff,(loops-2)*Tr*Tc);
		flag=!flag;
	}
	if(flag){
		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff0,coldbuff,(loops-1)*Tr*Tc);
	}else{
		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff1,coldbuff,(loops-1)*Tr*Tc);
	}
}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Abit,unsigned Wbit,unsigned Accbit,
         unsigned InWidth,unsigned WtWidth,unsigned Size>
void DW_Engine_DATAFLOW(ap_uint<InWidth> *In_ddrsrc,
		       ap_uint<WtWidth> (*Wt7x7_ddrsrc)[7][7],
			   ap_uint<Abit*Tn> coldbuff[Size],
			   unsigned row,unsigned col,unsigned R, unsigned C,unsigned N){

#pragma HLS inline off
	ap_uint<Abit>  Abuff0[Tn][Tr+6][Tc+6];
	ap_int<Accbit> Obuff0[Tn][Tr][Tc];
	ap_uint<Wbit>  Wbuff0[Tn][7][7];
	unsigned loops=N/Tn;
    DWEG1:
    for(unsigned n=0; n<loops; n++){//TODO optimization:dataflow
#pragma HLS dataflow// temp buffer need be defined inside the function.
        reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff0,n,row,col,C*loops,loops,R,C);
        loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc[n],Wbuff0);
        DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,0);
        Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,Size>(Obuff0,coldbuff,n*Tr*Tc);
    }

}
