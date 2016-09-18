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


module vc709_10g_interface(
// 200MHz reference clock input
    input                          clk_ref_p,
    input                          clk_ref_n,
    input                          reset,
    input                          aresetn,

    //-SI5324 I2C programming interface
    inout                          i2c_clk,
    inout                          i2c_data,
    output                         i2c_mux_rst_n,
    output                         si5324_rst_n,
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
wire                                  clk_ref_200_i;

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


/*wire[63:0]      axis_i_0_tdata;
wire            axis_i_0_tvalid;
wire            axis_i_0_tlast;
wire            axis_i_0_tuser;
wire[7:0]       axis_i_0_tkeep;
wire            axis_i_0_tready;

wire[63:0]      axis_o_0_tdata;
wire            axis_o_0_tvalid;
wire            axis_o_0_tlast;
//wire          axis_o_0_tuser;
wire[7:0]       axis_o_0_tkeep;
wire            axis_o_0_tready;*/

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


// 200mhz ref clk
IBUFGDS #(
  .DIFF_TERM    ("TRUE"),
  .IBUF_LOW_PWR ("FALSE")
) diff_clk_200 (
  .I    (clk_ref_p  ),
  .IB   (clk_ref_n  ),
  .O    (clk_ref_200_i )  
);

BUFG u_bufg_clk_ref
(
  .O (clk_ref_200),
  .I (clk_ref_200_i)
);

// 50mhz clk
wire          clk50;
reg [1:0]     clk_divide = 2'b00;

always @(posedge clk_ref_200)
clk_divide  <= clk_divide + 1'b1;

BUFG buffer_clk50 (
.I    (clk_divide[1]),
.O    (clk50        )
);
 
 
//-SI 5324 programming
clock_control cc_inst (
   .i2c_clk        (i2c_clk        ),
   .i2c_data       (i2c_data       ),
   .i2c_mux_rst_n  (i2c_mux_rst_n  ),
   .si5324_rst_n   (si5324_rst_n   ),
   .rst            (reset    ),  
   .clk50          (clk50          )
 );    


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
.core_status(core0_status)
);

//assign axis_o_0_tdata = axis_i_0_tdata;
///assign axis_o_0_tvalid = axis_i_0_tvalid;
//assign axis_o_0_tkeep = axis_i_0_tkeep;
//assign axis_o_0_tlast = axis_i_0_tlast;
//assign axis_o_0_tuser <= ;
//assign axis_i_0_tready = axis_o_0_tready;
/*
network_module network_inst_1
(
.clk156 (clk156_25),
.reset(reset),
.dclk                             (dclk_i),
.txusrclk                         (gt_txusrclk),
.txusrclk2                        (gt_txusrclk2),
.txclk322                         (),

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
.tx_resetdone                      (gt1_tx_resetdone),

.txp(xphy1_txp),
.txn(xphy1_txn),
.rxp(xphy1_rxp),
.rxn(xphy1_rxn),

.tx_axis_tdata(axis_o_1_tdata),
.tx_axis_tvalid(axis_o_1_tvalid),
.tx_axis_tlast(axis_o_1_tlast),
.tx_axis_tuser(1'b0),
.tx_axis_tkeep(axis_o_1_tkeep),
.tx_axis_tready(axis_o_1_tready),

.rx_axis_tdata(axis_i_1_tdata),
.rx_axis_tvalid(axis_i_1_tvalid),
.rx_axis_tuser(axis_i_1_tuser),
.rx_axis_tlast(axis_i_1_tlast),
.rx_axis_tkeep(axis_i_1_tkeep),
.rx_axis_tready(axis_i_1_tready),

.core_reset(core_reset),
.tx_fault(tx_fault),
.signal_detect(signal_detect),
.tx_ifg_delay(tx_ifg_delay),
.tx_disable(),
.core_status(core1_status)
);

assign axis_o_1_tdata = axis_i_1_tdata;
assign axis_o_1_tvalid = axis_i_1_tvalid;
assign axis_o_1_tkeep = axis_i_1_tkeep;
assign axis_o_1_tlast = axis_i_1_tlast;
//assign axis_o_0_tuser <= ;
assign axis_i_1_tready = axis_o_1_tready;


network_module network_inst_2
(
.clk156 (clk156_25),
.reset(reset),
.dclk                             (dclk_i),
.txusrclk                         (gt_txusrclk),
.txusrclk2                        (gt_txusrclk2),
.txclk322                         (),

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
.tx_resetdone                      (gt2_tx_resetdone),

.txp(xphy2_txp),
.txn(xphy2_txn),
.rxp(xphy2_rxp),
.rxn(xphy2_rxn),

.tx_axis_tdata(axis_o_2_tdata),
.tx_axis_tvalid(axis_o_2_tvalid),
.tx_axis_tlast(axis_o_2_tlast),
.tx_axis_tuser(1'b0),
.tx_axis_tkeep(axis_o_2_tkeep),
.tx_axis_tready(axis_o_2_tready),

.rx_axis_tdata(axis_i_2_tdata),
.rx_axis_tvalid(axis_i_2_tvalid),
.rx_axis_tuser(axis_i_2_tuser),
.rx_axis_tlast(axis_i_2_tlast),
.rx_axis_tkeep(axis_i_2_tkeep),
.rx_axis_tready(axis_i_2_tready),

.core_reset(core_reset),
.tx_fault(tx_fault),
.signal_detect(signal_detect),
.tx_ifg_delay(tx_ifg_delay),
.tx_disable(),
.core_status(core2_status)
);

network_module network_inst_3
(
.clk156 (clk156_25),
.reset(reset),
.dclk                             (dclk_i),
.txusrclk                         (gt_txusrclk),
.txusrclk2                        (gt_txusrclk2),
.txclk322                         (),

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
.tx_resetdone                      (gt3_tx_resetdone),

.txp(xphy3_txp),
.txn(xphy3_txn),
.rxp(xphy3_rxp),
.rxn(xphy3_rxn),

.tx_axis_tdata(axis_o_3_tdata),
.tx_axis_tvalid(axis_o_3_tvalid),
.tx_axis_tlast(axis_o_3_tlast),
.tx_axis_tuser(1'b0),
.tx_axis_tkeep(axis_o_3_tkeep),
.tx_axis_tready(axis_o_3_tready),

.rx_axis_tdata(axis_i_3_tdata),
.rx_axis_tvalid(axis_i_3_tvalid),
.rx_axis_tuser(axis_i_3_tuser),
.rx_axis_tlast(axis_i_3_tlast),
.rx_axis_tkeep(axis_i_3_tkeep),
.rx_axis_tready(axis_i_3_tready),

.core_reset(core_reset),
.tx_fault(tx_fault),
.signal_detect(signal_detect),
.tx_ifg_delay(tx_ifg_delay),
.tx_disable(),
.core_status(core3_status)
);

//switch btw 2 & 3
assign axis_o_2_tdata = axis_i_2_tdata;
assign axis_o_2_tvalid = axis_i_2_tvalid;
assign axis_o_2_tkeep = axis_i_2_tkeep;
assign axis_o_2_tlast = axis_i_2_tlast;
//assign axis_o_0_tuser <= ;
assign axis_i_2_tready = axis_o_2_tready;

assign axis_o_3_tdata = axis_i_3_tdata;
assign axis_o_3_tvalid = axis_i_3_tvalid;
assign axis_o_3_tkeep = axis_i_3_tkeep;
assign axis_o_3_tlast = axis_i_3_tlast;
//assign axis_o_0_tuser <= ;
assign axis_i_3_tready = axis_o_3_tready;
*/

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

/*always @(posedge gt_txclk322)
begin
    l2_ctr <= l2_ctr + {{(LED_CTR_WIDTH-1){1'b0}}, 1'b1};
end

always @(posedge user_clk2)
begin
    l3_ctr <= l3_ctr + {{(LED_CTR_WIDTH-1){1'b0}}, 1'b1};
end
*/
assign led[0] = l1_ctr[LED_CTR_WIDTH-1];
assign led[1] = l2_ctr[LED_CTR_WIDTH-1];
assign led[2] = reset;
assign led[3] = core_reset;
assign led[4] = core0_status[0];
assign led[5] = core1_status[0];
assign led[6] = core2_status[0];
assign led[7] = core3_status[0];

endmodule
