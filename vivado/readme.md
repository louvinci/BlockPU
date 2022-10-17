## block design
```.pdf```为block deign设计框图，三个BPU挂载在AXI总线上，并添加中断信号   
这里我们启用了**多个**```PL-PS Master port```以避免该接口带宽成为瓶颈    
桥的数量和配置对最终的**性能和资源**消耗有影响，因此我们这里采用了多个AXI桥进行连接      

:rocket:经过测试，我们这里使用```AXI-Interconnect IP```核即可满足性能需求（Outstanding、位宽转换、多对一路由），该IP核相比于```AXI-SMC```将节省大量LUT资源    

