open_project echo_server_prj

set_top echo_server_application

add_files echo_server_application.cpp
add_files -tb test_echo_server_application.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

#csim_design -clean -setup
csynth_design
export_design -format ip_catalog -display_name "Echo Server Application for 10G TOE" -description "Loops back packets from the TOE Rx side to the TOE Tx side." -vendor "xilinx.labs" -version "1.02"
exit
