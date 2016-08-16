open_project arp_server_prj

set_top arp_server

add_files arp_server.cpp
add_files -tb test_arp_server.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

#csim_design -clean
#csim_design -clean -setup
csynth_design
#cosim_design -tool xsim -rtl verilog -trace_level all 
export_design -format ip_catalog -display_name "ARP Server for 10G TOE Design" -description "Replies to ARP queries and resolves IP addresses." -vendor "xilinx.labs" -version "1.14"
exit
