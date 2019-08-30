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

module udp_stack #(
    parameter UDP_EN = 1,
    parameter WIDTH = 64
)(
    input wire          net_clk,
    input wire          net_aresetn,

    // network interface streams
    axi_stream.slave        s_axis_rx_data,
    axi_stream.master       m_axis_tx_data,

    //Role
    axis_meta.master        m_axis_udp_rx_metadata,
    axi_stream.master       m_axis_udp_rx_data,
    axis_meta.slave         s_axis_udp_tx_metadata,
    axi_stream.slave        s_axis_udp_tx_data,


   //Config
    //axis_meta.slave  s_axis_qp_interface,
    //axis_meta.slave  s_axis_qp_conn_interface,


   input wire[127:0]        local_ip_address,
   input wire[15:0]         listen_port
   
 );

generate
if (UDP_EN == 1) begin

axis_meta #(.WIDTH(48))         axis_ip_to_udp_meta();
axis_meta #(.WIDTH(48))         axis_udp_to_ip_meta();

axi_stream #(.WIDTH(WIDTH))       axis_ip_to_udp_data();
axi_stream #(.WIDTH(WIDTH))       axis_udp_to_ip_data();
 
 
ipv4_ip ipv4_inst (
   .local_ipv4_address_V(local_ip_address[31:0]),    // input wire [31 : 0] local_ipv4_address_V
   .protocol_V(8'h11), //UDP_PROTOCOL
   //RX
   .s_axis_rx_data_TVALID(axis_udp_slice_to_udp.valid),  // input wire s_axis_rx_data_TVALID
   .s_axis_rx_data_TREADY(axis_udp_slice_to_udp.ready),  // output wire s_axis_rx_data_TREADY
   .s_axis_rx_data_TDATA(axis_udp_slice_to_udp.data),    // input wire [63 : 0] s_axis_rx_data_TDATA
   .s_axis_rx_data_TKEEP(axis_udp_slice_to_udp.keep),    // input wire [7 : 0] s_axis_rx_data_TKEEP
   .s_axis_rx_data_TLAST(axis_udp_slice_to_udp.last),    // input wire [0 : 0] s_axis_rx_data_TLAST
   .m_axis_rx_meta_TVALID(axis_ip_to_udp_meta.valid),  // output wire m_axis_rx_meta_TVALID
   .m_axis_rx_meta_TREADY(axis_ip_to_udp_meta.ready),  // input wire m_axis_rx_meta_TREADY
   .m_axis_rx_meta_TDATA(axis_ip_to_udp_meta.data),    // output wire [47 : 0] m_axis_rx_meta_TDATA
   .m_axis_rx_data_TVALID(axis_ip_to_udp_data.valid),  // output wire m_axis_rx_data_TVALID
   .m_axis_rx_data_TREADY(axis_ip_to_udp_data.ready),  // input wire m_axis_rx_data_TREADY
   .m_axis_rx_data_TDATA(axis_ip_to_udp_data.data),    // output wire [63 : 0] m_axis_rx_data_TDATA
   .m_axis_rx_data_TKEEP(axis_ip_to_udp_data.keep),    // output wire [7 : 0] m_axis_rx_data_TKEEP
   .m_axis_rx_data_TLAST(axis_ip_to_udp_data.last),    // output wire [0 : 0] m_axis_rx_data_TLAST
   //TX
   .s_axis_tx_meta_TVALID(axis_udp_to_ip_meta.valid),  // input wire s_axis_tx_meta_TVALID
   .s_axis_tx_meta_TREADY(axis_udp_to_ip_meta.ready),  // output wire s_axis_tx_meta_TREADY
   .s_axis_tx_meta_TDATA(axis_udp_to_ip_meta.data),    // input wire [47 : 0] s_axis_tx_meta_TDATA
   .s_axis_tx_data_TVALID(axis_udp_to_ip_data.valid),  // input wire s_axis_tx_data_TVALID
   .s_axis_tx_data_TREADY(axis_udp_to_ip_data.ready),  // output wire s_axis_tx_data_TREADY
   .s_axis_tx_data_TDATA(axis_udp_to_ip_data.data),    // input wire [63 : 0] s_axis_tx_data_TDATA
   .s_axis_tx_data_TKEEP(axis_udp_to_ip_data.keep),    // input wire [7 : 0] s_axis_tx_data_TKEEP
   .s_axis_tx_data_TLAST(axis_udp_to_ip_data.last),    // input wire [0 : 0] s_axis_tx_data_TLAST
   .m_axis_tx_data_TVALID(axis_udp_to_udp_slice.valid),  // output wire m_axis_tx_data_TVALID
   .m_axis_tx_data_TREADY(axis_udp_to_udp_slice.ready),  // input wire m_axis_tx_data_TREADY
   .m_axis_tx_data_TDATA(axis_udp_to_udp_slice.data),    // output wire [63 : 0] m_axis_tx_data_TDATA
   .m_axis_tx_data_TKEEP(axis_udp_to_udp_slice.keep),    // output wire [7 : 0] m_axis_tx_data_TKEEP
   .m_axis_tx_data_TLAST(axis_udp_to_udp_slice.last),    // output wire [0 : 0] m_axis_tx_data_TLAST
 
   .aclk(net_clk),                                    // input wire aclk
   .aresetn(net_aresetn)                              // input wire aresetn
 );
 
 udp_ip udp_inst (
   .reg_listen_port_V(listen_port),
   //RX
   .s_axis_rx_meta_TVALID(axis_ip_to_udp_meta.valid),
   .s_axis_rx_meta_TREADY(axis_ip_to_udp_meta.ready),
   .s_axis_rx_meta_TDATA(axis_ip_to_udp_meta.data),
   .s_axis_rx_data_TVALID(axis_ip_to_udp_data.valid),        // input wire s_axis_rx_data_TVALID
   .s_axis_rx_data_TREADY(axis_ip_to_udp_data.ready),        // output wire s_axis_rx_data_TREADY
   .s_axis_rx_data_TDATA(axis_ip_to_udp_data.data),          // input wire [63 : 0] s_axis_rx_data_TDATA
   .s_axis_rx_data_TKEEP(axis_ip_to_udp_data.keep),          // input wire [7 : 0] s_axis_rx_data_TKEEP
   .s_axis_rx_data_TLAST(axis_ip_to_udp_data.last),          // input wire [0 : 0] s_axis_rx_data_TLAST
   .m_axis_rx_meta_TVALID(m_axis_udp_rx_metadata.valid),        // output wire m_axis_rx_meta_TVALID
   .m_axis_rx_meta_TREADY(m_axis_udp_rx_metadata.ready),        // input wire m_axis_rx_meta_TREADY
   .m_axis_rx_meta_TDATA(m_axis_udp_rx_metadata.data),          // output wire [159 : 0] m_axis_rx_meta_TDATA
   .m_axis_rx_data_TVALID(m_axis_udp_rx_data.valid),        // output wire m_axis_rx_data_TVALID
   .m_axis_rx_data_TREADY(m_axis_udp_rx_data.ready),        // input wire m_axis_rx_data_TREADY
   .m_axis_rx_data_TDATA(m_axis_udp_rx_data.data),          // output wire [63 : 0] m_axis_rx_data_TDATA
   .m_axis_rx_data_TKEEP(m_axis_udp_rx_data.keep),          // output wire [7 : 0] m_axis_rx_data_TKEEP
   .m_axis_rx_data_TLAST(m_axis_udp_rx_data.last),          // output wire [0 : 0] m_axis_rx_data_TLAST
   //TX
   .s_axis_tx_meta_TVALID(s_axis_udp_tx_metadata.valid),
   .s_axis_tx_meta_TREADY(s_axis_udp_tx_metadata.ready),
   .s_axis_tx_meta_TDATA(s_axis_udp_tx_metadata.data),
   .s_axis_tx_data_TVALID(s_axis_udp_tx_data.valid),        // input wire s_axis_tx_data_TVALID
   .s_axis_tx_data_TREADY(s_axis_udp_tx_data.ready),        // output wire s_axis_tx_data_TREADY
   .s_axis_tx_data_TDATA(s_axis_udp_tx_data.data),          // input wire [63 : 0] s_axis_tx_data_TDATA
   .s_axis_tx_data_TKEEP(s_axis_udp_tx_data.keep),          // input wire [7 : 0] s_axis_tx_data_TKEEP
   .s_axis_tx_data_TLAST(s_axis_udp_tx_data.last),          // input wire [0 : 0] s_axis_tx_data_TLAST
   .m_axis_tx_meta_TVALID(axis_udp_to_ip_meta.valid),        // input wire m_axis_tx_meta_TVALID
   .m_axis_tx_meta_TREADY(axis_udp_to_ip_meta.ready),        // output wire m_axis_tx_meta_TREADY
   .m_axis_tx_meta_TDATA(axis_udp_to_ip_meta.data),          // input wire [159 : 0] m_axis_tx_meta_TDATA
   .m_axis_tx_data_TVALID(axis_udp_to_ip_data.valid),        // output wire m_axis_tx_data_TVALID
   .m_axis_tx_data_TREADY(axis_udp_to_ip_data.ready),        // input wire m_axis_tx_data_TREADY
   .m_axis_tx_data_TDATA(axis_udp_to_ip_data.data),          // output wire [63 : 0] m_axis_tx_data_TDATA
   .m_axis_tx_data_TKEEP(axis_udp_to_ip_data.keep),          // output wire [7 : 0] m_axis_tx_data_TKEEP
   .m_axis_tx_data_TLAST(axis_udp_to_ip_data.last),          // output wire [0 : 0] m_axis_tx_data_TLAST
 
   .aclk(net_clk),                                          // input wire aclk
   .aresetn(net_aresetn)                                    // input wire aresetn
 );
end
else begin

assign s_axis_rx_data.ready = 1'b1;
assign m_axis_tx_data.valid = 1'b0;


assign m_axis_udp_rx_metadata.valid = 1'b0;
assign m_axis_udp_rx_data.valid = 1'b0;
assign s_axis_udp_tx_metadata.ready = 1'b0;
assign s_axis_udp_tx_data.ready = 1'b0;

end
endgenerate

endmodule

`default_nettype wire
