HLS生成硬件的源代码，分别包含了BPU1、BPU2、BPU3三个异构子核心

基本上都使用一套模板函数，但是部分函数在不同实例上，选择了固定的实现。因此，另建立一个文件夹。    
由于是多核，这里top函数的命名也需要不一样，否则```vivado block design```时难以区分。   
testbench中```tb.py```中的硬件参数也不同，因此另立文件夹比较合适  
