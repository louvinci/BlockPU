# BlockPU
hls 编写的细粒度流水核，针对dwconv+pwconv+pwconv的block结构。内部全流水以及手动双缓冲  
版本：```vitis hls 2020.2```  

## 工程创建
1. 生成测试文件
testbench文件夹下中执行```python tb.py```  
注意```tb.py```中```R1\C1\DwTn1\PwTn1```等参数需要和```block.h```中保持一致，否则测2试失败   

2. 创建工程并进行综合仿真
打开vitis HLS Command Prompt,在当前目录下执行：
```vitis_hls -f build_hls.tcl``` 

3. 输入以下命令选择图形化界面操作
```vitis_hls -p BPU3Core/```

## 代码说明
``block.cpp``:目前的top文件，函数``FusedDW_PW_InMode``为**模式1**数据流     
```dw_engine.h```:DWcore单元包含数据、权重加载与计算操作  
```dwconv.h```:包含了7x7 Depthwise计算核，内部可通过```ifndef Debug```来进行debug  
```pw_engine.h```:融合处理函数：```PW_PW_Fused```，pw1的Tn和pw2的Tm方向遍历放在一个循环内分别为(Inner/outer loop)    
```block_tb.cpp```: hls中的测试文件，创建工程时需要添加5个```.bin```文件    
```testbench```:包含了```tb.py```用于生成测试的```bin```文件,其中权重数据已经根据```Tn,Tm```数据重排以保证顺序访问      
## 软件block细节
```python
dwconv:HxWxN      -> HxWxN (7x7 kernel)
pw1conv:HxWxN     -> HxWx(exN) 1x1 kernel
pw2conv:HxWx(exN) -> HxWxN 1x1 kernel
```
注意的是：这里输入输出的N，以及H,W是相等的，不相等的block会在之前加入单独的转换conv层。目前先不考虑

## 硬件架构
以[Tn][Tr*Tc]为粒度细流水，这里```coldbuffer```意味着深度方向上完全缓冲，并运用```doubling buffer```  
- 模式1  
```DWcore+ coldbuffer + (PW1PW2_FusedPE)+coldbuffer```
- 模式0  
```(DWPW1_FusedPE+coldbuffer) + PW2core```

## 注意事项
- 由于这里只有```INT8```精度还未添加quant的单元。这里的权重数为```-1,0,1```
- 目前```block_tb.cpp```中开辟的数组为全局数组以避免数据过大局部变量超限，实际上板子测试时，内存分配应该**保持连续地址**，```new```方式并不能保证
- vitis flow中头文件外必须使用```extern "C"```，顶层变量写成数组样式可以省去axi interface中的depth
## 待修改
1. :sob: **顶层的带宽设置**:目前直接通过并行度和位宽确定的，如```ap_uint<Abit*Tn>```,由于带宽必须是2的幂。这么写会导致访存错误。
2. :sob:N/Tn=1时DW_engine有错
2. 当N,M不能整除Tn，Tm时未考虑。Tn,Tm可能的取值：{2，4，6，8，12，16，24，32，48，64}，N/M取值范围：{16，24，32，64，112，184，352}
3. ```pw_engine.h```中的```loadbram_D12_P8```用于对DwTn=12，PwTn=8的情况进行转换，单未进行测试
4. :rocket:```PW_PW_Fused```函数中将```InnerPW1```和```OutnerPW2```分开写将增加并行进入和退出的时间。例如pw1最后的comp可以和pw2的in,wt加载并行。应该写到一个循环中。但需要把最外成的循环也展开。否则更深的流水级重复了更多次数
## 待添加
- :rocket:**preload** weight buffer还是很重要的。可以掩盖setuplatnecy + weight transfer latency
- quant单元，这里没有显式的Norm和ReLU单元，被集成进quant单元。
