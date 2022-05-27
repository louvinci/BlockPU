#include <iostream>
#include<fstream>
#include<stdlib.h>
#include <stdio.h>
#include "hls_stream.h"
#include"block.h"
//using namespace std;

#define K 7
// top interface type
typedef ap_uint<InWidth1> ABWtype;
typedef ap_uint<DwWidth1> WBWtype;
typedef ap_uint<Pw1Width1> WBWtype2;
typedef ap_uint<Pw2Width1> WBWtype3;
typedef ap_uint<OutWidth1> OBWtype;

typedef ap_uint<Abit1> ADtype;
typedef ap_uint<Wbit1> WDtype;



//template <typename T>
//T* aligned_alloc(std::size_t num) {
//    void* ptr = NULL;
//
//    if (posix_memalign(&ptr, 4096, num * sizeof(T)) ) throw std::bad_alloc();
//    // ptr = (void*)malloc(num * sizeof(T));
//    return reinterpret_cast<T*>(ptr);
//}


int main(){
	
	ADtype    in[R1*C1*N1];
	WDtype    dw_wt[N1/DwTn1*K*K*DwTn1];
	WDtype    pw1_wt[M1*N1];// M*N format haven't packed to M1/Tm*N1/PwTn
	WDtype    pw2_wt[M1*N1];// N*M format haven't packed to M1/Tm*N1/PwTn
	ADtype    sw_res[R1*C1*N1];
	ADtype    hw_res[R1*C1*N1];

//std::cout<<sizeof( ap_uint<32> )<<" "<<sizeof( ap_uint<96> )<<std::endl;

    std::cout<<"INPUT DATALOAD IN"<<std::endl;
    std::ifstream infile("input.bin",std::ios::in | std::ios::binary);
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


    std::cout<<"DW WEIGHT DATALOAD IN"<<std::endl;
    std::ifstream wtfile("dw_weight.bin",std::ios::in | std::ios::binary);
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


    std::cout<<"PW1 WEIGHT DATALOAD IN"<<std::endl;
    std::ifstream pwwtfile("pw1_weight.bin",std::ios::in | std::ios::binary);
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


    std::cout<<"PW2 WEIGHT DATALOAD IN"<<std::endl;
    std::ifstream pwwtfile2("pw2_weight.bin",std::ios::in | std::ios::binary);
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


    std::cout<<"Hardware Compute"<<std::endl;
    FusedDW_PW_InMode((ABWtype*)in,(WBWtype*)dw_wt,(WBWtype2*)pw1_wt,(WBWtype3*)pw2_wt,(OBWtype*)hw_res);


    std::cout<<"Ref Result LOAD"<<std::endl;
    //RxCxM
    std::ifstream resfile("hls_res.bin",std::ios::in | std::ios::binary);
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
            			std::cout<<"R BUG "<<k<<","<<i<<","<<j<<","<<ii<<" "<<tmp<<" "<<tmp2<<"   ";
#endif
            		}
            	}

            }
            //std::cout<<std::endl;
       }

    if(w_cnt!=0){
    	std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;
    	std::cout<<"+++++++++++++++Exist Wrong Result++++++++++++++"<<std::endl;
    	std::cout<<"Wrong Result happens "<<w_cnt<<" times"<<std::endl;
    	std::cout<<"+++++++++++++++++++++++++++++++++++++++++++++++"<<std::endl;

    }else{
    	std::cout<<"=================================================="<<std::endl;
    	std::cout<<"==================Right Result===================="<<std::endl;
    	std::cout<<"=================================================="<<std::endl;

    }
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



