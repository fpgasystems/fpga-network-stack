open_project iperf_client_prj

set_top iperf_client

add_files iperf_client.cpp
add_files -tb test_iperf_client.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default

#csim_design  -clean -argv {in.dat out.dat}
#csim_design  -clean
csynth_design
#cosim_design -tool xsim -rtl verilog -trace_level all 
export_design -format ip_catalog -display_name "iperf client" -vendor "ethz.systems.fpga" -version "1.0"

exit

