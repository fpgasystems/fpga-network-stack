open_project icmp_server_prj

set_top icmp_server

add_files icmp_server.cpp
add_files -tb test_icmp_server.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "ICMP Server" -vendor "xilinx.labs" -version "1.67"

exit
