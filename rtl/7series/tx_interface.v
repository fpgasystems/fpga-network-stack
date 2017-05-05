`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 21.08.2013 09:24:34
// Design Name: 
// Module Name: tx_interface
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


module tx_interface #(
    parameter      FIFO_CNT_WIDTH = 11 //depth: 4096 not sure why
)
(
    output [63:0]   axi_str_tdata_to_xgmac,
    output [7:0]    axi_str_tkeep_to_xgmac,
    output          axi_str_tvalid_to_xgmac,
    output          axi_str_tlast_to_xgmac,
    output          axi_str_tuser_to_xgmac,
    input           axi_str_tready_from_xgmac,
    
    //input  [47:0]  mac_id,
    //input          mac_id_valid,
    //input          promiscuous_mode_en,
   //input  [29:0]  rx_statistics_vector,
    //input          rx_statistics_valid,
    input [63:0]  axi_str_tdata_from_fifo,   
    input [7:0]   axi_str_tkeep_from_fifo,   
    input         axi_str_tvalid_from_fifo,
    output          axi_str_tready_to_fifo,
    input         axi_str_tlast_from_fifo,
    //output [15:0]  rd_pkt_len,
   // output reg     rx_fifo_overflow = 1'b0,

    //output [FIFO_CNT_WIDTH-1:0]  rd_data_count ,

    input          user_clk,
    //input          soft_reset, 
    input          reset

);

reg state_wr;
reg state_rd;
reg pkg_push;

reg cmd_fifo_din;
reg cmd_fifo_wr_en;
//wire cmd_fifo_dout;
reg cmd_fifo_rd_en;
wire cmd_fifo_full;
wire cmd_fifo_empty;

wire [FIFO_CNT_WIDTH-1:0]  wr_data_count ;
  wire [FIFO_CNT_WIDTH-1:0]  left_over_space_in_fifo; 

localparam IDLE = 0;
localparam LOAD = 1;
localparam PUSH = 1;

wire axis_rd_tready;
wire axis_rd_tvalid;
wire axis_rd_tlast;
wire[63:0] axis_rd_tdata;
wire[7:0] axis_rd_tkeep;

wire axis_wr_tready;
wire axis_wr_tvalid;
wire axis_wr_tlast;
wire[63:0] axis_wr_tdata;
wire[7:0] axis_wr_tkeep;

assign axi_str_tready_to_fifo = (!cmd_fifo_full) & axis_wr_tready;
assign axis_wr_tvalid = axi_str_tvalid_from_fifo & axi_str_tready_to_fifo;
assign axis_wr_tlast = axi_str_tlast_from_fifo;
assign axis_wr_tdata = axi_str_tdata_from_fifo;
assign axis_wr_tkeep = axi_str_tkeep_from_fifo;

assign axis_rd_tready = axi_str_tready_from_xgmac & pkg_push;
assign axi_str_tvalid_to_xgmac = axis_rd_tvalid & pkg_push;
assign axi_str_tlast_to_xgmac = axis_rd_tlast;
assign axi_str_tdata_to_xgmac = axis_rd_tdata;
assign axi_str_tkeep_to_xgmac = axis_rd_tkeep;

assign axi_str_tuser_to_xgmac = 1'b0;

assign left_over_space_in_fifo = {1'b1,{(FIFO_CNT_WIDTH-1){1'b0}}} - wr_data_count[FIFO_CNT_WIDTH-1:0];


//observes if complete pkg in buffer
always @(posedge user_clk)
begin
    if (reset == 1) begin
        //pkg_loaded <= 1'b0;
        cmd_fifo_wr_en <= 1'b0;
        cmd_fifo_din <= 1'b0;
        state_wr <= IDLE;
    end
    else begin
        case (state_wr)
            IDLE: begin
                //pkg_loaded <= 1'b0;
                cmd_fifo_din <= 1'b0;
                cmd_fifo_wr_en <= 1'b0;
                if (axis_wr_tvalid) begin
                    state_wr <= LOAD;
                end
            end
            LOAD: begin
                cmd_fifo_wr_en <= 1'b0;
                if (axis_wr_tlast & axis_wr_tvalid) begin
                    //pkg_loaded <= 1'b1;
                    if (!cmd_fifo_full && axis_wr_tready) begin
                        cmd_fifo_din <= 1'b1;
                        cmd_fifo_wr_en <= 1'b1;
                        state_wr <= IDLE;
                    end                                        
                end
            end
        endcase 
    end
end

always @(posedge user_clk)
begin
    if (reset == 1) begin
        state_rd <= IDLE;
        pkg_push <= 1'b0;
        cmd_fifo_rd_en <= 1'b0;
    end
    else begin
        case (state_rd)
            IDLE: begin
                pkg_push <= 1'b0;
                cmd_fifo_rd_en <= 1'b0;
                if (!cmd_fifo_empty) begin
                    pkg_push <= 1'b1;
                    cmd_fifo_rd_en <= 1'b1;
                    state_rd <= PUSH;
                end
            end
            PUSH: begin
                pkg_push <= 1'b1;
                cmd_fifo_rd_en <= 1'b0;
                if (axis_rd_tlast & axis_rd_tready & axis_rd_tvalid) begin
                    pkg_push <= 1'b0;
                    state_rd <= IDLE;
                end
            end
         endcase
    end
end

//-Data FIFO instance: AXI Stream Asynchronous FIFO
  //XGEMAC interface outputs an entire frame in a single shot
  //TREADY signal from slave interface of FIFO is left unconnected
  axis_sync_fifo axis_fifo_inst1 (
    .m_axis_tready        (axis_rd_tready           ),
    .s_aresetn            (~reset                   ),
    .s_axis_tready        (axis_wr_tready           ),
    .s_aclk               (user_clk                 ),
    .s_axis_tvalid        (axis_wr_tvalid           ),
    .m_axis_tvalid        (axis_rd_tvalid           ),
    //.m_aclk               (user_clk                 ),
    .m_axis_tlast         (axis_rd_tlast            ),
    .s_axis_tlast         (axis_wr_tlast            ),
    .s_axis_tdata         (axis_wr_tdata            ),
    .m_axis_tdata         (axis_rd_tdata            ),
    .s_axis_tkeep         (axis_wr_tkeep            ),
    .m_axis_tkeep         (axis_rd_tkeep            ),
    //.axis_rd_data_count   (rd_data_count            ),
    .axis_data_count   (wr_data_count            )
  );
  
cmd_fifo_xgemac_txif cmd_fifo_inst (
.clk(user_clk), // input clk
.rst(reset), // input rst
.din(cmd_fifo_din), // input [0 : 0] din
.wr_en(cmd_fifo_wr_en), // input wr_en
.rd_en(cmd_fifo_rd_en), // input rd_en
.dout(), // output [0 : 0] dout
.full(cmd_fifo_full), // output full
.empty(cmd_fifo_empty) // output empty
);

/*wire [35:0] control0;
wire [35:0] control1;
wire [63:0] vio_signals;
wire [127:0] debug_signal;

icon icon_isnt
(
  .CONTROL0 (control0),
  .CONTROL1 (control1)
);

ila ila_inst
(
    .CLK (user_clk),
    .CONTROL (control0),
    .TRIG0 (debug_signal)
);

vio vio_inst
(
    .CLK (user_clk),
    .CONTROL (control1),
    .SYNC_OUT (vio_signals)
);

reg[2:0] pkg_count;

always @(posedge user_clk)
begin
    if (reset == 1) begin
        pkg_count <= 0;
    end
    else begin
        if (cmd_fifo_wr_en == 1'b1) begin
            pkg_count <= pkg_count + 1;
        end
    end
end

assign debug_signal[63:0] = axi_str_tdata_from_fifo;
assign debug_signal[71:64] = axi_str_tkeep_from_fifo;
assign debug_signal[72] = axi_str_tvalid_from_fifo;
assign debug_signal[73] = axi_str_tready_to_fifo;
assign debug_signal[74] = axi_str_tlast_from_fifo;
assign debug_signal[75] = 1'b0;
assign debug_signal[78:76] = pkg_count;
//assign debug_signal[79] = pkg_loaded;
assign debug_signal[80] = axis_wr_tready;
assign debug_signal[81] = axis_wr_tvalid;
assign debug_signal[82] = axis_wr_tlast;
assign debug_signal[98:83] = axis_wr_tdata[15:0];
assign debug_signal[106:99] = axis_wr_tkeep;
assign debug_signal[107] = cmd_fifo_din;
assign debug_signal[108] = cmd_fifo_wr_en;
assign debug_signal[109] = cmd_fifo_rd_en;
assign debug_signal[110] = cmd_fifo_full;
assign debug_signal[111] = cmd_fifo_empty;*/

endmodule

