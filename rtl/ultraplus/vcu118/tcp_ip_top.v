`timescale 1ns / 1ps
`default_nettype none

`define USE_DDR
`define ECHO_SERVER
//`define IPERF_CLIENT
`define UDP


module tcp_ip_top
(
    input  wire             gt_rxp_in,
    input  wire             gt_rxn_in,
    output wire             gt_txp_out,
    output wire             gt_txn_out,
   
    output wire rx_gt_locked_led,     // Indicates GT LOCK
    output wire rx_block_lock_led,    // Indicates Core Block Lock
    output wire [4:0] completion_status,

    input wire             sys_reset,
    input wire             gt_refclk_p,
    input wire             gt_refclk_n,
    input wire             dclk_p,
    input wire             dclk_n,

    //156.25MHz user clock
    //input wire             uclk_p,
    //input wire             uclk_n,
    
`ifdef USE_DDR
    //DDR0
    input wire                   c0_sys_clk_p,
    input wire                   c0_sys_clk_n,
    output wire                  c0_ddr4_act_n,
    output wire[16:0]            c0_ddr4_adr,
    output wire[1:0]            c0_ddr4_ba,
    output wire[0:0]            c0_ddr4_bg,
    output wire[0:0]            c0_ddr4_cke,
    output wire[0:0]            c0_ddr4_odt,
    output wire[0:0]            c0_ddr4_cs_n,
    output wire[0:0]                 c0_ddr4_ck_t,
    output wire[0:0]                c0_ddr4_ck_c,
    output wire                 c0_ddr4_reset_n,
    inout  wire[8:0]            c0_ddr4_dm_dbi_n,
    inout  wire[71:0]            c0_ddr4_dq,
    inout  wire[8:0]            c0_ddr4_dqs_t,
    inout  wire[8:0]            c0_ddr4_dqs_c,
    
    //DDR1
    input wire                   c1_sys_clk_p,
    input wire                   c1_sys_clk_n,
    output wire                  c1_ddr4_act_n,
    output wire[16:0]            c1_ddr4_adr,
    output wire[1:0]            c1_ddr4_ba,
    output wire[0:0]            c1_ddr4_bg,
    output wire[0:0]            c1_ddr4_cke,
    output wire[0:0]            c1_ddr4_odt,
    output wire[0:0]            c1_ddr4_cs_n,
    output wire[0:0]                 c1_ddr4_ck_t,
    output wire[0:0]                c1_ddr4_ck_c,
    output wire                 c1_ddr4_reset_n,
    inout  wire[8:0]            c1_ddr4_dm_dbi_n,
    inout  wire[71:0]            c1_ddr4_dq,
    inout  wire[8:0]            c1_ddr4_dqs_t,
    inout  wire[8:0]            c1_ddr4_dqs_c,
`endif
    
    //buttons
    input wire              button_center,
    input wire              button_north,
    input wire              button_west,
    input wire              button_south,
    input wire              button_east,
    
    input wire[3:0]         gpio_switch
);

wire aclk;
wire aresetn;
wire network_init;

wire [2:0] gt_loopback_in_0; 
wire[3:0] user_rx_reset;
wire[3:0] user_tx_reset;
wire gtpowergood_out;

//// For other GT loopback options please change the value appropriately
//// For example, for internal loopback gt_loopback_in[2:0] = 3'b010;
//// For more information and settings on loopback, refer GT Transceivers user guide

  wire dclk;
     IBUFDS #(
     .DQS_BIAS("FALSE")  // (FALSE, TRUE)
  )
  dclk_BUFG_inst (
     .O(dclk),   // 1-bit output: Buffer output
     .I(dclk_p),   // 1-bit input: Diff_p buffer input (connect directly to top-level port)
     .IB(dclk_n)  // 1-bit input: Diff_n buffer input (connect directly to top-level port)
  );

  /*wire uclk;
     IBUFDS #(
     .DQS_BIAS("FALSE")  // (FALSE, TRUE)
  )
  uclk_BUFG_inst (
     .O(uclk),   // 1-bit output: Buffer output
     .I(uclk_p),   // 1-bit input: Diff_p buffer input (connect directly to top-level port)
     .IB(uclk_n)  // 1-bit input: Diff_n buffer input (connect directly to top-level port)
  );*/

BUFG bufg_aresetn(
   .I(network_init),
   .O(aresetn)
);


  wire  [4:0 ]completion_status_0;
  assign completion_status = 0;

assign rx_block_lock_led = gtpowergood_out;
assign rx_gt_locked_led = network_init;


/*
 * Network Signals
 */
wire        axis_net_rx_data_tvalid;
wire        axis_net_rx_data_tready;
wire[63:0]  axis_net_rx_data_tdata;
wire[7:0]   axis_net_rx_data_tkeep;
wire        axis_net_rx_data_tlast;

wire        axis_net_tx_data_tvalid;
wire        axis_net_tx_data_tready;
wire[63:0]  axis_net_tx_data_tdata;
wire[7:0]   axis_net_tx_data_tkeep;
wire        axis_net_tx_data_tlast;


/*
 * RX Memory Signals
 */
// memory cmd streams
wire        axis_rxread_cmd_tvalid;
wire        axis_rxread_cmd_tready;
wire[71:0]  axis_rxread_cmd_tdata;
wire        axis_rxwrite_cmd_tvalid;
wire        axis_rxwrite_cmd_tready;
wire[71:0]  axis_rxwrite_cmd_tdata;
// memory sts streams
wire        axis_rxwrite_sts_tvalid;
wire        axis_rxwrite_sts_tready;
wire[7:0]  axis_rxwrite_sts_tdata;
// memory data streams
wire        axis_rxread_data_tvalid;
wire        axis_rxread_data_tready;
wire[63:0]  axis_rxread_data_tdata;
wire[7:0]   axis_rxread_data_tkeep;
wire        axis_rxread_data_tlast;

wire        axis_rxwrite_data_tvalid;
wire        axis_rxwrite_data_tready;
wire[63:0]  axis_rxwrite_data_tdata;
wire[7:0]   axis_rxwrite_data_tkeep;
wire        axis_rxwrite_data_tlast;

/*
 * TX Memory Signals
 */
// memory cmd streams
wire        axis_txread_cmd_tvalid;
wire        axis_txread_cmd_tready;
wire[71:0]  axis_txread_cmd_tdata;
wire        axis_txwrite_cmd_tvalid;
wire        axis_txwrite_cmd_tready;
wire[71:0]  axis_txwrite_cmd_tdata;
// memory sts streams
wire        axis_txwrite_sts_tvalid;
wire        axis_txwrite_sts_tready;
wire[7:0]  axis_txwrite_sts_tdata;
// memory data streams
wire        axis_txread_data_tvalid;
wire        axis_txread_data_tready;
wire[63:0]  axis_txread_data_tdata;
wire[7:0]   axis_txread_data_tkeep;
wire        axis_txread_data_tlast;

wire        axis_txwrite_data_tvalid;
wire        axis_txwrite_data_tready;
wire[63:0]  axis_txwrite_data_tdata;
wire[7:0]   axis_txwrite_data_tkeep;
wire        axis_txwrite_data_tlast;

/*
 * Application Signals
 */
 // listen&close port
  // open&close connection
wire        axis_listen_port_tvalid;
wire        axis_listen_port_tready;
wire[15:0]  axis_listen_port_tdata;
wire        axis_listen_port_status_tvalid;
wire        axis_listen_port_status_tready;
wire[7:0]   axis_listen_port_status_tdata;
 // notifications and pkg fetching
wire        axis_notifications_tvalid;
wire        axis_notifications_tready;
wire[87:0]  axis_notifications_tdata;
wire        axis_read_package_tvalid;
wire        axis_read_package_tready;
wire[31:0]  axis_read_package_tdata;
// open&close connection
wire        axis_open_connection_tvalid;
wire        axis_open_connection_tready;
wire[47:0]  axis_open_connection_tdata;
wire        axis_open_status_tvalid;
wire        axis_open_status_tready;
wire[23:0]  axis_open_status_tdata;
wire        axis_close_connection_tvalid;
wire        axis_close_connection_tready;
wire[15:0]  axis_close_connection_tdata;
// rx data
wire        axis_rx_metadata_tvalid;
wire        axis_rx_metadata_tready;
wire[15:0]  axis_rx_metadata_tdata;
wire        axis_rx_data_tvalid;
wire        axis_rx_data_tready;
wire[63:0]  axis_rx_data_tdata;
wire[7:0]   axis_rx_data_tkeep;
wire        axis_rx_data_tlast;
// tx data
wire        axis_tx_metadata_tvalid;
wire        axis_tx_metadata_tready;
wire[31:0]  axis_tx_metadata_tdata;
wire        axis_tx_data_tvalid;
wire        axis_tx_data_tready;
wire[63:0]  axis_tx_data_tdata;
wire[7:0]   axis_tx_data_tkeep;
wire        axis_tx_data_tlast;
wire        axis_tx_status_tvalid;
wire        axis_tx_status_tready;
wire[31:0]  axis_tx_status_tdata;

/*
 * UPD signals
 */
`ifdef UDP
wire        axis_rx_udp_metadata_tvalid;
wire        axis_rx_udp_metadata_tready;
wire[175:0] axis_rx_udp_metadata_tdata;
wire        axis_rx_udp_data_tvalid;
wire        axis_rx_udp_data_tready;
wire[63:0]  axis_rx_udp_data_tdata;
wire[7:0]   axis_rx_udp_data_tkeep;
wire        axis_rx_udp_data_tlast;

wire        axis_tx_udp_metadata_tvalid;
wire        axis_tx_udp_metadata_tready;
wire[175:0] axis_tx_udp_metadata_tdata;
wire        axis_tx_udp_data_tvalid;
wire        axis_tx_udp_data_tready;
wire[63:0]  axis_tx_udp_data_tdata;
wire[7:0]   axis_tx_udp_data_tkeep;
wire        axis_tx_udp_data_tlast;
`endif
  
network_module network_module_inst
(
    .dclk (dclk),
    .net_clk(aclk),
    .sys_reset (sys_reset),
    .aresetn(aresetn),
    .network_init_done(network_init),
    
    .gt_refclk_p(gt_refclk_p),
    .gt_refclk_n(gt_refclk_n),
    
    .gt_rxp_in(gt_rxp_in),
    .gt_rxn_in(gt_rxn_in),
    .gt_txp_out(gt_txp_out),
    .gt_txn_out(gt_txn_out),
    
    .user_rx_reset(user_rx_reset),
    .user_tx_reset(user_tx_reset),
    .gtpowergood_out(gtpowergood_out),
    
    //master 0
     .m_axis_0_tvalid(axis_net_rx_data_tvalid),
     .m_axis_0_tready(axis_net_rx_data_tready),
     .m_axis_0_tdata(axis_net_rx_data_tdata),
     .m_axis_0_tkeep(axis_net_rx_data_tkeep),
     .m_axis_0_tlast(axis_net_rx_data_tlast),
         
     //slave 0
     .s_axis_0_tvalid(axis_net_tx_data_tvalid),
     .s_axis_0_tready(axis_net_tx_data_tready),
     .s_axis_0_tdata(axis_net_tx_data_tdata),
     .s_axis_0_tkeep(axis_net_tx_data_tkeep),
     .s_axis_0_tlast(axis_net_tx_data_tlast)
    
     //master 1
     /*.m_axis_1_tvalid(axis_net_rx_data_tvalid[1]),
     .m_axis_1_tready(axis_net_rx_data_tready[1]),
     .m_axis_1_tdata(axis_net_rx_data_tdata[1]),
     .m_axis_1_tkeep(axis_net_rx_data_tkeep[1]),
     .m_axis_1_tlast(axis_net_rx_data_tlast[1]),
         
     //slave 1
     .s_axis_1_tvalid(axis_net_tx_data_tvalid[1]),
     .s_axis_1_tready(axis_net_tx_data_tready[1]),
     .s_axis_1_tdata(axis_net_tx_data_tdata[1]),
     .s_axis_1_tkeep(axis_net_tx_data_tkeep[1]),
     .s_axis_1_tlast(axis_net_tx_data_tlast[1]),
    
      //master 2
     .m_axis_2_tvalid(axis_net_rx_data_tvalid[2]),
     .m_axis_2_tready(axis_net_rx_data_tready[2]),
     .m_axis_2_tdata(axis_net_rx_data_tdata[2]),
     .m_axis_2_tkeep(axis_net_rx_data_tkeep[2]),
     .m_axis_2_tlast(axis_net_rx_data_tlast[2]),
         
     //slave 2
     .s_axis_2_tvalid(axis_net_tx_data_tvalid[2]),
     .s_axis_2_tready(axis_net_tx_data_tready[2]),
     .s_axis_2_tdata(axis_net_tx_data_tdata[2]),
     .s_axis_2_tkeep(axis_net_tx_data_tkeep[2]),
     .s_axis_2_tlast(axis_net_tx_data_tlast[2]),
      
     //master 3
     .m_axis_3_tvalid(axis_net_rx_data_tvalid[3]),
     .m_axis_3_tready(axis_net_rx_data_tready[3]),
     .m_axis_3_tdata(axis_net_rx_data_tdata[3]),
     .m_axis_3_tkeep(axis_net_rx_data_tkeep[3]),
     .m_axis_3_tlast(axis_net_rx_data_tlast[3]),
         
     //slave 3
     .s_axis_3_tvalid(axis_net_tx_data_tvalid[3]),
     .s_axis_3_tready(axis_net_tx_data_tready[3]),
     .s_axis_3_tdata(axis_net_tx_data_tdata[3]),
     .s_axis_3_tkeep(axis_net_tx_data_tkeep[3]),
     .s_axis_3_tlast(axis_net_tx_data_tlast[3])*/

);




/*
 * TCP/IP Wrapper Module
 */
wire [15:0] regSessionCount;
wire regSessionCount_valid;

wire[31:0]  ip_address_out;
reg[31:0] local_ip_address;
reg[31:0] target_ip_address;

always @(posedge aclk) begin
    local_ip_address <= 32'hD1D4010A; //0x0A01D4D1 -> 10.1.212.209
    target_ip_address <= {24'h0AD401, 8'h0A + gpio_switch[3]}; // 10.1.212.10
end


network_stack #(
    .IP_SUBNET_MASK     (32'h00FFFFFF), //reverse
    .IP_DEFAULT_GATEWAY     (32'h01D4010A) //reverse //TODO fix this
    )
network_stack_inst (
.aclk                           (aclk),
.aresetn                        (aresetn),

// network interface streams
.AXI_M_Stream_TVALID            (axis_net_tx_data_tvalid),
.AXI_M_Stream_TREADY            (axis_net_tx_data_tready),
.AXI_M_Stream_TDATA             (axis_net_tx_data_tdata),
.AXI_M_Stream_TKEEP             (axis_net_tx_data_tkeep),
.AXI_M_Stream_TLAST             (axis_net_tx_data_tlast),

.AXI_S_Stream_TVALID            (axis_net_rx_data_tvalid),
.AXI_S_Stream_TREADY            (axis_net_rx_data_tready),
.AXI_S_Stream_TDATA             (axis_net_rx_data_tdata),
.AXI_S_Stream_TKEEP             (axis_net_rx_data_tkeep),
.AXI_S_Stream_TLAST             (axis_net_rx_data_tlast),

// memory rx cmd streams
.m_axis_rxread_cmd_TVALID       (axis_rxread_cmd_tvalid),
.m_axis_rxread_cmd_TREADY       (axis_rxread_cmd_tready),
.m_axis_rxread_cmd_TDATA        (axis_rxread_cmd_tdata),
.m_axis_rxwrite_cmd_TVALID      (axis_rxwrite_cmd_tvalid),
.m_axis_rxwrite_cmd_TREADY      (axis_rxwrite_cmd_tready),
.m_axis_rxwrite_cmd_TDATA       (axis_rxwrite_cmd_tdata),
// memory rx status streams
.s_axis_rxwrite_sts_TVALID      (axis_rxwrite_sts_tvalid),
.s_axis_rxwrite_sts_TREADY      (axis_rxwrite_sts_tready),
.s_axis_rxwrite_sts_TDATA       (axis_rxwrite_sts_tdata),
// memory rx data streams
.s_axis_rxread_data_TVALID      (axis_rxread_data_tvalid),
.s_axis_rxread_data_TREADY      (axis_rxread_data_tready),
.s_axis_rxread_data_TDATA       (axis_rxread_data_tdata),
.s_axis_rxread_data_TKEEP       (axis_rxread_data_tkeep),
.s_axis_rxread_data_TLAST       (axis_rxread_data_tlast),
.m_axis_rxwrite_data_TVALID     (axis_rxwrite_data_tvalid),
.m_axis_rxwrite_data_TREADY     (axis_rxwrite_data_tready),
.m_axis_rxwrite_data_TDATA      (axis_rxwrite_data_tdata),
.m_axis_rxwrite_data_TKEEP      (axis_rxwrite_data_tkeep),
.m_axis_rxwrite_data_TLAST      (axis_rxwrite_data_tlast),

// memory tx cmd streams
.m_axis_txread_cmd_TVALID       (axis_txread_cmd_tvalid),
.m_axis_txread_cmd_TREADY       (axis_txread_cmd_tready),
.m_axis_txread_cmd_TDATA        (axis_txread_cmd_tdata),
.m_axis_txwrite_cmd_TVALID      (axis_txwrite_cmd_tvalid),
.m_axis_txwrite_cmd_TREADY      (axis_txwrite_cmd_tready),
.m_axis_txwrite_cmd_TDATA       (axis_txwrite_cmd_tdata),
// memory tx status streams
.s_axis_txwrite_sts_TVALID      (axis_txwrite_sts_tvalid),
.s_axis_txwrite_sts_TREADY      (axis_txwrite_sts_tready),
.s_axis_txwrite_sts_TDATA       (axis_txwrite_sts_tdata),
// memory tx data streams
.s_axis_txread_data_TVALID      (axis_txread_data_tvalid),
.s_axis_txread_data_TREADY      (axis_txread_data_tready),
.s_axis_txread_data_TDATA       (axis_txread_data_tdata),
.s_axis_txread_data_TKEEP       (axis_txread_data_tkeep),
.s_axis_txread_data_TLAST       (axis_txread_data_tlast),
.m_axis_txwrite_data_TVALID     (axis_txwrite_data_tvalid),
.m_axis_txwrite_data_TREADY     (axis_txwrite_data_tready),
.m_axis_txwrite_data_TDATA      (axis_txwrite_data_tdata),
.m_axis_txwrite_data_TKEEP      (axis_txwrite_data_tkeep),
.m_axis_txwrite_data_TLAST      (axis_txwrite_data_tlast),

//application interface streams
.m_axis_listen_port_status_TVALID       (axis_listen_port_status_tvalid),
.m_axis_listen_port_status_TREADY       (axis_listen_port_status_tready),
.m_axis_listen_port_status_TDATA        (axis_listen_port_status_tdata),
.m_axis_notifications_TVALID            (axis_notifications_tvalid),
.m_axis_notifications_TREADY            (axis_notifications_tready),
.m_axis_notifications_TDATA             (axis_notifications_tdata),
.m_axis_open_status_TVALID              (axis_open_status_tvalid),
.m_axis_open_status_TREADY              (axis_open_status_tready),
.m_axis_open_status_TDATA               (axis_open_status_tdata),
.m_axis_rx_data_TVALID                  (axis_rx_data_tvalid),
.m_axis_rx_data_TREADY                  (axis_rx_data_tready),
.m_axis_rx_data_TDATA                   (axis_rx_data_tdata),
.m_axis_rx_data_TKEEP                   (axis_rx_data_tkeep),
.m_axis_rx_data_TLAST                   (axis_rx_data_tlast),
.m_axis_rx_metadata_TVALID              (axis_rx_metadata_tvalid),
.m_axis_rx_metadata_TREADY              (axis_rx_metadata_tready),
.m_axis_rx_metadata_TDATA               (axis_rx_metadata_tdata),
.m_axis_tx_status_TVALID                (axis_tx_status_tvalid),
.m_axis_tx_status_TREADY                (axis_tx_status_tready),
.m_axis_tx_status_TDATA                 (axis_tx_status_tdata),
.s_axis_listen_port_TVALID              (axis_listen_port_tvalid),
.s_axis_listen_port_TREADY              (axis_listen_port_tready),
.s_axis_listen_port_TDATA               (axis_listen_port_tdata),
.s_axis_close_connection_TVALID         (axis_close_connection_tvalid),
.s_axis_close_connection_TREADY         (axis_close_connection_tready),
.s_axis_close_connection_TDATA          (axis_close_connection_tdata),
.s_axis_open_connection_TVALID          (axis_open_connection_tvalid),
.s_axis_open_connection_TREADY          (axis_open_connection_tready),
.s_axis_open_connection_TDATA           (axis_open_connection_tdata),
.s_axis_read_package_TVALID             (axis_read_package_tvalid),
.s_axis_read_package_TREADY             (axis_read_package_tready),
.s_axis_read_package_TDATA              (axis_read_package_tdata),
.s_axis_tx_data_TVALID                  (axis_tx_data_tvalid),
.s_axis_tx_data_TREADY                  (axis_tx_data_tready),
.s_axis_tx_data_TDATA                   (axis_tx_data_tdata),
.s_axis_tx_data_TKEEP                   (axis_tx_data_tkeep),
.s_axis_tx_data_TLAST                   (axis_tx_data_tlast),
.s_axis_tx_metadata_TVALID              (axis_tx_metadata_tvalid),
.s_axis_tx_metadata_TREADY              (axis_tx_metadata_tready),
.s_axis_tx_metadata_TDATA               (axis_tx_metadata_tdata),
`ifdef UDP
//UPD interface
//RX
.m_axis_udp_metadata_TVALID     (axis_rx_udp_metadata_tvalid),
.m_axis_udp_metadata_TREADY     (axis_rx_udp_metadata_tready),
.m_axis_udp_metadata_TDATA      (axis_rx_udp_metadata_tdata),
.m_axis_udp_data_TVALID         (axis_rx_udp_data_tvalid),
.m_axis_udp_data_TREADY         (axis_rx_udp_data_tready),
.m_axis_udp_data_TDATA          (axis_rx_udp_data_tdata),
.m_axis_udp_data_TKEEP          (axis_rx_udp_data_tkeep),
.m_axis_udp_data_TLAST          (axis_rx_udp_data_tlast),
//TX
.s_axis_udp_metadata_TVALID     (axis_tx_udp_metadata_tvalid),
.s_axis_udp_metadata_TREADY     (axis_tx_udp_metadata_tready),
.s_axis_udp_metadata_TDATA      (axis_tx_udp_metadata_tdata),
.s_axis_udp_data_TVALID         (axis_tx_udp_data_tvalid),
.s_axis_udp_data_TREADY         (axis_tx_udp_data_tready),
.s_axis_udp_data_TDATA          (axis_tx_udp_data_tdata),
.s_axis_udp_data_TKEEP          (axis_tx_udp_data_tkeep),
.s_axis_udp_data_TLAST          (axis_tx_udp_data_tlast),
`endif
.ip_address_in(local_ip_address),
.ip_address_out(ip_address_out),
.regSessionCount_V(regSessionCount),
.regSessionCount_V_ap_vld(regSessionCount_valid),

.board_number({1'b0, gpio_switch[2:0]}),
.subnet_number({1'b0, gpio_switch[3]})

);


/*
 * Application Module
 */
/*echo_server_application_ip echo_server (
  .m_axis_close_connection_V_V_TVALID(axis_close_connection_tvalid),      // output wire m_axis_close_connection_TVALID
  .m_axis_close_connection_V_V_TREADY(axis_close_connection_tready),      // input wire m_axis_close_connection_TREADY
  .m_axis_close_connection_V_V_TDATA(axis_close_connection_tdata),        // output wire [15 : 0] m_axis_close_connection_TDATA
  .m_axis_listen_port_V_V_TVALID(axis_listen_port_tvalid),                // output wire m_axis_listen_port_TVALID
  .m_axis_listen_port_V_V_TREADY(axis_listen_port_tready),                // input wire m_axis_listen_port_TREADY
  .m_axis_listen_port_V_V_TDATA(axis_listen_port_tdata),                  // output wire [15 : 0] m_axis_listen_port_TDATA
  .m_axis_open_connection_V_TVALID(axis_open_connection_tvalid),        // output wire m_axis_open_connection_TVALID
  .m_axis_open_connection_V_TREADY(axis_open_connection_tready),        // input wire m_axis_open_connection_TREADY
  .m_axis_open_connection_V_TDATA(axis_open_connection_tdata),          // output wire [47 : 0] m_axis_open_connection_TDATA
  .m_axis_read_package_V_TVALID(axis_read_package_tvalid),              // output wire m_axis_read_package_TVALID
  .m_axis_read_package_V_TREADY(axis_read_package_tready),              // input wire m_axis_read_package_TREADY
  .m_axis_read_package_V_TDATA(axis_read_package_tdata),                // output wire [31 : 0] m_axis_read_package_TDATA
  .m_axis_tx_data_TVALID(axis_tx_data_tvalid),                        // output wire m_axis_tx_data_TVALID
  .m_axis_tx_data_TREADY(axis_tx_data_tready),                        // input wire m_axis_tx_data_TREADY
  .m_axis_tx_data_TDATA(axis_tx_data_tdata),                          // output wire [63 : 0] m_axis_tx_data_TDATA
  .m_axis_tx_data_TKEEP(axis_tx_data_tkeep),                          // output wire [7 : 0] m_axis_tx_data_TKEEP
  .m_axis_tx_data_TLAST(axis_tx_data_tlast),                          // output wire [0 : 0] m_axis_tx_data_TLAST
  .m_axis_tx_metadata_V_TVALID(axis_tx_metadata_tvalid),                // output wire m_axis_tx_metadata_TVALID
  .m_axis_tx_metadata_V_TREADY(axis_tx_metadata_tready),                // input wire m_axis_tx_metadata_TREADY
  .m_axis_tx_metadata_V_TDATA(axis_tx_metadata_tdata),                  // output wire [15 : 0] m_axis_tx_metadata_TDATA
  .s_axis_listen_port_status_V_TVALID(axis_listen_port_status_tvalid),//axis_listen_port_status_tvalid),  // input wire s_axis_listen_port_status_TVALID
  .s_axis_listen_port_status_V_TREADY(axis_listen_port_status_tready),//axis_listen_port_status_tready),  // output wire s_axis_listen_port_status_TREADY
  .s_axis_listen_port_status_V_TDATA(axis_listen_port_status_tdata),//axis_listen_port_status_tdata),    // input wire [7 : 0] s_axis_listen_port_status_TDATA
  .s_axis_notifications_V_TVALID(axis_notifications_tvalid),            // input wire s_axis_notifications_TVALID
  .s_axis_notifications_V_TREADY(axis_notifications_tready),            // output wire s_axis_notifications_TREADY
  .s_axis_notifications_V_TDATA(axis_notifications_tdata),              // input wire [87 : 0] s_axis_notifications_TDATA
  .s_axis_open_status_V_TVALID(axis_open_status_tvalid),                // input wire s_axis_open_status_TVALID
  .s_axis_open_status_V_TREADY(axis_open_status_tready),                // output wire s_axis_open_status_TREADY
  .s_axis_open_status_V_TDATA(axis_open_status_tdata),                  // input wire [23 : 0] s_axis_open_status_TDATA
  .s_axis_rx_data_TVALID(axis_rx_data_tvalid),                        // input wire s_axis_rx_data_TVALID
  .s_axis_rx_data_TREADY(axis_rx_data_tready),                        // output wire s_axis_rx_data_TREADY
  .s_axis_rx_data_TDATA(axis_rx_data_tdata),                          // input wire [63 : 0] s_axis_rx_data_TDATA
  .s_axis_rx_data_TKEEP(axis_rx_data_tkeep),                          // input wire [7 : 0] s_axis_rx_data_TKEEP
  .s_axis_rx_data_TLAST(axis_rx_data_tlast),                          // input wire [0 : 0] s_axis_rx_data_TLAST
  .s_axis_rx_metadata_V_V_TVALID(axis_rx_metadata_tvalid),                // input wire s_axis_rx_metadata_TVALID
  .s_axis_rx_metadata_V_V_TREADY(axis_rx_metadata_tready),                // output wire s_axis_rx_metadata_TREADY
  .s_axis_rx_metadata_V_V_TDATA(axis_rx_metadata_tdata),                  // input wire [15 : 0] s_axis_rx_metadata_TDATA
  .s_axis_tx_status_V_TVALID(axis_tx_status_tvalid),                    // input wire s_axis_tx_status_TVALID
  .s_axis_tx_status_V_TREADY(axis_tx_status_tready),                    // output wire s_axis_tx_status_TREADY
  .s_axis_tx_status_V_TDATA(axis_tx_status_tdata),                      // input wire [23 : 0] s_axis_tx_status_TDATA
  .ap_clk(aclk),                                                          // input wire aclk
  .ap_rst_n(aresetn)                                                    // input wire aresetn
);*/


`ifdef ECHO_SERVER
echo_server_application_ip echo_server (
  .m_axis_close_connection_TVALID(axis_close_connection_tvalid),      // output wire m_axis_close_connection_TVALID
  .m_axis_close_connection_TREADY(axis_close_connection_tready),      // input wire m_axis_close_connection_TREADY
  .m_axis_close_connection_TDATA(axis_close_connection_tdata),        // output wire [15 : 0] m_axis_close_connection_TDATA
  .m_axis_listen_port_TVALID(axis_listen_port_tvalid),                // output wire m_axis_listen_port_TVALID
  .m_axis_listen_port_TREADY(axis_listen_port_tready),                // input wire m_axis_listen_port_TREADY
  .m_axis_listen_port_TDATA(axis_listen_port_tdata),                  // output wire [15 : 0] m_axis_listen_port_TDATA
  .m_axis_open_connection_TVALID(axis_open_connection_tvalid),        // output wire m_axis_open_connection_TVALID
  .m_axis_open_connection_TREADY(axis_open_connection_tready),        // input wire m_axis_open_connection_TREADY
  .m_axis_open_connection_TDATA(axis_open_connection_tdata),          // output wire [47 : 0] m_axis_open_connection_TDATA
  .m_axis_read_package_TVALID(axis_read_package_tvalid),              // output wire m_axis_read_package_TVALID
  .m_axis_read_package_TREADY(axis_read_package_tready),              // input wire m_axis_read_package_TREADY
  .m_axis_read_package_TDATA(axis_read_package_tdata),                // output wire [31 : 0] m_axis_read_package_TDATA
  .m_axis_tx_data_TVALID(axis_tx_data_tvalid),                        // output wire m_axis_tx_data_TVALID
  .m_axis_tx_data_TREADY(axis_tx_data_tready),                        // input wire m_axis_tx_data_TREADY
  .m_axis_tx_data_TDATA(axis_tx_data_tdata),                          // output wire [63 : 0] m_axis_tx_data_TDATA
  .m_axis_tx_data_TKEEP(axis_tx_data_tkeep),                          // output wire [7 : 0] m_axis_tx_data_TKEEP
  .m_axis_tx_data_TLAST(axis_tx_data_tlast),                          // output wire [0 : 0] m_axis_tx_data_TLAST
  .m_axis_tx_metadata_TVALID(axis_tx_metadata_tvalid),                // output wire m_axis_tx_metadata_TVALID
  .m_axis_tx_metadata_TREADY(axis_tx_metadata_tready),                // input wire m_axis_tx_metadata_TREADY
  .m_axis_tx_metadata_TDATA(axis_tx_metadata_tdata),                  // output wire [15 : 0] m_axis_tx_metadata_TDATA
  .s_axis_listen_port_status_TVALID(axis_listen_port_status_tvalid),  // input wire s_axis_listen_port_status_TVALID
  .s_axis_listen_port_status_TREADY(axis_listen_port_status_tready),  // output wire s_axis_listen_port_status_TREADY
  .s_axis_listen_port_status_TDATA(axis_listen_port_status_tdata),    // input wire [7 : 0] s_axis_listen_port_status_TDATA
  .s_axis_notifications_TVALID(axis_notifications_tvalid),            // input wire s_axis_notifications_TVALID
  .s_axis_notifications_TREADY(axis_notifications_tready),            // output wire s_axis_notifications_TREADY
  .s_axis_notifications_TDATA(axis_notifications_tdata),              // input wire [87 : 0] s_axis_notifications_TDATA
  .s_axis_open_status_TVALID(axis_open_status_tvalid),                // input wire s_axis_open_status_TVALID
  .s_axis_open_status_TREADY(axis_open_status_tready),                // output wire s_axis_open_status_TREADY
  .s_axis_open_status_TDATA(axis_open_status_tdata),                  // input wire [23 : 0] s_axis_open_status_TDATA
  .s_axis_rx_data_TVALID(axis_rx_data_tvalid),                        // input wire s_axis_rx_data_TVALID
  .s_axis_rx_data_TREADY(axis_rx_data_tready),                        // output wire s_axis_rx_data_TREADY
  .s_axis_rx_data_TDATA(axis_rx_data_tdata),                          // input wire [63 : 0] s_axis_rx_data_TDATA
  .s_axis_rx_data_TKEEP(axis_rx_data_tkeep),                          // input wire [7 : 0] s_axis_rx_data_TKEEP
  .s_axis_rx_data_TLAST(axis_rx_data_tlast),                          // input wire [0 : 0] s_axis_rx_data_TLAST
  .s_axis_rx_metadata_TVALID(axis_rx_metadata_tvalid),                // input wire s_axis_rx_metadata_TVALID
  .s_axis_rx_metadata_TREADY(axis_rx_metadata_tready),                // output wire s_axis_rx_metadata_TREADY
  .s_axis_rx_metadata_TDATA(axis_rx_metadata_tdata),                  // input wire [15 : 0] s_axis_rx_metadata_TDATA
  .s_axis_tx_status_TVALID(axis_tx_status_tvalid),                    // input wire s_axis_tx_status_TVALID
  .s_axis_tx_status_TREADY(axis_tx_status_tready),                    // output wire s_axis_tx_status_TREADY
  .s_axis_tx_status_TDATA(axis_tx_status_tdata),                      // input wire [23 : 0] s_axis_tx_status_TDATA
  .aclk(aclk),                                                          // input wire aclk
  .aresetn(aresetn)                                                    // input wire aresetn
);
`endif

`ifdef IPERF_CLIENT
wire        runExperiment;
wire        dualMode;
wire[7:0]   noOfConnections;
wire[7:0]   pkgWordCount;

vio_iperf vio_iperf_client_inst (
  .clk(aclk),                    // input wire clk
  .probe_out0(runExperiment),       // output wire [0 : 0] probe_out0
  .probe_out1(dualMode),            // output wire [0 : 0] probe_out1
  .probe_out2(noOfConnections),     // output wire [7 : 0] probe_out2
  .probe_out3(pkgWordCount)         // output wire [7 : 0] probe_out3
);

iperf_client_ip iperf_client (
  .m_axis_close_connection_TVALID(axis_close_connection_tvalid),      // output wire m_axis_close_connection_TVALID
  .m_axis_close_connection_TREADY(axis_close_connection_tready),      // input wire m_axis_close_connection_TREADY
  .m_axis_close_connection_TDATA(axis_close_connection_tdata),        // output wire [15 : 0] m_axis_close_connection_TDATA
  .m_axis_listen_port_TVALID(axis_listen_port_tvalid),                // output wire m_axis_listen_port_TVALID
  .m_axis_listen_port_TREADY(axis_listen_port_tready),                // input wire m_axis_listen_port_TREADY
  .m_axis_listen_port_TDATA(axis_listen_port_tdata),                  // output wire [15 : 0] m_axis_listen_port_TDATA
  .m_axis_open_connection_TVALID(axis_open_connection_tvalid),        // output wire m_axis_open_connection_TVALID
  .m_axis_open_connection_TREADY(axis_open_connection_tready),        // input wire m_axis_open_connection_TREADY
  .m_axis_open_connection_TDATA(axis_open_connection_tdata),          // output wire [47 : 0] m_axis_open_connection_TDATA
  .m_axis_read_package_TVALID(axis_read_package_tvalid),              // output wire m_axis_read_package_TVALID
  .m_axis_read_package_TREADY(axis_read_package_tready),              // input wire m_axis_read_package_TREADY
  .m_axis_read_package_TDATA(axis_read_package_tdata),                // output wire [31 : 0] m_axis_read_package_TDATA
  .m_axis_tx_data_TVALID(axis_tx_data_tvalid),                        // output wire m_axis_tx_data_TVALID
  .m_axis_tx_data_TREADY(axis_tx_data_tready),                        // input wire m_axis_tx_data_TREADY
  .m_axis_tx_data_TDATA(axis_tx_data_tdata),                          // output wire [63 : 0] m_axis_tx_data_TDATA
  .m_axis_tx_data_TKEEP(axis_tx_data_tkeep),                          // output wire [7 : 0] m_axis_tx_data_TKEEP
  .m_axis_tx_data_TLAST(axis_tx_data_tlast),                          // output wire [0 : 0] m_axis_tx_data_TLAST
  .m_axis_tx_metadata_TVALID(axis_tx_metadata_tvalid),                // output wire m_axis_tx_metadata_TVALID
  .m_axis_tx_metadata_TREADY(axis_tx_metadata_tready),                // input wire m_axis_tx_metadata_TREADY
  .m_axis_tx_metadata_TDATA(axis_tx_metadata_tdata),                  // output wire [15 : 0] m_axis_tx_metadata_TDATA
  .s_axis_listen_port_status_TVALID(axis_listen_port_status_tvalid),  // input wire s_axis_listen_port_status_TVALID
  .s_axis_listen_port_status_TREADY(axis_listen_port_status_tready),  // output wire s_axis_listen_port_status_TREADY
  .s_axis_listen_port_status_TDATA(axis_listen_port_status_tdata),    // input wire [7 : 0] s_axis_listen_port_status_TDATA
  .s_axis_notifications_TVALID(axis_notifications_tvalid),            // input wire s_axis_notifications_TVALID
  .s_axis_notifications_TREADY(axis_notifications_tready),            // output wire s_axis_notifications_TREADY
  .s_axis_notifications_TDATA(axis_notifications_tdata),              // input wire [87 : 0] s_axis_notifications_TDATA
  .s_axis_open_status_TVALID(axis_open_status_tvalid),                // input wire s_axis_open_status_TVALID
  .s_axis_open_status_TREADY(axis_open_status_tready),                // output wire s_axis_open_status_TREADY
  .s_axis_open_status_TDATA(axis_open_status_tdata),                  // input wire [23 : 0] s_axis_open_status_TDATA
  .s_axis_rx_data_TVALID(axis_rx_data_tvalid),                        // input wire s_axis_rx_data_TVALID
  .s_axis_rx_data_TREADY(axis_rx_data_tready),                        // output wire s_axis_rx_data_TREADY
  .s_axis_rx_data_TDATA(axis_rx_data_tdata),                          // input wire [63 : 0] s_axis_rx_data_TDATA
  .s_axis_rx_data_TKEEP(axis_rx_data_tkeep),                          // input wire [7 : 0] s_axis_rx_data_TKEEP
  .s_axis_rx_data_TLAST(axis_rx_data_tlast),                          // input wire [0 : 0] s_axis_rx_data_TLAST
  .s_axis_rx_metadata_TVALID(axis_rx_metadata_tvalid),                // input wire s_axis_rx_metadata_TVALID
  .s_axis_rx_metadata_TREADY(axis_rx_metadata_tready),                // output wire s_axis_rx_metadata_TREADY
  .s_axis_rx_metadata_TDATA(axis_rx_metadata_tdata),                  // input wire [15 : 0] s_axis_rx_metadata_TDATA
  .s_axis_tx_status_TVALID(axis_tx_status_tvalid),                    // input wire s_axis_tx_status_TVALID
  .s_axis_tx_status_TREADY(axis_tx_status_tready),                    // output wire s_axis_tx_status_TREADY
  .s_axis_tx_status_TDATA(axis_tx_status_tdata),                      // input wire [23 : 0] s_axis_tx_status_TDATA
  
  //Client only
  .runExperiment_V(runExperiment | button_west),
  .dualModeEn_V(dualMode),                                          // input wire [0 : 0] dualModeEn_V
  .useConn_V(noOfConnections),                                                // input wire [7 : 0] useConn_V
  .pkgWordCount_V(pkgWordCount),                                      // input wire [7 : 0] pkgWordCount_V
  .regIpAddress0_V(32'h0B01D40A),                                    // input wire [31 : 0] regIpAddress1_V
  .regIpAddress1_V(32'h0B01D40A),                                    // input wire [31 : 0] regIpAddress1_V
  .regIpAddress2_V(32'h0B01D40A),                                    // input wire [31 : 0] regIpAddress1_V
  .regIpAddress3_V(32'h0B01D40A),                                    // input wire [31 : 0] regIpAddress1_V
  .aclk(aclk),                                                          // input wire aclk
  .aresetn(aresetn)                                                    // input wire aresetn
);
`endif

/*
 * UDP Application Module
 */
`ifdef UDP
wire runUdpExperiment;

vio_udp_iperf_client vio_udp_iperf_client_inst (
  .clk(aclk),                // input wire clk
  .probe_out0(runUdpExperiment)  // output wire [0 : 0] probe_out0
);

reg runIperfUdp;
reg[7:0] packetGap;
 always @(posedge aclk) begin
     if (~aresetn) begin
         runIperfUdp <= 0;
         packetGap <= 0;
     end
     else begin
         runIperfUdp <= 0;
         if (button_north) begin
             packetGap <= 0;
             runIperfUdp <= 1;
         end
         if (button_center) begin
             packetGap <= 1;
             runIperfUdp <= 1;
         end
         if (button_south) begin
             packetGap <= 9;
             runIperfUdp <= 1;
         end
     end
 end
 
 iperf_udp_client_ip iperf_udp_client_inst (
   .aclk(aclk),                                            // input wire aclk
   .aresetn(aresetn),                                      // input wire aresetn
   .runExperiment_V(runUdpExperiment | runIperfUdp),                      // input wire [0 : 0] runExperiment_V
   .regPacketGap_V(packetGap),
   //.regMyIpAddress_V(32'h02D4010B),                    // input wire [31 : 0] regMyIpAddress_V
   .regTargetIpAddress_V({target_ip_address, target_ip_address, target_ip_address, target_ip_address}),            // input wire [31 : 0] regTargetIpAddress_V
   
   .s_axis_rx_metadata_TVALID(axis_rx_udp_metadata_tvalid),
   .s_axis_rx_metadata_TREADY(axis_rx_udp_metadata_tready),
   .s_axis_rx_metadata_TDATA(axis_rx_udp_metadata_tdata),  
   .s_axis_rx_data_TVALID(axis_rx_udp_data_tvalid),
   .s_axis_rx_data_TREADY(axis_rx_udp_data_tready),
   .s_axis_rx_data_TDATA(axis_rx_udp_data_tdata),
   .s_axis_rx_data_TKEEP(axis_rx_udp_data_tkeep),
   .s_axis_rx_data_TLAST(axis_rx_udp_data_tlast),
   
   .m_axis_tx_metadata_TVALID(axis_tx_udp_metadata_tvalid),
   .m_axis_tx_metadata_TREADY(axis_tx_udp_metadata_tready),
   .m_axis_tx_metadata_TDATA(axis_tx_udp_metadata_tdata),
   .m_axis_tx_data_TVALID(axis_tx_udp_data_tvalid),
   .m_axis_tx_data_TREADY(axis_tx_udp_data_tready),
   .m_axis_tx_data_TDATA(axis_tx_udp_data_tdata),
   .m_axis_tx_data_TKEEP(axis_tx_udp_data_tkeep),
   .m_axis_tx_data_TLAST(axis_tx_udp_data_tlast)
 );
 `endif
 

/*
 * DDR MEMORY
 */

wire c0_init_calib_complete;
wire c1_init_calib_complete;

wire c0_ui_clk;
wire ddr4_calib_complete;
//wire init_calib_complete;
//registers for crossing clock domains
reg c0_init_calib_complete_r1, c0_init_calib_complete_r2;
reg c1_init_calib_complete_r1, c1_init_calib_complete_r2;

always @(posedge aclk) 
if (~aresetn) begin
    c0_init_calib_complete_r1 <= 1'b0;
    c0_init_calib_complete_r2 <= 1'b0;
    c1_init_calib_complete_r1 <= 1'b0;
    c1_init_calib_complete_r2 <= 1'b0;
end
else begin
    c0_init_calib_complete_r1 <= c0_init_calib_complete;
    c0_init_calib_complete_r2 <= c0_init_calib_complete_r1;
    c1_init_calib_complete_r1 <= c1_init_calib_complete;
    c1_init_calib_complete_r2 <= c1_init_calib_complete_r1;
end

assign ddr4_calib_complete = c0_init_calib_complete_r2 & c1_init_calib_complete_r2;


mem_inf mem_inf_inst(
.axi_clk(aclk),
.aresetn(ddr4_calib_complete),
.sys_rst(sys_reset),

//ddr4 pins
//SODIMM 0
.c0_ddr4_adr(c0_ddr4_adr),                                // output wire [16 : 0] c0_ddr4_adr
.c0_ddr4_ba(c0_ddr4_ba),                                  // output wire [1 : 0] c0_ddr4_ba
.c0_ddr4_cke(c0_ddr4_cke),                                // output wire [0 : 0] c0_ddr4_cke
.c0_ddr4_cs_n(c0_ddr4_cs_n),                              // output wire [0 : 0] c0_ddr4_cs_n
.c0_ddr4_dm_dbi_n(c0_ddr4_dm_dbi_n),                      // inout wire [8 : 0] c0_ddr4_dm_dbi_n
.c0_ddr4_dq(c0_ddr4_dq),                                  // inout wire [71 : 0] c0_ddr4_dq
.c0_ddr4_dqs_c(c0_ddr4_dqs_c),                            // inout wire [8 : 0] c0_ddr4_dqs_c
.c0_ddr4_dqs_t(c0_ddr4_dqs_t),                            // inout wire [8 : 0] c0_ddr4_dqs_t
.c0_ddr4_odt(c0_ddr4_odt),                                // output wire [0 : 0] c0_ddr4_odt
.c0_ddr4_bg(c0_ddr4_bg),                                  // output wire [0 : 0] c0_ddr4_bg
.c0_ddr4_reset_n(c0_ddr4_reset_n),                        // output wire c0_ddr4_reset_n
.c0_ddr4_act_n(c0_ddr4_act_n),                            // output wire c0_ddr4_act_n
.c0_ddr4_ck_c(c0_ddr4_ck_c),                              // output wire [0 : 0] c0_ddr4_ck_c
.c0_ddr4_ck_t(c0_ddr4_ck_t),                              // output wire [0 : 0] c0_ddr4_ck_t

.c0_ui_clk(c0_ui_clk),
.c0_init_calib_complete(c0_init_calib_complete),

  // Differential system clocks
.c0_sys_clk_p(c0_sys_clk_p),
.c0_sys_clk_n(c0_sys_clk_n),
.c1_sys_clk_p(c1_sys_clk_p),
.c1_sys_clk_n(c1_sys_clk_n),

//SODIMM 1
// Inouts
.c1_ddr4_adr(c1_ddr4_adr),                                // output wire [16 : 0] c1_ddr4_adr
.c1_ddr4_ba(c1_ddr4_ba),                                  // output wire [1 : 0] c1_ddr4_ba
.c1_ddr4_cke(c1_ddr4_cke),                                // output wire [0 : 0] c1_ddr4_cke
.c1_ddr4_cs_n(c1_ddr4_cs_n),                              // output wire [0 : 0] c1_ddr4_cs_n
.c1_ddr4_dm_dbi_n(c1_ddr4_dm_dbi_n),                      // inout wire [8 : 0] c1_ddr4_dm_dbi_n
.c1_ddr4_dq(c1_ddr4_dq),                                  // inout wire [71 : 0] c1_ddr4_dq
.c1_ddr4_dqs_c(c1_ddr4_dqs_c),                            // inout wire [8 : 0] c1_ddr4_dqs_c
.c1_ddr4_dqs_t(c1_ddr4_dqs_t),                            // inout wire [8 : 0] c1_ddr4_dqs_t
.c1_ddr4_odt(c1_ddr4_odt),                                // output wire [0 : 0] c1_ddr4_odt
.c1_ddr4_bg(c1_ddr4_bg),                                  // output wire [0 : 0] c1_ddr4_bg
.c1_ddr4_reset_n(c1_ddr4_reset_n),                        // output wire c1_ddr4_reset_n
.c1_ddr4_act_n(c1_ddr4_act_n),                            // output wire c1_ddr4_act_n
.c1_ddr4_ck_c(c1_ddr4_ck_c),                              // output wire [0 : 0] c1_ddr4_ck_c
.c1_ddr4_ck_t(c1_ddr4_ck_t),                              // output wire [0 : 0] c1_ddr4_ck_t

.c1_ui_clk(),
.c1_init_calib_complete(c1_init_calib_complete),

//memory 0 read commands
.s_axis_mem0_read_cmd_tvalid(axis_rxread_cmd_tvalid),
.s_axis_mem0_read_cmd_tready(axis_rxread_cmd_tready),
.s_axis_mem0_read_cmd_tdata(axis_rxread_cmd_tdata),
//memory 0 read status
.m_axis_mem0_read_sts_tvalid(),
.m_axis_mem0_read_sts_tready(1'b1),
.m_axis_mem0_read_sts_tdata(),
//memory 0 read stream
.m_axis_mem0_read_tvalid(axis_rxread_data_tvalid),
.m_axis_mem0_read_tready(axis_rxread_data_tready),
.m_axis_mem0_read_tdata(axis_rxread_data_tdata),
.m_axis_mem0_read_tkeep(axis_rxread_data_tkeep),
.m_axis_mem0_read_tlast(axis_rxread_data_tlast),

//memory 0 write commands
.s_axis_mem0_write_cmd_tvalid(axis_rxwrite_cmd_tvalid),
.s_axis_mem0_write_cmd_tready(axis_rxwrite_cmd_tready),
.s_axis_mem0_write_cmd_tdata(axis_rxwrite_cmd_tdata),
//memory 0 write status
.m_axis_mem0_write_sts_tvalid(axis_rxwrite_sts_tvalid),
.m_axis_mem0_write_sts_tready(axis_rxwrite_sts_tready),
.m_axis_mem0_write_sts_tdata(axis_rxwrite_sts_tdata),
//memory 0 write stream
.s_axis_mem0_write_tvalid(axis_rxwrite_data_tvalid),
.s_axis_mem0_write_tready(axis_rxwrite_data_tready),
.s_axis_mem0_write_tdata(axis_rxwrite_data_tdata),
.s_axis_mem0_write_tkeep(axis_rxwrite_data_tkeep),
.s_axis_mem0_write_tlast(axis_rxwrite_data_tlast),

//memory 1 read commands
.s_axis_mem1_read_cmd_tvalid(axis_txread_cmd_tvalid),
.s_axis_mem1_read_cmd_tready(axis_txread_cmd_tready),
.s_axis_mem1_read_cmd_tdata(axis_txread_cmd_tdata),
//memory 1 read status
.m_axis_mem1_read_sts_tvalid(),
.m_axis_mem1_read_sts_tready(1'b1),
.m_axis_mem1_read_sts_tdata(),
//memory 1 read stream
.m_axis_mem1_read_tvalid(axis_txread_data_tvalid),
.m_axis_mem1_read_tready(axis_txread_data_tready),
.m_axis_mem1_read_tdata(axis_txread_data_tdata),
.m_axis_mem1_read_tkeep(axis_txread_data_tkeep),
.m_axis_mem1_read_tlast(axis_txread_data_tlast),

//memory 1 write commands
.s_axis_mem1_write_cmd_tvalid(axis_txwrite_cmd_tvalid),
.s_axis_mem1_write_cmd_tready(axis_txwrite_cmd_tready),
.s_axis_mem1_write_cmd_tdata(axis_txwrite_cmd_tdata),
//memory 1 write status
.m_axis_mem1_write_sts_tvalid(axis_txwrite_sts_tvalid),
.m_axis_mem1_write_sts_tready(axis_txwrite_sts_tready),
.m_axis_mem1_write_sts_tdata(axis_txwrite_sts_tdata),
//memory 1 write stream
.s_axis_mem1_write_tvalid(axis_txwrite_data_tvalid),
.s_axis_mem1_write_tready(axis_txwrite_data_tready),
.s_axis_mem1_write_tdata(axis_txwrite_data_tdata),
.s_axis_mem1_write_tkeep(axis_txwrite_data_tkeep),
.s_axis_mem1_write_tlast(axis_txwrite_data_tlast)

);

endmodule

`default_nettype wire
