# BlockPU
hls 编写的细粒度流水核，针对dwconv+pwconv+pwconv的block结构。内部全流水以及手动双缓冲  

## 代码说明
``block.cpp``:目前的top文件，函数``FusedDW_PW_InMode``为**模式1**架构
```dw_engine.h```:DWcore单元包含数据、权重加载与计算操作  
```dwconv.h```:包含了7x7 Depthwise计算核，内部可通过```ifndef Debug```来进行debug  
```pw_engine.h```:融合处理函数：```PW_PW_Fused```，pw1的Tn和pw2的Tm方向遍历放在一个循环内分别为(Inner/outer loop)  
```block_tb.cpp```: hls中的测试文件，该文件需要```.bin```文件  
```input\weight.bin```:由testbench文件夹中的```tb.py```生成,其中权重数据已经根据```Tn,Tm```数据重排以保证顺序访问    
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
- 测试时```tb.py```函数和```block.h```中的硬件参数要保持一致，否则测试失败
- 由于这里只有```INT8```精度还未添加quant的单元。这里的权重数为```-1,0,1```
- ```tb.py```中会输出各阶段的结果保存在相应的```txt```文件。
## 待修改
1. 顶层的带宽设置，目前直接通过并行度和位宽确定的，如```ap_uint<Abit*Tn>```,由于带宽必须是2的幂。这么写会导致访存错误。
2. 当N,M不能整除Tn，Tm时未考虑。Tn,Tm可能的取值：{2，4，6，8，12，16，24，32，48，64}，N/M取值范围：{16，24，32，64，112，184，352}
3. ```pw_engine.h```中的```loadbram_D12_P8```用于对DwTn=12，PwTn=8的情况进行转换，单未进行测试
## 待添加
- quant单元，这里没有显式的Norm和ReLU单元，被集成进quant单元。
