create_clock -period 5.000 -name mcb_clk_ref [get_ports clk_ref_p]


# Bank: 38 - Byte 
set_property VCCAUX_IO DONTCARE [get_ports clk_ref_p]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_ref_p]

# Bank: 38 - Byte 
set_property VCCAUX_IO DONTCARE [get_ports clk_ref_n]
set_property IOSTANDARD DIFF_SSTL15 [get_ports clk_ref_n]
set_property PACKAGE_PIN H19 [get_ports clk_ref_p]
set_property PACKAGE_PIN G18 [get_ports clk_ref_n]

create_clock -period 6.400 -name xgemac_clk_156 [get_ports xphy_refclk_p]

##GT Ref clk
set_property PACKAGE_PIN AH8 [get_ports xphy_refclk_p]
set_property PACKAGE_PIN AH7 [get_ports xphy_refclk_n]


create_generated_clock -name clk50 -source [get_ports clk_ref_p] -divide_by 4 [get_pins n10g_interface_inst/clk_divide_reg[1]/Q]
#set_clock_sense -positive n10g_interface_inst/clk_divide_reg[1]_i_1/O


#button
set_property PACKAGE_PIN AU38 [get_ports button_east]
set_property IOSTANDARD LVCMOS18 [get_ports button_east]
set_property PACKAGE_PIN AW40 [get_ports button_west]
set_property IOSTANDARD LVCMOS18 [get_ports button_west]

set_property PACKAGE_PIN AR40 [get_ports button_north]
set_property IOSTANDARD LVCMOS18 [get_ports button_north]
set_property PACKAGE_PIN AP40 [get_ports button_south]
set_property IOSTANDARD LVCMOS18 [get_ports button_south]
set_property PACKAGE_PIN AV39 [get_ports button_center]
set_property IOSTANDARD LVCMOS18 [get_ports button_center]


#UART
#set_property PACKAGE_PIN AU36 [get_ports TxD]
#set_property IOSTANDARD LVCMOS18 [get_ports TxD]

#set_property PACKAGE_PIN AU33 [get_ports RxD]
#set_property IOSTANDARD LVCMOS18 [get_ports RxD]

## bram locations
#set_property LOC RAMB36_X11Y69 [get_cells configIp/trmx/imx/ram[0].RAMB36_inst]
#set_property LOC RAMB36_X10Y68 [get_cells configIp/trmx/dmx/ram[0].RAMB36_inst]

# Needed by 10GBASE-R IP XDC
create_clock -name clk156 -period 6.400 [get_pins n10g_interface_inst/xgbaser_gt_wrapper_inst/clk156_bufg_inst/O]
create_clock -name dclk -period 12.800 [get_pins n10g_interface_inst/xgbaser_gt_wrapper_inst/dclk_bufg_inst/O]
create_clock -name refclk -period 6.400 [get_pins n10g_interface_inst/xgphy_refclk_ibuf/O]

# Needed by SmartCam
#create_clock -name clkout0 -period 6.400 [get_pins SmartCamCtl_inst/clk_u/clkout1_buf/O]
#create_clock -name clkout0 -period 6.400 [get_pins SmartCamCtl_inst/clk_u/mmcm_adv_inst/CLKOUT0]
#SmartCamCtl_inst/clk_u/mmcm_adv_inst/CLKOUT0
#SmartCamCtl_inst/clk_u/mmcm_adv_inst/CLKOUT0
#SmartCamCtl_inst/clk_u/mmcm_adv_inst/CLKFBOUT


# SFP TX Disable for 10G PHY
set_property LOC AB41  [get_ports {sfp_tx_disable[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {sfp_tx_disable[0]}]
set_property LOC Y42  [get_ports {sfp_tx_disable[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {sfp_tx_disable[1]}]
set_property LOC AC38  [get_ports {sfp_tx_disable[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {sfp_tx_disable[2]}]
set_property LOC AC40  [get_ports {sfp_tx_disable[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {sfp_tx_disable[3]}]

#10G
#create_generated_clock -name ddrclock -divide_by 1 -invert -source [get_pins DUT/rx_clk_ddr/C] [get_ports DUT/xgmii_rx_clk]
#set_output_delay -max 1.500 -clock [get_clocks ddrclock] [get_ports * -filter {NAME =~ *xgmii_rxd*}]
#set_output_delay -min -1.500 -clock [get_clocks ddrclock] [get_ports * -filter {NAME =~ *xgmii_rxd*}]
#set_output_delay -max 1.500 -clock [get_clocks ddrclock] [get_ports * -filter {NAME =~ *xgmii_rxc*}]
#set_output_delay -min -1.500 -clock [get_clocks ddrclock] [get_ports * -filter {NAME =~ *xgmii_rxc*}]

# False paths for async reset removal synchronizers
#set_false_path -to [get_pins -of_objects [get_cells -hierarchical -filter {NAME =~ ten_gig_eth_pcs_pma_core_support_layer/*shared*txusrclk2*}] -filter {NAME =~ *PRE}]
#set_false_path -to [get_pins -of_objects [get_cells -hierarchical -filter {NAME =~ ten_gig_eth_pcs_pma_core_support_layer/*shared*txusrclk2*}] -filter {NAME =~ *CLR}]
#set_false_path -to [get_pins -of_objects [get_cells -hierarchical -filter {NAME =~ ten_gig_eth_pcs_pma_core_support_layer/*shared*areset_refclk_bufh*}] -filter {NAME =~ *PRE}]
#set_false_path -to [get_pins -of_objects [get_cells -hierarchical -filter {NAME =~ ten_gig_eth_pcs_pma_core_support_layer/*shared*areset_clk156*}] -filter {NAME =~ *PRE}]
#set_false_path -to [get_pins -of_objects [get_cells -hierarchical -filter {NAME =~ ten_gig_eth_pcs_pma_core_support_layer/*shared*mmcm_locked_clk156*}] -filter {NAME =~ *CLR}]

##---------------------------------------------------------------------------------------
## 10GBASE-R constraints
##---------------------------------------------------------------------------------------
## SFP+ Cage mapping on VC709
# P2 --> X1Y13
# P3 --> X1Y12
# P4 --> X1Y14
# P5 --> X1Y15
## GT placement ## MGT_BANK_113

## Sample constraint for GT location
#set_property LOC GTHE2_CHANNEL_X1Y12 [get_cells n10g_interface_inst/network_inst_0/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i]
#set_property LOC GTHE2_CHANNEL_X1Y13 [get_cells network_inst_1/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i]
#set_property LOC GTHE2_CHANNEL_X1Y14 [get_cells network_inst_2/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i]
#set_property LOC GTHE2_CHANNEL_X1Y15 [get_cells network_inst_3/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i]

#set_property LOC GTHE2_CHANNEL_X1Y12 [get_cells DUT/ten_gig_eth_pcs_pma_core_support_layer/ten_gig_eth_pcs_pma_block/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i]
#set_property LOC GTHE2_CHANNEL_X1Y13 [get_cells inst_1/ten_gig_eth_pcs_pma_core_support_layer/ten_gig_eth_pcs_pma_block/*/gt0_gtwizard_gth_10gbaser_i/gthe2_i]
#set_property LOC GTHE2_COMMON_X1Y5 [get_cells */ten_gig_eth_pcs_pma_core_support_layer/ten_gig_eth_pcs_pma_gt_common_block/gthe2_common_0_i]

#set_property IOSTANDARD HSTL_I [get_ports {xgmii_txc[*]}]
#set_property IOSTANDARD HSTL_I [get_ports {xgmii_txd[*]}]

#set_property IOSTANDARD HSTL_I [get_ports {xgmii_rxc[*]}]
#set_property IOSTANDARD HSTL_I [get_ports {xgmii_rxd[*]}]

#set_property IOB TRUE [get_cells {xgmii_rxc_reg[*]}]
#set_property IOB TRUE [get_cells {xgmii_rxd_reg[*]}]

#set_property IOSTANDARD HSTL_I [get_ports xgmii_rx_clk]

set_property PACKAGE_PIN AP4 [get_ports xphy0_txp]
set_property PACKAGE_PIN AP3 [get_ports xphy0_txn]
set_property PACKAGE_PIN AN6 [get_ports xphy0_rxp]
set_property PACKAGE_PIN AN5 [get_ports xphy0_rxn]

#set_property PACKAGE_PIN AN2 [get_ports xphy1_txp]
#set_property PACKAGE_PIN AN1 [get_ports xphy1_txn]
#set_property PACKAGE_PIN AM8 [get_ports xphy1_rxp]
#set_property PACKAGE_PIN AM7 [get_ports xphy1_rxn]

#set_property PACKAGE_PIN AM4 [get_ports xphy2_txp]
#set_property PACKAGE_PIN AM3 [get_ports xphy2_txn]
#set_property PACKAGE_PIN AL6 [get_ports xphy2_rxp]
#set_property PACKAGE_PIN AL5 [get_ports xphy2_rxn]

#set_property PACKAGE_PIN AL2 [get_ports xphy3_txp]
#set_property PACKAGE_PIN AL1 [get_ports xphy3_txn]
#set_property PACKAGE_PIN AJ6 [get_ports xphy3_rxp]
#set_property PACKAGE_PIN AJ5 [get_ports xphy3_rxn]

#create_clock -name xphy_rxusrclkout0 -period 3.103 [get_pins n10g_interface_inst/network_inst_0/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/RXOUTCLK]
#create_clock -name xphy_txusrclkout0 -period 3.103 [get_pins n10g_interface_inst/network_inst_0/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/TXOUTCLK]
#create_clock -name xphy_rxusrclkout1 -period 3.103 [get_pins network_inst_1/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/RXOUTCLK]
#create_clock -name xphy_txusrclkout1 -period 3.103 [get_pins network_inst_1/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/TXOUTCLK]
#create_clock -name xphy_rxusrclkout2 -period 3.103 [get_pins network_inst_2/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/RXOUTCLK]
#create_clock -name xphy_txusrclkout2 -period 3.103 [get_pins network_inst_2/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/TXOUTCLK]
#create_clock -name xphy_rxusrclkout3 -period 3.103 [get_pins network_inst_3/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/RXOUTCLK]
#create_clock -name xphy_txusrclkout3 -period 3.103 [get_pins network_inst_3/ten_gig_eth_pcs_pma_inst/inst/gt0_gtwizard_gth_10gbaser_i/gthe2_i/TXOUTCLK]

##-------------------------------------
## LED Status Pinout   (bottom to top)
##-------------------------------------

set_property PACKAGE_PIN AM39 [get_ports {led[0]}]
set_property PACKAGE_PIN AN39 [get_ports {led[1]}]
set_property PACKAGE_PIN AR37 [get_ports {led[2]}]
set_property PACKAGE_PIN AT37 [get_ports {led[3]}]
set_property PACKAGE_PIN AR35 [get_ports {led[4]}]
set_property PACKAGE_PIN AP41 [get_ports {led[5]}]
set_property PACKAGE_PIN AP42 [get_ports {led[6]}]
set_property PACKAGE_PIN AU39 [get_ports {led[7]}]

set_property IOSTANDARD LVCMOS18 [get_ports {led[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led[7]}]

set_property SLEW SLOW [get_ports {led[7]}]
set_property SLEW SLOW [get_ports {led[6]}]
set_property SLEW SLOW [get_ports {led[5]}]
set_property SLEW SLOW [get_ports {led[4]}]
set_property SLEW SLOW [get_ports {led[3]}]
set_property SLEW SLOW [get_ports {led[2]}]
set_property SLEW SLOW [get_ports {led[1]}]
set_property SLEW SLOW [get_ports {led[0]}]
set_property DRIVE 4 [get_ports {led[7]}]
set_property DRIVE 4 [get_ports {led[6]}]
set_property DRIVE 4 [get_ports {led[5]}]
set_property DRIVE 4 [get_ports {led[4]}]
set_property DRIVE 4 [get_ports {led[3]}]
set_property DRIVE 4 [get_ports {led[2]}]
set_property DRIVE 4 [get_ports {led[1]}]
set_property DRIVE 4 [get_ports {led[0]}]


##
## Switches
##
set_property PACKAGE_PIN AV30 [get_ports {gpio_switch[0]}]
set_property PACKAGE_PIN AY33 [get_ports {gpio_switch[1]}]
set_property PACKAGE_PIN BA31 [get_ports {gpio_switch[2]}]
set_property PACKAGE_PIN BA32 [get_ports {gpio_switch[3]}]
set_property PACKAGE_PIN AW30 [get_ports {gpio_switch[4]}]
set_property PACKAGE_PIN AY30 [get_ports {gpio_switch[5]}]
set_property PACKAGE_PIN BA30 [get_ports {gpio_switch[6]}]
set_property PACKAGE_PIN BB31 [get_ports {gpio_switch[7]}]

set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[2]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[3]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[4]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[5]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[6]}]
set_property IOSTANDARD LVCMOS18 [get_ports {gpio_switch[7]}]

#i2c clk & stuff
set_property IOSTANDARD LVCMOS18 [get_ports i2c_clk]
set_property SLEW SLOW [get_ports i2c_clk]
set_property DRIVE 16 [get_ports i2c_clk]
set_property PULLUP TRUE [get_ports i2c_clk]
set_property LOC AT35 [get_ports i2c_clk]

set_property IOSTANDARD LVCMOS18 [get_ports i2c_data]
set_property SLEW SLOW [get_ports i2c_data]
set_property DRIVE 16 [get_ports i2c_data]
set_property PULLUP TRUE [get_ports i2c_data]
set_property LOC AU32 [get_ports i2c_data]

set_property IOSTANDARD LVCMOS18 [get_ports i2c_mux_rst_n]
set_property SLEW SLOW [get_ports i2c_mux_rst_n]
set_property DRIVE 16 [get_ports i2c_mux_rst_n]
set_property LOC AY42 [get_ports i2c_mux_rst_n]

set_property IOSTANDARD LVCMOS18 [get_ports si5324_rst_n]
set_property SLEW SLOW [get_ports si5324_rst_n]
set_property DRIVE 16 [get_ports si5324_rst_n]
set_property LOC AT36 [get_ports si5324_rst_n]


#Domain crossing constraints
set_clock_groups -name async_mcb_xgemac -asynchronous \
  -group [get_clocks  mcb_clk_ref] \
  -group [get_clocks  clk156]



set_clock_groups -name async_mig_ref_clk50 -asynchronous \
   -group [get_clocks mcb_clk_ref] \
   -group [get_clocks clk50]


#set_clock_groups -name async_rxusrclk_xgemac -asynchronous \
#  -group [get_clocks  xphy_rxusrclkout?] \
#  -group [get_clocks  clk156]

#set_clock_groups -name async_txusrclk_xgemac -asynchronous \
#  -group [get_clocks  xphy_txusrclkout?] \
#  -group [get_clocks  clk156]

#  set_clock_groups -name async_txusrclk_refclk -asynchronous \
#    -group [get_clocks  xphy_txusrclkout?] \
#    -group [get_clocks  -include_generated_clocks refclk]


set_clock_groups -name async_xgemac_drpclk -asynchronous \
   -group [get_clocks -include_generated_clocks clk156] \
   -group [get_clocks -include_generated_clocks dclk]
   
set_clock_groups -name async_xgemac_clk50 -asynchronous \
   -group [get_clocks -include_generated_clocks clk156] \
   -group [get_clocks clk50]
   
####contraints from DRAM MEM inf
create_clock -period 4.708 -name sys_clk [get_ports sys_clk_p]

# PadFunction: IO_L13P_T2_MRCC_32
set_property VCCAUX_IO DONTCARE [get_ports {sys_clk_p}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {sys_clk_p}]
set_property PACKAGE_PIN AY18 [get_ports {sys_clk_p}]

# PadFunction: IO_L13N_T2_MRCC_32
set_property VCCAUX_IO DONTCARE [get_ports {sys_clk_n}]
set_property IOSTANDARD DIFF_SSTL15 [get_ports {sys_clk_n}]
set_property PACKAGE_PIN AY17 [get_ports {sys_clk_n}]

# Reset
# PadFunction: IO_L13P_T2_MRCC_15 
set_property VCCAUX_IO DONTCARE [get_ports {sys_rst_i}]
set_property IOSTANDARD LVCMOS18 [get_ports {sys_rst_i}]
set_property PACKAGE_PIN AV40 [get_ports {sys_rst_i}]


set_property CLOCK_DEDICATED_ROUTE BACKBONE [get_nets clk233]

set_clock_groups -name clk156_pll_i -asynchronous -group [get_clocks clk_pll_i] -group [get_clocks clk156]
set_clock_groups -name clk156_pll_i_1 -asynchronous -group [get_clocks clk_pll_i_1] -group [get_clocks clk156]