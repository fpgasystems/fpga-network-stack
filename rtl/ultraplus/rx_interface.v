/*******************************************************************************
** ? Copyright 2010 - 2011 Xilinx, Inc. All rights reserved.
** This file contains confidential and proprietary information of Xilinx, Inc. and
** is protected under U.S. and international copyright and other intellectual property laww
w
s.
*******************************************************************************
**   ____  ____
**  /   /\/   /
** /___/  \  /   Vendor: Xilinx
** \   \   \/
**  \   \
**  /   /
** /___/   /\
** \   \  /  \   Virtex-7 XT Connectivity Domain Targeted Reference Design
**  \___\/\___\
**
**  Device: xc7k325t-ffg900-2
**  Version: 1.0
**
*******************************************************************************
**
*******************************************************************************/

/******************************************************************************
The module performs address filtering on the receive. The receive logic FSM detects
a good frame and makes it available to the packet FIFO interface. Two state machines
are implemented: one FSM covers the write data from XGEMAC interface and another FSM controls 
the read logic to packet FIFO
*******************************************************************************/

`timescale 1ps / 1ps

module rx_interface #(
    parameter      FIFO_CNT_WIDTH = 11
)
(
    input [63:0]   axi_str_tdata_from_xgmac,
    input [7:0]    axi_str_tkeep_from_xgmac,
    input          axi_str_tvalid_from_xgmac,
    input          axi_str_tlast_from_xgmac,
    input          axi_str_tuser_from_xgmac,

    input          axi_str_tready_from_fifo,

    output [63:0]  axi_str_tdata_to_fifo,   
    output [7:0]   axi_str_tkeep_to_fifo,   
    output         axi_str_tvalid_to_fifo,
    output         axi_str_tlast_to_fifo,
    output [15:0]  rd_pkt_len,
    output reg     rx_fifo_overflow = 1'b0,
    
    input  [13:0]  rx_statistics_vector,
    input          rx_statistics_valid,

    output [FIFO_CNT_WIDTH-1:0]  rd_data_count ,

    input          user_clk,
    input          reset

);

  //Wire declaration
  //wire                       broadcast_detect;
  //wire [47:0]                rx_mac_id_i;
  wire                       axis_rd_tlast;
  wire                       axis_rd_tvalid;
  wire [63:0]                axis_rd_tdata;
  wire [7:0]                 axis_rd_tkeep;
  wire                       axis_wr_tlast;
  wire                       axis_wr_tvalid;
  wire [63:0]                axis_wr_tdata;
  wire [7:0]                 axis_wr_tkeep;
  //wire                       da_match ;
  wire                       full;
  wire                       empty;
  wire                       valid_cmd;
  wire                       crc_pass;
  wire [15:0]                cmd_out;
  wire                       axis_wr_tready;
  wire [FIFO_CNT_WIDTH-1:0]  wr_data_count ;
  wire [FIFO_CNT_WIDTH-1:0]  left_over_space_in_fifo; 
  wire                       wr_reached_threshold;
  wire                       wr_reached_threshold_extend;
  //wire [47:0]                mac_id_sync;
  //wire                       mac_id_valid_sync;
  //wire                       promiscuous_mode_en_sync;
  wire                       frame_len_ctr_valid;

  //Reg declaration
  reg  [63:0]  axi_str_tdata_from_xgmac_r ;
  reg  [7:0]   axi_str_tkeep_from_xgmac_r ;
  reg          axi_str_tvalid_from_xgmac_r;
  reg          axi_str_tlast_from_xgmac_r ;
  reg          axi_str_tuser_from_xgmac_r ;
  reg          force_tlast_to_fifo='d0 ;
  reg          address_chk_en = 'd0;
  reg          assert_rd='d0;
  reg  [15:0]  cmd_in = 'd0;
  reg          wr_en=1'b0;
  reg          rd_en=1'b0;
  reg          axis_rd_tready='d0 ;
  reg          axis_rd_tvalid_from_fsm=1'b0;
  reg  [3:0]   tkeep_decoded_value;
//  reg          axi_str_tvalid_from_fsm=1'b0;
  reg  [12:0]  rd_pkt_len_count='d0;
  reg  [13:0]  rx_stats_vec_reg='d0;

  reg  [3:0]   frame_len_ctr;
 
localparam
   //states for Write FSM
   IDLE_WR       = 4'b0001,
   DA_DECODE     = 4'b0010,
   BEGIN_WRITE   = 4'b0100,
   DROP_FRAME    = 4'b1000,
   
   //states for Read FSM
   IDLE_RD         = 4'b0001,
   PREP_READ_1     = 4'b0010, 
   PREP_READ_2     = 4'b0100, 
   BEGIN_READ      = 4'b1000;

localparam   THRESHOLD           = 200;
localparam   THRESHOLD_EXT       = 400;

  reg  [3:0]   state_wr = IDLE_WR;
  reg  [3:0]   state_rd = IDLE_RD;


  //Synchronize mac_id, promiscuous_mode_en and mac_id_valid with the destination clock
  /*synchronizer_simple #(.DATA_WIDTH (1)) sync_to_mac_clk_0
  (
   .data_in          (promiscuous_mode_en),
   .new_clk          (user_clk),
   .data_out         (promiscuous_mode_en_sync)
  );

  synchronizer_simple #(.DATA_WIDTH (1)) sync_to_mac_clk_1
  (
   .data_in          (mac_id_valid),
   .new_clk          (user_clk),
   .data_out         (mac_id_valid_sync)
  );

  synchronizer_simple #(.DATA_WIDTH (48)) sync_to_mac_clk_2
  (
   .data_in          (mac_id),
   .new_clk          (user_clk),
   .data_out         (mac_id_sync)
  );*/
    
  //assign broadcast_detect = ((axi_str_tdata_from_xgmac_r[47:0]== {48{1'b1}}) &&  (address_chk_en == 1'b1))?1'b1:1'b0;
 
  //assign rx_mac_id_i = (address_chk_en == 1'b1)?axi_str_tdata_from_xgmac_r[47:0]:48'b0;

  //assign da_match = ((rx_mac_id_i == mac_id_sync) & mac_id_valid_sync)?1'b1:1'b0;
  
  //Add a pipelining stage for received data from xgemac interface.
  //This is necessary for FSM control logic
  always @(posedge user_clk)
  begin
         axi_str_tdata_from_xgmac_r   <= axi_str_tdata_from_xgmac;
         axi_str_tkeep_from_xgmac_r   <= axi_str_tkeep_from_xgmac;
         axi_str_tvalid_from_xgmac_r  <= axi_str_tvalid_from_xgmac;
         axi_str_tlast_from_xgmac_r   <= axi_str_tlast_from_xgmac;
         axi_str_tuser_from_xgmac_r   <= axi_str_tuser_from_xgmac;
  end
 
 
  begin
    assign axis_wr_tvalid = (state_wr==DROP_FRAME) ? 1'b0 : 
   (axi_str_tvalid_from_xgmac_r | (force_tlast_to_fifo & (state_wr == BEGIN_WRITE))); 
  end

  assign axis_wr_tlast  = (axi_str_tlast_from_xgmac_r | force_tlast_to_fifo); 
  assign axis_wr_tkeep  = axi_str_tkeep_from_xgmac_r; 
  assign axis_wr_tdata  = axi_str_tdata_from_xgmac_r;

  //Register Rx statistics vector to be used in the read FSM later
  //Rx statistics is valid only if rx_statistics_valid is asserted
  //from XGEMAC 
  //- bits 18:5 in stats vector provide frame length including FCS, hence
  //subtract 4 bytes to get the frame length only.
  always @(posedge user_clk)
  begin
       if(rx_statistics_valid)
             rx_stats_vec_reg <= rx_statistics_vector[13:0] - 14'd4;
  end

  assign left_over_space_in_fifo = {1'b1,{(FIFO_CNT_WIDTH-1){1'b0}}} - wr_data_count[FIFO_CNT_WIDTH-1:0];

  assign wr_reached_threshold        = (left_over_space_in_fifo < THRESHOLD)?1'b1:1'b0;
  assign wr_reached_threshold_extend = (left_over_space_in_fifo < THRESHOLD_EXT)?1'b1:1'b0;

  always @(posedge user_clk)
  begin
        if(force_tlast_to_fifo)
             force_tlast_to_fifo <= 1'b0;
        else if(wr_reached_threshold & !(axi_str_tlast_from_xgmac & axi_str_tvalid_from_xgmac))
             force_tlast_to_fifo <= 1'b1;
  end

  // Counter to count frame length when length is less than 64B
  // For frame length less than 64B, XGEMAC core reports length including the
  // padded characters. To overcome this situation, a separate counter is implemented
  always @(posedge user_clk)
  begin
        if (reset)
            frame_len_ctr <= 'd0;
        else if (axi_str_tlast_from_xgmac & axi_str_tvalid_from_xgmac)
            frame_len_ctr <= 'd0;
        else if (frame_len_ctr > 4'h8)
            frame_len_ctr <= frame_len_ctr;
        else if(axi_str_tvalid_from_xgmac)
            frame_len_ctr <= frame_len_ctr+1;
  end

  assign frame_len_ctr_valid = (frame_len_ctr != 0) & (frame_len_ctr < 8) & axi_str_tvalid_from_xgmac & axi_str_tlast_from_xgmac;

  // Decoder for TKEEP signal
  always @(axi_str_tkeep_from_xgmac)
   case(axi_str_tkeep_from_xgmac) 
      'h00 : tkeep_decoded_value <= 'd0;
      'h01 : tkeep_decoded_value <= 'd1;
      'h03 : tkeep_decoded_value <= 'd2;
      'h07 : tkeep_decoded_value <= 'd3;
      'h0F : tkeep_decoded_value <= 'd4;
      'h1F : tkeep_decoded_value <= 'd5;
      'h3F : tkeep_decoded_value <= 'd6;
      'h7F : tkeep_decoded_value <= 'd7;
      'hFF : tkeep_decoded_value <= 'd8;
      default : tkeep_decoded_value <= 'h00;
   endcase

  //Two FIFOs are implemented: one for XGEMAC data(data FIFO) and the other for controlling 
  //read side command(command FIFO). 
  //Write FSM: 6 states control the entire write operation
  //cmd_in is an input to the command FIFO and controls the read side command
  //Ethernet packet frame size is available from Rx statistics vector and is
  //made available to the read side through command FIFO
  //FSM states:
  //IDLE_WR  : Wait in this state until valid is received from XGEMAC. If the 
  //           data FIFO is full or tready is de-asserted from FIFO interface
  //           it drops the current frame from XGEMAC
  //DA_DECODE: Destination Address from XGEMAC is decoded in this state. If destination
  //           address matches with MAC address or promiscuous mode is enabled
  //           or broadcast is detected, next state is BEGIN_WRITE. Else the FSM transitions
  //           to IDLE_WR state                 
  //BEGIN_WRITE: The FSM continues to write data into data FIFO until tlast from XGEMAC is hit.
  //             FSM transitions to CHECK_ERROR state if tlast has arrived  
  //DROP_FRAME: The FSM enters into this state if the data FIFO is full or tready from data FIFO is de-asserted
  //            In this state, tvalid to FIFO is de-asserted
always @(posedge user_clk)
  begin
         if(reset)
                 state_wr  <= IDLE_WR;
         else
         begin
             case(state_wr)
                  IDLE_WR : begin
                             cmd_in           <= 'b0;
                             wr_en            <= 1'b0;

                             if(axi_str_tvalid_from_xgmac & (full | wr_reached_threshold))
                             begin
                                  state_wr                <=  DROP_FRAME;
                             end
                             else if(axi_str_tvalid_from_xgmac)
                             begin
                                  state_wr       <=  DA_DECODE;
                             end
                             else
                             begin
                                  state_wr       <=  IDLE_WR;
                             end
                             end
                  DA_DECODE : begin
                             wr_en            <= 1'b0; 
                             cmd_in[1]      <= 1'b1;

                                    state_wr <= BEGIN_WRITE;
                             end
                  BEGIN_WRITE : begin
                             cmd_in[15:2] <= frame_len_ctr_valid ?
          ((frame_len_ctr << 3) + tkeep_decoded_value) : rx_stats_vec_reg;
                             if(force_tlast_to_fifo)     
                             begin
                                    wr_en     <= 1'b1; 
                                    cmd_in[0] <= 1'b0;
                                    state_wr  <= DROP_FRAME;  
                             end  
                             else if(axi_str_tlast_from_xgmac & axi_str_tvalid_from_xgmac)
                             begin
                                    wr_en            <= 1'b1; 
                                    cmd_in[0] <= axi_str_tuser_from_xgmac;
                                    state_wr <= IDLE_WR;  
                             end
                             else
                             begin
                                    wr_en            <= 1'b0; 
                                    cmd_in[0] <= 1'b0;
                                    state_wr <= BEGIN_WRITE;
                             end
                             end
                  DROP_FRAME : begin
                             wr_en            <= 1'b0; 
                               if(axi_str_tlast_from_xgmac_r & axi_str_tvalid_from_xgmac_r & !wr_reached_threshold_extend)

                               begin
                                  //- signals a back 2 back packet
                                 if(axi_str_tvalid_from_xgmac)
                                 begin
                                      state_wr       <=  DA_DECODE;
                                 end
                                 else
                                    state_wr    <= IDLE_WR;
                               end
                               else
                                    state_wr    <= DROP_FRAME;
                               end
                   default :   state_wr    <= IDLE_WR;
             endcase
          end
  end
                              
  assign  valid_cmd  = cmd_out[1]; 
  assign  crc_pass   = ~cmd_out[0];
  assign  rd_pkt_len = {2'b0,cmd_out[15:2]};
 
  //Read FSM reads out the data from data FIFO and present it to the packet FIFO interface
  //The read FSM starts reading data from the data FIFO as soon as it decodes a valid command
  //from the command FIFO. Various state transitions are basically controlled by the command FIFO
  //empty flag and tready assertion from packet FIFO interface 
  //FSM states
  //IDLE_RD: The FSM stays in this state until command FIFO empty is de-asserted and tready from packet 
  //         FIFO interface is active low. 
  //PREP_READ_1: This is an idle cycle, used basically to de-assert rd_en so that command FIFO is read only 
  //             once
  //PREP_READ_2: If the decoded command from command FIFO is valid and CRC detects no error for the frame
  //             the FSM transitions to BEGIN_READ state. tready to FIFO is controlled by tready
  //             from the packet FIFO interface. If CRC fails for a frame, the entire frame is dropped
  //             by de-asserting tvalid to packet FIFO interface
  //BEGIN_READ: In this state, the FSM reads data until tlast from XGEMAC is encountered
  always @(posedge user_clk)
  begin
         if(reset)
         begin
                 state_rd  <= IDLE_RD;
         end
         else
         begin
             case(state_rd)
                  IDLE_RD : begin
                             if(axi_str_tready_from_fifo & !empty)
                             begin
                                  state_rd <=  PREP_READ_1;
                                  rd_en    <=  1'b1; 
                             end
                             else
                             begin
                                  state_rd <=  IDLE_RD;
                             end
                             end 
                  PREP_READ_1 : begin 
                             rd_en    <= 1'b0;
                             state_rd <= PREP_READ_2;
                             end
                  PREP_READ_2 : begin
                             //Continue reading data if CRC passes for a forthcoming frame
                             //CRC check is passed through command FIFO from write side logic 
                             if(valid_cmd & crc_pass)
                             begin
                                  state_rd                <= BEGIN_READ;   
                             end
                             else
                             begin
                                  state_rd                <= BEGIN_READ;
                             end
                             end
                  BEGIN_READ : begin
                             //Continue reading data until tlast from XGEMAC is received     
                             if(axis_rd_tlast & axis_rd_tvalid & axis_rd_tready)
                             begin
                                  state_rd                <= IDLE_RD;
                             end 
                             else
                             begin
                                  state_rd                <= BEGIN_READ;
                             end 
                             end
                  default    :   state_rd                <= IDLE_RD;
             endcase
         end
  end     
 
  always @(state_rd, valid_cmd, crc_pass,axis_rd_tlast,axis_rd_tvalid,axi_str_tready_from_fifo)
  begin
       if(state_rd==PREP_READ_2)
       begin 
               if(valid_cmd & crc_pass)
               begin
                    axis_rd_tready          <= axi_str_tready_from_fifo;
                    axis_rd_tvalid_from_fsm <= axis_rd_tvalid;
                   // rd_pkt_len_count <= rd_pkt_len;
               end
               else
               begin 
                    axis_rd_tready          <= 1'b1;
                    axis_rd_tvalid_from_fsm <= 1'b0;
               end
       end  
       else if(state_rd==BEGIN_READ)
       begin
        if (valid_cmd & crc_pass)
        begin
               //if (rd_pkt_len_count >
               axis_rd_tready          <= axi_str_tready_from_fifo;
               axis_rd_tvalid_from_fsm <= axis_rd_tvalid;
               //rd_pkt_len_count <= rd_pkt_len_count -= 8;
        end
        else
        begin
               axis_rd_tready          <= 1'b1;
               axis_rd_tvalid_from_fsm <= 1'b0;
        end
       end 
       else
       begin
              axis_rd_tready          <= 1'b0; 
              axis_rd_tvalid_from_fsm <= 1'b0;
       end
  end

  //-Data FIFO instance: AXI Stream Asynchronous FIFO
  //XGEMAC interface outputs an entire frame in a single shot
  //TREADY signal from slave interface of FIFO is left unconnected
  axis_sync_fifo axis_fifo_inst1 (
    .m_axis_tready        (axis_rd_tready           ),
    .s_aresetn            (~reset                   ),
    .s_axis_tready        (axis_wr_tready           ),
    //.s_aclk               (user_clk                 ),
    .s_axis_tvalid        (axis_wr_tvalid           ),
    .m_axis_tvalid        (axis_rd_tvalid           ),
    .s_aclk               (user_clk                 ),
    .m_axis_tlast         (axis_rd_tlast            ),
    .s_axis_tlast         (axis_wr_tlast            ),
    .s_axis_tdata         (axis_wr_tdata            ),
    .m_axis_tdata         (axis_rd_tdata            ),
    .s_axis_tkeep         (axis_wr_tkeep            ),
    .m_axis_tkeep         (axis_rd_tkeep            ),
    //.axis_rd_data_count   (rd_data_count            ),
    //.axis_wr_data_count   (wr_data_count            )
    .axis_data_count   (wr_data_count            ) //1024 items = [10:0]
  );

  //command FIFO interface for controlling the read side interface
  cmd_fifo_xgemac_rxif cmd_fifo_inst (
        .clk         (user_clk   ),
        .rst         (reset      ),
        .din         (cmd_in     ), // Bus [15 : 0]  
        .wr_en       (wr_en      ),
        .rd_en       (rd_en      ),
        .dout        (cmd_out    ), // Bus [15 : 0]  
        .full        (full       ),
        .empty       (empty      )
  );

  assign  axi_str_tdata_to_fifo  = axis_rd_tdata; 
  assign  axi_str_tkeep_to_fifo  = axis_rd_tkeep; 
  assign  axi_str_tlast_to_fifo  = axis_rd_tlast; 
  assign  axi_str_tvalid_to_fifo = axis_rd_tvalid_from_fsm; 

 always @(posedge user_clk)
    if (reset)
      rx_fifo_overflow  <= 1'b0;
    else if (state_wr==DROP_FRAME)
      rx_fifo_overflow  <= 1'b1;



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
        pkg_count <= 3'b000;
    end
    else begin
        if ((axi_str_tvalid_from_xgmac == 1'b1) && (axi_str_tlast_from_xgmac == 1'b1)) begin
            pkg_count <= pkg_count + 1;
        end
    end
end


assign debug_signal[3:0] =  frame_len_ctr;
assign debug_signal[4] = frame_len_ctr_valid;
assign debug_signal[8:5] = state_wr;
assign debug_signal[12:9] = state_rd;
assign debug_signal[28:13] = cmd_in;
assign debug_signal[31:29] = cmd_out[3:0];
assign debug_signal[63:32] = axi_str_tdata_from_xgmac[31:0];
assign debug_signal[71:64] = axi_str_tkeep_from_xgmac;
assign debug_signal[72] = axi_str_tvalid_from_xgmac;
assign debug_signal[73] = rx_statistics_valid;
assign debug_signal[87:74] = rx_stats_vec_reg;
assign debug_signal[105:90] = axi_str_tdata_to_fifo[15:0];
assign debug_signal[113:106] = axi_str_tkeep_to_fifo;
assign debug_signal[114] = axi_str_tvalid_to_fifo;
assign debug_signal[115] = axi_str_tready_from_fifo;
//assign debug_signal[118:116] = rx_stats_vec_reg[2:0];
//assign debug_signal[116] = ap_ready;
//assign debug_signal[117] = ap_done;
//assign debug_signal[118] = ap_idle;
assign debug_signal[119] = axi_str_tlast_from_xgmac;
assign debug_signal[120] = axi_str_tlast_to_fifo;
assign debug_signal[123:121] = pkg_count;
assign debug_signal[124] = force_tlast_to_fifo;

assign debug_signal[125] = axis_rd_tvalid;
assign debug_signal[126] = axis_rd_tready;
assign debug_signal[127] = axis_rd_tlast;*/

endmodule 
