open_project icmpServer_prj

set_top icmp_server

add_files icmp_server.cpp
add_files -tb test_icmp_server.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

#csim_design  -clean
#csim_design  -clean -setup
csynth_design
#cosim_design -tool xsim -rtl verilog -trace_level all 
export_design -format ip_catalog -display_name "ICMP Server" -vendor "xilinx.labs" -version "1.67"

exit
