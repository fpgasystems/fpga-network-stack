open_project udpLoopback_prj

set_top udpLoopback

add_files udpLoopback.cpp
add_files -tb udpLoopback_tb.cpp

open_solution "solution1"
set_part {xc7vx690tffg1157-2}
create_clock -period 6.4 -name default

csim_design -clean -setup
csynth_design
export_design -format ip_catalog -display_name "10G UDP Core Loopback module " -description "This traffic sink can be used for testing both paths of the XIR labs 10G UDP core. It loops the Rx data directly back to the Tx side." -vendor "xilinx.labs" -version "1.10"
#if {[file exists "/public/ireland/xlabs/memcached/ipRepository/toe/"] == 1} {
#	file delete -force /public/ireland/xlabs/memcached/ipRepository/toe/ 
#}
#file copy toe_prj/solution1/impl/ip/ /public/ireland/xlabs/memcached/ipRepository/toe/
exit
