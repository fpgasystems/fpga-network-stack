`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 21.11.2013 10:48:44
// Design Name: 
// Module Name: tcp_ip_wrapper
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


module tcp_ip_wrapper (
    input           aclk,
    input           aresetn,
    input[47:0]	    myMacAddress,
    input[31:0]     inputIpAddress,
    input           dhcpEnable,
    output[31:0]    ipAddressout,
    output[15:0]    regSessionCount,
    output[15:0]    relSessionCount,
    // Debug signals for the session lookup //
    output          upd_req_TVALID_out,
    output          upd_req_TREADY_out,
    output          upd_req_TDATA_out,
    output          upd_rsp_TVALID_out,
    output          upd_rsp_TREADY_out,
    // network interface streams
    output          AXI_M_Stream_TVALID,
    input           AXI_M_Stream_TREADY,
    output[63:0]    AXI_M_Stream_TDATA,
    output[7:0]     AXI_M_Stream_TKEEP,
    output          AXI_M_Stream_TLAST,

    input           AXI_S_Stream_TVALID,
    output          AXI_S_Stream_TREADY,
    input[63:0]     AXI_S_Stream_TDATA,
    input[7:0]      AXI_S_Stream_TKEEP,
    input           AXI_S_Stream_TLAST,
    // Debug streams
    output[7:0]     axi_debug1_tkeep,
    output[63:0]    axi_debug1_tdata,
    output          axi_debug1_tvalid,
    output          axi_debug1_tready,
    output          axi_debug1_tlast,
    
    output[7:0]     axi_debug2_tkeep,
    output[63:0]    axi_debug2_tdata,
    output          axi_debug2_tvalid,
    output          axi_debug2_tready,
    output          axi_debug2_tlast,
        // memory rx cmd streams
    output          m_axis_rxread_cmd_TVALID,
    input           m_axis_rxread_cmd_TREADY,
    output[71:0]    m_axis_rxread_cmd_TDATA,
    output          m_axis_rxwrite_cmd_TVALID,
    input           m_axis_rxwrite_cmd_TREADY,
    output[71:0]    m_axis_rxwrite_cmd_TDATA,
    // memory rx sts streams
    //input           s_axis_rxread_sts_TVALID,
    //output          s_axis_rxread_sts_TREADY,
    //input[7:0]      s_axis_rxread_sts_TDATA,
    input           s_axis_rxwrite_sts_TVALID,
    output          s_axis_rxwrite_sts_TREADY,
    input[7:0]      s_axis_rxwrite_sts_TDATA,
    // memory rx data streams  - Read status signals are not used
    input           s_axis_rxread_data_TVALID,
    output          s_axis_rxread_data_TREADY,
    input[63:0]     s_axis_rxread_data_TDATA,
    input[7:0]      s_axis_rxread_data_TKEEP,
    input           s_axis_rxread_data_TLAST,
    
    output          m_axis_rxwrite_data_TVALID,
    input           m_axis_rxwrite_data_TREADY,
    output[63:0]    m_axis_rxwrite_data_TDATA,
    output[7:0]     m_axis_rxwrite_data_TKEEP,
    output          m_axis_rxwrite_data_TLAST,
    
    // memory tx cmd streams
    output          m_axis_txread_cmd_TVALID,
    input           m_axis_txread_cmd_TREADY,
    output[71:0]    m_axis_txread_cmd_TDATA,
    output          m_axis_txwrite_cmd_TVALID,
    input           m_axis_txwrite_cmd_TREADY,
    output[71:0]    m_axis_txwrite_cmd_TDATA,
    // memory tx sts streams - Read status signals are not used
    //input           s_axis_txread_sts_TVALID,
    //output          s_axis_txread_sts_TREADY,
    //input[7:0]      s_axis_txread_sts_TDATA,
    input           s_axis_txwrite_sts_TVALID,
    output          s_axis_txwrite_sts_TREADY,
    input[7:0]      s_axis_txwrite_sts_TDATA,
    // memory tx data streams
    input           s_axis_txread_data_TVALID,
    output          s_axis_txread_data_TREADY,
    input[63:0]     s_axis_txread_data_TDATA,
    input[7:0]      s_axis_txread_data_TKEEP,
    input           s_axis_txread_data_TLAST,
    
    output          m_axis_txwrite_data_TVALID,
    input           m_axis_txwrite_data_TREADY,
    output[63:0]    m_axis_txwrite_data_TDATA,
    output[7:0]     m_axis_txwrite_data_TKEEP,
    output          m_axis_txwrite_data_TLAST,
    
    //application interface streams
    output          m_axis_listen_port_status_TVALID,
    input           m_axis_listen_port_status_TREADY,
    output[7:0]     m_axis_listen_port_status_TDATA,
    output          m_axis_notifications_TVALID,
    input           m_axis_notifications_TREADY,
    output[87:0]    m_axis_notifications_TDATA,
    output          m_axis_open_status_TVALID,
    input           m_axis_open_status_TREADY,
    output[23:0]    m_axis_open_status_TDATA,
    output          m_axis_rx_data_TVALID,
    input           m_axis_rx_data_TREADY,
    output[63:0]    m_axis_rx_data_TDATA,
    output[7:0]     m_axis_rx_data_TKEEP,
    output          m_axis_rx_data_TLAST,
    output          m_axis_rx_metadata_TVALID,
    input           m_axis_rx_metadata_TREADY,
    output[15:0]    m_axis_rx_metadata_TDATA,
    output          m_axis_tx_status_TVALID,
    input           m_axis_tx_status_TREADY,
    output[23:0]    m_axis_tx_status_TDATA,
    input           s_axis_listen_port_TVALID,
    output          s_axis_listen_port_TREADY,
    input[15:0]     s_axis_listen_port_TDATA,
    //input           s_axis_close_port_TVALID,
    //output          s_axis_close_port_TREADY,
    //input[15:0]     s_axis_close_port_TDATA,
    input           s_axis_close_connection_TVALID,
    output          s_axis_close_connection_TREADY,
    input[15:0]     s_axis_close_connection_TDATA,
    input           s_axis_open_connection_TVALID,
    output          s_axis_open_connection_TREADY,
    input[47:0]     s_axis_open_connection_TDATA,
    input           s_axis_read_package_TVALID,
    output          s_axis_read_package_TREADY,
    input[31:0]     s_axis_read_package_TDATA,
    input           s_axis_tx_data_TVALID,
    output          s_axis_tx_data_TREADY,
    input[63:0]     s_axis_tx_data_TDATA,
    input[7:0]      s_axis_tx_data_TKEEP,
    input           s_axis_tx_data_TLAST,
    input           s_axis_tx_metadata_TVALID,
    output          s_axis_tx_metadata_TREADY,
    input[15:0]     s_axis_tx_metadata_TDATA, //change to 15?
    // Session Counter Output // 
    output[15:0]    regSessionCount_V,
    output          regSessionCount_V_ap_vld,
    // UDP App Mux to UDP Loopback signals //
    output          lbPortOpenReplyIn_TVALID,           // output wire portOpenReplyIn_TVALID
    input           lbPortOpenReplyIn_TREADY,           // input wire portOpenReplyIn_TREADY
    output[7:0]     lbPortOpenReplyIn_TDATA,            // output wire [7 : 0] portOpenReplyIn_TDATA
    input           lbRequestPortOpenOut_TVALID,        // input wire requestPortOpenOut_TVALID
    output          lbRequestPortOpenOut_TREADY,        // output wire requestPortOpenOut_TREADY
    input[15:0]     lbRequestPortOpenOut_TDATA,         // input wire [15 : 0] requestPortOpenOut_TDATA
    output          lbRxDataIn_TVALID,                  // output wire rxDataIn_TVALID
    input           lbRxDataIn_TREADY,                  // input wire rxDataIn_TREADY
    output[63:0]    lbRxDataIn_TDATA,                   // output wire [63 : 0] rxDataIn_TDATA
    output[7:0]     lbRxDataIn_TKEEP,                   // output wire [7 : 0] rxDataIn_TKEEP
    output          lbRxDataIn_TLAST,                   // output wire [0 : 0] rxDataIn_TLAST
    output          lbRxMetadataIn_TVALID,              // output wire rxMetadataIn_TVALID
    input           lbRxMetadataIn_TREADY,              // input wire rxMetadataIn_TREADY
    output[95:0]    lbRxMetadataIn_TDATA,               // output wire [95 : 0] rxMetadataIn_TDATA
    input           lbTxDataOut_TVALID,                 // input wire txDataOut_TVALID
    output          lbTxDataOut_TREADY,                 // output wire txDataOut_TREADY
    input[63:0]     lbTxDataOut_TDATA,                  // input wire [63 : 0] txDataOut_TDATA
    input[7:0]      lbTxDataOut_TKEEP,                  // input wire [7 : 0] txDataOut_TKEEP
    input           lbTxDataOut_TLAST,                  // input wire [0 : 0] txDataOut_TLAST
    input           lbTxLengthOut_TVALID,               // input wire txLengthOut_TVALID
    output          lbTxLengthOut_TREADY,               // output wire txLengthOut_TREADY
    input[15:0]     lbTxLengthOut_TDATA,                // input wire [15 : 0] txLengthOut_TDATA
    input           lbTxMetadataOut_TVALID,             // input wire txMetadataOut_TVALID
    output          lbTxMetadataOut_TREADY,             // output wire txMetadataOut_TREADY
    input[95:0]     lbTxMetadataOut_TDATA,               // input wire [95 : 0] txMetadataOut_TDATA
    // Debug signals //
    output[99:0]  metadataFifo_din,    // [99:0] 
    output        metadataFifo_full,
    output        metadataFifo_write,
    
    output[15:0]  metadataHandlerFifo_din, // [15:0]
    output        metadataHandlerFifo_full,
    output        metadataHandlerFifo_write
    ////////////////////
    );

// IP Handler Outputs
wire            axi_iph_to_arp_slice_tvalid;
wire            axi_iph_to_arp_slice_tready;
wire[63:0]      axi_iph_to_arp_slice_tdata;
wire[7:0]       axi_iph_to_arp_slice_tkeep;
wire            axi_iph_to_arp_slice_tlast;
wire            axi_iph_to_icmp_slice_tvalid;
wire            axi_iph_to_icmp_slice_tready;
wire[63:0]      axi_iph_to_icmp_slice_tdata;
wire[7:0]       axi_iph_to_icmp_slice_tkeep;
wire            axi_iph_to_icmp_slice_tlast;

//Slice connections on RX path
wire            axi_arp_slice_to_arp_tvalid;
wire            axi_arp_slice_to_arp_tready;
wire[63:0]      axi_arp_slice_to_arp_tdata;
wire[7:0]       axi_arp_slice_to_arp_tkeep;
wire            axi_arp_slice_to_arp_tlast;
wire            axi_icmp_slice_to_icmp_tvalid;
wire            axi_icmp_slice_to_icmp_tready;
wire[63:0]      axi_icmp_slice_to_icmp_tdata;
wire[7:0]       axi_icmp_slice_to_icmp_tkeep;
wire            axi_icmp_slice_to_icmp_tlast;
wire            axi_iph_to_toe_tvalid;
wire            axi_iph_to_toe_tready;
wire[63:0]      axi_iph_to_toe_tdata;
wire[7:0]       axi_iph_to_toe_tkeep;
wire            axi_iph_to_toe_tlast;

// MAC-IP Encode Inputs
wire            axi_intercon_to_mie_tvalid;
wire            axi_intercon_to_mie_tready;
wire[63:0]      axi_intercon_to_mie_tdata;
wire[7:0]       axi_intercon_to_mie_tkeep;
wire            axi_intercon_to_mie_tlast;
wire            axi_mie_to_intercon_tvalid;
wire            axi_mie_to_intercon_tready;
wire[63:0]      axi_mie_to_intercon_tdata;
wire[7:0]       axi_mie_to_intercon_tkeep;
wire            axi_mie_to_intercon_tlast;
//Slice connections on RX path
wire            axi_arp_to_arp_slice_tvalid;
wire            axi_arp_to_arp_slice_tready;
wire[63:0]      axi_arp_to_arp_slice_tdata;
wire[7:0]       axi_arp_to_arp_slice_tkeep;
wire            axi_arp_to_arp_slice_tlast;
wire            axi_icmp_to_icmp_slice_tvalid;
wire            axi_icmp_to_icmp_slice_tready;
wire[63:0]      axi_icmp_to_icmp_slice_tdata;
wire[7:0]       axi_icmp_to_icmp_slice_tkeep;
wire            axi_icmp_to_icmp_slice_tlast;

wire        axi_udp_to_merge_tvalid;
wire        axi_udp_to_merge_tready;
wire[63:0]  axi_udp_to_merge_tdata;
wire[7:0]   axi_udp_to_merge_tkeep;
wire        axi_udp_to_merge_tlast;

wire        axi_iph_to_udp_tvalid;
wire        axi_iph_to_udp_tready;
wire[63:0]  axi_iph_to_udp_tdata;
wire[7:0]   axi_iph_to_udp_tkeep;
wire        axi_iph_to_udp_tlast;

wire        axis_udp_to_icmp_tready;
wire        axis_udp_to_icmp_tvalid;
wire[63:0]  axis_udp_to_icmp_tdata;
wire[7:0]   axis_udp_to_icmp_tkeep;
wire        axis_udp_to_icmp_tlast;

wire        axis_ttl_to_icmp_tready;
wire        axis_ttl_to_icmp_tvalid;
wire[63:0]  axis_ttl_to_icmp_tdata;
wire[7:0]   axis_ttl_to_icmp_tkeep;
wire        axis_ttl_to_icmp_tlast;
/// TOE Outputs ///
wire        axi_toe_to_toe_slice_tvalid; // output AXI_M_Stream_TVALID
wire        axi_toe_to_toe_slice_tready; // input AXI_M_Stream_TREADY
wire[63:0]  axi_toe_to_toe_slice_tdata; // output [63 : 0] AXI_M_Stream_TDATA
wire[7:0]   axi_toe_to_toe_slice_tkeep; // output [7 : 0] AXI_M_Stream_TSTRB
wire        axi_toe_to_toe_slice_tlast; // output [0 : 0] AXI_M_Stream_TLAST

/// UDP-to-App Mux Signals ///
wire        udp2muxRxDataIn_TVALID;
wire        udp2muxRxDataIn_TREADY;
wire[63:0]  udp2muxRxDataIn_TDATA;
wire        udp2muxRxDataIn_TLAST;
wire[7:0]   udp2muxRxDataIn_TKEEP;

wire        mux2dhcpRxDataIn_TVALID;
wire        mux2dhcpRxDataIn_TREADY;
wire[63:0]  mux2dhcpRxDataIn_TDATA;
wire        mux2dhcpRxDataIn_TLAST;
wire[7:0]   mux2dhcpRxDataIn_TKEEP;

/// Rx Side Metadata
wire        udp2muxRxMetadataIn_V_TVALID;
wire        udp2muxRxMetadataIn_V_TREADY;
wire[95:0]  udp2muxRxMetadataIn_V_TDATA;

wire        mux2dhcpRxMetadataIn_V_TVALID;
wire        mux2dhcpRxMetadataIn_V_TREADY;
wire[95:0]  mux2dhcpRxMetadataIn_V_TDATA;

/// Signals for opening ports ///
wire        mux2udp_requestPortOpenOut_V_TVALID;
wire        mux2udp_requestPortOpenOut_V_TREADY;
wire[15:0]  mux2udp_requestPortOpenOut_V_TDATA; // OK
wire        udp2mux_portOpenReplyIn_V_V_TVALID;
wire        udp2mux_portOpenReplyIn_V_V_TREADY;
wire[7:0]   udp2mux_portOpenReplyIn_V_V_TDATA; // OK

wire        dhcp2mux_requestPortOpenOut_V_TVALID;
wire        dhcp2mux_requestPortOpenOut_V_TREADY;
wire[15:0]  dhcp2mux_requestPortOpenOut_V_TDATA;
wire        mux2dhcp_portOpenReplyIn_V_V_TVALID;
wire        mux2dhcp_portOpenReplyIn_V_V_TREADY;
wire[7:0]   mux2dhcp_portOpenReplyIn_V_V_TDATA;

wire        mcd_to_shim_TVALID;
wire        mcd_to_shim_TREADY;
wire[63:0]  mcd_to_shim_TDATA;
wire[7:0]   mcd_to_shim_TKEEP;
wire        mcd_to_shim_TLAST;
wire[111:0] mcd_to_shim_TUSER;
/// Signals for connecting the data i/o's to the mux


wire        dhcp2mux_TVALID;
wire        dhcp2mux_TREADY;
wire[63:0]  dhcp2mux_TDATA;
wire[7:0]   dhcp2mux_TKEEP;
wire        dhcp2mux_TLAST;

wire        mux2udp_TVALID;
wire        mux2udp_TREADY;
wire[63:0]  mux2udp_TDATA;
wire[7:0]   mux2udp_TKEEP;
wire        mux2udp_TLAST;
//// Tx Side Metadata
wire        mux2udpTxMetadataOut_V_TVALID;
wire        mux2udpTxMetadataOut_V_TREADY;
wire[95:0]  mux2udpTxMetadataOut_V_TDATA;

wire        dhcp2muxTxMetadataOut_V_TVALID;
wire        dhcp2muxTxMetadataOut_V_TREADY;
wire[95:0]  dhcp2muxTxMetadataOut_V_TDATA;

/// Tx Side Packet Length
wire        mux2udpTxLengthOut_V_V_TVALID;
wire        mux2udpTxLengthOut_V_V_TREADY;
wire[15:0]  mux2udpTxLengthOut_V_V_TDATA;

wire        dhcp2muxTxLengthOut_V_V_TVALID;
wire        dhcp2muxTxLengthOut_V_V_TREADY;
wire[15:0]  dhcp2muxTxLengthOut_V_V_TDATA;

// TCP Offload SmartCAM signals //
wire        upd_req_TVALID;
wire        upd_req_TREADY;
wire[111:0] upd_req_TDATA; //(1 + 1 + 14 + 96) - 1 = 111
wire        upd_rsp_TVALID;
wire        upd_rsp_TREADY;
wire[15:0]  upd_rsp_TDATA;

wire        ins_req_TVALID;
wire        ins_req_TREADY;
wire[111:0] ins_req_TDATA;
wire        del_req_TVALID;
wire        del_req_TREADY;
wire[111:0] del_req_TDATA;

wire        lup_req_TVALID;
wire        lup_req_TREADY;
wire[97:0]  lup_req_TDATA; //should be 96, also wrong in SmartCam
wire        lup_rsp_TVALID;
wire        lup_rsp_TREADY;
wire[15:0]  lup_rsp_TDATA;
// DHCP Client IP address output //
wire[31:0]  dhcpAddressOut;

// because read status is not used
assign axis_rxread_sts_TREADY = 1'b1;
assign axis_txread_sts_TREADY = 1'b1;

toe_ip toe_inst (
// Data output
.m_axis_tcp_data_TVALID(axi_toe_to_toe_slice_tvalid), // output AXI_M_Stream_TVALID
.m_axis_tcp_data_TREADY(axi_toe_to_toe_slice_tready), // input AXI_M_Stream_TREADY
.m_axis_tcp_data_TDATA(axi_toe_to_toe_slice_tdata), // output [63 : 0] AXI_M_Stream_TDATA
.m_axis_tcp_data_TKEEP(axi_toe_to_toe_slice_tkeep), // output [7 : 0] AXI_M_Stream_TSTRB
.m_axis_tcp_data_TLAST(axi_toe_to_toe_slice_tlast), // output [0 : 0] AXI_M_Stream_TLAST
// Data input
.s_axis_tcp_data_TVALID(axi_iph_to_toe_tvalid), // input AXI_S_Stream_TVALID
.s_axis_tcp_data_TREADY(axi_iph_to_toe_tready), // output AXI_S_Stream_TREADY
.s_axis_tcp_data_TDATA(axi_iph_to_toe_tdata), // input [63 : 0] AXI_S_Stream_TDATA
.s_axis_tcp_data_TKEEP(axi_iph_to_toe_tkeep), // input [7 : 0] AXI_S_Stream_TKEEP
.s_axis_tcp_data_TLAST(axi_iph_to_toe_tlast), // input [0 : 0] AXI_S_Stream_TLAST
// rx read commands
.m_axis_rxread_cmd_TVALID(m_axis_rxread_cmd_TVALID),
.m_axis_rxread_cmd_TREADY(m_axis_rxread_cmd_TREADY),
.m_axis_rxread_cmd_TDATA(m_axis_rxread_cmd_TDATA),
// rx write commands
.m_axis_rxwrite_cmd_TVALID(m_axis_rxwrite_cmd_TVALID),
.m_axis_rxwrite_cmd_TREADY(m_axis_rxwrite_cmd_TREADY),
.m_axis_rxwrite_cmd_TDATA(m_axis_rxwrite_cmd_TDATA),
// rx write status
.s_axis_rxwrite_sts_TVALID(s_axis_rxwrite_sts_TVALID),
.s_axis_rxwrite_sts_TREADY(s_axis_rxwrite_sts_TREADY),
.s_axis_rxwrite_sts_TDATA(s_axis_rxwrite_sts_TDATA),
// rx buffer read path
.s_axis_rxread_data_TVALID(s_axis_rxread_data_TVALID),
.s_axis_rxread_data_TREADY(s_axis_rxread_data_TREADY),
.s_axis_rxread_data_TDATA(s_axis_rxread_data_TDATA),
.s_axis_rxread_data_TKEEP(s_axis_rxread_data_TKEEP),
.s_axis_rxread_data_TLAST(s_axis_rxread_data_TLAST),
// rx buffer write path
.m_axis_rxwrite_data_TVALID(m_axis_rxwrite_data_TVALID),
.m_axis_rxwrite_data_TREADY(m_axis_rxwrite_data_TREADY),
.m_axis_rxwrite_data_TDATA(m_axis_rxwrite_data_TDATA),
.m_axis_rxwrite_data_TKEEP(m_axis_rxwrite_data_TKEEP),
.m_axis_rxwrite_data_TLAST(m_axis_rxwrite_data_TLAST),
// tx read commands
.m_axis_txread_cmd_TVALID(m_axis_txread_cmd_TVALID),
.m_axis_txread_cmd_TREADY(m_axis_txread_cmd_TREADY),
.m_axis_txread_cmd_TDATA(m_axis_txread_cmd_TDATA),
//tx write commands
.m_axis_txwrite_cmd_TVALID(m_axis_txwrite_cmd_TVALID),
.m_axis_txwrite_cmd_TREADY(m_axis_txwrite_cmd_TREADY),
.m_axis_txwrite_cmd_TDATA(m_axis_txwrite_cmd_TDATA),
// tx write status
.s_axis_txwrite_sts_TVALID(s_axis_txwrite_sts_TVALID),
.s_axis_txwrite_sts_TREADY(s_axis_txwrite_sts_TREADY),
.s_axis_txwrite_sts_TDATA(s_axis_txwrite_sts_TDATA),
// tx read path
.s_axis_txread_data_TVALID(s_axis_txread_data_TVALID),
.s_axis_txread_data_TREADY(s_axis_txread_data_TREADY),
.s_axis_txread_data_TDATA(s_axis_txread_data_TDATA),
.s_axis_txread_data_TKEEP(s_axis_txread_data_TKEEP),
.s_axis_txread_data_TLAST(s_axis_txread_data_TLAST),
// tx write path
.m_axis_txwrite_data_TVALID(m_axis_txwrite_data_TVALID),
.m_axis_txwrite_data_TREADY(m_axis_txwrite_data_TREADY),
.m_axis_txwrite_data_TDATA(m_axis_txwrite_data_TDATA),
.m_axis_txwrite_data_TKEEP(m_axis_txwrite_data_TKEEP),
.m_axis_txwrite_data_TLAST(m_axis_txwrite_data_TLAST),
/// SmartCAM I/F ///
.m_axis_session_upd_req_TVALID(upd_req_TVALID),
.m_axis_session_upd_req_TREADY(upd_req_TREADY),
.m_axis_session_upd_req_TDATA(upd_req_TDATA),
//.m_axis_session_del_req_TVALID(del_req_TVALID), // output m_axis_session_del_req_TVALID
//.m_axis_session_del_req_TREADY(del_req_TREADY), // input m_axis_session_del_req_TREADY
//.m_axis_session_del_req_TDATA(del_req_TDATA), // output [111 : 0] m_axis_session_del_req_TDATA
//.m_axis_session_ins_req_TVALID(ins_req_TVALID), // output m_axis_session_ins_req_TVALID
//.m_axis_session_ins_req_TREADY(ins_req_TREADY), // input m_axis_session_ins_req_TREADY
//.m_axis_session_ins_req_TDATA(ins_req_TDATA), // output [111 : 0] m_axis_session_ins_req_TDATA

.s_axis_session_upd_rsp_TVALID(upd_rsp_TVALID),
.s_axis_session_upd_rsp_TREADY(upd_rsp_TREADY),
.s_axis_session_upd_rsp_TDATA(upd_rsp_TDATA),

.m_axis_session_lup_req_TVALID(lup_req_TVALID),
.m_axis_session_lup_req_TREADY(lup_req_TREADY),
.m_axis_session_lup_req_TDATA(lup_req_TDATA),
.s_axis_session_lup_rsp_TVALID(lup_rsp_TVALID),
.s_axis_session_lup_rsp_TREADY(lup_rsp_TREADY),
.s_axis_session_lup_rsp_TDATA(lup_rsp_TDATA),

/* Application Interface */
// listen&close port
.s_axis_listen_port_req_TVALID(s_axis_listen_port_TVALID),
.s_axis_listen_port_req_TREADY(s_axis_listen_port_TREADY),
.s_axis_listen_port_req_TDATA(s_axis_listen_port_TDATA),
.m_axis_listen_port_rsp_TVALID(m_axis_listen_port_status_TVALID),
.m_axis_listen_port_rsp_TREADY(m_axis_listen_port_status_TREADY),
.m_axis_listen_port_rsp_TDATA(m_axis_listen_port_status_TDATA),
//.s_axis_close_port_TVALID(s_axis_close_port_TVALID),
//.s_axis_close_port_TREADY(s_axis_close_port_TREADY),
//.s_axis_close_port_TDATA(s_axis_close_port_TDATA),
// notification & read request
.m_axis_notification_TVALID(m_axis_notifications_TVALID),
.m_axis_notification_TREADY(m_axis_notifications_TREADY),
.m_axis_notification_TDATA(m_axis_notifications_TDATA),
.s_axis_rx_data_req_TVALID(s_axis_read_package_TVALID),
.s_axis_rx_data_req_TREADY(s_axis_read_package_TREADY),
.s_axis_rx_data_req_TDATA(s_axis_read_package_TDATA),

// open&close connection
.s_axis_open_conn_req_TVALID(s_axis_open_connection_TVALID),
.s_axis_open_conn_req_TREADY(s_axis_open_connection_TREADY),
.s_axis_open_conn_req_TDATA(s_axis_open_connection_TDATA),
.m_axis_open_conn_rsp_TVALID(m_axis_open_status_TVALID),
.m_axis_open_conn_rsp_TREADY(m_axis_open_status_TREADY),
.m_axis_open_conn_rsp_TDATA(m_axis_open_status_TDATA),
.s_axis_close_conn_req_TVALID(s_axis_close_connection_TVALID),//axis_close_connection_TVALID
.s_axis_close_conn_req_TREADY(s_axis_close_connection_TREADY),
.s_axis_close_conn_req_TDATA(s_axis_close_connection_TDATA),

// rx data
.m_axis_rx_data_rsp_metadata_TVALID(m_axis_rx_metadata_TVALID),
.m_axis_rx_data_rsp_metadata_TREADY(m_axis_rx_metadata_TREADY),
.m_axis_rx_data_rsp_metadata_TDATA(m_axis_rx_metadata_TDATA),
.m_axis_rx_data_rsp_TVALID(m_axis_rx_data_TVALID),
.m_axis_rx_data_rsp_TREADY(m_axis_rx_data_TREADY),
.m_axis_rx_data_rsp_TDATA(m_axis_rx_data_TDATA),
.m_axis_rx_data_rsp_TKEEP(m_axis_rx_data_TKEEP),
.m_axis_rx_data_rsp_TLAST(m_axis_rx_data_TLAST),

// tx data
.s_axis_tx_data_req_metadata_TVALID(s_axis_tx_metadata_TVALID),
.s_axis_tx_data_req_metadata_TREADY(s_axis_tx_metadata_TREADY),
.s_axis_tx_data_req_metadata_TDATA(s_axis_tx_metadata_TDATA),
.s_axis_tx_data_req_TVALID(s_axis_tx_data_TVALID),
.s_axis_tx_data_req_TREADY(s_axis_tx_data_TREADY),
.s_axis_tx_data_req_TDATA(s_axis_tx_data_TDATA),
.s_axis_tx_data_req_TKEEP(s_axis_tx_data_TKEEP),
.s_axis_tx_data_req_TLAST(s_axis_tx_data_TLAST),
.m_axis_tx_data_rsp_TVALID(m_axis_tx_status_TVALID),
.m_axis_tx_data_rsp_TREADY(m_axis_tx_status_TREADY),
.m_axis_tx_data_rsp_TDATA(m_axis_tx_status_TDATA),
// Debug signals //
//.rxEng2eventEng_data(rxEng2eventEng_data),
//.rxEng2eventEng_write(rxEng2eventEng_write),
//.rxEng2eventEng_full(rxEng2eventEng_full),
//.eventEng2txEng_data(eventEng2txEng_data),
//.eventEng2txEng_write(eventEng2txEng_write),
//.eventEng2txEng_full(eventEng2txEng_full),
//.metadataFifo_din(metadataFifo_din),
//.metadataFifo_full(metadataFifo_full),
//.metadataFifo_write(metadataFifo_write),
//.metadataHandlerFifo_din(metadataHandlerFifo_din),
//.metadataHandlerFifo_full(metadataHandlerFifo_full),
//.metadataHandlerFifo_write(metadataHandlerFifo_write),
////////////////////
.regIpAddress_V(32'h01010101),                                      // input wire [31 : 0] regIpAddress_V
.relSessionCount_V(relSessionCount),                              // output wire [15 : 0] relSessionCount_V
.regSessionCount_V(regSessionCount),                              // output wire [15 : 0] regSessionCount_V
.aclk(aclk),                                                        // input aclk
.aresetn(aresetn)                                                   // input aresetn
);

SmartCamCtl SmartCamCtl_inst
(
.clk(aclk),
.rst(~aresetn),
.led0(sc_led0),
.led1(sc_led1),
.cam_ready(cam_ready),

.lup_req_valid(lup_req_TVALID),
.lup_req_ready(lup_req_TREADY),
.lup_req_din(lup_req_TDATA),

.lup_rsp_valid(lup_rsp_TVALID),
.lup_rsp_ready(lup_rsp_TREADY),
.lup_rsp_dout(lup_rsp_TDATA),

.upd_req_valid(upd_req_TVALID),
.upd_req_ready(upd_req_TREADY),
.upd_req_din(upd_req_TDATA),

.upd_rsp_valid(upd_rsp_TVALID),
.upd_rsp_ready(upd_rsp_TREADY),
.upd_rsp_dout(upd_rsp_TDATA),

.debug()
);
 
// UDP Offload Engine
udp_0 myUDP (
  .inputPathInData_TVALID(axi_iph_to_udp_tvalid),                   // input wire inputPathInData_TVALID
  .inputPathInData_TREADY(axi_iph_to_udp_tready),                   // output wire inputPathInData_TREADY
  .inputPathInData_TDATA(axi_iph_to_udp_tdata),                     // input wire [63 : 0] inputPathInData_TDATA
  .inputPathInData_TKEEP(axi_iph_to_udp_tkeep),                     // input wire [7 : 0] inputPathInData_TKEEP
  .inputPathInData_TLAST(axi_iph_to_udp_tlast),                     // input wire [0 : 0] inputPathInData_TLAST
  .inputpathOutData_TVALID(udp2muxRxDataIn_TVALID),                 // output wire inputpathOutData_V_TVALID
  .inputpathOutData_TREADY(udp2muxRxDataIn_TREADY),                 // input wire inputpathOutData_V_TREADY
  .inputpathOutData_TDATA(udp2muxRxDataIn_TDATA),                   // output wire [63 : 0] inputpathOutData_V_TDATA
  .inputpathOutData_TKEEP(udp2muxRxDataIn_TKEEP),                   // output wire [7:0]  
  .inputpathOutData_TLAST(udp2muxRxDataIn_TLAST),                   // output wire
  .openPort_TVALID(mux2udp_requestPortOpenOut_V_TVALID),            // input wire openPort_V_TVALID
  .openPort_TREADY(mux2udp_requestPortOpenOut_V_TREADY),            // output wire openPort_V_TREADY
  .openPort_TDATA(mux2udp_requestPortOpenOut_V_TDATA),              // input wire [7 : 0] openPort_V_TDATA
  .confirmPortStatus_TVALID(udp2mux_portOpenReplyIn_V_V_TVALID),    // output wire confirmPortStatus_V_V_TVALID
  .confirmPortStatus_TREADY(udp2mux_portOpenReplyIn_V_V_TREADY),    // input wire confirmPortStatus_V_V_TREADY
  .confirmPortStatus_TDATA(udp2mux_portOpenReplyIn_V_V_TDATA),      // output wire [15 : 0] confirmPortStatus_V_V_TDATA
  .inputPathOutputMetadata_TVALID(udp2muxRxMetadataIn_V_TVALID),    // output wire inputPathOutputMetadata_V_TVALID
  .inputPathOutputMetadata_TREADY(udp2muxRxMetadataIn_V_TREADY),    // input wire inputPathOutputMetadata_V_TREADY
  .inputPathOutputMetadata_TDATA(udp2muxRxMetadataIn_V_TDATA),      // output wire [95 : 0] inputPathOutputMetadata_V_TDATA
  .portRelease_TVALID(1'b0),                                        // input wire portRelease_V_V_TVALID
  .portRelease_TREADY(),                                            // output wire portRelease_V_V_TREADY
  .portRelease_TDATA(15'b0),                                        // input wire [15 : 0] portRelease_V_V_TDATA
  .outputPathInData_TVALID(mux2udp_TVALID),                         // input wire outputPathInData_V_TVALID
  .outputPathInData_TREADY(mux2udp_TREADY),                         // output wire outputPathInData_V_TREADY
  .outputPathInData_TDATA(mux2udp_TDATA),                           // input wire [71 : 0] outputPathInData_V_TDATA
  .outputPathInData_TKEEP(mux2udp_TKEEP),                           // input wire [7 : 0] outputPathInData_TKEEP
  .outputPathInData_TLAST(mux2udp_TLAST),                           // input wire [0 : 0] outputPathInData_TLAST
  .outputPathOutData_TVALID(axi_udp_to_merge_tvalid),               // output wire outputPathOutData_TVALID
  .outputPathOutData_TREADY(axi_udp_to_merge_tready),               // input wire outputPathOutData_TREADY
  .outputPathOutData_TDATA(axi_udp_to_merge_tdata),                 // output wire [63 : 0] outputPathOutData_TDATA
  .outputPathOutData_TKEEP(axi_udp_to_merge_tkeep),                 // output wire [7 : 0] outputPathOutData_TKEEP
  .outputPathOutData_TLAST(axi_udp_to_merge_tlast),                 // output wire [0 : 0] outputPathOutData_TLAST  
  .outputPathInMetadata_TVALID(mux2udpTxMetadataOut_V_TVALID),      // input wire outputPathInMetadata_V_TVALID
  .outputPathInMetadata_TREADY(mux2udpTxMetadataOut_V_TREADY),      // output wire outputPathInMetadata_V_TREADY
  .outputPathInMetadata_TDATA(mux2udpTxMetadataOut_V_TDATA),        // input wire [95 : 0] outputPathInMetadata_V_TDATA
  .outputpathInLength_TVALID(mux2udpTxLengthOut_V_V_TVALID),        // input wire outputpathInLength_V_V_TVALID
  .outputpathInLength_TREADY(mux2udpTxLengthOut_V_V_TREADY),        // output wire outputpathInLength_V_V_TREADY
  .outputpathInLength_TDATA(mux2udpTxLengthOut_V_V_TDATA),          // input wire [15 : 0] outputpathInLength_V_V_TDATA
  .inputPathPortUnreachable_TVALID(axis_udp_to_icmp_tvalid),        // output wire inputPathPortUnreachable_TVALID
  .inputPathPortUnreachable_TREADY(axis_udp_to_icmp_tready),        // input wire inputPathPortUnreachable_TREADY
  .inputPathPortUnreachable_TDATA(axis_udp_to_icmp_tdata),          // output wire [63 : 0] inputPathPortUnreachable_TDATA
  .inputPathPortUnreachable_TKEEP(axis_udp_to_icmp_tkeep),          // output wire [7 : 0] inputPathPortUnreachable_TKEEP
  .inputPathPortUnreachable_TLAST(axis_udp_to_icmp_tlast),          // output wire [0 : 0] inputPathPortUnreachable_TLAST
  //.ap_start(1'b1),                                                // input wire ap_start
  //.ap_ready(),                                                    // output wire ap_ready
  //.ap_done(),                                                     // output wire ap_done
  //.ap_idle(),                                                     // output wire ap_idle
  .aclk(aclk),                                                      // input wire ap_clk
  .aresetn(aresetn)                                                 // input wire ap_rst_n
);
  
ip_handler_ip ip_handler_inst (
  .dataIn_TVALID(AXI_S_Stream_TVALID),                  // input wire dataIn_TVALID
  .dataIn_TREADY(AXI_S_Stream_TREADY),                  // output wire dataIn_TREADY
  .dataIn_TDATA(AXI_S_Stream_TDATA),                    // input wire [63 : 0] dataIn_TDATA
  .dataIn_TKEEP(AXI_S_Stream_TKEEP),                    // input wire [7 : 0] dataIn_TKEEP
  .dataIn_TLAST(AXI_S_Stream_TLAST),                    // input wire [0 : 0] dataIn_TLAST
  .ARPdataOut_TVALID(axi_iph_to_arp_slice_tvalid),      // output wire ARPdataOut_TVALID
  .ARPdataOut_TREADY(axi_iph_to_arp_slice_tready),      // input wire ARPdataOut_TREADY
  .ARPdataOut_TDATA(axi_iph_to_arp_slice_tdata),        // output wire [63 : 0] ARPdataOut_TDATA
  .ARPdataOut_TKEEP(axi_iph_to_arp_slice_tkeep),        // output wire [7 : 0] ARPdataOut_TKEEP
  .ARPdataOut_TLAST(axi_iph_to_arp_slice_tlast),        // output wire [0 : 0] ARPdataOut_TLAST
  .ICMPdataOut_TVALID(axi_iph_to_icmp_slice_tvalid),    // output wire ICMPdataOut_TVALID
  .ICMPdataOut_TREADY(axi_iph_to_icmp_slice_tready),    // input wire ICMPdataOut_TREADY
  .ICMPdataOut_TDATA(axi_iph_to_icmp_slice_tdata),      // output wire [63 : 0] ICMPdataOut_TDATA
  .ICMPdataOut_TKEEP(axi_iph_to_icmp_slice_tkeep),      // output wire [7 : 0] ICMPdataOut_TKEEP
  .ICMPdataOut_TLAST(axi_iph_to_icmp_slice_tlast),      // output wire [0 : 0] ICMPdataOut_TLAST
  .ICMPexpDataOut_TVALID(axis_ttl_to_icmp_tvalid),      // output wire ICMPexpDataOut_TVALID
  .ICMPexpDataOut_TREADY(axis_ttl_to_icmp_tready),      // input wire ICMPexpDataOut_TREADY
  .ICMPexpDataOut_TDATA(axis_ttl_to_icmp_tdata),        // output wire [63 : 0] ICMPexpDataOut_TDATA
  .ICMPexpDataOut_TKEEP(axis_ttl_to_icmp_tkeep),        // output wire [7 : 0] ICMPexpDataOut_TKEEP
  .ICMPexpDataOut_TLAST(axis_ttl_to_icmp_tlast),        // output wire [0 : 0] ICMPexpDataOut_TLAST
  .UDPdataOut_TVALID(axi_iph_to_udp_tvalid),            // output wire UDPdataOut_TVALID
  .UDPdataOut_TREADY(axi_iph_to_udp_tready),            // input wire UDPdataOut_TREADY
  .UDPdataOut_TDATA(axi_iph_to_udp_tdata),              // output wire [63 : 0] UDPdataOut_TDATA
  .UDPdataOut_TKEEP(axi_iph_to_udp_tkeep),              // output wire [7 : 0] UDPdataOut_TKEEP
  .UDPdataOut_TLAST(axi_iph_to_udp_tlast),              // output wire [0 : 0] UDPdataOut_TLAST
  .TCPdataOut_TVALID(axi_iph_to_toe_tvalid),            // output wire TCPdataOut_TVALID
  .TCPdataOut_TREADY(axi_iph_to_toe_tready),            // input wire TCPdataOut_TREADY
  .TCPdataOut_TDATA(axi_iph_to_toe_tdata),              // output wire [63 : 0] TCPdataOut_TDATA
  .TCPdataOut_TKEEP(axi_iph_to_toe_tkeep),              // output wire [7 : 0] TCPdataOut_TKEEP
  .TCPdataOut_TLAST(axi_iph_to_toe_tlast),              // output wire [0 : 0] TCPdataOut_TLAST
  .regIpAddress_V(32'h01010101),                        // input wire [31 : 0] regIpAddress_V
  .myMacAddress_V(myMacAddress),                        // input wire [47 : 0] myMacAddress_V
  .ap_clk(aclk),                                        // input wire ap_clk
  .ap_rst_n(aresetn)                                    // input wire ap_rst_n
);

// ARP lookup
wire        axis_arp_lookup_request_TVALID;
wire        axis_arp_lookup_request_TREADY;
wire[31:0]  axis_arp_lookup_request_TDATA;
wire        axis_arp_lookup_reply_TVALID;
wire        axis_arp_lookup_reply_TREADY;
wire[55:0]  axis_arp_lookup_reply_TDATA;

mac_ip_encode_ip mac_ip_encode_inst (
  .dataIn_TVALID(axi_intercon_to_mie_tvalid),                    // input wire dataIn_TVALID
  .dataIn_TREADY(axi_intercon_to_mie_tready),                    // output wire dataIn_TREADY
  .dataIn_TDATA(axi_intercon_to_mie_tdata),                      // input wire [63 : 0] dataIn_TDATA
  .dataIn_TKEEP(axi_intercon_to_mie_tkeep),                      // input wire [7 : 0] dataIn_TKEEP
  .dataIn_TLAST(axi_intercon_to_mie_tlast),                      // input wire [0 : 0] dataIn_TLAST
  .arpTableIn_V_TVALID(axis_arp_lookup_reply_TVALID),        // input wire arpTableIn_V_TVALID
  .arpTableIn_V_TREADY(axis_arp_lookup_reply_TREADY),        // output wire arpTableIn_V_TREADY
  .arpTableIn_V_TDATA(axis_arp_lookup_reply_TDATA),          // input wire [55 : 0] arpTableIn_V_TDATA
  .dataOut_TVALID(axi_mie_to_intercon_tvalid),                  // output wire dataOut_TVALID
  .dataOut_TREADY(axi_mie_to_intercon_tready),                  // input wire dataOut_TREADY
  .dataOut_TDATA(axi_mie_to_intercon_tdata),                    // output wire [63 : 0] dataOut_TDATA
  .dataOut_TKEEP(axi_mie_to_intercon_tkeep),                    // output wire [7 : 0] dataOut_TKEEP
  .dataOut_TLAST(axi_mie_to_intercon_tlast),                    // output wire [0 : 0] dataOut_TLAST
  .arpTableOut_V_V_TVALID(axis_arp_lookup_request_TVALID),  // output wire arpTableOut_V_V_TVALID
  .arpTableOut_V_V_TREADY(axis_arp_lookup_request_TREADY),  // input wire arpTableOut_V_V_TREADY
  .arpTableOut_V_V_TDATA(axis_arp_lookup_request_TDATA),    // output wire [31 : 0] arpTableOut_V_V_TDATA
  .regSubNetMask_V(32'h00FFFFFF),                // input wire [31 : 0] regSubNetMask_V
  .regDefaultGateway_V(32'h01010101),        // input wire [31 : 0] regDefaultGateway_V
  .myMacAddress_V(myMacAddress),                  // input wire [47 : 0] myMacAddress_V
  .ap_clk(aclk),                                  // input wire ap_clk
  .ap_rst_n(aresetn)                              // input wire ap_rst_n
);
// merges icmp and tcp
axis_interconnect_3to1 ip_merger (
  .ACLK(aclk),                                  // input wire ACLK
  .ARESETN(aresetn),                            // input wire ARESETN
  .S00_AXIS_ACLK(aclk),                // input wire S00_AXIS_ACLK
  .S01_AXIS_ACLK(aclk),                // input wire S01_AXIS_ACLK
  .S02_AXIS_ACLK(aclk),                // input wire S02_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn),          // input wire S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn),          // input wire S01_AXIS_ARESETN
  .S02_AXIS_ARESETN(aresetn),          // input wire S02_AXIS_ARESETN
  .S00_AXIS_TVALID(axi_icmp_to_icmp_slice_tvalid),            // input wire S00_AXIS_TVALID
  .S01_AXIS_TVALID(axi_udp_to_merge_tvalid),            // input wire S01_AXIS_TVALID
  .S02_AXIS_TVALID(axi_toe_to_toe_slice_tvalid),            // input wire S02_AXIS_TVALID
  .S00_AXIS_TREADY(axi_icmp_to_icmp_slice_tready),            // output wire S00_AXIS_TREADY
  .S01_AXIS_TREADY(axi_udp_to_merge_tready),            // output wire S01_AXIS_TREADY
  .S02_AXIS_TREADY(axi_toe_to_toe_slice_tready),            // output wire S02_AXIS_TREADY
  .S00_AXIS_TDATA(axi_icmp_to_icmp_slice_tdata),              // input wire [63 : 0] S00_AXIS_TDATA
  .S01_AXIS_TDATA(axi_udp_to_merge_tdata),              // input wire [63 : 0] S01_AXIS_TDATA
  .S02_AXIS_TDATA(axi_toe_to_toe_slice_tdata),              // input wire [63 : 0] S02_AXIS_TDATA
  .S00_AXIS_TKEEP(axi_icmp_to_icmp_slice_tkeep),              // input wire [7 : 0] S00_AXIS_TKEEP
  .S01_AXIS_TKEEP(axi_udp_to_merge_tkeep),              // input wire [7 : 0] S01_AXIS_TKEEP
  .S02_AXIS_TKEEP(axi_toe_to_toe_slice_tkeep),              // input wire [7 : 0] S02_AXIS_TKEEP
  .S00_AXIS_TLAST(axi_icmp_to_icmp_slice_tlast),              // input wire S00_AXIS_TLAST
  .S01_AXIS_TLAST(axi_udp_to_merge_tlast),              // input wire S01_AXIS_TLAST
  .S02_AXIS_TLAST(axi_toe_to_toe_slice_tlast),              // input wire S02_AXIS_TLAST
  .M00_AXIS_ACLK(aclk),                // input wire M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn),          // input wire M00_AXIS_ARESETN
  .M00_AXIS_TVALID(axi_intercon_to_mie_tvalid),            // output wire M00_AXIS_TVALID
  .M00_AXIS_TREADY(axi_intercon_to_mie_tready),            // input wire M00_AXIS_TREADY
  .M00_AXIS_TDATA(axi_intercon_to_mie_tdata),              // output wire [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(axi_intercon_to_mie_tkeep),              // output wire [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(axi_intercon_to_mie_tlast),              // output wire M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0),  // input wire S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0),  // input wire S01_ARB_REQ_SUPPRESS
  .S02_ARB_REQ_SUPPRESS(1'b0)  // input wire S02_ARB_REQ_SUPPRESS
);

// merges ip and arp
axis_interconnect_2to1 mac_merger (
  .ACLK(aclk), // input ACLK
  .ARESETN(aresetn), // input ARESETN
  .S00_AXIS_ACLK(aclk), // input S00_AXIS_ACLK
  .S01_AXIS_ACLK(aclk), // input S01_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn), // input S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn), // input S01_AXIS_ARESETN
  .S00_AXIS_TVALID(axi_arp_to_arp_slice_tvalid), // input S00_AXIS_TVALID
  .S01_AXIS_TVALID(axi_mie_to_intercon_tvalid), // input S01_AXIS_TVALID
  .S00_AXIS_TREADY(axi_arp_to_arp_slice_tready), // output S00_AXIS_TREADY
  .S01_AXIS_TREADY(axi_mie_to_intercon_tready), // output S01_AXIS_TREADY
  .S00_AXIS_TDATA(axi_arp_to_arp_slice_tdata), // input [63 : 0] S00_AXIS_TDATA
  .S01_AXIS_TDATA(axi_mie_to_intercon_tdata), // input [63 : 0] S01_AXIS_TDATA
  .S00_AXIS_TKEEP(axi_arp_to_arp_slice_tkeep), // input [7 : 0] S00_AXIS_TKEEP
  .S01_AXIS_TKEEP(axi_mie_to_intercon_tkeep), // input [7 : 0] S01_AXIS_TKEEP
  .S00_AXIS_TLAST(axi_arp_to_arp_slice_tlast), // input S00_AXIS_TLAST
  .S01_AXIS_TLAST(axi_mie_to_intercon_tlast), // input S01_AXIS_TLAST
  .M00_AXIS_ACLK(aclk), // input M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn), // input M00_AXIS_ARESETN
  .M00_AXIS_TVALID(AXI_M_Stream_TVALID), // output M00_AXIS_TVALID
  .M00_AXIS_TREADY(AXI_M_Stream_TREADY), // input M00_AXIS_TREADY
  .M00_AXIS_TDATA(AXI_M_Stream_TDATA), // output [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(AXI_M_Stream_TKEEP), // output [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(AXI_M_Stream_TLAST), // output M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0), // input S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
);
arpServerWrapper arpServerInst (
.axi_arp_to_arp_slice_tvalid(axi_arp_to_arp_slice_tvalid),
.axi_arp_to_arp_slice_tready(axi_arp_to_arp_slice_tready),
.axi_arp_to_arp_slice_tdata(axi_arp_to_arp_slice_tdata),
.axi_arp_to_arp_slice_tkeep(axi_arp_to_arp_slice_tkeep),
.axi_arp_to_arp_slice_tlast(axi_arp_to_arp_slice_tlast),
.axis_arp_lookup_reply_TVALID(axis_arp_lookup_reply_TVALID),
.axis_arp_lookup_reply_TREADY(axis_arp_lookup_reply_TREADY),
.axis_arp_lookup_reply_TDATA(axis_arp_lookup_reply_TDATA),
.axi_arp_slice_to_arp_tvalid(axi_arp_slice_to_arp_tvalid),
.axi_arp_slice_to_arp_tready(axi_arp_slice_to_arp_tready),
.axi_arp_slice_to_arp_tdata(axi_arp_slice_to_arp_tdata),
.axi_arp_slice_to_arp_tkeep(axi_arp_slice_to_arp_tkeep),
.axi_arp_slice_to_arp_tlast(axi_arp_slice_to_arp_tlast),
.axis_arp_lookup_request_TVALID(axis_arp_lookup_request_TVALID),
.axis_arp_lookup_request_TREADY(axis_arp_lookup_request_TREADY),
.axis_arp_lookup_request_TDATA(axis_arp_lookup_request_TDATA),
.myMacAddress(myMacAddress),
.myIpAddress(32'h01010101),
.aclk(aclk), // input aclk
.aresetn(aresetn)); // input aresetn

assign  axi_debug1_tkeep    = axi_toe_to_toe_slice_tkeep;
assign  axi_debug1_tdata    = axi_toe_to_toe_slice_tdata;
assign  axi_debug1_tvalid   = axi_toe_to_toe_slice_tvalid;
assign  axi_debug1_tready   = axi_toe_to_toe_slice_tready;
assign  axi_debug1_tlast    = axi_toe_to_toe_slice_tlast;

assign  axi_debug2_tkeep    = axi_iph_to_toe_tkeep;
assign  axi_debug2_tdata    = axi_iph_to_toe_tdata;
assign  axi_debug2_tvalid   = axi_iph_to_toe_tvalid;
assign  axi_debug2_tready   = axi_iph_to_toe_tready;
assign  axi_debug2_tlast    = axi_iph_to_toe_tlast;

icmp_server_ip icmp_server_inst (
  .dataIn_TVALID(axi_icmp_slice_to_icmp_tvalid),    // input wire dataIn_TVALID
  .dataIn_TREADY(axi_icmp_slice_to_icmp_tready),    // output wire dataIn_TREADY
  .dataIn_TDATA(axi_icmp_slice_to_icmp_tdata),      // input wire [63 : 0] dataIn_TDATA
  .dataIn_TKEEP(axi_icmp_slice_to_icmp_tkeep),      // input wire [7 : 0] dataIn_TKEEP
  .dataIn_TLAST(axi_icmp_slice_to_icmp_tlast),      // input wire [0 : 0] dataIn_TLAST
  .udpIn_TVALID(axis_udp_to_icmp_tvalid),           // input wire udpIn_TVALID
  .udpIn_TREADY(axis_udp_to_icmp_tready),           // output wire udpIn_TREADY
  .udpIn_TDATA(axis_udp_to_icmp_tdata),             // input wire [63 : 0] udpIn_TDATA
  .udpIn_TKEEP(axis_udp_to_icmp_tkeep),             // input wire [7 : 0] udpIn_TKEEP
  .udpIn_TLAST(axis_udp_to_icmp_tlast),             // input wire [0 : 0] udpIn_TLAST
  .ttlIn_TVALID(axis_ttl_to_icmp_tvalid),           // input wire ttlIn_TVALID
  .ttlIn_TREADY(axis_ttl_to_icmp_tready),           // output wire ttlIn_TREADY
  .ttlIn_TDATA(axis_ttl_to_icmp_tdata),             // input wire [63 : 0] ttlIn_TDATA
  .ttlIn_TKEEP(axis_ttl_to_icmp_tkeep),             // input wire [7 : 0] ttlIn_TKEEP
  .ttlIn_TLAST(axis_ttl_to_icmp_tlast),             // input wire [0 : 0] ttlIn_TLAST
  .dataOut_TVALID(axi_icmp_to_icmp_slice_tvalid),   // output wire dataOut_TVALID
  .dataOut_TREADY(axi_icmp_to_icmp_slice_tready),   // input wire dataOut_TREADY
  .dataOut_TDATA(axi_icmp_to_icmp_slice_tdata),     // output wire [63 : 0] dataOut_TDATA
  .dataOut_TKEEP(axi_icmp_to_icmp_slice_tkeep),     // output wire [7 : 0] dataOut_TKEEP
  .dataOut_TLAST(axi_icmp_to_icmp_slice_tlast),     // output wire [0 : 0] dataOut_TLAST
  .ap_clk(aclk),                                    // input wire ap_clk
  .ap_rst_n(aresetn)                                // input wire ap_rst_n
);
   
/*
 * Slices
 */
 // ARP Input Slice
axis_register_slice_64 axis_register_arp_in_slice(
 .aclk(aclk),
 .aresetn(aresetn),
 .s_axis_tvalid(axi_iph_to_arp_slice_tvalid),
 .s_axis_tready(axi_iph_to_arp_slice_tready),
 .s_axis_tdata(axi_iph_to_arp_slice_tdata),
 .s_axis_tkeep(axi_iph_to_arp_slice_tkeep),
 .s_axis_tlast(axi_iph_to_arp_slice_tlast),
 .m_axis_tvalid(axi_arp_slice_to_arp_tvalid),
 .m_axis_tready(axi_arp_slice_to_arp_tready),
 .m_axis_tdata(axi_arp_slice_to_arp_tdata),
 .m_axis_tkeep(axi_arp_slice_to_arp_tkeep),
 .m_axis_tlast(axi_arp_slice_to_arp_tlast)
);
 // ICMP Input Slice
axis_register_slice_64 axis_register_icmp_in_slice(
  .aclk(aclk),
  .aresetn(aresetn),
  .s_axis_tvalid(axi_iph_to_icmp_slice_tvalid),
  .s_axis_tready(axi_iph_to_icmp_slice_tready),
  .s_axis_tdata(axi_iph_to_icmp_slice_tdata),
  .s_axis_tkeep(axi_iph_to_icmp_slice_tkeep),
  .s_axis_tlast(axi_iph_to_icmp_slice_tlast),
  .m_axis_tvalid(axi_icmp_slice_to_icmp_tvalid),
  .m_axis_tready(axi_icmp_slice_to_icmp_tready),
  .m_axis_tdata(axi_icmp_slice_to_icmp_tdata),
  .m_axis_tkeep(axi_icmp_slice_to_icmp_tkeep),
  .m_axis_tlast(axi_icmp_slice_to_icmp_tlast)
);
    
udpAppMux_0 myAppMux (
  .portOpenReplyIn_TVALID(udp2mux_portOpenReplyIn_V_V_TVALID),          // input wire portOpenReplyIn_TVALID
  .portOpenReplyIn_TREADY(udp2mux_portOpenReplyIn_V_V_TREADY),          // output wire portOpenReplyIn_TREADY
  .portOpenReplyIn_TDATA(udp2mux_portOpenReplyIn_V_V_TDATA),            // input wire [7 : 0] portOpenReplyIn_TDATA         
  .requestPortOpenOut_TVALID(mux2udp_requestPortOpenOut_V_TVALID),      // output wire requestPortOpenOut_TVALID
  .requestPortOpenOut_TREADY(mux2udp_requestPortOpenOut_V_TREADY),      // input wire requestPortOpenOut_TREADY
  .requestPortOpenOut_TDATA(mux2udp_requestPortOpenOut_V_TDATA),        // output wire [15 : 0] requestPortOpenOut_TDATA 
  
  .portOpenReplyOutApp_TVALID(lbPortOpenReplyIn_TVALID),                // output wire portOpenReplyOutApp_TVALID
  .portOpenReplyOutApp_TREADY(lbPortOpenReplyIn_TREADY),                // input wire portOpenReplyOutApp_TREADY
  .portOpenReplyOutApp_TDATA(lbPortOpenReplyIn_TDATA),                  // output wire [7 : 0] portOpenReplyOutApp_TDATA     
  .requestPortOpenInApp_TVALID(lbRequestPortOpenOut_TVALID),            // input wire requestPortOpenInApp_TVALID
  .requestPortOpenInApp_TREADY(lbRequestPortOpenOut_TREADY),            // output wire requestPortOpenInApp_TREADY
  .requestPortOpenInApp_TDATA(lbRequestPortOpenOut_TDATA),              // input wire [15 : 0] requestPortOpenInApp_TDATA       
    
  .portOpenReplyOutDhcp_TVALID(mux2dhcp_portOpenReplyIn_V_V_TVALID),    // output wire portOpenReplyOutDhcp_TVALID
  .portOpenReplyOutDhcp_TREADY(mux2dhcp_portOpenReplyIn_V_V_TREADY),    // input wire portOpenReplyOutDhcp_TREADY
  .portOpenReplyOutDhcp_TDATA(mux2dhcp_portOpenReplyIn_V_V_TDATA),      // output wire [7 : 0] portOpenReplyOutDhcp_TDATA
  .requestPortOpenInDhcp_TVALID(dhcp2mux_requestPortOpenOut_V_TVALID),  // input wire requestPortOpenInDhcp_TVALID
  .requestPortOpenInDhcp_TREADY(dhcp2mux_requestPortOpenOut_V_TREADY),  // output wire requestPortOpenInDhcp_TREADY
  .requestPortOpenInDhcp_TDATA(dhcp2mux_requestPortOpenOut_V_TDATA),    // input wire [15 : 0] requestPortOpenInDhcp_TDATA
  
  .rxDataIn_TVALID(udp2muxRxDataIn_TVALID),                             // input wire rxDataIn_TVALID
  .rxDataIn_TREADY(udp2muxRxDataIn_TREADY),                             // output wire rxDataIn_TREADY
  .rxDataIn_TDATA(udp2muxRxDataIn_TDATA),                               // input wire [63 : 0] rxDataIn_TDATA
  .rxDataIn_TKEEP(udp2muxRxDataIn_TKEEP),                               // input wire [7 : 0] rxDataIn_TKEEP
  .rxDataIn_TLAST(udp2muxRxDataIn_TLAST),                               // input wire [0 : 0] rxDataIn_TLAST
  .rxDataOutApp_TVALID(lbRxDataIn_TVALID),                              // output wire rxDataOutApp_TVALID
  .rxDataOutApp_TREADY(lbRxDataIn_TREADY),                              // input wire rxDataOutApp_TREADY
  .rxDataOutApp_TDATA(lbRxDataIn_TDATA),                                // output wire [63 : 0] rxDataOutApp_TDATA
  .rxDataOutApp_TKEEP(lbRxDataIn_TKEEP),                                // output wire [7 : 0] rxDataOutApp_TKEEP
  .rxDataOutApp_TLAST(lbRxDataIn_TLAST),                                // output wire [0 : 0] rxDataOutApp_TLAST
  .rxDataOutDhcp_TVALID(mux2dhcpRxDataIn_TVALID),                       // output wire rxDataOutDhcp_TVALID
  .rxDataOutDhcp_TREADY(mux2dhcpRxDataIn_TREADY),                       // input wire rxDataOutDhcp_TREADY
  .rxDataOutDhcp_TDATA(mux2dhcpRxDataIn_TDATA),                         // output wire [63 : 0] rxDataOutDhcp_TDATA
  .rxDataOutDhcp_TKEEP(mux2dhcpRxDataIn_TKEEP),                         // output wire [7 : 0] rxDataOutDhcp_TKEEP
  .rxDataOutDhcp_TLAST(mux2dhcpRxDataIn_TLAST),                         // output wire [0 : 0] rxDataOutDhcp_TLAST
  .rxMetadataIn_TVALID(udp2muxRxMetadataIn_V_TVALID),                   // input wire rxMetadataIn_TVALID
  .rxMetadataIn_TREADY(udp2muxRxMetadataIn_V_TREADY),                   // output wire rxMetadataIn_TREADY
  .rxMetadataIn_TDATA(udp2muxRxMetadataIn_V_TDATA),                     // input wire [95 : 0] rxMetadataIn_TDATA
  .rxMetadataOutApp_TVALID(lbRxMetadataIn_TVALID),                      // output wire rxMetadataOutApp_TVALID
  .rxMetadataOutApp_TREADY(lbRxMetadataIn_TREADY),                      // input wire rxMetadataOutApp_TREADY
  .rxMetadataOutApp_TDATA(lbRxMetadataIn_TDATA),                        // output wire [95 : 0] rxMetadataOutApp_TDATA
  .rxMetadataOutDhcp_TVALID(mux2dhcpRxMetadataIn_V_TVALID),             // output wire rxMetadataOutDhcp_TVALID
  .rxMetadataOutDhcp_TREADY(mux2dhcpRxMetadataIn_V_TREADY),             // input wire rxMetadataOutDhcp_TREADY
  .rxMetadataOutDhcp_TDATA(mux2dhcpRxMetadataIn_V_TDATA),               // output wire [95 : 0] rxMetadataOutDhcp_TDATA
  .txDataInApp_TVALID(lbTxDataOut_TVALID),                              // input wire txDataInApp_TVALID
  .txDataInApp_TREADY(lbTxDataOut_TREADY),                              // output wire txDataInApp_TREADY
  .txDataInApp_TDATA(lbTxDataOut_TDATA),                                // input wire [63 : 0] txDataInApp_TDATA
  .txDataInApp_TKEEP(lbTxDataOut_TKEEP),                                // input wire [7 : 0] txDataInApp_TKEEP
  .txDataInApp_TLAST(lbTxDataOut_TLAST),                                // input wire [0 : 0] txDataInApp_TLAST
  .txDataInDhcp_TVALID(dhcp2mux_TVALID),                                // input wire txDataInDhcp_TVALID
  .txDataInDhcp_TREADY(dhcp2mux_TREADY),                                // output wire txDataInDhcp_TREADY
  .txDataInDhcp_TDATA(dhcp2mux_TDATA),                                  // input wire [63 : 0] txDataInDhcp_TDATA
  .txDataInDhcp_TKEEP(dhcp2mux_TKEEP),                                  // input wire [7 : 0] txDataInDhcp_TKEEP
  .txDataInDhcp_TLAST(dhcp2mux_TLAST),                                  // input wire [0 : 0] txDataInDhcp_TLAST
  .txDataOut_TVALID(mux2udp_TVALID),                                    // output wire txDataOut_TVALID
  .txDataOut_TREADY(mux2udp_TREADY),                                    // input wire txDataOut_TREADY
  .txDataOut_TDATA(mux2udp_TDATA),                                      // output wire [63 : 0] txDataOut_TDATA
  .txDataOut_TKEEP(mux2udp_TKEEP),                                      // output wire [7 : 0] txDataOut_TKEEP
  .txDataOut_TLAST(mux2udp_TLAST),                                      // output wire [0 : 0] txDataOut_TLAST
  .txLengthInApp_TVALID(lbTxLengthOut_TVALID),                          // input wire txLengthInApp_TVALID
  .txLengthInApp_TREADY(lbTxLengthOut_TREADY),                          // output wire txLengthInApp_TREADY
  .txLengthInApp_TDATA(lbTxLengthOut_TDATA),                            // input wire [15 : 0] txLengthInApp_TDATA
  .txLengthInDhcp_TVALID(dhcp2muxTxLengthOut_V_V_TVALID),               // input wire txLengthInDhcp_TVALID
  .txLengthInDhcp_TREADY(dhcp2muxTxLengthOut_V_V_TREADY),               // output wire txLengthInDhcp_TREADY
  .txLengthInDhcp_TDATA(dhcp2muxTxLengthOut_V_V_TDATA),                 // input wire [15 : 0] txLengthInDhcp_TDATA
  .txLengthOut_TVALID(mux2udpTxLengthOut_V_V_TVALID),                   // output wire txLengthOut_TVALID
  .txLengthOut_TREADY(mux2udpTxLengthOut_V_V_TREADY),                   // input wire txLengthOut_TREADY
  .txLengthOut_TDATA(mux2udpTxLengthOut_V_V_TDATA),                     // output wire [15 : 0] txLengthOut_TDATA
  .txMetadataInApp_TVALID(lbTxMetadataOut_TVALID),                      // input wire txMetadataInApp_TVALID
  .txMetadataInApp_TREADY(lbTxMetadataOut_TREADY),                      // output wire txMetadataInApp_TREADY
  .txMetadataInApp_TDATA(lbTxMetadataOut_TDATA),                        // input wire [95 : 0] txMetadataInApp_TDATA
  .txMetadataInDhcp_TVALID(dhcp2muxTxMetadataOut_V_TVALID),             // input wire txMetadataInDhcp_TVALID
  .txMetadataInDhcp_TREADY(dhcp2muxTxMetadataOut_V_TREADY),             // output wire txMetadataInDhcp_TREADY
  .txMetadataInDhcp_TDATA(dhcp2muxTxMetadataOut_V_TDATA),               // input wire [95 : 0] txMetadataInDhcp_TDATA
  .txMetadataOut_TVALID(mux2udpTxMetadataOut_V_TVALID),                 // output wire txMetadataOut_TVALID
  .txMetadataOut_TREADY(mux2udpTxMetadataOut_V_TREADY),                 // input wire txMetadataOut_TREADY
  .txMetadataOut_TDATA(mux2udpTxMetadataOut_V_TDATA),                   // output wire [95 : 0] txMetadataOut_TDATA
  .aclk(aclk),                                                       // input wire aclk
  .aresetn(aresetn)                                                     // input wire aresetn
);

dhcp_client_0 myDhcpClient (
    .dhcpEnable_V(dhcpEnable),                                      // input wire [0 : 0] dhcpEnable_V
    .inputIpAddress_V(inputIpAddress),                              // input wire [31 : 0] inputIpAddress_V
    .dhcpIpAddressOut_V(dhcpAddressOut),                          // output wire [31 : 0] dhcpIpAddressOut_V
    .myMacAddress_V(myMacAddress),                                  // input wire [47 : 0] myMacAddress_V
    .m_axis_open_port_TVALID(dhcp2mux_requestPortOpenOut_V_TVALID),       // output wire m_axis_open_port_TVALID
    .m_axis_open_port_TREADY(dhcp2mux_requestPortOpenOut_V_TREADY),       // input wire m_axis_open_port_TREADY
    .m_axis_open_port_TDATA(dhcp2mux_requestPortOpenOut_V_TDATA),         // output wire [15 : 0] m_axis_open_port_TDATA
    .m_axis_tx_data_TVALID(dhcp2mux_TVALID),                              // output wire m_axis_tx_data_TVALID
    .m_axis_tx_data_TREADY(dhcp2mux_TREADY),                              // input wire m_axis_tx_data_TREADY
    .m_axis_tx_data_TDATA(dhcp2mux_TDATA),                                // output wire [63 : 0] m_axis_tx_data_TDATA
    .m_axis_tx_data_TKEEP(dhcp2mux_TKEEP),                                // output wire [7 : 0] m_axis_tx_data_TKEEP
    .m_axis_tx_data_TLAST(dhcp2mux_TLAST),                                // output wire [0 : 0] m_axis_tx_data_TLAST
    .m_axis_tx_length_TVALID(dhcp2muxTxLengthOut_V_V_TVALID),             // output wire m_axis_tx_length_TVALID
    .m_axis_tx_length_TREADY(dhcp2muxTxLengthOut_V_V_TREADY),             // input wire m_axis_tx_length_TREADY
    .m_axis_tx_length_TDATA(dhcp2muxTxLengthOut_V_V_TDATA),               // output wire [15 : 0] m_axis_tx_length_TDATA
    .m_axis_tx_metadata_TVALID(dhcp2muxTxMetadataOut_V_TVALID),           // output wire m_axis_tx_metadata_TVALID
    .m_axis_tx_metadata_TREADY(dhcp2muxTxMetadataOut_V_TREADY),           // input wire m_axis_tx_metadata_TREADY
    .m_axis_tx_metadata_TDATA(dhcp2muxTxMetadataOut_V_TDATA),             // output wire [95 : 0] m_axis_tx_metadata_TDATA
    .s_axis_open_port_status_TVALID(mux2dhcp_portOpenReplyIn_V_V_TVALID), // input wire s_axis_open_port_status_TVALID
    .s_axis_open_port_status_TREADY(mux2dhcp_portOpenReplyIn_V_V_TREADY), // output wire s_axis_open_port_status_TREADY
    .s_axis_open_port_status_TDATA(mux2dhcp_portOpenReplyIn_V_V_TDATA),   // input wire [7 : 0] s_axis_open_port_status_TDATA
    .s_axis_rx_data_TVALID(mux2dhcpRxDataIn_TVALID),                      // input wire s_axis_rx_data_TVALID
    .s_axis_rx_data_TREADY(mux2dhcpRxDataIn_TREADY),                      // output wire s_axis_rx_data_TREADY
    .s_axis_rx_data_TDATA(mux2dhcpRxDataIn_TDATA),                        // input wire [63 : 0] s_axis_rx_data_TDATA
    .s_axis_rx_data_TKEEP(mux2dhcpRxDataIn_TKEEP),                        // input wire [7 : 0] s_axis_rx_data_TKEEP
    .s_axis_rx_data_TLAST(mux2dhcpRxDataIn_TLAST),                        // input wire [0 : 0] s_axis_rx_data_TLAST
    .s_axis_rx_metadata_TVALID(mux2dhcpRxMetadataIn_V_TVALID),            // input wire s_axis_rx_metadata_TVALID
    .s_axis_rx_metadata_TREADY(mux2dhcpRxMetadataIn_V_TREADY),            // output wire s_axis_rx_metadata_TREADY
    .s_axis_rx_metadata_TDATA(mux2dhcpRxMetadataIn_V_TDATA),              // input wire [95 : 0] s_axis_rx_metadata_TDATA
    .aclk(aclk),                                                       // input wire aclk
    .aresetn(aresetn)                                                     // input wire aresetn
);

assign ipAddressOut = dhcpAddressOut;

assign        upd_req_TVALID_out = upd_req_TVALID;
assign        upd_req_TREADY_out = upd_req_TREADY;
assign        upd_req_TDATA_out  = upd_req_TDATA[1]; 
assign        upd_rsp_TVALID_out = upd_rsp_TVALID;
assign        upd_rsp_TREADY_out = upd_rsp_TREADY;
endmodule
