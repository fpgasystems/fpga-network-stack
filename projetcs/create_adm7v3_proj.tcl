set proj_name "tcp_ip_adm7v3"
set root_dir [pwd]
set proj_dir $root_dir/build/proj_adm7v3
set src_dir $root_dir/../rtl
set ip_dir $root_dir/../ip
set constraints_dir $root_dir/../constraints

if { [file isdirectory $root_dir/build/ipRepository] } {
	set lib_dir "$root_dir/build/ipRepository"
} else {
	puts "ipRepository directory could not be found."
	exit 1
}
# Create project
create_project $proj_name $proj_dir

# Set project properties
set obj [get_projects $proj_name]
set_property part xc7vx690tffg1157-2 $obj
set_property "target_language" "Verilog" $obj

set_property IP_REPO_PATHS $lib_dir [current_fileset]
update_ip_catalog

# Add sources
add_files $src_dir/common
add_files -norecurse $src_dir/7series
add_files $src_dir/7series/adm7v3

#foreach subdir [glob -type d $ip_dir/*] {
#	import_files [glob $subdir/*.xci]
#}
#add_files [glob ./ip/*.xcix]
add_files $ip_dir/mig_7series_0.dcp
add_files $ip_dir/SmartCamCtl.dcp
#add_files $ip_dir/SmartCamCtlArp.dcp
add_files -fileset constrs_1 $constraints_dir/adm7v3.xdc

#create ips
create_ip -name ten_gig_eth_pcs_pma -vendor xilinx.com -library ip -version 6.0 -module_name ten_gig_eth_pcs_pma_ip
set_property -dict [list CONFIG.MDIO_Management {false} CONFIG.base_kr {BASE-R} CONFIG.baser32 {64bit}] [get_ips ten_gig_eth_pcs_pma_ip]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/ten_gig_eth_pcs_pma_ip/ten_gig_eth_pcs_pma_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ten_gig_eth_mac -vendor xilinx.com -library ip -version 15.0 -module_name ten_gig_eth_mac_ip
set_property -dict [list CONFIG.Management_Interface {false} CONFIG.Statistics_Gathering {false}] [get_ips ten_gig_eth_mac_ip]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/ten_gig_eth_mac_ip/ten_gig_eth_mac_ip.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.0 -module_name rx_fifo
set_property -dict [list CONFIG.INTERFACE_TYPE {AXI_STREAM} CONFIG.TDATA_NUM_BYTES {8} CONFIG.TUSER_WIDTH {0} CONFIG.Enable_TLAST {true} CONFIG.HAS_TKEEP {true} CONFIG.Enable_Data_Counts_axis {true} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.TSTRB_WIDTH {8} CONFIG.TKEEP_WIDTH {8} CONFIG.FIFO_Implementation_wach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wach {15} CONFIG.Empty_Threshold_Assert_Value_wach {14} CONFIG.FIFO_Implementation_wrch {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wrch {15} CONFIG.Empty_Threshold_Assert_Value_wrch {14} CONFIG.FIFO_Implementation_rach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_rach {15} CONFIG.Empty_Threshold_Assert_Value_rach {14}] [get_ips rx_fifo]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/rx_fifo/rx_fifo.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.0 -module_name axis_sync_fifo
set_property -dict [list CONFIG.INTERFACE_TYPE {AXI_STREAM} CONFIG.TDATA_NUM_BYTES {8} CONFIG.TUSER_WIDTH {0} CONFIG.Enable_TLAST {true} CONFIG.HAS_TKEEP {true} CONFIG.Enable_Data_Counts_axis {true} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.TSTRB_WIDTH {8} CONFIG.TKEEP_WIDTH {8} CONFIG.FIFO_Implementation_wach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wach {15} CONFIG.Empty_Threshold_Assert_Value_wach {14} CONFIG.FIFO_Implementation_wrch {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wrch {15} CONFIG.Empty_Threshold_Assert_Value_wrch {14} CONFIG.FIFO_Implementation_rach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_rach {15} CONFIG.Empty_Threshold_Assert_Value_rach {14}] [get_ips axis_sync_fifo]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axis_sync_fifo/axis_sync_fifo.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.0 -module_name cmd_fifo_xgemac_txif
set_property -dict [list CONFIG.Input_Data_Width {1} CONFIG.Output_Data_Width {1} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1}] [get_ips cmd_fifo_xgemac_txif]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/cmd_fifo_xgemac_txif/cmd_fifo_xgemac_txif.xci]
update_compile_order -fileset sources_1

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_64
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_64]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axis_register_slice_64/axis_register_slice_64.xci]
update_compile_order -fileset sources_1

create_ip -name toe -vendor ethz.systems -library hls -version 1.6 -module_name toe_ip
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/toe_ip/toe_ip.xci]
update_compile_order -fileset sources_1

create_ip -name udp -vendor xilinx.labs -library hls -version 1.41 -module_name udp_0
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/udp_0/udp_0.xci]
update_compile_order -fileset sources_1

create_ip -name ip_handler -vendor ethz.systems -library hls -version 1.2 -module_name ip_handler_ip
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/ip_handler_ip/ip_handler_ip.xci]
update_compile_order -fileset sources_1

create_ip -name mac_ip_encode -vendor xilinx.labs -library hls -version 1.04 -module_name mac_ip_encode_ip
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/mac_ip_encode_ip/mac_ip_encode_ip.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_3to1
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {3} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true}] [get_ips axis_interconnect_3to1]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axis_interconnect_3to1/axis_interconnect_3to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_2to1
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_2to1]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axis_interconnect_2to1/axis_interconnect_2to1.xci]
update_compile_order -fileset sources_1

create_ip -name icmp_server -vendor xilinx.labs -library hls -version 1.67 -module_name icmp_server_ip
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/icmp_server_ip/icmp_server_ip.xci]
update_compile_order -fileset sources_1

create_ip -name udpAppMux -vendor xilinx.labs -library hls -version 1.05 -module_name udpAppMux_0
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/udpAppMux_0/udpAppMux_0.xci]
update_compile_order -fileset sources_1

create_ip -name dhcp_client -vendor xilinx.labs -library hls -version 1.05 -module_name dhcp_client_0
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/dhcp_client_0/dhcp_client_0.xci]
update_compile_order -fileset sources_1

create_ip -name echo_server_application -vendor ethz.systems -library hls -version 1.1 -module_name echo_server_application_ip
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/echo_server_application_ip/echo_server_application_ip.xci]
update_compile_order -fileset sources_1

create_ip -name udpLoopback -vendor xilinx.labs -library hls -version 1.10 -module_name udpLoopback_0
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/udpLoopback_0/udpLoopback_0.xci]
update_compile_order -fileset sources_1

create_ip -name axi_interconnect -vendor xilinx.com -library ip -version 1.7 -module_name axi_interconnect_ip
set_property -dict [list CONFIG.INTERCONNECT_DATA_WIDTH {512} CONFIG.S00_AXI_DATA_WIDTH {512} CONFIG.S01_AXI_DATA_WIDTH {512} CONFIG.M00_AXI_DATA_WIDTH {512} CONFIG.S00_AXI_IS_ACLK_ASYNC {1} CONFIG.S01_AXI_IS_ACLK_ASYNC {1} CONFIG.M00_AXI_IS_ACLK_ASYNC {1} CONFIG.S00_AXI_WRITE_ACCEPTANCE {16} CONFIG.S01_AXI_WRITE_ACCEPTANCE {16} CONFIG.S00_AXI_READ_ACCEPTANCE {16} CONFIG.S01_AXI_READ_ACCEPTANCE {16} CONFIG.M00_AXI_WRITE_ISSUING {16} CONFIG.M00_AXI_READ_ISSUING {16} CONFIG.S00_AXI_REGISTER {1} CONFIG.S01_AXI_REGISTER {1} CONFIG.M00_AXI_REGISTER {1}] [get_ips axi_interconnect_ip]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axi_interconnect_ip/axi_interconnect_ip.xci]
update_compile_order -fileset sources_1

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_0
set_property -dict [list CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {64} CONFIG.c_include_mm2s_dre {true} CONFIG.c_mm2s_burst_size {16} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {64} CONFIG.c_include_s2mm_dre {true} CONFIG.c_s2mm_burst_size {16}] [get_ips axi_datamover_0]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axi_datamover_0/axi_datamover_0.xci]
update_compile_order -fileset sources_1

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_1
set_property -dict [list CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {512} CONFIG.c_mm2s_burst_size {2} CONFIG.c_mm2s_btt_used {10} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {512} CONFIG.c_s2mm_btt_used {10} CONFIG.c_mm2s_include_sf {false} CONFIG.c_s2mm_include_sf {false} CONFIG.c_s2mm_burst_size {4}] [get_ips axi_datamover_1]
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/axi_datamover_1/axi_datamover_1.xci]
update_compile_order -fileset sources_1

create_ip -name arp_server_subnet -vendor ethz.systems -library hls -version 1.0 -module_name arp_server_subnet_ip
generate_target {instantiation_template} [get_files $proj_dir/tcp_ip.srcs/sources_1/ip/arp_server_subnet_ip/arp_server_subnet_ip.xci]
update_compile_order -fileset sources_1

start_gui
