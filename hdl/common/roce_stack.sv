/**
  * Copyright (c) 2021, Systems Group, ETH Zurich
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

`timescale 1ns / 1ps

`define DBG_IBV

import lynxTypes::*;

/**
 * @brief   RoCE instantiation
 *
 * RoCE stack
 */
module roce_stack (
    input  logic                nclk,
    input  logic                nresetn,

    // Network interface
    AXI4S.s                     s_axis_rx,
    AXI4S.m                     m_axis_tx,

    // User command
    metaIntf.s                  s_rdma_sq,
    metaIntf.m                  m_rdma_ack,

    // Memory
    metaIntf.m                  m_rdma_rd_req,
    metaIntf.m                  m_rdma_wr_req,
    AXI4S.s                     s_axis_rdma_rd,
    AXI4S.m                     m_axis_rdma_wr,

    // Control
    metaIntf.s                  s_rdma_qp_interface,
    metaIntf.s                  s_rdma_conn_interface,
    input  logic [31:0]         local_ip_address,

    output logic                ibv_rx_pkg_count_valid,
    output logic [31:0]         ibv_rx_pkg_count_data,    
    output logic                ibv_tx_pkg_count_valid,
    output logic [31:0]         ibv_tx_pkg_count_data,    
    output logic                crc_drop_pkg_count_valid,
    output logic [31:0]         crc_drop_pkg_count_data,
    output logic                psn_drop_pkg_count_valid,
    output logic [31:0]         psn_drop_pkg_count_data,
    output logic                retrans_count_valid,
    output logic [31:0]         retrans_count_data
);

//
// SQ -- reframing of the user commands between rdma_flow and roce_v2_ip
//
metaIntf #(.STYPE(rdma_req_t)) rdma_sq ();
`ifdef VITIS_HLS
    logic [RDMA_REQ_BITS+32-RDMA_OPCODE_BITS-1:0] rdma_sq_data;
`else
    logic [RDMA_REQ_BITS-1:0] rdma_sq_data;
`endif

always_comb begin
`ifdef VITIS_HLS
  rdma_sq_data                                                      = 0;
  
  rdma_sq_data[0+:RDMA_OPCODE_BITS]                                 = rdma_sq.data.opcode;
  rdma_sq_data[32+:RDMA_QPN_BITS]                                   = rdma_sq.data.qpn;

  rdma_sq_data[32+RDMA_QPN_BITS+0+:1]                               = rdma_sq.data.host;
  rdma_sq_data[32+RDMA_QPN_BITS+1+:1]                               = rdma_sq.data.last;

  rdma_sq_data[32+RDMA_QPN_BITS+2+:RDMA_OFFS_BITS]                  = rdma_sq.data.offs;

  rdma_sq_data[32+RDMA_QPN_BITS+2+RDMA_OFFS_BITS+:RDMA_MSG_BITS]    = rdma_sq.data.msg;
`else
  rdma_sq_data                                                      = 0;

  rdma_sq_data[0+:RDMA_OPCODE_BITS]                                 = rdma_sq.data.opcode;
  rdma_sq_data[RDMA_OPCODE_BITS+:RDMA_QPN_BITS]                     = rdma_sq.data.qpn;
  
  rdma_sq_data[RDMA_OPCODE_BITS+RDMA_QPN_BITS+0+:1]                 = rdma_sq.data.host;
  rdma_sq_data[RDMA_OPCODE_BITS+RDMA_QPN_BITS+1+:1]                 = rdma_sq.data.last;

  rdma_sq_data[RDMA_OPCODE_BITS+RDMA_QPN_BITS+2+:RDMA_OFFS_BITS]    = rdma_sq.data.offs;

  rdma_sq_data[RDMA_OPCODE_BITS+RDMA_QPN_BITS+2+RDMA_OFFS_BITS+:RDMA_MSG_BITS] = rdma_sq.data.msg;
`endif
end

//
// RD and WR interface - reframing of memory access commands 
// 
logic [RDMA_BASE_REQ_BITS-1:0] rd_cmd_data;
logic [RDMA_BASE_REQ_BITS-1:0] wr_cmd_data;

assign m_rdma_rd_req.data.vaddr             = rd_cmd_data[0+:VADDR_BITS];
assign m_rdma_rd_req.data.len               = rd_cmd_data[VADDR_BITS+:LEN_BITS];
assign m_rdma_rd_req.data.ctl               = rd_cmd_data[VADDR_BITS+LEN_BITS+:1];
assign m_rdma_rd_req.data.stream            = rd_cmd_data[VADDR_BITS+LEN_BITS+1+:1];
assign m_rdma_rd_req.data.sync              = rd_cmd_data[VADDR_BITS+LEN_BITS+2+:1];
assign m_rdma_rd_req.data.host              = rd_cmd_data[VADDR_BITS+LEN_BITS+3+:1];
assign m_rdma_rd_req.data.dest              = rd_cmd_data[VADDR_BITS+LEN_BITS+4+:DEST_BITS];
assign m_rdma_rd_req.data.pid               = rd_cmd_data[VADDR_BITS+LEN_BITS+4+DEST_BITS+:PID_BITS];
assign m_rdma_rd_req.data.vfid              = rd_cmd_data[VADDR_BITS+LEN_BITS+4+DEST_BITS+PID_BITS+:N_REGIONS_BITS];

assign m_rdma_wr_req.data.vaddr             = wr_cmd_data[0+:VADDR_BITS];
assign m_rdma_wr_req.data.len               = wr_cmd_data[VADDR_BITS+:LEN_BITS];
assign m_rdma_wr_req.data.ctl               = wr_cmd_data[VADDR_BITS+LEN_BITS+:1];
assign m_rdma_wr_req.data.stream            = wr_cmd_data[VADDR_BITS+LEN_BITS+1+:1];
assign m_rdma_wr_req.data.sync              = wr_cmd_data[VADDR_BITS+LEN_BITS+2+:1];
assign m_rdma_wr_req.data.host              = wr_cmd_data[VADDR_BITS+LEN_BITS+3+:1];
assign m_rdma_wr_req.data.dest              = wr_cmd_data[VADDR_BITS+LEN_BITS+4+:DEST_BITS];
assign m_rdma_wr_req.data.pid               = wr_cmd_data[VADDR_BITS+LEN_BITS+4+DEST_BITS+:PID_BITS];
assign m_rdma_wr_req.data.vfid              = wr_cmd_data[VADDR_BITS+LEN_BITS+4+DEST_BITS+PID_BITS+:N_REGIONS_BITS];

//
// ACKs - reframing of rdma_acks between rdma_flow and roce_v2_ip
//
metaIntf #(.STYPE(rdma_ack_t)) rdma_ack ();
logic [RDMA_ACK_BITS-1:0] ack_meta_data;
assign rdma_ack.data.rd = ack_meta_data[0];
assign rdma_ack.data.cmplt = 1'b0;
assign rdma_ack.data.pid = ack_meta_data[1+:PID_BITS];
assign rdma_ack.data.vfid = ack_meta_data[1+PID_BITS+:N_REGIONS_BITS]; 
assign rdma_ack.data.ssn = ack_meta_data[1+RDMA_ACK_QPN_BITS+:RDMA_ACK_PSN_BITS]; // msn

// Flow control - controls flow of user commands 
rdma_flow inst_rdma_flow (
    .aclk(nclk),
    .aresetn(nresetn),
    .s_req(s_rdma_sq),
    .m_req(rdma_sq),
    .s_ack(rdma_ack),
    .m_ack(m_rdma_ack)
);
/*
ila_ack inst_ila_ack (
    .clk(nclk),
    .probe0(rdma_sq.valid),
    .probe1(rdma_sq.ready),
    .probe2(rdma_sq.data), // 512
    .probe3(rdma_ack.valid)
);
*/

// Definition of the AXI-bus from roce-ip to icrc 
AXI4S #(.AXI4S_DATA_BITS(AXI_NET_BITS)) roce_to_icrc();

// Integrate the ICRC-module on the outgoing datapath 
icrc inst_icrc (
    .m_axis_rx(roce_to_icrc), 
    .m_axis_tx(m_axis_tx), 
    .nclk(nclk), 
    .nresetn(nresetn)
);

// ChipScope around the ICRC 
ila_icrc inst_ila_icrc (
    .clk(nclk),
    .probe0(s_axis_rx.tvalid), // 1
    .probe1(s_axis_rx.tready), // 1
    .probe2(s_axis_rx.tlast),  // 1
    .probe3(s_axis_rx.tkeep),  // 64
    .probe4(s_axis_rx.tdata),  // 512
    .probe5(m_axis_tx.tvalid),    // 1
    .probe6(m_axis_tx.tready),    // 1
    .probe7(m_axis_tx.tlast),     // 1
    .probe8(m_axis_tx.tkeep),     // 64
    .probe9(m_axis_tx.tdata),     // 512
    .probe10(roce_to_icrc.tvalid), // 1
    .probe11(roce_to_icrc.tready), // 1
    .probe12(roce_to_icrc.tlast),  // 1
    .probe13(roce_to_icrc.tkeep),  // 64
    .probe14(roce_to_icrc.tdata)   // 512
);


//
// DBG
// 
metaIntf #(.STYPE(logic [87:0])) axis_dbg ();
assign axis_dbg.ready = 1'b1;

// ila_dbg_rdma inst_ila_dbg_rdma (
//     .clk(nclk),
//     .probe0(axis_dbg.valid),
//     .probe1(axis_dbg.data[0+:4]), // 4
//     .probe2(axis_dbg.data[4+:24]), // 24
//     .probe3(axis_dbg.data[28+:24]), // 24
//     .probe4(axis_dbg.data[52+:5]), // 5
//     .probe5(s_axis_rx.tvalid),
//     .probe6(s_axis_rx.tready),
//     .probe7(s_axis_rx.tdata), // 512
//     .probe8(s_axis_rx.tlast),
//     .probe9(m_axis_tx.tvalid),
//     .probe10(m_axis_tx.tready),
//     .probe11(m_axis_tx.tdata), // 512
//     .probe12(m_axis_tx.tlast),
//     .probe13(rdma_ack.valid),
//     .probe14(rdma_ack.ready),
//     .probe15(ack_meta_data), // 40
//     .probe16(psn_drop_pkg_count_data), // 32
//     .probe17(psn_drop_pkg_count_valid)
// );

/*
create_ip -name ila -vendor xilinx.com -library ip -version 6.2 -module_name ila_dbg_rdma
set_property -dict [list CONFIG.C_PROBE16_WIDTH {32} CONFIG.C_PROBE15_WIDTH {40} CONFIG.C_PROBE11_WIDTH {512} CONFIG.C_PROBE7_WIDTH {512} CONFIG.C_PROBE4_WIDTH {5} CONFIG.C_PROBE3_WIDTH {24} CONFIG.C_PROBE2_WIDTH {24} CONFIG.C_PROBE1_WIDTH {4} CONFIG.C_DATA_DEPTH {32768} CONFIG.C_NUM_OF_PROBES {18} CONFIG.Component_Name {ila_dbg_rdma} CONFIG.C_EN_STRG_QUAL {1} CONFIG.C_PROBE17_MU_CNT {2} CONFIG.C_PROBE16_MU_CNT {2} CONFIG.C_PROBE15_MU_CNT {2} CONFIG.C_PROBE14_MU_CNT {2} CONFIG.C_PROBE13_MU_CNT {2} CONFIG.C_PROBE12_MU_CNT {2} CONFIG.C_PROBE11_MU_CNT {2} CONFIG.C_PROBE10_MU_CNT {2} CONFIG.C_PROBE9_MU_CNT {2} CONFIG.C_PROBE8_MU_CNT {2} CONFIG.C_PROBE7_MU_CNT {2} CONFIG.C_PROBE6_MU_CNT {2} CONFIG.C_PROBE5_MU_CNT {2} CONFIG.C_PROBE4_MU_CNT {2} CONFIG.C_PROBE3_MU_CNT {2} CONFIG.C_PROBE2_MU_CNT {2} CONFIG.C_PROBE1_MU_CNT {2} CONFIG.C_PROBE0_MU_CNT {2} CONFIG.ALL_PROBE_SAME_MU_CNT {2}] [get_ips ila_dbg_rdma]
*/

//
// RoCE stack - HLS stack for RDMA-networking
//
rocev2_ip rocev2_inst(
    .ap_clk(nclk), // input aclk
    .ap_rst_n(nresetn), // input aresetn
    
`ifdef VITIS_HLS

    // Debug
`ifdef DBG_IBV
    .m_axis_dbg_TVALID(axis_dbg.valid),
    .m_axis_dbg_TREADY(axis_dbg.ready),
    .m_axis_dbg_TDATA(axis_dbg.data),
`endif

    // RX - network input 
    .s_axis_rx_data_TVALID(s_axis_rx.tvalid),
    .s_axis_rx_data_TREADY(s_axis_rx.tready),
    .s_axis_rx_data_TDATA(s_axis_rx.tdata),
    .s_axis_rx_data_TKEEP(s_axis_rx.tkeep),
    .s_axis_rx_data_TLAST(s_axis_rx.tlast),
    
    // TX - network output
    .m_axis_tx_data_TVALID(roce_to_icrc.tvalid),
    .m_axis_tx_data_TREADY(roce_to_icrc.tready),
    .m_axis_tx_data_TDATA(roce_to_icrc.tdata),
    .m_axis_tx_data_TKEEP(roce_to_icrc.tkeep),
    .m_axis_tx_data_TLAST(roce_to_icrc.tlast),
    
    // User commands    
    .s_axis_sq_meta_TVALID(rdma_sq.valid),
    .s_axis_sq_meta_TREADY(rdma_sq.ready),
    .s_axis_sq_meta_TDATA(rdma_sq_data), 
    
    // Memory
    // Write commands
    .m_axis_mem_write_cmd_TVALID(m_rdma_wr_req.valid),
    .m_axis_mem_write_cmd_TREADY(m_rdma_wr_req.ready),
    //.m_axis_mem_write_cmd_TDATA(m_rdma_wr_req.data),
    .m_axis_mem_write_cmd_TDATA(wr_cmd_data),
    // Read commands
    .m_axis_mem_read_cmd_TVALID(m_rdma_rd_req.valid),
    .m_axis_mem_read_cmd_TREADY(m_rdma_rd_req.ready),
    //.m_axis_mem_read_cmd_TDATA(m_rdma_rd_req.data),
    .m_axis_mem_read_cmd_TDATA(rd_cmd_data),
    // Write data
    .m_axis_mem_write_data_TVALID(m_axis_rdma_wr.tvalid),
    .m_axis_mem_write_data_TREADY(m_axis_rdma_wr.tready),
    .m_axis_mem_write_data_TDATA(m_axis_rdma_wr.tdata),
    .m_axis_mem_write_data_TKEEP(m_axis_rdma_wr.tkeep),
    .m_axis_mem_write_data_TLAST(m_axis_rdma_wr.tlast),
    // Read data
    .s_axis_mem_read_data_TVALID(s_axis_rdma_rd.tvalid),
    .s_axis_mem_read_data_TREADY(s_axis_rdma_rd.tready),
    .s_axis_mem_read_data_TDATA(s_axis_rdma_rd.tdata),
    .s_axis_mem_read_data_TKEEP(s_axis_rdma_rd.tkeep),
    .s_axis_mem_read_data_TLAST(s_axis_rdma_rd.tlast),

    // QP intf
    .s_axis_qp_interface_TVALID(s_rdma_qp_interface.valid),
    .s_axis_qp_interface_TREADY(s_rdma_qp_interface.ready),
    .s_axis_qp_interface_TDATA(s_rdma_qp_interface.data),
    .s_axis_qp_conn_interface_TVALID(s_rdma_conn_interface.valid),
    .s_axis_qp_conn_interface_TREADY(s_rdma_conn_interface.ready),
    .s_axis_qp_conn_interface_TDATA(s_rdma_conn_interface.data),

    // ACK
    .m_axis_rx_ack_meta_TVALID(rdma_ack.valid),
    .m_axis_rx_ack_meta_TREADY(rdma_ack.ready),
    .m_axis_rx_ack_meta_TDATA(ack_meta_data),

    // IP
    .local_ip_address({local_ip_address,local_ip_address,local_ip_address,local_ip_address}), //Use IPv4 addr

    .regIbvCountRx(ibv_rx_pkg_count_data),
    .regIbvCountRx_ap_vld(ibv_rx_pkg_count_valid),
    .regIbvCountTx(ibv_tx_pkg_count_data),
    .regIbvCountTx_ap_vld(ibv_tx_pkg_count_valid),
    .regCrcDropPkgCount(crc_drop_pkg_count_data),
    .regCrcDropPkgCount_ap_vld(crc_drop_pkg_count_valid),
    .regInvalidPsnDropCount(psn_drop_pkg_count_data),
    .regInvalidPsnDropCount_ap_vld(psn_drop_pkg_count_valid),
    .regRetransCount(retrans_count_data),
    .regRetransCount_ap_vld(retrans_count_valid)
    
`else

    // Debug
`ifdef DBG_IBV
    .m_axis_dbg_TVALID(),
    .m_axis_dbg_TREADY(),
    .m_axis_dbg_TDATA(),
`endif

    // RX
    .s_axis_rx_data_TVALID(s_axis_rx.tvalid),
    .s_axis_rx_data_TREADY(s_axis_rx.tready),
    .s_axis_rx_data_TDATA(s_axis_rx.tdata),
    .s_axis_rx_data_TKEEP(s_axis_rx.tkeep),
    .s_axis_rx_data_TLAST(s_axis_rx.tlast),
    
    // TX
    .m_axis_tx_data_TVALID(roce_to_icrc.tvalid),
    .m_axis_tx_data_TREADY(roce_to_icrc.tready),
    .m_axis_tx_data_TDATA(roce_to_icrc.tdata),
    .m_axis_tx_data_TKEEP(roce_to_icrc.tkeep),
    .m_axis_tx_data_TLAST(roce_to_icrc.tlast),
    
    // User commands    
    .s_axis_sq_meta_V_TVALID(rdma_sq.valid),
    .s_axis_sq_meta_V_TREADY(rdma_sq.ready),
    .s_axis_sq_meta_V_TDATA(rdma_sq_data), 
    
    // Memory
    // Write commands
    .m_axis_mem_write_cmd_V_TVALID(m_rdma_wr_req.valid),
    .m_axis_mem_write_cmd_V_TREADY(m_rdma_wr_req.ready),
    //.m_axis_mem_write_cmd_V_TDATA(m_rdma_wr_req.data),
    .m_axis_mem_write_cmd_V_TDATA(wr_cmd_data),
    // Read commands
    .m_axis_mem_read_cmd_V_TVALID(m_rdma_rd_req.valid),
    .m_axis_mem_read_cmd_V_TREADY(m_rdma_rd_req.ready),
    //.m_axis_mem_read_cmd_V_TDATA(m_rdma_rd_req.data),
    .m_axis_mem_read_cmd_V_TDATA(rd_cmd_data),
    // Write data
    .m_axis_mem_write_data_TVALID(m_axis_rdma_wr.tvalid),
    .m_axis_mem_write_data_TREADY(m_axis_rdma_wr.tready),
    .m_axis_mem_write_data_TDATA(m_axis_rdma_wr.tdata),
    .m_axis_mem_write_data_TKEEP(m_axis_rdma_wr.tkeep),
    .m_axis_mem_write_data_TLAST(m_axis_rdma_wr.tlast),
    // Read data
    .s_axis_mem_read_data_TVALID(s_axis_rdma_rd.tvalid),
    .s_axis_mem_read_data_TREADY(s_axis_rdma_rd.tready),
    .s_axis_mem_read_data_TDATA(s_axis_rdma_rd.tdata),
    .s_axis_mem_read_data_TKEEP(s_axis_rdma_rd.tkeep),
    .s_axis_mem_read_data_TLAST(s_axis_rdma_rd.tlast),

    // QP intf
    .s_axis_qp_interface_V_TVALID(s_rdma_qp_interface.valid),
    .s_axis_qp_interface_V_TREADY(s_rdma_qp_interface.ready),
    .s_axis_qp_interface_V_TDATA(s_rdma_qp_interface.data),
    .s_axis_qp_conn_interface_V_TVALID(s_rdma_conn_interface.valid),
    .s_axis_qp_conn_interface_V_TREADY(s_rdma_conn_interface.ready),
    .s_axis_qp_conn_interface_V_TDATA(s_rdma_conn_interface.data),

    // ACK
    .m_axis_rx_ack_meta_V_TVALID(rdma_ack.valid),
    .m_axis_rx_ack_meta_V_TREADY(rdma_ack.ready),
    .m_axis_rx_ack_meta_V_TDATA(ack_meta_data),

    // IP
    .local_ip_address_V({local_ip_address,local_ip_address,local_ip_address,local_ip_address}), //Use IPv4 addr

    .regIbvCountRx_V(ibv_rx_pkg_count_data),
    .regIbvCountRx_V_ap_vld(ibv_rx_pkg_count_valid),
    .regIbvCountTx_V(ibv_tx_pkg_count_data),
    .regIbvCountTx_V_ap_vld(ibv_tx_pkg_count_valid),
    .regCrcDropPkgCount_V(crc_drop_pkg_count_data),
    .regCrcDropPkgCount_V_ap_vld(crc_drop_pkg_count_valid),
    .regInvalidPsnDropCount_V(psn_drop_pkg_count_data),
    .regInvalidPsnDropCount_V_ap_vld(psn_drop_pkg_count_valid),
    .regRetransCount_V(retrans_count_data),
    .regRetransCount_V_ap_vld(retrans_count_valid)

`endif
);


endmodule
