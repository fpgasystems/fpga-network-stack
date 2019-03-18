`timescale 1ns / 1ps
`default_nettype none


module network_module
(
    input wire          dclk,
    output wire         net_clk,
    input wire          sys_reset,
    input wire          aresetn,
    output wire         network_init_done,
    
    input wire          gt_refclk_p,
    input wire          gt_refclk_n,
    
	input  wire [0:0] gt_rxp_in,
	input  wire [0:0] gt_rxn_in,
	output wire [0:0] gt_txp_out,
	output wire [0:0] gt_txn_out,

	output wire [0:0] user_rx_reset,
	output wire [0:0] user_tx_reset,
	output wire        gtpowergood_out,
	
	//Axi Stream Interface
    input wire [63:0]     s_axis_0_tdata,
    input wire            s_axis_0_tvalid,
    input wire            s_axis_0_tlast,
    input wire [7:0]      s_axis_0_tkeep,
    output wire           s_axis_0_tready,
    
    output wire           m_axis_0_tvalid,
    input wire            m_axis_0_tready,
    output wire [63:0]    m_axis_0_tdata,
    output wire [7:0]     m_axis_0_tkeep,
    output wire           m_axis_0_tlast
    
    /*input wire [63:0]     s_axis_1_tdata,
    input wire            s_axis_1_tvalid,
    input wire            s_axis_1_tlast,
    input wire [7:0]      s_axis_1_tkeep,
    output wire           s_axis_1_tready,
    
    output wire           m_axis_1_tvalid,
    input wire            m_axis_1_tready,
    output wire [63:0]    m_axis_1_tdata,
    output wire [7:0]     m_axis_1_tkeep,
    output wire           m_axis_1_tlast,
    
    input wire [63:0]     s_axis_2_tdata,
    input wire            s_axis_2_tvalid,
    input wire            s_axis_2_tlast,
    input wire [7:0]      s_axis_2_tkeep,
    output wire           s_axis_2_tready,
    
    output wire           m_axis_2_tvalid,
    input wire            m_axis_2_tready,
    output wire [63:0]    m_axis_2_tdata,
    output wire [7:0]     m_axis_2_tkeep,
    output wire           m_axis_2_tlast,
    
    input wire [63:0]     s_axis_3_tdata,
    input wire            s_axis_3_tvalid,
    input wire            s_axis_3_tlast,
    input wire [7:0]      s_axis_3_tkeep,
    output wire           s_axis_3_tready,
    
    output wire           m_axis_3_tvalid,
    input wire            m_axis_3_tready,
    output wire [63:0]    m_axis_3_tdata,
    output wire [7:0]     m_axis_3_tkeep,
    output wire           m_axis_3_tlast*/
);

reg core_reset_tmp;
reg core_reset;
wire [0:0] rx_core_clk;
wire [0:0] rx_clk_out;
wire [0:0] tx_clk_out;
assign rx_core_clk[0] = tx_clk_out[0];
//assign rx_core_clk[1] = tx_clk_out[1];
//assign rx_core_clk[2] = tx_clk_out[2];
//assign rx_core_clk[3] = tx_clk_out[3];
assign net_clk = tx_clk_out[0];

wire [0:0] gtpowergood;
wire [2:0] gt_loopback_in [0:0];
assign gt_loopback_in[0] = 2'h00;
//assign gt_loopback_in[1] = 2'h00;
//assign gt_loopback_in[2] = 2'h00;
//assign gt_loopback_in[3] = 2'h00;

always @(posedge sys_reset or posedge net_clk) begin 
   if (sys_reset) begin
      core_reset_tmp <= 1'b1;
      core_reset <= 1'b1;
   end
   else begin
      //Hold core in reset until everything is ready
      core_reset_tmp <= sys_reset | user_tx_reset[0] | user_rx_reset[0];// | user_tx_reset[1] | user_rx_reset[1] | user_tx_reset[2] | user_rx_reset[2] | user_tx_reset[3] | user_rx_reset[3];
      core_reset <= core_reset_tmp;
   end
end
assign network_init_done = ~core_reset;

assign gtpowergood_out = gtpowergood[0];// & gtpowergood[1] & gtpowergood[2] & gtpowergood[3];

/*
 * RX
 */
wire [3:0]  axis_rxif_to_fifo_tvalid;
wire [3:0]  axis_rxif_to_fifo_tready;
wire [63:0] axis_rxif_to_fifo_tdata [3:0];
wire [7:0]  axis_rxif_to_fifo_tkeep [3:0];
wire [3:0]  axis_rxif_to_fifo_tlast;
wire [3:0]  axis_rxif_to_fifo_tuser;

//// RX_0 User Interface Signals
wire [3:0]  rx_axis_tvalid;
wire [63:0] rx_axis_tdata  [3:0];
wire [7:0]  rx_axis_tkeep  [3:0];
wire [3:0]  rx_axis_tlast;
wire [3:0]  rx_axis_tuser;
wire [55:0] rx_preambleout [3:0];

wire [3:0]  m_axis_tvalid;
wire [3:0]  m_axis_tready;
wire [63:0] m_axis_tdata  [3:0];
wire [7:0]  m_axis_tkeep  [3:0];
wire [3:0]  m_axis_tlast;
wire [3:0]  m_axis_tuser;

// RX  Control Signals
wire ctl_rx_test_pattern;
wire ctl_rx_test_pattern_enable;
wire ctl_rx_data_pattern_select;
wire ctl_rx_enable;
wire ctl_rx_delete_fcs;
wire ctl_rx_ignore_fcs;
wire [14:0] ctl_rx_max_packet_len;
wire [7:0] ctl_rx_min_packet_len;
wire ctl_rx_custom_preamble_enable;
wire ctl_rx_check_sfd;
wire ctl_rx_check_preamble;
wire ctl_rx_process_lfi;
wire ctl_rx_force_resync;

assign ctl_rx_enable              = 1'b1;
assign ctl_rx_check_preamble      = 1'b1;
assign ctl_rx_check_sfd           = 1'b1;
assign ctl_rx_force_resync        = 1'b0;
assign ctl_rx_delete_fcs          = 1'b1;
assign ctl_rx_ignore_fcs          = 1'b0;
assign ctl_rx_process_lfi         = 1'b0;
assign ctl_rx_test_pattern        = 1'b0;
assign ctl_rx_test_pattern_enable = 1'b0;
assign ctl_rx_data_pattern_select = 1'b0;
assign ctl_rx_max_packet_len      = 15'd1536;
assign ctl_rx_min_packet_len      = 15'd42;
assign ctl_rx_custom_preamble_enable = 1'b0;

/*
 * TX
 */
wire [3:0]  axis_tx_fifo_to_txif_tready;
wire [3:0]  axis_tx_fifo_to_txif_tvalid;
wire [63:0] axis_tx_fifo_to_txif_tdata [3:0];
wire [7:0]  axis_tx_fifo_to_txif_tkeep [3:0];
wire [3:0]  axis_tx_fifo_to_txif_tlast;
wire [3:0]  axis_tx_fifo_to_txif_tuser;


wire [3:0]  axis_tx_padding_to_fifo_tvalid;
wire [3:0]  axis_tx_padding_to_fifo_tready;
wire [63:0] axis_tx_padding_to_fifo_tdata [3:0];
wire [7:0]  axis_tx_padding_to_fifo_tkeep [3:0];
wire [3:0]  axis_tx_padding_to_fifo_tlast;

//// TX Data Signals
wire [3:0]  tx_axis_tvalid;
wire [3:0]  tx_axis_tready;
wire [63:0] tx_axis_tdata  [3:0];
wire [7:0]  tx_axis_tkeep  [3:0];
wire [3:0]  tx_axis_tlast;
wire [3:0]  tx_axis_tuser;

wire [3:0]  s_axis_tvalid;
wire [3:0]  s_axis_tready;
wire [63:0] s_axis_tdata  [3:0];
wire [7:0]  s_axis_tkeep  [3:0];
wire [3:0]  s_axis_tlast;
wire [3:0]  s_axis_tuser;


//// TX_0 Control Signals
wire ctl_tx_test_pattern;
wire ctl_tx_test_pattern_enable;
wire ctl_tx_test_pattern_select;
wire ctl_tx_data_pattern_select;
wire [57:0] ctl_tx_test_pattern_seed_a;
wire [57:0] ctl_tx_test_pattern_seed_b;
wire ctl_tx_enable;
wire ctl_tx_fcs_ins_enable;
wire [3:0] ctl_tx_ipg_value;
wire ctl_tx_send_lfi;
wire ctl_tx_send_rfi;
wire ctl_tx_send_idle;
wire ctl_tx_custom_preamble_enable;
wire ctl_tx_ignore_fcs;

assign ctl_tx_enable              = 1'b1;
assign ctl_tx_send_rfi            = 1'b0;
assign ctl_tx_send_lfi            = 1'b0;
assign ctl_tx_send_idle           = 1'b0;
assign ctl_tx_fcs_ins_enable      = 1'b1;
assign ctl_tx_ignore_fcs          = 1'b0;
assign ctl_tx_test_pattern        = 1'b0;
assign ctl_tx_test_pattern_enable = 1'b0;
assign ctl_tx_data_pattern_select = 1'b0;
assign ctl_tx_test_pattern_select = 1'b0;
assign ctl_tx_test_pattern_seed_a = 58'h0;
assign ctl_tx_test_pattern_seed_b = 58'h0;
assign ctl_tx_custom_preamble_enable = 1'b0;
assign ctl_tx_ipg_value           = 4'd12;

wire [3:0] gtwiz_reset_tx_datapath;
wire [3:0] gtwiz_reset_rx_datapath;
assign gtwiz_reset_tx_datapath = 4'b0000;
assign gtwiz_reset_rx_datapath = 4'b0000;
wire [2:0] txoutclksel_in [3:0];
wire [2:0] rxoutclksel_in [3:0];

assign txoutclksel_in[0] = 3'b101;    // this value should not be changed as per gtwizard 
assign rxoutclksel_in[0] = 3'b101;    // this value should not be changed as per gtwizard
assign txoutclksel_in[1] = 3'b101;    // this value should not be changed as per gtwizard 
assign rxoutclksel_in[1] = 3'b101;    // this value should not be changed as per gtwizard
assign txoutclksel_in[2] = 3'b101;    // this value should not be changed as per gtwizard 
assign rxoutclksel_in[2] = 3'b101;    // this value should not be changed as per gtwizard
assign txoutclksel_in[3] = 3'b101;    // this value should not be changed as per gtwizard 
assign rxoutclksel_in[3] = 3'b101;    // this value should not be changed as per gtwizard

wire  gt_refclk_out;



ethernet_10g_ip ethernet_inst
(
    .gt_refclk_p (gt_refclk_p),
    .gt_refclk_n (gt_refclk_n),
    .gt_refclk_out (gt_refclk_out),
    .sys_reset (sys_reset),
    .dclk (dclk),


    .gt_rxp_in_0(gt_rxp_in[0]),
    .gt_rxn_in_0(gt_rxn_in[0]),
    .gt_txp_out_0(gt_txp_out[0]),
    .gt_txn_out_0(gt_txn_out[0]),
    
    .tx_clk_out_0(tx_clk_out[0]),
    .rx_core_clk_0(rx_core_clk[0]),
    .rx_clk_out_0 (rx_clk_out[0]),
    .gt_loopback_in_0 (gt_loopback_in[0]),
    .rx_reset_0 (1'b0),
    .user_rx_reset_0 (user_rx_reset[0]),
    .rxrecclkout_0 (),
    .tx_reset_0 (1'b0),
    .user_tx_reset_0 (user_tx_reset[0]),
    .gtwiz_reset_tx_datapath_0 (gtwiz_reset_tx_datapath[0]),
    .gtwiz_reset_rx_datapath_0 (gtwiz_reset_rx_datapath[0]),
    .gtpowergood_out_0 (gtpowergood[0]),
    .txoutclksel_in_0 (txoutclksel_in[0]),
    .rxoutclksel_in_0 (rxoutclksel_in[0]),
    
    
    
    /*.gt_rxp_in_1(gt_rxp_in[1]),
    .gt_rxn_in_1(gt_rxn_in[1]),
    .gt_txp_out_1(gt_txp_out[1]),
    .gt_txn_out_1(gt_txn_out[1]),
    
    .tx_clk_out_1(tx_clk_out[1]),
    .rx_core_clk_1(rx_core_clk[1]),
    .rx_clk_out_1 (rx_clk_out[1]),
    .gt_loopback_in_1 (gt_loopback_in[1]),
    .rx_reset_1 (1'b0),
    .user_rx_reset_1 (user_rx_reset[1]),
    .rxrecclkout_1 (),
    .tx_reset_1 (1'b0),
    .user_tx_reset_1 (user_tx_reset[1]),
    .gtwiz_reset_tx_datapath_1 (gtwiz_reset_tx_datapath[1]),
    .gtwiz_reset_rx_datapath_1 (gtwiz_reset_rx_datapath[1]),
    .gtpowergood_out_1 (gtpowergood[1]),
    .txoutclksel_in_1 (txoutclksel_in[1]),
    .rxoutclksel_in_1 (rxoutclksel_in[1]),
    
    .gt_rxp_in_2(gt_rxp_in[2]),
    .gt_rxn_in_2(gt_rxn_in[2]),
    .gt_txp_out_2(gt_txp_out[2]),
    .gt_txn_out_2(gt_txn_out[2]),
    
    .tx_clk_out_2(tx_clk_out[2]),
    .rx_core_clk_2(rx_core_clk[2]),
    .rx_clk_out_2 (rx_clk_out[2]),
    .gt_loopback_in_2 (gt_loopback_in[2]),
    .rx_reset_2 (1'b0),
    .user_rx_reset_2 (user_rx_reset[2]),
    .rxrecclkout_2 (),
    .tx_reset_2 (1'b0),
    .user_tx_reset_2 (user_tx_reset[2]),
    .gtwiz_reset_tx_datapath_2 (gtwiz_reset_tx_datapath[2]),
    .gtwiz_reset_rx_datapath_2 (gtwiz_reset_rx_datapath[2]),
    .gtpowergood_out_2 (gtpowergood[2]),
    .txoutclksel_in_2 (txoutclksel_in[2]),
    .rxoutclksel_in_2 (rxoutclksel_in[2]),
    
    .gt_rxp_in_3(gt_rxp_in[3]),
    .gt_rxn_in_3(gt_rxn_in[3]),
    .gt_txp_out_3(gt_txp_out[3]),
    .gt_txn_out_3(gt_txn_out[3]),
    
    .tx_clk_out_3(tx_clk_out[3]),
    .rx_core_clk_3(rx_core_clk[3]),
    .rx_clk_out_3 (rx_clk_out[3]),
    .gt_loopback_in_3 (gt_loopback_in[3]),
    .rx_reset_3 (1'b0),
    .user_rx_reset_3 (user_rx_reset[3]),
    .rxrecclkout_3 (),
    .tx_reset_3 (1'b0),
    .user_tx_reset_3 (user_tx_reset[3]),
    .gtwiz_reset_tx_datapath_3 (gtwiz_reset_tx_datapath[3]),
    .gtwiz_reset_rx_datapath_3 (gtwiz_reset_rx_datapath[3]),
    .gtpowergood_out_3 (gtpowergood[3]),
    .txoutclksel_in_3 (txoutclksel_in[3]),
    .rxoutclksel_in_3 (rxoutclksel_in[3]),*/


    // RX Data Signals
    .rx_axis_tvalid_0 (rx_axis_tvalid[0]),
    .rx_axis_tdata_0 (rx_axis_tdata[0]),
    .rx_axis_tlast_0 (rx_axis_tlast[0]),
    .rx_axis_tkeep_0 (rx_axis_tkeep[0]),
    .rx_axis_tuser_0 (rx_axis_tuser[0]),
    .rx_preambleout_0 (rx_preambleout[0]),
    
    /*.rx_axis_tvalid_1 (rx_axis_tvalid[1]),
    .rx_axis_tdata_1 (rx_axis_tdata[1]),
    .rx_axis_tlast_1 (rx_axis_tlast[1]),
    .rx_axis_tkeep_1 (rx_axis_tkeep[1]),
    .rx_axis_tuser_1 (rx_axis_tuser[1]),
    .rx_preambleout_1 (rx_preambleout[1]),
    
    .rx_axis_tvalid_2 (rx_axis_tvalid[2]),
    .rx_axis_tdata_2 (rx_axis_tdata[2]),
    .rx_axis_tlast_2 (rx_axis_tlast[2]),
    .rx_axis_tkeep_2 (rx_axis_tkeep[2]),
    .rx_axis_tuser_2 (rx_axis_tuser[2]),
    .rx_preambleout_2 (rx_preambleout[2]),
    
    .rx_axis_tvalid_3 (rx_axis_tvalid[3]),
    .rx_axis_tdata_3 (rx_axis_tdata[3]),
    .rx_axis_tlast_3 (rx_axis_tlast[3]),
    .rx_axis_tkeep_3 (rx_axis_tkeep[3]),
    .rx_axis_tuser_3 (rx_axis_tuser[3]),
    .rx_preambleout_3 (rx_preambleout[3]),*/
    
    // TX Data Signals
    .tx_axis_tready_0 (tx_axis_tready[0]),
    .tx_axis_tvalid_0 (tx_axis_tvalid[0]),
    .tx_axis_tdata_0 (tx_axis_tdata[0]),
    .tx_axis_tlast_0 (tx_axis_tlast[0]),
    .tx_axis_tkeep_0 (tx_axis_tkeep[0]),
    .tx_axis_tuser_0 (tx_axis_tuser[0]),
    .tx_unfout_0 (),
    .tx_preamblein_0 (0),
    
    /*.tx_axis_tready_1 (tx_axis_tready[1]),
    .tx_axis_tvalid_1 (tx_axis_tvalid[1]),
    .tx_axis_tdata_1 (tx_axis_tdata[1]),
    .tx_axis_tlast_1 (tx_axis_tlast[1]),
    .tx_axis_tkeep_1 (tx_axis_tkeep[1]),
    .tx_axis_tuser_1 (tx_axis_tuser[1]),
    .tx_unfout_1 (),
    .tx_preamblein_1 (0),
    
    .tx_axis_tready_2 (tx_axis_tready[2]),
    .tx_axis_tvalid_2 (tx_axis_tvalid[2]),
    .tx_axis_tdata_2 (tx_axis_tdata[2]),
    .tx_axis_tlast_2 (tx_axis_tlast[2]),
    .tx_axis_tkeep_2 (tx_axis_tkeep[2]),
    .tx_axis_tuser_2 (tx_axis_tuser[2]),
    .tx_unfout_2 (),
    .tx_preamblein_2 (0),
    
    .tx_axis_tready_3 (tx_axis_tready[3]),
    .tx_axis_tvalid_3 (tx_axis_tvalid[3]),
    .tx_axis_tdata_3 (tx_axis_tdata[3]),
    .tx_axis_tlast_3 (tx_axis_tlast[3]),
    .tx_axis_tkeep_3 (tx_axis_tkeep[3]),
    .tx_axis_tuser_3 (tx_axis_tuser[3]),
    .tx_unfout_3 (),
    .tx_preamblein_3 (0),*/


    //RX Control Signals
    .ctl_rx_test_pattern_0 (ctl_rx_test_pattern),
    .ctl_rx_test_pattern_enable_0 (ctl_rx_test_pattern_enable),
    .ctl_rx_data_pattern_select_0 (ctl_rx_data_pattern_select),
    .ctl_rx_enable_0 (ctl_rx_enable),
    .ctl_rx_delete_fcs_0 (ctl_rx_delete_fcs),
    .ctl_rx_ignore_fcs_0 (ctl_rx_ignore_fcs),
    .ctl_rx_max_packet_len_0 (ctl_rx_max_packet_len),
    .ctl_rx_min_packet_len_0 (ctl_rx_min_packet_len),
    .ctl_rx_custom_preamble_enable_0 (ctl_rx_custom_preamble_enable),
    .ctl_rx_check_sfd_0 (ctl_rx_check_sfd),
    .ctl_rx_check_preamble_0 (ctl_rx_check_preamble),
    .ctl_rx_process_lfi_0 (ctl_rx_process_lfi),
    .ctl_rx_force_resync_0 (ctl_rx_force_resync),

    /*.ctl_rx_test_pattern_1 (ctl_rx_test_pattern),
    .ctl_rx_test_pattern_enable_1 (ctl_rx_test_pattern_enable),
    .ctl_rx_data_pattern_select_1 (ctl_rx_data_pattern_select),
    .ctl_rx_enable_1 (ctl_rx_enable),
    .ctl_rx_delete_fcs_1 (ctl_rx_delete_fcs),
    .ctl_rx_ignore_fcs_1 (ctl_rx_ignore_fcs),
    .ctl_rx_max_packet_len_1 (ctl_rx_max_packet_len),
    .ctl_rx_min_packet_len_1 (ctl_rx_min_packet_len),
    .ctl_rx_custom_preamble_enable_1 (ctl_rx_custom_preamble_enable),
    .ctl_rx_check_sfd_1 (ctl_rx_check_sfd),
    .ctl_rx_check_preamble_1 (ctl_rx_check_preamble),
    .ctl_rx_process_lfi_1 (ctl_rx_process_lfi),
    .ctl_rx_force_resync_1 (ctl_rx_force_resync),
    
    .ctl_rx_test_pattern_2 (ctl_rx_test_pattern),
    .ctl_rx_test_pattern_enable_2 (ctl_rx_test_pattern_enable),
    .ctl_rx_data_pattern_select_2 (ctl_rx_data_pattern_select),
    .ctl_rx_enable_2 (ctl_rx_enable),
    .ctl_rx_delete_fcs_2 (ctl_rx_delete_fcs),
    .ctl_rx_ignore_fcs_2 (ctl_rx_ignore_fcs),
    .ctl_rx_max_packet_len_2 (ctl_rx_max_packet_len),
    .ctl_rx_min_packet_len_2 (ctl_rx_min_packet_len),
    .ctl_rx_custom_preamble_enable_2 (ctl_rx_custom_preamble_enable),
    .ctl_rx_check_sfd_2 (ctl_rx_check_sfd),
    .ctl_rx_check_preamble_2 (ctl_rx_check_preamble),
    .ctl_rx_process_lfi_2 (ctl_rx_process_lfi),
    .ctl_rx_force_resync_2 (ctl_rx_force_resync),
    
    .ctl_rx_test_pattern_3 (ctl_rx_test_pattern),
    .ctl_rx_test_pattern_enable_3 (ctl_rx_test_pattern_enable),
    .ctl_rx_data_pattern_select_3 (ctl_rx_data_pattern_select),
    .ctl_rx_enable_3 (ctl_rx_enable),
    .ctl_rx_delete_fcs_3 (ctl_rx_delete_fcs),
    .ctl_rx_ignore_fcs_3 (ctl_rx_ignore_fcs),
    .ctl_rx_max_packet_len_3 (ctl_rx_max_packet_len),
    .ctl_rx_min_packet_len_3 (ctl_rx_min_packet_len),
    .ctl_rx_custom_preamble_enable_3 (ctl_rx_custom_preamble_enable),
    .ctl_rx_check_sfd_3 (ctl_rx_check_sfd),
    .ctl_rx_check_preamble_3 (ctl_rx_check_preamble),
    .ctl_rx_process_lfi_3 (ctl_rx_process_lfi),
    .ctl_rx_force_resync_3 (ctl_rx_force_resync),*/



    //RX Stats Signals
    .stat_rx_block_lock_0 (),
    .stat_rx_framing_err_valid_0 (),
    .stat_rx_framing_err_0 (),
    .stat_rx_hi_ber_0 (),
    .stat_rx_valid_ctrl_code_0 (),
    .stat_rx_bad_code_0 (),
    .stat_rx_total_packets_0 (),
    .stat_rx_total_good_packets_0 (),
    .stat_rx_total_bytes_0 (),
    .stat_rx_total_good_bytes_0 (),
    .stat_rx_packet_small_0 (),
    .stat_rx_jabber_0 (),
    .stat_rx_packet_large_0 (),
    .stat_rx_oversize_0 (),
    .stat_rx_undersize_0 (),
    .stat_rx_toolong_0 (),
    .stat_rx_fragment_0 (),
    .stat_rx_packet_64_bytes_0 (),
    .stat_rx_packet_65_127_bytes_0 (),
    .stat_rx_packet_128_255_bytes_0 (),
    .stat_rx_packet_256_511_bytes_0 (),
    .stat_rx_packet_512_1023_bytes_0 (),
    .stat_rx_packet_1024_1518_bytes_0 (),
    .stat_rx_packet_1519_1522_bytes_0 (),
    .stat_rx_packet_1523_1548_bytes_0 (),
    .stat_rx_bad_fcs_0 (),
    .stat_rx_packet_bad_fcs_0 (),
    .stat_rx_stomped_fcs_0 (),
    .stat_rx_packet_1549_2047_bytes_0 (),
    .stat_rx_packet_2048_4095_bytes_0 (),
    .stat_rx_packet_4096_8191_bytes_0 (),
    .stat_rx_packet_8192_9215_bytes_0 (),
    .stat_rx_bad_preamble_0 (),
    .stat_rx_bad_sfd_0 (),
    .stat_rx_got_signal_os_0 (),
    .stat_rx_test_pattern_mismatch_0 (),
    .stat_rx_truncated_0 (),
    .stat_rx_local_fault_0 (),
    .stat_rx_remote_fault_0 (),
    .stat_rx_internal_local_fault_0 (),
    .stat_rx_received_local_fault_0 (),
    .stat_rx_status_0 (),
    
    /*.stat_rx_block_lock_1 (),
    .stat_rx_framing_err_valid_1 (),
    .stat_rx_framing_err_1 (),
    .stat_rx_hi_ber_1 (),
    .stat_rx_valid_ctrl_code_1 (),
    .stat_rx_bad_code_1 (),
    .stat_rx_total_packets_1 (),
    .stat_rx_total_good_packets_1 (),
    .stat_rx_total_bytes_1 (),
    .stat_rx_total_good_bytes_1 (),
    .stat_rx_packet_small_1 (),
    .stat_rx_jabber_1 (),
    .stat_rx_packet_large_1 (),
    .stat_rx_oversize_1 (),
    .stat_rx_undersize_1 (),
    .stat_rx_toolong_1 (),
    .stat_rx_fragment_1 (),
    .stat_rx_packet_64_bytes_1 (),
    .stat_rx_packet_65_127_bytes_1 (),
    .stat_rx_packet_128_255_bytes_1 (),
    .stat_rx_packet_256_511_bytes_1 (),
    .stat_rx_packet_512_1023_bytes_1 (),
    .stat_rx_packet_1024_1518_bytes_1 (),
    .stat_rx_packet_1519_1522_bytes_1 (),
    .stat_rx_packet_1523_1548_bytes_1 (),
    .stat_rx_bad_fcs_1 (),
    .stat_rx_packet_bad_fcs_1 (),
    .stat_rx_stomped_fcs_1 (),
    .stat_rx_packet_1549_2047_bytes_1 (),
    .stat_rx_packet_2048_4095_bytes_1 (),
    .stat_rx_packet_4096_8191_bytes_1 (),
    .stat_rx_packet_8192_9215_bytes_1 (),
    .stat_rx_bad_preamble_1 (),
    .stat_rx_bad_sfd_1 (),
    .stat_rx_got_signal_os_1 (),
    .stat_rx_test_pattern_mismatch_1 (),
    .stat_rx_truncated_1 (),
    .stat_rx_local_fault_1 (),
    .stat_rx_remote_fault_1 (),
    .stat_rx_internal_local_fault_1 (),
    .stat_rx_received_local_fault_1 (),
    .stat_rx_status_1 (),
    
    .stat_rx_block_lock_2 (),
    .stat_rx_framing_err_valid_2 (),
    .stat_rx_framing_err_2 (),
    .stat_rx_hi_ber_2 (),
    .stat_rx_valid_ctrl_code_2 (),
    .stat_rx_bad_code_2 (),
    .stat_rx_total_packets_2 (),
    .stat_rx_total_good_packets_2 (),
    .stat_rx_total_bytes_2 (),
    .stat_rx_total_good_bytes_2 (),
    .stat_rx_packet_small_2 (),
    .stat_rx_jabber_2 (),
    .stat_rx_packet_large_2 (),
    .stat_rx_oversize_2 (),
    .stat_rx_undersize_2 (),
    .stat_rx_toolong_2 (),
    .stat_rx_fragment_2 (),
    .stat_rx_packet_64_bytes_2 (),
    .stat_rx_packet_65_127_bytes_2 (),
    .stat_rx_packet_128_255_bytes_2 (),
    .stat_rx_packet_256_511_bytes_2 (),
    .stat_rx_packet_512_1023_bytes_2 (),
    .stat_rx_packet_1024_1518_bytes_2 (),
    .stat_rx_packet_1519_1522_bytes_2 (),
    .stat_rx_packet_1523_1548_bytes_2 (),
    .stat_rx_bad_fcs_2 (),
    .stat_rx_packet_bad_fcs_2 (),
    .stat_rx_stomped_fcs_2 (),
    .stat_rx_packet_1549_2047_bytes_2 (),
    .stat_rx_packet_2048_4095_bytes_2 (),
    .stat_rx_packet_4096_8191_bytes_2 (),
    .stat_rx_packet_8192_9215_bytes_2 (),
    .stat_rx_bad_preamble_2 (),
    .stat_rx_bad_sfd_2 (),
    .stat_rx_got_signal_os_2 (),
    .stat_rx_test_pattern_mismatch_2 (),
    .stat_rx_truncated_2 (),
    .stat_rx_local_fault_2 (),
    .stat_rx_remote_fault_2 (),
    .stat_rx_internal_local_fault_2 (),
    .stat_rx_received_local_fault_2 (),
    .stat_rx_status_2 (),
    
    .stat_rx_block_lock_3 (),
    .stat_rx_framing_err_valid_3 (),
    .stat_rx_framing_err_3 (),
    .stat_rx_hi_ber_3 (),
    .stat_rx_valid_ctrl_code_3 (),
    .stat_rx_bad_code_3 (),
    .stat_rx_total_packets_3 (),
    .stat_rx_total_good_packets_3 (),
    .stat_rx_total_bytes_3 (),
    .stat_rx_total_good_bytes_3 (),
    .stat_rx_packet_small_3 (),
    .stat_rx_jabber_3 (),
    .stat_rx_packet_large_3 (),
    .stat_rx_oversize_3 (),
    .stat_rx_undersize_3 (),
    .stat_rx_toolong_3 (),
    .stat_rx_fragment_3 (),
    .stat_rx_packet_64_bytes_3 (),
    .stat_rx_packet_65_127_bytes_3 (),
    .stat_rx_packet_128_255_bytes_3 (),
    .stat_rx_packet_256_511_bytes_3 (),
    .stat_rx_packet_512_1023_bytes_3 (),
    .stat_rx_packet_1024_1518_bytes_3 (),
    .stat_rx_packet_1519_1522_bytes_3 (),
    .stat_rx_packet_1523_1548_bytes_3 (),
    .stat_rx_bad_fcs_3 (),
    .stat_rx_packet_bad_fcs_3 (),
    .stat_rx_stomped_fcs_3 (),
    .stat_rx_packet_1549_2047_bytes_3 (),
    .stat_rx_packet_2048_4095_bytes_3 (),
    .stat_rx_packet_4096_8191_bytes_3 (),
    .stat_rx_packet_8192_9215_bytes_3 (),
    .stat_rx_bad_preamble_3 (),
    .stat_rx_bad_sfd_3 (),
    .stat_rx_got_signal_os_3 (),
    .stat_rx_test_pattern_mismatch_3 (),
    .stat_rx_truncated_3 (),
    .stat_rx_local_fault_3 (),
    .stat_rx_remote_fault_3 (),
    .stat_rx_internal_local_fault_3 (),
    .stat_rx_received_local_fault_3 (),
    .stat_rx_status_3 (),*/


    // TX Control Signals
    .ctl_tx_test_pattern_0 (ctl_tx_test_pattern),
    .ctl_tx_test_pattern_enable_0 (ctl_tx_test_pattern_enable),
    .ctl_tx_test_pattern_select_0 (ctl_tx_test_pattern_select),
    .ctl_tx_data_pattern_select_0 (ctl_tx_data_pattern_select),
    .ctl_tx_test_pattern_seed_a_0 (ctl_tx_test_pattern_seed_a),
    .ctl_tx_test_pattern_seed_b_0 (ctl_tx_test_pattern_seed_b),
    .ctl_tx_enable_0 (ctl_tx_enable),
    .ctl_tx_fcs_ins_enable_0 (ctl_tx_fcs_ins_enable),
    .ctl_tx_ipg_value_0 (ctl_tx_ipg_value),
    .ctl_tx_send_lfi_0 (ctl_tx_send_lfi),
    .ctl_tx_send_rfi_0 (ctl_tx_send_rfi),
    .ctl_tx_send_idle_0 (ctl_tx_send_idle),
    .ctl_tx_custom_preamble_enable_0 (ctl_tx_custom_preamble_enable),
    .ctl_tx_ignore_fcs_0 (ctl_tx_ignore_fcs),
    
    /*.ctl_tx_test_pattern_1 (ctl_tx_test_pattern),
    .ctl_tx_test_pattern_enable_1 (ctl_tx_test_pattern_enable),
    .ctl_tx_test_pattern_select_1 (ctl_tx_test_pattern_select),
    .ctl_tx_data_pattern_select_1 (ctl_tx_data_pattern_select),
    .ctl_tx_test_pattern_seed_a_1 (ctl_tx_test_pattern_seed_a),
    .ctl_tx_test_pattern_seed_b_1 (ctl_tx_test_pattern_seed_b),
    .ctl_tx_enable_1 (ctl_tx_enable),
    .ctl_tx_fcs_ins_enable_1 (ctl_tx_fcs_ins_enable),
    .ctl_tx_ipg_value_1 (ctl_tx_ipg_value),
    .ctl_tx_send_lfi_1 (ctl_tx_send_lfi),
    .ctl_tx_send_rfi_1 (ctl_tx_send_rfi),
    .ctl_tx_send_idle_1 (ctl_tx_send_idle),
    .ctl_tx_custom_preamble_enable_1 (ctl_tx_custom_preamble_enable),
    .ctl_tx_ignore_fcs_1 (ctl_tx_ignore_fcs),
    
    .ctl_tx_test_pattern_2 (ctl_tx_test_pattern),
    .ctl_tx_test_pattern_enable_2 (ctl_tx_test_pattern_enable),
    .ctl_tx_test_pattern_select_2 (ctl_tx_test_pattern_select),
    .ctl_tx_data_pattern_select_2 (ctl_tx_data_pattern_select),
    .ctl_tx_test_pattern_seed_a_2 (ctl_tx_test_pattern_seed_a),
    .ctl_tx_test_pattern_seed_b_2 (ctl_tx_test_pattern_seed_b),
    .ctl_tx_enable_2 (ctl_tx_enable),
    .ctl_tx_fcs_ins_enable_2 (ctl_tx_fcs_ins_enable),
    .ctl_tx_ipg_value_2 (ctl_tx_ipg_value),
    .ctl_tx_send_lfi_2 (ctl_tx_send_lfi),
    .ctl_tx_send_rfi_2 (ctl_tx_send_rfi),
    .ctl_tx_send_idle_2 (ctl_tx_send_idle),
    .ctl_tx_custom_preamble_enable_2 (ctl_tx_custom_preamble_enable),
    .ctl_tx_ignore_fcs_2 (ctl_tx_ignore_fcs),
    
    .ctl_tx_test_pattern_3 (ctl_tx_test_pattern),
    .ctl_tx_test_pattern_enable_3 (ctl_tx_test_pattern_enable),
    .ctl_tx_test_pattern_select_3 (ctl_tx_test_pattern_select),
    .ctl_tx_data_pattern_select_3 (ctl_tx_data_pattern_select),
    .ctl_tx_test_pattern_seed_a_3 (ctl_tx_test_pattern_seed_a),
    .ctl_tx_test_pattern_seed_b_3 (ctl_tx_test_pattern_seed_b),
    .ctl_tx_enable_3 (ctl_tx_enable),
    .ctl_tx_fcs_ins_enable_3 (ctl_tx_fcs_ins_enable),
    .ctl_tx_ipg_value_3 (ctl_tx_ipg_value),
    .ctl_tx_send_lfi_3 (ctl_tx_send_lfi),
    .ctl_tx_send_rfi_3 (ctl_tx_send_rfi),
    .ctl_tx_send_idle_3 (ctl_tx_send_idle),
    .ctl_tx_custom_preamble_enable_3 (ctl_tx_custom_preamble_enable),
    .ctl_tx_ignore_fcs_3 (ctl_tx_ignore_fcs),*/


    // TX Stats Signals
    .stat_tx_total_packets_0 (),
    .stat_tx_total_bytes_0 (),
    .stat_tx_total_good_packets_0 (),
    .stat_tx_total_good_bytes_0 (),
    .stat_tx_packet_64_bytes_0 (),
    .stat_tx_packet_65_127_bytes_0 (),
    .stat_tx_packet_128_255_bytes_0 (),
    .stat_tx_packet_256_511_bytes_0 (),
    .stat_tx_packet_512_1023_bytes_0 (),
    .stat_tx_packet_1024_1518_bytes_0 (),
    .stat_tx_packet_1519_1522_bytes_0 (),
    .stat_tx_packet_1523_1548_bytes_0 (),
    .stat_tx_packet_small_0 (),
    .stat_tx_packet_large_0 (),
    .stat_tx_packet_1549_2047_bytes_0 (),
    .stat_tx_packet_2048_4095_bytes_0 (),
    .stat_tx_packet_4096_8191_bytes_0 (),
    .stat_tx_packet_8192_9215_bytes_0 (),
    .stat_tx_bad_fcs_0 (),
    .stat_tx_frame_error_0 (),
    .stat_tx_local_fault_0 ()
    
    /*.stat_tx_total_packets_1 (),
    .stat_tx_total_bytes_1 (),
    .stat_tx_total_good_packets_1 (),
    .stat_tx_total_good_bytes_1 (),
    .stat_tx_packet_64_bytes_1 (),
    .stat_tx_packet_65_127_bytes_1 (),
    .stat_tx_packet_128_255_bytes_1 (),
    .stat_tx_packet_256_511_bytes_1 (),
    .stat_tx_packet_512_1023_bytes_1 (),
    .stat_tx_packet_1024_1518_bytes_1 (),
    .stat_tx_packet_1519_1522_bytes_1 (),
    .stat_tx_packet_1523_1548_bytes_1 (),
    .stat_tx_packet_small_1 (),
    .stat_tx_packet_large_1 (),
    .stat_tx_packet_1549_2047_bytes_1 (),
    .stat_tx_packet_2048_4095_bytes_1 (),
    .stat_tx_packet_4096_8191_bytes_1 (),
    .stat_tx_packet_8192_9215_bytes_1 (),
    .stat_tx_bad_fcs_1 (),
    .stat_tx_frame_error_1 (),
    .stat_tx_local_fault_1 (),
    
    .stat_tx_total_packets_2 (),
    .stat_tx_total_bytes_2 (),
    .stat_tx_total_good_packets_2 (),
    .stat_tx_total_good_bytes_2 (),
    .stat_tx_packet_64_bytes_2 (),
    .stat_tx_packet_65_127_bytes_2 (),
    .stat_tx_packet_128_255_bytes_2 (),
    .stat_tx_packet_256_511_bytes_2 (),
    .stat_tx_packet_512_1023_bytes_2 (),
    .stat_tx_packet_1024_1518_bytes_2 (),
    .stat_tx_packet_1519_1522_bytes_2 (),
    .stat_tx_packet_1523_1548_bytes_2 (),
    .stat_tx_packet_small_2 (),
    .stat_tx_packet_large_2 (),
    .stat_tx_packet_1549_2047_bytes_2 (),
    .stat_tx_packet_2048_4095_bytes_2 (),
    .stat_tx_packet_4096_8191_bytes_2 (),
    .stat_tx_packet_8192_9215_bytes_2 (),
    .stat_tx_bad_fcs_2 (),
    .stat_tx_frame_error_2 (),
    .stat_tx_local_fault_2 (),
   
    .stat_tx_total_packets_3 (),
    .stat_tx_total_bytes_3 (),
    .stat_tx_total_good_packets_3 (),
    .stat_tx_total_good_bytes_3 (),
    .stat_tx_packet_64_bytes_3 (),
    .stat_tx_packet_65_127_bytes_3 (),
    .stat_tx_packet_128_255_bytes_3 (),
    .stat_tx_packet_256_511_bytes_3 (),
    .stat_tx_packet_512_1023_bytes_3 (),
    .stat_tx_packet_1024_1518_bytes_3 (),
    .stat_tx_packet_1519_1522_bytes_3 (),
    .stat_tx_packet_1523_1548_bytes_3 (),
    .stat_tx_packet_small_3 (),
    .stat_tx_packet_large_3 (),
    .stat_tx_packet_1549_2047_bytes_3 (),
    .stat_tx_packet_2048_4095_bytes_3 (),
    .stat_tx_packet_4096_8191_bytes_3 (),
    .stat_tx_packet_8192_9215_bytes_3 (),
    .stat_tx_bad_fcs_3 (),
    .stat_tx_frame_error_3 (),
    .stat_tx_local_fault_3 ()*/

);


/* RX */
assign m_axis_0_tvalid = m_axis_tvalid[0];
assign m_axis_tready[0] = m_axis_0_tready;
assign m_axis_0_tdata = m_axis_tdata[0];
assign m_axis_0_tkeep = m_axis_tkeep[0];
assign m_axis_0_tlast = m_axis_tlast[0];

/*assign m_axis_1_tvalid = m_axis_tvalid[1];
assign m_axis_tready[1] = m_axis_1_tready;
assign m_axis_1_tdata = m_axis_tdata[1];
assign m_axis_1_tkeep = m_axis_tkeep[1];
assign m_axis_1_tlast = m_axis_tlast[1];

assign m_axis_2_tvalid = m_axis_tvalid[2];
assign m_axis_tready[2] = m_axis_2_tready;
assign m_axis_2_tdata = m_axis_tdata[2];
assign m_axis_2_tkeep = m_axis_tkeep[2];
assign m_axis_2_tlast = m_axis_tlast[2];

assign m_axis_3_tvalid = m_axis_tvalid[3];
assign m_axis_tready[3] = m_axis_3_tready;
assign m_axis_3_tdata = m_axis_tdata[3];
assign m_axis_3_tkeep = m_axis_tkeep[3];
assign m_axis_3_tlast = m_axis_tlast[3];*/


//wire[3:0] rx_axis_tready;
genvar idx;
generate for (idx = 0; idx < 1; idx = idx + 1) begin

//assign rx_axis_tready_cross[idx] = 1'b1; // the rx_interface does not assert backpressure!

rx_interface rx_if    
(
    .axi_str_tvalid_from_xgmac(rx_axis_tvalid[idx]),
    .axi_str_tdata_from_xgmac(rx_axis_tdata[idx]),
    .axi_str_tkeep_from_xgmac(rx_axis_tkeep[idx]),
    .axi_str_tlast_from_xgmac(rx_axis_tlast[idx]),
    .axi_str_tuser_from_xgmac(rx_axis_tuser[idx]),

    .axi_str_tready_from_fifo(axis_rxif_to_fifo_tready[idx]),
    .axi_str_tdata_to_fifo(axis_rxif_to_fifo_tdata[idx]),   
    .axi_str_tkeep_to_fifo(axis_rxif_to_fifo_tkeep[idx]),   
    .axi_str_tvalid_to_fifo(axis_rxif_to_fifo_tvalid[idx]),
    .axi_str_tlast_to_fifo(axis_rxif_to_fifo_tlast[idx]),
    .rd_pkt_len(),
    .rx_fifo_overflow(),
    
    .rx_statistics_vector(),
    .rx_statistics_valid(),

    .rd_data_count(),

    .user_clk(rx_core_clk[idx]),
    .reset(sys_reset | user_rx_reset[idx])

);

axis_data_fifo_64_cc rx_crossing (
  .s_axis_aresetn(~(sys_reset | user_rx_reset[idx])),          // input wire s_axis_aresetn
  .s_axis_aclk(rx_core_clk[idx]),                // input wire s_axis_aclk
  .s_axis_tvalid(axis_rxif_to_fifo_tvalid[idx]),            // input wire s_axis_tvalid
  .s_axis_tready(axis_rxif_to_fifo_tready[idx]),            // output wire s_axis_tready
  .s_axis_tdata(axis_rxif_to_fifo_tdata[idx]),              // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axis_rxif_to_fifo_tkeep[idx]),              // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axis_rxif_to_fifo_tlast[idx]),              // input wire s_axis_tlast
  .m_axis_aclk(net_clk),                // input wire m_axis_aclk
  .m_axis_aresetn(aresetn),          // input wire m_axis_aresetn
  .m_axis_tvalid(m_axis_tvalid[idx]),            // output wire m_axis_tvalid
  .m_axis_tready(m_axis_tready[idx]),            // input wire m_axis_tready
  .m_axis_tdata(m_axis_tdata[idx]),              // output wire [63 : 0] m_axis_tdata
  .m_axis_tkeep(m_axis_tkeep[idx]),              // output wire [7 : 0] m_axis_tkeep
  .m_axis_tlast(m_axis_tlast[idx]),              // output wire m_axis_tlast
  .axis_data_count(),        // output wire [31 : 0] axis_data_count
  .axis_wr_data_count(),  // output wire [31 : 0] axis_wr_data_count
  .axis_rd_data_count()  // output wire [31 : 0] axis_rd_data_count
);

end
endgenerate

/* TX */
////----------------------------------------------------------------------
//// The following process is used to pad outgoing packets to span at least
//// eight words. Weirdly, if I set a keep value less then 0x0F on the eigtht
//// word, the packet will be dropped by the switch.
////----------------------------------------------------------------------
assign s_axis_tvalid[0] = s_axis_0_tvalid;
assign s_axis_0_tready = s_axis_tready[0];
assign s_axis_tdata[0] = s_axis_0_tdata;
assign s_axis_tkeep[0] = s_axis_0_tkeep;
assign s_axis_tlast[0] = s_axis_0_tlast;

/*assign s_axis_tvalid[1] = s_axis_1_tvalid;
assign s_axis_1_tready = s_axis_tready[1];
assign s_axis_tdata[1] = s_axis_1_tdata;
assign s_axis_tkeep[1] = s_axis_1_tkeep;
assign s_axis_tlast[1] = s_axis_1_tlast;

assign s_axis_tvalid[2] = s_axis_2_tvalid;
assign s_axis_2_tready = s_axis_tready[2];
assign s_axis_tdata[2] = s_axis_2_tdata;
assign s_axis_tkeep[2] = s_axis_2_tkeep;
assign s_axis_tlast[2] = s_axis_2_tlast;

assign s_axis_tvalid[3] = s_axis_3_tvalid;
assign s_axis_3_tready = s_axis_tready[3];
assign s_axis_tdata[3] = s_axis_3_tdata;
assign s_axis_tkeep[3] = s_axis_3_tkeep;
assign s_axis_tlast[3] = s_axis_3_tlast;*/

//genvar idx;
generate for (idx = 0; idx < 1; idx = idx + 1) begin

tx_interface tx_inf
(
    .axi_str_tdata_to_xgmac(tx_axis_tdata[idx]),
    .axi_str_tkeep_to_xgmac(tx_axis_tkeep[idx]),
    .axi_str_tvalid_to_xgmac(tx_axis_tvalid[idx]),
    .axi_str_tlast_to_xgmac(tx_axis_tlast[idx]),
    .axi_str_tuser_to_xgmac(tx_axis_tuser[idx]),
    .axi_str_tready_from_xgmac(tx_axis_tready[idx]),
    
    .axi_str_tvalid_from_fifo(axis_tx_fifo_to_txif_tvalid[idx]),
    .axi_str_tready_to_fifo(axis_tx_fifo_to_txif_tready[idx]),
    .axi_str_tdata_from_fifo(axis_tx_fifo_to_txif_tdata[idx]),   
    .axi_str_tkeep_from_fifo(axis_tx_fifo_to_txif_tkeep[idx]),   
    .axi_str_tlast_from_fifo(axis_tx_fifo_to_txif_tlast[idx]),

    .user_clk(tx_clk_out[idx]),
    .reset(sys_reset | user_tx_reset[idx])

);

axis_data_fifo_64_cc tx_crossing (
  .s_axis_aresetn(aresetn),          // input wire s_axis_aresetn
  .s_axis_aclk(net_clk),                // input wire s_axis_aclk
  .s_axis_tvalid(axis_tx_padding_to_fifo_tvalid[idx]),            // input wire s_axis_tvalid
  .s_axis_tready(axis_tx_padding_to_fifo_tready[idx]),            // output wire s_axis_tready
  .s_axis_tdata(axis_tx_padding_to_fifo_tdata[idx]),              // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axis_tx_padding_to_fifo_tkeep[idx]),              // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axis_tx_padding_to_fifo_tlast[idx]),              // input wire s_axis_tlast
  .m_axis_aclk(tx_clk_out[idx]),                // input wire m_axis_aclk
  .m_axis_aresetn(~(sys_reset | user_tx_reset[idx])),          // input wire m_axis_aresetn
  .m_axis_tvalid(axis_tx_fifo_to_txif_tvalid[idx]),            // output wire m_axis_tvalid
  .m_axis_tready(axis_tx_fifo_to_txif_tready[idx]),            // input wire m_axis_tready
  .m_axis_tdata(axis_tx_fifo_to_txif_tdata[idx]),              // output wire [63 : 0] m_axis_tdata
  .m_axis_tkeep(axis_tx_fifo_to_txif_tkeep[idx]),              // output wire [7 : 0] m_axis_tkeep
  .m_axis_tlast(axis_tx_fifo_to_txif_tlast[idx]),              // output wire m_axis_tlast
  .axis_data_count(),        // output wire [31 : 0] axis_data_count
  .axis_wr_data_count(),  // output wire [31 : 0] axis_wr_data_count
  .axis_rd_data_count()  // output wire [31 : 0] axis_rd_data_count
);

ethernet_frame_padding_ip ethernet_frame_padding_inst (
  .m_axis_TVALID(axis_tx_padding_to_fifo_tvalid[idx]),  // output wire m_axis_TVALID
  .m_axis_TREADY(axis_tx_padding_to_fifo_tready[idx]),  // input wire m_axis_TREADY
  .m_axis_TDATA(axis_tx_padding_to_fifo_tdata[idx]),    // output wire [63 : 0] m_axis_TDATA
  .m_axis_TKEEP(axis_tx_padding_to_fifo_tkeep[idx]),    // output wire [7 : 0] m_axis_TKEEP
  .m_axis_TLAST(axis_tx_padding_to_fifo_tlast[idx]),    // output wire [0 : 0] m_axis_TLAST
  .s_axis_TVALID(s_axis_tvalid[idx]),  // input wire s_axis_TVALID
  .s_axis_TREADY(s_axis_tready[idx]),  // output wire s_axis_TREADY
  .s_axis_TDATA(s_axis_tdata[idx]),    // input wire [63 : 0] s_axis_TDATA
  .s_axis_TKEEP(s_axis_tkeep[idx]),    // input wire [7 : 0] s_axis_TKEEP
  .s_axis_TLAST(s_axis_tlast[idx]),    // input wire [0 : 0] s_axis_TLAST
  .aclk(net_clk),                    // input wire aclk
  .aresetn(aresetn)              // input wire aresetn
);
end
endgenerate

endmodule

`default_nettype wire
