
open_project ipv4_prj

set_top ipv4

add_files ../packet.hpp
add_files ipv4.hpp
add_files ipv4.cpp


#add_files -tb test_ipv4.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "IPv4" -description "" -vendor "ethz.systems.fpga" -version "0.1"

exit
