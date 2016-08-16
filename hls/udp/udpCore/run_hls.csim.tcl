open_project udp_prj

set_top udp

add_files sources/udp.cpp
add_files -tb sources/udp_tb.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

csim_design -clean -argv {../../../../sources/rxInput.short.dat ../../../../sources/txInput.short.dat}
csynth_design
cosim_design -tool xsim -rtl verilog -trace_level all -argv {../../../../sources/rxInput.short.dat ../../../../sources/txInput.short.dat}
export_design -format ip_catalog -display_name "10G UDP Offload Engine" -description "UDP Offload Engine supporting 10Gbps line rate" -vendor "xilinx.labs" -version "1.22"
exit
