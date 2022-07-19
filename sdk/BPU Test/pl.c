#include"pl.h"

void pl_init(){
	XFuseddw_pw_inmode_Initialize(&hls_inst, 0);
}

void test(int8_t* in,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out){

	XFuseddw_pw_inmode_Set_In_ddrsrc(&hls_inst, (u32)in);
	XFuseddw_pw_inmode_Set_Wt7x7_ddrsrc(&hls_inst, (u32)dw_wt);
	XFuseddw_pw_inmode_Set_Wt1x1_ddrsrc(&hls_inst, (u32)pw1_wt);
	XFuseddw_pw_inmode_Set_Wt2_ddrsrc(&hls_inst, (u32)pw2_wt);
	XFuseddw_pw_inmode_Set_Branch_ddr(&hls_inst,(u32)branch);
	XFuseddw_pw_inmode_Set_Out_ddr(&hls_inst, (u32)out);

	XFuseddw_pw_inmode_Start(&hls_inst);
	while(XFuseddw_pw_inmode_IsDone(&hls_inst)==0);
}
