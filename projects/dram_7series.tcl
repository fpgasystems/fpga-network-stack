add_files $ip_dir/mig_axi_mm_dual.dcp
#Memory interface

create_ip -name axi_datamover -vendor xilinx.com -library ip -version 5.1 -module_name axi_datamover_64_to_512 -dir $device_ip_dir/
set_property -dict [list CONFIG.c_m_axi_mm2s_data_width {512} CONFIG.c_m_axis_mm2s_tdata_width {64} CONFIG.c_include_mm2s_dre {true} CONFIG.c_mm2s_burst_size {16} CONFIG.c_m_axi_s2mm_data_width {512} CONFIG.c_s_axis_s2mm_tdata_width {64} CONFIG.c_include_s2mm_dre {true} CONFIG.c_s2mm_burst_size {16} CONFIG.c_mm2s_stscmd_is_async {true} CONFIG.c_s2mm_stscmd_is_async {true}] [get_ips axi_datamover_64_to_512]
generate_target {instantiation_template} [get_files $device_ip_dir/axi_datamover_64_to_512/axi_datamover_64_to_512.xci]
update_compile_order -fileset sources_1




#add MIG for VC709
#add_files -norecurse $ip_dir/mig_axi_mm_dual/mig_axi_mm_dual.xci
#export_ip_user_files -of_objects  [get_files  $ip_dir/mig_axi_mm_dual/mig_axi_mm_dual.xci] -force -quiet
#update_compile_order -fileset sources_1
