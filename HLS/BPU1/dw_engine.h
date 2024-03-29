#include"dwconv.h"
#include"quant_node.h"
#include<string.h>

template<unsigned Tn,unsigned Tr,unsigned Tc,unsigned Abit,unsigned InWidth>
void reduceload_axi( ap_uint<InWidth>  *ddr_burst,
					 ap_int<Abit>      buf[Tn][Tr+6][Tc+6],
					unsigned short   ch,  unsigned short row,unsigned short col,
					unsigned offsetR, unsigned offsetC,unsigned short R,unsigned short C)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete

	static_assert(InWidth % Abit == 0,"DWIn bandwidth must be divisible by Abit");
	const unsigned short NUM = InWidth / Abit;
	const unsigned short tnloops = Tn/NUM;

	static_assert(Tn % NUM == 0,"DWIn bandwidth must be divisible by NUM");


    //j = row * S - P, j < (row + Tr - 1) * S + K - P
	//row+r-3 <0 | row+r-3>=R | col +c-3<0 | col+c-3>=C
	DwIN_R:
	for(unsigned short  r = 0; r < Tr+6; r++) {
		DwIN_C:
		for(unsigned short c = 0; c < Tc+6; c++) {
#pragma HLS PIPELINE II=8
			ap_uint<InWidth>  * brust_addr = ddr_burst + (row+r-3)*offsetR*tnloops + (col+c-3)*offsetC*tnloops + ch*tnloops;
			DwIN_P1:
			for(unsigned short tnn =0;tnn < tnloops;tnn++){
				ap_uint<InWidth> DATA;
				DATA = brust_addr[tnn];
				if (row+r<3 | row+r> R+2 | col +c<3 | col+c>C+2){
					for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
						buf[tnn*NUM+tn][r][c]= 0;
					}
				}else{
					for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
						buf[tnn*NUM+tn][r][c].range(Abit-1, 0) = DATA.range( (tn+1)*Abit-1, tn*Abit);
					}
				}
			}
			//memcpy(tmp_buf[r][c],brust_addr,tnloops*sizeof(InWidth));

		}
	}

}


template<unsigned Tn,unsigned Tr,unsigned Tc,unsigned Abit,unsigned InWidth>
void expandload3x3_axi( ap_uint<InWidth>  *ddr_burst,
					 ap_int<Abit>      buf[Tn][Tr+6][Tc+6],
					unsigned short ch,  unsigned short row,unsigned short col,
					unsigned offsetR,   unsigned  offsetC,unsigned short R,unsigned short C,unsigned short N)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete

	static_assert(InWidth % Abit == 0,"DWIn bandwidth must be divisible by Abit");
	const unsigned short NUM = InWidth / Abit;
	const unsigned short tnloops = 32/NUM; //N ==32
	const unsigned loops = (Tr+2)*(Tc+2);
	unsigned short r=0,c=0;

    //j = row * S - P, j < (row + Tr - 1) * S + K - P
	//row+r-3 <0 | row+r-3>=R | col +c-3<0 | col+c-3>=C
	Dw3I_R:
	for(unsigned short cnt = 0; cnt < loops; cnt++) {
#pragma HLS PIPELINE II=4
		ap_uint<InWidth>  * brust_addr = ddr_burst + (row+r-1)*offsetR*tnloops + (col+c-1)*offsetC*tnloops + ch*tnloops;
		Dw3I_P:
		for(unsigned short tnn =0;tnn < tnloops;tnn++){
			ap_uint<InWidth> DATA;
			DATA = brust_addr[tnn];
			if (row+r<1 | row+r> R | col + c<1 | col+c>C){
				for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
					buf[tnn*NUM+tn][r][c]= 0;
				}
			}else{

				for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
					buf[tnn*NUM+tn][r][c].range(Abit-1, 0) = DATA.range( (tn+1)*Abit-1, tn*Abit);
				}
			}
		}
		if(c == Tc+1){
			r+=1;
			c=0;
		}else{
			c+=1;
		}
	}

}

template<unsigned Tn,unsigned Wbit,unsigned WtWidth>
void expandloadwt3x3( ap_uint<WtWidth> *ddr_burst,
				      ap_int<Wbit> buf[Tn][7][7],unsigned short offset,unsigned short N)
{
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	static_assert( WtWidth%Wbit==0,"DWWt bandwidth must be divisible by Wbit");
	static_assert( Tn%(WtWidth/Wbit)==0,"DwTn must be divisible by WtNUM/cycle");
	const unsigned NUM = WtWidth / Wbit;
	unsigned tnloops = N/ NUM;
	const unsigned base1 = 9*tnloops;
	const unsigned base2 = 3*tnloops;


	ap_uint<WtWidth> * base_brust = ddr_burst + offset*base1;
    ap_uint<WtWidth> DATA;
	unsigned short r=0,c=0,tnn=0;

    LW7_L:
	for(unsigned short cnt = 0; cnt < 9*tnloops; cnt++) {
//#pragma HLS DEPENDENCE false intra variable=buf
//#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		DATA = base_brust[cnt];
		for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
			buf[tnn*NUM+tn][r][c].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);
		}

		if(tnn == tnloops -1){
			tnn=0;
			if (c==2){
				c=0;
				r+=1;
			}else{
				c+=1;
			}
		}else{
			tnn+=1;
		}

	}
}

template<unsigned Tn,unsigned Wbit,unsigned WtWidth>
void expandloadwt7x7( ap_uint<WtWidth> *ddr_burst,
				      ap_int<Wbit> buf[Tn][7][7],unsigned offset,unsigned N)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	static_assert( WtWidth%Wbit==0,"DWWt bandwidth must be divisible by Wbit");
	static_assert( Tn%(WtWidth/Wbit)==0,"DwTn must be divisible by WtNUM/cycle");
	const unsigned NUM = WtWidth / Wbit;
	unsigned tnloops = N/ NUM;
	const unsigned base1 = 49*tnloops;
	const unsigned base2 = 7*tnloops;


	ap_uint<WtWidth> * base_brust = ddr_burst + offset*base1;
    ap_uint<WtWidth> DATA;
	unsigned short r=0,c=0,tnn=0;

    LW7_L:
	for(unsigned cnt = 0; cnt < 49*tnloops; cnt++) {
//#pragma HLS DEPENDENCE false intra variable=buf
//#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		//DATA = ddr_burst[offset*base1+r*base2+c*tnloops+tnn]; this method is hard to infer for the synthezier
		DATA = base_brust[cnt];
		for(unsigned tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
			buf[tnn*NUM+tn][r][c].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);
		}

		if(tnn == tnloops -1){
			tnn=0;
			if (c==6){
				c=0;
				r+=1;
			}else{
				c+=1;
			}
		}else{
			tnn+=1;
		}

	}
}

template<unsigned NormWidth,unsigned NormSize>
void loadnormwt(ap_uint<NormWidth> *Normq_ddrsrc,ap_int<NormWidth> buf[NormSize],unsigned short N){
//#pragma HLS inline off
	for(unsigned short i=0;i<N;i++){
#pragma HLS PIPELINE II=1
		buf[i].range(NormWidth-1,0) = Normq_ddrsrc[i].range(NormWidth-1,0);
	}

}
template<unsigned Tn,unsigned Wbit,unsigned WtWidth>
void loadwt7x7_axi( ap_uint<WtWidth> *ddr_burst,
					ap_int<Wbit> buf[Tn][7][7],unsigned short offset)
{
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
	static_assert( WtWidth%Wbit==0,"DWWt bandwidth must be divisible by Wbit");
	static_assert( Tn%(WtWidth/Wbit)==0,"DwTn must be divisible by WtNUM/cycle");
	const unsigned NUM = WtWidth / Wbit;
	const unsigned tnloops = Tn/ NUM;
	const unsigned base1 = 49*tnloops;
	const unsigned base2 = 7*tnloops;


	ap_uint<WtWidth> * base_brust = ddr_burst + offset*base1;
    ap_uint<WtWidth> DATA;
	unsigned short r=0,c=0,tnn=0;

    LW7_L:
	for(unsigned short cnt = 0; cnt < 49*tnloops; cnt++) {
//#pragma HLS DEPENDENCE false intra variable=buf
//#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		//DATA = ddr_burst[offset*base1+r*base2+c*tnloops+tnn]; this method is hard to infer for the synthezier
		DATA = base_brust[cnt];
		for(unsigned short tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
			buf[tnn*NUM+tn][r][c].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);
		}

		if(tnn == tnloops -1){
			tnn=0;
			if (c==6){
				c=0;
				r+=1;
			}else{
				c+=1;
			}
		}else{
			tnn+=1;
		}

	}
}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Abit,unsigned Accbit, unsigned NormWidth,unsigned IntBit,unsigned Size,unsigned NormSize>
void Q2ColdBuffer(ap_int<Accbit> src[Tn][Tr][Tc],ap_int<Abit*Tn> dst[Size],ap_int<NormWidth> bn_buff[NormSize],unsigned short tile_n){
#pragma HLS inline off
#pragma HLS array_partition variable=src dim=1 complete
#pragma HLS ARRAY_PARTITION variable=bn_buff dim=1 factor=64 cyclic

	static_assert( NormWidth%2==0,"The bit of Norm QWeight and QBias is same");
	unsigned offset = tile_n*Tr*Tc;

	//std::cout<<"Tile in n: "<<tile_n<<std::endl;
    FillB:
    for(unsigned char r=0;r<Tr;r++){
        for(unsigned char c=0;c<Tc;c++){
#pragma HLS PIPELINE II=1
            for(unsigned char tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
            	ap_int<Accbit>      t_in     =  src[tn][r][c];
            	ap_int<NormWidth>   t_norm   =  bn_buff[tile_n*Tn+tn];
            	ap_int<Abit>        res      =  norm_quant<Accbit,Abit,NormWidth,NormWidth/2,IntBit>(t_in,t_norm);
                dst[offset +r*Tc+c].range((tn+1)*Abit-1,tn*Abit) = res.range(Abit-1,0);
                //std::cout<<res<<" ";
            }
           //std::cout<<std::endl;
        }
        //std::cout<<std::endl;
    }
}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Abit,unsigned Wbit,unsigned Accbit,
         unsigned InWidth,unsigned WtWidth,unsigned IntBit,unsigned Size,unsigned NormWidth,unsigned NormSize>
void DW_Engine(ap_uint<InWidth>      *In_ddrsrc,
		       ap_uint<WtWidth>      *Wt7x7_ddrsrc,
			   ap_uint<NormWidth>    *Normq_ddrsrc,
			   ap_int<Abit*Tn>       coldbuff[Size],
			   unsigned short row,unsigned short col,unsigned short R, unsigned short C,unsigned short N,
			   unsigned short K, unsigned short PadN){

#pragma HLS inline off
	ap_int<Abit>    Abuff0[Tn][Tr+6][Tc+6];
#pragma HLS BIND_STORAGE variable=Abuff0 type=ram_2p impl=bram
	ap_int<Accbit>  Obuff0[Tn][Tr][Tc];
#pragma HLS BIND_STORAGE variable=Obuff0 type=ram_2p impl=bram
	ap_int<Wbit>    Wbuff0[Tn][7][7];

	ap_int<Abit>    Abuff1[Tn][Tr+6][Tc+6];
#pragma HLS BIND_STORAGE variable=Abuff1 type=ram_2p impl=bram
	ap_int<Accbit>  Obuff1[Tn][Tr][Tc];
#pragma HLS BIND_STORAGE variable=Obuff1 type=ram_2p impl=bram
	ap_int<Wbit>    Wbuff1[Tn][7][7];

	ap_int<NormWidth> BN_buff[NormSize];
	unsigned short FinalN = (PadN == 0)?N:PadN;
	unsigned short loops  = FinalN/Tn;

	if (N == 32){
		expandload3x3_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff0,0,row,col,C,1,R,C,N);
		expandloadwt3x3<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff0,0,N);
	    loadnormwt<NormWidth,NormSize>(Normq_ddrsrc,BN_buff,N);
	    //DW_CONV3x3<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0);
	    DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,K);
	    Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff0,coldbuff,BN_buff,0);
	}else{
		bool flag=true;
	    DWEG1:
	    for(unsigned n=0; n<loops; n++){
	    	if(n==0){
	    		reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff0,0,row,col,C*loops,loops,R,C);
	    	    loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff0,0);
	    	    loadnormwt<NormWidth,NormSize>(Normq_ddrsrc,BN_buff,N);
	    	}else if(n==1) {
	    		reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff1,1,row,col,C*loops,loops,R,C);
	    		loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff1,1);
	    		//DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0);
	    		DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,K);
	    	} else{
	    		if(flag){
					reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff0,n,row,col,C*loops,loops,R,C);
					loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff0,n);
					//DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff1,Obuff1,Wbuff1);
					DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff1,Obuff1,Wbuff1,K);
					Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff0,coldbuff,BN_buff,(n-2));
	    			flag=!flag;
				}else{
					reduceload_axi<Tn,Tr,Tc,Abit,InWidth>(In_ddrsrc,Abuff1,n,row,col,C*loops,loops,R,C);
					loadwt7x7_axi<Tn,Wbit,WtWidth>(Wt7x7_ddrsrc,Wbuff1,n);
					//DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0);
					DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,K);
					Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff1,coldbuff,BN_buff,(n-2));
					flag=!flag;
				}
	    	}

	    }

	    if(loops>1){
	    	if(flag){
	    		//DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff1,Obuff1,Wbuff1);
	    		DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff1,Obuff1,Wbuff1,K);
	    		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff0,coldbuff,BN_buff,(loops-2));
	    		flag=!flag;
	    	}else{
	    		//DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0);
	    		DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,K);
	    		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff1,coldbuff,BN_buff,(loops-2));
	    		flag=!flag;
	    	}
	    	if(flag){
	    		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff0,coldbuff,BN_buff,(loops-1));
	    	}else{
	    		Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff1,coldbuff,BN_buff,(loops-1));
	    	}
	    }else{
	    	//DW_CONV7x7<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0);
	    	DW_CONV<Tr,Tc,Tn,Abit,Wbit,Accbit>(Abuff0,Obuff0,Wbuff0,K);
	    	Q2ColdBuffer<Tr,Tc,Tn,Abit,Accbit,NormWidth,IntBit,Size,NormSize>(Obuff0,coldbuff,BN_buff,0);
	    }

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

template<unsigned Tn,unsigned Tr,unsigned Tc,unsigned Abit,unsigned InWidth>
void expandload_axi( ap_uint<InWidth>  *ddr_burst,
					 ap_int<Abit>      buf[Tn][Tr+6][Tc+6],
					unsigned ch,  unsigned row,unsigned col,
					unsigned  offsetR, unsigned offsetC,unsigned R,unsigned C,unsigned N)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete

	static_assert(InWidth % Abit == 0,"DWIn bandwidth must be divisible by Abit");
	const unsigned short NUM = InWidth / Abit;
	unsigned short tnloops = N/NUM;

    ap_uint<InWidth> DATA;

    //j = row * S - P, j < (row + Tr - 1) * S + K - P
	//row+r-3 <0 | row+r-3>=R | col +c-3<0 | col+c-3>=C
	DwIN_R:
	for(unsigned r = 0; r < Tr+6; r++) {
		DwIN_C:
		for(unsigned c = 0; c < Tc+6; c++) {
			ap_uint<InWidth>  * brust_addr = ddr_burst + (row+r-3)*offsetR*tnloops + (col+c-3)*offsetC*tnloops + ch*tnloops;
			DwIN_P:
			for(unsigned short tnn =0;tnn < tnloops;tnn++){ //tnn < tnloops
#pragma HLS PIPELINE II=1

				if (row+r<3 | row+r> R+2 | col +c<3 | col+c>C+2){
					for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
						buf[tnn*NUM+tn][r][c]= 0;
					}
				}else{
					DATA = brust_addr[tnn];
					for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
						buf[tnn*NUM+tn][r][c].range(Abit-1, 0) = DATA.range( (tn+1)*Abit-1, tn*Abit);
					}
				}
			}
		}
	}

}

