set proj_name "tcp_ip_vc709"
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
set_property part xc7vx690tffg1761-2 $obj
set_property "target_language" "Verilog" $obj

set_property IP_REPO_PATHS $lib_dir [current_fileset]
update_ip_catalog

# Add sources
add_files $src_dir/common
add_files -norecurse $src_dir/7series
add_files $src_dir/7series/vc709
set_property top tcp_ip_top [current_fileset]

add_files $ip_dir/mig_axi_mm_dual.dcp
add_files $ip_dir/SmartCamCtl.dcp
add_files -fileset constrs_1 $constraints_dir/vc709.xdc

#create ip directory
file mkdir $ip_dir/vc709

#create ips

#Network interface

create_ip -name ten_gig_eth_pcs_pma -vendor xilinx.com -library ip -version 6.0 -module_name ten_gig_eth_pcs_pma_ip -dir $ip_dir/vc709
set_property -dict [list CONFIG.MDIO_Management {false} CONFIG.base_kr {BASE-R} CONFIG.baser32 {64bit}] [get_ips ten_gig_eth_pcs_pma_ip]
generate_target {instantiation_template} [get_files $ip_dir/vc709/ten_gig_eth_pcs_pma_ip/ten_gig_eth_pcs_pma_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ten_gig_eth_mac -vendor xilinx.com -library ip -version 15.1 -module_name ten_gig_eth_mac_ip -dir $ip_dir/vc709
set_property -dict [list CONFIG.Management_Interface {false} CONFIG.Statistics_Gathering {false}] [get_ips ten_gig_eth_mac_ip]
generate_target {instantiation_template} [get_files $ip_dir/vc709/ten_gig_eth_mac_ip/ten_gig_eth_mac_ip.xci]
update_compile_order -fileset sources_1

#FIFOs

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 1.1 -module_name axis_data_fifo_64_cc -dir $ip_dir/vc709
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.IS_ACLK_ASYNC {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.SYNCHRONIZATION_STAGES {3} CONFIG.Component_Name {axis_data_fifo_64_cc}] [get_ips axis_data_fifo_64_cc]
generate_target {instantiation_template} [get_files $ip_dir/vc709/axis_data_fifo_64_cc/axis_data_fifo_64_cc.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name axis_sync_fifo -dir $ip_dir/vc709
set_property -dict [list CONFIG.INTERFACE_TYPE {AXI_STREAM} CONFIG.FIFO_Implementation_axis {Common_Clock_Block_RAM} CONFIG.TDATA_NUM_BYTES {8} CONFIG.TUSER_WIDTH {0} CONFIG.Enable_TLAST {true} CONFIG.HAS_TKEEP {true} CONFIG.Enable_Data_Counts_axis {true} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.TSTRB_WIDTH {8} CONFIG.TKEEP_WIDTH {8} CONFIG.FIFO_Implementation_wach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wach {15} CONFIG.Empty_Threshold_Assert_Value_wach {14} CONFIG.FIFO_Implementation_wrch {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wrch {15} CONFIG.Empty_Threshold_Assert_Value_wrch {14} CONFIG.FIFO_Implementation_rach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_rach {15} CONFIG.Empty_Threshold_Assert_Value_rach {14}] [get_ips axis_sync_fifo]
generate_target {instantiation_template} [get_files $ip_dir/vc709/axis_sync_fifo/axis_sync_fifo.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name cmd_fifo_xgemac_rxif -dir $ip_dir/vc709
set_property -dict [list CONFIG.Fifo_Implementation {Common_Clock_Block_RAM} CONFIG.Input_Data_Width {16} CONFIG.Output_Data_Width {16} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.Use_Embedded_Registers {false} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Enable_Safety_Circuit {false}] [get_ips cmd_fifo_xgemac_rxif]
generate_target {instantiation_template} [get_files $ip_dir/vc709/cmd_fifo_xgemac_rxif/cmd_fifo_xgemac_rxif.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name cmd_fifo_xgemac_txif -dir $ip_dir/vc709
set_property -dict [list CONFIG.Fifo_Implementation {Common_Clock_Block_RAM} CONFIG.Input_Data_Width {1} CONFIG.Output_Data_Width {1} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Enable_Safety_Circuit {false}] [get_ips cmd_fifo_xgemac_txif]
generate_target {instantiation_template} [get_files $ip_dir/vc709/cmd_fifo_xgemac_txif/cmd_fifo_xgemac_txif.xci]
update_compile_order -fileset sources_1

#Register slices

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_64 -dir $ip_dir/vc709 
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_64]
generate_target {instantiation_template} [get_files $ip_dir/vc709/axis_register_slice_64/axis_register_slice_64.xci]
update_compile_order -fileset sources_1

#Interconnects

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_3to1 -dir $ip_dir/vc709
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {3} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true}] [get_ips axis_interconnect_3to1]
generate_target {instantiation_template} [get_files $ip_dir/vc709/axis_interconnect_3to1/axis_interconnect_3to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_2to1 -dir $ip_dir/vc709
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_2to1]
generate_target {instantiation_template} [get_files $ip_dir/axis_interconnect_2to1/axis_interconnect_2to1.xci]
update_compile_order -fileset sources_1

#HLS IP cores

create_ip -name toe -vendor ethz.systems -library hls -version 1.6 -module_name toe_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/toe_ip/toe_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ip_handler -vendor ethz.systems -library hls -version 1.2 -module_name ip_handler_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/ip_handler_ip/ip_handler_ip.xci]
update_compile_order -fileset sources_1

create_ip -name mac_ip_encode -vendor xilinx.labs -library hls -version 1.04 -module_name mac_ip_encode_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/mac_ip_encode_ip/mac_ip_encode_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ethernet_frame_padding -vendor ethz.systems.fpga -library hls -version 0.1 -module_name ethernet_frame_padding_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/ethernet_frame_padding_ip/ethernet_frame_padding_ip.xci]
update_compile_order -fileset sources_1

create_ip -name icmp_server -vendor xilinx.labs -library hls -version 1.67 -module_name icmp_server_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/icmp_server_ip/icmp_server_ip.xci]
update_compile_order -fileset sources_1

create_ip -name echo_server_application -vendor ethz.systems -library hls -version 1.2 -module_name echo_server_application_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/echo_server_application_ip/echo_server_application_ip.xci]
update_compile_order -fileset sources_1

create_ip -name iperf_client -vendor ethz.systems.fpga -library hls -version 1.0 -module_name iperf_client_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/iperf_client_ip/iperf_client_ip.xci]
update_compile_order -fileset sources_1

create_ip -name arp_server_subnet -vendor ethz.systems -library hls -version 1.0 -module_name arp_server_subnet_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/arp_server_subnet_ip/arp_server_subnet_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ipv4 -vendor ethz.systems.fpga -library hls -version 0.1 -module_name ipv4_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/ipv4_ip/ipv4_ip.xci]
update_compile_order -fileset sources_1

create_ip -name udp -vendor ethz.systems.fpga -library hls -version 0.4 -module_name udp_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/udp_ip/udp_ip.xci]
update_compile_order -fileset sources_1

create_ip -name iperf_udp_client -vendor ethz.systems.fpga -library hls -version 0.8 -module_name iperf_udp_client_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/iperf_udp_client_ip/iperf_udp_client_ip.xci]
update_compile_order -fileset sources_1

#create_ip -name udpAppMux -vendor xilinx.labs -library hls -version 1.05 -module_name udpAppMux_0 -dir $ip_dir/vc709
#generate_target {instantiation_template} [get_files $ip_dir/vc709/udpAppMux_0/udpAppMux_0.xci]
#update_compile_order -fileset sources_1

create_ip -name dhcp_client -vendor xilinx.labs -library hls -version 1.05 -module_name dhcp_client_ip -dir $ip_dir/vc709
generate_target {instantiation_template} [get_files $ip_dir/vc709/dhcp_client_ip/dhcp_client_ip.xci]
update_compile_order -fileset sources_1


#VIOs

create_ip -name vio -vendor xilinx.com -library ip -version 3.0 -module_name vio_iperf -dir $ip_dir/vc709
set_property -dict [list CONFIG.C_NUM_PROBE_OUT {2} CONFIG.C_EN_PROBE_IN_ACTIVITY {0} CONFIG.C_NUM_PROBE_IN {0} CONFIG.Component_Name {vio_iperf}] [get_ips vio_iperf]
generate_target {instantiation_template} [get_files $ip_dir/vc709/vio_iperf/vio_iperf.xci]

create_ip -name vio -vendor xilinx.com -library ip -version 3.0 -module_name vio_udp_iperf_client -dir $ip_dir/vc709
set_property -dict [list CONFIG.C_EN_PROBE_IN_ACTIVITY {0} CONFIG.C_NUM_PROBE_IN {0} CONFIG.Component_Name {vio_udp_iperf_client}] [get_ips vio_udp_iperf_client]
generate_target {instantiation_template} [get_files $ip_dir/vc709/vio_udp_iperf_client/vio_udp_iperf_client.xci]

#Memory interface

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_64_to_512 -dir $ip_dir/vc709
set_property -dict [list CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {64} CONFIG.c_include_mm2s_dre {true} CONFIG.c_mm2s_burst_size {16} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {64} CONFIG.c_include_s2mm_dre {true} CONFIG.c_s2mm_burst_size {16} CONFIG.c_mm2s_stscmd_is_async {true} CONFIG.c_s2mm_stscmd_is_async {true}] [get_ips axi_datamover_64_to_512]
generate_target {instantiation_template} [get_files $ip_dir/vc709/axi_datamover_64_to_512/axi_datamover_64_to_512.xci]
update_compile_order -fileset sources_1




#add MIG for VC709
#add_files -norecurse $ip_dir/mig_axi_mm_dual/mig_axi_mm_dual.xci
#export_ip_user_files -of_objects  [get_files  $ip_dir/mig_axi_mm_dual/mig_axi_mm_dual.xci] -force -quiet
#update_compile_order -fileset sources_1

start_gui
