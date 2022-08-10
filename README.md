# BlockPU
hls 编写的细粒度流水核，针对dwconv+pwconv+pwconv的block结构。内部全流水以及手动双缓冲
单/双核心下INT8位宽可达250Mhz，三核心由于消耗资源较多，最大222Mhz      
版本：```vitis hls 2020.2, viviado 2020.2 vitis 2020.2```  

## 工程创建
1. 生成测试文件
testbench文件夹下中执行```python tb.py```  
注意```tb.py```中```R1\C1\DwTn1\PwTn1```等参数需要和```block.h```中保持一致，否则测2试失败   
2. 创建工程并进行综合仿真
```
sudo apt-get install libc6-dev-amd64
sudo apt-get install linux-libc-dev:i386
ln -s /usr/include/asm-generic /usr/include/asm
```
打开vitis HLS Command Prompt,在当前目录下执行：
```vitis_hls -f build_hls.tcl``` 

3. 输入以下命令选择图形化界面操作

```vitis_hls -p BPU3Core/```

## 代码说明
``block.cpp``:top文件，函数``BPUx``意味着子核心      
```dw_engine.h```:DWcore单元包含数据、权重加载与计算操作  
```dwconv.h```:包含了**3x3, 7x7 以及KxK Depthwise**计算核，内部可通过```ifndef Debug```来进行debug  
```pw_engine.h```:融合处理函数：```PW_PW_Fused```，pw1的Tn和pw2的Tm方向遍历放在一个循环内分别为(Inner/outer loop)    
```block_tb.cpp```: hls中的测试文件，创建工程时需要添加5个```.bin```文件    
```testbench```:包含了```tb.py```用于生成测试的```bin```文件,其中权重数据已经根据```Tn,Tm```数据重排以保证顺序访问      
```BPUx```： 异构核心，虽然可以使用一套模板函数，但是这里为了方便略有区别分别归类了          
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
## 涉及访存细节
- 权重**预取**以及**重排**对访存影响很大，这里经过优化```brust```模式充分启用并且掩盖了```setup latency```
- 第一层的输入数据padding离线完成，后续的padding由输出单元完成。这里局限于```hls```，带判断条件的访存难以启动```brust```

## 注意事项
- 由于这里只有```INT8```精度，随机数范围为```-127~127```    
- 目前```block_tb.cpp```中开辟的数组为全局数组以避免数据过大局部变量超限，实际上板子测试时，内存分配应该**保持连续地址**，```new```方式并不能保证
- vitis flow中头文件外必须使用```extern "C"```，顶层变量写成数组样式可以省去axi interface中的depth

## 通用性问题
- [x] :rocket:**prefetch** weight 还是很重要的。可以掩盖setuplatnecy + weight transfer latency,这里可选择的preload= 1,2,4分别对应三个异构核心.(网络前端通道数较少，prefetch =1即，不预取 )
- [x] branch-add 重新从片外加载至片上，完成相加后量化输出，以节省BRAM资源  
- [x] 虽然这里是通用核心，但是在NAS框架下，还是比较有规律的。因此，这里设计的核心满足以下条件时可以正常运行。1）DwTn可以被N整除，或者DwTn大于N; 2) PwTn/PwTm是可以被eN，M整除 3) PwTn 整除 DwTn，或者DwTn小于PwTn; 
