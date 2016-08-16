open_project ipHandler_prj

set_top ip_handler

add_files ip_handler.cpp
add_files -tb test_ip_handler.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

#csim_design  -clean
csynth_design
#cosim_design -tool xsim -rtl verilog -trace_level none 
export_design -format ip_catalog -display_name "IP Handler for 10G TCP Offload Engine" -vendor "xilinx.labs" -version "1.21"

exit

