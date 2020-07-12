open_project ethernet_frame_padding_512_prj

set_top ethernet_frame_padding_512

add_files ethernet_frame_padding_512.cpp
add_files -tb test_ethernet_frame_padding_512.cpp

open_solution "solution1"
#set_part {xc7vx690tffg1157-2}
set_part {xcvu9p-flga2104-2L-e}
create_clock -period 3.2 -name default

#config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "Ethernet Frame Padding 512" -vendor "ethz.systems.fpga" -version "0.1"

exit

