open_project mac_ip_encode_prj

set_top mac_ip_encode

add_files mac_ip_encode.cpp
add_files -tb test_mac_ip_encode.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "MAC IP Encoder for 10G TCP Offload Engine" -vendor "xilinx.labs" -version "1.04"

exit

