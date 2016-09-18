`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/11/2013 02:22:48 PM
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
    parameter C0_SIMULATION          =  "FALSE",
    parameter C1_SIMULATION           = "FALSE",
    parameter C0_SIM_BYPASS_INIT_CAL  = "OFF",
    parameter C1_SIM_BYPASS_INIT_CAL = "OFF"
)
(
input               clk156_25,
input               reset156_25_n,
//ddr3 pins
//SODIMM 0
   // Inouts
inout [71:0]        c0_ddr3_dq,
inout [8:0]         c0_ddr3_dqs_n,
inout [8:0]         c0_ddr3_dqs_p,
// Outputs
output [15:0]       c0_ddr3_addr,
output [2:0]        c0_ddr3_ba,
output              c0_ddr3_ras_n,
output              c0_ddr3_cas_n,
output              c0_ddr3_we_n,
output              c0_ddr3_reset_n,
output [1:0]        c0_ddr3_ck_p,
output [1:0]        c0_ddr3_ck_n,
output [1:0]        c0_ddr3_cke,
output [1:0]        c0_ddr3_cs_n,
output [1:0]        c0_ddr3_odt,
output              c0_ui_clk,
output              c0_init_calib_complete,
//CLOCKS and reset
input               c0_sys_clk_p,
input               c0_sys_clk_n,
input               clk_ref_p,
input               clk_ref_n,
input               c1_sys_clk_p,
input               c1_sys_clk_n,
input sys_rst,
//SODIMM 1
inout [71:0]        c1_ddr3_dq,
inout [8:0]         c1_ddr3_dqs_n,
inout [8:0]         c1_ddr3_dqs_p,
// Outputs
output [15:0]       c1_ddr3_addr,
output [2:0]        c1_ddr3_ba,
output              c1_ddr3_ras_n,
output              c1_ddr3_cas_n,
output              c1_ddr3_we_n,
output              c1_ddr3_reset_n,
output [1:0]        c1_ddr3_ck_p,
output [1:0]        c1_ddr3_ck_n,
output [1:0]        c1_ddr3_cke,
output [1:0]        c1_ddr3_cs_n,
output [1:0]        c1_ddr3_odt,
//ui outputs
output              c1_ui_clk,
output              c1_init_calib_complete,
//toe stream interface signals
input               toeTX_s_axis_read_cmd_tvalid,
output              toeTX_s_axis_read_cmd_tready,
input[71:0]         toeTX_s_axis_read_cmd_tdata,
//read status
output              toeTX_m_axis_read_sts_tvalid,
input               toeTX_m_axis_read_sts_tready,
output[7:0]         toeTX_m_axis_read_sts_tdata,
//read stream
output[63:0]        toeTX_m_axis_read_tdata,
output[7:0]         toeTX_m_axis_read_tkeep,
output              toeTX_m_axis_read_tlast,
output              toeTX_m_axis_read_tvalid,
input               toeTX_m_axis_read_tready,

//write commands
input               toeTX_s_axis_write_cmd_tvalid,
output              toeTX_s_axis_write_cmd_tready,
input[71:0]         toeTX_s_axis_write_cmd_tdata,
//write status
output              toeTX_m_axis_write_sts_tvalid,
input               toeTX_m_axis_write_sts_tready,
output[7:0]        toeTX_m_axis_write_sts_tdata,
//write stream
input[63:0]         toeTX_s_axis_write_tdata,
input[7:0]          toeTX_s_axis_write_tkeep,
input               toeTX_s_axis_write_tlast,
input               toeTX_s_axis_write_tvalid,
output              toeTX_s_axis_write_tready,

input               toeRX_s_axis_read_cmd_tvalid,
output              toeRX_s_axis_read_cmd_tready,
input[71:0]         toeRX_s_axis_read_cmd_tdata,
//read status
output              toeRX_m_axis_read_sts_tvalid,
input               toeRX_m_axis_read_sts_tready,
output[7:0]         toeRX_m_axis_read_sts_tdata,
//read stream
output[63:0]        toeRX_m_axis_read_tdata,
output[7:0]         toeRX_m_axis_read_tkeep,
output              toeRX_m_axis_read_tlast,
output              toeRX_m_axis_read_tvalid,
input               toeRX_m_axis_read_tready,

//write commands
input               toeRX_s_axis_write_cmd_tvalid,
output              toeRX_s_axis_write_cmd_tready,
input[71:0]         toeRX_s_axis_write_cmd_tdata,
//write status
output              toeRX_m_axis_write_sts_tvalid,
input               toeRX_m_axis_write_sts_tready,
output[7:0]        toeRX_m_axis_write_sts_tdata,
//write stream
input[63:0]         toeRX_s_axis_write_tdata,
input[7:0]          toeRX_s_axis_write_tkeep,
input               toeRX_s_axis_write_tlast,
input               toeRX_s_axis_write_tvalid,
output              toeRX_s_axis_write_tready,
//ht stream interface signals
input               ht_s_axis_read_cmd_tvalid,
output              ht_s_axis_read_cmd_tready,
input[71:0]         ht_s_axis_read_cmd_tdata,
//read status
output              ht_m_axis_read_sts_tvalid,
input               ht_m_axis_read_sts_tready,
output[7:0]         ht_m_axis_read_sts_tdata,
//read stream
output[511:0]       ht_m_axis_read_tdata,
output[63:0]        ht_m_axis_read_tkeep,
output              ht_m_axis_read_tlast,
output              ht_m_axis_read_tvalid,
input               ht_m_axis_read_tready,

//write commands
input               ht_s_axis_write_cmd_tvalid,
output              ht_s_axis_write_cmd_tready,
input[71:0]         ht_s_axis_write_cmd_tdata,
//write status
output              ht_m_axis_write_sts_tvalid,
input               ht_m_axis_write_sts_tready,
output[7:0]        ht_m_axis_write_sts_tdata,
//write stream
input[511:0]        ht_s_axis_write_tdata,
input[63:0]         ht_s_axis_write_tkeep,
input               ht_s_axis_write_tlast,
input               ht_s_axis_write_tvalid,
output              ht_s_axis_write_tready,

//upd stream interface signals
input               upd_s_axis_read_cmd_tvalid,
output              upd_s_axis_read_cmd_tready,
input[71:0]         upd_s_axis_read_cmd_tdata,
//read status
output              upd_m_axis_read_sts_tvalid,
input               upd_m_axis_read_sts_tready,
output[7:0]         upd_m_axis_read_sts_tdata,
//read stream
output[511:0]       upd_m_axis_read_tdata,
output[63:0]        upd_m_axis_read_tkeep,
output              upd_m_axis_read_tlast,
output              upd_m_axis_read_tvalid,
input               upd_m_axis_read_tready,

//write commands
input               upd_s_axis_write_cmd_tvalid,
output              upd_s_axis_write_cmd_tready,
input[71:0]         upd_s_axis_write_cmd_tdata,
//write status
output              upd_m_axis_write_sts_tvalid,
input               upd_m_axis_write_sts_tready,
output[7:0]        upd_m_axis_write_sts_tdata,
//write stream
input[511:0]        upd_s_axis_write_tdata,
input[63:0]         upd_s_axis_write_tkeep,
input               upd_s_axis_write_tlast,
input               upd_s_axis_write_tvalid,
output              upd_s_axis_write_tready);

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

mig_7series_0 u_mig_7series_0 (
  // Memory interface ports
  .c0_ddr3_addr                         (c0_ddr3_addr),            // output [15:0]        c0_ddr3_addr
  .c0_ddr3_ba                           (c0_ddr3_ba),              // output [2:0]        c0_ddr3_ba
  .c0_ddr3_cas_n                        (c0_ddr3_cas_n),           // output            c0_ddr3_cas_n
  .c0_ddr3_ck_n                         (c0_ddr3_ck_n),            // output [1:0]        c0_ddr3_ck_n
  .c0_ddr3_ck_p                         (c0_ddr3_ck_p),            // output [1:0]        c0_ddr3_ck_p
  .c0_ddr3_cke                          (c0_ddr3_cke),             // output [1:0]        c0_ddr3_cke
  .c0_ddr3_ras_n                        (c0_ddr3_ras_n),           // output            c0_ddr3_ras_n
  .c0_ddr3_reset_n                      (c0_ddr3_reset_n),         // output            c0_ddr3_reset_n
  .c0_ddr3_we_n                         (c0_ddr3_we_n),            // output            c0_ddr3_we_n
  .c0_ddr3_dq                           (c0_ddr3_dq),              // inout [71:0]        c0_ddr3_dq
  .c0_ddr3_dqs_n                        (c0_ddr3_dqs_n),           // inout [8:0]        c0_ddr3_dqs_n
  .c0_ddr3_dqs_p                        (c0_ddr3_dqs_p),           // inout [8:0]        c0_ddr3_dqs_p
  .c0_init_calib_complete               (c0_init_calib_complete),  // output            init_calib_complete
    
  .c0_ddr3_cs_n                         (c0_ddr3_cs_n),            // output [1:0]        c0_ddr3_cs_n
  .c0_ddr3_odt                          (c0_ddr3_odt),             // output [1:0]        c0_ddr3_odt
  // Application interface ports        
  .c0_ui_clk                            (c0_ui_clk),               // output            c0_ui_clk
  .c0_ui_clk_sync_rst                   (c0_ui_clk_sync_rst),      // output            c0_ui_clk_sync_rst
  .c0_mmcm_locked                       (c0_mmcm_locked),          // output            c0_mmcm_locked
  .c0_aresetn                           (c0_aresetn_r),            // input            c0_aresetn
  .c0_app_sr_req                        (0),                       // input            c0_app_sr_req
  .c0_app_ref_req                       (0),                       // input            c0_app_ref_req
  .c0_app_zq_req                        (0),                       // input            c0_app_zq_req
  .c0_app_sr_active                     (c0_app_sr_active),        // output            c0_app_sr_active
  .c0_app_ref_ack                       (c0_app_ref_ack),          // output            c0_app_ref_ack
  .c0_app_zq_ack                        (c0_app_zq_ack),           // output            c0_app_zq_ack
  // Slave Interface Write Address Ports
  .c0_s_axi_awid                        (c0_s_axi_awid),           // input [0:0]            c0_s_axi_awid
  .c0_s_axi_awaddr                      ({1'b0, c0_s_axi_awaddr[31:0]}),  // input [32:0]            c0_s_axi_awaddr
  .c0_s_axi_awlen                       (c0_s_axi_awlen),          // input [7:0]            c0_s_axi_awlen
  .c0_s_axi_awsize                      (c0_s_axi_awsize),         // input [2:0]            c0_s_axi_awsize
  .c0_s_axi_awburst                     (c0_s_axi_awburst),        // input [1:0]            c0_s_axi_awburst
  .c0_s_axi_awlock                      (0),                       // input [0:0]            c0_s_axi_awlock
  .c0_s_axi_awcache                     (0),                       // input [3:0]            c0_s_axi_awcache
  .c0_s_axi_awprot                      (0),                       // input [2:0]            c0_s_axi_awprot
  .c0_s_axi_awqos                       (0),                       // input [3:0]            c0_s_axi_awqos
  .c0_s_axi_awvalid                     (c0_s_axi_awvalid),        // input            c0_s_axi_awvalid
  .c0_s_axi_awready                     (c0_s_axi_awready),        // output            c0_s_axi_awready
  // Slave Interface Write Data Ports
  .c0_s_axi_wdata                       (c0_s_axi_wdata),          // input [511:0]            c0_s_axi_wdata
  .c0_s_axi_wstrb                       (c0_s_axi_wstrb),          // input [63:0]            c0_s_axi_wstrb
  .c0_s_axi_wlast                       (c0_s_axi_wlast),          // input            c0_s_axi_wlast
  .c0_s_axi_wvalid                      (c0_s_axi_wvalid),         // input            c0_s_axi_wvalid
  .c0_s_axi_wready                      (c0_s_axi_wready),         // output            c0_s_axi_wready
  // Slave Interface Write Response Ports
  .c0_s_axi_bid                         (c0_s_axi_bid),            // output [0:0]            c0_s_axi_bid
  .c0_s_axi_bresp                       (c0_s_axi_bresp),          // output [1:0]            c0_s_axi_bresp
  .c0_s_axi_bvalid                      (c0_s_axi_bvalid),         // output            c0_s_axi_bvalid
  .c0_s_axi_bready                      (c0_s_axi_bready),         // input            c0_s_axi_bready
  // Slave Interface Read Address Ports
  .c0_s_axi_arid                        (c0_s_axi_arid),           // input [0:0]            c0_s_axi_arid
  .c0_s_axi_araddr                      ({1'b0, c0_s_axi_araddr[31:0]}),  // input [32:0]            c0_s_axi_araddr
  .c0_s_axi_arlen                       (c0_s_axi_arlen),          // input [7:0]            c0_s_axi_arlen
  .c0_s_axi_arsize                      (c0_s_axi_arsize),         // input [2:0]            c0_s_axi_arsize
  .c0_s_axi_arburst                     (c0_s_axi_arburst),        // input [1:0]            c0_s_axi_arburst
  .c0_s_axi_arlock                      (0),                       // input [0:0]            c0_s_axi_arlock
  .c0_s_axi_arcache                     (0),                       // input [3:0]            c0_s_axi_arcache
  .c0_s_axi_arprot                      (0),                       // input [2:0]            c0_s_axi_arprot
  .c0_s_axi_arqos                       (0),                       // input [3:0]            c0_s_axi_arqos
  .c0_s_axi_arvalid                     (c0_s_axi_arvalid),        // input            c0_s_axi_arvalid
  .c0_s_axi_arready                     (c0_s_axi_arready),        // output            c0_s_axi_arready
  // Slave Interface Read Data Ports
  .c0_s_axi_rid                         (c0_s_axi_rid),            // output [0:0]            c0_s_axi_rid
  .c0_s_axi_rdata                       (c0_s_axi_rdata),          // output [511:0]            c0_s_axi_rdata
  .c0_s_axi_rresp                       (c0_s_axi_rresp),          // output [1:0]            c0_s_axi_rresp
  .c0_s_axi_rlast                       (c0_s_axi_rlast),          // output            c0_s_axi_rlast
  .c0_s_axi_rvalid                      (c0_s_axi_rvalid),         // output            c0_s_axi_rvalid
  .c0_s_axi_rready                      (c0_s_axi_rready),         // input            c0_s_axi_rready
  // AXI CTRL port
  .c0_s_axi_ctrl_awvalid                (0),                       // input            c0_s_axi_ctrl_awvalid
  .c0_s_axi_ctrl_awready                (c0_s_axi_ctrl_awready),   // output            c0_s_axi_ctrl_awready
  .c0_s_axi_ctrl_awaddr                 (0),                       // input [31:0]            c0_s_axi_ctrl_awaddr
  // Slave Interface Write Data Ports   
  .c0_s_axi_ctrl_wvalid                 (0),                       // input            c0_s_axi_ctrl_wvalid
  .c0_s_axi_ctrl_wready                 (c0_s_axi_ctrl_wready),    // output            c0_s_axi_ctrl_wready
  .c0_s_axi_ctrl_wdata                  (0),                       // input [31:0]            c0_s_axi_ctrl_wdata
  // Slave Interface Write Response Ports
  .c0_s_axi_ctrl_bvalid                 (c0_s_axi_ctrl_bvalid),    // output            c0_s_axi_ctrl_bvalid
  .c0_s_axi_ctrl_bready                 (1),                       // input            c0_s_axi_ctrl_bready
  .c0_s_axi_ctrl_bresp                  (c0_s_axi_ctrl_bresp),     // output [1:0]            c0_s_axi_ctrl_bresp
  // Slave Interface Read Address Ports
  .c0_s_axi_ctrl_arvalid                (0),                       // input            c0_s_axi_ctrl_arvalid
  .c0_s_axi_ctrl_arready                (c0_s_axi_ctrl_arready),   // output            c0_s_axi_ctrl_arready
  .c0_s_axi_ctrl_araddr                 (0),                       // input [31:0]            c0_s_axi_ctrl_araddr
  // Slave Interface Read Data Ports
  .c0_s_axi_ctrl_rvalid                 (c0_s_axi_ctrl_rvalid),    // output            c0_s_axi_ctrl_rvalid
  .c0_s_axi_ctrl_rready                 (1),                       // input            c0_s_axi_ctrl_rready
  .c0_s_axi_ctrl_rdata                  (c0_s_axi_ctrl_rdata),     // output [31:0]            c0_s_axi_ctrl_rdata
  .c0_s_axi_ctrl_rresp                  (c0_s_axi_ctrl_rresp),     // output [1:0]            c0_s_axi_ctrl_rresp
  // Interrupt output
  .c0_interrupt                         (),                        // output            c0_interrupt
  .c0_app_ecc_multiple_err              (c0_app_ecc_multiple_err), // output [7:0]            c0_app_ecc_multiple_err
  // System Clock Ports
  .c0_sys_clk_p                         (c0_sys_clk_p),           // input                c0_sys_clk_p
  .c0_sys_clk_n                         (c0_sys_clk_n),           // input                c0_sys_clk_n
  // Reference Clock Ports
  .clk_ref_p                            (clk_ref_p),                  // input                clk_ref_p
  .clk_ref_n                            (clk_ref_n),                  // input                clk_ref_n
  // Memory interface ports
  .c1_ddr3_addr                         (c1_ddr3_addr),            // output [15:0]        c1_ddr3_addr
  .c1_ddr3_ba                           (c1_ddr3_ba),              // output [2:0]        c1_ddr3_ba
  .c1_ddr3_cas_n                        (c1_ddr3_cas_n),           // output            c1_ddr3_cas_n
  .c1_ddr3_ck_n                         (c1_ddr3_ck_n),            // output [1:0]        c1_ddr3_ck_n
  .c1_ddr3_ck_p                         (c1_ddr3_ck_p),            // output [1:0]        c1_ddr3_ck_p
  .c1_ddr3_cke                          (c1_ddr3_cke),             // output [1:0]        c1_ddr3_cke
  .c1_ddr3_ras_n                        (c1_ddr3_ras_n),           // output            c1_ddr3_ras_n
  .c1_ddr3_reset_n                      (c1_ddr3_reset_n),         // output            c1_ddr3_reset_n
  .c1_ddr3_we_n                         (c1_ddr3_we_n),            // output            c1_ddr3_we_n
  .c1_ddr3_dq                           (c1_ddr3_dq),              // inout [71:0]        c1_ddr3_dq
  .c1_ddr3_dqs_n                        (c1_ddr3_dqs_n),           // inout [8:0]        c1_ddr3_dqs_n
  .c1_ddr3_dqs_p                        (c1_ddr3_dqs_p),           // inout [8:0]        c1_ddr3_dqs_p
  .c1_init_calib_complete               (c1_init_calib_complete),  // output            init_calib_complete
    
  .c1_ddr3_cs_n                         (c1_ddr3_cs_n),            // output [1:0]        c1_ddr3_cs_n
  .c1_ddr3_odt                          (c1_ddr3_odt),             // output [1:0]        c1_ddr3_odt
  // Application interface ports
  .c1_ui_clk                            (c1_ui_clk),               // output            c1_ui_clk
  .c1_ui_clk_sync_rst                   (c1_ui_clk_sync_rst),      // output            c1_ui_clk_sync_rst
  .c1_mmcm_locked                       (c1_mmcm_locked),          // output            c1_mmcm_locked
  .c1_aresetn                           (c1_aresetn_r),            // input            c1_aresetn
  .c1_app_sr_req                        (0),                       // input            c1_app_sr_req
  .c1_app_ref_req                       (0),                       // input            c1_app_ref_req
  .c1_app_zq_req                        (0),                       // input            c1_app_zq_req
  .c1_app_sr_active                     (c1_app_sr_active),        // output            c1_app_sr_active
  .c1_app_ref_ack                       (c1_app_ref_ack),          // output            c1_app_ref_ack
  .c1_app_zq_ack                        (c1_app_zq_ack),           // output            c1_app_zq_ack
  // Slave Interface Write Address Ports
  .c1_s_axi_awid                        (c1_s_axi_awid),           // input [0:0]            c1_s_axi_awid
  .c1_s_axi_awaddr                      ({1'b0, c1_s_axi_awaddr[31:0]}),  // input [32:0]            c1_s_axi_awaddr
  .c1_s_axi_awlen                       (c1_s_axi_awlen),          // input [7:0]            c1_s_axi_awlen
  .c1_s_axi_awsize                      (c1_s_axi_awsize),         // input [2:0]            c1_s_axi_awsize
  .c1_s_axi_awburst                     (c1_s_axi_awburst),        // input [1:0]            c1_s_axi_awburst
  .c1_s_axi_awlock                      (0),                       // input [0:0]            c1_s_axi_awlock
  .c1_s_axi_awcache                     (0),                       // input [3:0]            c1_s_axi_awcache
  .c1_s_axi_awprot                      (0),                       // input [2:0]            c1_s_axi_awprot
  .c1_s_axi_awqos                       (0),                       // input [3:0]            c1_s_axi_awqos
  .c1_s_axi_awvalid                     (c1_s_axi_awvalid),        // input            c1_s_axi_awvalid
  .c1_s_axi_awready                     (c1_s_axi_awready),        // output            c1_s_axi_awready
  // Slave Interface Write Data Ports
  .c1_s_axi_wdata                       (c1_s_axi_wdata),          // input [511:0]            c1_s_axi_wdata
  .c1_s_axi_wstrb                       (c1_s_axi_wstrb),          // input [63:0]            c1_s_axi_wstrb
  .c1_s_axi_wlast                       (c1_s_axi_wlast),          // input            c1_s_axi_wlast
  .c1_s_axi_wvalid                      (c1_s_axi_wvalid),         // input            c1_s_axi_wvalid
  .c1_s_axi_wready                      (c1_s_axi_wready),         // output            c1_s_axi_wready
  // Slave Interface Write Response Ports
  .c1_s_axi_bid                         (c1_s_axi_bid),            // output [0:0]            c1_s_axi_bid
  .c1_s_axi_bresp                       (c1_s_axi_bresp),          // output [1:0]            c1_s_axi_bresp
  .c1_s_axi_bvalid                      (c1_s_axi_bvalid),         // output            c1_s_axi_bvalid
  .c1_s_axi_bready                      (c1_s_axi_bready),         // input            c1_s_axi_bready
  // Slave Interface Read Address Ports
  .c1_s_axi_arid                        (c1_s_axi_arid),           // input [0:0]            c1_s_axi_arid
  .c1_s_axi_araddr                      ({1'b0, c1_s_axi_araddr[31:0]}),  // input [32:0]            c1_s_axi_araddr
  .c1_s_axi_arlen                       (c1_s_axi_arlen),          // input [7:0]            c1_s_axi_arlen
  .c1_s_axi_arsize                      (c1_s_axi_arsize),         // input [2:0]            c1_s_axi_arsize
  .c1_s_axi_arburst                     (c1_s_axi_arburst),        // input [1:0]            c1_s_axi_arburst
  .c1_s_axi_arlock                      (0),                       // input [0:0]            c1_s_axi_arlock
  .c1_s_axi_arcache                     (0),                       // input [3:0]            c1_s_axi_arcache
  .c1_s_axi_arprot                      (0),                       // input [2:0]            c1_s_axi_arprot
  .c1_s_axi_arqos                       (0),                       // input [3:0]            c1_s_axi_arqos
  .c1_s_axi_arvalid                     (c1_s_axi_arvalid),        // input            c1_s_axi_arvalid
  .c1_s_axi_arready                     (c1_s_axi_arready),        // output            c1_s_axi_arready
  // Slave Interface Read Data Ports
  .c1_s_axi_rid                         (c1_s_axi_rid),            // output [0:0]            c1_s_axi_rid
  .c1_s_axi_rdata                       (c1_s_axi_rdata),          // output [511:0]            c1_s_axi_rdata
  .c1_s_axi_rresp                       (c1_s_axi_rresp),          // output [1:0]            c1_s_axi_rresp
  .c1_s_axi_rlast                       (c1_s_axi_rlast),          // output            c1_s_axi_rlast
  .c1_s_axi_rvalid                      (c1_s_axi_rvalid),         // output            c1_s_axi_rvalid
  .c1_s_axi_rready                      (c1_s_axi_rready),         // input            c1_s_axi_rready
  // AXI CTRL port
  .c1_s_axi_ctrl_awvalid                (0),                       // input            c1_s_axi_ctrl_awvalid
  .c1_s_axi_ctrl_awready                (c1_s_axi_ctrl_awready),   // output            c1_s_axi_ctrl_awready
  .c1_s_axi_ctrl_awaddr                 (0),                       // input [31:0]            c1_s_axi_ctrl_awaddr
  // Slave Interface Write Data Ports
  .c1_s_axi_ctrl_wvalid                 (0),                       // input            c1_s_axi_ctrl_wvalid
  .c1_s_axi_ctrl_wready                 (c1_s_axi_ctrl_wready),    // output            c1_s_axi_ctrl_wready
  .c1_s_axi_ctrl_wdata                  (0),                       // input [31:0]            c1_s_axi_ctrl_wdata
  // Slave Interface Write Response Ports
  .c1_s_axi_ctrl_bvalid                 (c1_s_axi_ctrl_bvalid),    // output            c1_s_axi_ctrl_bvalid
  .c1_s_axi_ctrl_bready                 (1),                       // input            c1_s_axi_ctrl_bready
  .c1_s_axi_ctrl_bresp                  (c1_s_axi_ctrl_bresp),     // output [1:0]            c1_s_axi_ctrl_bresp
  // Slave Interface Read Address Ports
  .c1_s_axi_ctrl_arvalid                (0),                       // input            c1_s_axi_ctrl_arvalid
  .c1_s_axi_ctrl_arready                (c1_s_axi_ctrl_arready),   // output            c1_s_axi_ctrl_arready
  .c1_s_axi_ctrl_araddr                 (0),                       // input [31:0]            c1_s_axi_ctrl_araddr
  // Slave Interface Read Data Ports
  .c1_s_axi_ctrl_rvalid                 (c1_s_axi_ctrl_rvalid),    // output            c1_s_axi_ctrl_rvalid
  .c1_s_axi_ctrl_rready                 (1),                       // input            c1_s_axi_ctrl_rready
  .c1_s_axi_ctrl_rdata                  (c1_s_axi_ctrl_rdata),     // output [31:0]            c1_s_axi_ctrl_rdata
  .c1_s_axi_ctrl_rresp                  (c1_s_axi_ctrl_rresp),     // output [1:0]            c1_s_axi_ctrl_rresp
  // Interrupt output
  .c1_interrupt                         (),                        // output            c1_interrupt
  .c1_app_ecc_multiple_err              (c1_app_ecc_multiple_err), // output [7:0]            c1_app_ecc_multiple_err
  // System Clock Ports
  .c1_sys_clk_p                         (c1_sys_clk_p),           // input                c1_sys_clk_p
  .c1_sys_clk_n                         (c1_sys_clk_n),           // input                c1_sys_clk_n
  .sys_rst                              (sys_rst)                     // input sys_rst
  );

always @(posedge c0_ui_clk)
    c0_aresetn_r <= ~c0_ui_clk_sync_rst & c0_mmcm_locked;
    
always @(posedge c1_ui_clk)
    c1_aresetn_r <= ~c1_ui_clk_sync_rst & c1_mmcm_locked;
    
    wire [0 : 0] S10_AXI_AWID;
    wire [31 : 0] S10_AXI_AWADDR;
    wire [7 : 0] S10_AXI_AWLEN;
    wire [2 : 0] S10_AXI_AWSIZE;
    wire [1 : 0] S10_AXI_AWBURST;
    wire S10_AXI_AWLOCK;
    wire [3 : 0] S10_AXI_AWCACHE;
    wire [2 : 0] S10_AXI_AWPROT;
    wire [3 : 0] S10_AXI_AWQOS;
    wire S10_AXI_AWVALID;
    wire S10_AXI_AWREADY;
    wire [511 : 0] S10_AXI_WDATA;
    wire [63 : 0] S10_AXI_WSTRB;
    wire S10_AXI_WLAST;
    wire S10_AXI_WVALID;
    wire S10_AXI_WREADY;
    wire [0 : 0] S10_AXI_BID;
    wire [1 : 0] S10_AXI_BRESP;
    wire S10_AXI_BVALID;
    wire S10_AXI_BREADY;
    wire [0 : 0] S10_AXI_ARID;
    wire [31 : 0] S10_AXI_ARADDR;
    wire [7 : 0] S10_AXI_ARLEN;
    wire [2 : 0] S10_AXI_ARSIZE;
    wire [1 : 0] S10_AXI_ARBURST;
    wire S10_AXI_ARLOCK;
    wire [3 : 0] S10_AXI_ARCACHE;
    wire [2 : 0] S10_AXI_ARPROT;
    wire [3 : 0] S10_AXI_ARQOS;
    wire S10_AXI_ARVALID;
    wire S10_AXI_ARREADY;
    wire [0 : 0] S10_AXI_RID;
    wire [511 : 0] S10_AXI_RDATA;
    wire [1 : 0] S10_AXI_RRESP;
    wire S10_AXI_RLAST;
    wire S10_AXI_RVALID;
    wire S10_AXI_RREADY;
    wire S11_AXI_ARESET_OUT_N;
    wire S11_AXI_ACLK;
    wire [0 : 0] S11_AXI_AWID;
    wire [31 : 0] S11_AXI_AWADDR;
    wire [7 : 0] S11_AXI_AWLEN;
    wire [2 : 0] S11_AXI_AWSIZE;
    wire [1 : 0] S11_AXI_AWBURST;
    wire S11_AXI_AWLOCK;
    wire [3 : 0] S11_AXI_AWCACHE;
    wire [2 : 0] S11_AXI_AWPROT;
    wire [3 : 0] S11_AXI_AWQOS;
    wire S11_AXI_AWVALID;
    wire S11_AXI_AWREADY;
    wire [511 : 0] S11_AXI_WDATA;
    wire [63 : 0] S11_AXI_WSTRB;
    wire S11_AXI_WLAST;
    wire S11_AXI_WVALID;
    wire S11_AXI_WREADY;
    wire [0 : 0] S11_AXI_BID;
    wire [1 : 0] S11_AXI_BRESP;
    wire S11_AXI_BVALID;
    wire S11_AXI_BREADY;
    wire [0 : 0] S11_AXI_ARID;
    wire [31 : 0] S11_AXI_ARADDR;
    wire [7 : 0] S11_AXI_ARLEN;
    wire [2 : 0] S11_AXI_ARSIZE;
    wire [1 : 0] S11_AXI_ARBURST;
    wire S11_AXI_ARLOCK;
    wire [3 : 0] S11_AXI_ARCACHE;
    wire [2 : 0] S11_AXI_ARPROT;
    wire [3 : 0] S11_AXI_ARQOS;
    wire S11_AXI_ARVALID;
    wire S11_AXI_ARREADY;
    wire [0 : 0] S11_AXI_RID;
    wire [511 : 0] S11_AXI_RDATA;
    wire [1 : 0] S11_AXI_RRESP;
    wire S11_AXI_RLAST;
    wire S11_AXI_RVALID;
    wire S11_AXI_RREADY;
    
    wire [3:0] c0_s_axi_arid_x;
    assign c0_s_axi_arid = c0_s_axi_arid_x[0];
    wire [3:0] S10_AXI_ARID_x, S10_AXI_AWID_x;
    assign S10_AXI_ARID = S10_AXI_ARID_x[0];
    assign S10_AXI_AWID = S10_AXI_AWID_x[0];
    
    wire [3:0] S11_AXI_ARID_x, S11_AXI_AWID_x;
    assign S11_AXI_ARID = S11_AXI_ARID_x[0];
    assign S11_AXI_AWID = S11_AXI_AWID_x[0];
    
    axi_interconnect_ip toeTX_axi_switch (
    .INTERCONNECT_ACLK(clk156_25),//input ;
    .INTERCONNECT_ARESETN(reset156_25_n),//input ;
    .S00_AXI_ARESET_OUT_N(),//output ;
    .S00_AXI_ACLK(clk156_25),//input ;
    .S00_AXI_AWID(S10_AXI_AWID),//input [0 : 0] ;
    .S00_AXI_AWADDR(S10_AXI_AWADDR[31:0]),//input [31 : 0] ;
    .S00_AXI_AWLEN(S10_AXI_AWLEN),//input [7 : 0] ;
    .S00_AXI_AWSIZE(S10_AXI_AWSIZE),//input [2 : 0] ;
    .S00_AXI_AWBURST(S10_AXI_AWBURST),//input [1 : 0] ;
    .S00_AXI_AWLOCK(1'b0),//input ;
    .S00_AXI_AWCACHE(4'b0),//input [3 : 0] ;
    .S00_AXI_AWPROT(3'b0),//input [2 : 0] ;
    .S00_AXI_AWQOS(4'b0),//input [3 : 0] ;
    .S00_AXI_AWVALID(S10_AXI_AWVALID),//input ;
    .S00_AXI_AWREADY(S10_AXI_AWREADY),//output ;
    .S00_AXI_WDATA(S10_AXI_WDATA),//input [511 : 0] ;
    .S00_AXI_WSTRB(S10_AXI_WSTRB),//input [63 : 0] ;
    .S00_AXI_WLAST(S10_AXI_WLAST),//input ;
    .S00_AXI_WVALID(S10_AXI_WVALID),//input ;
    .S00_AXI_WREADY(S10_AXI_WREADY),//output ;
    .S00_AXI_BID(S10_AXI_BID),//output [0 : 0] ;
    .S00_AXI_BRESP(S10_AXI_BRESP),//output [1 : 0] ;
    .S00_AXI_BVALID(S10_AXI_BVALID),//output ;
    .S00_AXI_BREADY(S10_AXI_BREADY),//input ;
    .S00_AXI_ARID(S10_AXI_ARID),//input [0 : 0] ;
    .S00_AXI_ARADDR(S10_AXI_ARADDR[31:0]),//input [31 : 0] ;
    .S00_AXI_ARLEN(S10_AXI_ARLEN),//input [7 : 0] ;
    .S00_AXI_ARSIZE(S10_AXI_ARSIZE),//input [2 : 0] ;
    .S00_AXI_ARBURST(S10_AXI_ARBURST),//input [1 : 0] ;
    .S00_AXI_ARLOCK(1'b0),//input ;
    .S00_AXI_ARCACHE(4'b0),//input [3 : 0] ;
    .S00_AXI_ARPROT(3'b0),//input [2 : 0] ;
    .S00_AXI_ARQOS(4'b0),//input [3 : 0] ;
    .S00_AXI_ARVALID(S10_AXI_ARVALID),//input ;
    .S00_AXI_ARREADY(S10_AXI_ARREADY),//output ;
    .S00_AXI_RID(S10_AXI_RID),//output [0 : 0] ;
    .S00_AXI_RDATA(S10_AXI_RDATA),//output [511 : 0] ;
    .S00_AXI_RRESP(S10_AXI_RRESP),//output [1 : 0] ;
    .S00_AXI_RLAST(S10_AXI_RLAST),//output ;
    .S00_AXI_RVALID(S10_AXI_RVALID),//output ;
    .S00_AXI_RREADY(S10_AXI_RREADY),//input ;
    .S01_AXI_ARESET_OUT_N(S11_AXI_ARESET_OUT_N),//output ;
    .S01_AXI_ACLK(clk156_25),//input ;
    .S01_AXI_AWID(S11_AXI_AWID),//input [0 : 0] ;
    .S01_AXI_AWADDR(S11_AXI_AWADDR[31:0]),//input [31 : 0] ;
    .S01_AXI_AWLEN(S11_AXI_AWLEN),//input [7 : 0] ;
    .S01_AXI_AWSIZE(S11_AXI_AWSIZE),//input [2 : 0] ;
    .S01_AXI_AWBURST(S11_AXI_AWBURST),//input [1 : 0] ;
    .S01_AXI_AWLOCK(1'b0),//input ;
    .S01_AXI_AWCACHE(4'b0),//input [3 : 0] ;
    .S01_AXI_AWPROT(3'b0),//input [2 : 0] ;
    .S01_AXI_AWQOS(4'b0),//input [3 : 0] ;
    .S01_AXI_AWVALID(S11_AXI_AWVALID),//input ;
    .S01_AXI_AWREADY(S11_AXI_AWREADY),//output ;
    .S01_AXI_WDATA(S11_AXI_WDATA),//input [511 : 0] ;
    .S01_AXI_WSTRB(S11_AXI_WSTRB),//input [63 : 0] ;
    .S01_AXI_WLAST(S11_AXI_WLAST),//input ;
    .S01_AXI_WVALID(S11_AXI_WVALID),//input ;
    .S01_AXI_WREADY(S11_AXI_WREADY),//output ;
    .S01_AXI_BID(S11_AXI_BID),//output [0 : 0] ;
    .S01_AXI_BRESP(S11_AXI_BRESP),//output [1 : 0] ;
    .S01_AXI_BVALID(S11_AXI_BVALID),//output ;
    .S01_AXI_BREADY(S11_AXI_BREADY),//input ;
    .S01_AXI_ARID(S11_AXI_ARID),//input [0 : 0] ;
    .S01_AXI_ARADDR(S11_AXI_ARADDR[31:0]),//input [31 : 0] ;
    .S01_AXI_ARLEN(S11_AXI_ARLEN),//input [7 : 0] ;
    .S01_AXI_ARSIZE(S11_AXI_ARSIZE),//input [2 : 0] ;
    .S01_AXI_ARBURST(S11_AXI_ARBURST),//input [1 : 0] ;
    .S01_AXI_ARLOCK(1'b0),//input ;
    .S01_AXI_ARCACHE(4'b0),//input [3 : 0] ;
    .S01_AXI_ARPROT(3'b0),//input [2 : 0] ;
    .S01_AXI_ARQOS(4'b0),//input [3 : 0] ;
    .S01_AXI_ARVALID(S11_AXI_ARVALID),//input ;
    .S01_AXI_ARREADY(S11_AXI_ARREADY),//output ;
    .S01_AXI_RID(S11_AXI_RID),//output [0 : 0] ;
    .S01_AXI_RDATA(S11_AXI_RDATA),//output [511 : 0] ;
    .S01_AXI_RRESP(S11_AXI_RRESP),//output [1 : 0] ;
    .S01_AXI_RLAST(S11_AXI_RLAST),//output ;
    .S01_AXI_RVALID(S11_AXI_RVALID),//output ;
    .S01_AXI_RREADY(S11_AXI_RREADY),//input ;
    .M00_AXI_ARESET_OUT_N(),//output ;
    .M00_AXI_ACLK(c0_ui_clk),//(clk156_25),//input ;
    .M00_AXI_AWID(c0_s_axi_awid),//output [3 : 0] ;
    .M00_AXI_AWADDR(c0_s_axi_awaddr[31:0]),//output [31 : 0] ;
    .M00_AXI_AWLEN(c0_s_axi_awlen),//output [7 : 0] ;
    .M00_AXI_AWSIZE(c0_s_axi_awsize),//output [2 : 0] ;
    .M00_AXI_AWBURST(c0_s_axi_awburst),//output [1 : 0] ;
    .M00_AXI_AWLOCK(),//output ;
    .M00_AXI_AWCACHE(),//output [3 : 0] ;
    .M00_AXI_AWPROT(),//output [2 : 0] ;
    .M00_AXI_AWQOS(),//output [3 : 0] ;
    .M00_AXI_AWVALID(c0_s_axi_awvalid),//output ;
    .M00_AXI_AWREADY(c0_s_axi_awready),//input ;
    .M00_AXI_WDATA(c0_s_axi_wdata),//output [511 : 0] ;
    .M00_AXI_WSTRB(c0_s_axi_wstrb),//output [63 : 0] ;
    .M00_AXI_WLAST(c0_s_axi_wlast),//output ;
    .M00_AXI_WVALID(c0_s_axi_wvalid),//output ;
    .M00_AXI_WREADY(c0_s_axi_wready),//input ;
    .M00_AXI_BID({3'b0,c0_s_axi_bid}),//input [3 : 0] ;
    .M00_AXI_BRESP(c0_s_axi_bresp),//input [1 : 0] ;
    .M00_AXI_BVALID(c0_s_axi_bvalid),//input ;
    .M00_AXI_BREADY(c0_s_axi_bready),//output ;
    .M00_AXI_ARID(c0_s_axi_arid_x),//output [3 : 0] ;
    .M00_AXI_ARADDR(c0_s_axi_araddr[31:0]),//output [31 : 0] ;
    .M00_AXI_ARLEN(c0_s_axi_arlen),//output [7 : 0] ;
    .M00_AXI_ARSIZE(c0_s_axi_arsize),//output [2 : 0] ;
    .M00_AXI_ARBURST(c0_s_axi_arburst),//output [1 : 0] ;
    .M00_AXI_ARLOCK(),//output ;
    .M00_AXI_ARCACHE(),//output [3 : 0] ;
    .M00_AXI_ARPROT(),//output [2 : 0] ;
    .M00_AXI_ARQOS(),//output [3 : 0] ;
    .M00_AXI_ARVALID(c0_s_axi_arvalid),//output ;
    .M00_AXI_ARREADY(c0_s_axi_arready),//input ;
    .M00_AXI_RID({3'b0, c0_s_axi_rid}),//input [3 : 0] ;
    .M00_AXI_RDATA(c0_s_axi_rdata),//input [511 : 0] ;
    .M00_AXI_RRESP(c0_s_axi_rresp),//input [1 : 0] ;
    .M00_AXI_RLAST(c0_s_axi_rlast),//input ;
    .M00_AXI_RVALID(c0_s_axi_rvalid),//input ;
    .M00_AXI_RREADY(c0_s_axi_rready)//output ;
    );

    //convert 512-bit AXI full to 64-bit AXI stream
    axi_datamover_0 toeTX_data_mover0 (
      .m_axi_mm2s_aclk(clk156_25), // input m_axi_mm2s_aclk
      .m_axi_mm2s_aresetn(reset156_25_n), // input m_axi_mm2s_aresetn
      .mm2s_err(), // output mm2s_err
      .m_axis_mm2s_cmdsts_aclk(clk156_25), // input m_axis_mm2s_cmdsts_aclk
      .m_axis_mm2s_cmdsts_aresetn(reset156_25_n), // input m_axis_mm2s_cmdsts_aresetn
      // mm2s => read
      .s_axis_mm2s_cmd_tvalid(toeTX_s_axis_read_cmd_tvalid), // input s_axis_mm2s_cmd_tvalid
      .s_axis_mm2s_cmd_tready(toeTX_s_axis_read_cmd_tready), // output s_axis_mm2s_cmd_tready
      .s_axis_mm2s_cmd_tdata(toeTX_s_axis_read_cmd_tdata), // input [71 : 0] s_axis_mm2s_cmd_tdata
      .m_axis_mm2s_sts_tvalid(toeTX_m_axis_read_sts_tvalid), // output m_axis_mm2s_sts_tvalid
      .m_axis_mm2s_sts_tready(toeTX_m_axis_read_sts_tready), // input m_axis_mm2s_sts_tready
      .m_axis_mm2s_sts_tdata(toeTX_m_axis_read_sts_tdata), // output [7 : 0] m_axis_mm2s_sts_tdata
      .m_axis_mm2s_sts_tkeep(), // output [0 : 0] m_axis_mm2s_sts_tkeep
      .m_axis_mm2s_sts_tlast(), // output m_axis_mm2s_sts_tlast
      .m_axi_mm2s_arid(S10_AXI_ARID_x), // output [3 : 0] m_axi_mm2s_arid
      .m_axi_mm2s_araddr(S10_AXI_ARADDR), // output [31 : 0] m_axi_mm2s_araddr
      .m_axi_mm2s_arlen(S10_AXI_ARLEN), // output [7 : 0] m_axi_mm2s_arlen
      .m_axi_mm2s_arsize(S10_AXI_ARSIZE), // output [2 : 0] m_axi_mm2s_arsize
      .m_axi_mm2s_arburst(S10_AXI_ARBURST), // output [1 : 0] m_axi_mm2s_arburst
      .m_axi_mm2s_arprot(), // output [2 : 0] m_axi_mm2s_arprot
      .m_axi_mm2s_arcache(), // output [3 : 0] m_axi_mm2s_arcache
      .m_axi_mm2s_aruser(), // output [3 : 0] m_axi_mm2s_aruser
      .m_axi_mm2s_arvalid(S10_AXI_ARVALID), // output m_axi_mm2s_arvalid
      .m_axi_mm2s_arready(S10_AXI_ARREADY), // input m_axi_mm2s_arready
      .m_axi_mm2s_rdata(S10_AXI_RDATA), // input [511 : 0] m_axi_mm2s_rdata
      .m_axi_mm2s_rresp(S10_AXI_RRESP), // input [1 : 0] m_axi_mm2s_rresp
      .m_axi_mm2s_rlast(S10_AXI_RLAST), // input m_axi_mm2s_rlast
      .m_axi_mm2s_rvalid(S10_AXI_RVALID), // input m_axi_mm2s_rvalid
      .m_axi_mm2s_rready(S10_AXI_RREADY), // output m_axi_mm2s_rready
      // read output to app 
      .m_axis_mm2s_tdata(toeTX_m_axis_read_tdata), // output [63 : 0] m_axis_mm2s_tdata
      .m_axis_mm2s_tkeep(toeTX_m_axis_read_tkeep), // output [7 : 0] m_axis_mm2s_tkeep
      .m_axis_mm2s_tlast(toeTX_m_axis_read_tlast), // output m_axis_mm2s_tlast
      .m_axis_mm2s_tvalid(toeTX_m_axis_read_tvalid), // output m_axis_mm2s_tvalid
      .m_axis_mm2s_tready(toeTX_m_axis_read_tready), // input m_axis_mm2s_tready
      .m_axi_s2mm_aclk(clk156_25), // input m_axi_s2mm_aclk
      .m_axi_s2mm_aresetn(reset156_25_n), // input m_axi_s2mm_aresetn
      .s2mm_err(), // output s2mm_err
      .m_axis_s2mm_cmdsts_awclk(clk156_25), // input m_axis_s2mm_cmdsts_awclk
      .m_axis_s2mm_cmdsts_aresetn(reset156_25_n), // input m_axis_s2mm_cmdsts_aresetn
      // s2mm => write
      .s_axis_s2mm_cmd_tvalid(toeTX_s_axis_write_cmd_tvalid), // input s_axis_s2mm_cmd_tvalid
      .s_axis_s2mm_cmd_tready(toeTX_s_axis_write_cmd_tready), // output s_axis_s2mm_cmd_tready
      .s_axis_s2mm_cmd_tdata(toeTX_s_axis_write_cmd_tdata), // input [71 : 0] s_axis_s2mm_cmd_tdata
      .m_axis_s2mm_sts_tvalid(toeTX_m_axis_write_sts_tvalid), // output m_axis_s2mm_sts_tvalid
      .m_axis_s2mm_sts_tready(toeTX_m_axis_write_sts_tready), // input m_axis_s2mm_sts_tready
      .m_axis_s2mm_sts_tdata(toeTX_m_axis_write_sts_tdata), // OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
      .m_axis_s2mm_sts_tkeep(), // output [0 : 0] m_axis_s2mm_sts_tkeep
      .m_axis_s2mm_sts_tlast(), // output m_axis_s2mm_sts_tlast
      .m_axi_s2mm_awid(S10_AXI_AWID_x), // output [3 : 0] m_axi_s2mm_awid
      .m_axi_s2mm_awaddr(S10_AXI_AWADDR), // output [31 : 0] m_axi_s2mm_awaddr
      .m_axi_s2mm_awlen(S10_AXI_AWLEN), // output [7 : 0] m_axi_s2mm_awlen
      .m_axi_s2mm_awsize(S10_AXI_AWSIZE), // output [2 : 0] m_axi_s2mm_awsize
      .m_axi_s2mm_awburst(S10_AXI_AWBURST), // output [1 : 0] m_axi_s2mm_awburst
      .m_axi_s2mm_awprot(), // output [2 : 0] m_axi_s2mm_awprot
      .m_axi_s2mm_awcache(), // output [3 : 0] m_axi_s2mm_awcache
      .m_axi_s2mm_awuser(), // output [3 : 0] m_axi_s2mm_awuser
      .m_axi_s2mm_awvalid(S10_AXI_AWVALID), // output m_axi_s2mm_awvalid
      .m_axi_s2mm_awready(S10_AXI_AWREADY), // input m_axi_s2mm_awready
      .m_axi_s2mm_wdata(S10_AXI_WDATA), // output [511 : 0] m_axi_s2mm_wdata
      .m_axi_s2mm_wstrb(S10_AXI_WSTRB), // output [63 : 0] m_axi_s2mm_wstrb
      .m_axi_s2mm_wlast(S10_AXI_WLAST), // output m_axi_s2mm_wlast
      .m_axi_s2mm_wvalid(S10_AXI_WVALID), // output m_axi_s2mm_wvalid
      .m_axi_s2mm_wready(S10_AXI_WREADY), // input m_axi_s2mm_wready
      .m_axi_s2mm_bresp(S10_AXI_BRESP), // input [1 : 0] m_axi_s2mm_bresp
      .m_axi_s2mm_bvalid(S10_AXI_BVALID), // input m_axi_s2mm_bvalid
      .m_axi_s2mm_bready(S10_AXI_BREADY), // output m_axi_s2mm_bready
      // write input from tcp
      .s_axis_s2mm_tdata(toeTX_s_axis_write_tdata), // input [63 : 0] s_axis_s2mm_tdata
      .s_axis_s2mm_tkeep(toeTX_s_axis_write_tkeep), // input [7 : 0] s_axis_s2mm_tkeep
      .s_axis_s2mm_tlast(toeTX_s_axis_write_tlast), // input s_axis_s2mm_tlast
      .s_axis_s2mm_tvalid(toeTX_s_axis_write_tvalid), // input s_axis_s2mm_tvalid
      .s_axis_s2mm_tready(toeTX_s_axis_write_tready) // output s_axis_s2mm_tready
);

//convert 512-bit AXI full to 64-bit AXI stream
axi_datamover_0 toeRX_data_mover0 (
  .m_axi_mm2s_aclk(clk156_25), // input m_axi_mm2s_aclk
  .m_axi_mm2s_aresetn(reset156_25_n), // input m_axi_mm2s_aresetn
  .mm2s_err(), // output mm2s_err
  .m_axis_mm2s_cmdsts_aclk(clk156_25), // input m_axis_mm2s_cmdsts_aclk
  .m_axis_mm2s_cmdsts_aresetn(reset156_25_n), // input m_axis_mm2s_cmdsts_aresetn
  // mm2s => read
  .s_axis_mm2s_cmd_tvalid(toeRX_s_axis_read_cmd_tvalid), // input s_axis_mm2s_cmd_tvalid
  .s_axis_mm2s_cmd_tready(toeRX_s_axis_read_cmd_tready), // output s_axis_mm2s_cmd_tready
  .s_axis_mm2s_cmd_tdata(toeRX_s_axis_read_cmd_tdata), // input [71 : 0] s_axis_mm2s_cmd_tdata
  .m_axis_mm2s_sts_tvalid(toeRX_m_axis_read_sts_tvalid), // output m_axis_mm2s_sts_tvalid
  .m_axis_mm2s_sts_tready(toeRX_m_axis_read_sts_tready), // input m_axis_mm2s_sts_tready
  .m_axis_mm2s_sts_tdata(toeRX_m_axis_read_sts_tdata), // output [7 : 0] m_axis_mm2s_sts_tdata
  .m_axis_mm2s_sts_tkeep(), // output [0 : 0] m_axis_mm2s_sts_tkeep
  .m_axis_mm2s_sts_tlast(), // output m_axis_mm2s_sts_tlast
  .m_axi_mm2s_arid(S11_AXI_ARID_x), // output [3 : 0] m_axi_mm2s_arid
  .m_axi_mm2s_araddr(S11_AXI_ARADDR), // output [31 : 0] m_axi_mm2s_araddr
  .m_axi_mm2s_arlen(S11_AXI_ARLEN), // output [7 : 0] m_axi_mm2s_arlen
  .m_axi_mm2s_arsize(S11_AXI_ARSIZE), // output [2 : 0] m_axi_mm2s_arsize
  .m_axi_mm2s_arburst(S11_AXI_ARBURST), // output [1 : 0] m_axi_mm2s_arburst
  .m_axi_mm2s_arprot(), // output [2 : 0] m_axi_mm2s_arprot
  .m_axi_mm2s_arcache(), // output [3 : 0] m_axi_mm2s_arcache
  .m_axi_mm2s_aruser(), // output [3 : 0] m_axi_mm2s_aruser
  .m_axi_mm2s_arvalid(S11_AXI_ARVALID), // output m_axi_mm2s_arvalid
  .m_axi_mm2s_arready(S11_AXI_ARREADY), // input m_axi_mm2s_arready
  .m_axi_mm2s_rdata(S11_AXI_RDATA), // input [511 : 0] m_axi_mm2s_rdata
  .m_axi_mm2s_rresp(S11_AXI_RRESP), // input [1 : 0] m_axi_mm2s_rresp
  .m_axi_mm2s_rlast(S11_AXI_RLAST), // input m_axi_mm2s_rlast
  .m_axi_mm2s_rvalid(S11_AXI_RVALID), // input m_axi_mm2s_rvalid
  .m_axi_mm2s_rready(S11_AXI_RREADY), // output m_axi_mm2s_rready
  // read output to app 
  .m_axis_mm2s_tdata(toeRX_m_axis_read_tdata), // output [63 : 0] m_axis_mm2s_tdata
  .m_axis_mm2s_tkeep(toeRX_m_axis_read_tkeep), // output [7 : 0] m_axis_mm2s_tkeep
  .m_axis_mm2s_tlast(toeRX_m_axis_read_tlast), // output m_axis_mm2s_tlast
  .m_axis_mm2s_tvalid(toeRX_m_axis_read_tvalid), // output m_axis_mm2s_tvalid
  .m_axis_mm2s_tready(toeRX_m_axis_read_tready), // input m_axis_mm2s_tready
  .m_axi_s2mm_aclk(clk156_25), // input m_axi_s2mm_aclk
  .m_axi_s2mm_aresetn(reset156_25_n), // input m_axi_s2mm_aresetn
  .s2mm_err(), // output s2mm_err
  .m_axis_s2mm_cmdsts_awclk(clk156_25), // input m_axis_s2mm_cmdsts_awclk
  .m_axis_s2mm_cmdsts_aresetn(reset156_25_n), // input m_axis_s2mm_cmdsts_aresetn
  // s2mm => write
  .s_axis_s2mm_cmd_tvalid(toeRX_s_axis_write_cmd_tvalid), // input s_axis_s2mm_cmd_tvalid
  .s_axis_s2mm_cmd_tready(toeRX_s_axis_write_cmd_tready), // output s_axis_s2mm_cmd_tready
  .s_axis_s2mm_cmd_tdata(toeRX_s_axis_write_cmd_tdata), // input [71 : 0] s_axis_s2mm_cmd_tdata
  .m_axis_s2mm_sts_tvalid(toeRX_m_axis_write_sts_tvalid), // output m_axis_s2mm_sts_tvalid
  .m_axis_s2mm_sts_tready(toeRX_m_axis_write_sts_tready), // input m_axis_s2mm_sts_tready
  .m_axis_s2mm_sts_tdata(toeRX_m_axis_write_sts_tdata), // OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
  .m_axis_s2mm_sts_tkeep(), // output [0 : 0] m_axis_s2mm_sts_tkeep
  .m_axis_s2mm_sts_tlast(), // output m_axis_s2mm_sts_tlast
  .m_axi_s2mm_awid(S11_AXI_AWID_x), // output [3 : 0] m_axi_s2mm_awid
  .m_axi_s2mm_awaddr(S11_AXI_AWADDR), // output [31 : 0] m_axi_s2mm_awaddr
  .m_axi_s2mm_awlen(S11_AXI_AWLEN), // output [7 : 0] m_axi_s2mm_awlen
  .m_axi_s2mm_awsize(S11_AXI_AWSIZE), // output [2 : 0] m_axi_s2mm_awsize
  .m_axi_s2mm_awburst(S11_AXI_AWBURST), // output [1 : 0] m_axi_s2mm_awburst
  .m_axi_s2mm_awprot(), // output [2 : 0] m_axi_s2mm_awprot
  .m_axi_s2mm_awcache(), // output [3 : 0] m_axi_s2mm_awcache
  .m_axi_s2mm_awuser(), // output [3 : 0] m_axi_s2mm_awuser
  .m_axi_s2mm_awvalid(S11_AXI_AWVALID), // output m_axi_s2mm_awvalid
  .m_axi_s2mm_awready(S11_AXI_AWREADY), // input m_axi_s2mm_awready
  .m_axi_s2mm_wdata(S11_AXI_WDATA), // output [511 : 0] m_axi_s2mm_wdata
  .m_axi_s2mm_wstrb(S11_AXI_WSTRB), // output [63 : 0] m_axi_s2mm_wstrb
  .m_axi_s2mm_wlast(S11_AXI_WLAST), // output m_axi_s2mm_wlast
  .m_axi_s2mm_wvalid(S11_AXI_WVALID), // output m_axi_s2mm_wvalid
  .m_axi_s2mm_wready(S11_AXI_WREADY), // input m_axi_s2mm_wready
  .m_axi_s2mm_bresp(S11_AXI_BRESP), // input [1 : 0] m_axi_s2mm_bresp
  .m_axi_s2mm_bvalid(S11_AXI_BVALID), // input m_axi_s2mm_bvalid
  .m_axi_s2mm_bready(S11_AXI_BREADY), // output m_axi_s2mm_bready
  // write input from tcp
  .s_axis_s2mm_tdata(toeRX_s_axis_write_tdata), // input [63 : 0] s_axis_s2mm_tdata
  .s_axis_s2mm_tkeep(toeRX_s_axis_write_tkeep), // input [7 : 0] s_axis_s2mm_tkeep
  .s_axis_s2mm_tlast(toeRX_s_axis_write_tlast), // input s_axis_s2mm_tlast
  .s_axis_s2mm_tvalid(toeRX_s_axis_write_tvalid), // input s_axis_s2mm_tvalid
  .s_axis_s2mm_tready(toeRX_s_axis_write_tready) // output s_axis_s2mm_tready
);

wire [0 : 0] S00_AXI_AWID;
wire [31 : 0] S00_AXI_AWADDR;
wire [7 : 0] S00_AXI_AWLEN;
wire [2 : 0] S00_AXI_AWSIZE;
wire [1 : 0] S00_AXI_AWBURST;
wire S00_AXI_AWLOCK;
wire [3 : 0] S00_AXI_AWCACHE;
wire [2 : 0] S00_AXI_AWPROT;
wire [3 : 0] S00_AXI_AWQOS;
wire S00_AXI_AWVALID;
wire S00_AXI_AWREADY;
wire [511 : 0] S00_AXI_WDATA;
wire [63 : 0] S00_AXI_WSTRB;
wire S00_AXI_WLAST;
wire S00_AXI_WVALID;
wire S00_AXI_WREADY;
wire [0 : 0] S00_AXI_BID;
wire [1 : 0] S00_AXI_BRESP;
wire S00_AXI_BVALID;
wire S00_AXI_BREADY;
wire [0 : 0] S00_AXI_ARID;
wire [31 : 0] S00_AXI_ARADDR;
wire [7 : 0] S00_AXI_ARLEN;
wire [2 : 0] S00_AXI_ARSIZE;
wire [1 : 0] S00_AXI_ARBURST;
wire S00_AXI_ARLOCK;
wire [3 : 0] S00_AXI_ARCACHE;
wire [2 : 0] S00_AXI_ARPROT;
wire [3 : 0] S00_AXI_ARQOS;
wire S00_AXI_ARVALID;
wire S00_AXI_ARREADY;
wire [0 : 0] S00_AXI_RID;
wire [511 : 0] S00_AXI_RDATA;
wire [1 : 0] S00_AXI_RRESP;
wire S00_AXI_RLAST;
wire S00_AXI_RVALID;
wire S00_AXI_RREADY;
wire S01_AXI_ARESET_OUT_N;
wire S01_AXI_ACLK;
wire [0 : 0] S01_AXI_AWID;
wire [31 : 0] S01_AXI_AWADDR;
wire [7 : 0] S01_AXI_AWLEN;
wire [2 : 0] S01_AXI_AWSIZE;
wire [1 : 0] S01_AXI_AWBURST;
wire S01_AXI_AWLOCK;
wire [3 : 0] S01_AXI_AWCACHE;
wire [2 : 0] S01_AXI_AWPROT;
wire [3 : 0] S01_AXI_AWQOS;
wire S01_AXI_AWVALID;
wire S01_AXI_AWREADY;
wire [511 : 0] S01_AXI_WDATA;
wire [63 : 0] S01_AXI_WSTRB;
wire S01_AXI_WLAST;
wire S01_AXI_WVALID;
wire S01_AXI_WREADY;
wire [0 : 0] S01_AXI_BID;
wire [1 : 0] S01_AXI_BRESP;
wire S01_AXI_BVALID;
wire S01_AXI_BREADY;
wire [0 : 0] S01_AXI_ARID;
wire [31 : 0] S01_AXI_ARADDR;
wire [7 : 0] S01_AXI_ARLEN;
wire [2 : 0] S01_AXI_ARSIZE;
wire [1 : 0] S01_AXI_ARBURST;
wire S01_AXI_ARLOCK;
wire [3 : 0] S01_AXI_ARCACHE;
wire [2 : 0] S01_AXI_ARPROT;
wire [3 : 0] S01_AXI_ARQOS;
wire S01_AXI_ARVALID;
wire S01_AXI_ARREADY;
wire [0 : 0] S01_AXI_RID;
wire [511 : 0] S01_AXI_RDATA;
wire [1 : 0] S01_AXI_RRESP;
wire S01_AXI_RLAST;
wire S01_AXI_RVALID;
wire S01_AXI_RREADY;

//wire [3:0] c1_m_axi_arid_x;
//assign c1_m_axi_arid = c1_m_axi_arid_x[0];

wire [3:0] c1_s_axi_arid_x;
assign c1_s_axi_arid = c1_s_axi_arid_x[0];


axi_interconnect_ip ht_upd_axi_switch (
.INTERCONNECT_ACLK(clk156_25),//input ;
.INTERCONNECT_ARESETN(reset156_25_n),//input ;
.S00_AXI_ARESET_OUT_N(),//output ;
.S00_AXI_ACLK(clk156_25),//input ;
.S00_AXI_AWID(S00_AXI_AWID),//input [0 : 0] ;
.S00_AXI_AWADDR(S00_AXI_AWADDR),//input [31 : 0] ;
.S00_AXI_AWLEN(S00_AXI_AWLEN),//input [7 : 0] ;
.S00_AXI_AWSIZE(S00_AXI_AWSIZE),//input [2 : 0] ;
.S00_AXI_AWBURST(S00_AXI_AWBURST),//input [1 : 0] ;
.S00_AXI_AWLOCK(1'b0),//input ;
.S00_AXI_AWCACHE(4'b0),//input [3 : 0] ;
.S00_AXI_AWPROT(3'b0),//input [2 : 0] ;
.S00_AXI_AWQOS(4'b0),//input [3 : 0] ;
.S00_AXI_AWVALID(S00_AXI_AWVALID),//input ;
.S00_AXI_AWREADY(S00_AXI_AWREADY),//output ;
.S00_AXI_WDATA(S00_AXI_WDATA),//input [511 : 0] ;
.S00_AXI_WSTRB(S00_AXI_WSTRB),//input [63 : 0] ;
.S00_AXI_WLAST(S00_AXI_WLAST),//input ;
.S00_AXI_WVALID(S00_AXI_WVALID),//input ;
.S00_AXI_WREADY(S00_AXI_WREADY),//output ;
.S00_AXI_BID(S00_AXI_BID),//output [0 : 0] ;
.S00_AXI_BRESP(S00_AXI_BRESP),//output [1 : 0] ;
.S00_AXI_BVALID(S00_AXI_BVALID),//output ;
.S00_AXI_BREADY(S00_AXI_BREADY),//input ;
.S00_AXI_ARID(S00_AXI_ARID),//input [0 : 0] ;
.S00_AXI_ARADDR(S00_AXI_ARADDR),//input [31 : 0] ;
.S00_AXI_ARLEN(S00_AXI_ARLEN),//input [7 : 0] ;
.S00_AXI_ARSIZE(S00_AXI_ARSIZE),//input [2 : 0] ;
.S00_AXI_ARBURST(S00_AXI_ARBURST),//input [1 : 0] ;
.S00_AXI_ARLOCK(1'b0),//input ;
.S00_AXI_ARCACHE(4'b0),//input [3 : 0] ;
.S00_AXI_ARPROT(3'b0),//input [2 : 0] ;
.S00_AXI_ARQOS(4'b0),//input [3 : 0] ;
.S00_AXI_ARVALID(S00_AXI_ARVALID),//input ;
.S00_AXI_ARREADY(S00_AXI_ARREADY),//output ;
.S00_AXI_RID(S00_AXI_RID),//output [0 : 0] ;
.S00_AXI_RDATA(S00_AXI_RDATA),//output [511 : 0] ;
.S00_AXI_RRESP(S00_AXI_RRESP),//output [1 : 0] ;
.S00_AXI_RLAST(S00_AXI_RLAST),//output ;
.S00_AXI_RVALID(S00_AXI_RVALID),//output ;
.S00_AXI_RREADY(S00_AXI_RREADY),//input ;
.S01_AXI_ARESET_OUT_N(S01_AXI_ARESET_OUT_N),//output ;
.S01_AXI_ACLK(clk156_25),//input ;
.S01_AXI_AWID(S01_AXI_AWID),//input [0 : 0] ;
.S01_AXI_AWADDR(S01_AXI_AWADDR),//input [31 : 0] ;
.S01_AXI_AWLEN(S01_AXI_AWLEN),//input [7 : 0] ;
.S01_AXI_AWSIZE(S01_AXI_AWSIZE),//input [2 : 0] ;
.S01_AXI_AWBURST(S01_AXI_AWBURST),//input [1 : 0] ;
.S01_AXI_AWLOCK(1'b0),//input ;
.S01_AXI_AWCACHE(4'b0),//input [3 : 0] ;
.S01_AXI_AWPROT(3'b0),//input [2 : 0] ;
.S01_AXI_AWQOS(4'b0),//input [3 : 0] ;
.S01_AXI_AWVALID(S01_AXI_AWVALID),//input ;
.S01_AXI_AWREADY(S01_AXI_AWREADY),//output ;
.S01_AXI_WDATA(S01_AXI_WDATA),//input [511 : 0] ;
.S01_AXI_WSTRB(S01_AXI_WSTRB),//input [63 : 0] ;
.S01_AXI_WLAST(S01_AXI_WLAST),//input ;
.S01_AXI_WVALID(S01_AXI_WVALID),//input ;
.S01_AXI_WREADY(S01_AXI_WREADY),//output ;
.S01_AXI_BID(S01_AXI_BID),//output [0 : 0] ;
.S01_AXI_BRESP(S01_AXI_BRESP),//output [1 : 0] ;
.S01_AXI_BVALID(S01_AXI_BVALID),//output ;
.S01_AXI_BREADY(S01_AXI_BREADY),//input ;
.S01_AXI_ARID(S01_AXI_ARID),//input [0 : 0] ;
.S01_AXI_ARADDR(S01_AXI_ARADDR),//input [31 : 0] ;
.S01_AXI_ARLEN(S01_AXI_ARLEN),//input [7 : 0] ;
.S01_AXI_ARSIZE(S01_AXI_ARSIZE),//input [2 : 0] ;
.S01_AXI_ARBURST(S01_AXI_ARBURST),//input [1 : 0] ;
.S01_AXI_ARLOCK(1'b0),//input ;
.S01_AXI_ARCACHE(4'b0),//input [3 : 0] ;
.S01_AXI_ARPROT(3'b0),//input [2 : 0] ;
.S01_AXI_ARQOS(4'b0),//input [3 : 0] ;
.S01_AXI_ARVALID(S01_AXI_ARVALID),//input ;
.S01_AXI_ARREADY(S01_AXI_ARREADY),//output ;
.S01_AXI_RID(S01_AXI_RID),//output [0 : 0] ;
.S01_AXI_RDATA(S01_AXI_RDATA),//output [511 : 0] ;
.S01_AXI_RRESP(S01_AXI_RRESP),//output [1 : 0] ;
.S01_AXI_RLAST(S01_AXI_RLAST),//output ;
.S01_AXI_RVALID(S01_AXI_RVALID),//output ;
.S01_AXI_RREADY(S01_AXI_RREADY),//input ;
.M00_AXI_ARESET_OUT_N(),//output ;
.M00_AXI_ACLK(c1_ui_clk),//(clk156_25),//input ;
.M00_AXI_AWID(c1_s_axi_awid),//output [3 : 0] ;
.M00_AXI_AWADDR(c1_s_axi_awaddr),//output [31 : 0] ;
.M00_AXI_AWLEN(c1_s_axi_awlen),//output [7 : 0] ;
.M00_AXI_AWSIZE(c1_s_axi_awsize),//output [2 : 0] ;
.M00_AXI_AWBURST(c1_s_axi_awburst),//output [1 : 0] ;
.M00_AXI_AWLOCK(),//output ;
.M00_AXI_AWCACHE(),//output [3 : 0] ;
.M00_AXI_AWPROT(),//output [2 : 0] ;
.M00_AXI_AWQOS(),//output [3 : 0] ;
.M00_AXI_AWVALID(c1_s_axi_awvalid),//output ;
.M00_AXI_AWREADY(c1_s_axi_awready),//input ;
.M00_AXI_WDATA(c1_s_axi_wdata),//output [511 : 0] ;
.M00_AXI_WSTRB(c1_s_axi_wstrb),//output [63 : 0] ;
.M00_AXI_WLAST(c1_s_axi_wlast),//output ;
.M00_AXI_WVALID(c1_s_axi_wvalid),//output ;
.M00_AXI_WREADY(c1_s_axi_wready),//input ;
.M00_AXI_BID({3'b0,c1_s_axi_bid}),//input [3 : 0] ;
.M00_AXI_BRESP(c1_s_axi_bresp),//input [1 : 0] ;
.M00_AXI_BVALID(c1_s_axi_bvalid),//input ;
.M00_AXI_BREADY(c1_s_axi_bready),//output ;
.M00_AXI_ARID(c1_s_axi_arid_x),//output [3 : 0] ;
.M00_AXI_ARADDR(c1_s_axi_araddr),//output [31 : 0] ;
.M00_AXI_ARLEN(c1_s_axi_arlen),//output [7 : 0] ;
.M00_AXI_ARSIZE(c1_s_axi_arsize),//output [2 : 0] ;
.M00_AXI_ARBURST(c1_s_axi_arburst),//output [1 : 0] ;
.M00_AXI_ARLOCK(),//output ;
.M00_AXI_ARCACHE(),//output [3 : 0] ;
.M00_AXI_ARPROT(),//output [2 : 0] ;
.M00_AXI_ARQOS(),//output [3 : 0] ;
.M00_AXI_ARVALID(c1_s_axi_arvalid),//output ;
.M00_AXI_ARREADY(c1_s_axi_arready),//input ;
.M00_AXI_RID({3'b0, c1_s_axi_rid}),//input [3 : 0] ;
.M00_AXI_RDATA(c1_s_axi_rdata),//input [511 : 0] ;
.M00_AXI_RRESP(c1_s_axi_rresp),//input [1 : 0] ;
.M00_AXI_RLAST(c1_s_axi_rlast),//input ;
.M00_AXI_RVALID(c1_s_axi_rvalid),//input ;
.M00_AXI_RREADY(c1_s_axi_rready)//output ;
);

wire [3:0] S00_AXI_ARID_x, S00_AXI_AWID_x;
assign S00_AXI_ARID = S00_AXI_ARID_x[0];
assign S00_AXI_AWID = S00_AXI_AWID_x[0];
//data movers between ht and upd axi streams and S00, S01 axi full interfaces
axi_datamover_1 ht_data_mover (
    .m_axi_mm2s_aclk(clk156_25),// : IN STD_LOGIC;
    .m_axi_mm2s_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .mm2s_err(), //: OUT STD_LOGIC;
    .m_axis_mm2s_cmdsts_aclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_mm2s_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tvalid(ht_s_axis_read_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tready(ht_s_axis_read_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_mm2s_cmd_tdata(ht_s_axis_read_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_mm2s_sts_tvalid(ht_m_axis_read_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_sts_tready(ht_m_axis_read_sts_tready), //: IN STD_LOGIC;
    .m_axis_mm2s_sts_tdata(ht_m_axis_read_sts_tdata), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axis_mm2s_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(0 DOWNTO 0);
    .m_axis_mm2s_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_mm2s_arid(S00_AXI_ARID_x), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_araddr(S00_AXI_ARADDR), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_mm2s_arlen(S00_AXI_ARLEN), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_mm2s_arsize(S00_AXI_ARSIZE), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arburst(S00_AXI_ARBURST), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_arprot(), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arcache(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_aruser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_arvalid(S00_AXI_ARVALID), //: OUT STD_LOGIC;
    .m_axi_mm2s_arready(S00_AXI_ARREADY), //: IN STD_LOGIC;
    .m_axi_mm2s_rdata(S00_AXI_RDATA), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_mm2s_rresp(S00_AXI_RRESP), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_rlast(S00_AXI_RLAST), //: IN STD_LOGIC;
    .m_axi_mm2s_rvalid(S00_AXI_RVALID), //: IN STD_LOGIC;
    .m_axi_mm2s_rready(S00_AXI_RREADY), //: OUT STD_LOGIC;
    .m_axis_mm2s_tdata(ht_m_axis_read_tdata), //: OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axis_mm2s_tkeep(ht_m_axis_read_tkeep), //: OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    .m_axis_mm2s_tlast(ht_m_axis_read_tlast), //: OUT STD_LOGIC;
    .m_axis_mm2s_tvalid(ht_m_axis_read_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_tready(ht_m_axis_read_tready), //: IN STD_LOGIC;
    .m_axi_s2mm_aclk(clk156_25), //: IN STD_LOGIC;
    .m_axi_s2mm_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s2mm_err(), //: OUT STD_LOGIC;
    .m_axis_s2mm_cmdsts_awclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_s2mm_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tvalid(ht_s_axis_write_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tready(ht_s_axis_write_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_s2mm_cmd_tdata(ht_s_axis_write_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_s2mm_sts_tvalid(ht_m_axis_write_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_s2mm_sts_tready(ht_m_axis_write_sts_tready), //: IN STD_LOGIC;
    .m_axis_s2mm_sts_tdata(ht_m_axis_write_sts_tdata), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axis_s2mm_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axis_s2mm_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_s2mm_awid(S00_AXI_AWID_x), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awaddr(S00_AXI_AWADDR), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_s2mm_awlen(S00_AXI_AWLEN), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_s2mm_awsize(S00_AXI_AWSIZE), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awburst(S00_AXI_AWBURST), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_awprot(), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awcache(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awuser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awvalid(S00_AXI_AWVALID), //: OUT STD_LOGIC;
    .m_axi_s2mm_awready(S00_AXI_AWREADY), //: IN STD_LOGIC;
    .m_axi_s2mm_wdata(S00_AXI_WDATA), //: OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_s2mm_wstrb(S00_AXI_WSTRB), //: OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    .m_axi_s2mm_wlast(S00_AXI_WLAST), //: OUT STD_LOGIC;
    .m_axi_s2mm_wvalid(S00_AXI_WVALID), //: OUT STD_LOGIC;
    .m_axi_s2mm_wready(S00_AXI_WREADY), //: IN STD_LOGIC;
    .m_axi_s2mm_bresp(S00_AXI_BRESP), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_bvalid(S00_AXI_BVALID), //: IN STD_LOGIC;
    .m_axi_s2mm_bready(S00_AXI_BREADY), //: OUT STD_LOGIC;
    .s_axis_s2mm_tdata(ht_s_axis_write_tdata), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .s_axis_s2mm_tkeep(ht_s_axis_write_tkeep), //: IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    .s_axis_s2mm_tlast(ht_s_axis_write_tlast), //: IN STD_LOGIC;
    .s_axis_s2mm_tvalid(ht_s_axis_write_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_tready(ht_s_axis_write_tready) //: OUT STD_LOGIC;
);

wire [3:0] S01_AXI_ARID_x, S01_AXI_AWID_x;
assign S01_AXI_ARID = S01_AXI_ARID_x[0];
assign S01_AXI_AWID = S01_AXI_AWID_x[0];

axi_datamover_1 upd_data_mover (
    .m_axi_mm2s_aclk(clk156_25),// : IN STD_LOGIC;
    .m_axi_mm2s_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .mm2s_err(), //: OUT STD_LOGIC;
    .m_axis_mm2s_cmdsts_aclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_mm2s_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tvalid(upd_s_axis_read_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_mm2s_cmd_tready(upd_s_axis_read_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_mm2s_cmd_tdata(upd_s_axis_read_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_mm2s_sts_tvalid(upd_m_axis_read_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_sts_tready(upd_m_axis_read_sts_tready), //: IN STD_LOGIC;
    .m_axis_mm2s_sts_tdata(upd_m_axis_read_sts_tdata), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axis_mm2s_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(0 DOWNTO 0);
    .m_axis_mm2s_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_mm2s_arid(S01_AXI_ARID_x), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_araddr(S01_AXI_ARADDR), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_mm2s_arlen(S01_AXI_ARLEN), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_mm2s_arsize(S01_AXI_ARSIZE), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arburst(S01_AXI_ARBURST), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_arprot(), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_mm2s_arcache(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_aruser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_mm2s_arvalid(S01_AXI_ARVALID), //: OUT STD_LOGIC;
    .m_axi_mm2s_arready(S01_AXI_ARREADY), //: IN STD_LOGIC;
    .m_axi_mm2s_rdata(S01_AXI_RDATA), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_mm2s_rresp(S01_AXI_RRESP), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_mm2s_rlast(S01_AXI_RLAST), //: IN STD_LOGIC;
    .m_axi_mm2s_rvalid(S01_AXI_RVALID), //: IN STD_LOGIC;
    .m_axi_mm2s_rready(S01_AXI_RREADY), //: OUT STD_LOGIC;
    .m_axis_mm2s_tdata(upd_m_axis_read_tdata), //: OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axis_mm2s_tkeep(upd_m_axis_read_tkeep), //: OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    .m_axis_mm2s_tlast(upd_m_axis_read_tlast), //: OUT STD_LOGIC;
    .m_axis_mm2s_tvalid(upd_m_axis_read_tvalid), //: OUT STD_LOGIC;
    .m_axis_mm2s_tready(upd_m_axis_read_tready), //: IN STD_LOGIC;
    .m_axi_s2mm_aclk(clk156_25), //: IN STD_LOGIC;
    .m_axi_s2mm_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s2mm_err(), //: OUT STD_LOGIC;
    .m_axis_s2mm_cmdsts_awclk(clk156_25), //: IN STD_LOGIC;
    .m_axis_s2mm_cmdsts_aresetn(reset156_25_n), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tvalid(upd_s_axis_write_cmd_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_cmd_tready(upd_s_axis_write_cmd_tready), //: OUT STD_LOGIC;
    .s_axis_s2mm_cmd_tdata(upd_s_axis_write_cmd_tdata), //: IN STD_LOGIC_VECTOR(71 DOWNTO 0);
    .m_axis_s2mm_sts_tvalid(upd_m_axis_write_sts_tvalid), //: OUT STD_LOGIC;
    .m_axis_s2mm_sts_tready(upd_m_axis_write_sts_tready), //: IN STD_LOGIC;
    .m_axis_s2mm_sts_tdata(upd_m_axis_write_sts_tdata), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axis_s2mm_sts_tkeep(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axis_s2mm_sts_tlast(), //: OUT STD_LOGIC;
    .m_axi_s2mm_awid(S01_AXI_AWID_x), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awaddr(S01_AXI_AWADDR), //: OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
    .m_axi_s2mm_awlen(S01_AXI_AWLEN), //: OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    .m_axi_s2mm_awsize(S01_AXI_AWSIZE), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awburst(S01_AXI_AWBURST), //: OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_awprot(), //: OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
    .m_axi_s2mm_awcache(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awuser(), //: OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
    .m_axi_s2mm_awvalid(S01_AXI_AWVALID), //: OUT STD_LOGIC;
    .m_axi_s2mm_awready(S01_AXI_AWREADY), //: IN STD_LOGIC;
    .m_axi_s2mm_wdata(S01_AXI_WDATA), //: OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
    .m_axi_s2mm_wstrb(S01_AXI_WSTRB), //: OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
    .m_axi_s2mm_wlast(S01_AXI_WLAST), //: OUT STD_LOGIC;
    .m_axi_s2mm_wvalid(S01_AXI_WVALID), //: OUT STD_LOGIC;
    .m_axi_s2mm_wready(S01_AXI_WREADY), //: IN STD_LOGIC;
    .m_axi_s2mm_bresp(S01_AXI_BRESP), //: IN STD_LOGIC_VECTOR(1 DOWNTO 0);
    .m_axi_s2mm_bvalid(S01_AXI_BVALID), //: IN STD_LOGIC;
    .m_axi_s2mm_bready(S01_AXI_BREADY), //: OUT STD_LOGIC;
    .s_axis_s2mm_tdata(upd_s_axis_write_tdata), //: IN STD_LOGIC_VECTOR(511 DOWNTO 0);
    .s_axis_s2mm_tkeep(upd_s_axis_write_tkeep), //: IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    .s_axis_s2mm_tlast(upd_s_axis_write_tlast), //: IN STD_LOGIC;
    .s_axis_s2mm_tvalid(upd_s_axis_write_tvalid), //: IN STD_LOGIC;
    .s_axis_s2mm_tready(upd_s_axis_write_tready) //: OUT STD_LOGIC;
);
endmodule
