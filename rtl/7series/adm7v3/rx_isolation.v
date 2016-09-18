`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: Xilinx
// Engineer: Lisa Liu
// 
// Create Date: 06/30/2014 03:42:37 PM
// Design Name: 
// Module Name: rx_isolation
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: read back presure signal (tready) from 8K byte FIFO and drop packets from xgmac whenever the empty space in the FIFO is less than 5K.
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module rx_isolation #(
    FIFO_FULL_THRESHOLD = 11'd256
)
(
   input [63:0]   axi_str_tdata_from_xgmac,
   input [7:0]    axi_str_tkeep_from_xgmac,
   input          axi_str_tvalid_from_xgmac,
   input          axi_str_tlast_from_xgmac,
   //input          axi_str_tuser_from_xgmac,
   
   input          axi_str_tready_from_fifo,
   output [63:0]  axi_str_tdata_to_fifo,   
   output [7:0]   axi_str_tkeep_to_fifo,   
   output         axi_str_tvalid_to_fifo,
   output         axi_str_tlast_to_fifo,
   
   input          user_clk, 
   input          reset
);
reg [63:0]   axi_str_tdata_from_xgmac_r;
reg [7:0]    axi_str_tkeep_from_xgmac_r;
reg          axi_str_tvalid_from_xgmac_r;
reg          axi_str_tlast_from_xgmac_r;

wire[10:0] fifo_occupacy_count;
wire s_axis_tvalid;
reg [10:0] wcount_r; //number of words in the current xgmac packet

wire fifo_has_space;

localparam IDLE = 1'd0,
           STREAMING = 1'd1;
           
reg curr_state_r;

rx_fifo rx_fifo_inst (
  .s_aclk(user_clk),                    // input wire s_aclk
  .s_aresetn(~reset),              // input wire s_aresetn
  .s_axis_tvalid(s_axis_tvalid),      // input wire s_axis_tvalid
  .s_axis_tready(),      // output wire s_axis_tready
  .s_axis_tdata(axi_str_tdata_from_xgmac_r),        // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axi_str_tkeep_from_xgmac_r),        // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axi_str_tlast_from_xgmac_r),        // input wire s_axis_tlast
  .m_axis_tvalid(axi_str_tvalid_to_fifo),      // output wire m_axis_tvalid
  .m_axis_tready(axi_str_tready_from_fifo),      // input wire m_axis_tready
  .m_axis_tdata(axi_str_tdata_to_fifo),        // output wire [63 : 0] m_axis_tdata
  .m_axis_tkeep(axi_str_tkeep_to_fifo),        // output wire [7 : 0] m_axis_tkeep
  .m_axis_tlast(axi_str_tlast_to_fifo),        // output wire m_axis_tlast
  .axis_data_count(fifo_occupacy_count)  // output wire [10 : 0] axis_data_count
);





assign fifo_has_space = (fifo_occupacy_count < FIFO_FULL_THRESHOLD);
assign s_axis_tvalid = axi_str_tvalid_from_xgmac_r & (((wcount_r == 0) & fifo_has_space) | (curr_state_r == STREAMING));

always @(posedge user_clk) begin
    axi_str_tdata_from_xgmac_r <= axi_str_tdata_from_xgmac;
    axi_str_tkeep_from_xgmac_r <= axi_str_tkeep_from_xgmac;
    axi_str_tvalid_from_xgmac_r <= axi_str_tvalid_from_xgmac;
    axi_str_tlast_from_xgmac_r <= axi_str_tlast_from_xgmac;
end

always @(posedge user_clk)
    if (reset)
        wcount_r <= 0;
    else if (axi_str_tvalid_from_xgmac_r & ~axi_str_tlast_from_xgmac_r)
        wcount_r <= wcount_r + 1;
    else if (axi_str_tvalid_from_xgmac_r & axi_str_tlast_from_xgmac_r)
        wcount_r <= 0;

always @(posedge user_clk)
    if (reset)
        curr_state_r <= IDLE;
    else 
        case (curr_state_r) 
            IDLE: if ((wcount_r == 0) & fifo_has_space & axi_str_tvalid_from_xgmac_r)
                    curr_state_r <= STREAMING;
            STREAMING: if (axi_str_tvalid_from_xgmac_r & axi_str_tlast_from_xgmac_r)
                    curr_state_r <= IDLE;
        endcase
                
     /*   always @(posedge user_clk) begin
            data [63:0] <=  axi_str_tdata_from_xgmac_r;
            data [71:64] <=  axi_str_tkeep_from_xgmac_r;
            data[72] <= axi_str_tvalid_from_xgmac_r;
            data[73] <= axi_str_tlast_from_xgmac_r;
            
            data[74] <= axi_str_tready_from_fifo;
            
            data[138:75] <= axi_str_tdata_to_fifo;   
            data[146:139] <= axi_str_tkeep_to_fifo;   
            data[147] <= axi_str_tvalid_to_fifo;
            data[148] <=  axi_str_tlast_to_fifo;
            data[159:149] <= wcount_r;
            data[160] <= curr_state_r;
            data[171:161] <= fifo_occupacy_count;
             
            trig0[10:0] <= wcount_r;
            trig0[11] <= axi_str_tvalid_to_fifo;
            trig0[12] <= axi_str_tready_from_fifo;
        end*/
endmodule