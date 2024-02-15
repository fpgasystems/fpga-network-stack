import lynxTypes::*;

module rdma_flow (
    // Incoming user commands 
    metaIntf.s                  s_req,
    
    // user commands to roce_v2_ip
    metaIntf.m                  m_req,

    // Acks from roce_v2_ip
    metaIntf.s                  s_ack,

    // Acks as responses to the user commands 
    metaIntf.m                  m_ack,

    // Incoming clock and reset
    input  logic                aclk,
    input  logic                aresetn
);

// localparams for bitwidths
localparam integer RDMA_N_OST = 16;
localparam integer RDMA_OST_BITS = $clog2(RDMA_N_OST);

// READ and WRITE opcode 
localparam integer RD_OP = 0;
localparam integer WR_OP = 1;

// Definition of status variables for C and N (head, tail and issue)
logic [1:0][N_REGIONS_BITS-1:0][PID_BITS-1:0][RDMA_OST_BITS-1:0] head_C = 0, head_N;
logic [1:0][N_REGIONS_BITS-1:0][PID_BITS-1:0][RDMA_OST_BITS-1:0] tail_C = 0, tail_N;
logic [1:0][N_REGIONS_BITS-1:0][PID_BITS-1:0] issued_C = 0, issued_N;

// Further variables C and N for ACKs etc. 
logic ssn_rd_C = 0, ssn_rd_N;
logic [N_REGIONS_BITS-1:0] ack_vfid_C, ack_vfid_N;
logic [PID_BITS-1:0] ack_pid_C, ack_pid_N;
logic ack_rd_C, ack_rd_N;

// Control and Data signals for the memory buffer 
// Way selector
logic [3:0] ssn_wr;
// Memory Address 
logic [1+N_REGIONS_BITS+PID_BITS+RDMA_OST_BITS-1:0] ssn_addr;
// Data In 
logic [31:0] ssn_in;
// Data Out
logic [31:0] ssn_out;

// Definition of ACKs for reads 
logic ack_rd;
logic [N_REGIONS_BITS-1:0] ack_vfid;
logic [PID_BITS-1:0] ack_pid;

// Definition of Requests for reads
logic req_rd;
logic [N_REGIONS_BITS-1:0] req_vfid;
logic [PID_BITS-1:0] req_pid;

metaIntf #(.STYPE(rdma_ack_t)) ack_que_in ();

// Definition of a memory as buffer 
ram_sp_nc #(
    .ADDR_BITS(1+N_REGIONS_BITS+PID_BITS+RDMA_OST_BITS),
    .DATA_BITS(32)
) inst_ssn (
    .clk(aclk),
    .a_en(1'b1),
    .a_we(ssn_wr),
    .a_addr(ssn_addr),
    .a_data_in(ssn_in),
    .a_data_out(ssn_out)
);

// REG - at every clock counter, switch from N to C
always_ff @(posedge aclk) begin
    if(~aresetn) begin
        head_C <= 0;
        tail_C <= 0;
        issued_C <= 0;

        ssn_rd_C <= 1'b0;
        ack_vfid_C <= 'X;
        ack_pid_C <= 'X;
        ack_rd_C <= 'X;
    end
    else begin
        head_C <= head_N;
        tail_C <= tail_N;
        issued_C <= issued_N;
        
        ssn_rd_C <= ssn_rd_N;
        ack_vfid_C <= ack_vfid_N;
        ack_pid_C <= ack_pid_N;
        ack_rd_C <= ack_rd_N;
    end
end

// Service
always_comb begin
    // Default case: Loop-back from C to N (opposed to always_ff)
    head_N = head_C;
    tail_N = tail_C;
    issued_N = issued_C;
    ssn_rd_N = 1'b0;
    ack_vfid_N = ack_vfid_C;
    ack_pid_N = ack_pid_C;
    ack_rd_N = ack_rd_C;
    
    ssn_wr = 0;
    ssn_addr = 0;
    ssn_in = {6'd0, s_req.data.cmplt, s_req.data.last, s_req.data.ssn};

    s_ack.ready = 1'b0;
    s_req.ready = 1'b0;

    if(s_ack.valid) begin
        // Service ack
        s_ack.ready = 1'b1;

        tail_N[ack_rd][ack_vfid][ack_pid] = tail_C[ack_rd][ack_vfid][ack_pid] + 1;
        if(head_C[ack_rd][ack_vfid][ack_pid] == tail_N[ack_rd][ack_vfid][ack_pid]) begin
            issued_N[ack_rd][ack_vfid][ack_pid] = 1'b0;
        end

        ssn_rd_N = 1'b1;
        ssn_addr = {ack_rd, ack_vfid[N_REGIONS_BITS-1:0], ack_pid, tail_C[ack_rd][ack_vfid][ack_pid]};
        
        ack_vfid_N = ack_vfid;
        ack_pid_N = ack_pid;
        ack_rd_N = ack_rd;
    end
    else if(s_req.valid) begin
        // Service req
        if((!issued_C[req_rd][req_vfid][req_pid] || (head_C[req_rd][req_vfid][req_pid] != tail_C[req_rd][req_vfid][req_pid])) && m_req.ready) begin
            s_req.ready = 1'b1;

            head_N[req_rd][req_vfid][req_pid] = head_C[req_rd][req_vfid][req_pid] + 1;
            issued_N[req_rd][req_vfid][req_pid] = 1'b1;

            ssn_wr = ~0;
            ssn_addr = {req_rd, req_vfid[N_REGIONS_BITS-1:0], req_pid, head_C[req_rd][req_vfid][req_pid]};
        end
    end
end

always_comb begin
    m_req.valid = s_req.valid & s_req.ready;

    m_req.data = s_req.data;
    m_req.data.offs = head_C[req_rd][req_vfid][req_pid];
end

// DP
assign ack_que_in.valid = ssn_rd_C && ssn_out[RDMA_MSN_BITS];
assign ack_que_in.data.rd = ack_rd_C;
assign ack_que_in.data.cmplt = ssn_out[RDMA_MSN_BITS+1];
assign ack_que_in.data.vfid = ack_vfid_C;
assign ack_que_in.data.pid = ack_pid_C;
assign ack_que_in.data.ssn = ssn_out[RDMA_MSN_BITS-1:0];

// ACK queue
queue_meta #(
    .QDEPTH(RDMA_N_OST)
) inst_cq (
    .aclk(aclk),
    .aresetn(aresetn),
    .s_meta(ack_que_in),
    .m_meta(m_ack)
);

// Internal
assign ack_rd = ~s_ack.data.rd;
assign ack_pid = s_ack.data.pid;
assign ack_vfid = s_ack.data.vfid;

assign req_rd = s_req.data.opcode == RC_RDMA_READ_REQUEST;
assign req_pid = s_req.data.qpn[0+:PID_BITS];
assign req_vfid = s_req.data.qpn[PID_BITS+:N_REGIONS_BITS];

/*
ila_req inst_ila_req (
    .clk(aclk),
    .probe0(s_req.valid),
    .probe1(s_req.ready),
    .probe2(s_req.data), // 512
    .probe3(m_req.valid),
    .probe4(m_req.ready), 
    .probe5(head_C[0][0][0]), // 4 
    .probe6(tail_C[0][0][0]), // 4
    .probe7(issued_C[0][0][0]),
    .probe8(ssn_rd_C),
    .probe9(ack_vfid_C[0]),
    .probe10(ack_pid_C[0]), // 6
    .probe11(ack_rd_C),
    .probe12(s_ack.valid),
    .probe13(s_ack.ready),
    .probe14(s_ack.data), // 40
    .probe15(ssn_wr), // 4
    .probe16(ssn_addr), // 12
    .probe17(ssn_in), // 32
    .probe18(ssn_out) // 32
);
*/

endmodule