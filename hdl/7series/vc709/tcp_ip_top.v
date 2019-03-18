`timescale 1ns / 1ps
`default_nettype none

`define USE_DDR
`define ECHO_SERVER
//`define IPERF_CLIENT
`define UDP

module tcp_ip_top(
    // 212MHz clock input
input wire                          sys_clk_p,
input wire                         sys_clk_n,
// 200MHz reference clock input
input wire                         clk_ref_p,
input wire                         clk_ref_n,

//-SI5324 I2C programming interface
inout wire                         i2c_clk,
inout wire                         i2c_data,
output wire                         i2c_mux_rst_n,
output wire                        si5324_rst_n,
// 156.25 MHz clock in
input wire                         xphy_refclk_p,
input wire                         xphy_refclk_n,

output wire                         xphy0_txp,
output wire                         xphy0_txn,
input wire                          xphy0_rxp,
input wire                          xphy0_rxn,

input wire         button_center,
input wire         button_north,
input wire         button_east,
input wire         button_south,
input wire         button_west,

/*  output wire                         xphy1_txp,
output wire                         xphy1_txn,
input wire                          xphy1_rxp,
input wire                          xphy1_rxn,

output wire                         xphy2_txp,
output wire                         xphy2_txn,
input wire                          xphy2_rxp,
input wire                          xphy2_rxn,

output wire                         xphy3_txp,
output wire                         xphy3_txn,
input wire                          xphy3_rxp,
input wire                          xphy3_rxn,*/

output wire[3:0] sfp_tx_disable,

  // Connection to SODIMM-A
  output wire [15:0]                  c0_ddr3_addr,             
  output wire [2:0]                   c0_ddr3_ba,               
  output wire                         c0_ddr3_cas_n,            
  output wire                         c0_ddr3_ck_p,               
  output wire                         c0_ddr3_ck_n,             
  output wire                         c0_ddr3_cke,              
  output wire                         c0_ddr3_cs_n,             
  output wire [7:0]                   c0_ddr3_dm,               
  inout  wire [63:0]                  c0_ddr3_dq,               
  inout  wire [7:0]                   c0_ddr3_dqs_p,              
  inout  wire [7:0]                   c0_ddr3_dqs_n,            
  output wire                         c0_ddr3_odt,              
  output wire                         c0_ddr3_ras_n,            
  output wire                         c0_ddr3_reset_n,          
  output wire                         c0_ddr3_we_n,             

    // Connection to SODIMM-B
  output wire [15:0]                  c1_ddr3_addr,             
  output wire [2:0]                   c1_ddr3_ba,               
  output wire                         c1_ddr3_cas_n,            
  output wire                         c1_ddr3_ck_p,               
  output wire                         c1_ddr3_ck_n,             
  output wire                         c1_ddr3_cke,              
  output wire                         c1_ddr3_cs_n,             
  output wire [7:0]                   c1_ddr3_dm,               
  inout  wire[63:0]                  c1_ddr3_dq,               
  inout  wire[7:0]                   c1_ddr3_dqs_p,              
  inout  wire[7:0]                   c1_ddr3_dqs_n,            
  output wire                         c1_ddr3_odt,              
  output wire                         c1_ddr3_ras_n,            
  output wire                         c1_ddr3_reset_n,          
  output wire                         c1_ddr3_we_n,
  
  input wire                         sys_rst_i,
  
  // UART
  //input wire                          RxD,
  //output wire                         TxD,
  input wire   [7:0]                  gpio_switch,
  output wire [7:0]                  led 
    );

wire aclk;
wire aresetn;
wire clk_ref_200;
wire reset;
wire network_init;    
reg button_east_reg;
reg[7:0] led_reg;
wire[7:0] led_out;

assign reset = button_east_reg;
assign aresetn = network_init;

always @(posedge aclk) begin
    button_east_reg <= button_east;
    led_reg <= led_out;
end
assign led = led_reg;

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


/*
 * 10G Network Interface Module
 */
vc709_10g_interface n10g_interface_inst
 (
 .clk_ref_p(clk_ref_p),
 .clk_ref_n(clk_ref_n),
 .reset(reset),
 .aresetn(aresetn),
 
 .i2c_clk(i2c_clk),
 .i2c_data(i2c_data),
 .i2c_mux_rst_n(i2c_mux_rst_n),
 .si5324_rst_n(si5324_rst_n),
 
 .xphy_refclk_p(xphy_refclk_p),
 .xphy_refclk_n(xphy_refclk_n),
 
 .xphy0_txp(xphy0_txp),
 .xphy0_txn(xphy0_txn),
 .xphy0_rxp(xphy0_rxp),
 .xphy0_rxn(xphy0_rxn),
 
 
 //master
 .axis_i_0_tdata(axis_net_rx_data_tdata),
 .axis_i_0_tvalid(axis_net_rx_data_tvalid),
 .axis_i_0_tlast(axis_net_rx_data_tlast),
 .axis_i_0_tuser(),
 .axis_i_0_tkeep(axis_net_rx_data_tkeep),
 .axis_i_0_tready(axis_net_rx_data_tready),
     
 //slave
 .axis_o_0_tdata(axis_net_tx_data_tdata),
 .axis_o_0_tvalid(axis_net_tx_data_tvalid),
 .axis_o_0_tlast(axis_net_tx_data_tlast),
 .axis_o_0_tuser(0),
 .axis_o_0_tkeep(axis_net_tx_data_tkeep),
 .axis_o_0_tready(axis_net_tx_data_tready),
     
 .sfp_tx_disable(sfp_tx_disable),
 .clk156_out(aclk),
 .clk_ref_200_out(clk_ref_200),
 .network_reset_done(network_init),
 .led(led_out)
 
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

 .board_number(gpio_switch[3:0]),
 .subnet_number(gpio_switch[5:4])
 
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
wire clk233;
wire clk200, clk200_i;
wire c0_init_calib_complete;
wire c1_init_calib_complete;
wire ddr3_calib_complete, init_calib_complete;
//registers for crossing clock domains (from 233MHz to 156.25MHz)
reg c0_init_calib_complete_r1, c0_init_calib_complete_r2;
reg c1_init_calib_complete_r1, c1_init_calib_complete_r2;

//- 212MHz differential clock for 1866Mbps DDR3 controller
IBUFGDS #(
 .DIFF_TERM    ("TRUE"),
 .IBUF_LOW_PWR ("FALSE")
) clk_233_ibufg (
 .I            (sys_clk_p),
 .IB           (sys_clk_n),
 .O            (clk233)
);

// sys_rst
wire sys_rst;
IBUF clk_212_bufg
 (
     .I                              (sys_rst_i),
     .O                              (sys_rst) 
 );


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

assign ddr3_calib_complete = c0_init_calib_complete_r2 & c1_init_calib_complete_r2;
assign init_calib_complete = ddr3_calib_complete;


mem_inf  mem_inf_inst(
.clk156_25(aclk),
//.reset233_n(reset233_n), //active low reset signal for 233MHz clock domain
.reset156_25_n(ddr3_calib_complete),
.clk212(clk233),
.clk200(clk_ref_200),
.sys_rst(sys_rst),

//ddr3 pins
//SODIMM 0
// Inouts
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
.c0_ui_clk(),
.c0_init_calib_complete(c0_init_calib_complete),

//SODIMM 1
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
