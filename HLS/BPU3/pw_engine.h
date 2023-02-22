#include"pwconv.h"
#include"quant_node.h"

template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned OutWidth>
void PW_DDR( ap_int<Accbit> buf[Tm][Tr][Tc],
			 ap_uint<OutWidth> * ddr_burst,
			 unsigned m,unsigned row,unsigned col,unsigned offsetR,unsigned offsetC)
{
	//[R1*C1*M1/Tm1]
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=1 complete
    ap_uint<OutWidth> DATA;
    const unsigned NUM= OutWidth/Abit;
    const unsigned loops = Tm/(OutWidth/Abit);
    PW_R:
	for(unsigned r = 0; r < Tr; r++) {
        PW_C:
		for(unsigned c = 0; c < Tc; c++) {
            PW_L:
			for(unsigned tm = 0; tm < loops; tm++) {//TODO Merge the loops
#pragma HLS PIPELINE II=1
                for(unsigned ii=0;ii<NUM;ii++){
#pragma HLS UNROLL
                    DATA.range( (ii+1)*Abit-1, ii*Abit)=buf[tm*NUM+ii][r][c].range(Abit-1, 0); //TODO,direct clip, plan to add quant operation
                }
                ddr_burst[(row+r)*offsetR + (col+c)*offsetC + m+tm].range(OutWidth-1, 0)=DATA.range(OutWidth-1, 0);
			}
		}
	}
#ifdef DEBUG
	std::cout<<"++++++++PE Result"<<std::endl;
	for(unsigned tm = 0; tm < Tm; tm+=loops){
		for(unsigned r = 0; r < Tr; r++) {
			for(unsigned c = 0; c < Tc; c++) {
				std::cout<<buf[tm][r][c]<<" "<<std::endl;
			}
			std::cout<<std::endl;
		}
		std::cout<<std::endl;
	}
#endif

}

template<unsigned Tr, unsigned Tc, unsigned Tm,unsigned Inbit,unsigned Outbit,unsigned ScaleBit,unsigned IntBit>
void QBramCopy(ap_int<Inbit>  inbuf[Tm][Tr][Tc],
		       ap_int<Outbit*Tm> outbuf[Tr][Tc],ap_uint<ScaleBit> rescale){
//#pragma HLS inline off
#pragma HLS array_partition variable=outbuf dim=1 complete
	DBuffer:
	for(unsigned r=0;r<Tr;r++){
		for(unsigned c=0; c<Tc;c++){
#pragma HLS PIPELINE II=1
			for(unsigned k=0;k<Tm;k++){
#pragma HLS UNROLL
				ap_int<Inbit> in = inbuf[k][r][c];
				ap_int<Outbit> res = quant<Inbit,Outbit,ScaleBit,IntBit>(in,rescale);
				outbuf[r][c].range((k+1)*Outbit-1,k*Outbit) = res.range(Outbit-1,0);
			}
		}
	}
}


//fixed means the parallel equals the weight port's width,Tm == (WtWidth / Wbit)
template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void ldwtTm_fixed( ap_uint<WtWidth> *ddr_burst,
				   ap_int<Wbit> buf[PRELOAD][Tm][Tn],
				   unsigned short m,unsigned short n,unsigned short offset)
{
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=2 complete
#pragma HLS array_partition variable=buf dim=3 complete

	static_assert(Tm == (WtWidth / Wbit), "Here, PW2 bandwidth must equal to PwTn");
	// N/Tn*M/Tm *Tn*Tm
	//unsigned base= (n*offset+m*PRELOAD)*Tn;
	ap_uint<WtWidth>* base_brust = ddr_burst+(n*offset+m*PRELOAD)*Tn;
	unsigned char tn=0,index = 0;
    ap_uint<WtWidth> DATA;

    PW_WR:
	for(unsigned cnt = 0; cnt < PRELOAD*Tn; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1

		DATA.range(WtWidth-1, 0) = base_brust[cnt].range(WtWidth-1, 0);
		for(unsigned tm = 0; tm < Tm; tm++) {
#pragma HLS UNROLL
			buf[index][tm][tn].range(Wbit-1, 0) = DATA.range( (tm+1)*Wbit-1, tm*Wbit);

		}
		if(tn == Tn-1){
			index += 1;
			tn = 0;
		}else{
			tn+=1;
		}
	}
}



// wt:M/Tm*N/Tn *Tm*Tn, fixed means the parallel equals the weight port's width,Tn == (WtWidth / Wbit)
template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void ldwt1x1_fixed( ap_uint<WtWidth> *ddr_burst,
					ap_int<Wbit>     buf[PRELOAD][Tm][Tn],
					unsigned m,unsigned short n,unsigned short offset)
{
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=2 complete
#pragma HLS array_partition variable=buf dim=3 complete

unsigned char index = 0;
unsigned char tm =0;
//unsigned base = m*offset*Tm + n*PRELOAD*Tm;
ap_uint<WtWidth> * base_brust = ddr_burst + m*offset*Tm + n*PRELOAD*Tm;

	static_assert(Tn == (WtWidth / Wbit), "Here, PW bandwidth must equal to PwTn");

    ap_uint<WtWidth> DATA;
    LW1x1_R:
	for(unsigned cnt = 0; cnt < PRELOAD*Tm; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1

		DATA.range(WtWidth-1, 0) = base_brust[cnt].range(WtWidth-1, 0);
		for(unsigned tn = 0; tn < Tn; tn++) {
#pragma HLS UNROLL
			buf[index][tm][tn].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);

		}
		if (tm==Tm-1){
			index += 1;
			tm = 0;
		}else{
			tm+=1;
		}
	}
}

template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void ldwtTm_axiOP( ap_uint<WtWidth> *ddr_burst,
				   ap_int<Wbit> buf[PRELOAD][Tm][Tn],
				   unsigned short m,unsigned short n,unsigned short offset)
{
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=2 complete
#pragma HLS array_partition variable=buf dim=3 complete

	//static_assert(Tm == (WtWidth / Wbit), "Here, PW2 bandwidth must equal to PwTn");
	static_assert( WtWidth%Wbit==0,"PwWt2 bandwidth must be divisible by Wbit");
	static_assert( Tm%(WtWidth/Wbit)==0,"PwTm2 must be divisible by WtNUM/cycle");
	const unsigned NUM = WtWidth / Wbit;
	const unsigned tmloops = Tm/ NUM;

	ap_uint<WtWidth>* base_brust = ddr_burst+(n*offset+m*PRELOAD)*Tn*tmloops;
	unsigned char tn=0,index = 0,tmm=0;
    ap_uint<WtWidth> DATA;

    PW_WR:
	for(unsigned cnt = 0; cnt < PRELOAD*tmloops*Tn; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		DATA.range(WtWidth-1, 0) = base_brust[cnt].range(WtWidth-1, 0);
		for(unsigned tm = 0; tm < NUM; tm++) {
#pragma HLS UNROLL
			buf[index][tmm*NUM+tm][tn].range(Wbit-1, 0) = DATA.range( (tm+1)*Wbit-1, tm*Wbit);

		}
		if(tmm == tmloops-1){
			tmm=0;
			if(tn == Tn-1){
				tn = 0;
				index+=1;
			}else{
				tn+=1;
			}
		}else{
			tmm+=1;
		}
	}

}



// wt:M/Tm*N/Tn *Tm*Tn
template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void ldwtTn_axiOP( ap_uint<WtWidth> *ddr_burst,
					 ap_int<Wbit>     buf[PRELOAD][Tm][Tn],
					unsigned m,unsigned short n,unsigned short offset)
{
//#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=2 complete
#pragma HLS array_partition variable=buf dim=3 complete

    unsigned char index = 0, tm=0, tnn=0;


	//static_assert(Tn == (WtWidth / Wbit), "Here, PW bandwidth must equal to PwTn");
	static_assert( WtWidth%Wbit==0,"PwWt2 bandwidth must be divisible by Wbit");
	static_assert( Tn%(WtWidth/Wbit)==0,"PwTm2 must be divisible by WtNUM/cycle");
	const unsigned NUM = WtWidth / Wbit;
	const unsigned tnloops = Tn/ NUM;
	ap_uint<WtWidth> * base_brust = ddr_burst + m*offset*Tm*tnloops + n*PRELOAD*Tm*tnloops;

    ap_uint<WtWidth> DATA;
    LW1x1_R:
	for(unsigned cnt = 0; cnt < PRELOAD*tnloops*Tm; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		DATA.range(WtWidth-1, 0) = base_brust[cnt].range(WtWidth-1, 0);
		for(unsigned tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
			buf[index][tm][tnn*NUM+tn].range(Wbit-1, 0) = DATA.range( (tn+1)*Wbit-1, tn*Wbit);

		}
		if(tnn == tnloops-1){
			tnn=0;
			if (tm==Tm-1){
				index += 1;
				tm = 0;
			}else{
				tm+=1;
			}
		}else{
			tnn+=1;
		}

	}
}


//loadAt1x1_bram<Tr1,Tc1,Tn1,Abit1>(coldbuff,1x1Abuff0,n);
template<unsigned Tr,unsigned Tc, unsigned Tn, unsigned Abit,unsigned Size>
void loadAt1x1_bram(ap_uint<Abit*Tn> src[Size], ap_uint<Abit> dst[Tn][Tr][Tc],unsigned offset){
//#pragma HLS inline off
#pragma HLS array_partition variable=dst dim=1 complete

	ldAtBram:
    for(unsigned r=0;r<Tr;r++){
        for(unsigned c=0; c<Tc;c++){
#pragma HLS PIPELINE II=1
            for(unsigned tn=0;tn<Tn;tn++){
#pragma HLS UNROLL
                dst[tn][r][c].range(Abit-1,0) = src[offset+r*Tc+c].range((tn+1)*Abit -1, tn*Abit);
            }
        }
    }

}



template <unsigned Tr, unsigned Tc,unsigned DwTn,unsigned PwTn,unsigned Abit,unsigned Size>
void ldbram_ReducedOP(ap_int<Abit*DwTn> src[Size], ap_int<Abit*PwTn> dst[PRELOAD][Tr][Tc],unsigned short n){

//#pragma HLS inline off
#pragma HLS array_partition variable=dst dim=1 complete

static_assert(DwTn % PwTn == 0,"Fixed Reduced Load, DwTn mod PwTn must be 0");
const unsigned loops = PRELOAD*Tr*Tc;
const unsigned char times = DwTn/PwTn;
const unsigned width = Abit*PwTn;

unsigned char r=0,c=0;
unsigned offset1 = (PRELOAD*n/times)*Tr*Tc;
unsigned left=0,right=0;
unsigned char index = 0;
unsigned short tmp = 0;

ap_uint<Abit*PwTn> DATA;
	ldAtBram:
    for(unsigned cnt=0;cnt<loops;cnt++){
#pragma HLS PIPELINE II=1
    		tmp = (PRELOAD*n+index);
    	    left  = (tmp%times+1)*width-1;
    	    right = (tmp%times)*width;
//    	    std::cout<<"ele index:  "<<offset1+r*Tc+c<<std::endl;
//    	    std::cout<<"bit index:  "<<left<<" "<<right<<std::endl;
			DATA.range(Abit*PwTn-1,0) = src[offset1+(index/times)*Tr*Tc+r*Tc+c].range(left,right);
            for(unsigned tn=0;tn<PwTn;tn++){
#pragma HLS UNROLL
                dst[index][r][c].range((tn+1)*Abit-1,tn*Abit) = DATA.range((tn+1)*Abit -1, tn*Abit);
            }
			if (c==Tc-1){
				c=0;
				if(r ==Tr-1){
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

template <unsigned Tr, unsigned Tc,unsigned DwTn,unsigned PwTn,unsigned Abit,unsigned Size>
void loadbram_D12_P8(ap_uint<Abit*DwTn> src[Size], ap_uint<Abit> dst[PwTn][Tr][Tc],unsigned n){

//#pragma HLS inline off
#pragma HLS array_partition variable=dst dim=1 complete

const unsigned loops = Tr*Tc;
static unsigned short Second[3] = {0,1,0};
static unsigned short F1[3] = {0,8,4};
static unsigned short F2[3] = {4,0,8};

unsigned index1 = n*PwTn/DwTn*Tr*Tc;
unsigned index2 = (n*PwTn/DwTn+Second[n%3])*Tr*Tc;

unsigned right1  = F1[n%3]*Abit;
unsigned left1   = (F1[n%3]+4)*Abit-1;

unsigned right2  =  F2[n%3]*Abit;
unsigned left2  = (F2[n%3]+4)*Abit-1;
//std::cout<<"\nindex1: "<<index1<<"  index2: "<<index2<<std::endl;
//std::cout<<"left1: "<<left1<<"  right1: "<<right1<<std::endl;
//std::cout<<"left2: "<<left2<<"  right2: "<<right2<<std::endl;
unsigned r=0,c=0;
ap_uint<Abit*PwTn> DATA;
	ldAtBram:
    for(unsigned cnt=0;cnt<loops;cnt++){
#pragma HLS PIPELINE II=1

			DATA.range(Abit*PwTn/2-1,0)         = src[index1+cnt].range(left1,right1);
			DATA.range(Abit*PwTn-1,Abit*PwTn/2) = src[index2+cnt].range(left2,right2);
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


template<unsigned Tr,unsigned Tc,unsigned DwTn,unsigned PwTn, unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
         unsigned PW1Width, unsigned dwSize>
void InnerPW1_OP(ap_uint<PW1Width>  *Wt1ddrsrc,
			     ap_int<Abit*DwTn>  coldbuff[dwSize],
			     ap_int<Accbit>     Obuff[Tm][Tr][Tc],
			  unsigned short m,unsigned short N){
//#pragma HLS inline off

	const unsigned DPwTn = PRELOAD*PwTn;
	unsigned loops = N/DPwTn;
	unsigned short pre_n=0;

	bool flag = true;
	ap_int<Wbit>  Wbuff_0[PRELOAD][Tm][PwTn];
	ap_int<Abit*PwTn>  Abuff_0[PRELOAD][Tr][Tc];
#pragma HLS BIND_STORAGE variable=Abuff_0 type=ram_2p impl=bram

	ap_int<Wbit>  Wbuff_1[PRELOAD][Tm][PwTn];
	ap_int<Abit*PwTn>  Abuff_1[PRELOAD][Tr][Tc];
#pragma HLS BIND_STORAGE variable=Abuff_1 type=ram_2p impl=bram


	PW1N:
	for(unsigned short n=0;n<loops;n++){

		if(n==0){
			ldwtTn_axiOP<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_0,m/Tm,n,N/PwTn);
			ldbram_ReducedOP<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
			//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
		}else{
			if(flag){
				ldwtTn_axiOP<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_1,m/Tm,n,N/PwTn);
				ldbram_ReducedOP<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_1,n);
				//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_1,n);
				CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff,pre_n);
				flag = !flag;
			}else{
				ldwtTn_axiOP<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_0,m/Tm,n,N/PwTn);
				ldbram_ReducedOP<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
				//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
				CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff,pre_n);
				flag = !flag;
			}
		}
		pre_n = n;

	}

	if(flag){
		CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff,pre_n);
	}else{
		CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff,pre_n);
	}


}


template<unsigned Tr,unsigned Tc,unsigned PwTn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
        unsigned PW2Width,unsigned ScaleBit,unsigned IntBit,unsigned pwSize>
void OutnerPW2_OP(ap_uint<PW2Width> *Wt2ddrsrc,
		       ap_int<Accbit>    PW1Obuff[Tm][Tr][Tc],
		       ap_int<Accbit>    Outbuff[PwTn][pwSize],
			   unsigned short n,unsigned short N,
			   ap_uint<ScaleBit> rescale){
//#pragma HLS inline off
	ap_int<Wbit>  Wbuff_0[PRELOAD][PwTn][Tm]; //transpose
	ap_int<Wbit>  Wbuff_1[PRELOAD][PwTn][Tm];
	ap_int<Abit*Tm>  dbuf[Tr][Tc];
	unsigned short loops = N/(PwTn*PRELOAD);
	unsigned short pw2m=0,pre_pw2m=0;
	bool flag = true;

	PW2M:
	for(unsigned short pw2m=0;pw2m<loops;pw2m++){
		if(pw2m==0){
			QBramCopy<Tr,Tc,Tm,Accbit,Abit,ScaleBit,IntBit>(PW1Obuff,dbuf,rescale);//[Tm][Tr][Tc](outbuf+quant -> inbuf)
			ldwtTm_axiOP<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_0,pw2m,n/Tm,N/PwTn);
		}else{
			if(flag){
				ldwtTm_axiOP<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_1,pw2m,n/Tm,N/PwTn);
				CONV1x1_acc_OP<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_0,Outbuff,n/Tm,pre_pw2m*Tr*Tc);
				flag = !flag;
			}else{
				ldwtTm_axiOP<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_0,pw2m,n/Tm,N/PwTn);
				CONV1x1_acc_OP<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_1,Outbuff,n/Tm,pre_pw2m*Tr*Tc);
				flag = !flag;
			}
		}
		pre_pw2m = pw2m;

	}
	if(flag){
		CONV1x1_acc_OP<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_0,Outbuff,n,pre_pw2m*Tr*Tc);
	}else{
		CONV1x1_acc_OP<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_1,Outbuff,n,pre_pw2m*Tr*Tc);
	}

}

template<unsigned Tr,unsigned Tc,unsigned DwTn,unsigned PwTn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
        unsigned PW1Width,unsigned PW2Width,unsigned ScaleBit,unsigned IntBit,unsigned dwSize,unsigned pwSize>
void PW_PW_Fused(ap_uint<PW1Width> *Wt1ddrsrc,
				 ap_uint<PW2Width> *Wt2ddrsrc,
			     ap_int<Abit*DwTn>  coldbuff[dwSize],
			     ap_int<Accbit>     Outbuff2[PwTn][pwSize],
			     unsigned short M,unsigned short N,
			     ap_uint<ScaleBit> rescale
){
#pragma HLS inline off

	ap_int<Accbit> PW1Obuff_0[Tm][Tr][Tc];
#pragma HLS BIND_STORAGE variable=PW1Obuff_0 type=ram_2p impl=bram
	ap_int<Accbit> PW1Obuff_1[Tm][Tr][Tc];
#pragma HLS BIND_STORAGE variable=PW1Obuff_1 type=ram_2p impl=bram

	bool flag = true;
	unsigned short loops = M/Tm;
	unsigned short pre_m=0,m=0;


	PW1M_PW2N:
	for(unsigned short cnt=0;cnt<loops;cnt++){

		if(cnt==0){
			InnerPW1_OP<Tr,Tc,DwTn,PwTn,Tm,Abit,Wbit,Accbit,PW1Width,dwSize>(Wt1ddrsrc,coldbuff,PW1Obuff_0,m,N);
		}else{
			if(flag){
				InnerPW1_OP<Tr,Tc,DwTn,PwTn,Tm,Abit,Wbit,Accbit,PW1Width,dwSize>(Wt1ddrsrc,coldbuff,PW1Obuff_1,m,N);
				OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,ScaleBit,IntBit,pwSize>(Wt2ddrsrc,PW1Obuff_0,Outbuff2,pre_m,N,rescale);
				flag = !flag;
			}else{
				InnerPW1_OP<Tr,Tc,DwTn,PwTn,Tm,Abit,Wbit,Accbit,PW1Width,dwSize>(Wt1ddrsrc,coldbuff,PW1Obuff_0,m,N);
				OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,ScaleBit,IntBit,pwSize>(Wt2ddrsrc,PW1Obuff_1,Outbuff2,pre_m,N,rescale);
				flag = !flag;
			}

		}
		pre_m = m;
		m+=Tm;
	}

	if(flag){
		OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,ScaleBit,IntBit,pwSize>(Wt2ddrsrc,PW1Obuff_0,Outbuff2,pre_m,N,rescale);
	}else{
		OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,ScaleBit,IntBit,pwSize>(Wt2ddrsrc,PW1Obuff_1,Outbuff2,pre_m,N,rescale);
	}

}





