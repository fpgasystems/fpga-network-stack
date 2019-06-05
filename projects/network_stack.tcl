#IPs used in network_stack.sv


#Register slices

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name net_axis_register_slice_64 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips net_axis_register_slice_64]
generate_target {instantiation_template} [get_files $device_ip_dir/net_axis_register_slice_64/net_axis_register_slice_64.xci]
update_compile_order -fileset sources_1

#Interconnects

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name net_axis_interconnect_3to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {3} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true}] [get_ips net_axis_interconnect_3to1]
generate_target {instantiation_template} [get_files $device_ip_dir/net_axis_interconnect_3to1/net_axis_interconnect_3to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name net_axis_interconnect_2to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips net_axis_interconnect_2to1]
generate_target {instantiation_template} [get_files $device_ip_dir/net_axis_interconnect_2to1/net_axis_interconnect_2to1.xci]
update_compile_order -fileset sources_1

#HLS IP cores

create_ip -name toe -vendor ethz.systems -library hls -version 1.6 -module_name toe_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/toe_ip/toe_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ip_handler -vendor ethz.systems.fpga -library hls -version 2.0 -module_name ip_handler_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/ip_handler_ip/ip_handler_ip.xci]
update_compile_order -fileset sources_1

create_ip -name mac_ip_encode -vendor ethz.systems.fpga -library hls -version 2.0 -module_name mac_ip_encode_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/mac_ip_encode_ip/mac_ip_encode_ip.xci]
update_compile_order -fileset sources_1

create_ip -name icmp_server -vendor xilinx.labs -library hls -version 1.67 -module_name icmp_server_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/icmp_server_ip/icmp_server_ip.xci]
update_compile_order -fileset sources_1

create_ip -name echo_server_application -vendor ethz.systems -library hls -version 1.2 -module_name echo_server_application_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/echo_server_application_ip/echo_server_application_ip.xci]
update_compile_order -fileset sources_1

create_ip -name iperf_client -vendor ethz.systems.fpga -library hls -version 1.0 -module_name iperf_client_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/iperf_client_ip/iperf_client_ip.xci]
update_compile_order -fileset sources_1

create_ip -name arp_server_subnet -vendor ethz.systems.fpga -library hls -version 1.1 -module_name arp_server_subnet_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/arp_server_subnet_ip/arp_server_subnet_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ipv4 -vendor ethz.systems.fpga -library hls -version 0.1 -module_name ipv4_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/ipv4_ip/ipv4_ip.xci]
update_compile_order -fileset sources_1

create_ip -name udp -vendor ethz.systems.fpga -library hls -version 0.4 -module_name udp_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/udp_ip/udp_ip.xci]
update_compile_order -fileset sources_1

create_ip -name iperf_udp_client -vendor ethz.systems.fpga -library hls -version 0.8 -module_name iperf_udp_client_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/iperf_udp_client_ip/iperf_udp_client_ip.xci]
update_compile_order -fileset sources_1

#create_ip -name udpAppMux -vendor xilinx.labs -library hls -version 1.05 -module_name udpAppMux_0 -dir $device_ip_dir
#generate_target {instantiation_template} [get_files $device_ip_dir/udpAppMux_0/udpAppMux_0.xci]
#update_compile_order -fileset sources_1

create_ip -name dhcp_client -vendor xilinx.labs -library hls -version 1.05 -module_name dhcp_client_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/dhcp_client_ip/dhcp_client_ip.xci]
update_compile_order -fileset sources_1

