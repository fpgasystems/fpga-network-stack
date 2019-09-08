#Network interface

create_ip -name xxv_ethernet -vendor xilinx.com -library ip -version 2.4 -module_name ethernet_10g_ip -dir $device_ip_dir
set_property -dict [list CONFIG.LINE_RATE {10} CONFIG.NUM_OF_CORES {1} CONFIG.INCLUDE_AXI4_INTERFACE {0} CONFIG.GT_REF_CLK_FREQ {161.1328125} CONFIG.GT_DRP_CLK {125} CONFIG.GT_GROUP_SELECT {Quad_X1Y12} CONFIG.LANE1_GT_LOC {X1Y48} CONFIG.ENABLE_PIPELINE_REG {1} CONFIG.Component_Name {ethernet_10g_ip}] [get_ips ethernet_10g_ip]
generate_target {instantiation_template} [get_files $device_ip_dir/ethernet_10g_ip/ethernet_10g_ip.xci]
update_compile_order -fileset sources_1

create_ip -name ethernet_frame_padding -vendor ethz.systems.fpga -library hls -version 0.1 -module_name ethernet_frame_padding_ip -dir $device_ip_dir
generate_target {instantiation_template} [get_files $device_ip_dir/ethernet_frame_padding_ip/ethernet_frame_padding_ip.xci]
update_compile_order -fileset sources_1
