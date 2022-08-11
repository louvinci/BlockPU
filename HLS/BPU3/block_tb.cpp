#include <iostream>
#include<fstream>
#include<stdlib.h>
#include <stdio.h>
#include "hls_stream.h"
#include"block.h"
//using namespace std;

#define K 7
// top interface type
typedef ap_uint<InWidth1>       ABWtype;
typedef ap_uint<DwWidth1>       WBWtype;
typedef ap_uint<Pw1Width1>      WBWtype2;
typedef ap_uint<Pw2Width1>      WBWtype3;
typedef ap_uint<OutWidth1>      OBWtype;
typedef ap_uint<NormWidth1>     NWtype;


//read bin type
typedef ap_uint<Abit1>          ADtype;
typedef ap_uint<Wbit1>          WDtype;
typedef ap_uint<32>             Uint32;


// linux system
//template <typename T>
//T* aligned_alloc(std::size_t num) {
//    void* ptr = NULL;
//
//    if (posix_memalign(&ptr, 4096, num * sizeof(T)) ) throw std::bad_alloc();
//    // ptr = (void*)malloc(num * sizeof(T));
//    return reinterpret_cast<T*>(ptr);
//}


int main(){
	//software parameters, this BPU covers the blocks near the output end.

	//unsigned R1=14,C1=14,N1=128,M1=128*6;
	unsigned R1=7,C1=7,N1=256,M1=256*4;
	
	ADtype*   in        =    new ADtype[MXR1*MXC1*MXN1];
    Uint32*   norm_wt   =    new Uint32[NormSize1*2];
	WDtype*   dw_wt     =    new WDtype[MXN1*K*K];
	WDtype*   pw1_wt    =    new WDtype[MXM1*MXN1];
	WDtype*   pw2_wt    =    new WDtype[MXM1*MXN1];
	ADtype*   sw_res    =    new ADtype[MXR1*MXC1*MXN1];
	ADtype*   hw_res    =    new ADtype[MXR1*MXC1*MXN1];

	//this value is decreased by 10 times, due to the random value more easily to overflow
	ap_uint<ScaleBit1>     rescale1   = 6783;//173644;
	ap_uint<ScaleBit1>     rescale2   = 11715;//299893;
	ap_uint<ScaleBit1>     id_rescale = 16384;

    std::cout<<"INPUT DATA IN...  "<<R1<<"x"<<C1<<"x"<<N1<<std::endl;
    std::ifstream infile("input.bin",std::ios::in | std::ios::binary);
    if (!infile) {
        std::cout << "Error : " << " Input file doesn't exist !" << std::endl;
        exit(1);
    }
    infile.read((char*) (in), R1*C1*N1*sizeof(ADtype));

#ifdef DEBUG
    for(int k=0;k<R1;k++){
          for(int i =0;i<C1;i++){
              for(int j=0;j<N1;j++){
            	ap_int<Abit1> tmp;
              	tmp.range(Abit1-1,0)  = in[k*C1*N1 + i*N1 +j].range(Abit1-1,0);
              	std::cout<<tmp<<" ";

              }
              std::cout<<std::endl;
          }
      std::cout<<std::endl;
    }

#endif


    std::cout<<"DW WEIGHT LOAD IN..."<<std::endl;
    std::ifstream wtfile("dw_wt.bin",std::ios::in | std::ios::binary);
    if (!wtfile) {
        std::cout << "Error : " << " DW Weight file doesn't exist !" << std::endl;
        exit(1);
    }
    wtfile.read((char*) dw_wt, N1*K*K*sizeof(WDtype));

    #ifdef DEBUG
    for(int k=0;k<N1/DwTn1;k++)
    {
        for(int i =0;i<K;i++){
            for(int j=0;j<K;j++){
            	for(int ii=0;ii<DwTn1;ii++){
            		ap_int<Wbit1> wt;
            		wt.range(Wbit1-1,0)=dw_wt[k*(K*K*DwTn1)+i*(K*DwTn1)+j*DwTn1+ii].range(Wbit1-1,0);
            		std::cout << wt<< ",";
            	}
            	std::cout<<std::endl;
            }
            std::cout<<std::endl;
        }
        std::cout<<std::endl;
    }
    #endif // DEBUG
    
    std::cout<<"NORM WEIGHT LOAD IN..."<<std::endl;
    std::ifstream normfile("norm.bin",std::ios::in | std::ios::binary);
    if (!normfile) {
        std::cout << "Error : " << " NORM Weight file doesn't exist !" << std::endl;
        exit(1);
    }
    normfile.read((char*) norm_wt, 2*N1*sizeof(Uint32));


    std::cout<<"PW1 WEIGHT LOAD IN..."<<M1<<"x"<<N1<<std::endl;
    std::ifstream pwwtfile("pw1_wt.bin",std::ios::in | std::ios::binary);
    if (!pwwtfile) {
        std::cout << "Error : " << " PW1 Weight file doesn't exist !" << std::endl;
        exit(1);
    }
    pwwtfile.read((char*)pw1_wt, M1*N1*sizeof(WDtype));
	#ifdef DEBUG
		for(int j=0;j<M1/Tm1;j++)
		{
			for(int i =0;i<N1/PwTn1;i++){
				for(unsigned ii=0;ii<Tm1;ii++){
					for(unsigned jj=0;jj<PwTn1;jj++){
						ap_int<Wbit1> wt;
						wt.range(Wbit1-1,0) = pw1_wt[(j*N1/PwTn1+i)*Tm1*PwTn1+ii*PwTn1+jj].range(Wbit1-1,0);
						std::cout <<wt << ", ";
					}
					std::cout<<std::endl;
				}
				std::cout<<std::endl;
			}
			std::cout<<std::endl;
		}
	#endif // DEBUG


    std::cout<<"PW2 WEIGHT LOAD IN..."<<std::endl;
    std::ifstream pwwtfile2("pw2_wt.bin",std::ios::in | std::ios::binary);
    if (!pwwtfile2) {
        std::cout << "Error : " << " PW2 Weight file doesn't exist !" << std::endl;
        exit(1);
    }
    pwwtfile2.read((char*)pw2_wt,  M1*N1*sizeof(WDtype));
	#ifdef DEBUG
		for(int j=0;j<M1/Tm1;j++)
		{
			for(int i =0;i<N1/PwTn1;i++){
				for(unsigned ii=0;ii<Tm1;ii++){
					for(unsigned jj=0;jj<PwTn1;jj++){
						ap_int<Wbit1> wt;
						wt.range(Wbit1-1,0) = pw2_wt[(j*N1/PwTn1+i)*Tm1*PwTn1+ii*PwTn1+jj].range(Wbit1-1,0);
						std::cout <<wt << ", ";
					}
					std::cout<<std::endl;
				}
				std::cout<<std::endl;
			}
			std::cout<<std::endl;
		}
	#endif // DEBUG


    std::cout<<"<----Hardware Computing---->"<<std::endl;
    QBPU3((ABWtype*)in,(NWtype*)norm_wt,(WBWtype*)dw_wt,(WBWtype2*)pw1_wt,(WBWtype3*)pw2_wt,(OBWtype*)in,(OBWtype*)hw_res,
    		           rescale1,rescale2,id_rescale,R1,C1,M1,N1);


    std::cout<<"Ref Result LOAD..."<<std::endl;
    //RxCxM
    std::ifstream resfile("sw_res.bin",std::ios::in | std::ios::binary);
    if (!resfile) {
        std::cout << "Error : " << " Result file doesn't exist !" << std::endl;
        exit(1);
    }
    resfile.read((char*) sw_res, R1*C1*N1*sizeof(ADtype));

    int w_cnt=0;

    for(int k=0;k<R1;k++){
        for(int i =0;i<C1;i++){
            for(int j=0;j<N1;j++){
            	ap_int<Abit1> tmp,tmp2;
            	tmp.range(Abit1-1,0)  = sw_res[k*C1*N1+i*N1+j].range(Abit1-1,0);
            	tmp2.range(Abit1-1,0) = hw_res[k*C1*N1+i*N1+j].range(Abit1-1,0);
            	if (tmp!= tmp2){
            		w_cnt+=1;
#ifdef DEBUG
            			std::cout<<k<<","<<i<<","<<j<<" "<<tmp<<" "<<tmp2<<"   "<<std::endl;
#endif
            		}
            	}

            }
            //std::cout<<std::endl;
       }

    if(w_cnt!=0){
    	std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
    	std::cout<<"+++++++++++++++Exist Wrong Result++++++++++++++"<<std::endl;
    	std::cout<<"++++++Wrong Result happens "<<w_cnt<<" times++++++"<<std::endl;
    	std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;

    }else{
    	std::cout<<"=================================================="<<std::endl;
    	std::cout<<"==================Right Result===================="<<std::endl;
    	std::cout<<"=================================================="<<std::endl;

    }
      ap_int<8> a= -11,b=10,c=11;
      std::cout<< (a>>1) <<" "<< (b>>1) <<" "<<(c>>1)<<std::endl;
//    ap_fixed<16,6> scale ;// 8-bit integer
//    scale = 133.45; // because the sign bit, integer range:[-128,127]
//    std::cout<<"scale1: "<<scale<<std::endl;
//    scale = 12.45; // because the sign bit, integer range:[-128,127]
//    std::cout<<"scale2: "<<scale<<std::endl;
//    scale = 0.002123;
//    std::cout<<"scale3: "<<scale<<std::endl;
//    scale = 0.00523;
//    std::cout<<"scale4: "<<scale<<std::endl;
//    scale = -0.2123;
//    std::cout<<"scale5: "<<scale<<std::endl;

}


