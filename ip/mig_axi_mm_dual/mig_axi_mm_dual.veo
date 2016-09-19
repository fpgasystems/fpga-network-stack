//*****************************************************************************
// (c) Copyright 2009 - 2010 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
//
//*****************************************************************************
//   ____  ____
//  /   /\/   /
// /___/  \  /   Vendor             : Xilinx
// \   \   \/    Version            : 2.4
//  \   \        Application        : MIG
//  /   /        Filename           : mig_axi_mm_dual.veo
// /___/   /\    Date Last Modified : $Date: 2011/06/02 08:34:47 $
// \   \  /  \   Date Created       : Tue Sept 21 2010
//  \___\/\___\
//
// Device           : 7 Series
// Design Name      : DDR3 SDRAM
// Purpose          : Template file containing code that can be used as a model
//                    for instantiating a CORE Generator module in a HDL design.
// Revision History :
//*****************************************************************************

// The following must be inserted into your Verilog file for this
// core to be instantiated. Change the instance name and port connections
// (in parentheses) to your own signal names.

//----------- Begin Cut here for INSTANTIATION Template ---// INST_TAG

  mig_axi_mm_dual u_mig_axi_mm_dual (

    // Memory interface ports
    .c0_ddr3_addr                      (c0_ddr3_addr),  // output [15:0]		c0_ddr3_addr
    .c0_ddr3_ba                        (c0_ddr3_ba),  // output [2:0]		c0_ddr3_ba
    .c0_ddr3_cas_n                     (c0_ddr3_cas_n),  // output			c0_ddr3_cas_n
    .c0_ddr3_ck_n                      (c0_ddr3_ck_n),  // output [0:0]		c0_ddr3_ck_n
    .c0_ddr3_ck_p                      (c0_ddr3_ck_p),  // output [0:0]		c0_ddr3_ck_p
    .c0_ddr3_cke                       (c0_ddr3_cke),  // output [0:0]		c0_ddr3_cke
    .c0_ddr3_ras_n                     (c0_ddr3_ras_n),  // output			c0_ddr3_ras_n
    .c0_ddr3_reset_n                   (c0_ddr3_reset_n),  // output			c0_ddr3_reset_n
    .c0_ddr3_we_n                      (c0_ddr3_we_n),  // output			c0_ddr3_we_n
    .c0_ddr3_dq                        (c0_ddr3_dq),  // inout [63:0]		c0_ddr3_dq
    .c0_ddr3_dqs_n                     (c0_ddr3_dqs_n),  // inout [7:0]		c0_ddr3_dqs_n
    .c0_ddr3_dqs_p                     (c0_ddr3_dqs_p),  // inout [7:0]		c0_ddr3_dqs_p
    .c0_init_calib_complete            (c0_init_calib_complete),  // output			init_calib_complete
      
	.c0_ddr3_cs_n                      (c0_ddr3_cs_n),  // output [0:0]		c0_ddr3_cs_n
    .c0_ddr3_dm                        (c0_ddr3_dm),  // output [7:0]		c0_ddr3_dm
    .c0_ddr3_odt                       (c0_ddr3_odt),  // output [0:0]		c0_ddr3_odt
    // Application interface ports
    .c0_ui_clk                         (c0_ui_clk),  // output			c0_ui_clk
    .c0_ui_clk_sync_rst                (c0_ui_clk_sync_rst),  // output			c0_ui_clk_sync_rst
    .c0_mmcm_locked                    (c0_mmcm_locked),  // output			c0_mmcm_locked
    .c0_aresetn                        (c0_aresetn),  // input			c0_aresetn
    .c0_app_sr_req                     (c0_app_sr_req),  // input			c0_app_sr_req
    .c0_app_ref_req                    (c0_app_ref_req),  // input			c0_app_ref_req
    .c0_app_zq_req                     (c0_app_zq_req),  // input			c0_app_zq_req
    .c0_app_sr_active                  (c0_app_sr_active),  // output			c0_app_sr_active
    .c0_app_ref_ack                    (c0_app_ref_ack),  // output			c0_app_ref_ack
    .c0_app_zq_ack                     (c0_app_zq_ack),  // output			c0_app_zq_ack
    // Slave Interface Write Address Ports
    .c0_s_axi_awid                     (c0_s_axi_awid),  // input [0:0]			c0_s_axi_awid
    .c0_s_axi_awaddr                   (c0_s_axi_awaddr),  // input [31:0]			c0_s_axi_awaddr
    .c0_s_axi_awlen                    (c0_s_axi_awlen),  // input [7:0]			c0_s_axi_awlen
    .c0_s_axi_awsize                   (c0_s_axi_awsize),  // input [2:0]			c0_s_axi_awsize
    .c0_s_axi_awburst                  (c0_s_axi_awburst),  // input [1:0]			c0_s_axi_awburst
    .c0_s_axi_awlock                   (c0_s_axi_awlock),  // input [0:0]			c0_s_axi_awlock
    .c0_s_axi_awcache                  (c0_s_axi_awcache),  // input [3:0]			c0_s_axi_awcache
    .c0_s_axi_awprot                   (c0_s_axi_awprot),  // input [2:0]			c0_s_axi_awprot
    .c0_s_axi_awqos                    (c0_s_axi_awqos),  // input [3:0]			c0_s_axi_awqos
    .c0_s_axi_awvalid                  (c0_s_axi_awvalid),  // input			c0_s_axi_awvalid
    .c0_s_axi_awready                  (c0_s_axi_awready),  // output			c0_s_axi_awready
    // Slave Interface Write Data Ports
    .c0_s_axi_wdata                    (c0_s_axi_wdata),  // input [511:0]			c0_s_axi_wdata
    .c0_s_axi_wstrb                    (c0_s_axi_wstrb),  // input [63:0]			c0_s_axi_wstrb
    .c0_s_axi_wlast                    (c0_s_axi_wlast),  // input			c0_s_axi_wlast
    .c0_s_axi_wvalid                   (c0_s_axi_wvalid),  // input			c0_s_axi_wvalid
    .c0_s_axi_wready                   (c0_s_axi_wready),  // output			c0_s_axi_wready
    // Slave Interface Write Response Ports
    .c0_s_axi_bid                      (c0_s_axi_bid),  // output [0:0]			c0_s_axi_bid
    .c0_s_axi_bresp                    (c0_s_axi_bresp),  // output [1:0]			c0_s_axi_bresp
    .c0_s_axi_bvalid                   (c0_s_axi_bvalid),  // output			c0_s_axi_bvalid
    .c0_s_axi_bready                   (c0_s_axi_bready),  // input			c0_s_axi_bready
    // Slave Interface Read Address Ports
    .c0_s_axi_arid                     (c0_s_axi_arid),  // input [0:0]			c0_s_axi_arid
    .c0_s_axi_araddr                   (c0_s_axi_araddr),  // input [31:0]			c0_s_axi_araddr
    .c0_s_axi_arlen                    (c0_s_axi_arlen),  // input [7:0]			c0_s_axi_arlen
    .c0_s_axi_arsize                   (c0_s_axi_arsize),  // input [2:0]			c0_s_axi_arsize
    .c0_s_axi_arburst                  (c0_s_axi_arburst),  // input [1:0]			c0_s_axi_arburst
    .c0_s_axi_arlock                   (c0_s_axi_arlock),  // input [0:0]			c0_s_axi_arlock
    .c0_s_axi_arcache                  (c0_s_axi_arcache),  // input [3:0]			c0_s_axi_arcache
    .c0_s_axi_arprot                   (c0_s_axi_arprot),  // input [2:0]			c0_s_axi_arprot
    .c0_s_axi_arqos                    (c0_s_axi_arqos),  // input [3:0]			c0_s_axi_arqos
    .c0_s_axi_arvalid                  (c0_s_axi_arvalid),  // input			c0_s_axi_arvalid
    .c0_s_axi_arready                  (c0_s_axi_arready),  // output			c0_s_axi_arready
    // Slave Interface Read Data Ports
    .c0_s_axi_rid                      (c0_s_axi_rid),  // output [0:0]			c0_s_axi_rid
    .c0_s_axi_rdata                    (c0_s_axi_rdata),  // output [511:0]			c0_s_axi_rdata
    .c0_s_axi_rresp                    (c0_s_axi_rresp),  // output [1:0]			c0_s_axi_rresp
    .c0_s_axi_rlast                    (c0_s_axi_rlast),  // output			c0_s_axi_rlast
    .c0_s_axi_rvalid                   (c0_s_axi_rvalid),  // output			c0_s_axi_rvalid
    .c0_s_axi_rready                   (c0_s_axi_rready),  // input			c0_s_axi_rready
    // System Clock Ports
    .c0_sys_clk_i                       (c0_sys_clk_i),
    // Reference Clock Ports
    .clk_ref_i                      (clk_ref_i),
    // Memory interface ports
    .c1_ddr3_addr                      (c1_ddr3_addr),  // output [15:0]		c1_ddr3_addr
    .c1_ddr3_ba                        (c1_ddr3_ba),  // output [2:0]		c1_ddr3_ba
    .c1_ddr3_cas_n                     (c1_ddr3_cas_n),  // output			c1_ddr3_cas_n
    .c1_ddr3_ck_n                      (c1_ddr3_ck_n),  // output [0:0]		c1_ddr3_ck_n
    .c1_ddr3_ck_p                      (c1_ddr3_ck_p),  // output [0:0]		c1_ddr3_ck_p
    .c1_ddr3_cke                       (c1_ddr3_cke),  // output [0:0]		c1_ddr3_cke
    .c1_ddr3_ras_n                     (c1_ddr3_ras_n),  // output			c1_ddr3_ras_n
    .c1_ddr3_reset_n                   (c1_ddr3_reset_n),  // output			c1_ddr3_reset_n
    .c1_ddr3_we_n                      (c1_ddr3_we_n),  // output			c1_ddr3_we_n
    .c1_ddr3_dq                        (c1_ddr3_dq),  // inout [63:0]		c1_ddr3_dq
    .c1_ddr3_dqs_n                     (c1_ddr3_dqs_n),  // inout [7:0]		c1_ddr3_dqs_n
    .c1_ddr3_dqs_p                     (c1_ddr3_dqs_p),  // inout [7:0]		c1_ddr3_dqs_p
    .c1_init_calib_complete            (c1_init_calib_complete),  // output			init_calib_complete
      
	.c1_ddr3_cs_n                      (c1_ddr3_cs_n),  // output [0:0]		c1_ddr3_cs_n
    .c1_ddr3_dm                        (c1_ddr3_dm),  // output [7:0]		c1_ddr3_dm
    .c1_ddr3_odt                       (c1_ddr3_odt),  // output [0:0]		c1_ddr3_odt
    // Application interface ports
    .c1_ui_clk                         (c1_ui_clk),  // output			c1_ui_clk
    .c1_ui_clk_sync_rst                (c1_ui_clk_sync_rst),  // output			c1_ui_clk_sync_rst
    .c1_mmcm_locked                    (c1_mmcm_locked),  // output			c1_mmcm_locked
    .c1_aresetn                        (c1_aresetn),  // input			c1_aresetn
    .c1_app_sr_req                     (c1_app_sr_req),  // input			c1_app_sr_req
    .c1_app_ref_req                    (c1_app_ref_req),  // input			c1_app_ref_req
    .c1_app_zq_req                     (c1_app_zq_req),  // input			c1_app_zq_req
    .c1_app_sr_active                  (c1_app_sr_active),  // output			c1_app_sr_active
    .c1_app_ref_ack                    (c1_app_ref_ack),  // output			c1_app_ref_ack
    .c1_app_zq_ack                     (c1_app_zq_ack),  // output			c1_app_zq_ack
    // Slave Interface Write Address Ports
    .c1_s_axi_awid                     (c1_s_axi_awid),  // input [0:0]			c1_s_axi_awid
    .c1_s_axi_awaddr                   (c1_s_axi_awaddr),  // input [31:0]			c1_s_axi_awaddr
    .c1_s_axi_awlen                    (c1_s_axi_awlen),  // input [7:0]			c1_s_axi_awlen
    .c1_s_axi_awsize                   (c1_s_axi_awsize),  // input [2:0]			c1_s_axi_awsize
    .c1_s_axi_awburst                  (c1_s_axi_awburst),  // input [1:0]			c1_s_axi_awburst
    .c1_s_axi_awlock                   (c1_s_axi_awlock),  // input [0:0]			c1_s_axi_awlock
    .c1_s_axi_awcache                  (c1_s_axi_awcache),  // input [3:0]			c1_s_axi_awcache
    .c1_s_axi_awprot                   (c1_s_axi_awprot),  // input [2:0]			c1_s_axi_awprot
    .c1_s_axi_awqos                    (c1_s_axi_awqos),  // input [3:0]			c1_s_axi_awqos
    .c1_s_axi_awvalid                  (c1_s_axi_awvalid),  // input			c1_s_axi_awvalid
    .c1_s_axi_awready                  (c1_s_axi_awready),  // output			c1_s_axi_awready
    // Slave Interface Write Data Ports
    .c1_s_axi_wdata                    (c1_s_axi_wdata),  // input [511:0]			c1_s_axi_wdata
    .c1_s_axi_wstrb                    (c1_s_axi_wstrb),  // input [63:0]			c1_s_axi_wstrb
    .c1_s_axi_wlast                    (c1_s_axi_wlast),  // input			c1_s_axi_wlast
    .c1_s_axi_wvalid                   (c1_s_axi_wvalid),  // input			c1_s_axi_wvalid
    .c1_s_axi_wready                   (c1_s_axi_wready),  // output			c1_s_axi_wready
    // Slave Interface Write Response Ports
    .c1_s_axi_bid                      (c1_s_axi_bid),  // output [0:0]			c1_s_axi_bid
    .c1_s_axi_bresp                    (c1_s_axi_bresp),  // output [1:0]			c1_s_axi_bresp
    .c1_s_axi_bvalid                   (c1_s_axi_bvalid),  // output			c1_s_axi_bvalid
    .c1_s_axi_bready                   (c1_s_axi_bready),  // input			c1_s_axi_bready
    // Slave Interface Read Address Ports
    .c1_s_axi_arid                     (c1_s_axi_arid),  // input [0:0]			c1_s_axi_arid
    .c1_s_axi_araddr                   (c1_s_axi_araddr),  // input [31:0]			c1_s_axi_araddr
    .c1_s_axi_arlen                    (c1_s_axi_arlen),  // input [7:0]			c1_s_axi_arlen
    .c1_s_axi_arsize                   (c1_s_axi_arsize),  // input [2:0]			c1_s_axi_arsize
    .c1_s_axi_arburst                  (c1_s_axi_arburst),  // input [1:0]			c1_s_axi_arburst
    .c1_s_axi_arlock                   (c1_s_axi_arlock),  // input [0:0]			c1_s_axi_arlock
    .c1_s_axi_arcache                  (c1_s_axi_arcache),  // input [3:0]			c1_s_axi_arcache
    .c1_s_axi_arprot                   (c1_s_axi_arprot),  // input [2:0]			c1_s_axi_arprot
    .c1_s_axi_arqos                    (c1_s_axi_arqos),  // input [3:0]			c1_s_axi_arqos
    .c1_s_axi_arvalid                  (c1_s_axi_arvalid),  // input			c1_s_axi_arvalid
    .c1_s_axi_arready                  (c1_s_axi_arready),  // output			c1_s_axi_arready
    // Slave Interface Read Data Ports
    .c1_s_axi_rid                      (c1_s_axi_rid),  // output [0:0]			c1_s_axi_rid
    .c1_s_axi_rdata                    (c1_s_axi_rdata),  // output [511:0]			c1_s_axi_rdata
    .c1_s_axi_rresp                    (c1_s_axi_rresp),  // output [1:0]			c1_s_axi_rresp
    .c1_s_axi_rlast                    (c1_s_axi_rlast),  // output			c1_s_axi_rlast
    .c1_s_axi_rvalid                   (c1_s_axi_rvalid),  // output			c1_s_axi_rvalid
    .c1_s_axi_rready                   (c1_s_axi_rready),  // input			c1_s_axi_rready
    // System Clock Ports
    .c1_sys_clk_i                       (c1_sys_clk_i),
    .sys_rst                        (sys_rst) // input sys_rst
    );

// INST_TAG_END ------ End INSTANTIATION Template ---------

// You must compile the wrapper file mig_axi_mm_dual.v when simulating
// the core, mig_axi_mm_dual. When compiling the wrapper file, be sure to
// reference the XilinxCoreLib Verilog simulation library. For detailed
// instructions, please refer to the "CORE Generator Help".
