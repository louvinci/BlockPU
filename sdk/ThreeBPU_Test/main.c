/*
 * main.c
 *
 *  Created on: 2016年8月20日
 *      Author: hsp
 *  本文件实现SD写入一段字符串，然后从其中读出并打印到串口。
 *
 */

#include <string.h>
#include "platform.h"
#include "xparameters.h"
#include"pl.h"
#include "xil_printf.h"
#include "ff.h"
#include"xil_cache.h"
#include "xtime_l.h"




static FATFS fatfs;

int SD_Init()
{
    FRESULT rc;

    rc = f_mount(&fatfs,"",0);
    if(rc)
    {
        xil_printf("ERROR : f_mount returned %d\r\n",rc);
        return 0;
    }
    return 1;
}

int SD_Transfer_read(char *FileName,u32 DestinationAddress,u32 ByteLength)
{
    FIL fil;
    FRESULT rc;
    UINT br;

    rc = f_open(&fil,FileName,FA_READ);
    if(rc)
    {
        xil_printf("ERROR : f_open returned %d\r\n",rc);
        return 0;
    }
    rc = f_lseek(&fil, 0);
    if(rc)
    {
        xil_printf("ERROR : f_lseek returned %d\r\n",rc);
        return 0;
    }
    rc = f_read(&fil, (void*)DestinationAddress,ByteLength,&br);
    if(rc)
    {
        xil_printf("ERROR : f_read returned %d\r\n",rc);
        return 0;
    }
    rc = f_close(&fil);
    if(rc)
    {
        xil_printf(" ERROR : f_close returned %d\r\n", rc);
        return 0;
    }
    return 1;
}

int SD_Transfer_write(char *FileName,u32 SourceAddress,u32 ByteLength)
{
    FIL fil;
    FRESULT rc;
    UINT bw;

    rc = f_open(&fil,FileName,FA_CREATE_ALWAYS | FA_WRITE);
    if(rc)
    {
        xil_printf("ERROR : f_open returned %d\r\n",rc);
        return 0;
    }
    rc = f_lseek(&fil, 0);
    if(rc)
    {
        xil_printf("ERROR : f_lseek returned %d\r\n",rc);
        return 0;
    }
    rc = f_write(&fil,(void*) SourceAddress,ByteLength,&bw);
    if(rc)
    {
        xil_printf("ERROR : f_write returned %d\r\n", rc);
        return 0;
    }
    rc = f_close(&fil);
    if(rc){
        xil_printf("ERROR : f_close returned %d\r\n",rc);
        return 0;
    }
    return 1;
}

void read_layer(int8_t *in,int8_t *norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* out,
		int R1, int C1,int M1, int N1,int K,char * suffix){
	char in_str[20] = "input",norm_str[20]="norm",dw_wt_str[20] = "dw_wt";
	char pw1_wt_str[20]="pw1_wt",pw2_wt_str[20]="pw2_wt",sw_res_str[20] = "sw_res";

	strcat(in_str,suffix);
	strcat(norm_str,suffix);
	strcat(dw_wt_str,suffix);
	strcat(pw1_wt_str,suffix);
	strcat(pw2_wt_str,suffix);
	strcat(sw_res_str,suffix);
	SD_Transfer_read(in_str,(u32)in,sizeof(int8_t)*(R1*C1*N1));
	SD_Transfer_read(norm_str,(u32)norm,sizeof(int)*(2*N1));
	SD_Transfer_read(dw_wt_str,(u32)dw_wt,sizeof(int8_t)*(K*K*N1));
	SD_Transfer_read(pw1_wt_str,(u32)pw1_wt,sizeof(int8_t)*(M1*N1));
	SD_Transfer_read(pw2_wt_str,(u32)pw2_wt,sizeof(int8_t)*(M1*N1));
	SD_Transfer_read(sw_res_str,(u32)out,sizeof(int8_t)*(R1*C1*N1));
}

void result_check(unsigned N,unsigned R,unsigned C, int8_t* out, int8_t* out_ref){
	int cnt=0;
	    for(int i=0;i<N*R*C;i++){
	    	if(out[i] != out_ref[i]){
	    		//printf("%d %d\r\n",out[i],out_ref[i]);
	    		cnt++;
	    	}
	    	//printf("%d,%d\n",out[i],out_ref[i]);
	    }
	    if(cnt==0){
	    	printf("=============Right Result=============\n");
	    }else{
	    	printf("========Error happens %d times=============\n",cnt);
	    }
}

void BPU1_Run(int8_t *in,int8_t *norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* out,int8_t* out_ref,
		int rescale1,int rescale2,int id_rescale,
		int R, int C,int M, int N,int K,int padN,int layer_id,int cnt){
    //printf("Start Testing Block %d \n",layer_id);
//    XTime tEnd, tCur;
//    u32 tUsed=0;
//    XTime_GetTime(&tCur);
//    for(int i=0;i<cnt;i++){
//    	BPU1(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N,K,padN);
//    }
//    XTime_GetTime(&tEnd);
//    tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
//    printf("total time elapsed is %d us\r\n",tUsed);
//    printf("time elapsed is %d us\r\n",tUsed/cnt);
//    result_check( N,R,C, out, out_ref);
//    printf("Test Block%d End\n",layer_id);
    BPU1(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N,K,padN);
}

void BPU2_Run(int8_t *in,int8_t *norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* out,int8_t* out_ref,
		int rescale1,int rescale2,int id_rescale,
		int R, int C,int M, int N,int K,int layer_id,int cnt){
    //printf("Start Testing Block %d \n",layer_id);
//    XTime tEnd, tCur;
//    u32 tUsed=0;
//    XTime_GetTime(&tCur);
//    for(int i=0;i<cnt;i++){
//    	BPU2(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N);
//    }
//    XTime_GetTime(&tEnd);
//    tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
//    printf("total time elapsed is %d us\r\n",tUsed);
//    printf("time elapsed is %d us\r\n",tUsed/cnt);
//    result_check( N,R,C, out, out_ref);
//    printf("Test Block%d End\n",layer_id);
    BPU2(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N);
}

void BPU3_Run(int8_t *in,int8_t *norm,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* out,int8_t* out_ref,
		int rescale1,int rescale2,int id_rescale,
		int R, int C,int M, int N,int K,int layer_id,int cnt){
    //printf("Start Testing Block %d \n",layer_id);
//    XTime tEnd, tCur;
//    u32 tUsed=0;
//    XTime_GetTime(&tCur);
//    for(int i=0;i<cnt;i++){
//    	BPU3(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N);
//    }
//    XTime_GetTime(&tEnd);
//    tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
//    printf("total time elapsed is %d us\r\n",tUsed);
//    printf("time elapsed is %d us\r\n",tUsed/cnt);
//    result_check( N,R,C, out, out_ref);
//    printf("Test Block%d End\n",layer_id);
    BPU3(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N);
}
#define FILE "test.txt"
#define CNT 50
////////////////////BPU3 parameters//////////////
#define MXN3 256
#define MXM3 256*6
#define MXR3 14
#define MXC3 14

////////////////////BPU2 parameters//////////////
#define MXN2 128
#define MXM2 128*6
#define MXR2 14
#define MXC2 14

////////////////////BPU1 parameters//////////////
#define MXN1 96
#define MXM1 96*4
#define MXR1 112
#define MXC1 112

int main()
{
    init_platform();
    Xil_DCacheDisable();
    SD_Init();
    // because in BPU1, the N would be less than the DwTn and exist "K=3", so we add the "padN" parameter to the addr stride.
    // (Note that the first input need padding)
    // K is tell the BPU1 the Kernel size.
    int rescale1=6783,rescale2=11715,id_rescale=16384;
    int R3,C3,N3,M3;
    int K3=7;
    R3=14;
    C3=14;
    N3=128;
    M3=128*6;
    int8_t* in     =(int8_t*)memalign(8,MXR3*MXC3*MXN3*sizeof(int8_t));
    int* norm      =(int8_t*)memalign(8,2*MXN3*sizeof(int));
    int8_t* dw_wt  =(int8_t*)memalign(8,K3*K3*MXN3*sizeof(int8_t));
    int8_t* pw1_wt =(int8_t*)memalign(8,MXM3*MXN3*sizeof(int8_t));
    int8_t* pw2_wt =(int8_t*)memalign(8,MXM3*MXN3*sizeof(int8_t));
    int8_t* out    =(int8_t*)memalign(8,MXR3*MXC3*MXN3*sizeof(int8_t));
    int8_t* out_ref=(int8_t*)memalign(8,MXR3*MXC3*MXN3*sizeof(int8_t));

    int R2,C2,N2,M2;
    int K2=7;
    R2=14;
    C2=14;
    N2=96;
    M2=96*6;
    int8_t* in2     =(int8_t*)memalign(8,MXR2*MXC2*MXN2*sizeof(int8_t));
    int* norm2      =(int8_t*)memalign(8,2*MXN2*sizeof(int));
    int8_t* dw_wt2  =(int8_t*)memalign(8,K2*K2*MXN2*sizeof(int8_t));
    int8_t* pw1_wt2 =(int8_t*)memalign(8,MXM2*MXN2*sizeof(int8_t));
    int8_t* pw2_wt2 =(int8_t*)memalign(8,MXM2*MXN2*sizeof(int8_t));
    int8_t* out2    =(int8_t*)memalign(8,MXR2*MXC2*MXN2*sizeof(int8_t));
    int8_t* out_ref2=(int8_t*)memalign(8,MXR2*MXC2*MXN2*sizeof(int8_t));

    int R1,C1,N1,M1;
    int K1=7,padN=0;
    R1=28;
    C1=28;
    N1=64;
    M1=64*6;
    int8_t* in1      =(int8_t*)memalign(8,MXR1*MXC1*MXN1*sizeof(int8_t));
    int* norm1       =(int8_t*)memalign(8,2*MXN1*sizeof(int));
    int8_t* dw_wt1   =(int8_t*)memalign(8,K1*K1*MXN1*sizeof(int8_t));
    int8_t* pw1_wt1  =(int8_t*)memalign(8,MXM1*MXN1*sizeof(int8_t));
    int8_t* pw2_wt1  =(int8_t*)memalign(8,MXM1*MXN1*sizeof(int8_t));
    int8_t* out1     =(int8_t*)memalign(8,MXR1*MXC1*MXN1*sizeof(int8_t));
    int8_t* out_ref1 =(int8_t*)memalign(8,MXR1*MXC1*MXN1*sizeof(int8_t));

//    pl_init_Q3();
//    printf("==========Test BPU3==========\r\n");
//    read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R3,C3,M3,N3,K3,"31.bin");
//    BPU3_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R3,C3,M3,N3,K3,1,CNT);

//    R3=7;
//    C3=7;
//    N3=256;
//    M3=256*4;
//    read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R3,C3,M3,N3,K3,"3_2.bin");
//    BPU3_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R3,C3,M3,N3,K3,2,CNT);


//    pl_init_Q2();
//    printf("==========Test BPU2==========\r\n");
//    read_layer(in2,norm2,dw_wt2,pw1_wt2,pw2_wt2,out_ref2,R2,C2,M2,N2,K2,"21.bin");
//    BPU2_Run(in2,norm2,dw_wt2,pw1_wt2,pw2_wt2,out2,out_ref2,rescale1,rescale2,id_rescale,R2,C2,M2,N2,K2,1,CNT);

//    N2=128;
//    M2=128*4;
//    read_layer(in2,norm2,dw_wt2,pw1_wt2,pw2_wt2,out_ref2,R2,C2,M2,N2,K2,"2_2.bin");
//    BPU2_Run(in2,norm2,dw_wt2,pw1_wt2,pw2_wt2,out2,out_ref2,rescale1,rescale2,id_rescale,R2,C2,M2,N2,K2,2,CNT);

    // this time we didn't test the situation that need padding
//    pl_init_Q1();
//    printf("==========Test BPU1==========\r\n");
//    read_layer(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out_ref1,R1,C1,M1,N1,K1,"12.bin");
//    BPU1_Run(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out1,out_ref1,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K1,padN,1,CNT);


//    M1=64*4;
//    read_layer(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out_ref1,R1,C1,M1,N1,K1,"1_4.bin");
//    BPU1_Run(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out1,out_ref1,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K1,padN,2,CNT);

    // parallel
    pl_init_Q3();
    pl_init_Q2();
    pl_init_Q1();
    read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R3,C3,M3,N3,K3,"31.bin");
    read_layer(in2,norm2,dw_wt2,pw1_wt2,pw2_wt2,out_ref2,R2,C2,M2,N2,K2,"21.bin");
    read_layer(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out_ref1,R1,C1,M1,N1,K1,"12.bin");
    printf("Data Loaded \r\n");
    XTime tEnd, tCur;
	u32 tUsed=0;
	XTime_GetTime(&tCur);
	for(int i=0;i<CNT;i++){
	    BPU3_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R3,C3,M3,N3,K3,1,CNT);
	    BPU2_Run(in2,norm2,dw_wt2,pw1_wt2,pw2_wt2,out2,out_ref2,rescale1,rescale2,id_rescale,R2,C2,M2,N2,K2,2,CNT);
	    BPU1_Run(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out1,out_ref1,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K1,padN,2,CNT);
	    QBPU321_IsDone();
	}
    XTime_GetTime(&tEnd);
	tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
	printf("total time elapsed is %d us\r\n",tUsed);
	printf("time elapsed is %d us\r\n",tUsed/CNT);

	result_check( N3,R3,C3, out, out_ref);
	result_check( N2,R2,C2, out2, out_ref2);
	result_check( N1,R1,C1, out1, out_ref1);
    cleanup_platform();
    return 0;
}
