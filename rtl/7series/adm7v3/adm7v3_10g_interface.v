`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 21.08.2013 15:05:03
// Design Name: 
// Module Name: vc709_10g_interface
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


module adm7v3_10g_interface(
// 200MHz reference clock input
    input                          reset,
    input                          aresetn,
    
    // 156.25 MHz clock in
    input                          xphy_refclk_p,
    input                          xphy_refclk_n,
    
   output                         xphy0_txp,
  output                         xphy0_txn,
  input                          xphy0_rxp,
  input                          xphy0_rxn,
  
    output                         xphy1_txp,
    output                         xphy1_txn,
    input                          xphy1_rxp,
    input                          xphy1_rxn,
    
    output                         xphy2_txp,
    output                         xphy2_txn,
    input                          xphy2_rxp,
    input                          xphy2_rxn,
    
    output                         xphy3_txp,
    output                         xphy3_txn,
    input                          xphy3_rxp,
    input                          xphy3_rxn,
    
    output[63:0]      axis_i_0_tdata,
    output            axis_i_0_tvalid,
    output            axis_i_0_tlast,
    output            axis_i_0_tuser,
    output[7:0]       axis_i_0_tkeep,
    input            axis_i_0_tready,
    //rx status
    output           nic_rx_fifo_overflow,
    output [29:0]   nic_rx_statistics_vector,
    output          nic_rx_statistics_valid,
    
    input[63:0]      axis_o_0_tdata,
    input            axis_o_0_tvalid,
    input            axis_o_0_tlast,
    input            axis_o_0_tuser,
    input[7:0]       axis_o_0_tkeep,
    output            axis_o_0_tready,
    
    output[3:0] sfp_tx_disable,
    output clk156_out,
    output clk_ref_200_out,
    output network_reset_done,
    
  
  output  [7:0]                  led 
);



wire                                  clk_ref_200;

wire[7:0] core0_status;
wire[7:0] core1_status;
wire[7:0] core2_status;
wire[7:0] core3_status;

// Shared clk signals
wire gt_txclk322;
wire gt_txusrclk;
wire gt_txusrclk2;
wire gt_qplllock;
wire gt_gpllrefclklost;
wire gt_gplloutrefclk;
wire gt_gplllock_txusrclk2;
wire gttxreset_txusrclk2;
wire gt_txuserrdy;
wire tx_fault;
wire core_reset;
wire gt0_tx_resetdone;
wire gt1_tx_resetdone;
wire gt2_tx_resetdone;
wire gt3_tx_resetdone;
wire areset_clk_156_25_bufh;
wire areset_clk_156_25;
wire mmcm_locked_clk156;
wire reset_counter_done;
wire gttxreset;
wire gtrxreset;
wire clk156_25;
wire dclk_i;
wire xphyrefclk_i;

assign network_reset_done = ~core_reset;

wire[63:0]      axis_i_1_tdata;
wire            axis_i_1_tvalid;
wire            axis_i_1_tlast;
wire            axis_i_1_tuser;
wire[7:0]       axis_i_1_tkeep;
wire            axis_i_1_tready;

wire[63:0]      axis_o_1_tdata;
wire            axis_o_1_tvalid;
wire            axis_o_1_tlast;
//wire            axis_o_1_tuser;
wire[7:0]       axis_o_1_tkeep;
wire            axis_o_1_tready;

wire[63:0]      axis_i_2_tdata;
wire            axis_i_2_tvalid;
wire            axis_i_2_tlast;
wire            axis_i_2_tuser;
wire[7:0]       axis_i_2_tkeep;
wire            axis_i_2_tready;

wire[63:0]      axis_o_2_tdata;
wire            axis_o_2_tvalid;
wire            axis_o_2_tlast;
//wire          axis_o_2_tuser;
wire[7:0]       axis_o_2_tkeep;
wire            axis_o_2_tready;

wire[63:0]      axis_i_3_tdata;
wire            axis_i_3_tvalid;
wire            axis_i_3_tlast;
wire            axis_i_3_tuser;
wire[7:0]       axis_i_3_tkeep;
wire            axis_i_3_tready;

wire[63:0]      axis_o_3_tdata;
wire            axis_o_3_tvalid;
wire            axis_o_3_tlast;
//wire          axis_o_3_tuser;
wire[7:0]       axis_o_3_tkeep;
wire            axis_o_3_tready;


assign clk156_out = clk156_25;
assign clk_ref_200_out = clk_ref_200;
/*
 * Clocks
 */


/*
 * Network modules
 */

wire[7:0]   tx_ifg_delay;
wire        signal_detect;
//wire        tx_fault;
assign tx_ifg_delay     = 8'h00; 
assign signal_detect    = 1'b1;
//assign tx_fault         = 1'b0;


network_module network_inst_0
(
.clk156 (clk156_25),
.reset(reset),
.aresetn(aresetn),
.dclk                             (dclk_i),
.txusrclk                         (gt_txusrclk),
.txusrclk2                        (gt_txusrclk2),
.txclk322                         (gt_txclk322),

.areset_refclk_bufh               (areset_clk_156_25_bufh),
.areset_clk156                    (areset_clk_156_25),
.mmcm_locked_clk156              (mmcm_locked_clk156),
.gttxreset_txusrclk2              (gttxreset_txusrclk2),
.gttxreset                        (gttxreset),
.gtrxreset                        (gtrxreset),
.txuserrdy                        (gt_txuserrdy),
.qplllock                         (gt_qplllock),
.qplloutclk                       (gt_qplloutclk),
.qplloutrefclk                    (gt_qplloutrefclk),
.reset_counter_done               (reset_counter_done),
.tx_resetdone                      (gt0_tx_resetdone),

.txp(xphy0_txp),
.txn(xphy0_txn),
.rxp(xphy0_rxp),
.rxn(xphy0_rxn),

.tx_axis_tdata(axis_o_0_tdata),
.tx_axis_tvalid(axis_o_0_tvalid),
.tx_axis_tlast(axis_o_0_tlast),
.tx_axis_tuser(1'b0),
.tx_axis_tkeep(axis_o_0_tkeep),
.tx_axis_tready(axis_o_0_tready),

.rx_axis_tdata(axis_i_0_tdata),
.rx_axis_tvalid(axis_i_0_tvalid),
.rx_axis_tuser(axis_i_0_tuser),
.rx_axis_tlast(axis_i_0_tlast),
.rx_axis_tkeep(axis_i_0_tkeep),
.rx_axis_tready(axis_i_0_tready),

.core_reset(core_reset),
.tx_fault(tx_fault),
.signal_detect(signal_detect),
.tx_ifg_delay(tx_ifg_delay),
.tx_disable(),
.rx_fifo_overflow(nic_rx_fifo_overflow),
.rx_statistics_vector(nic_rx_statistics_vector),
.rx_statistics_valid(nic_rx_statistics_valid),
.core_status(core0_status)
);

//wire xphyrefclk_i;

IBUFDS_GTE2 xgphy_refclk_ibuf (

    .I      (xphy_refclk_p),
    .IB     (xphy_refclk_n),
    .O      (xphyrefclk_i  ),
    .CEB    (1'b0          ),
    .ODIV2  (              )   

);


assign gt1_tx_resetdone = 1'b1;
assign gt2_tx_resetdone = 1'b1;
assign gt3_tx_resetdone = 1'b1;

xgbaser_gt_same_quad_wrapper #(
.WRAPPER_SIM_GTRESET_SPEEDUP     ("TRUE"                        )
) xgbaser_gt_wrapper_inst (
.gt_txclk322                       (gt_txclk322),
.gt_txusrclk                       (gt_txusrclk),
.gt_txusrclk2                      (gt_txusrclk2),
.qplllock                          (gt_qplllock),
.qpllrefclklost                    (gt_qpllrefclklost),
.qplloutclk                        (gt_qplloutclk),
.qplloutrefclk                     (gt_qplloutrefclk),
.qplllock_txusrclk2                (gt_qplllock_txusrclk2), //not used
.gttxreset_txusrclk2               (gttxreset_txusrclk2),
.txuserrdy                         (gt_txuserrdy),
.tx_fault                          (tx_fault), 
.core_reset                        (core_reset),
.gt0_tx_resetdone                  (gt0_tx_resetdone),
.gt1_tx_resetdone                  (gt1_tx_resetdone),
.gt2_tx_resetdone                  (gt2_tx_resetdone),
.gt3_tx_resetdone                  (gt3_tx_resetdone),
.areset_clk_156_25_bufh            (areset_clk_156_25_bufh),
.areset_clk_156_25                 (areset_clk_156_25),
.mmcm_locked_clk156                (mmcm_locked_clk156),
.reset_counter_done                (reset_counter_done),
.gttxreset                         (gttxreset),
.gtrxreset                         (gtrxreset),
.clk156                            (clk156_25            ),
.areset                            (reset),  
.dclk                              (dclk_i                     ), 
.gt_refclk                         (xphyrefclk_i               )
);
    
assign sfp_tx_disable = 4'b0000;

localparam  LED_CTR_WIDTH           = 26;
reg     [LED_CTR_WIDTH-1:0]           l1_ctr;
reg     [LED_CTR_WIDTH-1:0]           l2_ctr;

always @(posedge clk156_25)
begin
    l1_ctr <= l1_ctr + {{(LED_CTR_WIDTH-1){1'b0}}, 1'b1};
end

assign led[0] = l1_ctr[LED_CTR_WIDTH-1];
assign led[1] = l2_ctr[LED_CTR_WIDTH-1];
//assign led[2] = reset;
//assign led[3] = core_reset;
assign led[2] = core0_status[0];

endmodule
