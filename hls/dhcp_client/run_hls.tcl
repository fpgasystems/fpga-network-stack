open_project dhcp_prj

set_top dhcp_client

add_files dhcp_client.cpp
add_files -tb test_dhcp_client.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

#csim_design -clean -argv {../../../../sources/rxInput.dat ../../../../sources/txInput.dat}
#csim_design -clean 
csynth_design
#cosim_design -tool xsim -rtl verilog -trace_level all
#cosim_design -tool modelsim -rtl verilog -trace_level all -argv {../../../../sources/rxInput.dat ../../../../sources/txInput.dat}
export_design -format ip_catalog -display_name "DHCP Client" -description "DHCP Client to be used with the Xilinx Labs TCP & UDP offload engines." -vendor "xilinx.labs" -version "1.05"
#if {[file exists "/public/ireland/xlabs/memcached/ipRepository/toe/"] == 1} {
#	file delete -force /public/ireland/xlabs/memcached/ipRepository/toe/ 
#}
#file copy toe_prj/solution1/impl/ip/ /public/ireland/xlabs/memcached/ipRepository/toe/
exit
