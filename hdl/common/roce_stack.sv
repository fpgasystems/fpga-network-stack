`timescale 1ns / 1ps
`default_nettype none

`include "os_types.svh"

`define POINTER_CHASING

module roce_stack #(
    parameter ROCE_EN = 1
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


   input wire[127:0]    local_ip_address,
   output logic      crc_drop_pkg_count_valid,
   output logic[31:0]   crc_drop_pkg_count_data,
   output logic      psn_drop_pkg_count_valid,
   output logic[31:0]   psn_drop_pkg_count_data


 );

generate
if (ROCE_EN == 1) begin
rocev2_ip rocev2_inst(
    .aclk(net_clk), // input aclk
    .aresetn(net_aresetn), // input aresetn
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
    .s_axis_tx_meta_TVALID(s_axis_tx_meta.valid),
    .s_axis_tx_meta_TREADY(s_axis_tx_meta.ready),
    .s_axis_tx_meta_TDATA(s_axis_tx_meta.data),
    .s_axis_tx_data_TVALID(s_axis_tx_data.valid),
    .s_axis_tx_data_TREADY(s_axis_tx_data.ready),
    .s_axis_tx_data_TDATA(s_axis_tx_data.data),
    .s_axis_tx_data_TKEEP(s_axis_tx_data.keep),
    .s_axis_tx_data_TLAST(s_axis_tx_data.last),
    
    // IPv4
    .m_axis_tx_data_TVALID(m_axis_tx_data.valid),
    .m_axis_tx_data_TREADY(m_axis_tx_data.ready),
    .m_axis_tx_data_TDATA(m_axis_tx_data.data),
    .m_axis_tx_data_TKEEP(m_axis_tx_data.keep),
    .m_axis_tx_data_TLAST(m_axis_tx_data.last),

    //Memory
    .m_axis_mem_write_cmd_TVALID(m_axis_mem_write_cmd.valid),
    .m_axis_mem_write_cmd_TREADY(m_axis_mem_write_cmd.ready),
    .m_axis_mem_write_cmd_TDATA({m_axis_mem_write_cmd.dest, m_axis_mem_write_cmd.data}),
    
    .m_axis_mem_read_cmd_TVALID(m_axis_mem_read_cmd.valid),
    .m_axis_mem_read_cmd_TREADY(m_axis_mem_read_cmd.ready),
    .m_axis_mem_read_cmd_TDATA({m_axis_mem_read_cmd.dest, m_axis_mem_read_cmd.data}),
    // Memory Write
    .m_axis_mem_write_data_TVALID(m_axis_mem_write_data.valid),
    .m_axis_mem_write_data_TREADY(m_axis_mem_write_data.ready),
    .m_axis_mem_write_data_TDATA(m_axis_mem_write_data.data),
    .m_axis_mem_write_data_TKEEP(m_axis_mem_write_data.keep),
    .m_axis_mem_write_data_TLAST(m_axis_mem_write_data.last),
    .m_axis_mem_write_data_TDEST(m_axis_mem_write_data.dest),
    // Memory Read
    .s_axis_mem_read_data_TVALID(s_axis_mem_read_data.valid),
    .s_axis_mem_read_data_TREADY(s_axis_mem_read_data.ready),
    .s_axis_mem_read_data_TDATA(s_axis_mem_read_data.data),
    .s_axis_mem_read_data_TKEEP(s_axis_mem_read_data.keep),
    .s_axis_mem_read_data_TLAST(s_axis_mem_read_data.last),
    // Memory Write Status
    //.s_axis_mem_write_status_TVALID(s_axis_rxwrite_sts_TVALID),
    //.s_axis_mem_write_status_TREADY(s_axis_rxwrite_sts_TREADY),
    //.s_axis_mem_write_status_TDATA(s_axis_rxwrite_sts_TDATA),
    
    //Pointer chaising
`ifdef POINTER_CHASING
    .m_axis_rx_pcmeta_TVALID(m_axis_rx_pcmeta.valid),
    .m_axis_rx_pcmeta_TREADY(m_axis_rx_pcmeta.ready),
    .m_axis_rx_pcmeta_TDATA(m_axis_rx_pcmeta.data),
    .s_axis_tx_pcmeta_TVALID(s_axis_tx_pcmeta.valid),
    .s_axis_tx_pcmeta_TREADY(s_axis_tx_pcmeta.ready),
    .s_axis_tx_pcmeta_TDATA(s_axis_tx_pcmeta.data),
`endif
    //CONTROL
    .s_axis_qp_interface_TVALID(s_axis_qp_interface.valid),
    .s_axis_qp_interface_TREADY(s_axis_qp_interface.ready),
    .s_axis_qp_interface_TDATA(s_axis_qp_interface.data),
    .s_axis_qp_conn_interface_TVALID(s_axis_qp_conn_interface.valid),
    .s_axis_qp_conn_interface_TREADY(s_axis_qp_conn_interface.ready),
    .s_axis_qp_conn_interface_TDATA(s_axis_qp_conn_interface.data),
    
    //.local_ip_address_V(link_local_ipv6_address), // Use IPv6 addr
    .local_ip_address_V(local_ip_address), //Use IPv4 addr
    .regCrcDropPkgCount_V(crc_drop_pkg_count_data),
    .regCrcDropPkgCount_V_ap_vld(crc_drop_pkg_count_valid),
    .regInvalidPsnDropCount_V(psn_drop_pkg_count_data),
    .regInvalidPsnDropCount_V_ap_vld(psn_drop_pkg_count_valid)
);
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
