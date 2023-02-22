注意事项
- 需要使用linux版本的vitis sdk软件，并在root模式下启动，否则无法```program FPGA```    
- 测试需要的```.bin```文件需要使用hls工程中的```testbench\tb.py```产生   

Test case
jtag模式在zcu104上启动，sd卡存储数据，网络配置14*14*128(e6)->14*14*128, cosim cycles：115815(463us).     
250Mhz上板实测 481 us (96.25%)


## TODO
- [ ] 测试中断模式
