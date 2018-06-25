`timescale 1ns / 1ps
`default_nettype none
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

//`define RX_DDR_BYPASS
`define UDP

module network_stack #(
    parameter MAC_ADDRESS = 48'hE59D02350A00, // LSB first, 00:0A:35:02:9D:E5
    parameter IP_SUBNET_MASK = 32'h00FFFFFF,
    parameter IP_DEFAULT_GATEWAY = 32'h00000000
)(
    input wire           aclk,
    input wire           aresetn,
    // network interface streams
    output wire          AXI_M_Stream_TVALID,
    input wire           AXI_M_Stream_TREADY,
    output wire[63:0]    AXI_M_Stream_TDATA,
    output wire[7:0]     AXI_M_Stream_TKEEP,
    output wire         AXI_M_Stream_TLAST,

    input wire           AXI_S_Stream_TVALID,
    output wire          AXI_S_Stream_TREADY,
    input wire[63:0]     AXI_S_Stream_TDATA,
    input wire[7:0]      AXI_S_Stream_TKEEP,
    input wire           AXI_S_Stream_TLAST,
    
`ifndef RX_DDR_BYPASS
    // memory rx cmd streams
    output wire          m_axis_rxread_cmd_TVALID,
    input wire           m_axis_rxread_cmd_TREADY,
    output wire[71:0]    m_axis_rxread_cmd_TDATA,
    output wire          m_axis_rxwrite_cmd_TVALID,
    input wire           m_axis_rxwrite_cmd_TREADY,
    output wire[71:0]    m_axis_rxwrite_cmd_TDATA,
    // memory rx sts streams
    input wire           s_axis_rxwrite_sts_TVALID,
    output wire          s_axis_rxwrite_sts_TREADY,
    input wire[7:0]      s_axis_rxwrite_sts_TDATA,
    // memory rx data streams  - Read status signals are not used
    input wire           s_axis_rxread_data_TVALID,
    output wire          s_axis_rxread_data_TREADY,
    input wire[63:0]     s_axis_rxread_data_TDATA,
    input wire[7:0]      s_axis_rxread_data_TKEEP,
    input wire           s_axis_rxread_data_TLAST,
    
    output wire          m_axis_rxwrite_data_TVALID,
    input wire           m_axis_rxwrite_data_TREADY,
    output wire[63:0]    m_axis_rxwrite_data_TDATA,
    output wire[7:0]     m_axis_rxwrite_data_TKEEP,
    output wire          m_axis_rxwrite_data_TLAST,
`endif    
    // memory tx cmd streams
    output wire          m_axis_txread_cmd_TVALID,
    input wire           m_axis_txread_cmd_TREADY,
    output wire[71:0]    m_axis_txread_cmd_TDATA,
    output wire          m_axis_txwrite_cmd_TVALID,
    input wire           m_axis_txwrite_cmd_TREADY,
    output wire[71:0]    m_axis_txwrite_cmd_TDATA,
    // memory tx sts streams - Read status signals are not used
    input wire           s_axis_txwrite_sts_TVALID,
    output wire          s_axis_txwrite_sts_TREADY,
    input wire[7:0]      s_axis_txwrite_sts_TDATA,
    // memory tx data streams
    input wire           s_axis_txread_data_TVALID,
    output wire          s_axis_txread_data_TREADY,
    input wire[63:0]     s_axis_txread_data_TDATA,
    input wire[7:0]      s_axis_txread_data_TKEEP,
    input wire           s_axis_txread_data_TLAST,
    
    output wire          m_axis_txwrite_data_TVALID,
    input wire           m_axis_txwrite_data_TREADY,
    output wire[63:0]    m_axis_txwrite_data_TDATA,
    output wire[7:0]     m_axis_txwrite_data_TKEEP,
    output wire          m_axis_txwrite_data_TLAST,
    
    //application interface streams
    output wire          m_axis_listen_port_status_TVALID,
    input wire           m_axis_listen_port_status_TREADY,
    output wire[7:0]     m_axis_listen_port_status_TDATA,
    output wire          m_axis_notifications_TVALID,
    input wire           m_axis_notifications_TREADY,
    output wire[87:0]    m_axis_notifications_TDATA,
    output wire          m_axis_open_status_TVALID,
    input wire           m_axis_open_status_TREADY,
    output wire[23:0]    m_axis_open_status_TDATA,
    output wire          m_axis_rx_data_TVALID,
    input wire           m_axis_rx_data_TREADY,
    output wire[63:0]    m_axis_rx_data_TDATA,
    output wire[7:0]     m_axis_rx_data_TKEEP,
    output wire          m_axis_rx_data_TLAST,
    output wire          m_axis_rx_metadata_TVALID,
    input wire           m_axis_rx_metadata_TREADY,
    output wire[15:0]    m_axis_rx_metadata_TDATA,
    output wire          m_axis_tx_status_TVALID,
    input wire           m_axis_tx_status_TREADY,
    output wire[31:0]    m_axis_tx_status_TDATA,
    input wire           s_axis_listen_port_TVALID,
    output wire          s_axis_listen_port_TREADY,
    input wire[15:0]     s_axis_listen_port_TDATA,
    //input wire           s_axis_close_port_TVALID,
    //output wire          s_axis_close_port_TREADY,
    //input wire[15:0]     s_axis_close_port_TDATA,
    input wire           s_axis_close_connection_TVALID,
    output wire          s_axis_close_connection_TREADY,
    input wire[15:0]     s_axis_close_connection_TDATA,
    input wire           s_axis_open_connection_TVALID,
    output wire          s_axis_open_connection_TREADY,
    input wire[47:0]     s_axis_open_connection_TDATA,
    input wire           s_axis_read_package_TVALID,
    output wire          s_axis_read_package_TREADY,
    input wire[31:0]     s_axis_read_package_TDATA,
    input wire           s_axis_tx_data_TVALID,
    output wire          s_axis_tx_data_TREADY,
    input wire[63:0]     s_axis_tx_data_TDATA,
    input wire[7:0]      s_axis_tx_data_TKEEP,
    input wire           s_axis_tx_data_TLAST,
    input wire           s_axis_tx_metadata_TVALID,
    output wire          s_axis_tx_metadata_TREADY,
    input wire[31:0]     s_axis_tx_metadata_TDATA,
`ifdef UDP
    output wire          m_axis_udp_data_TVALID,
    input wire           m_axis_udp_data_TREADY,
    output wire[63:0]    m_axis_udp_data_TDATA,
    output wire[7:0]     m_axis_udp_data_TKEEP,
    output wire          m_axis_udp_data_TLAST,
    output wire          m_axis_udp_metadata_TVALID,
    input wire           m_axis_udp_metadata_TREADY,
    output wire[175:0]   m_axis_udp_metadata_TDATA,
    input wire           s_axis_udp_data_TVALID,
    output wire          s_axis_udp_data_TREADY,
    input wire[63:0]     s_axis_udp_data_TDATA,
    input wire[7:0]      s_axis_udp_data_TKEEP,
    input wire           s_axis_udp_data_TLAST,
    input wire           s_axis_udp_metadata_TVALID,
    output wire          s_axis_udp_metadata_TREADY,
    input wire[175:0]    s_axis_udp_metadata_TDATA,
`endif
   
    input  wire[31:0]    ip_address_in,
    output wire[31:0]    ip_address_out,
    output wire[15:0]    regSessionCount_V,
    output wire          regSessionCount_V_ap_vld,

    input wire[3:0]      board_number,
    input wire[1:0]      subnet_number
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

wire            axi_iph_to_toe_slice_tvalid;
wire            axi_iph_to_toe_slice_tready;
wire[63:0]      axi_iph_to_toe_slice_tdata;
wire[7:0]       axi_iph_to_toe_slice_tkeep;
wire            axi_iph_to_toe_slice_tlast;

wire            axi_toe_slice_to_toe_tvalid;
wire            axi_toe_slice_to_toe_tready;
wire[63:0]      axi_toe_slice_to_toe_tdata;
wire[7:0]       axi_toe_slice_to_toe_tkeep;
wire            axi_toe_slice_to_toe_tlast;

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
wire        axi_toe_to_toe_slice_tvalid;
wire        axi_toe_to_toe_slice_tready;
wire[63:0]  axi_toe_to_toe_slice_tdata;
wire[7:0]   axi_toe_to_toe_slice_tkeep;
wire        axi_toe_to_toe_slice_tlast;


// TCP Offload SmartCAM signals //
wire        upd_req_TVALID;
wire        upd_req_TREADY;
wire[111:0] upd_req_TDATA; //(1 + 1 + 14 + 96) - 1 = 111
wire        upd_rsp_TVALID;
wire        upd_rsp_TREADY;
wire[15:0]  upd_rsp_TDATA;

wire        lup_req_TVALID;
wire        lup_req_TREADY;
wire[97:0]  lup_req_TDATA; //should be 96, also wrong in SmartCam
wire        lup_rsp_TVALID;
wire        lup_rsp_TREADY;
wire[15:0]  lup_rsp_TDATA;

`ifdef RX_DDR_BYPASS
//RX Buffer bypass data streams
wire axis_rxbuffer2app_tvalid;
wire axis_rxbuffer2app_tready;
wire[63:0] axis_rxbuffer2app_tdata;
wire[7:0] axis_rxbuffer2app_tkeep;
wire axis_rxbuffer2app_tlast;

wire axis_tcp2rxbuffer_tvalid;
wire axis_tcp2rxbuffer_tready;
wire[63:0] axis_tcp2rxbuffer_tdata;
wire[7:0] axis_tcp2rxbuffer_tkeep;
wire axis_tcp2rxbuffer_tlast;

wire[31:0] rx_buffer_data_count;
`endif


// Register and distribute ip address
wire[31:0]  dhcp_ip_address;
wire        dhcp_ip_address_en;
reg[47:0]   mie_mac_address;
reg[47:0]   arp_mac_address;
reg[31:0]   iph_ip_address;
reg[31:0]   arp_ip_address;
reg[31:0]   toe_ip_address;
reg[31:0]   udp_ip_address;
reg[31:0]   ip_subnet_mask;
reg[31:0]   ip_default_gateway;

always @(posedge aclk)
begin
    if (aresetn == 0) begin
        mie_mac_address <= 48'h000000000000;
        arp_mac_address <= 48'h000000000000;
        iph_ip_address <= 32'h00000000;
        arp_ip_address <= 32'h00000000;
        toe_ip_address <= 32'h00000000;
        udp_ip_address <= 32'h00000000;
        ip_subnet_mask <= 32'h00000000;
        ip_default_gateway <= 32'h00000000;
    end
    else begin
        mie_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        arp_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        /*if (DHCP_EN == 1) begin
            if (dhcp_ip_address_en == 1'b1) begin
                iph_ip_address <= dhcp_ip_address;
                arp_ip_address <= dhcp_ip_address;
                toe_ip_address <= dhcp_ip_address;
            end
        end
        else begin*/
            iph_ip_address <= {ip_address_in[31:28], ip_address_in[27:24]+board_number, ip_address_in[23:4], ip_address_in[3:0]+subnet_number};
            arp_ip_address <= {ip_address_in[31:28], ip_address_in[27:24]+board_number, ip_address_in[23:4], ip_address_in[3:0]+subnet_number};
            toe_ip_address <= {ip_address_in[31:28], ip_address_in[27:24]+board_number, ip_address_in[23:4], ip_address_in[3:0]+subnet_number};
            udp_ip_address <= {ip_address_in[31:28], ip_address_in[27:24]+board_number, ip_address_in[23:4], ip_address_in[3:0]+subnet_number};
            ip_subnet_mask <= IP_SUBNET_MASK;
            ip_default_gateway <= {IP_DEFAULT_GATEWAY[31:4], IP_DEFAULT_GATEWAY[3:0]+subnet_number};
        //end
    end
end
// ip address output
assign ip_address_out = iph_ip_address;


toe_ip toe_inst (
// Data output
.m_axis_tcp_data_TVALID(axi_toe_to_toe_slice_tvalid), // output AXI_M_Stream_TVALID
.m_axis_tcp_data_TREADY(axi_toe_to_toe_slice_tready), // input AXI_M_Stream_TREADY
.m_axis_tcp_data_TDATA(axi_toe_to_toe_slice_tdata), // output [63 : 0] AXI_M_Stream_TDATA
.m_axis_tcp_data_TKEEP(axi_toe_to_toe_slice_tkeep), // output [7 : 0] AXI_M_Stream_TSTRB
.m_axis_tcp_data_TLAST(axi_toe_to_toe_slice_tlast), // output [0 : 0] AXI_M_Stream_TLAST
// Data input
.s_axis_tcp_data_TVALID(axi_toe_slice_to_toe_tvalid), // input AXI_S_Stream_TVALID
.s_axis_tcp_data_TREADY(axi_toe_slice_to_toe_tready), // output AXI_S_Stream_TREADY
.s_axis_tcp_data_TDATA(axi_toe_slice_to_toe_tdata), // input [63 : 0] AXI_S_Stream_TDATA
.s_axis_tcp_data_TKEEP(axi_toe_slice_to_toe_tkeep), // input [7 : 0] AXI_S_Stream_TKEEP
.s_axis_tcp_data_TLAST(axi_toe_slice_to_toe_tlast), // input [0 : 0] AXI_S_Stream_TLAST
`ifndef RX_DDR_BYPASS
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
`else
// rx buffer read path
.s_axis_rxread_data_TVALID(axis_rxbuffer2app_tvalid),
.s_axis_rxread_data_TREADY(axis_rxbuffer2app_tready),
.s_axis_rxread_data_TDATA(axis_rxbuffer2app_tdata),
.s_axis_rxread_data_TKEEP(axis_rxbuffer2app_tkeep),
.s_axis_rxread_data_TLAST(axis_rxbuffer2app_tlast),
// rx buffer write path
.m_axis_rxwrite_data_TVALID(axis_tcp2rxbuffer_tvalid),
.m_axis_rxwrite_data_TREADY(axis_tcp2rxbuffer_tready),
.m_axis_rxwrite_data_TDATA(axis_tcp2rxbuffer_tdata),
.m_axis_rxwrite_data_TKEEP(axis_tcp2rxbuffer_tkeep),
.m_axis_rxwrite_data_TLAST(axis_tcp2rxbuffer_tlast),
`endif
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
.s_axis_close_conn_req_TVALID(s_axis_close_connection_TVALID),
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

.myIpAddress_V(toe_ip_address),
.regSessionCount_V(regSessionCount_V),
.regSessionCount_V_ap_vld(regSessionCount_V_ap_vld),
`ifdef RX_DDR_BYPASS
//for external RX Buffer
.axis_data_count_V(rx_buffer_data_count),
.axis_max_data_count_V(32'd2048),
`endif
.aclk(aclk),                                                        // input aclk
.aresetn(aresetn)                                                   // input aresetn
);

`ifdef RX_DDR_BYPASS
//RX BUFFER FIFO
axis_data_fifo_64_d2048 rx_buffer_fifo (
  .s_aresetn(aresetn),          // input wire s_axis_aresetn
  .s_aclk(aclk),                // input wire s_axis_aclk
  .s_axis_tvalid(axis_tcp2rxbuffer_tvalid),
  .s_axis_tready(axis_tcp2rxbuffer_tready),
  .s_axis_tdata(axis_tcp2rxbuffer_tdata),
  .s_axis_tkeep(axis_tcp2rxbuffer_tkeep),
  .s_axis_tlast(axis_tcp2rxbuffer_tlast),
  .m_axis_tvalid(axis_rxbuffer2app_tvalid),
  .m_axis_tready(axis_rxbuffer2app_tready),
  .m_axis_tdata(axis_rxbuffer2app_tdata),
  .m_axis_tkeep(axis_rxbuffer2app_tkeep),
  .m_axis_tlast(axis_rxbuffer2app_tlast),
  .axis_data_count(rx_buffer_data_count[11:0])
);
assign rx_buffer_data_count[31:12] = 20'h0;
`endif

SmartCamCtl SmartCamCtl_inst
(
.clk(aclk),
.rst(~aresetn),
.led0(),//(sc_led0),
.led1(),//(sc_led1),
.cam_ready(),//(cam_ready),

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


`ifndef UDP
assign axi_udp_to_merge_tvalid = 1'b0;
assign axi_udp_to_merge_tdata = 0;
assign axi_udp_to_merge_tkeep = 0;
assign axi_udp_to_merge_tlast = 0;
assign axi_iph_to_udp_tready = 1'b1;
`else

wire        axis_ip_to_udp_meta_tvalid;
wire        axis_ip_to_udp_meta_tready;
wire[47:0]  axis_ip_to_udp_meta_tdata;
wire        axis_ip_to_udp_data_tvalid;
wire        axis_ip_to_udp_data_tready;
wire[63:0]  axis_ip_to_udp_data_tdata;
wire[7:0]   axis_ip_to_udp_data_tkeep;
wire        axis_ip_to_udp_data_tlast;

wire        axis_udp_to_ip_meta_tvalid;
wire        axis_udp_to_ip_meta_tready;
wire[47:0]  axis_udp_to_ip_meta_tdata;
wire        axis_udp_to_ip_data_tvalid;
wire        axis_udp_to_ip_data_tready;
wire[63:0]  axis_udp_to_ip_data_tdata;
wire[7:0]   axis_udp_to_ip_data_tkeep;
wire        axis_udp_to_ip_data_tlast;
 
// UDP Offload Engine
ipv4_ip ipv4_inst (
  .local_ipv4_address_V(udp_ip_address),    // input wire [31 : 0] local_ipv4_address_V
  //RX
  .s_axis_rx_data_TVALID(axi_iph_to_udp_tvalid),  // input wire s_axis_rx_data_TVALID
  .s_axis_rx_data_TREADY(axi_iph_to_udp_tready),  // output wire s_axis_rx_data_TREADY
  .s_axis_rx_data_TDATA(axi_iph_to_udp_tdata),    // input wire [63 : 0] s_axis_rx_data_TDATA
  .s_axis_rx_data_TKEEP(axi_iph_to_udp_tkeep),    // input wire [7 : 0] s_axis_rx_data_TKEEP
  .s_axis_rx_data_TLAST(axi_iph_to_udp_tlast),    // input wire [0 : 0] s_axis_rx_data_TLAST
  .m_axis_rx_meta_TVALID(axis_ip_to_udp_meta_tvalid),  // output wire m_axis_rx_meta_TVALID
  .m_axis_rx_meta_TREADY(axis_ip_to_udp_meta_tready),  // input wire m_axis_rx_meta_TREADY
  .m_axis_rx_meta_TDATA(axis_ip_to_udp_meta_tdata),    // output wire [47 : 0] m_axis_rx_meta_TDATA
  .m_axis_rx_data_TVALID(axis_ip_to_udp_data_tvalid),  // output wire m_axis_rx_data_TVALID
  .m_axis_rx_data_TREADY(axis_ip_to_udp_data_tready),  // input wire m_axis_rx_data_TREADY
  .m_axis_rx_data_TDATA(axis_ip_to_udp_data_tdata),    // output wire [63 : 0] m_axis_rx_data_TDATA
  .m_axis_rx_data_TKEEP(axis_ip_to_udp_data_tkeep),    // output wire [7 : 0] m_axis_rx_data_TKEEP
  .m_axis_rx_data_TLAST(axis_ip_to_udp_data_tlast),    // output wire [0 : 0] m_axis_rx_data_TLAST
  //TX
  .s_axis_tx_meta_TVALID(axis_udp_to_ip_meta_tvalid),  // input wire s_axis_tx_meta_TVALID
  .s_axis_tx_meta_TREADY(axis_udp_to_ip_meta_tready),  // output wire s_axis_tx_meta_TREADY
  .s_axis_tx_meta_TDATA(axis_udp_to_ip_meta_tdata),    // input wire [47 : 0] s_axis_tx_meta_TDATA
  .s_axis_tx_data_TVALID(axis_udp_to_ip_data_tvalid),  // input wire s_axis_tx_data_TVALID
  .s_axis_tx_data_TREADY(axis_udp_to_ip_data_tready),  // output wire s_axis_tx_data_TREADY
  .s_axis_tx_data_TDATA(axis_udp_to_ip_data_tdata),    // input wire [63 : 0] s_axis_tx_data_TDATA
  .s_axis_tx_data_TKEEP(axis_udp_to_ip_data_tkeep),    // input wire [7 : 0] s_axis_tx_data_TKEEP
  .s_axis_tx_data_TLAST(axis_udp_to_ip_data_tlast),    // input wire [0 : 0] s_axis_tx_data_TLAST
  .m_axis_tx_data_TVALID(axi_udp_to_merge_tvalid),  // output wire m_axis_tx_data_TVALID
  .m_axis_tx_data_TREADY(axi_udp_to_merge_tready),  // input wire m_axis_tx_data_TREADY
  .m_axis_tx_data_TDATA(axi_udp_to_merge_tdata),    // output wire [63 : 0] m_axis_tx_data_TDATA
  .m_axis_tx_data_TKEEP(axi_udp_to_merge_tkeep),    // output wire [7 : 0] m_axis_tx_data_TKEEP
  .m_axis_tx_data_TLAST(axi_udp_to_merge_tlast),    // output wire [0 : 0] m_axis_tx_data_TLAST

  .aclk(aclk),                                    // input wire aclk
  .aresetn(aresetn)                              // input wire aresetn
);

udp_ip udp_inst (
  //.reg_ip_address_V(udp_ip_address),                  // input wire [127 : 0] reg_ip_address_V
  .reg_listen_port_V(16'h8000),
  //RX
  .s_axis_rx_meta_TVALID(axis_ip_to_udp_meta_tvalid),
  .s_axis_rx_meta_TREADY(axis_ip_to_udp_meta_tready),
  .s_axis_rx_meta_TDATA(axis_ip_to_udp_meta_tdata),
  .s_axis_rx_data_TVALID(axis_ip_to_udp_data_tvalid),        // input wire s_axis_rx_data_TVALID
  .s_axis_rx_data_TREADY(axis_ip_to_udp_data_tready),        // output wire s_axis_rx_data_TREADY
  .s_axis_rx_data_TDATA(axis_ip_to_udp_data_tdata),          // input wire [63 : 0] s_axis_rx_data_TDATA
  .s_axis_rx_data_TKEEP(axis_ip_to_udp_data_tkeep),          // input wire [7 : 0] s_axis_rx_data_TKEEP
  .s_axis_rx_data_TLAST(axis_ip_to_udp_data_tlast),          // input wire [0 : 0] s_axis_rx_data_TLAST
  .m_axis_rx_meta_TVALID(m_axis_udp_metadata_TVALID),        // output wire m_axis_rx_meta_TVALID
  .m_axis_rx_meta_TREADY(m_axis_udp_metadata_TREADY),        // input wire m_axis_rx_meta_TREADY
  .m_axis_rx_meta_TDATA(m_axis_udp_metadata_TDATA),          // output wire [159 : 0] m_axis_rx_meta_TDATA
  .m_axis_rx_data_TVALID(m_axis_udp_data_TVALID),        // output wire m_axis_rx_data_TVALID
  .m_axis_rx_data_TREADY(m_axis_udp_data_TREADY),        // input wire m_axis_rx_data_TREADY
  .m_axis_rx_data_TDATA(m_axis_udp_data_TDATA),          // output wire [63 : 0] m_axis_rx_data_TDATA
  .m_axis_rx_data_TKEEP(m_axis_udp_data_TKEEP),          // output wire [7 : 0] m_axis_rx_data_TKEEP
  .m_axis_rx_data_TLAST(m_axis_udp_data_TLAST),          // output wire [0 : 0] m_axis_rx_data_TLAST
  //TX
  .s_axis_tx_meta_TVALID(s_axis_udp_metadata_TVALID),
  .s_axis_tx_meta_TREADY(s_axis_udp_metadata_TREADY),
  .s_axis_tx_meta_TDATA(s_axis_udp_metadata_TDATA),
  .s_axis_tx_data_TVALID(s_axis_udp_data_TVALID),        // input wire s_axis_tx_data_TVALID
  .s_axis_tx_data_TREADY(s_axis_udp_data_TREADY),        // output wire s_axis_tx_data_TREADY
  .s_axis_tx_data_TDATA(s_axis_udp_data_TDATA),          // input wire [63 : 0] s_axis_tx_data_TDATA
  .s_axis_tx_data_TKEEP(s_axis_udp_data_TKEEP),          // input wire [7 : 0] s_axis_tx_data_TKEEP
  .s_axis_tx_data_TLAST(s_axis_udp_data_TLAST),          // input wire [0 : 0] s_axis_tx_data_TLAST
  .m_axis_tx_meta_TVALID(axis_udp_to_ip_meta_tvalid),        // input wire m_axis_tx_meta_TVALID
  .m_axis_tx_meta_TREADY(axis_udp_to_ip_meta_tready),        // output wire m_axis_tx_meta_TREADY
  .m_axis_tx_meta_TDATA(axis_udp_to_ip_meta_tdata),          // input wire [159 : 0] m_axis_tx_meta_TDATA
  .m_axis_tx_data_TVALID(axis_udp_to_ip_data_tvalid),        // output wire m_axis_tx_data_TVALID
  .m_axis_tx_data_TREADY(axis_udp_to_ip_data_tready),        // input wire m_axis_tx_data_TREADY
  .m_axis_tx_data_TDATA(axis_udp_to_ip_data_tdata),          // output wire [63 : 0] m_axis_tx_data_TDATA
  .m_axis_tx_data_TKEEP(axis_udp_to_ip_data_tkeep),          // output wire [7 : 0] m_axis_tx_data_TKEEP
  .m_axis_tx_data_TLAST(axis_udp_to_ip_data_tlast),          // output wire [0 : 0] m_axis_tx_data_TLAST

  .aclk(aclk),                                          // input wire aclk
  .aresetn(aresetn)                                    // input wire aresetn
);

`endif

 
ip_handler_ip ip_handler_inst (
.m_axis_ARP_TVALID(axi_iph_to_arp_slice_tvalid), // output AXI4Stream_M_TVALID
.m_axis_ARP_TREADY(axi_iph_to_arp_slice_tready), // input AXI4Stream_M_TREADY
.m_axis_ARP_TDATA(axi_iph_to_arp_slice_tdata), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_ARP_TKEEP(axi_iph_to_arp_slice_tkeep), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_ARP_TLAST(axi_iph_to_arp_slice_tlast), // output [0 : 0] AXI4Stream_M_TLAST

.m_axis_ICMP_TVALID(axi_iph_to_icmp_slice_tvalid), // output AXI4Stream_M_TVALID
.m_axis_ICMP_TREADY(axi_iph_to_icmp_slice_tready), // input AXI4Stream_M_TREADY
.m_axis_ICMP_TDATA(axi_iph_to_icmp_slice_tdata), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_ICMP_TKEEP(axi_iph_to_icmp_slice_tkeep), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_ICMP_TLAST(axi_iph_to_icmp_slice_tlast), // output [0 : 0] AXI4Stream_M_TLAST

.m_axis_UDP_TVALID(axi_iph_to_udp_tvalid),          // output AXI4Stream_M_TVALID
.m_axis_UDP_TREADY(axi_iph_to_udp_tready),          // input AXI4Stream_M_TREADY
.m_axis_UDP_TDATA(axi_iph_to_udp_tdata),            // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_UDP_TKEEP(axi_iph_to_udp_tkeep),            // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_UDP_TLAST(axi_iph_to_udp_tlast),            // output [0 : 0]  

.m_axis_TCP_TVALID(axi_iph_to_toe_slice_tvalid), // output AXI4Stream_M_TVALID
.m_axis_TCP_TREADY(axi_iph_to_toe_slice_tready), // input AXI4Stream_M_TREADY
.m_axis_TCP_TDATA(axi_iph_to_toe_slice_tdata), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_TCP_TKEEP(axi_iph_to_toe_slice_tkeep), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_TCP_TLAST(axi_iph_to_toe_slice_tlast), // output [0 : 0] AXI4Stream_M_TLAST

.s_axis_raw_TVALID(AXI_S_Stream_TVALID), // input AXI4Stream_S_TVALID
.s_axis_raw_TREADY(AXI_S_Stream_TREADY), // output AXI4Stream_S_TREADY
.s_axis_raw_TDATA(AXI_S_Stream_TDATA), // input [63 : 0] AXI4Stream_S_TDATA
.s_axis_raw_TKEEP(AXI_S_Stream_TKEEP), // input [7 : 0] AXI4Stream_S_TSTRB
.s_axis_raw_TLAST(AXI_S_Stream_TLAST), // input [0 : 0] AXI4Stream_S_TLAST

.myIpAddress_V(iph_ip_address),

.aclk(aclk), // input aclk
.aresetn(aresetn) // input aresetn
);

// ARP lookup
wire        axis_arp_lookup_request_TVALID;
wire        axis_arp_lookup_request_TREADY;
wire[31:0]  axis_arp_lookup_request_TDATA;
wire        axis_arp_lookup_reply_TVALID;
wire        axis_arp_lookup_reply_TREADY;
wire[55:0]  axis_arp_lookup_reply_TDATA;

mac_ip_encode_ip mac_ip_encode_inst (
.m_axis_ip_TVALID(axi_mie_to_intercon_tvalid),
.m_axis_ip_TREADY(axi_mie_to_intercon_tready),
.m_axis_ip_TDATA(axi_mie_to_intercon_tdata),
.m_axis_ip_TKEEP(axi_mie_to_intercon_tkeep),
.m_axis_ip_TLAST(axi_mie_to_intercon_tlast),
.m_axis_arp_lookup_request_TVALID(axis_arp_lookup_request_TVALID),
.m_axis_arp_lookup_request_TREADY(axis_arp_lookup_request_TREADY),
.m_axis_arp_lookup_request_TDATA(axis_arp_lookup_request_TDATA),
.s_axis_ip_TVALID(axi_intercon_to_mie_tvalid),
.s_axis_ip_TREADY(axi_intercon_to_mie_tready),
.s_axis_ip_TDATA(axi_intercon_to_mie_tdata),
.s_axis_ip_TKEEP(axi_intercon_to_mie_tkeep),
.s_axis_ip_TLAST(axi_intercon_to_mie_tlast),
.s_axis_arp_lookup_reply_TVALID(axis_arp_lookup_reply_TVALID),
.s_axis_arp_lookup_reply_TREADY(axis_arp_lookup_reply_TREADY),
.s_axis_arp_lookup_reply_TDATA(axis_arp_lookup_reply_TDATA),

.myMacAddress_V(mie_mac_address),                                    // input wire [47 : 0] regMacAddress_V
.regSubNetMask_V(ip_subnet_mask),                                    // input wire [31 : 0] regSubNetMask_V
.regDefaultGateway_V(ip_default_gateway),                            // input wire [31 : 0] regDefaultGateway_V
  
.aclk(aclk), // input aclk
.aresetn(aresetn) // input aresetn
);

// merges icmp and tcp
axis_interconnect_3to1 ip_merger (
  .ACLK(aclk),                                  // input wire ACLK
  .ARESETN(aresetn),                            // input wire ARESETN

  .S00_AXIS_ACLK(aclk),                // input wire S00_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn),          // input wire S00_AXIS_ARESETN
  .S00_AXIS_TVALID(axi_icmp_to_icmp_slice_tvalid),            // input wire S00_AXIS_TVALID
  .S00_AXIS_TREADY(axi_icmp_to_icmp_slice_tready),            // output wire S00_AXIS_TREADY
  .S00_AXIS_TDATA(axi_icmp_to_icmp_slice_tdata),              // input wire [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axi_icmp_to_icmp_slice_tkeep),              // input wire [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axi_icmp_to_icmp_slice_tlast),              // input wire S00_AXIS_TLAST

  .S01_AXIS_ACLK(aclk),                // input wire S01_AXIS_ACLK
  .S01_AXIS_ARESETN(aresetn),          // input wire S01_AXIS_ARESETN
  .S01_AXIS_TVALID(axi_udp_to_merge_tvalid),            // input wire S01_AXIS_TVALID
  .S01_AXIS_TREADY(axi_udp_to_merge_tready),            // output wire S01_AXIS_TREADY
  .S01_AXIS_TDATA(axi_udp_to_merge_tdata),              // input wire [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axi_udp_to_merge_tkeep),              // input wire [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axi_udp_to_merge_tlast),              // input wire S01_AXIS_TLAST

  .S02_AXIS_ACLK(aclk),                // input wire S02_AXIS_ACLK
  .S02_AXIS_ARESETN(aresetn),          // input wire S02_AXIS_ARESETN
  .S02_AXIS_TVALID(axi_toe_to_toe_slice_tvalid),            // input wire S02_AXIS_TVALID
  .S02_AXIS_TREADY(axi_toe_to_toe_slice_tready),            // output wire S02_AXIS_TREADY
  .S02_AXIS_TDATA(axi_toe_to_toe_slice_tdata),              // input wire [63 : 0] S02_AXIS_TDATA
  .S02_AXIS_TKEEP(axi_toe_to_toe_slice_tkeep),              // input wire [7 : 0] S02_AXIS_TKEEP
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

arp_server_subnet_ip arp_server_inst(
.m_axis_TVALID(axi_arp_to_arp_slice_tvalid),
.m_axis_TREADY(axi_arp_to_arp_slice_tready),
.m_axis_TDATA(axi_arp_to_arp_slice_tdata),
.m_axis_TKEEP(axi_arp_to_arp_slice_tkeep),
.m_axis_TLAST(axi_arp_to_arp_slice_tlast),
.m_axis_arp_lookup_reply_TVALID(axis_arp_lookup_reply_TVALID),
.m_axis_arp_lookup_reply_TREADY(axis_arp_lookup_reply_TREADY),
.m_axis_arp_lookup_reply_TDATA(axis_arp_lookup_reply_TDATA),
.s_axis_TVALID(axi_arp_slice_to_arp_tvalid),
.s_axis_TREADY(axi_arp_slice_to_arp_tready),
.s_axis_TDATA(axi_arp_slice_to_arp_tdata),
.s_axis_TKEEP(axi_arp_slice_to_arp_tkeep),
.s_axis_TLAST(axi_arp_slice_to_arp_tlast),
.s_axis_arp_lookup_request_TVALID(axis_arp_lookup_request_TVALID),
.s_axis_arp_lookup_request_TREADY(axis_arp_lookup_request_TREADY),
.s_axis_arp_lookup_request_TDATA(axis_arp_lookup_request_TDATA),

.myMacAddress_V(arp_mac_address),
.myIpAddress_V(arp_ip_address),

.aclk(aclk), // input aclk
.aresetn(aresetn) // input aresetn
);

assign axis_udp_to_icmp_tvalid = 0;
assign axis_udp_to_icmp_tdata = 0;
assign axis_udp_to_icmp_tkeep = 0;
assign axis_udp_to_icmp_tlast = 0;
assign axis_ttl_to_icmp_tvalid = 0;
assign axis_ttl_to_icmp_tdata = 0;
assign axis_ttl_to_icmp_tkeep = 0;
assign axis_ttl_to_icmp_tlast = 0;

icmp_server_ip icmp_server_inst (
  .s_axis_TVALID(axi_icmp_slice_to_icmp_tvalid),    // input wire dataIn_TVALID
  .s_axis_TREADY(axi_icmp_slice_to_icmp_tready),    // output wire dataIn_TREADY
  .s_axis_TDATA(axi_icmp_slice_to_icmp_tdata),      // input wire [63 : 0] dataIn_TDATA
  .s_axis_TKEEP(axi_icmp_slice_to_icmp_tkeep),      // input wire [7 : 0] dataIn_TKEEP
  .s_axis_TLAST(axi_icmp_slice_to_icmp_tlast),      // input wire [0 : 0] dataIn_TLAST
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
  .m_axis_TVALID(axi_icmp_to_icmp_slice_tvalid),   // output wire dataOut_TVALID
  .m_axis_TREADY(axi_icmp_to_icmp_slice_tready),   // input wire dataOut_TREADY
  .m_axis_TDATA(axi_icmp_to_icmp_slice_tdata),     // output wire [63 : 0] dataOut_TDATA
  .m_axis_TKEEP(axi_icmp_to_icmp_slice_tkeep),     // output wire [7 : 0] dataOut_TKEEP
  .m_axis_TLAST(axi_icmp_to_icmp_slice_tlast),     // output wire [0 : 0] dataOut_TLAST
  .aclk(aclk),                                    // input wire ap_clk
  .aresetn(aresetn)                                // input wire ap_rst_n
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
 // TOE Input Slice
axis_register_slice_64 axis_register_toe_in_slice(
.aclk(aclk),
.aresetn(aresetn),
.s_axis_tvalid(axi_iph_to_toe_slice_tvalid),
.s_axis_tready(axi_iph_to_toe_slice_tready),
.s_axis_tdata(axi_iph_to_toe_slice_tdata),
.s_axis_tkeep(axi_iph_to_toe_slice_tkeep),
.s_axis_tlast(axi_iph_to_toe_slice_tlast),
.m_axis_tvalid(axi_toe_slice_to_toe_tvalid),
.m_axis_tready(axi_toe_slice_to_toe_tready),
.m_axis_tdata(axi_toe_slice_to_toe_tdata),
.m_axis_tkeep(axi_toe_slice_to_toe_tkeep),
.m_axis_tlast(axi_toe_slice_to_toe_tlast)
);



endmodule

`default_nettype wire
