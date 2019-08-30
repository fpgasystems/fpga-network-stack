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

   input wire[31:0]        local_ip_address,
   input wire[15:0]         listen_port
   
 );

generate
if (UDP_EN == 1) begin

axis_meta #(.WIDTH(48))         axis_ip_to_udp_meta();
axis_meta #(.WIDTH(48))         axis_udp_to_ip_meta();

axi_stream #(.WIDTH(WIDTH))       axis_ip_to_udp_data();
axi_stream #(.WIDTH(WIDTH))       axis_udp_to_ip_data();
 
 
ipv4_ip ipv4_inst (
   .local_ipv4_address_V(local_ip_address),
   .protocol_V(8'h11), //UDP_PROTOCOL
   //RX
   .s_axis_rx_data_TVALID(s_axis_rx_data.valid),
   .s_axis_rx_data_TREADY(s_axis_rx_data.ready),
   .s_axis_rx_data_TDATA(s_axis_rx_data.data),
   .s_axis_rx_data_TKEEP(s_axis_rx_data.keep),
   .s_axis_rx_data_TLAST(s_axis_rx_data.last),
   .m_axis_rx_meta_V_TVALID(axis_ip_to_udp_meta.valid),
   .m_axis_rx_meta_V_TREADY(axis_ip_to_udp_meta.ready),
   .m_axis_rx_meta_V_TDATA(axis_ip_to_udp_meta.data),
   .m_axis_rx_data_TVALID(axis_ip_to_udp_data.valid),
   .m_axis_rx_data_TREADY(axis_ip_to_udp_data.ready),
   .m_axis_rx_data_TDATA(axis_ip_to_udp_data.data),
   .m_axis_rx_data_TKEEP(axis_ip_to_udp_data.keep),
   .m_axis_rx_data_TLAST(axis_ip_to_udp_data.last),
   //TX
   .s_axis_tx_meta_V_TVALID(axis_udp_to_ip_meta.valid),
   .s_axis_tx_meta_V_TREADY(axis_udp_to_ip_meta.ready),
   .s_axis_tx_meta_V_TDATA(axis_udp_to_ip_meta.data),
   .s_axis_tx_data_TVALID(axis_udp_to_ip_data.valid),
   .s_axis_tx_data_TREADY(axis_udp_to_ip_data.ready),
   .s_axis_tx_data_TDATA(axis_udp_to_ip_data.data),
   .s_axis_tx_data_TKEEP(axis_udp_to_ip_data.keep),
   .s_axis_tx_data_TLAST(axis_udp_to_ip_data.last),
   .m_axis_tx_data_TVALID(m_axis_tx_data.valid),
   .m_axis_tx_data_TREADY(m_axis_tx_data.ready),
   .m_axis_tx_data_TDATA(m_axis_tx_data.data),
   .m_axis_tx_data_TKEEP(m_axis_tx_data.keep),
   .m_axis_tx_data_TLAST(m_axis_tx_data.last),
 
   .ap_clk(net_clk),
   .ap_rst_n(net_aresetn)
 );
 
 udp_ip udp_inst (
   .reg_listen_port_V(listen_port),
   .reg_ip_address_V({local_ip_address,local_ip_address,local_ip_address,local_ip_address}),
   //RX
   .s_axis_rx_meta_V_TVALID(axis_ip_to_udp_meta.valid),
   .s_axis_rx_meta_V_TREADY(axis_ip_to_udp_meta.ready),
   .s_axis_rx_meta_V_TDATA(axis_ip_to_udp_meta.data),
   .s_axis_rx_data_TVALID(axis_ip_to_udp_data.valid),
   .s_axis_rx_data_TREADY(axis_ip_to_udp_data.ready),
   .s_axis_rx_data_TDATA(axis_ip_to_udp_data.data),
   .s_axis_rx_data_TKEEP(axis_ip_to_udp_data.keep),
   .s_axis_rx_data_TLAST(axis_ip_to_udp_data.last),
   .m_axis_rx_meta_V_TVALID(m_axis_udp_rx_metadata.valid),
   .m_axis_rx_meta_V_TREADY(m_axis_udp_rx_metadata.ready),
   .m_axis_rx_meta_V_TDATA(m_axis_udp_rx_metadata.data),
   .m_axis_rx_data_TVALID(m_axis_udp_rx_data.valid),
   .m_axis_rx_data_TREADY(m_axis_udp_rx_data.ready),
   .m_axis_rx_data_TDATA(m_axis_udp_rx_data.data),
   .m_axis_rx_data_TKEEP(m_axis_udp_rx_data.keep),
   .m_axis_rx_data_TLAST(m_axis_udp_rx_data.last),
   //TX
   .s_axis_tx_meta_V_TVALID(s_axis_udp_tx_metadata.valid),
   .s_axis_tx_meta_V_TREADY(s_axis_udp_tx_metadata.ready),
   .s_axis_tx_meta_V_TDATA(s_axis_udp_tx_metadata.data),
   .s_axis_tx_data_TVALID(s_axis_udp_tx_data.valid),
   .s_axis_tx_data_TREADY(s_axis_udp_tx_data.ready),
   .s_axis_tx_data_TDATA(s_axis_udp_tx_data.data),
   .s_axis_tx_data_TKEEP(s_axis_udp_tx_data.keep),
   .s_axis_tx_data_TLAST(s_axis_udp_tx_data.last),
   .m_axis_tx_meta_V_TVALID(axis_udp_to_ip_meta.valid),
   .m_axis_tx_meta_V_TREADY(axis_udp_to_ip_meta.ready),
   .m_axis_tx_meta_V_TDATA(axis_udp_to_ip_meta.data),
   .m_axis_tx_data_TVALID(axis_udp_to_ip_data.valid),
   .m_axis_tx_data_TREADY(axis_udp_to_ip_data.ready),
   .m_axis_tx_data_TDATA(axis_udp_to_ip_data.data),
   .m_axis_tx_data_TKEEP(axis_udp_to_ip_data.keep),
   .m_axis_tx_data_TLAST(axis_udp_to_ip_data.last),
 
   .ap_clk(net_clk),
   .ap_rst_n(net_aresetn)
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
