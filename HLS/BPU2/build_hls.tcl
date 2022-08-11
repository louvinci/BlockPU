set Project     QBPU2
set Solution    solution1
set Device      "xczu9eg-ffvb1156-2-e"
#set Flow        "vitis"
#set Device      "xczu7ev-ffvc1156-2-e"
set Flow        "vivado"

set Clock       4

open_project $Project -reset

set_top QBPU2

add_files block.cpp
add_files block.h
add_files dw_engine.h
add_files dwconv.h
add_files pw_engine.h
add_files pwconv.h
add_files quant_node.h

add_files -tb block_tb.cpp
add_files -tb ./testbench/input.bin
add_files -tb ./testbench/dw_wt.bin
add_files -tb ./testbench/norm.bin
add_files -tb ./testbench/pw1_wt.bin
add_files -tb ./testbench/pw2_wt.bin
add_files -tb ./testbench/sw_res.bin



open_solution -flow_target $Flow -reset $Solution
set_part $Device
create_clock -period $Clock

csim_design
csynth_design
cosim_design
export_design -rtl verilog -format ip_catalog -output ./export/QBPU2.zip
exit