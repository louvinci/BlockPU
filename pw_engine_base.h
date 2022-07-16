#include"pwconv.h"
template<unsigned Tr,unsigned Tc,unsigned DwTn,unsigned PwTn, unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
         unsigned PW1Width, unsigned dwSize>
void InnerPW1(ap_uint<PW1Width>  *Wt1ddrsrc,
			  ap_uint<Abit*DwTn>  coldbuff[dwSize],
			  ap_int<Accbit>      Obuff[Tm][Tr][Tc],
			  unsigned m,unsigned N){
//#pragma HLS inline off

	//Three's no need to apply double buffering to PW1 outbuf
	// DwTn \ PwTn, and N\DwTn, N\PwTn

	unsigned loops = N/PwTn,pre_n=0;
	const unsigned time = DwTn / PwTn;

	bool flag = true;
	ap_uint<Wbit>  Wbuff_0[Tm][PwTn];
	ap_uint<Abit>  Abuff_0[PwTn][Tr][Tc];

	ap_uint<Wbit>  Wbuff_1[Tm][PwTn];
	ap_uint<Abit>  Abuff_1[PwTn][Tr][Tc];


	PW1N:
	for(unsigned n=0;n<loops;n++){
		if(n==0){
			loadwt1x1_axi<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_0,m/Tm,n,N/PwTn);
			loadbram_Reduced<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
			//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
		}else{
			if(flag){
				loadwt1x1_axi<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_1,m/Tm,n,N/PwTn);
				loadbram_Reduced<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_1,n);
				//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_1,n);
				CONV1x1<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff,pre_n,0);
				flag = !flag;
			}else{
				loadwt1x1_axi<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_0,m/Tm,n,N/PwTn);
				loadbram_Reduced<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
				//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
				CONV1x1<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff,pre_n,0);
				flag = !flag;
			}
		}
		pre_n = n;

	}

	if(flag){
		CONV1x1<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff,pre_n,0);
	}else{
		CONV1x1<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff,pre_n,0);
	}


}

//load in the tm direction
template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void loadwtTm_axi( ap_uint<WtWidth> *ddr_burst,
							 ap_uint<Wbit> buf[Tm][Tn],
							 unsigned m,unsigned n,unsigned offset)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=0 complete

    ap_uint<WtWidth> DATA;
    PW_WR:
	for(unsigned tn = 0; tn < Tn; tn++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
        //(m*offset+n)*block+tm
        DATA.range(WtWidth-1, 0) = ddr_burst[(n*offset+m)*Tn+tn].range(WtWidth-1, 0);
		for(unsigned tm = 0; tm < Tm; tm++) {
#pragma HLS UNROLL
			buf[tm][tn].range(Wbit-1, 0) = DATA.range( (tm+1)*Wbit-1, tm*Wbit);

		}
	}
}

//  M/Tm*N/Tn*Tm*Tn
template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void loadwt1x1_axi( ap_uint<WtWidth> *ddr_burst,
					ap_uint<Wbit> buf[Tm][Tn],
					unsigned m,unsigned n,unsigned offset)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=0 complete

    ap_uint<WtWidth> DATA;
    LW1_R:
	for(unsigned tm = 0; tm < Tm; tm++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf

#pragma HLS PIPELINE II=1
		DATA.range(WtWidth-1, 0) = ddr_burst[(m*offset+n)*Tm+tm].range(WtWidth-1, 0);
		for(unsigned tn = 0; tn < Tn; tn++) {
#pragma HLS UNROLL
			buf[tm][tn].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);

		}
	}
}


template <unsigned Tr, unsigned Tc,unsigned DwTn,unsigned PwTn,unsigned Abit,unsigned Size>
void loadbram_Reduced(ap_uint<Abit*DwTn> src[Size], ap_uint<Abit> dst[PwTn][Tr][Tc],unsigned n){

#pragma HLS inline off
#pragma HLS array_partition variable=dst dim=1 complete

static_assert(DwTn % PwTn == 0,"Fixed Reduced Load, DwTn mod PwTn must be 0");
const unsigned loops = Tr*Tc;
const unsigned times = DwTn/PwTn;

unsigned r=0,c=0;
unsigned offset1 = (n/times)*Tr*Tc;
unsigned left  = (n%times+1)*Abit*PwTn-1;
unsigned right = (n%times)*Abit*PwTn;

ap_uint<Abit*PwTn> DATA;
	ldAtBram:
    for(unsigned cnt=0;cnt<loops;cnt++){
#pragma HLS PIPELINE II=1

			DATA.range(Abit*PwTn-1,0) = src[offset1+cnt].range(left,right);
            for(unsigned tn=0;tn<PwTn;tn++){
#pragma HLS UNROLL
                dst[tn][r][c].range(Abit-1,0) = DATA.range((tn+1)*Abit -1, tn*Abit);
            }
			if (c==Tc-1){
				r+=1;
				c=0;
			}else{
				c+=1;
			}
    }

}

template<unsigned Tr,unsigned Tc,unsigned PwTn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
        unsigned PW2Width,unsigned pwSize>
void OutnerPW2(ap_uint<PW2Width> *Wt2ddrsrc,
		       ap_int<Accbit>    PW1Obuff[Tm][Tr][Tc],
		       ap_int<Accbit>    Outbuff[PwTn][pwSize],
			   unsigned n,unsigned N){
//#pragma HLS inline off
	ap_uint<Wbit>  Wbuff_0[PwTn][Tm];
	ap_uint<Wbit>  Wbuff_1[PwTn][Tm];
	ap_uint<Abit>  dbuf[Tm][Tr][Tc];
	unsigned loops = N/PwTn;
	unsigned pw2m=0,pre_pw2m=0;
	bool flag = true;

	PW2M:
	for(unsigned pw2m=0;pw2m<loops;pw2m++){
		if(pw2m==0){
			QBramCopy<Tr,Tc,Tm,Accbit,Abit>(PW1Obuff,dbuf);//[Tm][Tr][Tc](outbuf+quant -> inbuf)
			loadwtTm_axi<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_0,pw2m,n/Tm,N/PwTn);
		}else{
			if(flag){
				loadwtTm_axi<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_1,pw2m,n/Tm,N/PwTn);
				CONV1x1_acc<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_0,Outbuff,n/Tm,pre_pw2m*Tr*Tc);
				flag = !flag;
			}else{
				loadwtTm_axi<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_0,pw2m,n/Tm,N/PwTn);
				CONV1x1_acc<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_1,Outbuff,n/Tm,pre_pw2m*Tr*Tc);
				flag = !flag;
			}
		}
		pre_pw2m = pw2m;

	}
	if(flag){
		CONV1x1_acc<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_0,Outbuff,n,pre_pw2m*Tr*Tc);
	}else{
		CONV1x1_acc<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_1,Outbuff,n,pre_pw2m*Tr*Tc);
	}

}

//don't use
template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
        unsigned WtWidth,unsigned OutWidth,unsigned Size>
void PW_EngineDATAFLOW(ap_uint<WtWidth> *Wt1x1_ddrsrc,
			   ap_uint<Abit> coldbuff[Tn][Size],
			   ap_uint<OutWidth> *Out_ddr,
			   unsigned row,unsigned col,unsigned C,unsigned M,unsigned N){
#pragma HLS inline off
	ap_uint<Abit>  Abuff1x1_0[Tn][Tr][Tc];
	ap_uint<Wbit>  Wbuff1x1_0[Tm][Tn];
	ap_int<Accbit> Obuff1x1_0[Tm][Tr][Tc];
	unsigned loops = N/Tn;
    PWEG1:
    for(unsigned m=0; m<M; m+=Tm){
        for(unsigned n=0; n<loops; n++){
#pragma HLS dataflow
            loadwt1x1_axi<Tn,Tm,Wbit,WtWidth>(Wt1x1_ddrsrc,Wbuff1x1_0,m,n,N/Tn);
            loadAt1x1_bram<Tr,Tc,Tn,Abit,Size>(coldbuff,Abuff1x1_0,n*Tr*Tc);
            CONV1x1<Tr,Tc,Tn,Tm,Abit,Wbit,Accbit>(Abuff1x1_0,Wbuff1x1_0,Obuff1x1_0,n*Tn,0);
        }

        PW_DDR<Tm,Tr,Tc,Abit,Accbit,OutWidth>(Obuff1x1_0,Out_ddr,m/Tm,row,col,C*M/Tm,M/Tm);
        //PW_DDR_ADD_RELU
    }
}

template<unsigned Tr,unsigned Tc,unsigned Tn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
        unsigned WtWidth,unsigned OutWidth,unsigned Size>
void PW_Engine(ap_uint<WtWidth> *Wt1x1_ddrsrc,
			   ap_uint<Abit*Tn> coldbuff[Size],
			   ap_uint<OutWidth> *Out_ddr,
			   unsigned row,unsigned col,unsigned C,unsigned M,unsigned N){
#pragma HLS inline off
	ap_uint<Abit> Abuff_0[Tn][Tr][Tc];
	ap_uint<Wbit> Wbuff_0[Tm][Tn];
	ap_uint<Abit> Abuff_1[Tn][Tr][Tc];
	ap_uint<Wbit> Wbuff_1[Tm][Tn];

	ap_int<Accbit> Obuff_0[Tm][Tr][Tc];
	unsigned inner_loop = N/Tn;
	unsigned loops = inner_loop* M/Tm;

	unsigned n=0,pre_n=0,m=0,pre_m=0;

	unsigned mac_cnt=0;
	bool flag=true;

	{
		loadwt1x1_axi<Tn,Tm,Wbit,WtWidth>(Wt1x1_ddrsrc,Wbuff_0,0,0,N/Tn);
		loadAt1x1_bram<Tr,Tc,Tn,Abit,Size>(coldbuff,Abuff_0,0);
	}
	//default the loops>1
    pre_n=n;
    pre_m=m;
    if(n==inner_loop-1){
        m+=Tm;
        n=0;
    }else{
    	n+=1;
    }

    PWEG1:
    for(unsigned cnt=1; cnt<loops; cnt++){
		if(flag){
			loadwt1x1_axi<Tn,Tm,Wbit,WtWidth>(Wt1x1_ddrsrc,Wbuff_1,m,n,N/Tn);
			loadAt1x1_bram<Tr,Tc,Tn,Abit,Size>(coldbuff,Abuff_1,n*Tr*Tc);
			CONV1x1<Tr,Tc,Tn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff_0,pre_n*Tn,0);
			if(mac_cnt==inner_loop-1){
				PW_DDR<Tm,Tr,Tc,Abit,Accbit,OutWidth>(Obuff_0,Out_ddr,pre_m/Tm,row,col,C*M/Tm,M/Tm);
			}
			flag=!flag;
			if(mac_cnt==inner_loop-1){
				mac_cnt=0;
			}else{
				mac_cnt+=1;
			}

		}else{
			//! the m,n make the false schedule
			loadwt1x1_axi<Tn,Tm,Wbit,WtWidth>(Wt1x1_ddrsrc,Wbuff_0,m,n,N/Tn);
			loadAt1x1_bram<Tr,Tc,Tn,Abit,Size>(coldbuff,Abuff_0,n*Tr*Tc);
			CONV1x1<Tr,Tc,Tn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff_0,pre_n*Tn,0);

			if(mac_cnt==inner_loop-1){
				PW_DDR<Tm,Tr,Tc,Abit,Accbit,OutWidth>(Obuff_0,Out_ddr,pre_m/Tm,row,col,C*M/Tm,M/Tm);
			}
			flag=!flag;
			if(mac_cnt==inner_loop-1){
				mac_cnt=0;
			}else{
				mac_cnt+=1;
			}
		}

        pre_n=n;
        pre_m=m;
        if(n==N/Tn-1){
            m+=Tm;
            n=0;
        }else{
        	n+=1;
        }
    }
    if(flag){
    	CONV1x1<Tr,Tc,Tn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff_0,pre_n*Tn,0);
    }else{
    	CONV1x1<Tr,Tc,Tn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff_0,pre_n*Tn,0);
    }
    PW_DDR<Tm,Tr,Tc,Abit,Accbit,OutWidth>(Obuff_0,Out_ddr,pre_m/Tm,row,col,C*M/Tm,M/Tm);

}
