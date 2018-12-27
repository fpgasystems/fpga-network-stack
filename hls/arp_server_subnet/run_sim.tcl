open_project arp_server_subnet_prj

set_top arp_server_subnet

add_files arp_server_subnet.cpp
add_files ../axi_utils.cpp
add_files -tb test_arp_server_subnet.cpp

open_solution "solution1"
#set_part {xc7vx690tffg1761-2}
set_part {xcvu9p-flga2104-2L-e}

create_clock -period 6.4 -name default

config_rtl -disable_start_propagation
csim_design
exit
