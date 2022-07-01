#include <ap_fixed.h>


//hardware parameters
#define Tr1 7
#define Tc1 7
#define DwTn1 32
#define PwTn1 8
#define Tm1 32
#define Abit1 8
#define Wbit1 8
#define Accbit1 16
#define MXN1 64
#define PRELOAD 4

#define DWinNum1 8
#define DWwtNum1 4
#define PW1wtNum1 8
#define PW2wtNum1 8
#define PW2OutNum1 4

#define InWidth1  Abit1*DWinNum1 // axi4 datawidth
#define DwWidth1  Wbit1*DWwtNum1
#define Pw1Width1 Wbit1*PW1wtNum1
#define Pw2Width1 Wbit1*PW2wtNum1
#define OutWidth1 Abit1*PW2OutNum1//output bandwidth
#define ScaleBit1 16


//software parameters
#define R1 14
#define C1 14
#define N1 128
#define E  6
#define M1 E*N1

#define SIZE1 N1/DwTn1*Tr1*Tc1
#define SIZE2 N1/PwTn1*Tr1*Tc1


// this method can specify the deepth of axi-interface
extern "C"{
void FusedDW_PW_InMode(
					   ap_uint<InWidth1>  In_ddrsrc[R1*C1*N1/DWinNum1],
					   ap_uint<DwWidth1>  Wt7x7_ddrsrc[N1/DWwtNum1*7*7],
					   ap_uint<Pw1Width1> Wt1x1_ddrsrc[M1*N1/PW1wtNum1],
					   ap_uint<Pw2Width1> Wt2_ddrsrc[N1*M1/PW2wtNum1],
					   ap_uint<OutWidth1> Branch_ddr[R1*C1*N1/PW2OutNum1],
					   ap_uint<OutWidth1> Out_ddr[R1*C1*N1/PW2OutNum1]
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
