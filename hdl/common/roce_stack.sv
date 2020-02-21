/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
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
`default_nettype none

`include "davos_types.svh"

//`define POINTER_CHASING

module roce_stack #(
    parameter ROCE_EN = 1,
    parameter WIDTH = 64
)(
    input wire          net_clk,
    input wire          net_aresetn,

    // network interface streams
    axi_stream.slave        s_axis_rx_data,
    axi_stream.master       m_axis_tx_data,

    //RoCE Interface
    //DMA
    axis_meta.rmaster       m_axis_mem_read_cmd,
    axis_meta.rmaster       m_axis_mem_write_cmd,
    axi_stream.slave        s_axis_mem_read_data,
    axi_stream.rmaster      m_axis_mem_write_data,

    //Role
    //axis_mem_cmd.master     m_axis_roce_role_rx_cmd,
    axis_meta.slave         s_axis_tx_meta,
    //axi_stream.master       m_axis_roce_role_rx_data,
    axi_stream.slave        s_axis_tx_data,
    
    axis_meta.master    m_axis_rx_pcmeta,
    axis_meta.slave     s_axis_tx_pcmeta,


   //Config
    axis_meta.slave  s_axis_qp_interface,
    axis_meta.slave  s_axis_qp_conn_interface,


   input wire[31:0]     local_ip_address,
   output logic         crc_drop_pkg_count_valid,
   output logic[31:0]   crc_drop_pkg_count_data,
   output logic         psn_drop_pkg_count_valid,
   output logic[31:0]   psn_drop_pkg_count_data


 );

generate
if (ROCE_EN == 1) begin
rocev2_ip rocev2_inst(
    .ap_clk(net_clk), // input aclk
    .ap_rst_n(net_aresetn), // input aresetn
    //RX
//`ifdef IP_VERSION4
     //IPv4
    .s_axis_rx_data_TVALID(s_axis_rx_data.valid),
    .s_axis_rx_data_TREADY(s_axis_rx_data.ready),
    .s_axis_rx_data_TDATA(s_axis_rx_data.data),
    .s_axis_rx_data_TKEEP(s_axis_rx_data.keep),
    .s_axis_rx_data_TLAST(s_axis_rx_data.last),
/*`else
    //IPv6
    .s_axis_rx_data_TVALID(axis_iph_to_rocev6_slice_tvalid),
    .s_axis_rx_data_TREADY(axis_iph_to_rocev6_slice_tready),
    .s_axis_rx_data_TDATA(axis_iph_to_rocev6_slice_tdata),
    .s_axis_rx_data_TKEEP(axis_iph_to_rocev6_slice_tkeep),
    .s_axis_rx_data_TLAST(axis_iph_to_rocev6_slice_tlast),
`endif*/
    
    /*.m_axis_rx_data_TVALID(),
    .m_axis_rx_data_TREADY(1'b1),
    .m_axis_rx_data_TDATA(),
    .m_axis_rx_data_TKEEP(),
    .m_axis_rx_data_TLAST(),*/
    //TX
    .s_axis_tx_meta_V_TVALID(s_axis_tx_meta.valid),
    .s_axis_tx_meta_V_TREADY(s_axis_tx_meta.ready),
    .s_axis_tx_meta_V_TDATA(s_axis_tx_meta.data),
    .s_axis_tx_data_TVALID(axis_tx_data.valid),
    .s_axis_tx_data_TREADY(axis_tx_data.ready),
    .s_axis_tx_data_TDATA(axis_tx_data.data),
    .s_axis_tx_data_TKEEP(axis_tx_data.keep),
    .s_axis_tx_data_TLAST(axis_tx_data.last),
    
    // IPv4
    .m_axis_tx_data_TVALID(m_axis_tx_data.valid),
    .m_axis_tx_data_TREADY(m_axis_tx_data.ready),
    .m_axis_tx_data_TDATA(m_axis_tx_data.data),
    .m_axis_tx_data_TKEEP(m_axis_tx_data.keep),
    .m_axis_tx_data_TLAST(m_axis_tx_data.last),

    //Memory
    .m_axis_mem_write_cmd_TVALID(m_axis_mem_write_cmd.valid),
    .m_axis_mem_write_cmd_TREADY(m_axis_mem_write_cmd.ready),
    .m_axis_mem_write_cmd_TDATA(m_axis_mem_write_cmd.data),
    .m_axis_mem_write_cmd_TDEST(m_axis_mem_write_cmd.dest),
    
    .m_axis_mem_read_cmd_TVALID(m_axis_mem_read_cmd.valid),
    .m_axis_mem_read_cmd_TREADY(m_axis_mem_read_cmd.ready),
    .m_axis_mem_read_cmd_TDATA(m_axis_mem_read_cmd.data),
    .m_axis_mem_read_cmd_TDEST(m_axis_mem_read_cmd.dest),
    // Memory Write
    .m_axis_mem_write_data_TVALID(axis_mem_write_data.valid),
    .m_axis_mem_write_data_TREADY(axis_mem_write_data.ready),
    .m_axis_mem_write_data_TDATA(axis_mem_write_data.data),
    .m_axis_mem_write_data_TKEEP(axis_mem_write_data.keep),
    .m_axis_mem_write_data_TLAST(axis_mem_write_data.last),
    .m_axis_mem_write_data_TDEST(axis_mem_write_data.dest),
    // Memory Read
    .s_axis_mem_read_data_TVALID(axis_mem_read_data.valid),
    .s_axis_mem_read_data_TREADY(axis_mem_read_data.ready),
    .s_axis_mem_read_data_TDATA(axis_mem_read_data.data),
    .s_axis_mem_read_data_TKEEP(axis_mem_read_data.keep),
    .s_axis_mem_read_data_TLAST(axis_mem_read_data.last),
    // Memory Write Status
    //.s_axis_mem_write_status_TVALID(s_axis_rxwrite_sts_TVALID),
    //.s_axis_mem_write_status_TREADY(s_axis_rxwrite_sts_TREADY),
    //.s_axis_mem_write_status_TDATA(s_axis_rxwrite_sts_TDATA),
    
    //Pointer chaising
/*`ifdef POINTER_CHASING
    .m_axis_rx_pcmeta_V_TVALID(m_axis_rx_pcmeta.valid),
    .m_axis_rx_pcmeta_V_TREADY(m_axis_rx_pcmeta.ready),
    .m_axis_rx_pcmeta_V_TDATA(m_axis_rx_pcmeta.data),
    .s_axis_tx_pcmeta_V_TVALID(s_axis_tx_pcmeta.valid),
    .s_axis_tx_pcmeta_V_TREADY(s_axis_tx_pcmeta.ready),
    .s_axis_tx_pcmeta_V_TDATA(s_axis_tx_pcmeta.data),
`endif*/
    //CONTROL
    .s_axis_qp_interface_V_TVALID(s_axis_qp_interface.valid),
    .s_axis_qp_interface_V_TREADY(s_axis_qp_interface.ready),
    .s_axis_qp_interface_V_TDATA(s_axis_qp_interface.data),
    .s_axis_qp_conn_interface_V_TVALID(s_axis_qp_conn_interface.valid),
    .s_axis_qp_conn_interface_V_TREADY(s_axis_qp_conn_interface.ready),
    .s_axis_qp_conn_interface_V_TDATA(s_axis_qp_conn_interface.data),
    
    //.local_ip_address_V(link_local_ipv6_address), // Use IPv6 addr
    .local_ip_address_V({local_ip_address,local_ip_address,local_ip_address,local_ip_address}), //Use IPv4 addr
    .regCrcDropPkgCount_V(crc_drop_pkg_count_data),
    .regCrcDropPkgCount_V_ap_vld(crc_drop_pkg_count_valid),
    .regInvalidPsnDropCount_V(psn_drop_pkg_count_data),
    .regInvalidPsnDropCount_V_ap_vld(psn_drop_pkg_count_valid)
);

/*
 * Width alignment
 */
axi_stream #(.WIDTH(WIDTH) )   axis_mem_read_data();
axi_stream #(.WIDTH(WIDTH) )   axis_mem_write_data();
axi_stream #(.WIDTH(WIDTH) )   axis_tx_data();
//generate
if (WIDTH==64) begin
//RoCE Data Path
axis_512_to_64_converter roce_read_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_mem_read_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_mem_read_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_mem_read_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_mem_read_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_mem_read_data.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_mem_read_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_mem_read_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_mem_read_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(axis_mem_read_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(axis_mem_read_data.last)    // output wire m_axis_tlast
);

axis_512_to_64_converter roce_tx_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_tx_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_tx_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_tx_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_tx_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_tx_data.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_tx_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_tx_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_tx_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(axis_tx_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(axis_tx_data.last)    // output wire m_axis_tlast
);

axis_64_to_512_converter roce_write_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(axis_mem_write_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(axis_mem_write_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(axis_mem_write_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axis_mem_write_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axis_mem_write_data.last),    // input wire s_axis_tlast
  .s_axis_tdest(axis_mem_write_data.dest),    // input wire s_axis_tlast
  .m_axis_tvalid(m_axis_mem_write_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(m_axis_mem_write_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(m_axis_mem_write_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(m_axis_mem_write_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(m_axis_mem_write_data.last),    // output wire m_axis_tlast
  .m_axis_tdest(m_axis_mem_write_data.dest)    // output wire m_axis_tlast
);
end
if (WIDTH==512) begin
//RoCE Data Path
assign axis_mem_read_data.valid = s_axis_mem_read_data.valid;
assign s_axis_mem_read_data.ready = axis_mem_read_data.ready;
assign axis_mem_read_data.data = s_axis_mem_read_data.data;
assign axis_mem_read_data.keep = s_axis_mem_read_data.keep;
assign axis_mem_read_data.last = s_axis_mem_read_data.last;

assign axis_tx_data.valid = s_axis_tx_data.valid;
assign s_axis_tx_data.ready = axis_tx_data.ready;
assign axis_tx_data.data = s_axis_tx_data.data;
assign axis_tx_data.keep = s_axis_tx_data.keep;
assign axis_tx_data.last = s_axis_tx_data.last;

assign m_axis_mem_write_data.valid = axis_mem_write_data.valid;
assign axis_mem_write_data.ready = m_axis_mem_write_data.ready;
assign m_axis_mem_write_data.data = axis_mem_write_data.data;
assign m_axis_mem_write_data.keep = axis_mem_write_data.keep;
assign m_axis_mem_write_data.last = axis_mem_write_data.last;
end
//endgenerate

assign m_axis_rx_pcmeta.valid = 1'b0;
assign m_axis_rx_pcmeta.data = 0;
assign s_axis_tx_pcmeta.ready = 1'b1;

end
else begin
assign s_axis_rx_data.ready = 1'b1;
assign m_axis_tx_data.valid = 1'b0;

assign s_axis_tx_meta.ready = 1'b0;
assign s_axis_tx_data.ready = 1'b0;

assign m_axis_mem_write_cmd.valid = 1'b0;
assign m_axis_mem_read_cmd.valid = 1'b0;
assign m_axis_mem_write_data.valid = 1'b0;
assign s_axis_mem_read_data.ready = 1'b0;

assign s_axis_qp_interface.ready = 1'b0;
assign s_axis_qp_conn_interface.ready = 1'b0;

end
endgenerate

endmodule

`default_nettype wire
