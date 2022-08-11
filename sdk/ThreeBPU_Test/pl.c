#include"pl.h"

void pl_init_Q1(){
	XQbpu1_Initialize(&hls_inst_Q1, 0);
}
void pl_init_Q2(){
	XQbpu2_Initialize(&hls_inst_Q2, 0);
}
void pl_init_Q3(){
	XQbpu3_Initialize(&hls_inst_Q3, 0);
}

void BPU1(int8_t* in, int8_t * norm, int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		  int rescale1, int rescale2,int id_rescale,
		  int R,int C,int M,int N,int K, int PadN){

	XQbpu1_Set_In_ddrsrc(&hls_inst_Q1, (u32)in);
	XQbpu1_Set_Normq_ddrsrc(&hls_inst_Q1,(u32)norm);
	XQbpu1_Set_Wt7x7_ddrsrc(&hls_inst_Q1, (u32)dw_wt);
	XQbpu1_Set_Wt1_ddrsrc(&hls_inst_Q1, (u32)pw1_wt);
	XQbpu1_Set_Wt2_ddrsrc(&hls_inst_Q1, (u32)pw2_wt);
	XQbpu1_Set_Branch_ddr(&hls_inst_Q1,(u32)branch);
	XQbpu1_Set_Out_ddr(&hls_inst_Q1, (u32)out);

	XQbpu1_Set_rescale1(&hls_inst_Q1, (u32)rescale1);
	XQbpu1_Set_rescale2(&hls_inst_Q1, (u32)rescale2);
	XQbpu1_Set_id_rescale(&hls_inst_Q1, (u32)id_rescale);
	XQbpu1_Set_R1(&hls_inst_Q1, (u32)R);
	XQbpu1_Set_C1(&hls_inst_Q1, (u32)C);
	XQbpu1_Set_M1(&hls_inst_Q1, (u32)M);
	XQbpu1_Set_N1(&hls_inst_Q1, (u32)N);
	XQbpu1_Set_K1(&hls_inst_Q1, (u32)K);
	XQbpu1_Set_PadN1(&hls_inst_Q1, (u32)PadN);


	XQbpu1_Start(&hls_inst_Q1);
	//while(XQbpu1_IsDone(&hls_inst_Q1)==0);
}

void BPU2(int8_t* in, int8_t * norm, int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		  int rescale1, int rescale2,int id_rescale,
		  int R,int C,int M,int N){

	XQbpu2_Set_In_ddrsrc(&hls_inst_Q2, (u32)in);
	XQbpu2_Set_Normq_ddrsrc(&hls_inst_Q2,(u32)norm);
	XQbpu2_Set_Wt7x7_ddrsrc(&hls_inst_Q2, (u32)dw_wt);
	XQbpu2_Set_Wt1_ddrsrc(&hls_inst_Q2, (u32)pw1_wt);
	XQbpu2_Set_Wt2_ddrsrc(&hls_inst_Q2, (u32)pw2_wt);
	XQbpu2_Set_Branch_ddr(&hls_inst_Q2,(u32)branch);
	XQbpu2_Set_Out_ddr(&hls_inst_Q2, (u32)out);

	XQbpu2_Set_rescale1(&hls_inst_Q2, (u32)rescale1);
	XQbpu2_Set_rescale2(&hls_inst_Q2, (u32)rescale2);
	XQbpu2_Set_id_rescale(&hls_inst_Q2, (u32)id_rescale);
	XQbpu2_Set_R1(&hls_inst_Q2, (u32)R);
	XQbpu2_Set_C1(&hls_inst_Q2, (u32)C);
	XQbpu2_Set_M1(&hls_inst_Q2, (u32)M);
	XQbpu2_Set_N1(&hls_inst_Q2, (u32)N);


	XQbpu2_Start(&hls_inst_Q2);
	//while(XQbpu2_IsDone(&hls_inst_Q2)==0);
}

void BPU3(int8_t* in, int8_t * norm, int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		  int rescale1, int rescale2,int id_rescale,
		  int R,int C,int M,int N){

	XQbpu3_Set_In_ddrsrc(&hls_inst_Q3, (u32)in);
	XQbpu3_Set_Normq_ddrsrc(&hls_inst_Q3,(u32)norm);
	XQbpu3_Set_Wt7x7_ddrsrc(&hls_inst_Q3, (u32)dw_wt);
	XQbpu3_Set_Wt1_ddrsrc(&hls_inst_Q3, (u32)pw1_wt);
	XQbpu3_Set_Wt2_ddrsrc(&hls_inst_Q3, (u32)pw2_wt);
	XQbpu3_Set_Branch_ddr(&hls_inst_Q3,(u32)branch);
	XQbpu3_Set_Out_ddr(&hls_inst_Q3, (u32)out);

	XQbpu3_Set_rescale1(&hls_inst_Q3, (u32)rescale1);
	XQbpu3_Set_rescale2(&hls_inst_Q3, (u32)rescale2);
	XQbpu3_Set_id_rescale(&hls_inst_Q3, (u32)id_rescale);
	XQbpu3_Set_R1(&hls_inst_Q3, (u32)R);
	XQbpu3_Set_C1(&hls_inst_Q3, (u32)C);
	XQbpu3_Set_M1(&hls_inst_Q3, (u32)M);
	XQbpu3_Set_N1(&hls_inst_Q3, (u32)N);


	XQbpu3_Start(&hls_inst_Q3);
	//while(XQbpu3_IsDone(&hls_inst_Q3)==0);
}

void QBPU321_IsDone(){
	while(XQbpu3_IsDone(&hls_inst_Q3)==0);
	while(XQbpu2_IsDone(&hls_inst_Q2)==0);
	while(XQbpu1_IsDone(&hls_inst_Q1)==0);
}
