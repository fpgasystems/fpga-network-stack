`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/13/2013 03:24:40 PM
// Design Name: 
// Module Name: stream_tg
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


module stream_tg #(
    parameter DATA_WIDTH = 64,
    parameter KEEP_WIDTH = 8,
    parameter START_ADDR = 0,
    parameter START_DATA = 0,
    parameter BTT = 23'd8,
    parameter DRR = 1'b0,
    parameter DSA = 6'd0
)
(
    input aclk,
    input aresetn,
    output [71:0] write_cmd,
    output write_cmd_valid,
    input write_cmd_ready,
    output [DATA_WIDTH-1: 0] write_data,
    output write_data_valid,
    input write_data_ready,
    output [KEEP_WIDTH-1: 0] write_data_keep,
    output write_data_last,
    output [71:0] read_cmd,
    output read_cmd_valid,
    input read_cmd_ready,
    input [DATA_WIDTH-1: 0] read_data,
    input read_data_valid,
    input [KEEP_WIDTH-1: 0] read_data_keep,
    input read_data_last,
    output read_data_ready,
    input [7:0] read_sts_data,
    input read_sts_valid,
    output read_sts_ready,
    input [31:0] write_sts_data,
    input write_sts_valid,
    output write_sts_ready,
    output compare_error
);

//local parameters for test state machine
localparam ITERATIONS = 1; //test iterations. Each iteration involves state transition from WRITE TO COMPARE
localparam IDLE = 3'd0;
localparam START = 3'd1;
localparam WRITE_CMD = 3'd2;
localparam WRITE_DATA = 3'd3;
localparam READ_CMD = 3'd4;
localparam READ_DATA = 3'd5;
localparam COMPARE = 3'd6;
localparam FINISH = 3'd7;

//localparam BTT = 23'd16;
localparam TYPE = 1'd1;
localparam EOF = 1'd1;
//localparam DRR = 1'd1;
localparam TAG = 4'd0;
localparam RSVD = 4'd0;

reg [31:0] iter_count_r;
reg [2:0] test_state_r;

reg [71:0] write_cmd_r;
reg write_cmd_valid_r;
reg [DATA_WIDTH-1: 0] write_data_r, write_data_r1;
reg write_data_valid_r;
reg [KEEP_WIDTH-1: 0] write_data_keep_r, write_data_keep_r1;
reg write_data_last_r;
reg [71:0] read_cmd_r;
reg read_cmd_valid_r;
reg [DATA_WIDTH-1: 0] read_data_r, read_data_r1;
reg read_data_valid_r;
reg [KEEP_WIDTH-1: 0] read_data_keep_r, read_data_keep_r1;
reg read_data_last_r, read_data_last_r1;

reg compare_error_r;

assign compare_error = compare_error_r;
assign write_cmd = write_cmd_r;
assign write_cmd_valid = write_cmd_valid_r;
assign write_data = write_data_r;
assign write_data_valid = write_data_valid_r;
assign write_data_keep = write_data_keep_r;
assign write_data_last = write_data_last_r;
assign read_cmd = read_cmd_r;
assign read_cmd_valid = read_cmd_valid_r;
assign read_data_ready = 1'b1;
assign read_sts_ready = 1'b1;
assign write_sts_ready = 1'b1;
//main state machine
always @(posedge aclk)
    if (~aresetn) begin
        write_cmd_r[22:0] <= BTT;
        write_cmd_r[23] <= TYPE;
        write_cmd_r[29:24] <= DSA;
        write_cmd_r[30] <= EOF;
        write_cmd_r[31] <= DRR;
        write_cmd_r[63:32] <= START_ADDR;
        write_cmd_r[67:64] <= TAG;
        write_cmd_r[71:68] <= RSVD;      
        
        read_cmd_r[22:0] <= BTT;
        read_cmd_r[23] <= TYPE;
        read_cmd_r[29:24] <= DSA;
        read_cmd_r[30] <= EOF;
        read_cmd_r[31] <= DRR;
        read_cmd_r[63:32] <= START_ADDR;
        read_cmd_r[67:64] <= TAG;
        read_cmd_r[71:68] <= RSVD;
        
        write_data_r <= START_DATA;
        
        write_cmd_valid_r <= 1'b0;
        write_data_valid_r <= 1'b0;
        read_cmd_valid_r <= 1'b0;
        
        write_data_keep_r <= 0;
        write_data_last_r <= 1'b0;
        
        //iter_count_r <= 0;
        
        test_state_r <= IDLE;
        
        compare_error_r <= 1'b0;
        
        read_data_r <= 0;
        read_data_keep_r <= 0;
        read_data_last_r <= 0;
    end
    else
        case (test_state_r)
            IDLE:   begin test_state_r <= START; end
            START:  begin 
                        test_state_r <= WRITE_CMD;
                        write_cmd_valid_r <= 1'b1; 
                    end
            WRITE_CMD: begin
                        if (write_cmd_ready) begin
                            write_cmd_valid_r <= 1'b0;
                            test_state_r <= WRITE_DATA;
                            write_data_keep_r <= {{KEEP_WIDTH}{1'b1}};
                            write_data_last_r <= 1'b1;
                            write_data_valid_r <= 1'b1;
                            write_cmd_r[63:32] <= write_cmd_r[63:32] + 1;
                        end
                       end
            WRITE_DATA: begin
                            if (write_data_ready) begin
                                write_data_valid_r <= 1'b0;
                                write_data_last_r <= 1'b0;
                            end
                            if (write_sts_valid & write_sts_data[7]) begin
                                test_state_r <= READ_CMD;
                                read_cmd_valid_r <= 1'b1;
                            end
                        end
            READ_CMD: begin
                        if (read_cmd_ready) begin
                            read_cmd_valid_r <= 1'b0;
                        end
                        if (read_sts_valid & read_sts_data[7]) begin
                            test_state_r <= READ_DATA;
                            read_cmd_r[63:32] <= read_cmd_r[63:32] + 1;
                        end
                      end
            READ_DATA: begin
                           if (read_data_valid) begin
                                read_data_r <= read_data;
                                read_data_keep_r <= read_data_keep;
                                read_data_last_r <= read_data_last;
                           end
                           if (read_data_last_r) begin
                                test_state_r <= COMPARE;
                           end
                       end
            COMPARE: begin
                        if ((read_data_r1 != write_data_r1) | (read_data_keep_r1 != write_data_keep_r1)) begin
                            compare_error_r <= 1'b1;
                        end
                        test_state_r <= FINISH;
                     end
            FINISH: begin
                        //test_state_r <= FINISH;
                        test_state_r <= START;
                    end    
        endcase 
        
        always @(posedge aclk) begin
            read_data_r1 <= read_data_r;
            read_data_keep_r1 <= read_data_keep_r;
            write_data_r1 <= write_data_r;
            write_data_keep_r1 <= write_data_keep_r;
        end

endmodule
