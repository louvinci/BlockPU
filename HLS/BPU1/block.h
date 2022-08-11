#include <ap_fixed.h>


//hardware parameters
#define Tr1 14
#define Tc1 14
#define DwTn1 64
#define PwTn1 32
#define Tm1 16
#define Abit1 8
#define Wbit1 8
#define Obit1 8
#define Accbit1 32
#define Accbit2 22
#define MXN1 128
#define MXM1 384
#define MXR1 112
#define MXC1 112
#define PRELOAD 1

#define SIZE1      MXN1/DwTn1*Tr1*Tc1
#define SIZE2      MXN1/PwTn1*Tr1*Tc1

#define NormSize1  96


#define DWinNum1 8
#define DWwtNum1 4
#define PW1wtNum1 4
#define PW2wtNum1 4
#define PW2OutNum1 4

#define InWidth1  Abit1*DWinNum1 // axi4 datawidth
#define DwWidth1  Wbit1*DWwtNum1
#define Pw1Width1 Wbit1*PW1wtNum1
#define Pw2Width1 Wbit1*PW2wtNum1
#define OutWidth1 Abit1*PW2OutNum1//output bandwidth
#define ScaleBit1 32
#define NormWidth1 32*2  // norm +bias, scale1+scale2
#define BN_int1 10
#define Scale_int1 10
#define ConfigWidth1 ScaleBit1*4





// this method can specify the deepth of axi-interface
extern "C"{
void QBPU1(
					   ap_uint<InWidth1>      In_ddrsrc[MXR1*MXC1*MXN1/DWinNum1],
					   ap_uint<NormWidth1>    Normq_ddrsrc[NormSize1],
					   ap_uint<DwWidth1>      Wt7x7_ddrsrc[MXN1/DWwtNum1*7*7],
					   ap_uint<Pw1Width1>     Wt1_ddrsrc[MXM1*MXN1/PW1wtNum1],
					   ap_uint<Pw2Width1>     Wt2_ddrsrc[MXN1*MXM1/PW2wtNum1],
					   ap_uint<OutWidth1>     Branch_ddr[MXR1*MXC1*MXN1/PW2OutNum1],
					   ap_uint<OutWidth1>     Out_ddr[MXR1*MXC1*MXN1/PW2OutNum1],
					   ap_uint<ScaleBit1>     rescale1,
					   ap_uint<ScaleBit1>     rescale2,
					   ap_uint<ScaleBit1>     id_rescale,
					   unsigned   R1, unsigned C1,
					   unsigned   M1, unsigned N1,unsigned K,unsigned PadN1
					);
}

//extern "C"{
//void FusedDW_PW_InMode(
//					   ap_uint<InWidth1> *In_ddrsrc,
//					   ap_uint<DwWidth1> *Wt7x7_ddrsrc,
//					   ap_uint<Pw1Width1> *Wt1x1_ddrsrc,
//					   ap_uint<Pw2Width1> *Wt2_ddrsrc,
//					   ap_uint<OutWidth1> *Out_ddr
//					);
//}
