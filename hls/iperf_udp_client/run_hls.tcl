
open_project iperf_udp_client_prj

set_top iperf_udp_client

#add_files ../udp/udpCore/sources/udp.h
add_files iperf_udp_client.hpp
add_files iperf_udp_client.cpp


add_files -tb test_iperf_udp_client.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "iperf udp client" -description "" -vendor "ethz.systems.fpga" -version "0.8"

exit
