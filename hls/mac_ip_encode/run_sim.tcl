open_project mac_ip_encode_prj

set_top mac_ip_encode

add_files mac_ip_encode.cpp
add_files -tb test_mac_ip_encode.cpp

open_solution "solution1"
set_part {xcvu9p-flga2104-2L-e}
create_clock -period 6.4 -name default

# config_rtl -disable_start_propagation
csim_design

exit

