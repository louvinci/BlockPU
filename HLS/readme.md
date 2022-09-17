## 简要说明
HLS生成硬件的源代码，分别包含了BPU1、BPU2、BPU3三个异构子核心

基本上都使用一套模板函数，但是部分函数在不同实例上，选择了固定的实现。因此，另建立一个文件夹。    
由于是多核，这里top函数的命名也需要不一样，否则```vivado block design```时难以区分。   
testbench中```tb.py```中的硬件参数也不同，因此另立文件夹比较合适  

## 注意事项

- csim以及co-sim时，需要修改```dw_engine.h```中 ```reducedload_axi```以及```expandload3x3_axi```函数中**条件加载语句**。   
具体来讲，这里为了使能 ```Burst```访问，采用了越界模式，多读取了一些不重要的数据(未被使用)，上板验证是正确的。  
- 将```Pipeline```pragma提前也是为了方便综合器识别，将loop进行展开以避免多个axi-burst setup latency
```C++
DwIN_R:
	for(unsigned short  r = 0; r < Tr+6; r++) {
		DwIN_C:
		for(unsigned short c = 0; c < Tc+6; c++) {
#pragma HLS PIPELINE II=8 // 这里II=8是为了方便综合器识别连续访问。
			ap_uint<InWidth>  * brust_addr = ddr_burst + (row+r-3)*offsetR*tnloops + (col+c-3)*offsetC*tnloops + ch*tnloops;
			DwIN_P1:
			for(unsigned short tnn =0;tnn < tnloops;tnn++){
				ap_uint<InWidth> DATA;
				//DATA = brust_addr[tnn];  这里将数据访问放在条件语句之外，以使能Burst访问。由于数组越界，不能进行co-sim，但是不影响实际部署的正确性。
				if (row+r<3 | row+r> R+2 | col +c<3 | col+c>C+2){
					for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
						buf[tnn*NUM+tn][r][c]= 0;
					}
				}else{
					//DATA = brust_addr[tnn]; 早期验证功能正确性时，可以写在条件循环里面。
					for(unsigned char tn = 0; tn < NUM; tn++) {
#pragma HLS UNROLL
						buf[tnn*NUM+tn][r][c].range(Abit-1, 0) = DATA.range( (tn+1)*Abit-1, tn*Abit);
					}
				}
			}
			//memcpy(tmp_buf[r][c],brust_addr,tnloops*sizeof(InWidth));

		}
	}
```


