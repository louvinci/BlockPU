#include <ap_fixed.h>


//hardware parameters
#define Tr1 7
#define Tc1 7
#define DwTn1 8
#define PwTn1 8
#define Tm1 2
#define Abit1 8
#define Wbit1 8
#define Accbit1 16
#define MXN1 64

#define DWinNum DwTn1
#define DWwtNum DwTn1
#define PW1wtNum PwTn1
#define PW2wtNum PwTn1
#define PW2OutNum 4

#define InWidth1  Abit1*DWinNum // axi4 datawidth
#define DwWidth1  Wbit1*DWwtNum
#define Pw1Width1 Wbit1*PW1wtNum
#define Pw2Width1 Wbit1*PW2wtNum
#define OutWidth1 Abit1*PW2OutNum//output bandwidth

//software parameters
#define R1 14
#define C1 14
#define N1 16
#define E  1
#define M1 E*N1

#define SIZE1 N1/DwTn1*Tr1*Tc1
#define SIZE2 N1/PwTn1*Tr1*Tc1


// this method can specify the deepth of axi-interface
extern "C"{
void FusedDW_PW_InMode(
					   ap_uint<InWidth1> In_ddrsrc[R1*C1*N1/DWinNum],
					   ap_uint<DwWidth1> Wt7x7_ddrsrc[N1/DWwtNum*7*7],
					   ap_uint<Pw1Width1> Wt1x1_ddrsrc[M1*N1/PW1wtNum],
					   ap_uint<Pw2Width1> Wt2_ddrsrc[N1*M1/PW2wtNum],
					   ap_uint<OutWidth1> Out_ddr[R1*C1*N1/PW2OutNum]
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
