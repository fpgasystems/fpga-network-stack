#AXI Infrastructure

#FIFOs

create_ip -name axis_data_fifo -vendor xilinx.com -library ip -version 1.1 -module_name axis_data_fifo_64_cc -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.IS_ACLK_ASYNC {1} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1} CONFIG.SYNCHRONIZATION_STAGES {3} CONFIG.Component_Name {axis_data_fifo_64_cc}] [get_ips axis_data_fifo_64_cc]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_data_fifo_64_cc/axis_data_fifo_64_cc.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name axis_sync_fifo -dir $device_ip_dir
set_property -dict [list CONFIG.INTERFACE_TYPE {AXI_STREAM} CONFIG.FIFO_Implementation_axis {Common_Clock_Block_RAM} CONFIG.TDATA_NUM_BYTES {8} CONFIG.TUSER_WIDTH {0} CONFIG.Enable_TLAST {true} CONFIG.HAS_TKEEP {true} CONFIG.Enable_Data_Counts_axis {true} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.TSTRB_WIDTH {8} CONFIG.TKEEP_WIDTH {8} CONFIG.FIFO_Implementation_wach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wach {15} CONFIG.Empty_Threshold_Assert_Value_wach {14} CONFIG.FIFO_Implementation_wrch {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_wrch {15} CONFIG.Empty_Threshold_Assert_Value_wrch {14} CONFIG.FIFO_Implementation_rach {Common_Clock_Distributed_RAM} CONFIG.Full_Threshold_Assert_Value_rach {15} CONFIG.Empty_Threshold_Assert_Value_rach {14}] [get_ips axis_sync_fifo]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_sync_fifo/axis_sync_fifo.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name cmd_fifo_xgemac_rxif -dir $device_ip_dir
set_property -dict [list CONFIG.Fifo_Implementation {Common_Clock_Block_RAM} CONFIG.Input_Data_Width {16} CONFIG.Output_Data_Width {16} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.Use_Embedded_Registers {false} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Enable_Safety_Circuit {false}] [get_ips cmd_fifo_xgemac_rxif]
generate_target {instantiation_template} [get_files $device_ip_dir/cmd_fifo_xgemac_rxif/cmd_fifo_xgemac_rxif.xci]
update_compile_order -fileset sources_1

create_ip -name fifo_generator -vendor xilinx.com -library ip -version 13.2 -module_name cmd_fifo_xgemac_txif -dir $device_ip_dir
set_property -dict [list CONFIG.Fifo_Implementation {Common_Clock_Block_RAM} CONFIG.Input_Data_Width {1} CONFIG.Output_Data_Width {1} CONFIG.Reset_Type {Asynchronous_Reset} CONFIG.Full_Flags_Reset_Value {1} CONFIG.Full_Threshold_Assert_Value {1022} CONFIG.Full_Threshold_Negate_Value {1021} CONFIG.Enable_Safety_Circuit {false}] [get_ips cmd_fifo_xgemac_txif]
generate_target {instantiation_template} [get_files $device_ip_dir/cmd_fifo_xgemac_txif/cmd_fifo_xgemac_txif.xci]
update_compile_order -fileset sources_1

#Register slices

create_ip -name axis_register_slice -vendor xilinx.com -library ip -version 1.1 -module_name axis_register_slice_64 -dir $device_ip_dir
set_property -dict [list CONFIG.TDATA_NUM_BYTES {8} CONFIG.HAS_TKEEP {1} CONFIG.HAS_TLAST {1}] [get_ips axis_register_slice_64]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_register_slice_64/axis_register_slice_64.xci]
update_compile_order -fileset sources_1

#Interconnects

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_3to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {3} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_S02_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.S02_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true} CONFIG.M00_S02_CONNECTIVITY {true}] [get_ips axis_interconnect_3to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_3to1/axis_interconnect_3to1.xci]
update_compile_order -fileset sources_1

create_ip -name axis_interconnect -vendor xilinx.com -library ip -version 1.1 -module_name axis_interconnect_2to1 -dir $device_ip_dir
set_property -dict [list CONFIG.C_NUM_SI_SLOTS {2} CONFIG.SWITCH_TDATA_NUM_BYTES {8} CONFIG.HAS_TSTRB {false} CONFIG.HAS_TID {false} CONFIG.HAS_TDEST {false} CONFIG.SWITCH_PACKET_MODE {true} CONFIG.C_SWITCH_MAX_XFERS_PER_ARB {0} CONFIG.C_M00_AXIS_REG_CONFIG {1} CONFIG.C_S00_AXIS_REG_CONFIG {1} CONFIG.C_S01_AXIS_REG_CONFIG {1} CONFIG.C_SWITCH_NUM_CYCLES_TIMEOUT {0} CONFIG.M00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S00_AXIS_TDATA_NUM_BYTES {8} CONFIG.S01_AXIS_TDATA_NUM_BYTES {8} CONFIG.M00_S01_CONNECTIVITY {true}] [get_ips axis_interconnect_2to1]
generate_target {instantiation_template} [get_files $device_ip_dir/axis_interconnect_2to1/axis_interconnect_2to1.xci]
update_compile_order -fileset sources_1

