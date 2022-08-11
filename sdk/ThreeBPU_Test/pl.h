#include"xqbpu1.h"
#include"xqbpu2.h"
#include"xqbpu3.h"

typedef signed char int8_t;

static XQbpu1 hls_inst_Q1;
static XQbpu2 hls_inst_Q2;
static XQbpu3 hls_inst_Q3;

void pl_init_Q1();
void pl_init_Q2();
void pl_init_Q3();
void QBPU321_IsDone();

void BPU1(int8_t* in,int8_t* norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		int rescale1, int rescale2,int id_rescale,
		int R,int C,int M,int N,int K, int PadN);

void BPU2(int8_t* in,int8_t* norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		int rescale1, int rescale2,int id_rescale,
		int R,int C,int M,int N);

void BPU3(int8_t* in,int8_t* norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out,
		int rescale1, int rescale2,int id_rescale,
		int R,int C,int M,int N);
