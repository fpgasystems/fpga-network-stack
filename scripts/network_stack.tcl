#IPs used in network_stack.sv


#Register slices

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name net_axis_register_slice_64 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips net_axis_register_slice_64]
generate_target {instantiation_template} [get_files $device_ip_dir/net_axis_register_slice_64/net_axis_register_slice_64.xci]
update_compile_order -fileset sources_1

#FIFO / RX Buffer
create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_64_d1024 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1} CONFIG.Component_Name {axis_data_fifo_64_d1024}] [get_ips axis_data_fifo_64_d1024]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_64_d1024/axis_data_fifo_64_d1024.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_128_d1024 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {16} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1} CONFIG.Component_Name {axis_data_fifo_128_d1024}] [get_ips axis_data_fifo_128_d1024]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_128_d1024/axis_data_fifo_128_d1024.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_256_d1024 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {32} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1} CONFIG.Component_Name {axis_data_fifo_256_d1024}] [get_ips axis_data_fifo_256_d1024]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_256_d1024/axis_data_fifo_256_d1024.xci]
update_compile_order -fileset sources_1

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 2.0 -module_name axis_data_fifo_512_d1024 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {64} CONFIG.FIFO_DEPTH {1024} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.HAS_WR_DATA_COUNT {1} CONFIG.HAS_RD_DATA_COUNT {1} CONFIG.Component_Name {axis_data_fifo_512_d1024}] [get_ips axis_data_fifo_512_d1024]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_512_d1024/axis_data_fifo_512_d1024.xci]
update_compile_order -fileset sources_1

#Interconnects

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_merger_160 -dir $device_ip_dir
set_property -dict [list CONFIG.Component_Name {axis_interconnect_merger_160} CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {20} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TKEEP {false} CONFIG.HAS_TLAST {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {false} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {20} CONFIG.S00_AXIS_TDATA_NUM_BYTES {20} CONFIG.S01_AXIS_TDATA_NUM_BYTES {20} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_merger_160]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_merger_160/axis_interconnect_merger_160.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_2to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_2to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_2to1/axis_interconnect_2to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_3to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {3} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true}] [get_ips axis_interconnect_3to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_3to1/axis_interconnect_3to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_4to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {4} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_S03_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.S03_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true} CONFIG.M00_S03_CONNECTIVITY {true}] [get_ips axis_interconnect_4to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_4to1/axis_interconnect_4to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_128_2to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {16} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {16} CONFIG.S00_AXIS_TDATA_NUM_BYTES {16} CONFIG.S01_AXIS_TDATA_NUM_BYTES {16} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_128_2to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_128_2to1/axis_interconnect_128_2to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_128_4to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {4} CONFIG.SWITCH_TDATA_NUM_BYTES {16} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_S03_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {16} CONFIG.S00_AXIS_TDATA_NUM_BYTES {16} CONFIG.S01_AXIS_TDATA_NUM_BYTES {16} CONFIG.S02_AXIS_TDATA_NUM_BYTES {16} CONFIG.S03_AXIS_TDATA_NUM_BYTES {16} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true} CONFIG.M00_S03_CONNECTIVITY {true}] [get_ips axis_interconnect_128_4to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_4to1/axis_interconnect_128_4to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_256_2to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {32} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {32} CONFIG.S00_AXIS_TDATA_NUM_BYTES {32} CONFIG.S01_AXIS_TDATA_NUM_BYTES {32} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_256_2to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_256_2to1/axis_interconnect_256_2to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_256_4to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {4} CONFIG.SWITCH_TDATA_NUM_BYTES {32} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_S03_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {32} CONFIG.S00_AXIS_TDATA_NUM_BYTES {32} CONFIG.S01_AXIS_TDATA_NUM_BYTES {32} CONFIG.S02_AXIS_TDATA_NUM_BYTES {32} CONFIG.S03_AXIS_TDATA_NUM_BYTES {32} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true} CONFIG.M00_S03_CONNECTIVITY {true}] [get_ips axis_interconnect_256_4to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_4to1/axis_interconnect_256_4to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_512_2to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {64} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S01_AXIS_TDATA_NUM_BYTES {64} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_512_2to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_512_2to1/axis_interconnect_512_2to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_512_4to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {4} CONFIG.SWITCH_TDATA_NUM_BYTES {64} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_S03_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S00_AXIS_TDATA_NUM_BYTES {64} CONFIG.S01_AXIS_TDATA_NUM_BYTES {64} CONFIG.S02_AXIS_TDATA_NUM_BYTES {64} CONFIG.S03_AXIS_TDATA_NUM_BYTES {64} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true} CONFIG.M00_S03_CONNECTIVITY {true}] [get_ips axis_interconnect_512_4to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_4to1/axis_interconnect_512_4to1.xci]
update_compile_order -fileset sources_1

#Data Width Converter

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_64_to_512_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {8} CONFIG.M_TDATA_NUM_BYTES {64} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1} CONFIG.Component_Name {axis_64_to_512_converter}] [get_ips axis_64_to_512_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_64_to_512_converter/axis_64_to_512_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_512_to_64_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {64} CONFIG.M_TDATA_NUM_BYTES {8} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.Component_Name {axis_512_to_64_converter}] [get_ips axis_512_to_64_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_512_to_64_converter/axis_512_to_64_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_128_to_512_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {16} CONFIG.M_TDATA_NUM_BYTES {64} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1} CONFIG.Component_Name {axis_128_to_512_converter}] [get_ips axis_128_to_512_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_128_to_512_converter/axis_128_to_512_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_512_to_128_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {64} CONFIG.M_TDATA_NUM_BYTES {16} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.Component_Name {axis_512_to_128_converter}] [get_ips axis_512_to_128_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_512_to_128_converter/axis_512_to_128_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_256_to_512_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {32} CONFIG.M_TDATA_NUM_BYTES {64} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1} CONFIG.Component_Name {axis_256_to_512_converter}] [get_ips axis_256_to_512_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_256_to_512_converter/axis_256_to_512_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_512_to_256_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {64} CONFIG.M_TDATA_NUM_BYTES {32} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.Component_Name {axis_512_to_256_converter}] [get_ips axis_512_to_256_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_512_to_256_converter/axis_512_to_256_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_64_to_128_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {8} CONFIG.M_TDATA_NUM_BYTES {16} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1} CONFIG.Component_Name {axis_64_to_128_converter}] [get_ips axis_64_to_128_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_64_to_128_converter/axis_64_to_128_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_128_to_64_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {16} CONFIG.M_TDATA_NUM_BYTES {8} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.Component_Name {axis_128_to_64_converter}] [get_ips axis_128_to_64_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_128_to_64_converter/axis_128_to_64_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_64_to_256_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {8} CONFIG.M_TDATA_NUM_BYTES {32} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.TDEST_WIDTH {1} CONFIG.Component_Name {axis_64_to_256_converter}] [get_ips axis_64_to_256_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_64_to_256_converter/axis_64_to_256_converter.xci]
update_compile_order -fileset sources_1

create_ip -name axis_dwidth_converter -vendor xilinx.com -library ip -version 1.1 -module_name axis_256_to_64_converter -dir $device_ip_dir
set_property -dict [list CONFIG.S_TDATA_NUM_BYTES {32} CONFIG.M_TDATA_NUM_BYTES {8} CONFIG.HAS_TLAST {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_MI_TKEEP {1} CONFIG.Component_Name {axis_256to_64_converter}] [get_ips axis_256_to_64_converter]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_256_to_64_converter/axis_256_to_64_converter.xci]
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

create_ip -name iperf_udp -vendor ethz.systems.fpga -library hls -version 0.9 -module_name iperf_udp_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/iperf_udp_ip/iperf_udp_ip.xci]
update_compile_order -fileset sources_1

#create_ip -name udpAppMux -vendor xilinx.labs -library hls -version 1.05 -module_name udpAppMux_0 -dir $device_ip_dir
#generate_target {instantiation_template} [get_files $device_ip_dir/udpAppMux_0/udpAppMux_0.xci]
#update_compile_order -fileset sources_1

create_ip -name dhcp_client -vendor xilinx.labs -library hls -version 1.05 -module_name dhcp_client_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/dhcp_client_ip/dhcp_client_ip.xci]
update_compile_order -fileset sources_1

create_ip -name rocev2 -vendor ethz.systems.fpga -library hls -version 0.82 -module_name rocev2_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/rocev2_ip/rocev2_ip.xci]
update_compile_order -fileset sources_1

create_ip -name hash_table -vendor ethz.systems.fpga -library hls -version 1.0 -module_name hash_table_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/hash_table_ip/hash_table_ip.xci]
update_compile_order -fileset sources_1

