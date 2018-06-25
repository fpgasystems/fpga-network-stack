open_project udpAppMux_prj

set_top udpAppMux

add_files udpAppMux.cpp
add_files -tb udpAppMux_tb.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.4 -name default
config_rtl -reset all -reset_async

#csim_design -clean -setup
csynth_design
export_design -format ip_catalog -display_name "UDP App Multiplexer for DHCP" -description "This module enables the connection of both the DHCP server and an additional application to the UDP offload engine, granted the DHCP server uses the standard ports." -vendor "xilinx.labs" -version "1.05"
#if {[file exists "/public/ireland/xlabs/memcached/ipRepository/toe/"] == 1} {
#	file delete -force /public/ireland/xlabs/memcached/ipRepository/toe/ 
#}
#file copy toe_prj/solution1/impl/ip/ /public/ireland/xlabs/memcached/ipRepository/toe/
exit
