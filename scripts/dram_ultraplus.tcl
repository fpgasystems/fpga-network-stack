#DRAM Interface

create_ip -name ddr4 -vendor xilinx.com -library ip -version 2.2 -module_name ddr4_ip -dir $device_ip_dir
set_property -dict [list CONFIG.C0.DDR4_TimePeriod {833} CONFIG.C0.DDR4_InputClockPeriod {4000} CONFIG.C0.DDR4_CLKOUT0_DIVIDE {5} CONFIG.C0.DDR4_MemoryPart {EDY4016AABG-DR-F} CONFIG.C0.DDR4_DataWidth {72} CONFIG.C0.DDR4_DataMask {NO_DM_NO_DBI} CONFIG.C0.DDR4_Ecc {true} CONFIG.C0.DDR4_AxiSelection {true} CONFIG.C0.DDR4_CasLatency {16} CONFIG.C0.DDR4_CasWriteLatency {12} CONFIG.C0.DDR4_AxiDataWidth {512} CONFIG.C0.DDR4_AxiAddressWidth {31} CONFIG.Component_Name {ddr4_ip} CONFIG.C0.BANK_GROUP_WIDTH {1}] [get_ips ddr4_ip]
generate_target {instantiation_template} [get_files $device_ip_dir/ddr4_ip/ddr4_ip.xci]
update_compile_order -fileset sources_1

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_64_to_512 -dir $device_ip_dir
set_property -dict [list CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {64} CONFIG.c_include_mm2s_dre {true} CONFIG.c_mm2s_burst_size {16} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {64} CONFIG.c_include_s2mm_dre {true} CONFIG.c_s2mm_burst_size {16} CONFIG.c_mm2s_stscmd_is_async {true} CONFIG.c_s2mm_stscmd_is_async {true}] [get_ips axi_datamover_64_to_512]
generate_target {instantiation_template} [get_files $device_ip_dir/axi_datamover_64_to_512/axi_datamover_64_to_512.xci]
update_compile_order -fileset sources_1

