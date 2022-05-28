set Project     BPU3Core
set Solution    solution1
set Device      "xczu9eg-ffvb1156-2-e"
set Flow        "vitis"
set Clock       5

open_project $Project -reset

set_top FusedDW_PW_InMode

add_files block.cpp
add_files block.h
add_files dw_engine.h
add_files dwconv.h
add_files pw_engine.h
add_files pwconv.h

add_files -tb block_tb.cpp
add_files -tb ./testbench/dw_weight.bin
add_files -tb ./testbench/hls_res.bin
add_files -tb ./testbench/input.bin
add_files -tb ./testbench/pw1_weight.bin
add_files -tb ./testbench/pw2_weight.bin



open_solution -flow_target $Flow -reset $Solution
set_part $Device
create_clock -period $Clock

csim_design
csynth_design
cosim_design

exit