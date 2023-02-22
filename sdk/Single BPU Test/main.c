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
    printf("Start Testing Block %d \n",layer_id);
    XTime tEnd, tCur;
    u32 tUsed=0;
    XTime_GetTime(&tCur);
    for(int i=0;i<cnt;i++){
    	BPU1(in,norm,dw_wt,pw1_wt,pw2_wt,in,out,rescale1,rescale2,id_rescale,R,C,M,N,K,padN);
    }
    XTime_GetTime(&tEnd);
    tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
    printf("total time elapsed is %d us\r\n",tUsed);
    printf("time elapsed is %d us\r\n",tUsed/cnt);
    result_check( N,R,C, out, out_ref);
    printf("Test Block%d End\n",layer_id);
}
#define FILE "test.txt"
#define CNT 50
#define MXN1 256
#define MXM1 256*6
#define MXR1 14
#define MXC1 14r
int main()
{
    init_platform();
    Xil_DCacheDisable();
    SD_Init();
    //
    int rescale1=6783,rescale2=11715,id_rescale=16384;
    int R1,C1,N1,M1;
    int K1=7,padN=0;
    R1=28;
    C1=28;
    N1=64;
    M1=64*6;
    int8_t* in1       = (int8_t*)memalign(8,MXR1*MXC1*MXN1*sizeof(int8_t));
    int* norm1        = (int8_t*)memalign(8,2*MXN1*sizeof(int));
    int8_t* dw_wt1    = (int8_t*)memalign(8,K1*K1*MXN1*sizeof(int8_t));
    int8_t* pw1_wt1   = (int8_t*)memalign(8,MXM1*MXN1*sizeof(int8_t));
    int8_t* pw2_wt1   = (int8_t*)memalign(8,MXM1*MXN1*sizeof(int8_t));
    int8_t* out1      = (int8_t*)memalign(8,MXR1*MXC1*MXN1*sizeof(int8_t));
    int8_t* out_ref1  = (int8_t*)memalign(8,MXR1*MXC1*MXN1*sizeof(int8_t));

    pl_init_Q1();

    read_layer(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out_ref1,R1,C1,M1,N1,K1,"12.bin");

    BPU1_Run(in1,norm1,dw_wt1,pw1_wt1,pw2_wt1,out1,out_ref1,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K1,padN,2,CNT);

//    M1=128*3;
//    read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R1,C1,M1,N1,K,"2.bin");
//    BPU_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K,2,CNT);
//
//    M1 = 128*4;
//    read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R1,C1,M1,N1,K,"3.bin");
//    BPU_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K,3,CNT);
//
//    R1=7;
//    C1=7;
//    N1=256;
//    M1 = 256*6;
//    read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R1,C1,M1,N1,K,"4.bin");
//    BPU_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K,4,CNT);
//
//    M1 = 256*4;
//	read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R1,C1,M1,N1,K,"5.bin");
//	BPU_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K,5,CNT);
//
//    M1 = 256*3;
//	read_layer(in,norm,dw_wt,pw1_wt,pw2_wt,out_ref,R1,C1,M1,N1,K,"6.bin");
//	BPU_Run(in,norm,dw_wt,pw1_wt,pw2_wt,out,out_ref,rescale1,rescale2,id_rescale,R1,C1,M1,N1,K,6,CNT);

    cleanup_platform();
    return 0;
}
