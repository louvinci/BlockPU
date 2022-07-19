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

#define K 7
#define R1 14
#define C1 14
#define N1 128
#define M1 128*6

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

void read_data(int8_t *in,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* out){
	SD_Transfer_read("input.bin",(u32)in,sizeof(int8_t)*(R1*C1*N1));
	//SD_Transfer_read("input.bin",(u32)branch,sizeof(int8_t)*(R1*C1*N1));
	SD_Transfer_read("dw_wt.bin",(u32)dw_wt,sizeof(int8_t)*(K*K*N1));
	SD_Transfer_read("pw1_wt.bin",(u32)pw1_wt,sizeof(int8_t)*(M1*N1));
	SD_Transfer_read("pw2_wt.bin",(u32)pw2_wt,sizeof(int8_t)*(M1*N1));
	SD_Transfer_read("hls_res.bin",(u32)out,sizeof(int8_t)*(R1*C1*N1));
}

#define FILE "test.txt"
#define N 50
int main()
{
    init_platform();
    Xil_DCacheDisable();
    SD_Init();
    //
    int8_t* in=(int8_t*)memalign(8,R1*C1*N1*sizeof(int8_t));
    int8_t* dw_wt=(int8_t*)memalign(8,K*K*N1*sizeof(int8_t));
    int8_t* pw1_wt=(int8_t*)memalign(8,M1*N1*sizeof(int8_t));
    int8_t* pw2_wt=(int8_t*)memalign(8,M1*N1*sizeof(int8_t));
    int8_t* out=(int8_t*)memalign(8,R1*C1*N1*sizeof(int8_t));
    int8_t* out_ref=(int8_t*)memalign(8,R1*C1*N1*sizeof(int8_t));
    read_data(in,dw_wt,pw1_wt,pw2_wt,out_ref);
    //void test(int8_t* in,int8_t* dw_wt,int8_t* pw1_wt,int8_t* pw2_wt,int8_t* out);
    pl_init();
    printf("Begin Test\n");
    XTime tEnd, tCur;
    u32 tUsed=0;
    XTime_GetTime(&tCur);
    for(int i=0;i<N;i++){
    	test(in,dw_wt,pw1_wt,pw2_wt,in,out);
    }
    XTime_GetTime(&tEnd);
    tUsed = ((tEnd-tCur)*1000000)/(COUNTS_PER_SECOND);
    printf("total time elapsed is %d us\r\n",tUsed);
    printf("time elapsed is %d us\r\n",tUsed/N);
    int cnt=0;
    for(int i=0;i<N1*R1*C1;i++){
    	if(out[i] != out_ref[i]){
    		cnt++;
    	}
    	//printf("%d,%d\n",out[i],out_ref[i]);
    }
    if(cnt==0){
    	printf("=============Right Result=============\n");
    }else{
    	printf("========Error happens %d times=============\n",cnt);
    }
    printf("Test end\n");
    cleanup_platform();
    return 0;
}
