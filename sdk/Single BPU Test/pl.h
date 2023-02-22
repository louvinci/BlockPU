#include"xqbpu1.h"
// this file can be found in the 'hw/drivers/QBPU1_v1_0/src'
typedef signed char int8_t;

static XQbpu1 hls_inst_Q1;

void pl_init_Q1();

void BPU1(int8_t* in,int8_t* norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		int rescale1, int rescale2,int id_rescale,
		int R,int C,int M,int N,int K, int PadN);
