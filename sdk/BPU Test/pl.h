#include"xfuseddw_pw_inmode.h"

typedef signed char int8_t;

static XFuseddw_pw_inmode hls_inst;

void pl_init();

void test(int8_t* in,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* branch,int8_t* out);
