set proj_name "tcp_ip_vcu118"
set root_dir [pwd]
set proj_dir $root_dir/$proj_name
set src_dir $root_dir/../rtl
set ip_dir $root_dir/../ip
set ip_repo $root_dir/../iprepo
set constraints_dir $root_dir/../constraints

#Check if iprepo is available
if { [file isdirectory $ip_repo] } {
	set lib_dir "$ip_repo"
} else {
	puts "iprepo directory could not be found."
	exit 1
}
# Create project
create_project $proj_name $proj_dir

# Set project properties
set obj [get_projects $proj_name]
set_property part {xcvu9p-flga2104-2L-e} $obj
set_property "target_language" "Verilog" $obj

set_property IP_REPO_PATHS $lib_dir [current_fileset]
update_ip_catalog

# Add sources
add_files $src_dir/common
add_files -norecurse $src_dir/ultraplus
add_files $src_dir/ultraplus/vcu118
set_property top tcp_ip_top [current_fileset]

add_files $ip_dir/SmartCamCtl.dcp
add_files -fileset constrs_1 $constraints_dir/vcu118.xdc

#create ip directory
file mkdir $ip_dir/vu9p


#create ips

#Network interface

create_ip -name xxv_ethernet -vendor xilinx.com -library ip -version 2.4 -module_name ethernet_10g_ip -dir $ip_dir/vu9p
set_property -dict [list CONFIG.LINE_RATE {10} CONFIG.NUM_OF_CORES {1} CONFIG.INCLUDE_AXI4_INTERFACE {0} CONFIG.GT_REF_CLK_FREQ {161.1328125} CONFIG.GT_DRP_CLK {125} CONFIG.GT_GROUP_SELECT {Quad_X1Y12} CONFIG.LANE1_GT_LOC {X1Y48} CONFIG.ENABLE_PIPELINE_REG {1} CONFIG.Component_Name {ethernet_10g_ip}] [get_ips ethernet_10g_ip]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/ethernet_10g_ip/ethernet_10g_ip.xci]
update_compile_order -fileset sources_1

#FIFOs

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 1.1 -module_name axis_data_fifo_64_cc -dir $ip_dir/vu9p
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.IS_ACLK_ASYNC {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.SYNCHRONIZATION_STAGES {3} CONFIG.Component_Name {axis_data_fifo_64_cc}] [get_ips axis_data_fifo_64_cc]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/axis_data_fifo_64_cc/axis_data_fifo_64_cc.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name axis_sync_fifo -dir $ip_dir/vu9p
set_property -dict [list CONFIG.INTERFACE_TYPE {AXI_STREAM} CONFIG.FIFO_Implementation_axis {Common_Clock_Block_RAM} CONFIG.TDATA_NUM_BYTES {8} CONFIG.TUSER_WIDTH {0} CONFIG.Enable_TLAST {true} CONFIG.HAS_TKEEP {true} CONFIG.Enable_Data_Counts_axis {true} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.TSTRB_WIDTH {8} CONFIG.TKEEP_WIDTH {8} CONFIG.FIFO_Implementation_wach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wach {15} CONFIG.Empty_Threshold_Assert_Value_wach {14} CONFIG.FIFO_Implementation_wrch {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wrch {15} CONFIG.Empty_Threshold_Assert_Value_wrch {14} CONFIG.FIFO_Implementation_rach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_rach {15} CONFIG.Empty_Threshold_Assert_Value_rach {14}] [get_ips axis_sync_fifo]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/axis_sync_fifo/axis_sync_fifo.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name cmd_fifo_xgemac_rxif -dir $ip_dir/vu9p
set_property -dict [list CONFIG.Fifo_Implementation {Common_Clock_Block_RAM} CONFIG.Input_Data_Width {16} CONFIG.Output_Data_Width {16} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.Use_Embedded_Registers {false} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Enable_Safety_Circuit {false}] [get_ips cmd_fifo_xgemac_rxif]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/cmd_fifo_xgemac_rxif/cmd_fifo_xgemac_rxif.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name cmd_fifo_xgemac_txif -dir $ip_dir/vu9p
set_property -dict [list CONFIG.Fifo_Implementation {Common_Clock_Block_RAM} CONFIG.Input_Data_Width {1} CONFIG.Output_Data_Width {1} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Enable_Safety_Circuit {false}] [get_ips cmd_fifo_xgemac_txif]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/cmd_fifo_xgemac_txif/cmd_fifo_xgemac_txif.xci]
update_compile_order -fileset sources_1

#Register slices

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_64 -dir $ip_dir/vu9p
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_64]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/axis_register_slice_64/axis_register_slice_64.xci]
update_compile_order -fileset sources_1

#Interconnects

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_3to1 -dir $ip_dir/vu9p
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {3} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true}] [get_ips axis_interconnect_3to1]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/axis_interconnect_3to1/axis_interconnect_3to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_2to1 -dir $ip_dir/vu9p
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_2to1]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/axis_interconnect_2to1/axis_interconnect_2to1.xci]
update_compile_order -fileset sources_1

#HLS IP cores

create_ip -name toe -vendor ethz.systems -library hls -version 1.6 -module_name toe_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/toe_ip/toe_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ip_handler -vendor ethz.systems -library hls -version 1.2 -module_name ip_handler_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/ip_handler_ip/ip_handler_ip.xci]
update_compile_order -fileset sources_1

create_ip -name mac_ip_encode -vendor xilinx.labs -library hls -version 1.04 -module_name mac_ip_encode_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/mac_ip_encode_ip/mac_ip_encode_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ethernet_frame_padding -vendor ethz.systems.fpga -library hls -version 0.1 -module_name ethernet_frame_padding_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/ethernet_frame_padding_ip/ethernet_frame_padding_ip.xci]
update_compile_order -fileset sources_1

create_ip -name icmp_server -vendor xilinx.labs -library hls -version 1.67 -module_name icmp_server_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/icmp_server_ip/icmp_server_ip.xci]
update_compile_order -fileset sources_1

create_ip -name echo_server_application -vendor ethz.systems -library hls -version 1.2 -module_name echo_server_application_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/echo_server_application_ip/echo_server_application_ip.xci]
update_compile_order -fileset sources_1

create_ip -name iperf_client -vendor ethz.systems.fpga -library hls -version 1.0 -module_name iperf_client_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/iperf_client_ip/iperf_client_ip.xci]
update_compile_order -fileset sources_1

create_ip -name arp_server_subnet -vendor ethz.systems -library hls -version 1.0 -module_name arp_server_subnet_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/arp_server_subnet_ip/arp_server_subnet_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ipv4 -vendor ethz.systems.fpga -library hls -version 0.1 -module_name ipv4_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/ipv4_ip/ipv4_ip.xci]
update_compile_order -fileset sources_1

create_ip -name udp -vendor ethz.systems.fpga -library hls -version 0.4 -module_name udp_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/udp_ip/udp_ip.xci]
update_compile_order -fileset sources_1

create_ip -name iperf_udp_client -vendor ethz.systems.fpga -library hls -version 0.8 -module_name iperf_udp_client_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/iperf_udp_client_ip/iperf_udp_client_ip.xci]
update_compile_order -fileset sources_1

#create_ip -name udpAppMux -vendor xilinx.labs -library hls -version 1.05 -module_name udpAppMux_0 -dir $ip_dir/vu9p
#generate_target {instantiation_template} [get_files $ip_dir/vu9p/udpAppMux_0/udpAppMux_0.xci]
#update_compile_order -fileset sources_1

create_ip -name dhcp_client -vendor xilinx.labs -library hls -version 1.05 -module_name dhcp_client_ip -dir $ip_dir/vu9p
generate_target {instantiation_template} [get_files $ip_dir/vu9p/dhcp_client_ip/dhcp_client_ip.xci]
update_compile_order -fileset sources_1


#VIOs

create_ip -name vio -vendor xilinx.com -library ip -version 3.0 -module_name vio_iperf -dir $ip_dir/vu9p
set_property -dict [list CONFIG.C_NUM_PROBE_OUT {2} CONFIG.C_EN_PROBE_IN_ACTIVITY {0} CONFIG.C_NUM_PROBE_IN {0} CONFIG.Component_Name {vio_iperf}] [get_ips vio_iperf]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/vio_iperf/vio_iperf.xci]

create_ip -name vio -vendor xilinx.com -library ip -version 3.0 -module_name vio_udp_iperf_client -dir $ip_dir/vu9p
set_property -dict [list CONFIG.C_EN_PROBE_IN_ACTIVITY {0} CONFIG.C_NUM_PROBE_IN {0} CONFIG.Component_Name {vio_udp_iperf_client}] [get_ips vio_udp_iperf_client]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/vio_udp_iperf_client/vio_udp_iperf_client.xci]

#Memory interface

create_ip -name ddr4 -vendor xilinx.com -library ip -version 2.2 -module_name ddr4_ip -dir $ip_dir/vu9p
set_property -dict [list CONFIG.C0.DDR4_TimePeriod {833} CONFIG.C0.DDR4_InputClockPeriod {4000} CONFIG.C0.DDR4_CLKOUT0_DIVIDE {5} CONFIG.C0.DDR4_MemoryPart {EDY4016AABG-DR-F} CONFIG.C0.DDR4_DataWidth {72} CONFIG.C0.DDR4_DataMask {NO_DM_NO_DBI} CONFIG.C0.DDR4_Ecc {true} CONFIG.C0.DDR4_AxiSelection {true} CONFIG.C0.DDR4_CasLatency {16} CONFIG.C0.DDR4_CasWriteLatency {12} CONFIG.C0.DDR4_AxiDataWidth {512} CONFIG.C0.DDR4_AxiAddressWidth {31} CONFIG.Component_Name {ddr4_ip} CONFIG.C0.BANK_GROUP_WIDTH {1}] [get_ips ddr4_ip]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/ddr4_ip/ddr4_ip.xci]
update_compile_order -fileset sources_1

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_64_to_512 -dir $ip_dir/vu9p
set_property -dict [list CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {64} CONFIG.c_include_mm2s_dre {true} CONFIG.c_mm2s_burst_size {16} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {64} CONFIG.c_include_s2mm_dre {true} CONFIG.c_s2mm_burst_size {16} CONFIG.c_mm2s_stscmd_is_async {true} CONFIG.c_s2mm_stscmd_is_async {true}] [get_ips axi_datamover_64_to_512]
generate_target {instantiation_template} [get_files $ip_dir/vu9p/axi_datamover_64_to_512/axi_datamover_64_to_512.xci]
update_compile_order -fileset sources_1




start_gui
