

##GT Ref clk differential pair for 10gig eth.  MGTREFCLK0P_116
create_clock -period 6.400 -name xgemac_clk_156 [get_ports xphy_refclk_p]
set_property PACKAGE_PIN T6 [get_ports xphy_refclk_p]

set_property PACKAGE_PIN T2 [get_ports xphy0_txp]

# SFP TX Disable for 10G PHY. Chip package 1157 on alpha data board only breaks out 2 transceivers!
set_property PACKAGE_PIN AC34 [get_ports {sfp_tx_disable[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {sfp_tx_disable[0]}]
set_property PACKAGE_PIN AA34 [get_ports {sfp_tx_disable[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {sfp_tx_disable[1]}]

set_property PACKAGE_PIN AA23 [get_ports sfp_on]
set_property IOSTANDARD LVCMOS18 [get_ports sfp_on]

create_clock -period 6.400 -name clk156 [get_pins n10g_interface_inst/xgbaser_gt_wrapper_inst/clk156_bufg_inst/O]
create_clock -period 12.800 -name dclk [get_pins n10g_interface_inst/xgbaser_gt_wrapper_inst/dclk_bufg_inst/O]
create_clock -period 6.400 -name refclk [get_pins n10g_interface_inst/xgphy_refclk_ibuf/O]

set_clock_groups -name async_xgemac_drpclk -asynchronous -group [get_clocks -include_generated_clocks clk156] -group [get_clocks -include_generated_clocks dclk]

set_clock_groups -name async_ref_gmacTx -asynchronous -group [get_clocks clk156] -group [get_clocks n10g_interface_inst/network_inst_0/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_10gbaser_multi_gt_i/gt0_gtwizard_gth_10gbaser_i/gthe2_i/TXOUTCLK]

set_false_path -from [get_cells {n10g_interface_inst/xgbaser_gt_wrapper_inst/reset_pulse_reg[0]}]

set_property BITSTREAM.GENERAL.COMPRESS true [current_design]

set_property PACKAGE_PIN W27 [get_ports perst_n]
set_property IOSTANDARD LVCMOS18 [get_ports perst_n]
set_property PULLUP true [get_ports perst_n]

set_false_path -from [get_ports perst_n]
set_false_path -from [get_ports pok_dram]

####contraints from DRAM MEM inf

set_property PACKAGE_PIN AH10 [get_ports {c0_ddr3_dm[0]}]
set_property PACKAGE_PIN AF9 [get_ports {c0_ddr3_dm[1]}]
set_property PACKAGE_PIN AM13 [get_ports {c0_ddr3_dm[2]}]
set_property PACKAGE_PIN AL10 [get_ports {c0_ddr3_dm[3]}]
set_property PACKAGE_PIN AL20 [get_ports {c0_ddr3_dm[4]}]
set_property PACKAGE_PIN AJ24 [get_ports {c0_ddr3_dm[5]}]
set_property PACKAGE_PIN AD22 [get_ports {c0_ddr3_dm[6]}]
set_property PACKAGE_PIN AD15 [get_ports {c0_ddr3_dm[7]}]
set_property PACKAGE_PIN AM23 [get_ports {c0_ddr3_dm[8]}]

set_property VCCAUX_IO NORMAL [get_ports {c0_ddr3_dm[*]}]
set_property SLEW FAST [get_ports {c0_ddr3_dm[*]}]
set_property IOSTANDARD SSTL15 [get_ports {c0_ddr3_dm[*]}]


set_property PACKAGE_PIN B32 [get_ports {c1_ddr3_dm[0]}]
set_property PACKAGE_PIN A30 [get_ports {c1_ddr3_dm[1]}]
set_property PACKAGE_PIN E24 [get_ports {c1_ddr3_dm[2]}]
set_property PACKAGE_PIN B26 [get_ports {c1_ddr3_dm[3]}]
set_property PACKAGE_PIN U31 [get_ports {c1_ddr3_dm[4]}]
set_property PACKAGE_PIN R29 [get_ports {c1_ddr3_dm[5]}]
set_property PACKAGE_PIN K34 [get_ports {c1_ddr3_dm[6]}]
set_property PACKAGE_PIN N34 [get_ports {c1_ddr3_dm[7]}]
set_property PACKAGE_PIN P25 [get_ports {c1_ddr3_dm[8]}]

set_property VCCAUX_IO NORMAL [get_ports {c1_ddr3_dm[*]}]
set_property SLEW FAST [get_ports {c1_ddr3_dm[*]}]
set_property IOSTANDARD SSTL15 [get_ports {c1_ddr3_dm[*]}]
# DDR3 SDRAM
set_property PACKAGE_PIN AA24 [get_ports {dram_on[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {dram_on[*]}]
set_property PACKAGE_PIN AB25 [get_ports {dram_on[1]}]
set_property PACKAGE_PIN AA31 [get_ports pok_dram]
set_property IOSTANDARD LVCMOS18 [get_ports pok_dram]


set_property IOSTANDARD LVCMOS18 [get_ports {usr_led[*]}]
set_property PACKAGE_PIN AC33 [get_ports {usr_led[0]}]
set_property PACKAGE_PIN V32 [get_ports {usr_led[1]}]
set_property PACKAGE_PIN V33 [get_ports {usr_led[2]}]
set_property PACKAGE_PIN AB31 [get_ports {usr_led[3]}]
set_property PACKAGE_PIN AB32 [get_ports {usr_led[4]}]
set_property PACKAGE_PIN U30 [get_ports {usr_led[5]}]

set_property IOSTANDARD LVCMOS18 [get_ports usr_sw]
set_property PACKAGE_PIN AB30 [get_ports usr_sw]
#create_clock -name sys_clk233 -period 4.288 [get_ports sys_clk_p]
#
#set_property VCCAUX_IO DONTCARE [get_ports {sys_clk_p}]
#set_property IOSTANDARD DIFF_SSTL15 [get_ports {sys_clk_p}]
#set_property LOC AY18 [get_ports {sys_clk_p}]
#
## Bank: 32 - Byte
#set_property VCCAUX_IO DONTCARE [get_ports {sys_clk_n}]
#set_property IOSTANDARD DIFF_SSTL15 [get_ports {sys_clk_n}]
#set_property LOC AY17 [get_ports {sys_clk_n}]
#
#set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets clk233]
#

#using c0/1_init_calib_complete as input
set_clock_groups -name clk156_pll_i -asynchronous -group [get_clocks clk_pll_i] -group [get_clocks clk156]
set_clock_groups -name clk156_pll_i_1 -asynchronous -group [get_clocks clk_pll_i_1] -group [get_clocks clk156]

#
#set_clock_groups -name async_ref_sys_clk -asynchronous #    -group [get_clocks  sys_clk233] #    -group [get_clocks  mcb_clk_ref]
#
#set_false_path -from [get_cells rst_n_r3_reg__0]
##set_false_path -from [get_cells reset156_25_n_r3_reg__0]
#
#set_false_path -from [get_cells n10g_interface_inst/xgbaser_gt_wrapper_inst/reset_pulse_reg[0]]

#create_debug_core u_ila_0 ila
#set_property ALL_PROBE_SAME_MU true [get_debug_cores u_ila_0]
#set_property ALL_PROBE_SAME_MU_CNT 4 [get_debug_cores u_ila_0]
#set_property C_ADV_TRIGGER true [get_debug_cores u_ila_0]
#set_property C_DATA_DEPTH 1024 [get_debug_cores u_ila_0]
#set_property C_EN_STRG_QUAL true [get_debug_cores u_ila_0]
#set_property C_INPUT_PIPE_STAGES 0 [get_debug_cores u_ila_0]
#set_property C_TRIGIN_EN false [get_debug_cores u_ila_0]
#set_property C_TRIGOUT_EN false [get_debug_cores u_ila_0]
#set_property port_width 1 [get_debug_ports u_ila_0/clk]
#connect_debug_port u_ila_0/clk [get_nets [list mem_inf_inst/n_33_u_mig_7series_0]]
#set_property port_width 33 [get_debug_ports u_ila_0/probe0]
#connect_debug_port u_ila_0/probe0 [get_nets [list {mem_inf_inst/c0_s_axi_araddr_debug[0]} {mem_inf_inst/c0_s_axi_araddr_debug[1]} {mem_inf_inst/c0_s_axi_araddr_debug[2]} {mem_inf_inst/c0_s_axi_araddr_debug[3]} {mem_inf_inst/c0_s_axi_araddr_debug[4]} {mem_inf_inst/c0_s_axi_araddr_debug[5]} {mem_inf_inst/c0_s_axi_araddr_debug[6]} {mem_inf_inst/c0_s_axi_araddr_debug[7]} {mem_inf_inst/c0_s_axi_araddr_debug[8]} {mem_inf_inst/c0_s_axi_araddr_debug[9]} {mem_inf_inst/c0_s_axi_araddr_debug[10]} {mem_inf_inst/c0_s_axi_araddr_debug[11]} {mem_inf_inst/c0_s_axi_araddr_debug[12]} {mem_inf_inst/c0_s_axi_araddr_debug[13]} {mem_inf_inst/c0_s_axi_araddr_debug[14]} {mem_inf_inst/c0_s_axi_araddr_debug[15]} {mem_inf_inst/c0_s_axi_araddr_debug[16]} {mem_inf_inst/c0_s_axi_araddr_debug[17]} {mem_inf_inst/c0_s_axi_araddr_debug[18]} {mem_inf_inst/c0_s_axi_araddr_debug[19]} {mem_inf_inst/c0_s_axi_araddr_debug[20]} {mem_inf_inst/c0_s_axi_araddr_debug[21]} {mem_inf_inst/c0_s_axi_araddr_debug[22]} {mem_inf_inst/c0_s_axi_araddr_debug[23]} {mem_inf_inst/c0_s_axi_araddr_debug[24]} {mem_inf_inst/c0_s_axi_araddr_debug[25]} {mem_inf_inst/c0_s_axi_araddr_debug[26]} {mem_inf_inst/c0_s_axi_araddr_debug[27]} {mem_inf_inst/c0_s_axi_araddr_debug[28]} {mem_inf_inst/c0_s_axi_araddr_debug[29]} {mem_inf_inst/c0_s_axi_araddr_debug[30]} {mem_inf_inst/c0_s_axi_araddr_debug[31]} {mem_inf_inst/c0_s_axi_araddr_debug[32]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 2 [get_debug_ports u_ila_0/probe1]
#connect_debug_port u_ila_0/probe1 [get_nets [list {mem_inf_inst/c0_s_axi_arburst_debug[0]} {mem_inf_inst/c0_s_axi_arburst_debug[1]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 4 [get_debug_ports u_ila_0/probe2]
#connect_debug_port u_ila_0/probe2 [get_nets [list {mem_inf_inst/c0_s_axi_arcache_debug[0]} {mem_inf_inst/c0_s_axi_arcache_debug[1]} {mem_inf_inst/c0_s_axi_arcache_debug[2]} {mem_inf_inst/c0_s_axi_arcache_debug[3]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 8 [get_debug_ports u_ila_0/probe3]
#connect_debug_port u_ila_0/probe3 [get_nets [list {mem_inf_inst/c0_s_axi_arlen_debug[0]} {mem_inf_inst/c0_s_axi_arlen_debug[1]} {mem_inf_inst/c0_s_axi_arlen_debug[2]} {mem_inf_inst/c0_s_axi_arlen_debug[3]} {mem_inf_inst/c0_s_axi_arlen_debug[4]} {mem_inf_inst/c0_s_axi_arlen_debug[5]} {mem_inf_inst/c0_s_axi_arlen_debug[6]} {mem_inf_inst/c0_s_axi_arlen_debug[7]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 3 [get_debug_ports u_ila_0/probe4]
#connect_debug_port u_ila_0/probe4 [get_nets [list {mem_inf_inst/c0_s_axi_arprot_debug[0]} {mem_inf_inst/c0_s_axi_arprot_debug[1]} {mem_inf_inst/c0_s_axi_arprot_debug[2]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 3 [get_debug_ports u_ila_0/probe5]
#connect_debug_port u_ila_0/probe5 [get_nets [list {mem_inf_inst/c0_s_axi_arsize_debug[0]} {mem_inf_inst/c0_s_axi_arsize_debug[1]} {mem_inf_inst/c0_s_axi_arsize_debug[2]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 33 [get_debug_ports u_ila_0/probe6]
#connect_debug_port u_ila_0/probe6 [get_nets [list {mem_inf_inst/c0_s_axi_awaddr_debug[0]} {mem_inf_inst/c0_s_axi_awaddr_debug[1]} {mem_inf_inst/c0_s_axi_awaddr_debug[2]} {mem_inf_inst/c0_s_axi_awaddr_debug[3]} {mem_inf_inst/c0_s_axi_awaddr_debug[4]} {mem_inf_inst/c0_s_axi_awaddr_debug[5]} {mem_inf_inst/c0_s_axi_awaddr_debug[6]} {mem_inf_inst/c0_s_axi_awaddr_debug[7]} {mem_inf_inst/c0_s_axi_awaddr_debug[8]} {mem_inf_inst/c0_s_axi_awaddr_debug[9]} {mem_inf_inst/c0_s_axi_awaddr_debug[10]} {mem_inf_inst/c0_s_axi_awaddr_debug[11]} {mem_inf_inst/c0_s_axi_awaddr_debug[12]} {mem_inf_inst/c0_s_axi_awaddr_debug[13]} {mem_inf_inst/c0_s_axi_awaddr_debug[14]} {mem_inf_inst/c0_s_axi_awaddr_debug[15]} {mem_inf_inst/c0_s_axi_awaddr_debug[16]} {mem_inf_inst/c0_s_axi_awaddr_debug[17]} {mem_inf_inst/c0_s_axi_awaddr_debug[18]} {mem_inf_inst/c0_s_axi_awaddr_debug[19]} {mem_inf_inst/c0_s_axi_awaddr_debug[20]} {mem_inf_inst/c0_s_axi_awaddr_debug[21]} {mem_inf_inst/c0_s_axi_awaddr_debug[22]} {mem_inf_inst/c0_s_axi_awaddr_debug[23]} {mem_inf_inst/c0_s_axi_awaddr_debug[24]} {mem_inf_inst/c0_s_axi_awaddr_debug[25]} {mem_inf_inst/c0_s_axi_awaddr_debug[26]} {mem_inf_inst/c0_s_axi_awaddr_debug[27]} {mem_inf_inst/c0_s_axi_awaddr_debug[28]} {mem_inf_inst/c0_s_axi_awaddr_debug[29]} {mem_inf_inst/c0_s_axi_awaddr_debug[30]} {mem_inf_inst/c0_s_axi_awaddr_debug[31]} {mem_inf_inst/c0_s_axi_awaddr_debug[32]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 2 [get_debug_ports u_ila_0/probe7]
#connect_debug_port u_ila_0/probe7 [get_nets [list {mem_inf_inst/c0_s_axi_awburst_debug[0]} {mem_inf_inst/c0_s_axi_awburst_debug[1]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 4 [get_debug_ports u_ila_0/probe8]
#connect_debug_port u_ila_0/probe8 [get_nets [list {mem_inf_inst/c0_s_axi_awcache_debug[0]} {mem_inf_inst/c0_s_axi_awcache_debug[1]} {mem_inf_inst/c0_s_axi_awcache_debug[2]} {mem_inf_inst/c0_s_axi_awcache_debug[3]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 8 [get_debug_ports u_ila_0/probe9]
#connect_debug_port u_ila_0/probe9 [get_nets [list {mem_inf_inst/c0_s_axi_awlen_debug[0]} {mem_inf_inst/c0_s_axi_awlen_debug[1]} {mem_inf_inst/c0_s_axi_awlen_debug[2]} {mem_inf_inst/c0_s_axi_awlen_debug[3]} {mem_inf_inst/c0_s_axi_awlen_debug[4]} {mem_inf_inst/c0_s_axi_awlen_debug[5]} {mem_inf_inst/c0_s_axi_awlen_debug[6]} {mem_inf_inst/c0_s_axi_awlen_debug[7]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 3 [get_debug_ports u_ila_0/probe10]
#connect_debug_port u_ila_0/probe10 [get_nets [list {mem_inf_inst/c0_s_axi_awprot_debug[0]} {mem_inf_inst/c0_s_axi_awprot_debug[1]} {mem_inf_inst/c0_s_axi_awprot_debug[2]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 3 [get_debug_ports u_ila_0/probe11]
#connect_debug_port u_ila_0/probe11 [get_nets [list {mem_inf_inst/c0_s_axi_awsize_debug[0]} {mem_inf_inst/c0_s_axi_awsize_debug[1]} {mem_inf_inst/c0_s_axi_awsize_debug[2]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 2 [get_debug_ports u_ila_0/probe12]
#connect_debug_port u_ila_0/probe12 [get_nets [list {mem_inf_inst/c0_s_axi_bresp_debug[0]} {mem_inf_inst/c0_s_axi_bresp_debug[1]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 2 [get_debug_ports u_ila_0/probe13]
#connect_debug_port u_ila_0/probe13 [get_nets [list {mem_inf_inst/c0_s_axi_rresp_debug[0]} {mem_inf_inst/c0_s_axi_rresp_debug[1]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 64 [get_debug_ports u_ila_0/probe14]
#connect_debug_port u_ila_0/probe14 [get_nets [list {mem_inf_inst/c0_s_axi_wstrb_debug[0]} {mem_inf_inst/c0_s_axi_wstrb_debug[1]} {mem_inf_inst/c0_s_axi_wstrb_debug[2]} {mem_inf_inst/c0_s_axi_wstrb_debug[3]} {mem_inf_inst/c0_s_axi_wstrb_debug[4]} {mem_inf_inst/c0_s_axi_wstrb_debug[5]} {mem_inf_inst/c0_s_axi_wstrb_debug[6]} {mem_inf_inst/c0_s_axi_wstrb_debug[7]} {mem_inf_inst/c0_s_axi_wstrb_debug[8]} {mem_inf_inst/c0_s_axi_wstrb_debug[9]} {mem_inf_inst/c0_s_axi_wstrb_debug[10]} {mem_inf_inst/c0_s_axi_wstrb_debug[11]} {mem_inf_inst/c0_s_axi_wstrb_debug[12]} {mem_inf_inst/c0_s_axi_wstrb_debug[13]} {mem_inf_inst/c0_s_axi_wstrb_debug[14]} {mem_inf_inst/c0_s_axi_wstrb_debug[15]} {mem_inf_inst/c0_s_axi_wstrb_debug[16]} {mem_inf_inst/c0_s_axi_wstrb_debug[17]} {mem_inf_inst/c0_s_axi_wstrb_debug[18]} {mem_inf_inst/c0_s_axi_wstrb_debug[19]} {mem_inf_inst/c0_s_axi_wstrb_debug[20]} {mem_inf_inst/c0_s_axi_wstrb_debug[21]} {mem_inf_inst/c0_s_axi_wstrb_debug[22]} {mem_inf_inst/c0_s_axi_wstrb_debug[23]} {mem_inf_inst/c0_s_axi_wstrb_debug[24]} {mem_inf_inst/c0_s_axi_wstrb_debug[25]} {mem_inf_inst/c0_s_axi_wstrb_debug[26]} {mem_inf_inst/c0_s_axi_wstrb_debug[27]} {mem_inf_inst/c0_s_axi_wstrb_debug[28]} {mem_inf_inst/c0_s_axi_wstrb_debug[29]} {mem_inf_inst/c0_s_axi_wstrb_debug[30]} {mem_inf_inst/c0_s_axi_wstrb_debug[31]} {mem_inf_inst/c0_s_axi_wstrb_debug[32]} {mem_inf_inst/c0_s_axi_wstrb_debug[33]} {mem_inf_inst/c0_s_axi_wstrb_debug[34]} {mem_inf_inst/c0_s_axi_wstrb_debug[35]} {mem_inf_inst/c0_s_axi_wstrb_debug[36]} {mem_inf_inst/c0_s_axi_wstrb_debug[37]} {mem_inf_inst/c0_s_axi_wstrb_debug[38]} {mem_inf_inst/c0_s_axi_wstrb_debug[39]} {mem_inf_inst/c0_s_axi_wstrb_debug[40]} {mem_inf_inst/c0_s_axi_wstrb_debug[41]} {mem_inf_inst/c0_s_axi_wstrb_debug[42]} {mem_inf_inst/c0_s_axi_wstrb_debug[43]} {mem_inf_inst/c0_s_axi_wstrb_debug[44]} {mem_inf_inst/c0_s_axi_wstrb_debug[45]} {mem_inf_inst/c0_s_axi_wstrb_debug[46]} {mem_inf_inst/c0_s_axi_wstrb_debug[47]} {mem_inf_inst/c0_s_axi_wstrb_debug[48]} {mem_inf_inst/c0_s_axi_wstrb_debug[49]} {mem_inf_inst/c0_s_axi_wstrb_debug[50]} {mem_inf_inst/c0_s_axi_wstrb_debug[51]} {mem_inf_inst/c0_s_axi_wstrb_debug[52]} {mem_inf_inst/c0_s_axi_wstrb_debug[53]} {mem_inf_inst/c0_s_axi_wstrb_debug[54]} {mem_inf_inst/c0_s_axi_wstrb_debug[55]} {mem_inf_inst/c0_s_axi_wstrb_debug[56]} {mem_inf_inst/c0_s_axi_wstrb_debug[57]} {mem_inf_inst/c0_s_axi_wstrb_debug[58]} {mem_inf_inst/c0_s_axi_wstrb_debug[59]} {mem_inf_inst/c0_s_axi_wstrb_debug[60]} {mem_inf_inst/c0_s_axi_wstrb_debug[61]} {mem_inf_inst/c0_s_axi_wstrb_debug[62]} {mem_inf_inst/c0_s_axi_wstrb_debug[63]}]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe15]
#connect_debug_port u_ila_0/probe15 [get_nets [list mem_inf_inst/c0_s_axi_arid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe16]
#connect_debug_port u_ila_0/probe16 [get_nets [list mem_inf_inst/c0_s_axi_arlock_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe17]
#connect_debug_port u_ila_0/probe17 [get_nets [list mem_inf_inst/c0_s_axi_arready_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe18]
#connect_debug_port u_ila_0/probe18 [get_nets [list mem_inf_inst/c0_s_axi_arvalid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe19]
#connect_debug_port u_ila_0/probe19 [get_nets [list mem_inf_inst/c0_s_axi_awid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe20]
#connect_debug_port u_ila_0/probe20 [get_nets [list mem_inf_inst/c0_s_axi_awlock_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe21]
#connect_debug_port u_ila_0/probe21 [get_nets [list mem_inf_inst/c0_s_axi_awready_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe22]
#connect_debug_port u_ila_0/probe22 [get_nets [list mem_inf_inst/c0_s_axi_awvalid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe23]
#connect_debug_port u_ila_0/probe23 [get_nets [list mem_inf_inst/c0_s_axi_bid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe24]
#connect_debug_port u_ila_0/probe24 [get_nets [list mem_inf_inst/c0_s_axi_bready_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe25]
#connect_debug_port u_ila_0/probe25 [get_nets [list mem_inf_inst/c0_s_axi_bvalid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe26]
#connect_debug_port u_ila_0/probe26 [get_nets [list mem_inf_inst/c0_s_axi_rid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe27]
#connect_debug_port u_ila_0/probe27 [get_nets [list mem_inf_inst/c0_s_axi_rlast_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe28]
#connect_debug_port u_ila_0/probe28 [get_nets [list mem_inf_inst/c0_s_axi_rready_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe29]
#connect_debug_port u_ila_0/probe29 [get_nets [list mem_inf_inst/c0_s_axi_rvalid_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe30]
#connect_debug_port u_ila_0/probe30 [get_nets [list mem_inf_inst/c0_s_axi_wlast_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe31]
#connect_debug_port u_ila_0/probe31 [get_nets [list mem_inf_inst/c0_s_axi_wready_debug]]
#create_debug_port u_ila_0 probe
#set_property port_width 1 [get_debug_ports u_ila_0/probe32]
#connect_debug_port u_ila_0/probe32 [get_nets [list mem_inf_inst/c0_s_axi_wvalid_debug]]
#set_property C_CLK_INPUT_FREQ_HZ 300000000 [get_debug_cores dbg_hub]
#set_property C_ENABLE_CLK_DIVIDER false [get_debug_cores dbg_hub]
#set_property C_USER_SCAN_CHAIN 1 [get_debug_cores dbg_hub]
#onnect_debug_port dbg_hub/clk [get_nets u_ila_0_n_33_u_mig_7series_0]
