`timescale 1ns / 1ps
`default_nettype none
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: David Sidler
// 
// Create Date: 05/09/2018 02:32:06 PM
// Design Name: 
// Module Name: mem_inf
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module mem_inf #(
    parameter ENABLE_DDR0 = 1,
    parameter ENABLE_DDR1 = 1
)(
input wire               clk156_25,
input wire               reset156_25_n,
input wire clk212,
input wire clk200,
input wire sys_rst,

//ddr3 pins
//SODIMM 0
// Inouts
inout wire [63:0]                         c0_ddr3_dq,
inout wire [7:0]                        c0_ddr3_dqs_n,
inout wire [7:0]                        c0_ddr3_dqs_p,

// output wires
output wire [15:0]                     c0_ddr3_addr,
output wire [2:0]                      c0_ddr3_ba,
output wire                            c0_ddr3_ras_n,
output wire                            c0_ddr3_cas_n,
output wire                            c0_ddr3_we_n,
output wire                            c0_ddr3_reset_n,
output wire                        c0_ddr3_ck_p,
output wire                        c0_ddr3_ck_n,
output wire                        c0_ddr3_cke,
output wire                        c0_ddr3_cs_n,
output wire [7:0]                  c0_ddr3_dm,
output wire                        c0_ddr3_odt,
output wire                        c0_ui_clk,
output wire                        c0_init_calib_complete,

//SODIMM 1
// Inouts
inout wire [63:0]                         c1_ddr3_dq,
inout wire [7:0]                        c1_ddr3_dqs_n,
inout wire [7:0]                        c1_ddr3_dqs_p,

// output wires
output wire [15:0]                     c1_ddr3_addr,
output wire [2:0]                      c1_ddr3_ba,
output wire                            c1_ddr3_ras_n,
output wire                            c1_ddr3_cas_n,
output wire                            c1_ddr3_we_n,
output wire                            c1_ddr3_reset_n,
output wire                        c1_ddr3_ck_p,
output wire                        c1_ddr3_ck_n,
output wire                        c1_ddr3_cke,
output wire                        c1_ddr3_cs_n,
output wire [7:0]                  c1_ddr3_dm,
output wire                        c1_ddr3_odt,

//ui output wires
output wire                        c1_ui_clk,
output wire                        c1_init_calib_complete,

//memory access
    //memory 0 read path
    input wire               s_axis_mem0_read_cmd_tvalid,
    output wire              s_axis_mem0_read_cmd_tready,
    input wire[71:0]         s_axis_mem0_read_cmd_tdata,
    //read status
    output wire              m_axis_mem0_read_sts_tvalid,
    input wire               m_axis_mem0_read_sts_tready,
    output wire[7:0]         m_axis_mem0_read_sts_tdata,
    //read stream
    output wire[63:0]        m_axis_mem0_read_tdata,
    output wire[7:0]         m_axis_mem0_read_tkeep,
    output wire              m_axis_mem0_read_tlast,
    output wire              m_axis_mem0_read_tvalid,
    input wire               m_axis_mem0_read_tready,
    
    //memory 0 write path
    input wire               s_axis_mem0_write_cmd_tvalid,
    output wire              s_axis_mem0_write_cmd_tready,
    input wire[71:0]         s_axis_mem0_write_cmd_tdata,
    //write status
    output wire              m_axis_mem0_write_sts_tvalid,
    input wire               m_axis_mem0_write_sts_tready,
    output wire[7:0]         m_axis_mem0_write_sts_tdata,
    //write stream
    input wire[63:0]         s_axis_mem0_write_tdata,
    input wire[7:0]          s_axis_mem0_write_tkeep,
    input wire               s_axis_mem0_write_tlast,
    input wire               s_axis_mem0_write_tvalid,
    output wire              s_axis_mem0_write_tready,
    
    //memory 1 read path
    input wire               s_axis_mem1_read_cmd_tvalid,
    output wire              s_axis_mem1_read_cmd_tready,
    input wire[71:0]         s_axis_mem1_read_cmd_tdata,
    //read status
    output wire              m_axis_mem1_read_sts_tvalid,
    input wire               m_axis_mem1_read_sts_tready,
    output wire[7:0]         m_axis_mem1_read_sts_tdata,
    //read stream
    output wire[63:0]        m_axis_mem1_read_tdata,
    output wire[7:0]         m_axis_mem1_read_tkeep,
    output wire              m_axis_mem1_read_tlast,
    output wire              m_axis_mem1_read_tvalid,
    input wire               m_axis_mem1_read_tready,
    
    //memory 1 write path
    input wire               s_axis_mem1_write_cmd_tvalid,
    output wire              s_axis_mem1_write_cmd_tready,
    input wire[71:0]         s_axis_mem1_write_cmd_tdata,
    //write status
    output wire              m_axis_mem1_write_sts_tvalid,
    input wire               m_axis_mem1_write_sts_tready,
    output wire[7:0]         m_axis_mem1_write_sts_tdata,
    //write stream
    input wire[63:0]         s_axis_mem1_write_tdata,
    input wire[7:0]          s_axis_mem1_write_tkeep,
    input wire               s_axis_mem1_write_tlast,
    input wire               s_axis_mem1_write_tvalid,
    output wire              s_axis_mem1_write_tready
);

localparam C0_C_S_AXI_ID_WIDTH = 1;
localparam C0_C_S_AXI_ADDR_WIDTH = 33;
localparam C0_C_S_AXI_DATA_WIDTH = 512;
localparam C1_C_S_AXI_ID_WIDTH = 1;
localparam C1_C_S_AXI_ADDR_WIDTH = 33;
localparam C1_C_S_AXI_DATA_WIDTH = 512;

 // user interface signals
wire                                    c0_ui_clk_sync_rst;
wire                                    c0_mmcm_locked;
reg                                     c0_aresetn_r; 
  // Slave Interface Write Address Ports
wire [C0_C_S_AXI_ID_WIDTH-1:0]          c0_s_axi_awid;
wire [C0_C_S_AXI_ADDR_WIDTH-1:0]        c0_s_axi_awaddr;
wire [7:0]                              c0_s_axi_awlen;
wire [2:0]                              c0_s_axi_awsize;
wire [1:0]                              c0_s_axi_awburst;
wire [0:0]                              c0_s_axi_awlock;
wire [3:0]                              c0_s_axi_awcache;
wire [2:0]                              c0_s_axi_awprot;
wire                                    c0_s_axi_awvalid;
wire                                    c0_s_axi_awready;
 // Slave Interface Write Data Ports
wire [C0_C_S_AXI_DATA_WIDTH-1:0]        c0_s_axi_wdata;
wire [(C0_C_S_AXI_DATA_WIDTH/8)-1:0]    c0_s_axi_wstrb;
wire                                    c0_s_axi_wlast;
wire                                    c0_s_axi_wvalid;
wire                                    c0_s_axi_wready;
 // Slave Interface Write Response Ports
wire                                    c0_s_axi_bready;
wire [C0_C_S_AXI_ID_WIDTH-1:0]          c0_s_axi_bid;
wire [1:0]                              c0_s_axi_bresp;
wire                                    c0_s_axi_bvalid;
 // Slave Interface Read Address Ports
wire [C0_C_S_AXI_ID_WIDTH-1:0]          c0_s_axi_arid;
wire [C0_C_S_AXI_ADDR_WIDTH-1:0]        c0_s_axi_araddr;
wire [7:0]                              c0_s_axi_arlen;
wire [2:0]                              c0_s_axi_arsize;
wire [1:0]                              c0_s_axi_arburst;
wire [0:0]                              c0_s_axi_arlock;
wire [3:0]                              c0_s_axi_arcache;
wire [2:0]                              c0_s_axi_arprot;
wire                                    c0_s_axi_arvalid;
wire                                    c0_s_axi_arready;
 // Slave Interface Read Data Ports
wire                                    c0_s_axi_rready;
wire [C0_C_S_AXI_ID_WIDTH-1:0]          c0_s_axi_rid;
wire [C0_C_S_AXI_DATA_WIDTH-1:0]        c0_s_axi_rdata;
wire [1:0]                              c0_s_axi_rresp;
wire                                    c0_s_axi_rlast;
wire                                    c0_s_axi_rvalid;
// user interface signals
wire                                    c1_ui_clk_sync_rst;
wire                                    c1_mmcm_locked;
reg                                     c1_aresetn_r;
// Slave Interface Write Address Ports
wire [C1_C_S_AXI_ID_WIDTH-1:0]          c1_s_axi_awid;
wire [C1_C_S_AXI_ADDR_WIDTH-1:0]        c1_s_axi_awaddr;
wire [7:0]                              c1_s_axi_awlen;
wire [2:0]                              c1_s_axi_awsize;
wire [1:0]                              c1_s_axi_awburst;
wire [0:0]                              c1_s_axi_awlock;
wire [3:0]                              c1_s_axi_awcache;
wire [2:0]                              c1_s_axi_awprot;
wire                                    c1_s_axi_awvalid;
wire                                    c1_s_axi_awready;
// Slave Interface Write Data Ports
wire [C1_C_S_AXI_DATA_WIDTH-1:0]        c1_s_axi_wdata;
wire [(C1_C_S_AXI_DATA_WIDTH/8)-1:0]    c1_s_axi_wstrb;
wire                                    c1_s_axi_wlast;
wire                                    c1_s_axi_wvalid;
wire                                    c1_s_axi_wready;
// Slave Interface Write Response Ports
wire                                    c1_s_axi_bready;
wire [C1_C_S_AXI_ID_WIDTH-1:0]          c1_s_axi_bid;
wire [1:0]                              c1_s_axi_bresp;
wire                                    c1_s_axi_bvalid;
// Slave Interface Read Address Ports
wire [C1_C_S_AXI_ID_WIDTH-1:0]          c1_s_axi_arid;
wire [C1_C_S_AXI_ADDR_WIDTH-1:0]        c1_s_axi_araddr;
wire [7:0]                              c1_s_axi_arlen;
wire [2:0]                              c1_s_axi_arsize;
wire [1:0]                              c1_s_axi_arburst;
wire [0:0]                              c1_s_axi_arlock;
wire [3:0]                              c1_s_axi_arcache;
wire [2:0]                              c1_s_axi_arprot;
wire                                    c1_s_axi_arvalid;
wire                                    c1_s_axi_arready;
// Slave Interface Read Data Ports
wire                                    c1_s_axi_rready;
wire [C1_C_S_AXI_ID_WIDTH-1:0]          c1_s_axi_rid;
wire [C1_C_S_AXI_DATA_WIDTH-1:0]        c1_s_axi_rdata;
wire [1:0]                              c1_s_axi_rresp;
wire                                    c1_s_axi_rlast;
wire                                    c1_s_axi_rvalid;

mig_axi_mm_dual u_mig_7series_0
(
    .c0_ddr3_dq(c0_ddr3_dq),
    .c0_ddr3_dqs_n(c0_ddr3_dqs_n),
    .c0_ddr3_dqs_p(c0_ddr3_dqs_p),

    // Outputs
    .c0_ddr3_addr(c0_ddr3_addr),
    .c0_ddr3_ba(c0_ddr3_ba),
    .c0_ddr3_ras_n(c0_ddr3_ras_n),
    .c0_ddr3_cas_n(c0_ddr3_cas_n),
    .c0_ddr3_we_n(c0_ddr3_we_n),
    .c0_ddr3_reset_n(c0_ddr3_reset_n),
    .c0_ddr3_ck_p(c0_ddr3_ck_p),
    .c0_ddr3_ck_n(c0_ddr3_ck_n),
    .c0_ddr3_cke(c0_ddr3_cke),
    .c0_ddr3_cs_n(c0_ddr3_cs_n),
    .c0_ddr3_dm(c0_ddr3_dm),
    .c0_ddr3_odt(c0_ddr3_odt),

 // Inputs
  // Single-ended system clock
    .c0_sys_clk_i(clk212),
  // Single-ended iodelayctrl clk (reference clock)
    .clk_ref_i(clk200),
    //.device_temp_i(12'd0),
     
  // Inouts
    .c1_ddr3_dq(c1_ddr3_dq),
    .c1_ddr3_dqs_n(c1_ddr3_dqs_n),
    .c1_ddr3_dqs_p(c1_ddr3_dqs_p),

  // Outputs
    .c1_ddr3_addr(c1_ddr3_addr),
    .c1_ddr3_ba(c1_ddr3_ba),
    .c1_ddr3_ras_n(c1_ddr3_ras_n),
    .c1_ddr3_cas_n(c1_ddr3_cas_n),
    .c1_ddr3_we_n(c1_ddr3_we_n),
    .c1_ddr3_reset_n(c1_ddr3_reset_n),
    .c1_ddr3_ck_p(c1_ddr3_ck_p),
    .c1_ddr3_ck_n(c1_ddr3_ck_n),
    .c1_ddr3_cke(c1_ddr3_cke),
    .c1_ddr3_cs_n(c1_ddr3_cs_n),
    .c1_ddr3_dm(c1_ddr3_dm),
    .c1_ddr3_odt(c1_ddr3_odt),

  // Inputs
  // Single-ended system clock
    .c1_sys_clk_i(clk212),
  // System reset - Default polarity of sys_rst pin is Active Low.
  // System reset polarity will change based on the option 
  // selected in GUI.
    .sys_rst(sys_rst),
  
  // user interface signals
    .c0_ui_clk(c0_ui_clk),
    .c0_ui_clk_sync_rst(c0_ui_clk_sync_rst),
     
    .c0_mmcm_locked(c0_mmcm_locked),
    .c0_aresetn(c0_aresetn_r),
    .c0_app_sr_req                     (1'b0),
    .c0_app_ref_req                    (1'b0),
    .c0_app_zq_req                     (1'b0),
  
  // Slave Interface Write Address Ports
    .c0_s_axi_awid(c0_s_axi_awid),
    .c0_s_axi_awaddr(c0_s_axi_awaddr),
    .c0_s_axi_awlen(c0_s_axi_awlen),
    .c0_s_axi_awsize(c0_s_axi_awsize),
    .c0_s_axi_awburst(c0_s_axi_awburst),
    .c0_s_axi_awlock(2'b0),//(c0_s_axi_awlock),
    .c0_s_axi_awcache(4'b0),//(c0_s_axi_awcache),
    .c0_s_axi_awprot(3'b0),//(c0_s_axi_awprot),
    .c0_s_axi_awqos(4'b0),
    .c0_s_axi_awvalid(c0_s_axi_awvalid),
    .c0_s_axi_awready(c0_s_axi_awready),
  // Slave Interface Write Data Ports
    .c0_s_axi_wdata(c0_s_axi_wdata),
    .c0_s_axi_wstrb(c0_s_axi_wstrb),
    .c0_s_axi_wlast(c0_s_axi_wlast),
    .c0_s_axi_wvalid(c0_s_axi_wvalid),
    .c0_s_axi_wready(c0_s_axi_wready),
  // Slave Interface Write Response Ports
    .c0_s_axi_bready(c0_s_axi_bready),
    .c0_s_axi_bid(c0_s_axi_bid),
    .c0_s_axi_bresp(c0_s_axi_bresp),
    .c0_s_axi_bvalid(c0_s_axi_bvalid),
  // Slave Interface Read Address Ports
    .c0_s_axi_arid(c0_s_axi_arid),
    .c0_s_axi_araddr(c0_s_axi_araddr),
    .c0_s_axi_arlen(c0_s_axi_arlen),
    .c0_s_axi_arsize(c0_s_axi_arsize),
    .c0_s_axi_arburst(c0_s_axi_arburst),
    .c0_s_axi_arlock(2'b0),//(c0_s_axi_arlock),
    .c0_s_axi_arcache(4'b0),//(c0_s_axi_arcache),
    .c0_s_axi_arprot(3'b0),//(c0_s_axi_arprot),
    .c0_s_axi_arqos(4'h0),
    .c0_s_axi_arvalid(c0_s_axi_arvalid),
    .c0_s_axi_arready(c0_s_axi_arready),
  // Slave Interface Read Data Ports
    .c0_s_axi_rready(c0_s_axi_rready),
    .c0_s_axi_rid(c0_s_axi_rid),
    .c0_s_axi_rdata(c0_s_axi_rdata),
    .c0_s_axi_rresp(c0_s_axi_rresp),
    .c0_s_axi_rlast(c0_s_axi_rlast),
    .c0_s_axi_rvalid(c0_s_axi_rvalid),
    .c0_init_calib_complete(c0_init_calib_complete),
     
  // user interface signals
    .c1_ui_clk(c1_ui_clk),
    .c1_ui_clk_sync_rst(c1_ui_clk_sync_rst),
    .c1_mmcm_locked(c1_mmcm_locked),
    .c1_aresetn(c1_aresetn_r),
    .c1_app_sr_req                     (1'b0),
    .c1_app_ref_req                    (1'b0),
    .c1_app_zq_req                     (1'b0),
          
  // Slave Interface Write Address Ports
    .c1_s_axi_awid(c1_s_axi_awid),
    .c1_s_axi_awaddr(c1_s_axi_awaddr),
    .c1_s_axi_awlen(c1_s_axi_awlen),
    .c1_s_axi_awsize(c1_s_axi_awsize),
    .c1_s_axi_awburst(c1_s_axi_awburst),
    .c1_s_axi_awlock(2'b0),//(c1_s_axi_awlock),
    .c1_s_axi_awcache(4'b0), //(c1_s_axi_awcache),
    .c1_s_axi_awprot(3'b0), //(c1_s_axi_awprot),
    .c1_s_axi_awqos(4'b0),
    .c1_s_axi_awvalid(c1_s_axi_awvalid),
    .c1_s_axi_awready(c1_s_axi_awready),
  // Slave Interface Write Data Ports
    .c1_s_axi_wdata(c1_s_axi_wdata),
    .c1_s_axi_wstrb(c1_s_axi_wstrb),
    .c1_s_axi_wlast(c1_s_axi_wlast),
    .c1_s_axi_wvalid(c1_s_axi_wvalid),
    .c1_s_axi_wready(c1_s_axi_wready),
  // Slave Interface Write Response Ports
    .c1_s_axi_bready(c1_s_axi_bready),
    .c1_s_axi_bid(c1_s_axi_bid),
    .c1_s_axi_bresp(c1_s_axi_bresp),
    .c1_s_axi_bvalid(c1_s_axi_bvalid),
  // Slave Interface Read Address Ports
    .c1_s_axi_arid(c1_s_axi_arid),
    .c1_s_axi_araddr(c1_s_axi_araddr),
    .c1_s_axi_arlen(c1_s_axi_arlen),
    .c1_s_axi_arsize(c1_s_axi_arsize),
    .c1_s_axi_arburst(c1_s_axi_arburst),
    .c1_s_axi_arlock(2'b0), //(c1_s_axi_arlock),
    .c1_s_axi_arcache(4'b0), //(c1_s_axi_arcache),
    .c1_s_axi_arprot(3'b0), //(c1_s_axi_arprot),
    .c1_s_axi_arqos(4'h0),
    .c1_s_axi_arvalid(c1_s_axi_arvalid),
    .c1_s_axi_arready(c1_s_axi_arready),
  // Slave Interface Read Data Ports
    .c1_s_axi_rready(c1_s_axi_rready),
    .c1_s_axi_rid(c1_s_axi_rid),
    .c1_s_axi_rdata(c1_s_axi_rdata),
    .c1_s_axi_rresp(c1_s_axi_rresp),
    .c1_s_axi_rlast(c1_s_axi_rlast),
    .c1_s_axi_rvalid(c1_s_axi_rvalid),
    .c1_init_calib_complete(c1_init_calib_complete)
);

always @(posedge c0_ui_clk)
    c0_aresetn_r <= ~c0_ui_clk_sync_rst & c0_mmcm_locked;
    
always @(posedge c1_ui_clk)
    c1_aresetn_r <= ~c1_ui_clk_sync_rst & c1_mmcm_locked;
   

/*
 * CLOCK CROSSING
 */

wire        axis_mem0_cc_to_dm_write_tvalid;
wire        axis_mem0_cc_to_dm_write_tready;
wire[63:0]  axis_mem0_cc_to_dm_write_tdata;
wire[7:0]   axis_mem0_cc_to_dm_write_tkeep;
wire        axis_mem0_cc_to_dm_write_tlast;

wire        axis_mem0_dm_to_cc_read_tvalid;
wire        axis_mem0_dm_to_cc_read_tready;
wire[63:0]  axis_mem0_dm_to_cc_read_tdata;
wire[7:0]   axis_mem0_dm_to_cc_read_tkeep;
wire        axis_mem0_dm_to_cc_read_tlast;

generate
    if (ENABLE_DDR0 == 1) begin
    
axis_data_fifo_64_cc axis_write_data_fifo_mem0 (
   .s_axis_aclk(clk156_25),                // input wire s_axis_aclk
   .s_axis_aresetn(reset156_25_n),          // input wire s_axis_aresetn
   .s_axis_tvalid(s_axis_mem0_write_tvalid),            // input wire s_axis_tvalid
   .s_axis_tready(s_axis_mem0_write_tready),            // output wire s_axis_tready
   .s_axis_tdata(s_axis_mem0_write_tdata),              // input wire [255 : 0] s_axis_tdata
   .s_axis_tkeep(s_axis_mem0_write_tkeep),              // input wire [31 : 0] s_axis_tkeep
   .s_axis_tlast(s_axis_mem0_write_tlast),              // input wire s_axis_tlast
   
   .m_axis_aclk(c0_ui_clk),                // input wire m_axis_aclk
   .m_axis_aresetn(c0_aresetn_r),          // input wire m_axis_aresetn
   .m_axis_tvalid(axis_mem0_cc_to_dm_write_tvalid),            // output wire m_axis_tvalid
   .m_axis_tready(axis_mem0_cc_to_dm_write_tready),            // input wire m_axis_tready
   .m_axis_tdata(axis_mem0_cc_to_dm_write_tdata),              // output wire [255 : 0] m_axis_tdata
   .m_axis_tkeep(axis_mem0_cc_to_dm_write_tkeep),              // output wire [31 : 0] m_axis_tkeep
   .m_axis_tlast(axis_mem0_cc_to_dm_write_tlast),              // output wire m_axis_tlast
   
   .axis_data_count(),        // output wire [31 : 0] axis_data_count
   .axis_wr_data_count(),  // output wire [31 : 0] axis_wr_data_count
   .axis_rd_data_count()  // output wire [31 : 0] axis_rd_data_count
 );

axis_data_fifo_64_cc axis_read_data_fifo_mem0 (
   .s_axis_aclk(c0_ui_clk),                // input wire s_axis_aclk
   .s_axis_aresetn(c0_aresetn_r),          // input wire s_axis_aresetn
   .s_axis_tvalid(axis_mem0_dm_to_cc_read_tvalid),            // input wire s_axis_tvalid
   .s_axis_tready(axis_mem0_dm_to_cc_read_tready),            // output wire s_axis_tready
   .s_axis_tdata(axis_mem0_dm_to_cc_read_tdata),              // input wire [255 : 0] s_axis_tdata
   .s_axis_tkeep(axis_mem0_dm_to_cc_read_tkeep),              // input wire [31 : 0] s_axis_tkeep
   .s_axis_tlast(axis_mem0_dm_to_cc_read_tlast),              // input wire s_axis_tlast
   
   .m_axis_aclk(clk156_25),                // input wire m_axis_aclk
   .m_axis_aresetn(reset156_25_n),          // input wire m_axis_aresetn
   .m_axis_tvalid(m_axis_mem0_read_tvalid),            // output wire m_axis_tvalid
   .m_axis_tready(m_axis_mem0_read_tready),            // input wire m_axis_tready
   .m_axis_tdata(m_axis_mem0_read_tdata),              // output wire [255 : 0] m_axis_tdata
   .m_axis_tkeep(m_axis_mem0_read_tkeep),              // output wire [31 : 0] m_axis_tkeep
   .m_axis_tlast(m_axis_mem0_read_tlast),              // output wire m_axis_tlast
   
   .axis_data_count(),        // output wire [31 : 0] axis_data_count
   .axis_wr_data_count(),  // output wire [31 : 0] axis_wr_data_count
   .axis_rd_data_count()  // output wire [31 : 0] axis_rd_data_count
 );
    end
 else begin
     assign s_axis_mem0_write_tready = 1'b1;
     assign m_axis_mem0_read_tvalid = 1'b0;
 end
endgenerate


wire        axis_mem1_cc_to_dm_write_tvalid;
wire        axis_mem1_cc_to_dm_write_tready;
wire[63:0]  axis_mem1_cc_to_dm_write_tdata;
wire[7:0]   axis_mem1_cc_to_dm_write_tkeep;
wire        axis_mem1_cc_to_dm_write_tlast;

wire        axis_mem1_dm_to_cc_read_tvalid;
wire        axis_mem1_dm_to_cc_read_tready;
wire[63:0]  axis_mem1_dm_to_cc_read_tdata;
wire[7:0]   axis_mem1_dm_to_cc_read_tkeep;
wire        axis_mem1_dm_to_cc_read_tlast;

generate
    if (ENABLE_DDR1 == 1) begin
    
axis_data_fifo_64_cc axis_write_data_fifo_mem1 (
   .s_axis_aclk(clk156_25),                // input wire s_axis_aclk
   .s_axis_aresetn(reset156_25_n),          // input wire s_axis_aresetn
   .s_axis_tvalid(s_axis_mem1_write_tvalid),            // input wire s_axis_tvalid
   .s_axis_tready(s_axis_mem1_write_tready),            // output wire s_axis_tready
   .s_axis_tdata(s_axis_mem1_write_tdata),              // input wire [255 : 0] s_axis_tdata
   .s_axis_tkeep(s_axis_mem1_write_tkeep),              // input wire [31 : 0] s_axis_tkeep
   .s_axis_tlast(s_axis_mem1_write_tlast),              // input wire s_axis_tlast
   
   .m_axis_aclk(c1_ui_clk),                // input wire m_axis_aclk
   .m_axis_aresetn(c1_aresetn_r),          // input wire m_axis_aresetn
   .m_axis_tvalid(axis_mem1_cc_to_dm_write_tvalid),            // output wire m_axis_tvalid
   .m_axis_tready(axis_mem1_cc_to_dm_write_tready),            // input wire m_axis_tready
   .m_axis_tdata(axis_mem1_cc_to_dm_write_tdata),              // output wire [255 : 0] m_axis_tdata
   .m_axis_tkeep(axis_mem1_cc_to_dm_write_tkeep),              // output wire [31 : 0] m_axis_tkeep
   .m_axis_tlast(axis_mem1_cc_to_dm_write_tlast),              // output wire m_axis_tlast
   
   .axis_data_count(),        // output wire [31 : 0] axis_data_count
   .axis_wr_data_count(),  // output wire [31 : 0] axis_wr_data_count
   .axis_rd_data_count()  // output wire [31 : 0] axis_rd_data_count
 );

axis_data_fifo_64_cc axis_read_data_fifo_mem1 (
   .s_axis_aclk(c1_ui_clk),                // input wire s_axis_aclk
   .s_axis_aresetn(c1_aresetn_r),          // input wire s_axis_aresetn
   .s_axis_tvalid(axis_mem1_dm_to_cc_read_tvalid),            // input wire s_axis_tvalid
   .s_axis_tready(axis_mem1_dm_to_cc_read_tready),            // output wire s_axis_tready
   .s_axis_tdata(axis_mem1_dm_to_cc_read_tdata),              // input wire [255 : 0] s_axis_tdata
   .s_axis_tkeep(axis_mem1_dm_to_cc_read_tkeep),              // input wire [31 : 0] s_axis_tkeep
   .s_axis_tlast(axis_mem1_dm_to_cc_read_tlast),              // input wire s_axis_tlast
   
   .m_axis_aclk(clk156_25),                // input wire m_axis_aclk
   .m_axis_aresetn(reset156_25_n),          // input wire m_axis_aresetn
   .m_axis_tvalid(m_axis_mem1_read_tvalid),            // output wire m_axis_tvalid
   .m_axis_tready(m_axis_mem1_read_tready),            // input wire m_axis_tready
   .m_axis_tdata(m_axis_mem1_read_tdata),              // output wire [255 : 0] m_axis_tdata
   .m_axis_tkeep(m_axis_mem1_read_tkeep),              // output wire [31 : 0] m_axis_tkeep
   .m_axis_tlast(m_axis_mem1_read_tlast),              // output wire m_axis_tlast
   
   .axis_data_count(),        // output wire [31 : 0] axis_data_count
   .axis_wr_data_count(),  // output wire [31 : 0] axis_wr_data_count
   .axis_rd_data_count()  // output wire [31 : 0] axis_rd_data_count
 );
    end
    else begin
        assign s_axis_mem1_write_tready = 1'b1;
        assign m_axis_mem1_read_tvalid = 1'b0;
    end
endgenerate
/*
 * DATA MOVERS
 */


generate
    if (ENABLE_DDR0 == 1) begin
wire m0_s2mm_err;
wire m0_mm2s_err;

axi_datamover_64_to_512 datamover_m0 (
    .m_axi_mm2s_aclk(c0_ui_clk),// : IN STD_LOGIC;
    .m_axi_mm2s_aresetn(c0_aresetn_r), //: IN STD_LOGIC;
    .mm2s_err(m0_mm2s_err), //: OUT STD_LOGIC;
    .m_axis_mm2s_cmdsts_aclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_mm2s_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tvalid(s_axis_mem0_read_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tready(s_axis_mem0_read_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_mm2s_cmd_tdata(s_axis_mem0_read_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_mm2s_sts_tvalid(m_axis_mem0_read_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_sts_tready(m_axis_mem0_read_sts_tready), //: IN STD_LOGIC;
    .m_axis_mm2s_sts_tdata(m_axis_mem0_read_sts_tdata), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axis_mm2s_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(0 DOWNTO 0);
    .m_axis_mm2s_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_mm2s_arid(c0_s_axi_arid), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_araddr(c0_s_axi_araddr), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_mm2s_arlen(c0_s_axi_arlen), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_mm2s_arsize(c0_s_axi_arsize), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arburst(c0_s_axi_arburst), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_arprot(c0_s_axi_arprot), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arcache(c0_s_axi_arcache), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_aruser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_arvalid(c0_s_axi_arvalid), //: OUT STD_LOGIC;
    .m_axi_mm2s_arready(c0_s_axi_arready), //: IN STD_LOGIC;
    .m_axi_mm2s_rdata(c0_s_axi_rdata), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_mm2s_rresp(c0_s_axi_rresp), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_rlast(c0_s_axi_rlast), //: IN STD_LOGIC;
    .m_axi_mm2s_rvalid(c0_s_axi_rvalid), //: IN STD_LOGIC;
    .m_axi_mm2s_rready(c0_s_axi_rready), //: OUT STD_LOGIC;
    .m_axis_mm2s_tdata(axis_mem0_dm_to_cc_read_tdata), //: OUT STD_LOGIC_VECTOR(255 DOWNTO 0);
    .m_axis_mm2s_tkeep(axis_mem0_dm_to_cc_read_tkeep), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axis_mm2s_tlast(axis_mem0_dm_to_cc_read_tlast), //: OUT STD_LOGIC;
    .m_axis_mm2s_tvalid(axis_mem0_dm_to_cc_read_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_tready(axis_mem0_dm_to_cc_read_tready), //: IN STD_LOGIC;
    .m_axi_s2mm_aclk(c0_ui_clk), //: IN STD_LOGIC;
    .m_axi_s2mm_aresetn(c0_aresetn_r), //: IN STD_LOGIC;
    .s2mm_err(m0_s2mm_err), //: OUT STD_LOGIC;
    .m_axis_s2mm_cmdsts_awclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_s2mm_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tvalid(s_axis_mem0_write_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tready(s_axis_mem0_write_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_s2mm_cmd_tdata(s_axis_mem0_write_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_s2mm_sts_tvalid(m_axis_mem0_write_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_s2mm_sts_tready(m_axis_mem0_write_sts_tready), //: IN STD_LOGIC;
    .m_axis_s2mm_sts_tdata(m_axis_mem0_write_sts_tdata), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axis_s2mm_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axis_s2mm_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_s2mm_awid(c0_s_axi_awid), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awaddr(c0_s_axi_awaddr), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_s2mm_awlen(c0_s_axi_awlen), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_s2mm_awsize(c0_s_axi_awsize), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awburst(c0_s_axi_awburst), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_awprot(c0_s_axi_awprot), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awcache(c0_s_axi_awcache), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awuser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awvalid(c0_s_axi_awvalid), //: OUT STD_LOGIC;
    .m_axi_s2mm_awready(c0_s_axi_awready), //: IN STD_LOGIC;
    .m_axi_s2mm_wdata(c0_s_axi_wdata), //: OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_s2mm_wstrb(c0_s_axi_wstrb), //: OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    .m_axi_s2mm_wlast(c0_s_axi_wlast), //: OUT STD_LOGIC;
    .m_axi_s2mm_wvalid(c0_s_axi_wvalid), //: OUT STD_LOGIC;
    .m_axi_s2mm_wready(c0_s_axi_wready), //: IN STD_LOGIC;
    .m_axi_s2mm_bresp(c0_s_axi_bresp), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_bvalid(c0_s_axi_bvalid), //: IN STD_LOGIC;
    .m_axi_s2mm_bready(c0_s_axi_bready), //: OUT STD_LOGIC;
    .s_axis_s2mm_tdata(axis_mem0_cc_to_dm_write_tdata), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .s_axis_s2mm_tkeep(axis_mem0_cc_to_dm_write_tkeep), //: IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    .s_axis_s2mm_tlast(axis_mem0_cc_to_dm_write_tlast), //: IN STD_LOGIC;
    .s_axis_s2mm_tvalid(axis_mem0_cc_to_dm_write_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_tready(axis_mem0_cc_to_dm_write_tready) //: OUT STD_LOGIC;
);
    end
else begin
    assign s_axis_mem0_read_cmd_tready = 1'b1;
    assign m_axis_mem0_read_sts_tvalid = 1'b0;
    assign s_axis_mem0_write_cmd_tready = 1'b1;
    assign m_axis_mem0_write_sts_tvalid = 1'b0;
end
endgenerate



generate
    if (ENABLE_DDR1 == 1) begin
wire m1_s2mm_err;
wire m1_mm2s_err;


axi_datamover_64_to_512 datamover_m1 (
    .m_axi_mm2s_aclk(c1_ui_clk),// : IN STD_LOGIC;
    .m_axi_mm2s_aresetn(c1_aresetn_r), //: IN STD_LOGIC;
    .mm2s_err(m1_mm2s_err), //: OUT STD_LOGIC;
    .m_axis_mm2s_cmdsts_aclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_mm2s_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tvalid(s_axis_mem1_read_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tready(s_axis_mem1_read_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_mm2s_cmd_tdata(s_axis_mem1_read_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_mm2s_sts_tvalid(m_axis_mem1_read_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_sts_tready(m_axis_mem1_read_sts_tready), //: IN STD_LOGIC;
    .m_axis_mm2s_sts_tdata(m_axis_mem1_read_sts_tdata), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axis_mm2s_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(0 DOWNTO 0);
    .m_axis_mm2s_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_mm2s_arid(c1_s_axi_arid), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_araddr(c1_s_axi_araddr), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_mm2s_arlen(c1_s_axi_arlen), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_mm2s_arsize(c1_s_axi_arsize), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arburst(c1_s_axi_arburst), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_arprot(c1_s_axi_arprot), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arcache(c1_s_axi_arcache), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_aruser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_arvalid(c1_s_axi_arvalid), //: OUT STD_LOGIC;
    .m_axi_mm2s_arready(c1_s_axi_arready), //: IN STD_LOGIC;
    .m_axi_mm2s_rdata(c1_s_axi_rdata), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_mm2s_rresp(c1_s_axi_rresp), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_rlast(c1_s_axi_rlast), //: IN STD_LOGIC;
    .m_axi_mm2s_rvalid(c1_s_axi_rvalid), //: IN STD_LOGIC;
    .m_axi_mm2s_rready(c1_s_axi_rready), //: OUT STD_LOGIC;
    .m_axis_mm2s_tdata(axis_mem1_dm_to_cc_read_tdata), //: OUT STD_LOGIC_VECTOR(255 DOWNTO 0);
    .m_axis_mm2s_tkeep(axis_mem1_dm_to_cc_read_tkeep), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axis_mm2s_tlast(axis_mem1_dm_to_cc_read_tlast), //: OUT STD_LOGIC;
    .m_axis_mm2s_tvalid(axis_mem1_dm_to_cc_read_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_tready(axis_mem1_dm_to_cc_read_tready), //: IN STD_LOGIC;
    .m_axi_s2mm_aclk(c1_ui_clk), //: IN STD_LOGIC;
    .m_axi_s2mm_aresetn(c1_aresetn_r), //: IN STD_LOGIC;
    .s2mm_err(m1_s2mm_err), //: OUT STD_LOGIC;
    .m_axis_s2mm_cmdsts_awclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_s2mm_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tvalid(s_axis_mem1_write_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tready(s_axis_mem1_write_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_s2mm_cmd_tdata(s_axis_mem1_write_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_s2mm_sts_tvalid(m_axis_mem1_write_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_s2mm_sts_tready(m_axis_mem1_write_sts_tready), //: IN STD_LOGIC;
    .m_axis_s2mm_sts_tdata(m_axis_mem1_write_sts_tdata), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axis_s2mm_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axis_s2mm_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_s2mm_awid(c1_s_axi_awid), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awaddr(c1_s_axi_awaddr), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_s2mm_awlen(c1_s_axi_awlen), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_s2mm_awsize(c1_s_axi_awsize), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awburst(c1_s_axi_awburst), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_awprot(c1_s_axi_awprot), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awcache(c1_s_axi_awcache), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awuser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awvalid(c1_s_axi_awvalid), //: OUT STD_LOGIC;
    .m_axi_s2mm_awready(c1_s_axi_awready), //: IN STD_LOGIC;
    .m_axi_s2mm_wdata(c1_s_axi_wdata), //: OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_s2mm_wstrb(c1_s_axi_wstrb), //: OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    .m_axi_s2mm_wlast(c1_s_axi_wlast), //: OUT STD_LOGIC;
    .m_axi_s2mm_wvalid(c1_s_axi_wvalid), //: OUT STD_LOGIC;
    .m_axi_s2mm_wready(c1_s_axi_wready), //: IN STD_LOGIC;
    .m_axi_s2mm_bresp(c1_s_axi_bresp), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_bvalid(c1_s_axi_bvalid), //: IN STD_LOGIC;
    .m_axi_s2mm_bready(c1_s_axi_bready), //: OUT STD_LOGIC;
    .s_axis_s2mm_tdata(axis_mem1_cc_to_dm_write_tdata), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .s_axis_s2mm_tkeep(axis_mem1_cc_to_dm_write_tkeep), //: IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    .s_axis_s2mm_tlast(axis_mem1_cc_to_dm_write_tlast), //: IN STD_LOGIC;
    .s_axis_s2mm_tvalid(axis_mem1_cc_to_dm_write_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_tready(axis_mem1_cc_to_dm_write_tready) //: OUT STD_LOGIC;
);
    end
    else begin
        assign s_axis_mem1_read_cmd_tready = 1'b1;
        assign m_axis_mem1_read_sts_tvalid = 1'b0;
        assign s_axis_mem1_write_cmd_tready = 1'b1;
        assign m_axis_mem1_write_sts_tvalid = 1'b0;
    end
endgenerate

endmodule

`default_nettype wire
