#include"pwconv.h"


template<unsigned Tm,unsigned Tr, unsigned Tc,unsigned Abit,unsigned Accbit,unsigned OutWidth>
void PW_DDR( ap_int<Accbit> buf[Tm][Tr][Tc],
			 ap_uint<OutWidth> * ddr_burst,
			 unsigned m,unsigned row,unsigned col,unsigned offsetR,unsigned offsetC)
{
	//[R1*C1*M1/Tm1]
#pragma HLS inline off
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

template<unsigned Tr, unsigned Tc, unsigned Tm,unsigned Inbit,unsigned Outbit>
void QBramCopy(ap_int<Inbit>  inbuf[Tm][Tr][Tc],
		       ap_uint<Outbit> outbuf[Tm][Tr][Tc]){
#pragma HLS inline off
#pragma HLS array_partition variable=outbuf dim=1 complete
	DBuffer:
	for(unsigned r=0;r<Tr;r++){
		for(unsigned c=0; c<Tc;c++){
#pragma HLS PIPELINE II=1
			for(unsigned k=0;k<Tm;k++){
#pragma HLS UNROLL
				outbuf[k][r][c].range(Outbit-1,0) = inbuf[k][r][c].range(Outbit-1,0); //TODO quant
			}
		}
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

template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void ldwtTm_axiOP( ap_uint<WtWidth> *ddr_burst,
							 ap_uint<Wbit> buf[PRELOAD][Tm][Tn],
							 unsigned short m,unsigned short n,unsigned short offset)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=2 complete
#pragma HLS array_partition variable=buf dim=3 complete

	static_assert(Tm == (WtWidth / Wbit), "Here, PW2 bandwidth must equal to PwTn");
	// N/Tn*M/Tm *Tn*Tm
	unsigned base= (n*offset+m*PRELOAD)*Tn;
	unsigned char tn=0,index = 0;
    ap_uint<WtWidth> DATA;
    PW_WR:
	for(unsigned cnt = 0; cnt < PRELOAD*Tn; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
        //(n*offset+m)*Tn+tn
        DATA.range(WtWidth-1, 0) = ddr_burst[base+index*Tn+tn].range(WtWidth-1, 0);
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

// wt:M/Tm*N/Tn *Tm*Tn
template<unsigned Tn,unsigned Tm,unsigned Wbit,unsigned WtWidth>
void ldwt1x1_axiOP( ap_uint<WtWidth> *ddr_burst,
					ap_uint<Wbit> buf[PRELOAD][Tm][Tn],
					unsigned m,unsigned short n,unsigned short offset)
{
#pragma HLS inline off
#pragma HLS array_partition variable=buf dim=2 complete
#pragma HLS array_partition variable=buf dim=3 complete

unsigned char index = 0;
unsigned char tm =0;
unsigned base = m*offset*Tm + n*PRELOAD*Tm;

	static_assert(Tn == (WtWidth / Wbit), "Here, PW bandwidth must equal to PwTn");

    ap_uint<WtWidth> DATA;
    LW1x1_R:
	for(unsigned cnt = 0; cnt < PRELOAD*Tm; cnt++) {
#pragma HLS DEPENDENCE false intra variable=buf
#pragma HLS DEPENDENCE false inter variable=buf
#pragma HLS PIPELINE II=1
		//addr= (m*offset+n*2+index)*Tm+tm;
		//std::cout<<"Wt addr: " <<addr<<std::endl;

		DATA.range(WtWidth-1, 0) = ddr_burst[base+index*Tm+tm].range(WtWidth-1, 0);
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


//loadAt1x1_bram<Tr1,Tc1,Tn1,Abit1>(coldbuff,1x1Abuff0,n);
template<unsigned Tr,unsigned Tc, unsigned Tn, unsigned Abit,unsigned Size>
void loadAt1x1_bram(ap_uint<Abit*Tn> src[Size], ap_uint<Abit> dst[Tn][Tr][Tc],unsigned offset){
#pragma HLS inline off
#pragma HLS array_partition variable=dst dim=1 complete
//#pragma HLS array_partition variable=src dim=1 complete
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

template <unsigned Tr, unsigned Tc,unsigned DwTn,unsigned PwTn,unsigned Abit,unsigned Size>
void ldbram_ReducedOP(ap_uint<Abit*DwTn> src[Size], ap_uint<Abit> dst[PwTn][PRELOAD][Tr][Tc],unsigned short n){

#pragma HLS inline off
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
//std::cout<<"======Load Input Preload======"<<std::endl;

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
                dst[tn][index][r][c].range(Abit-1,0) = DATA.range((tn+1)*Abit -1, tn*Abit);
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

#pragma HLS inline off
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
template<unsigned Tr,unsigned Tc,unsigned DwTn,unsigned PwTn, unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
         unsigned PW1Width, unsigned dwSize>
void InnerPW1_OP(ap_uint<PW1Width>  *Wt1ddrsrc,
			  ap_uint<Abit*DwTn>  coldbuff[dwSize],
			  ap_int<Accbit>      Obuff[Tm][Tr][Tc],
			  unsigned short m,unsigned short N){
//#pragma HLS inline off

	const unsigned DPwTn = PRELOAD*PwTn;
	unsigned loops = N/DPwTn;
	unsigned short pre_n=0;

	bool flag = true;
	ap_uint<Wbit>  Wbuff_0[PRELOAD][Tm][PwTn];
	ap_uint<Abit>  Abuff_0[PwTn][PRELOAD][Tr][Tc];

	ap_uint<Wbit>  Wbuff_1[PRELOAD][Tm][PwTn];
	ap_uint<Abit>  Abuff_1[PwTn][PRELOAD][Tr][Tc];


	PW1N:
	for(unsigned short n=0;n<loops;n++){

		if(n==0){
			ldwt1x1_axiOP<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_0,m/Tm,n,N/PwTn);
			ldbram_ReducedOP<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
			//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
		}else{
			if(flag){
				ldwt1x1_axiOP<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_1,m/Tm,n,N/PwTn);
				ldbram_ReducedOP<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_1,n);
				//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_1,n);
				CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff,pre_n,0);
				flag = !flag;
			}else{
				ldwt1x1_axiOP<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,Wbuff_0,m/Tm,n,N/PwTn);
				ldbram_ReducedOP<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
				//loadbram_D12_P8<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,Abuff_0,n);
				CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff,pre_n,0);
				flag = !flag;
			}
		}
		pre_n = n;

	}

	if(flag){
		CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_0,Wbuff_0,Obuff,pre_n,0);
	}else{
		CONV1x1_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(Abuff_1,Wbuff_1,Obuff,pre_n,0);
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

template<unsigned Tr,unsigned Tc,unsigned PwTn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
        unsigned PW2Width,unsigned pwSize>
void OutnerPW2_OP(ap_uint<PW2Width> *Wt2ddrsrc,
		       ap_int<Accbit>    PW1Obuff[Tm][Tr][Tc],
		       ap_int<Accbit>    Outbuff[PwTn][pwSize],
			   unsigned short n,unsigned short N){
#pragma HLS inline off
	ap_uint<Wbit>  Wbuff_0[PRELOAD][PwTn][Tm]; //transpose
	ap_uint<Wbit>  Wbuff_1[PRELOAD][PwTn][Tm];
	ap_uint<Abit>  dbuf[Tm][Tr][Tc];
	unsigned loops = N/(PwTn*PRELOAD);
	unsigned short pw2m=0,pre_pw2m=0;
	bool flag = true;

	PW2M:
	for(unsigned pw2m=0;pw2m<loops;pw2m++){
		if(pw2m==0){
			QBramCopy<Tr,Tc,Tm,Accbit,Abit>(PW1Obuff,dbuf);//[Tm][Tr][Tc](outbuf+quant -> inbuf)
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
        unsigned PW1Width,unsigned PW2Width,unsigned dwSize,unsigned pwSize>
void PW_PW_Fused(ap_uint<PW1Width> *Wt1ddrsrc,
				 ap_uint<PW2Width> *Wt2ddrsrc,
			   ap_uint<Abit*DwTn>  coldbuff[dwSize],
			   ap_int<Accbit> Outbuff2[PwTn][pwSize],
			   unsigned M,unsigned N){
#pragma HLS inline off

	ap_int<Accbit> PW1Obuff_0[Tm][Tr][Tc];
	ap_int<Accbit> PW1Obuff_1[Tm][Tr][Tc];


	bool flag = true;
	unsigned loops = M/Tm;
	unsigned short pre_m=0,m=0;


	PW1M_PW2N:
	for(unsigned cnt=0;cnt<loops;cnt++){

		if(cnt==0){
			InnerPW1_OP<Tr,Tc,DwTn,PwTn,Tm,Abit,Wbit,Accbit,PW1Width,dwSize>(Wt1ddrsrc,coldbuff,PW1Obuff_0,m,N);
		}else{
			if(flag){
				InnerPW1_OP<Tr,Tc,DwTn,PwTn,Tm,Abit,Wbit,Accbit,PW1Width,dwSize>(Wt1ddrsrc,coldbuff,PW1Obuff_1,m,N);
				OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,pwSize>(Wt2ddrsrc,PW1Obuff_0,Outbuff2,pre_m,N);
				flag = !flag;
			}else{
				InnerPW1_OP<Tr,Tc,DwTn,PwTn,Tm,Abit,Wbit,Accbit,PW1Width,dwSize>(Wt1ddrsrc,coldbuff,PW1Obuff_0,m,N);
				OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,pwSize>(Wt2ddrsrc,PW1Obuff_1,Outbuff2,pre_m,N);
				flag = !flag;
			}

		}
		pre_m = m;
		m+=Tm;
	}

	if(flag){
		OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,pwSize>(Wt2ddrsrc,PW1Obuff_0,Outbuff2,pre_m,N);
	}else{
		OutnerPW2_OP<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit,PW2Width,pwSize>(Wt2ddrsrc,PW1Obuff_1,Outbuff2,pre_m,N);
	}

}

//template<unsigned Tr,unsigned Tc,unsigned DwTn,unsigned PwTn,unsigned Tm,unsigned Abit,unsigned Wbit,unsigned Accbit,
//        unsigned PW1Width,unsigned PW2Width,unsigned dwSize,unsigned pwSize>
//void PW_PW_Optimized(ap_uint<PW1Width> *Wt1ddrsrc,
//				 ap_uint<PW2Width> *Wt2ddrsrc,
//			     ap_uint<Abit*DwTn>  coldbuff[dwSize],
//			     ap_int<Accbit> Outbuff2[PwTn][pwSize],
//			     unsigned M,unsigned N){
//#pragma HLS inline off
//	ap_int<Accbit> PW1Obuff_0[Tm][Tr][Tc];
//	ap_int<Accbit> PW1Obuff_1[Tm][Tr][Tc];
//
//
//	bool flag = true;
//
//	unsigned totalloops = M/Tm * N/PwTn;
//	unsigned pre_m=0,m=0;
//	//inner function
//	unsigned Innerloops = N/PwTn;
//	unsigned pre_n=0;
//	ap_uint<Wbit>  pw1Wbuf_0[Tm][PwTn];
//	ap_uint<Abit>  pw1Abuf_0[PwTn][Tr][Tc];
//
//	ap_uint<Wbit>  pw1Wbuf_1[Tm][PwTn];
//	ap_uint<Abit>  pw1Abuf_1[PwTn][Tr][Tc];
//
//
//	// Outer function
//	ap_uint<Wbit>  pw2Wbuf_0[PwTn][Tm];
//	ap_uint<Wbit>  pw2Wbuf_1[PwTn][Tm];
//	ap_uint<Abit>  dbuf[Tm][Tr][Tc];
//	unsigned Outerloops = N/PwTn;
//	unsigned pw2m=0,pre_pw2m=0;
//	QBramCopy<Tr,Tc,Tm,Accbit,Abit>(PW1Obuff,dbuf);//[Tm][Tr][Tc](outbuf+quant -> inbuf)
//	loadwtTm_axi<Tm,PwTn,Wbit,PW2Width>(Wt2ddrsrc,Wbuff_0,pw2m,n/Tm,N/PwTn);
//	CONV1x1_acc<Tr,Tc,Tm,PwTn,Abit,Wbit,Accbit,pwSize>(dbuf,Wbuff_1,Outbuff,n/Tm,pre_pw2m*Tr*Tc);
//	unsigned pw1n=0,pw1m=0;
//	PW1PW2O:
//	for(unsigned cnt = 0; cnt < totalloops;cnt++){
//		if(cnt==0){
//			loadwt1x1_axi<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,pw1Wbuf_0,m/Tm,n,N/PwTn);
//			loadbram_Reduced<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,pw1Abuf_0,n);
//		}else if (cnt==1){
//			loadwt1x1_axi<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,pw1Wbuf_1,m/Tm,n,N/PwTn);
//			loadbram_Reduced<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,pw1Abuf_1,n);
//			CONV1x1<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(pw1Abuf_0,pw1Wbuf_0,PW1Obuff_0,pre_n,0);
//		}else if (cnt==2){
//			loadwt1x1_axi<PwTn,Tm,Wbit,PW1Width>(Wt1ddrsrc,pw1Wbuf_0,m/Tm,n,N/PwTn);
//			loadbram_Reduced<Tr,Tc,DwTn,PwTn,Abit,dwSize>(coldbuff,pw1Abuf_0,n);
//			CONV1x1<Tr,Tc,PwTn,Tm,Abit,Wbit,Accbit>(pw1Abuf_1,pw1Wbuf_1,PW1Obuff_1,pre_n,0);
//
//		}else{
//			if(flag){
//
//			}else{
//
//			}
//		}
//		if (pw1n == N-PwTn){
//			pw1n = 0;
//			pw1m+=Tm;
//		}
//		else{
//			pw1n += PwTn;
//		}
//
//	}
//
//}

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
