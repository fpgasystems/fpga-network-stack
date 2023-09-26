open_project iperf_prj

set_top iperf

add_files iperf.cpp
add_files -tb test_iperf.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

# config_rtl -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "iperf server" -vendor "ethz.systems.fpga" -version "1.0"

exit

