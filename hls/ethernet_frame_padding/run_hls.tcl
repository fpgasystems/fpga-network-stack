open_project ethernet_frame_padding_prj

set_top ethernet_frame_padding

add_files ethernet_frame_padding.cpp
add_files -tb test_ethernet_frame_padding.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

#config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "Ethernet Frame Padding" -vendor "ethz.systems.fpga" -version "0.1"

exit

