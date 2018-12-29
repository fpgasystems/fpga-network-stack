
open_project udp_prj

set_top udp

add_files ../packet.hpp
add_files udp.hpp
add_files udp.cpp


#add_files -tb test_udp.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "UDP" -description "" -vendor "ethz.systems.fpga" -version "0.4"

exit
