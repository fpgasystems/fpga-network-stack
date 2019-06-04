`timescale 1ns / 1ps
`default_nettype none
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 21.11.2013 10:48:44
// Design Name: 
// Module Name: roce_stack_wrapper
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

`define IP_VERSION4
`define POINTER_CHASING

`define UDP


`include "os_types.svh"

module network_stack #(
    parameter NET_BANDWIDTH = 10,
    parameter WIDTH = 64,
    parameter MAC_ADDRESS = 48'hE59D02350A00, // LSB first, 00:0A:35:02:9D:E5
    parameter IPV6_ADDRESS= 128'hE59D_02FF_FF35_0A02_0000_0000_0000_80FE, //LSB first: FE80_0000_0000_0000_020A_35FF_FF02_9DE5,
    parameter IP_SUBNET_MASK = 32'h00FFFFFF,
    parameter IP_DEFAULT_GATEWAY = 32'h00000000,
    parameter DHCP_EN   = 0,
    parameter RX_DDR_BYPASS_EN = 0
)(
    input wire          net_clk,
    input wire          net_aresetn,
    input wire          pcie_clk,
    input wire          pcie_aresetn,

    /* CONTROL INTERFACE */
    axi_lite.slave      s_axil,
    axi_mm.slave        s_axim,

    // network interface streams
    axi_stream.slave        s_axis_net,
    axi_stream.master       m_axis_net,

    //RoCE Interface
    //DMA
    axis_meta.rmaster       m_axis_roce_read_cmd,
    axis_meta.rmaster       m_axis_roce_write_cmd,
    axi_stream.slave        s_axis_roce_read_data,
    axi_stream.rmaster      m_axis_roce_write_data,

    //Role
    //axis_mem_cmd.master     m_axis_roce_role_rx_cmd,
    axis_meta.slave         s_axis_roce_role_tx_meta,
    //axi_stream.master       m_axis_roce_role_rx_data,
    axi_stream.slave        s_axis_roce_role_tx_data,

    //TCP/IP Interface
    // memory cmd streams
    axis_mem_cmd.master    m_axis_read_cmd[NUM_TCP_CHANNELS],
    axis_mem_cmd.master    m_axis_write_cmd[NUM_TCP_CHANNELS],
    // memory sts streams
    axis_mem_status.slave     s_axis_read_sts[NUM_TCP_CHANNELS],
    axis_mem_status.slave     s_axis_write_sts[NUM_TCP_CHANNELS],
    // memory data streams
    axi_stream.slave    s_axis_read_data[NUM_TCP_CHANNELS],
    axi_stream.master   m_axis_write_data[NUM_TCP_CHANNELS],

    //pointer chasing
`ifdef POINTER_CHASING
    axis_meta.master    m_axis_rx_pcmeta,
    axis_meta.slave     s_axis_tx_pcmeta,
`endif 

    //Application interface streams
    axis_meta.slave     s_axis_listen_port,
    axis_meta.master    m_axis_listen_port_status,
   
    axis_meta.slave     s_axis_open_connection,
    axis_meta.master    m_axis_open_status,
    axis_meta.slave     s_axis_close_connection,

    axis_meta.master    m_axis_notifications,
    axis_meta.slave     s_axis_read_package,
    
    axis_meta.master    m_axis_rx_metadata,
    axi_stream.master   m_axis_rx_data,
    
    axis_meta.slave     s_axis_tx_metadata,
    axi_stream.slave    s_axis_tx_data,
    axis_meta.master    m_axis_tx_status,
    
    
    //UDP/IP Interface
    axis_meta.master    m_axis_udp_rx_metadata,
    axi_stream.master   m_axis_udp_rx_data,
    axis_meta.slave     s_axis_udp_tx_metadata,
    axi_stream.slave    s_axis_udp_tx_data
    
 );
 
localparam ddrPortNetworkRx = 1;
localparam ddrPortNetworkTx = 0;

// IP Handler Outputs
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_arp_slice();
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_icmp_slice();
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_icmpv6_slice();
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_rocev6_slice();

(* mark_debug = "true" *)wire            axi_iph_to_arp_slice_tvalid;
(* mark_debug = "true" *)wire            axi_iph_to_arp_slice_tready;
(* mark_debug = "true" *)wire            axi_iph_to_arp_slice_tlast;
assign axi_iph_to_arp_slice_tvalid = axis_iph_to_arp_slice.valid;
assign axi_iph_to_arp_slice_tready = axis_iph_to_arp_slice.ready;
assign axi_iph_to_arp_slice_tlast = axis_iph_to_arp_slice.last;

(* mark_debug = "true" *)wire            axi_iph_to_udp_slice_tvalid;
(* mark_debug = "true" *)wire            axi_iph_to_udp_slice_tready;
(* mark_debug = "true" *)wire            axi_iph_to_udp_slice_tlast;
assign axi_iph_to_udp_slice_tvalid = axis_iph_to_udp_slice.valid;
assign axi_iph_to_udp_slice_tready = axis_iph_to_udp_slice.ready;
assign axi_iph_to_udp_slice_tlast = axis_iph_to_udp_slice.last;

//Slice connections on RX path
axi_stream #(.WIDTH(WIDTH))     axis_arp_slice_to_arp();
axi_stream #(.WIDTH(64))     axis_icmp_slice_to_icmp();
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_toe();

// MAC-IP Encode Inputs
axi_stream #(.WIDTH(WIDTH))     axis_intercon_to_mie();
axi_stream #(.WIDTH(WIDTH))     axis_mie_to_intercon();

(* mark_debug = "true" *)wire            axi_intercon_to_mie_tvalid;
(* mark_debug = "true" *)wire            axi_intercon_to_mie_tready;
//wire[63:0]      axi_intercon_to_mie_tdata;
//wire[7:0]       axi_intercon_to_mie_tkeep;
(* mark_debug = "true" *)wire            axi_intercon_to_mie_tlast;
(* mark_debug = "true" *)wire            axi_mie_to_intercon_tvalid;
(* mark_debug = "true" *)wire            axi_mie_to_intercon_tready;
//wire[63:0]      axi_mie_to_intercon_tdata;
//wire[7:0]       axi_mie_to_intercon_tkeep;
(* mark_debug = "true" *)wire            axi_mie_to_intercon_tlast;

assign axi_intercon_to_mie_tvalid = axis_intercon_to_mie.valid;
assign axi_intercon_to_mie_tready = axis_intercon_to_mie.ready;
assign axi_intercon_to_mie_tlast = axis_intercon_to_mie.last;
assign axi_mie_to_intercon_tvalid = axis_mie_to_intercon.valid;
assign axi_mie_to_intercon_tready = axis_mie_to_intercon.ready;
assign axi_mie_to_intercon_tlast = axis_mie_to_intercon.last;

//Slice connections on RX path
axi_stream #(.WIDTH(WIDTH))     axis_arp_to_arp_slice();
axi_stream #(.WIDTH(64))     axis_icmp_to_icmp_slice(); //TODO

//TCP
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_toe_slice();
axi_stream #(.WIDTH(WIDTH))     axis_toe_slice_to_toe();

//UDP
axi_stream #(.WIDTH(WIDTH))     axis_udp_to_udp_slice();
axi_stream #(.WIDTH(WIDTH))     axis_udp_slice_to_merge();
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_udp_slice();
axi_stream #(.WIDTH(WIDTH))     axis_udp_slice_to_udp();

//ROCE
axi_stream #(.WIDTH(WIDTH))     axis_roce_to_roce_slice();
axi_stream #(.WIDTH(WIDTH))     axis_roce_slice_to_merge();
axi_stream #(.WIDTH(WIDTH))     axis_iph_to_roce_slice();
axi_stream #(.WIDTH(WIDTH))     axis_roce_slice_to_udp();


/*assign roce_received = (axi_iph_to_udp_slice_tvalid & axi_iph_to_udp_slice_tready);
reg[7:0] word_counter;
always @(posedge net_clk) begin
    if (~net_aresetn) begin
        word_counter <= 0;
        roce_clear <= 0;
    end
    else begin
        roce_clear <= 0;
        if (axi_iph_to_udp_slice_tvalid & axi_iph_to_udp_slice_tready) begin
            word_counter <= word_counter + 1;
            if (axi_iph_to_udp_slice_tlast) begin
                word_counter <= 0;
                if (word_counter < 10) begin
                    roce_clear <= 1;
                end
            end
        end
    end
end*/

axi_stream #(.WIDTH(WIDTH))     axis_slice_to_ibh();
axi_stream #(.WIDTH(WIDTH))     axis_toe_to_toe_slice();


//ICMPv6
axi_stream #(.WIDTH(WIDTH))   axis_ipv6_to_ethen();
axi_stream #(.WIDTH(WIDTH))   axis_ethencode_to_intercon();

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

//IPV6
axi_stream #(.WIDTH(WIDTH))   axis_icmpv6_to_intercon();
axi_stream #(.WIDTH(WIDTH))   axis_ipv6_to_intercon();

// IPv6 lookup
wire axis_ipv6_res_rsp_TVALID;
wire axis_ipv6_res_rsp_TREADY;
wire [55:0] axis_ipv6_res_rsp_TDATA;

wire axis_ipv6_res_req_TVALID;
wire axis_ipv6_res_req_TREADY;
wire [127:0] axis_ipv6_res_req_TDATA;


// Register and distribute ip address
wire[31:0]  dhcp_ip_address;
wire        dhcp_ip_address_en;
reg[47:0]   mie_mac_address;
reg[47:0]   arp_mac_address;
reg[47:0]   ipv6_mac_address;
reg[31:0]   iph_ip_address;
reg[31:0]   arp_ip_address;
reg[31:0]   toe_ip_address;
reg[31:0]   ip_subnet_mask;
reg[31:0]   ip_default_gateway;
reg[127:0] link_local_ipv6_address;

//assign dhcp_ip_address_en = 1'b1;
//assign dhcp_ip_address = 32'hD1D4010A;

//register reset
(* keep = "true", max_fanout = "256" *) reg aresetn_reg;
(* keep = "true" *)  reg aresetn_reg2;
always @(posedge net_clk)
begin
    aresetn_reg2 <=  net_aresetn;
    aresetn_reg <= aresetn_reg2;
end

always @(posedge net_clk)
begin
    if (aresetn_reg == 0) begin
        mie_mac_address <= 48'h000000000000;
        arp_mac_address <= 48'h000000000000;
        ipv6_mac_address <= 48'h000000000000;
        iph_ip_address <= 32'h00000000;
        arp_ip_address <= 32'h00000000;
        toe_ip_address <= 32'h00000000;
        ip_subnet_mask <= 32'h00000000;
        ip_default_gateway <= 32'h00000000;
        link_local_ipv6_address <= 0;
    end
    else begin
        mie_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        arp_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        ipv6_mac_address <= {MAC_ADDRESS[47:44], (MAC_ADDRESS[43:40]+board_number), MAC_ADDRESS[39:0]};
        //link_local_ipv6_address[127:80] <= ipv6_mac_address;
        //link_local_ipv6_address[15:0] <= 16'h80fe; // fe80
        //link_local_ipv6_address[79:16] <= 64'h0000_0000_0000_0000;
        link_local_ipv6_address <= {IPV6_ADDRESS[127:120]+board_number, IPV6_ADDRESS[119:0]};
        if (DHCP_EN == 1) begin
            if (dhcp_ip_address_en == 1'b1) begin
                iph_ip_address <= dhcp_ip_address;
                arp_ip_address <= dhcp_ip_address;
                toe_ip_address <= dhcp_ip_address;
            end
        end
        else begin
            iph_ip_address <= local_ip_address;
            arp_ip_address <= local_ip_address;
            toe_ip_address <= local_ip_address;
            ip_subnet_mask <= IP_SUBNET_MASK;
            ip_default_gateway <= {local_ip_address[31:28], 8'h01, local_ip_address[23:0]};
        end
    end
end
// ip address output
assign ip_address_used = iph_ip_address;




/*
 * TOE
 */
 
 //TODO
logic[15:0] regSessionCount_V;
logic       regSessionCount_V_ap_vld;

generate
if (RX_DDR_BYPASS_EN == 1) begin
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
end
else begin
assign s_axis_read_sts[ddrPortNetworkRx].ready = 1'b1;
end
endgenerate
assign s_axis_read_sts[ddrPortNetworkTx].ready = 1'b1;


//hack for now //TODO
wire[71:0] axis_write_cmd_data [1:0];
wire[71:0] axis_read_cmd_data [1:0];
generate
if (RX_DDR_BYPASS_EN == 0) begin
assign m_axis_write_cmd[ddrPortNetworkRx].address = axis_write_cmd_data[ddrPortNetworkRx][63:32];
assign m_axis_write_cmd[ddrPortNetworkRx].length = axis_write_cmd_data[ddrPortNetworkRx][22:0];
assign m_axis_read_cmd[ddrPortNetworkRx].address = axis_read_cmd_data[ddrPortNetworkRx][63:32];
assign m_axis_read_cmd[ddrPortNetworkRx].length = axis_read_cmd_data[ddrPortNetworkRx][22:0];
end
endgenerate
assign m_axis_write_cmd[ddrPortNetworkTx].address = axis_write_cmd_data[ddrPortNetworkTx][63:32];
assign m_axis_write_cmd[ddrPortNetworkTx].length = axis_write_cmd_data[ddrPortNetworkTx][22:0];
assign m_axis_read_cmd[ddrPortNetworkTx].address = axis_read_cmd_data[ddrPortNetworkTx][63:32];
assign m_axis_read_cmd[ddrPortNetworkTx].length = axis_read_cmd_data[ddrPortNetworkTx][22:0];



toe_ip toe_inst (
 // Data output
 .m_axis_tcp_data_TVALID(axis_toe_to_toe_slice.valid), // output AXI_M_Stream_TVALID
 .m_axis_tcp_data_TREADY(axis_toe_to_toe_slice.ready), // input AXI_M_Stream_TREADY
 .m_axis_tcp_data_TDATA(axis_toe_to_toe_slice.data), // output [63 : 0] AXI_M_Stream_TDATA
 .m_axis_tcp_data_TKEEP(axis_toe_to_toe_slice.keep), // output [7 : 0] AXI_M_Stream_TSTRB
 .m_axis_tcp_data_TLAST(axis_toe_to_toe_slice.last), // output [0 : 0] AXI_M_Stream_TLAST
 // Data input
 .s_axis_tcp_data_TVALID(axis_toe_slice_to_toe.valid), // input AXI_S_Stream_TVALID
 .s_axis_tcp_data_TREADY(axis_toe_slice_to_toe.ready), // output AXI_S_Stream_TREADY
 .s_axis_tcp_data_TDATA(axis_toe_slice_to_toe.data), // input [63 : 0] AXI_S_Stream_TDATA
 .s_axis_tcp_data_TKEEP(axis_toe_slice_to_toe.keep), // input [7 : 0] AXI_S_Stream_TKEEP
 .s_axis_tcp_data_TLAST(axis_toe_slice_to_toe.last), // input [0 : 0] AXI_S_Stream_TLAST
 `ifndef RX_DDR_BYPASS
 // rx read commands
 .m_axis_rxread_cmd_TVALID(m_axis_read_cmd[ddrPortNetworkRx].valid),
 .m_axis_rxread_cmd_TREADY(m_axis_read_cmd[ddrPortNetworkRx].ready),
 .m_axis_rxread_cmd_TDATA(axis_read_cmd_data[ddrPortNetworkRx]),
 // rx write commands
 .m_axis_rxwrite_cmd_TVALID(m_axis_write_cmd[ddrPortNetworkRx].valid),
 .m_axis_rxwrite_cmd_TREADY(m_axis_write_cmd[ddrPortNetworkRx].ready),
 .m_axis_rxwrite_cmd_TDATA(axis_write_cmd_data[ddrPortNetworkRx]),
 // rx write status
 .s_axis_rxwrite_sts_TVALID(s_axis_write_sts[ddrPortNetworkRx].valid),
 .s_axis_rxwrite_sts_TREADY(s_axis_write_sts[ddrPortNetworkRx].ready),
 .s_axis_rxwrite_sts_TDATA(s_axis_write_sts[ddrPortNetworkRx].data),
 // rx buffer read path
 .s_axis_rxread_data_TVALID(axis_rxread_data.valid),
 .s_axis_rxread_data_TREADY(axis_rxread_data.ready),
 .s_axis_rxread_data_TDATA(axis_rxread_data.data),
 .s_axis_rxread_data_TKEEP(axis_rxread_data.keep),
 .s_axis_rxread_data_TLAST(axis_rxread_data.last),
 // rx buffer write path
 .m_axis_rxwrite_data_TVALID(axis_rxwrite_data.valid),
 .m_axis_rxwrite_data_TREADY(axis_rxwrite_data.ready),
 .m_axis_rxwrite_data_TDATA(axis_rxwrite_data.data),
 .m_axis_rxwrite_data_TKEEP(axis_rxwrite_data.keep),
 .m_axis_rxwrite_data_TLAST(axis_rxwrite_data.last),
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
 .m_axis_txread_cmd_TVALID(m_axis_read_cmd[ddrPortNetworkTx].valid),
 .m_axis_txread_cmd_TREADY(m_axis_read_cmd[ddrPortNetworkTx].ready),
 .m_axis_txread_cmd_TDATA(axis_read_cmd_data[ddrPortNetworkTx]),
 //tx write commands
 .m_axis_txwrite_cmd_TVALID(m_axis_write_cmd[ddrPortNetworkTx].valid),
 .m_axis_txwrite_cmd_TREADY(m_axis_write_cmd[ddrPortNetworkTx].ready),
 .m_axis_txwrite_cmd_TDATA(axis_write_cmd_data[ddrPortNetworkTx]),
 // tx write status
 .s_axis_txwrite_sts_TVALID(s_axis_write_sts[ddrPortNetworkTx].valid),
 .s_axis_txwrite_sts_TREADY(s_axis_write_sts[ddrPortNetworkTx].ready),
 .s_axis_txwrite_sts_TDATA(s_axis_write_sts[ddrPortNetworkTx].data),
 // tx read path
 .s_axis_txread_data_TVALID(axis_txread_data.valid),
 .s_axis_txread_data_TREADY(axis_txread_data.ready),
 .s_axis_txread_data_TDATA(axis_txread_data.data),
 .s_axis_txread_data_TKEEP(axis_txread_data.keep),
 .s_axis_txread_data_TLAST(axis_txread_data.last),
 // tx write path
 .m_axis_txwrite_data_TVALID(axis_txwrite_data.valid),
 .m_axis_txwrite_data_TREADY(axis_txwrite_data.ready),
 .m_axis_txwrite_data_TDATA(axis_txwrite_data.data),
 .m_axis_txwrite_data_TKEEP(axis_txwrite_data.keep),
 .m_axis_txwrite_data_TLAST(axis_txwrite_data.last),
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
 .s_axis_listen_port_req_TVALID(s_axis_listen_port.valid),
 .s_axis_listen_port_req_TREADY(s_axis_listen_port.ready),
 .s_axis_listen_port_req_TDATA(s_axis_listen_port.data),
 .m_axis_listen_port_rsp_TVALID(m_axis_listen_port_status.valid),
 .m_axis_listen_port_rsp_TREADY(m_axis_listen_port_status.ready),
 .m_axis_listen_port_rsp_TDATA(m_axis_listen_port_status.data),
 
 // notification & read request
 .m_axis_notification_TVALID(m_axis_notifications.valid),
 .m_axis_notification_TREADY(m_axis_notifications.ready),
 .m_axis_notification_TDATA(m_axis_notifications.data),
 .s_axis_rx_data_req_TVALID(s_axis_read_package.valid),
 .s_axis_rx_data_req_TREADY(s_axis_read_package.ready),
 .s_axis_rx_data_req_TDATA(s_axis_read_package.data),
 
 // open&close connection
 .s_axis_open_conn_req_TVALID(s_axis_open_connection.valid),
 .s_axis_open_conn_req_TREADY(s_axis_open_connection.ready),
 .s_axis_open_conn_req_TDATA(s_axis_open_connection.data),
 .m_axis_open_conn_rsp_TVALID(m_axis_open_status.valid),
 .m_axis_open_conn_rsp_TREADY(m_axis_open_status.ready),
 .m_axis_open_conn_rsp_TDATA(m_axis_open_status.data),
 .s_axis_close_conn_req_TVALID(s_axis_close_connection.valid),
 .s_axis_close_conn_req_TREADY(s_axis_close_connection.ready),
 .s_axis_close_conn_req_TDATA(s_axis_close_connection.data),
 
 // rx data
 .m_axis_rx_data_rsp_metadata_TVALID(m_axis_rx_metadata.valid),
 .m_axis_rx_data_rsp_metadata_TREADY(m_axis_rx_metadata.ready),
 .m_axis_rx_data_rsp_metadata_TDATA(m_axis_rx_metadata.data),
 .m_axis_rx_data_rsp_TVALID(m_axis_rx_data.valid),
 .m_axis_rx_data_rsp_TREADY(m_axis_rx_data.ready),
 .m_axis_rx_data_rsp_TDATA(m_axis_rx_data.data),
 .m_axis_rx_data_rsp_TKEEP(m_axis_rx_data.keep),
 .m_axis_rx_data_rsp_TLAST(m_axis_rx_data.last),
 
 // tx data
 .s_axis_tx_data_req_metadata_TVALID(s_axis_tx_metadata.valid),
 .s_axis_tx_data_req_metadata_TREADY(s_axis_tx_metadata.ready),
 .s_axis_tx_data_req_metadata_TDATA(s_axis_tx_metadata.data),
 .s_axis_tx_data_req_TVALID(s_axis_tx_data.valid),
 .s_axis_tx_data_req_TREADY(s_axis_tx_data.ready),
 .s_axis_tx_data_req_TDATA(s_axis_tx_data.data),
 .s_axis_tx_data_req_TKEEP(s_axis_tx_data.keep),
 .s_axis_tx_data_req_TLAST(s_axis_tx_data.last),
 .m_axis_tx_data_rsp_TVALID(m_axis_tx_status.valid),
 .m_axis_tx_data_rsp_TREADY(m_axis_tx_status.ready),
 .m_axis_tx_data_rsp_TDATA(m_axis_tx_status.data),
 
 .myIpAddress_V(toe_ip_address),
 .regSessionCount_V(regSessionCount_V),
 .regSessionCount_V_ap_vld(regSessionCount_V_ap_vld),
 `ifdef RX_DDR_BYPASS
 //for external RX Buffer
 .axis_data_count_V(rx_buffer_data_count),
 .axis_max_data_count_V(32'd2048),
 `endif
 .aclk(net_clk),                                                        // input aclk
 .aresetn(net_aresetn)                                                   // input aresetn
 );
 
 `ifdef RX_DDR_BYPASS
 //RX BUFFER FIFO
 axis_data_fifo_64_d2048 rx_buffer_fifo (
   .s_axis_aresetn(net_aresetn),          // input wire s_axis_aresetn
   .s_axis_aclk(net_clk),                // input wire s_axis_aclk
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
 .clk(net_clk),
 .rst(~net_aresetn),
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

/*
 * UDP/IP
 */

axis_meta #(.WIDTH(48))         axis_ip_to_udp_meta();
axis_meta #(.WIDTH(48))         axis_udp_to_ip_meta();

axi_stream #(.WIDTH(WIDTH))       axis_ip_to_udp_data();
axi_stream #(.WIDTH(WIDTH))       axis_udp_to_ip_data();

/*axis_meta #(.WIDTH(176))        s_axis_udp_metadata();
axis_meta #(.WIDTH(176))        m_axis_udp_metadata();

axi_stream #(.WIDTH(WIDTH))      s_axis_udp_data();
axi_stream #(.WIDTH(WIDTH))      m_axis_udp_data();*/
 
 
ipv4_ip ipv4_inst (
   .local_ipv4_address_V(local_ip_address[31:0]),    // input wire [31 : 0] local_ipv4_address_V
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
   //.reg_ip_address_V(udp_ip_address),                  // input wire [127 : 0] reg_ip_address_V
   .reg_listen_port_V(16'h8000),
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


/*
 * Test Dropper
 */
 
//`define ENABLE_DROP

`ifdef ENABLE_DROP 
wire        roce_2_drop_valid;
wire        roce_2_drop_ready; 
wire[63:0]  roce_2_drop_data; 
wire[7:0]   roce_2_drop_keep; 
wire        roce_2_drop_last; 
 
test_dropper_ip test_dropper_inst (
   .dropFrequency_V(16'd10),        // input wire [15 : 0] dropFrequency_V
   .m_axis_data_TVALID(axi_udp_to_udp_slice_tvalid),  // output wire m_axis_data_TVALID
   .m_axis_data_TREADY(axi_udp_to_udp_slice_tready),  // input wire m_axis_data_TREADY
   .m_axis_data_TDATA(axi_udp_to_udp_slice_tdata),    // output wire [63 : 0] m_axis_data_TDATA
   .m_axis_data_TKEEP(axi_udp_to_udp_slice_tkeep),    // output wire [7 : 0] m_axis_data_TKEEP
   .m_axis_data_TLAST(axi_udp_to_udp_slice_tlast),    // output wire [0 : 0] m_axis_data_TLAST
   .s_axis_data_TVALID(roce_2_drop_valid),  // input wire s_axis_data_TVALID
   .s_axis_data_TREADY(roce_2_drop_ready),  // output wire s_axis_data_TREADY
   .s_axis_data_TDATA(roce_2_drop_data),    // input wire [63 : 0] s_axis_data_TDATA
   .s_axis_data_TKEEP(roce_2_drop_keep),    // input wire [7 : 0] s_axis_data_TKEEP
   .s_axis_data_TLAST(roce_2_drop_last),    // input wire [0 : 0] s_axis_data_TLAST
   .aclk(net_clk),                              // input wire aclk
   .aresetn(aresetn_reg)                        // input wire aresetn
 );
`endif

/*
 * RoCEv2
 */
//assign s_axis_rxread_sts_TREADY = 1'b1;
//assign s_axis_rxwrite_sts_TREADY = 1'b1;


`ifndef IP_VERSION4
//IPv4
assign axis_iph_to_udp_tready = 1'b1;
assign axis_udp_to_merge_tvalid = 1'b0;
assign axis_udp_to_merge_tdata = 0;
assign axis_udp_to_merge_tkeep = 0;
assign axis_udp_to_merge_tlast = 1'b0;
`else
// IPv6
assign axis_iph_to_rocev6_slice.ready = 1'b1;
assign axis_ipv6_to_intercon.valid = 1'b0;
assign axis_ipv6_to_intercon.data = 0;
assign axis_ipv6_to_intercon.keep = 0;
assign axis_ipv6_to_intercon.last = 1'b0;
`endif


roce_stack #(
    .ROCE_EN(0)
) rocev2_stack_inst(
    .net_clk(net_clk), // input aclk
    .net_aresetn(aresetn_reg), // input aresetn
    //RX
`ifdef IP_VERSION4
     //IPv4
    .s_axis_rx_data(axis_udp_slice_to_udp),
`else
    //IPv6
    .s_axis_rx_data(axis_iph_to_rocev6_slice),
`endif
    
   //TX
    .s_axis_tx_meta(axis_tx_metadata),
    .s_axis_tx_data(axis_tx_data),
    
`ifdef IP_VERSION4
    // IPv4
`ifndef ENABLE_DROP 
    .m_axis_tx_data(axis_udp_to_udp_slice),
`else
    .m_axis_tx_data(roce_2_drop),
`endif
`else
    //IPv6
    .m_axis_tx_data(axis_ipv6_to_intercon),
`endif
    //Memory
    .m_axis_mem_write_cmd(m_axis_roce_write_cmd),
    
    .m_axis_mem_read_cmd(m_axis_roce_read_cmd),
    // Memory Write
    .m_axis_mem_write_data(axis_roce_write_data),
    // Memory Read
    .s_axis_mem_read_data(axis_roce_read_data),
    // Memory Write Status
    //.s_axis_mem_write_status_TVALID(s_axis_rxwrite_sts_TVALID),
    //.s_axis_mem_write_status_TREADY(s_axis_rxwrite_sts_TREADY),
    //.s_axis_mem_write_status_TDATA(s_axis_rxwrite_sts_TDATA),
    
    //Pointer chaising
`ifdef POINTER_CHASING
    /*.m_axis_rx_pcmeta_TVALID(m_axis_rx_pcmeta.valid),
    .m_axis_rx_pcmeta_TREADY(m_axis_rx_pcmeta.ready),
    .m_axis_rx_pcmeta_TDATA(m_axis_rx_pcmeta.data),
    .s_axis_tx_pcmeta_TVALID(s_axis_tx_pcmeta.valid),
    .s_axis_tx_pcmeta_TREADY(s_axis_tx_pcmeta.ready),
    .s_axis_tx_pcmeta_TDATA(s_axis_tx_pcmeta.data),*/
`endif
    //CONTROL
    .s_axis_qp_interface(axis_qp_interface),
    .s_axis_qp_conn_interface(axis_qp_conn_interface),
    
    //.local_ip_address_V(link_local_ipv6_address), // Use IPv6 addr
    .local_ip_address(iph_ip_address), //Use IPv4 addr
    .crc_drop_pkg_count_valid(regCrcDropPkgCount_valid),
    .crc_drop_pkg_count_data(regCrcDropPkgCount),
    .psn_drop_pkg_count_valid(regInvalidPsnDropCount_valid),
    .psn_drop_pkg_count_data(regInvalidPsnDropCount)
);


//assign axi_iph_to_toe_slice_tready = 1'b1;
generate
if (WIDTH==64) begin
axis_register_slice_64 axis_register_AXI_S (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_net.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_net.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_net.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_net.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_net.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_slice_to_ibh.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_slice_to_ibh.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_slice_to_ibh.data),    // output wire [63 : 0] m_axis_tdata
  .m_axis_tkeep(axis_slice_to_ibh.keep),    // output wire [7 : 0] m_axis_tkeep
  .m_axis_tlast(axis_slice_to_ibh.last)    // output wire m_axis_tlast
);
end
if (WIDTH==512) begin
axis_register_slice_512 axis_register_AXI_S (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_net.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_net.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_net.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_net.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_net.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_slice_to_ibh.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_slice_to_ibh.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_slice_to_ibh.data),    // output wire [63 : 0] m_axis_tdata
  .m_axis_tkeep(axis_slice_to_ibh.keep),    // output wire [7 : 0] m_axis_tkeep
  .m_axis_tlast(axis_slice_to_ibh.last)    // output wire m_axis_tlast
);
end
endgenerate
 
ip_handler_ip ip_handler_inst (
.m_axis_arp_TVALID(axis_iph_to_arp_slice.valid), // output AXI4Stream_M_TVALID
.m_axis_arp_TREADY(axis_iph_to_arp_slice.ready), // input AXI4Stream_M_TREADY
.m_axis_arp_TDATA(axis_iph_to_arp_slice.data), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_arp_TKEEP(axis_iph_to_arp_slice.keep), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_arp_TLAST(axis_iph_to_arp_slice.last), // output [0 : 0] AXI4Stream_M_TLAST

.m_axis_icmp_TVALID(axis_iph_to_icmp_slice.valid), // output AXI4Stream_M_TVALID
.m_axis_icmp_TREADY(axis_iph_to_icmp_slice.ready), // input AXI4Stream_M_TREADY
.m_axis_icmp_TDATA(axis_iph_to_icmp_slice.data), // output [63 : 0] AXI4Stream_M_TDATA
.m_axis_icmp_TKEEP(axis_iph_to_icmp_slice.keep), // output [7 : 0] AXI4Stream_M_TSTRB
.m_axis_icmp_TLAST(axis_iph_to_icmp_slice.last), // output [0 : 0] AXI4Stream_M_TLAST

.m_axis_icmpv6_TVALID(axis_iph_to_icmpv6_slice.valid),
.m_axis_icmpv6_TREADY(axis_iph_to_icmpv6_slice.ready),
.m_axis_icmpv6_TDATA(axis_iph_to_icmpv6_slice.data),
.m_axis_icmpv6_TKEEP(axis_iph_to_icmpv6_slice.keep),
.m_axis_icmpv6_TLAST(axis_iph_to_icmpv6_slice.last),

.m_axis_ipv6udp_TVALID(axis_iph_to_rocev6_slice.valid),
.m_axis_ipv6udp_TREADY(axis_iph_to_rocev6_slice.ready),
.m_axis_ipv6udp_TDATA(axis_iph_to_rocev6_slice.data), 
.m_axis_ipv6udp_TKEEP(axis_iph_to_rocev6_slice.keep),
.m_axis_ipv6udp_TLAST(axis_iph_to_rocev6_slice.last),

.m_axis_udp_TVALID(axis_iph_to_udp_slice.valid),
.m_axis_udp_TREADY(axis_iph_to_udp_slice.ready),
.m_axis_udp_TDATA(axis_iph_to_udp_slice.data),
.m_axis_udp_TKEEP(axis_iph_to_udp_slice.keep),
.m_axis_udp_TLAST(axis_iph_to_udp_slice.last),

.m_axis_tcp_TVALID(axis_iph_to_toe_slice.valid),
.m_axis_tcp_TREADY(axis_iph_to_toe_slice.ready),
.m_axis_tcp_TDATA(axis_iph_to_toe_slice.data),
.m_axis_tcp_TKEEP(axis_iph_to_toe_slice.keep),
.m_axis_tcp_TLAST(axis_iph_to_toe_slice.last),

.m_axis_roce_TVALID(axis_iph_to_roce_slice.valid),
.m_axis_roce_TREADY(axis_iph_to_roce_slice.ready),
.m_axis_roce_TDATA(axis_iph_to_roce_slice.data),
.m_axis_roce_TKEEP(axis_iph_to_roce_slice.keep),
.m_axis_roce_TLAST(axis_iph_to_roce_slice.last),

.s_axis_raw_TVALID(axis_slice_to_ibh.valid),
.s_axis_raw_TREADY(axis_slice_to_ibh.ready),
.s_axis_raw_TDATA(axis_slice_to_ibh.data),
.s_axis_raw_TKEEP(axis_slice_to_ibh.keep),
.s_axis_raw_TLAST(axis_slice_to_ibh.last),

.myIpAddress_V(iph_ip_address),

.aclk(net_clk), // input aclk
.aresetn(aresetn_reg) // input aresetn
);

// ARP lookup
(* mark_debug = "true" *)wire        axis_arp_lookup_request_TVALID;
(* mark_debug = "true" *)wire        axis_arp_lookup_request_TREADY;
wire[31:0]  axis_arp_lookup_request_TDATA;
(* mark_debug = "true" *)wire        axis_arp_lookup_reply_TVALID;
(* mark_debug = "true" *)wire        axis_arp_lookup_reply_TREADY;
wire[55:0]  axis_arp_lookup_reply_TDATA;

mac_ip_encode_ip mac_ip_encode_inst (
.m_axis_ip_TVALID(axis_mie_to_intercon.valid),
.m_axis_ip_TREADY(axis_mie_to_intercon.ready),
.m_axis_ip_TDATA(axis_mie_to_intercon.data),
.m_axis_ip_TKEEP(axis_mie_to_intercon.keep),
.m_axis_ip_TLAST(axis_mie_to_intercon.last),
.m_axis_arp_lookup_request_TVALID(axis_arp_lookup_request_TVALID),
.m_axis_arp_lookup_request_TREADY(axis_arp_lookup_request_TREADY),
.m_axis_arp_lookup_request_TDATA(axis_arp_lookup_request_TDATA),
.s_axis_ip_TVALID(axis_intercon_to_mie.valid),
.s_axis_ip_TREADY(axis_intercon_to_mie.ready),
.s_axis_ip_TDATA(axis_intercon_to_mie.data),
.s_axis_ip_TKEEP(axis_intercon_to_mie.keep),
.s_axis_ip_TLAST(axis_intercon_to_mie.last),
.s_axis_arp_lookup_reply_TVALID(axis_arp_lookup_reply_TVALID),
.s_axis_arp_lookup_reply_TREADY(axis_arp_lookup_reply_TREADY),
.s_axis_arp_lookup_reply_TDATA(axis_arp_lookup_reply_TDATA),

.myMacAddress_V(mie_mac_address),                                    // input wire [47 : 0] regMacAddress_V
.regSubNetMask_V(ip_subnet_mask),                                    // input wire [31 : 0] regSubNetMask_V
.regDefaultGateway_V(ip_default_gateway),                            // input wire [31 : 0] regDefaultGateway_V
  
.aclk(net_clk), // input aclk
.aresetn(aresetn_reg) // input aresetn
);

`ifdef IP_VERSION4

assign axis_ethencode_to_intercon.valid = 1'b0;
assign axis_ethencode_to_intercon.data = 0;
assign axis_ethencode_to_intercon.keep = 0;
assign axis_ethencode_to_intercon.last = 1'b0;

`else

eth_encode_ip eth_encode_inst (
.m_axis_eth_data_TVALID(axis_ethencode_to_intercon.valid),
.m_axis_eth_data_TREADY(axis_ethencode_to_intercon.ready),
.m_axis_eth_data_TDATA(axis_ethencode_to_intercon.data),
.m_axis_eth_data_TKEEP(axis_ethencode_to_intercon.keep),
.m_axis_eth_data_TLAST(axis_ethencode_to_intercon.last),
.m_axis_ndp_lookup_request_TVALID(axis_ipv6_res_req_TVALID),
.m_axis_ndp_lookup_request_TREADY(axis_ipv6_res_req_TREADY),
.m_axis_ndp_lookup_request_TDATA(axis_ipv6_res_req_TDATA),
.s_axis_ipv6_data_TVALID(axis_ipv6_to_ethen.valid),
.s_axis_ipv6_data_TREADY(axis_ipv6_to_ethen.ready),
.s_axis_ipv6_data_TDATA(axis_ipv6_to_ethen.data),
.s_axis_ipv6_data_TKEEP(axis_ipv6_to_ethen.keep),
.s_axis_ipv6_data_TLAST(axis_ipv6_to_ethen.last),
.s_axis_ndp_lookup_reply_TVALID(axis_ipv6_res_rsp_TVALID),
.s_axis_ndp_lookup_reply_TREADY(axis_ipv6_res_rsp_TREADY),
.s_axis_ndp_lookup_reply_TDATA(axis_ipv6_res_rsp_TDATA),

.localMacAddress_V(mie_mac_address),  
.aclk(net_clk), // input aclk
.aresetn(aresetn_reg) // input aresetn
);

`endif

generate
if (WIDTH==64) begin
// merges icmp and tcp
axis_interconnect_4to1 ip_merger (
  .ACLK(net_clk),                                  // input wire ACLK
  .ARESETN(aresetn_reg),                            // input wire ARESETN
  .S00_AXIS_ACLK(net_clk),                // input wire S00_AXIS_ACLK
  .S01_AXIS_ACLK(net_clk),                // input wire S01_AXIS_ACLK
  .S02_AXIS_ACLK(net_clk),                // input wire S02_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn_reg),          // input wire S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn_reg),          // input wire S01_AXIS_ARESETN
  .S02_AXIS_ARESETN(aresetn_reg),          // input wire S02_AXIS_ARESETN
  
  .S00_AXIS_TVALID(axis_icmp_to_icmp_slice.valid),            // input wire S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_icmp_to_icmp_slice.ready),            // output wire S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_icmp_to_icmp_slice.data),              // input wire [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axis_icmp_to_icmp_slice.keep),              // input wire [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axis_icmp_to_icmp_slice.last),              // input wire S00_AXIS_TLAST

  .S01_AXIS_TVALID(axis_udp_slice_to_merge.valid),            // input wire S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_udp_slice_to_merge.ready),            // output wire S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_udp_slice_to_merge.data),              // input wire [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axis_udp_slice_to_merge.keep),              // input wire [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axis_udp_slice_to_merge.last),              // input wire S01_AXIS_TLAST

  .S02_AXIS_TVALID(axis_toe_to_toe_slice.valid),            // input wire S02_AXIS_TVALID
  .S02_AXIS_TREADY(axis_toe_to_toe_slice.ready),            // output wire S02_AXIS_TREADY
  .S02_AXIS_TDATA(axis_toe_to_toe_slice.data),              // input wire [63 : 0] S02_AXIS_TDATA
  .S02_AXIS_TKEEP(axis_toe_to_toe_slice.keep),              // input wire [7 : 0] S02_AXIS_TKEEP
  .S02_AXIS_TLAST(axis_toe_to_toe_slice.last),              // input wire S02_AXIS_TLAST

  .S03_AXIS_TVALID(axis_roce_slice_to_merge.valid),            // input wire S01_AXIS_TVALID
  .S03_AXIS_TREADY(axis_roce_slice_to_merge.ready),            // output wire S01_AXIS_TREADY
  .S03_AXIS_TDATA(axis_roce_slice_to_merge.data),              // input wire [63 : 0] S01_AXIS_TDATA
  .S03_AXIS_TKEEP(axis_roce_slice_to_merge.keep),              // input wire [7 : 0] S01_AXIS_TKEEP
  .S03_AXIS_TLAST(axis_roce_slice_to_merge.last),              // input wire S01_AXIS_TLAST

  .M00_AXIS_ACLK(net_clk),                // input wire M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn_reg),          // input wire M00_AXIS_ARESETN
  .M00_AXIS_TVALID(axis_intercon_to_mie.valid),            // output wire M00_AXIS_TVALID
  .M00_AXIS_TREADY(axis_intercon_to_mie.ready),            // input wire M00_AXIS_TREADY
  .M00_AXIS_TDATA(axis_intercon_to_mie.data),              // output wire [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(axis_intercon_to_mie.keep),              // output wire [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(axis_intercon_to_mie.last),              // output wire M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0),  // input wire S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0),  // input wire S01_ARB_REQ_SUPPRESS
  .S02_ARB_REQ_SUPPRESS(1'b0)  // input wire S02_ARB_REQ_SUPPRESS
);

// merges ip and arp
axis_interconnect_2to1 mac_merger (
  .ACLK(net_clk), // input ACLK
  .ARESETN(aresetn_reg), // input ARESETN
  .S00_AXIS_ACLK(net_clk), // input S00_AXIS_ACLK
  .S01_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  //.S02_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn_reg), // input S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn_reg), // input S01_AXIS_ARESETN
  //.S02_AXIS_ARESETN(aresetn_reg), // input S01_AXIS_ARESETN  
  .S00_AXIS_TVALID(axis_arp_to_arp_slice.valid), // input S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_arp_to_arp_slice.ready), // output S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_arp_to_arp_slice.data), // input [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axis_arp_to_arp_slice.keep), // input [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axis_arp_to_arp_slice.last), // input S00_AXIS_TLAST
  
  .S01_AXIS_TVALID(axis_mie_to_intercon.valid), // input S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_mie_to_intercon.ready), // output S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_mie_to_intercon.data), // input [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axis_mie_to_intercon.keep), // input [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axis_mie_to_intercon.last), // input S01_AXIS_TLAST
  
  /*.S02_AXIS_TVALID(axis_ethencode_to_intercon.valid), // input S01_AXIS_TVALID
  .S02_AXIS_TREADY(axis_ethencode_to_intercon.ready), // output S01_AXIS_TREADY
  .S02_AXIS_TDATA(axis_ethencode_to_intercon.data), // input [63 : 0] S01_AXIS_TDATA
  .S02_AXIS_TKEEP(axis_ethencode_to_intercon.keep), // input [7 : 0] S01_AXIS_TKEEP
  .S02_AXIS_TLAST(axis_ethencode_to_intercon.last), // input S01_AXIS_TLAST*/
  
  .M00_AXIS_ACLK(net_clk), // input M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn_reg), // input M00_AXIS_ARESETN
  .M00_AXIS_TVALID(m_axis_net.valid), // output M00_AXIS_TVALID
  .M00_AXIS_TREADY(m_axis_net.ready), // input M00_AXIS_TREADY
  .M00_AXIS_TDATA(m_axis_net.data), // output [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(m_axis_net.keep), // output [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(m_axis_net.last), // output M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0), // input S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
  //.S02_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
);
end
if (WIDTH==512) begin
axi_stream #(.WIDTH(512))    axis_icmp_slice_to_merge();
axis_64_to_512_converter icmp_out_data_converter (
  .aclk(net_clk),
  .aresetn(aresetn_reg),
  .s_axis_tvalid(axis_icmp_to_icmp_slice.valid),
  .s_axis_tready(axis_icmp_to_icmp_slice.ready),
  .s_axis_tdata(axis_icmp_to_icmp_slice.data),
  .s_axis_tkeep(axis_icmp_to_icmp_slice.keep),
  .s_axis_tlast(axis_icmp_to_icmp_slice.last),
  .s_axis_tdest(0),
  .m_axis_tvalid(axis_icmp_slice_to_merge.valid),
  .m_axis_tready(axis_icmp_slice_to_merge.ready),
  .m_axis_tdata(axis_icmp_slice_to_merge.data),
  .m_axis_tkeep(axis_icmp_slice_to_merge.keep),
  .m_axis_tlast(axis_icmp_slice_to_merge.last),
  .m_axis_tdest()
);
axis_interconnect_512_4to1 ip_merger (
  .ACLK(net_clk),                                  // input wire ACLK
  .ARESETN(aresetn_reg),                            // input wire ARESETN
  .S00_AXIS_ACLK(net_clk),                // input wire S00_AXIS_ACLK
  .S01_AXIS_ACLK(net_clk),                // input wire S01_AXIS_ACLK
  .S02_AXIS_ACLK(net_clk),                // input wire S02_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn_reg),          // input wire S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn_reg),          // input wire S01_AXIS_ARESETN
  .S02_AXIS_ARESETN(aresetn_reg),          // input wire S02_AXIS_ARESETN
  
  .S00_AXIS_TVALID(axis_icmp_slice_to_merge.valid),            // input wire S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_icmp_slice_to_merge.ready),            // output wire S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_icmp_slice_to_merge.data),              // input wire [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axis_icmp_slice_to_merge.keep),              // input wire [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axis_icmp_slice_to_merge.last),              // input wire S00_AXIS_TLAST

  .S01_AXIS_TVALID(axis_udp_slice_to_merge.valid),            // input wire S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_udp_slice_to_merge.ready),            // output wire S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_udp_slice_to_merge.data),              // input wire [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axis_udp_slice_to_merge.keep),              // input wire [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axis_udp_slice_to_merge.last),              // input wire S01_AXIS_TLAST

  .S02_AXIS_TVALID(axis_toe_to_toe_slice.valid),            // input wire S02_AXIS_TVALID
  .S02_AXIS_TREADY(axis_toe_to_toe_slice.ready),            // output wire S02_AXIS_TREADY
  .S02_AXIS_TDATA(axis_toe_to_toe_slice.data),              // input wire [63 : 0] S02_AXIS_TDATA
  .S02_AXIS_TKEEP(axis_toe_to_toe_slice.keep),              // input wire [7 : 0] S02_AXIS_TKEEP
  .S02_AXIS_TLAST(axis_toe_to_toe_slice.last),              // input wire S02_AXIS_TLAST

  .S03_AXIS_TVALID(axis_roce_slice_to_merge.valid),            // input wire S01_AXIS_TVALID
  .S03_AXIS_TREADY(axis_roce_slice_to_merge.ready),            // output wire S01_AXIS_TREADY
  .S03_AXIS_TDATA(axis_roce_slice_to_merge.data),              // input wire [63 : 0] S01_AXIS_TDATA
  .S03_AXIS_TKEEP(axis_roce_slice_to_merge.keep),              // input wire [7 : 0] S01_AXIS_TKEEP
  .S03_AXIS_TLAST(axis_roce_slice_to_merge.last),              // input wire S01_AXIS_TLAST

  .M00_AXIS_ACLK(net_clk),                // input wire M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn_reg),          // input wire M00_AXIS_ARESETN
  .M00_AXIS_TVALID(axis_intercon_to_mie.valid),            // output wire M00_AXIS_TVALID
  .M00_AXIS_TREADY(axis_intercon_to_mie.ready),            // input wire M00_AXIS_TREADY
  .M00_AXIS_TDATA(axis_intercon_to_mie.data),              // output wire [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(axis_intercon_to_mie.keep),              // output wire [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(axis_intercon_to_mie.last),              // output wire M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0),  // input wire S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0),  // input wire S01_ARB_REQ_SUPPRESS
  .S02_ARB_REQ_SUPPRESS(1'b0)  // input wire S02_ARB_REQ_SUPPRESS
);

// merges ip and arp
axis_interconnect_512_2to1 mac_merger (
  .ACLK(net_clk), // input ACLK
  .ARESETN(aresetn_reg), // input ARESETN
  .S00_AXIS_ACLK(net_clk), // input S00_AXIS_ACLK
  .S01_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  //.S02_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn_reg), // input S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn_reg), // input S01_AXIS_ARESETN
  //.S02_AXIS_ARESETN(aresetn_reg), // input S01_AXIS_ARESETN  
  .S00_AXIS_TVALID(axis_arp_to_arp_slice.valid), // input S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_arp_to_arp_slice.ready), // output S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_arp_to_arp_slice.data), // input [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axis_arp_to_arp_slice.keep), // input [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axis_arp_to_arp_slice.last), // input S00_AXIS_TLAST
  
  .S01_AXIS_TVALID(axis_mie_to_intercon.valid), // input S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_mie_to_intercon.ready), // output S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_mie_to_intercon.data), // input [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axis_mie_to_intercon.keep), // input [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axis_mie_to_intercon.last), // input S01_AXIS_TLAST
  
  /*.S02_AXIS_TVALID(axis_ethencode_to_intercon.valid), // input S01_AXIS_TVALID
  .S02_AXIS_TREADY(axis_ethencode_to_intercon.ready), // output S01_AXIS_TREADY
  .S02_AXIS_TDATA(axis_ethencode_to_intercon.data), // input [63 : 0] S01_AXIS_TDATA
  .S02_AXIS_TKEEP(axis_ethencode_to_intercon.keep), // input [7 : 0] S01_AXIS_TKEEP
  .S02_AXIS_TLAST(axis_ethencode_to_intercon.last), // input S01_AXIS_TLAST*/
  
  .M00_AXIS_ACLK(net_clk), // input M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn_reg), // input M00_AXIS_ARESETN
  .M00_AXIS_TVALID(m_axis_net.valid), // output M00_AXIS_TVALID
  .M00_AXIS_TREADY(m_axis_net.ready), // input M00_AXIS_TREADY
  .M00_AXIS_TDATA(m_axis_net.data), // output [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(m_axis_net.keep), // output [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(m_axis_net.last), // output M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0), // input S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
  //.S02_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
);
end
endgenerate
`ifndef IP_VERSION4

// merges icmpv6 & ipv6
axis_interconnect_2to1 ipv6_merger (
  .ACLK(net_clk), // input ACLK
  .ARESETN(aresetn_reg), // input ARESETN
  .S00_AXIS_ACLK(net_clk), // input S00_AXIS_ACLK
  .S01_AXIS_ACLK(net_clk), // input S01_AXIS_ACLK
  .S00_AXIS_ARESETN(aresetn_reg), // input S00_AXIS_ARESETN
  .S01_AXIS_ARESETN(aresetn_reg), // input S01_AXIS_ARESETN
  
  .S00_AXIS_TVALID(axis_icmpv6_to_intercon.valid), // input S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_icmpv6_to_intercon.ready), // output S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_icmpv6_to_intercon.data), // input [63 : 0] S00_AXIS_TDATA
  .S00_AXIS_TKEEP(axis_icmpv6_to_intercon.keep), // input [7 : 0] S00_AXIS_TKEEP
  .S00_AXIS_TLAST(axis_icmpv6_to_intercon.last), // input S00_AXIS_TLAST
  
  .S01_AXIS_TVALID(axis_ipv6_to_intercon.valid), // input S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_ipv6_to_intercon.ready), // output S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_ipv6_to_intercon.data), // input [63 : 0] S01_AXIS_TDATA
  .S01_AXIS_TKEEP(axis_ipv6_to_intercon.keep), // input [7 : 0] S01_AXIS_TKEEP
  .S01_AXIS_TLAST(axis_ipv6_to_intercon.last), // input S01_AXIS_TLAST
  
  .M00_AXIS_ACLK(net_clk), // input M00_AXIS_ACLK
  .M00_AXIS_ARESETN(aresetn_reg), // input M00_AXIS_ARESETN
  .M00_AXIS_TVALID(axis_ipv6_to_ethen.valid), // output M00_AXIS_TVALID
  .M00_AXIS_TREADY(axis_ipv6_to_ethen.ready), // input M00_AXIS_TREADY
  .M00_AXIS_TDATA(axis_ipv6_to_ethen.data), // output [63 : 0] M00_AXIS_TDATA
  .M00_AXIS_TKEEP(axis_ipv6_to_ethen.keep), // output [7 : 0] M00_AXIS_TKEEP
  .M00_AXIS_TLAST(axis_ipv6_to_ethen.last), // output M00_AXIS_TLAST
  .S00_ARB_REQ_SUPPRESS(1'b0), // input S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0) // input S01_ARB_REQ_SUPPRESS
);
`endif

logic[15:0] arp_request_pkg_counter;
logic[15:0] arp_reply_pkg_counter;

arp_server_subnet_ip arp_server_inst(
.m_axis_TVALID(axis_arp_to_arp_slice.valid),
.m_axis_TREADY(axis_arp_to_arp_slice.ready),
.m_axis_TDATA(axis_arp_to_arp_slice.data),
.m_axis_TKEEP(axis_arp_to_arp_slice.keep),
.m_axis_TLAST(axis_arp_to_arp_slice.last),
.m_axis_arp_lookup_reply_TVALID(axis_arp_lookup_reply_TVALID),
.m_axis_arp_lookup_reply_TREADY(axis_arp_lookup_reply_TREADY),
.m_axis_arp_lookup_reply_TDATA(axis_arp_lookup_reply_TDATA),
.m_axis_host_arp_lookup_reply_TVALID(axis_host_arp_lookup_reply_TVALID),
.m_axis_host_arp_lookup_reply_TREADY(axis_host_arp_lookup_reply_TREADY),
.m_axis_host_arp_lookup_reply_TDATA(axis_host_arp_lookup_reply_TDATA),
.s_axis_TVALID(axis_arp_slice_to_arp.valid),
.s_axis_TREADY(axis_arp_slice_to_arp.ready),
.s_axis_TDATA(axis_arp_slice_to_arp.data),
.s_axis_TKEEP(axis_arp_slice_to_arp.keep),
.s_axis_TLAST(axis_arp_slice_to_arp.last),
.s_axis_arp_lookup_request_TVALID(axis_arp_lookup_request_TVALID),
.s_axis_arp_lookup_request_TREADY(axis_arp_lookup_request_TREADY),
.s_axis_arp_lookup_request_TDATA(axis_arp_lookup_request_TDATA),
.s_axis_host_arp_lookup_request_TVALID(axis_host_arp_lookup_request_TVALID),
.s_axis_host_arp_lookup_request_TREADY(axis_host_arp_lookup_request_TREADY),
.s_axis_host_arp_lookup_request_TDATA(axis_host_arp_lookup_request_TDATA),

.myMacAddress_V(arp_mac_address),
.myIpAddress_V(arp_ip_address),
//.regRequestCount_V(arp_request_pkg_counter),
//.regRequestCount_V_ap_vld(),
//.regReplyCount_V(arp_reply_pkg_counter),
//.regReplyCount_V_ap_vld(),

.aclk(net_clk), // input aclk
.aresetn(aresetn_reg) // input aresetn
);

/*assign axis_ttl_to_icmp_tvalid = 0;
assign axis_ttl_to_icmp_tdata = 0;
assign axis_ttl_to_icmp_tkeep = 0;
assign axis_ttl_to_icmp_tlast = 0;*/

icmp_server_ip icmp_server_inst (
  .s_axis_TVALID(axis_icmp_slice_to_icmp.valid),    // input wire dataIn_TVALID
  .s_axis_TREADY(axis_icmp_slice_to_icmp.ready),    // output wire dataIn_TREADY
  .s_axis_TDATA(axis_icmp_slice_to_icmp.data),      // input wire [63 : 0] dataIn_TDATA
  .s_axis_TKEEP(axis_icmp_slice_to_icmp.keep),      // input wire [7 : 0] dataIn_TKEEP
  .s_axis_TLAST(axis_icmp_slice_to_icmp.last),      // input wire [0 : 0] dataIn_TLAST
  .udpIn_TVALID(1'b0),//(axis_udp_to_icmp_tvalid),           // input wire udpIn_TVALID
  .udpIn_TREADY(),           // output wire udpIn_TREADY
  .udpIn_TDATA(0),//(axis_udp_to_icmp_tdata),             // input wire [63 : 0] udpIn_TDATA
  .udpIn_TKEEP(0),//(axis_udp_to_icmp_tkeep),             // input wire [7 : 0] udpIn_TKEEP
  .udpIn_TLAST(0),//(axis_udp_to_icmp_tlast),             // input wire [0 : 0] udpIn_TLAST
  .ttlIn_TVALID(1'b0),//(axis_ttl_to_icmp_tvalid),           // input wire ttlIn_TVALID
  .ttlIn_TREADY(),           // output wire ttlIn_TREADY
  .ttlIn_TDATA(0),//(axis_ttl_to_icmp_tdata),             // input wire [63 : 0] ttlIn_TDATA
  .ttlIn_TKEEP(0),//(axis_ttl_to_icmp_tkeep),             // input wire [7 : 0] ttlIn_TKEEP
  .ttlIn_TLAST(0),//(axis_ttl_to_icmp_tlast),             // input wire [0 : 0] ttlIn_TLAST
  .m_axis_TVALID(axis_icmp_to_icmp_slice.valid),   // output wire dataOut_TVALID
  .m_axis_TREADY(axis_icmp_to_icmp_slice.ready),   // input wire dataOut_TREADY
  .m_axis_TDATA(axis_icmp_to_icmp_slice.data),     // output wire [63 : 0] dataOut_TDATA
  .m_axis_TKEEP(axis_icmp_to_icmp_slice.keep),     // output wire [7 : 0] dataOut_TKEEP
  .m_axis_TLAST(axis_icmp_to_icmp_slice.last),     // output wire [0 : 0] dataOut_TLAST
  .aclk(net_clk),                                    // input wire ap_clk
  .aresetn(aresetn_reg)                                // input wire ap_rst_n
);

// IPv6 to ICMPv6
axis_meta #(.WIDTH(152))      axis_ipv6_to_icmpv6_meta();
axi_stream #(.WIDTH(WIDTH))   axis_ipv6_to_icmpv6_data();

axis_meta #(.WIDTH(152))      axis_icmpv6_to_ipv6_meta();
axi_stream #(.WIDTH(WIDTH))   axis_icmpv6_to_ipv6_data();

`ifdef IP_VERSION4

assign axis_iph_to_icmpv6_slice.ready = 1'b1;

`else
ipv6_ip ipv6_inst(

.m_axis_rx_meta_TVALID(axi_ipv6_to_icmpv6_meta.valid,  // output wire m_axis_rx_meta_TVALID
.m_axis_rx_meta_TREADY(axi_ipv6_to_icmpv6_meta.ready,  // input wire m_axis_rx_meta_TREADY
.m_axis_rx_meta_TDATA(axi_ipv6_to_icmpv6_meta.data,    // output wire [151 : 0] m_axis_rx_meta_TDATA


.m_axis_rx_data_TVALID(axi_ipv6_to_icmpv6_data.valid,  // output wire m_axis_rx_data_TVALID
.m_axis_rx_data_TREADY(axi_ipv6_to_icmpv6_data.ready,  // input wire m_axis_rx_data_TREADY
.m_axis_rx_data_TDATA(axi_ipv6_to_icmpv6_data.data,    // output wire [63 : 0] m_axis_rx_data_TDATA
.m_axis_rx_data_TKEEP(axi_ipv6_to_icmpv6_data.keep,    // output wire [7 : 0] m_axis_rx_data_TKEEP
.m_axis_rx_data_TLAST(axi_ipv6_to_icmpv6_data.last,    // output wire [0 : 0] m_axis_rx_data_TLAST

.s_axis_rx_data_TVALID(axi_iph_to_icmpv6_slice.valid,
.s_axis_rx_data_TREADY(axi_iph_to_icmpv6_slice.ready,
.s_axis_rx_data_TDATA(axi_iph_to_icmpv6_slice.data,
.s_axis_rx_data_TKEEP(axi_iph_to_icmpv6_slice.keep,
.s_axis_rx_data_TLAST(axi_iph_to_icmpv6_slice.last,


.m_axis_tx_data_TVALID(axi_icmpv6_to_intercon.valid,  // output wire m_axis_tx_data_TVALID
.m_axis_tx_data_TREADY(axi_icmpv6_to_intercon.ready,  // input wire m_axis_tx_data_TREADY
.m_axis_tx_data_TDATA(axi_icmpv6_to_intercon.data,    // output wire [63 : 0] m_axis_tx_data_TDATA
.m_axis_tx_data_TKEEP(axi_icmpv6_to_intercon.keep,    // output wire [7 : 0] m_axis_tx_data_TKEEP
.m_axis_tx_data_TLAST(axi_icmpv6_to_intercon.last, 

.s_axis_tx_meta_TVALID(axi_icmpv6_to_ipv6_meta.valid,
.s_axis_tx_meta_TREADY(axi_icmpv6_to_ipv6_meta.ready,
.s_axis_tx_meta_TDATA(axi_icmpv6_to_ipv6_meta.data,
.s_axis_tx_data_TVALID(axi_icmpv6_to_ipv6_data.valid,        // input wire s_axis_data_TVALID
.s_axis_tx_data_TREADY(axi_icmpv6_to_ipv6_data.ready,        // output wire s_axis_data_TREADY
.s_axis_tx_data_TDATA(axi_icmpv6_to_ipv6_data.data,          // input wire [63 : 0] s_axis_data_TDATA
.s_axis_tx_data_TKEEP(axi_icmpv6_to_ipv6_data.keep,          // input wire [7 : 0] s_axis_data_TKEEP
.s_axis_tx_data_TLAST(axi_icmpv6_to_ipv6_data.last,          // input wire [0 : 0] s_axis_data_TLAST
 
.reg_ip_address_V(link_local_ipv6_address),
 
.aclk(net_clk),                                    // input wire aclk
.aresetn(aresetn_reg)                              // input wire aresetn
);

icmpv6_server_ip icmpv6_server_inst (
  .m_axis_data_TVALID(axi_icmpv6_to_ipv6_data.valid,        // output wire m_axis_data_TVALID
  .m_axis_data_TREADY(axi_icmpv6_to_ipv6_data.ready,        // input wire m_axis_data_TREADY
  .m_axis_data_TDATA(axi_icmpv6_to_ipv6_data.data,          // output wire [63 : 0] m_axis_data_TDATA
  .m_axis_data_TKEEP(axi_icmpv6_to_ipv6_data.keep,          // output wire [7 : 0] m_axis_data_TKEEP
  .m_axis_data_TLAST(axi_icmpv6_to_ipv6_data.last,          // output wire [0 : 0] m_axis_data_TLAST
  .m_axis_meta_TVALID(axi_icmpv6_to_ipv6_meta.valid,        // output wire m_axis_meta_TVALID
  .m_axis_meta_TREADY(axi_icmpv6_to_ipv6_meta.ready,        // input wire m_axis_meta_TREADY
  .m_axis_meta_TDATA(axi_icmpv6_to_ipv6_meta.data,          // output wire [151 : 0] m_axis_meta_TDATA
  
  .m_ipv6_res_rsp_TVALID(axis_ipv6_res_rsp_TVALID),  // output wire m_ipv6_res_rsp_TVALID
  .m_ipv6_res_rsp_TREADY(axis_ipv6_res_rsp_TREADY),  // input wire m_ipv6_res_rsp_TREADY
  .m_ipv6_res_rsp_TDATA(axis_ipv6_res_rsp_TDATA),    // output wire [55 : 0] m_ipv6_res_rsp_TDATA
  
  .s_axis_data_TVALID(axi_ipv6_to_icmpv6_data.valid,        // input wire s_axis_data_TVALID
  .s_axis_data_TREADY(axi_ipv6_to_icmpv6_data.ready,        // output wire s_axis_data_TREADY
  .s_axis_data_TDATA(axi_ipv6_to_icmpv6_data.data,          // input wire [63 : 0] s_axis_data_TDATA
  .s_axis_data_TKEEP(axi_ipv6_to_icmpv6_data.keep,          // input wire [7 : 0] s_axis_data_TKEEP
  .s_axis_data_TLAST(axi_ipv6_to_icmpv6_data.last,          // input wire [0 : 0] s_axis_data_TLAST
  .s_axis_meta_TVALID(axi_ipv6_to_icmpv6_meta.valid,        // input wire s_axis_meta_TVALID
  .s_axis_meta_TREADY(axi_ipv6_to_icmpv6_meta.ready,        // output wire s_axis_meta_TREADY
  .s_axis_meta_TDATA(axi_ipv6_to_icmpv6_meta.data,          // input wire [151 : 0] s_axis_meta_TDATA
  
  .s_ipv6_res_req_TVALID(axis_ipv6_res_req_TVALID),  // input wire s_ipv6_res_req_TVALID
  .s_ipv6_res_req_TREADY(axis_ipv6_res_req_TREADY),  // output wire s_ipv6_res_req_TREADY
  .s_ipv6_res_req_TDATA(axis_ipv6_res_req_TDATA),    // input wire [127 : 0] s_ipv6_res_req_TDATA
  
  .local_mac_address_V(ipv6_mac_address),      // input wire [47 : 0] local_mac_address_V
  .local_ipv6_address_V(link_local_ipv6_address),    // input wire [127 : 0] local_ipv6_address_V
   
  .aclk(net_clk),                                    // input wire aclk
  .aresetn(aresetn_reg)                              // input wire aresetn
);
`endif

/*
 * Slices
 */
 // ARP Input Slice
generate
if (WIDTH==64) begin
axis_register_slice_64 axis_register_arp_in_slice(
 .aclk(net_clk),
 .aresetn(aresetn_reg),
 .s_axis_tvalid(axis_iph_to_arp_slice.valid),
 .s_axis_tready(axis_iph_to_arp_slice.ready),
 .s_axis_tdata(axis_iph_to_arp_slice.data),
 .s_axis_tkeep(axis_iph_to_arp_slice.keep),
 .s_axis_tlast(axis_iph_to_arp_slice.last),
 .m_axis_tvalid(axis_arp_slice_to_arp.valid),
 .m_axis_tready(axis_arp_slice_to_arp.ready),
 .m_axis_tdata(axis_arp_slice_to_arp.data),
 .m_axis_tkeep(axis_arp_slice_to_arp.keep),
 .m_axis_tlast(axis_arp_slice_to_arp.last)
);
 // ICMP Input Slice
axis_register_slice_64 axis_register_icmp_in_slice(
  .aclk(net_clk),
  .aresetn(aresetn_reg),
  .s_axis_tvalid(axis_iph_to_icmp_slice.valid),
  .s_axis_tready(axis_iph_to_icmp_slice.ready),
  .s_axis_tdata(axis_iph_to_icmp_slice.data),
  .s_axis_tkeep(axis_iph_to_icmp_slice.keep),
  .s_axis_tlast(axis_iph_to_icmp_slice.last),
  .m_axis_tvalid(axis_icmp_slice_to_icmp.valid),
  .m_axis_tready(axis_icmp_slice_to_icmp.ready),
  .m_axis_tdata(axis_icmp_slice_to_icmp.data),
  .m_axis_tkeep(axis_icmp_slice_to_icmp.keep),
  .m_axis_tlast(axis_icmp_slice_to_icmp.last)
);
 // UDP Input Slice
axis_register_slice_64 axis_register_upd_in_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_iph_to_udp_slice.valid),
.s_axis_tready(axis_iph_to_udp_slice.ready),
.s_axis_tdata(axis_iph_to_udp_slice.data),
.s_axis_tkeep(axis_iph_to_udp_slice.keep),
.s_axis_tlast(axis_iph_to_udp_slice.last),
.m_axis_tvalid(axis_udp_slice_to_udp.valid),
.m_axis_tready(axis_udp_slice_to_udp.ready),
.m_axis_tdata(axis_udp_slice_to_udp.data),
.m_axis_tkeep(axis_udp_slice_to_udp.keep),
.m_axis_tlast(axis_udp_slice_to_udp.last)
);
 // UDP Output Slice
axis_register_slice_64 axis_register_upd_out_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_udp_to_udp_slice.valid),
.s_axis_tready(axis_udp_to_udp_slice.ready),
.s_axis_tdata(axis_udp_to_udp_slice.data),
.s_axis_tkeep(axis_udp_to_udp_slice.keep),
.s_axis_tlast(axis_udp_to_udp_slice.last),
.m_axis_tvalid(axis_udp_slice_to_merge.valid),
.m_axis_tready(axis_udp_slice_to_merge.ready),
.m_axis_tdata(axis_udp_slice_to_merge.data),
.m_axis_tkeep(axis_udp_slice_to_merge.keep),
.m_axis_tlast(axis_udp_slice_to_merge.last)
);
 // TOE Input Slice
axis_register_slice_64 axis_register_toe_in_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_iph_to_toe_slice.valid),
.s_axis_tready(axis_iph_to_toe_slice.ready),
.s_axis_tdata(axis_iph_to_toe_slice.data),
.s_axis_tkeep(axis_iph_to_toe_slice.keep),
.s_axis_tlast(axis_iph_to_toe_slice.last),
.m_axis_tvalid(axis_toe_slice_to_toe.valid),
.m_axis_tready(axis_toe_slice_to_toe.ready),
.m_axis_tdata(axis_toe_slice_to_toe.data),
.m_axis_tkeep(axis_toe_slice_to_toe.keep),
.m_axis_tlast(axis_toe_slice_to_toe.last)
);
 // ROCE Input Slice
axis_register_slice_64 axis_register_roce_in_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_iph_to_roce_slice.valid),
.s_axis_tready(axis_iph_to_roce_slice.ready),
.s_axis_tdata(axis_iph_to_roce_slice.data),
.s_axis_tkeep(axis_iph_to_roce_slice.keep),
.s_axis_tlast(axis_iph_to_roce_slice.last),
.m_axis_tvalid(axis_roce_slice_to_roce.valid),
.m_axis_tready(axis_roce_slice_to_roce.ready),
.m_axis_tdata(axis_roce_slice_to_roce.data),
.m_axis_tkeep(axis_roce_slice_to_roce.keep),
.m_axis_tlast(axis_roce_slice_to_roce.last)
);
// ROCE Output Slice
axis_register_slice_64 axis_register_roce_out_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_roce_to_roce_slice.valid),
.s_axis_tready(axis_roce_to_roce_slice.ready),
.s_axis_tdata(axis_roce_to_roce_slice.data),
.s_axis_tkeep(axis_roce_to_roce_slice.keep),
.s_axis_tlast(axis_roce_to_roce_slice.last),
.m_axis_tvalid(axis_roce_slice_to_merge.valid),
.m_axis_tready(axis_roce_slice_to_merge.ready),
.m_axis_tdata(axis_roce_slice_to_merge.data),
.m_axis_tkeep(axis_roce_slice_to_merge.keep),
.m_axis_tlast(axis_roce_slice_to_merge.last)
);
end
if (WIDTH==512) begin
axis_register_slice_512 axis_register_arp_in_slice(
 .aclk(net_clk),
 .aresetn(aresetn_reg),
 .s_axis_tvalid(axis_iph_to_arp_slice.valid),
 .s_axis_tready(axis_iph_to_arp_slice.ready),
 .s_axis_tdata(axis_iph_to_arp_slice.data),
 .s_axis_tkeep(axis_iph_to_arp_slice.keep),
 .s_axis_tlast(axis_iph_to_arp_slice.last),
 .m_axis_tvalid(axis_arp_slice_to_arp.valid),
 .m_axis_tready(axis_arp_slice_to_arp.ready),
 .m_axis_tdata(axis_arp_slice_to_arp.data),
 .m_axis_tkeep(axis_arp_slice_to_arp.keep),
 .m_axis_tlast(axis_arp_slice_to_arp.last)
);
 // ICMP Input Slice
//axis_register_slice_512 axis_register_icmp_in_slice(
axis_512_to_64_converter icmp_in_data_converter (
  .aclk(net_clk),
  .aresetn(aresetn_reg),
  .s_axis_tvalid(axis_iph_to_icmp_slice.valid),
  .s_axis_tready(axis_iph_to_icmp_slice.ready),
  .s_axis_tdata(axis_iph_to_icmp_slice.data),
  .s_axis_tkeep(axis_iph_to_icmp_slice.keep),
  .s_axis_tlast(axis_iph_to_icmp_slice.last),
  .m_axis_tvalid(axis_icmp_slice_to_icmp.valid),
  .m_axis_tready(axis_icmp_slice_to_icmp.ready),
  .m_axis_tdata(axis_icmp_slice_to_icmp.data),
  .m_axis_tkeep(axis_icmp_slice_to_icmp.keep),
  .m_axis_tlast(axis_icmp_slice_to_icmp.last)
);
 // UDP Input Slice
axis_register_slice_512 axis_register_upd_in_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_iph_to_udp_slice.valid),
.s_axis_tready(axis_iph_to_udp_slice.ready),
.s_axis_tdata(axis_iph_to_udp_slice.data),
.s_axis_tkeep(axis_iph_to_udp_slice.keep),
.s_axis_tlast(axis_iph_to_udp_slice.last),
.m_axis_tvalid(axis_udp_slice_to_udp.valid),
.m_axis_tready(axis_udp_slice_to_udp.ready),
.m_axis_tdata(axis_udp_slice_to_udp.data),
.m_axis_tkeep(axis_udp_slice_to_udp.keep),
.m_axis_tlast(axis_udp_slice_to_udp.last)
);
 // UDP Output Slice
axis_register_slice_512 axis_register_upd_out_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_udp_to_udp_slice.valid),
.s_axis_tready(axis_udp_to_udp_slice.ready),
.s_axis_tdata(axis_udp_to_udp_slice.data),
.s_axis_tkeep(axis_udp_to_udp_slice.keep),
.s_axis_tlast(axis_udp_to_udp_slice.last),
.m_axis_tvalid(axis_udp_slice_to_merge.valid),
.m_axis_tready(axis_udp_slice_to_merge.ready),
.m_axis_tdata(axis_udp_slice_to_merge.data),
.m_axis_tkeep(axis_udp_slice_to_merge.keep),
.m_axis_tlast(axis_udp_slice_to_merge.last)
);
 // TOE Input Slice
axis_register_slice_512 axis_register_toe_in_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_iph_to_toe_slice.valid),
.s_axis_tready(axis_iph_to_toe_slice.ready),
.s_axis_tdata(axis_iph_to_toe_slice.data),
.s_axis_tkeep(axis_iph_to_toe_slice.keep),
.s_axis_tlast(axis_iph_to_toe_slice.last),
.m_axis_tvalid(axis_toe_slice_to_toe.valid),
.m_axis_tready(axis_toe_slice_to_toe.ready),
.m_axis_tdata(axis_toe_slice_to_toe.data),
.m_axis_tkeep(axis_toe_slice_to_toe.keep),
.m_axis_tlast(axis_toe_slice_to_toe.last)
);
// ROCE Input Slice
axis_register_slice_512 axis_register_roce_in_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_iph_to_roce_slice.valid),
.s_axis_tready(axis_iph_to_roce_slice.ready),
.s_axis_tdata(axis_iph_to_roce_slice.data),
.s_axis_tkeep(axis_iph_to_roce_slice.keep),
.s_axis_tlast(axis_iph_to_roce_slice.last),
.m_axis_tvalid(axis_roce_slice_to_roce.valid),
.m_axis_tready(axis_roce_slice_to_roce.ready),
.m_axis_tdata(axis_roce_slice_to_roce.data),
.m_axis_tkeep(axis_roce_slice_to_roce.keep),
.m_axis_tlast(axis_roce_slice_to_roce.last)
);
// ROCE Output Slice
axis_register_slice_512 axis_register_roce_out_slice(
.aclk(net_clk),
.aresetn(aresetn_reg),
.s_axis_tvalid(axis_roce_to_roce_slice.valid),
.s_axis_tready(axis_roce_to_roce_slice.ready),
.s_axis_tdata(axis_roce_to_roce_slice.data),
.s_axis_tkeep(axis_roce_to_roce_slice.keep),
.s_axis_tlast(axis_roce_to_roce_slice.last),
.m_axis_tvalid(axis_roce_slice_to_merge.valid),
.m_axis_tready(axis_roce_slice_to_merge.ready),
.m_axis_tdata(axis_roce_slice_to_merge.data),
.m_axis_tkeep(axis_roce_slice_to_merge.keep),
.m_axis_tlast(axis_roce_slice_to_merge.last)
);
end
endgenerate


/*
 * Network Controller
 */
 axis_meta #(.WIDTH(144))  axis_qp_interface();
 axis_meta #(.WIDTH(184))  axis_qp_conn_interface();
 
 wire        axis_host_arp_lookup_request_TVALID;
 wire        axis_host_arp_lookup_request_TREADY;
 wire[31:0]  axis_host_arp_lookup_request_TDATA;
 wire        axis_host_arp_lookup_reply_TVALID;
 wire        axis_host_arp_lookup_reply_TREADY;
 wire[55:0]  axis_host_arp_lookup_reply_TDATA;
 
wire[31:0]    regCrcDropPkgCount;
wire          regCrcDropPkgCount_valid;
 
wire[31:0]    regInvalidPsnDropCount;
wire          regInvalidPsnDropCount_valid;

// tx metadata
axis_meta #(.WIDTH(160))    axis_tx_metadata();
axis_meta #(.WIDTH(160))    axis_host_tx_metadata();

 
network_controller controller_inst(
    .pcie_clk(pcie_clk),
    .pcie_aresetn(pcie_aresetn),
    .net_clk(net_clk),
    .net_aresetn(net_aresetn),
    
     // AXI Lite Master Interface connections
    .s_axil         (s_axil),
    .s_axim         (s_axim),
    
    // Control streams
    // Control streams
    .m_axis_qp_interface_valid         (axis_qp_interface.valid),
    .m_axis_qp_interface_ready         (axis_qp_interface.ready),
    .m_axis_qp_interface_data          (axis_qp_interface.data),
    .m_axis_qp_conn_interface_valid    (axis_qp_conn_interface.valid),
    .m_axis_qp_conn_interface_ready    (axis_qp_conn_interface.ready),
    .m_axis_qp_conn_interface_data     (axis_qp_conn_interface.data),

    .m_axis_tx_meta_valid              (axis_host_tx_metadata.valid),
    .m_axis_tx_meta_ready              (axis_host_tx_metadata.ready),
    .m_axis_tx_meta_data               (axis_host_tx_metadata.data),


    //Host ARP lookup
    .m_axis_host_arp_lookup_request_TVALID(axis_host_arp_lookup_request_TVALID),
    .m_axis_host_arp_lookup_request_TREADY(axis_host_arp_lookup_request_TREADY),
    .m_axis_host_arp_lookup_request_TDATA(axis_host_arp_lookup_request_TDATA),
    .s_axis_host_arp_lookup_reply_TVALID(axis_host_arp_lookup_reply_TVALID),
    .s_axis_host_arp_lookup_reply_TREADY(axis_host_arp_lookup_reply_TREADY),
    .s_axis_host_arp_lookup_reply_TDATA(axis_host_arp_lookup_reply_TDATA),
`ifdef POINTER_CHAISING
    .m_axis_pc_meta_valid(axis_pcie_tx_pc_meta_tvalid),
    .m_axis_pc_meta_ready(axis_pcie_tx_pc_meta_tready),
    .m_axis_pc_meta_data(axis_pcie_tx_pc_meta_tdata),
`endif
    //general
    .roce_crc_pkg_drop_count            (regCrcDropPkgCount),
    .roce_psn_pkg_drop_count            (regInvalidPsnDropCount),
    .rx_word_counter                    (rx_word_counter),
    .rx_pkg_counter                     (rx_pkg_counter),
    .tx_word_counter                    (tx_word_counter),
    .tx_pkg_counter                     (tx_pkg_counter),

    //arp
    .arp_rx_pkg_counter                 (arp_rx_pkg_counter),
    .arp_tx_pkg_counter                 (arp_tx_pkg_counter),
    .arp_request_pkg_counter            (arp_request_pkg_counter),
    .arp_reply_pkg_counter              (arp_reply_pkg_counter),
    //tcp
    .tcp_rx_pkg_counter                 (tcp_rx_pkg_counter),
    .tcp_tx_pkg_counter                 (tcp_tx_pkg_counter),
    //roce
    .roce_rx_pkg_counter                (roce_rx_pkg_counter),
    .roce_tx_pkg_counter                (roce_tx_pkg_counter),
    //roce data
    .roce_data_rx_word_counter          (roce_data_rx_word_counter),
    .roce_data_rx_pkg_counter           (roce_data_rx_pkg_counter),
    .roce_data_tx_role_word_counter     (roce_data_tx_role_word_counter),
    .roce_data_tx_role_pkg_counter      (roce_data_tx_role_pkg_counter),
    .roce_data_tx_host_word_counter     (roce_data_tx_host_word_counter),
    .roce_data_tx_host_pkg_counter      (roce_data_tx_host_pkg_counter),


    .axis_stream_down                   (axis_stream_down),

    .set_ip_addr_valid(set_ip_addr_valid),
    .set_ip_addr_data(set_ip_addr_data),
    .set_board_number_valid(set_board_number_valid),
    .set_board_number_data(set_board_number_data)

);


wire set_ip_addr_valid;
wire [31:0] set_ip_addr_data;
reg[31:0] local_ip_address;
wire[31:0]ip_address_used;

wire set_board_number_valid;
wire[3:0] set_board_number_data;
reg[3:0] board_number;

always @(posedge net_clk) begin
    if (~net_aresetn) begin
        local_ip_address <= 32'hD1D4010B;
        board_number <= 0;
    end
    else begin
        if (set_ip_addr_valid) begin
            local_ip_address[7:0] <= set_ip_addr_data[31:24];
            local_ip_address[15:8] <= set_ip_addr_data[23:16];
            local_ip_address[23:16] <= set_ip_addr_data[15:8];
            local_ip_address[31:24] <= set_ip_addr_data[7:0];
        end
        if (set_board_number_valid) begin
            board_number <= set_board_number_data;
        end
    end
end

axis_interconnect_merger_160 tx_metadata_merger (
  .ACLK(net_clk),                                  // input wire ACLK
  .ARESETN(net_aresetn),                            // input wire ARESETN
  .S00_AXIS_ACLK(net_clk),                // input wire S00_AXIS_ACLK
  .S00_AXIS_ARESETN(net_aresetn),          // input wire S00_AXIS_ARESETN
  .S00_AXIS_TVALID(axis_host_tx_metadata.valid),            // input wire S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_host_tx_metadata.ready),            // output wire S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_host_tx_metadata.data),              // input wire [159 : 0] S00_AXIS_TDATA
  .S01_AXIS_ACLK(net_clk),                // input wire S01_AXIS_ACLK
  .S01_AXIS_ARESETN(net_aresetn),          // input wire S01_AXIS_ARESETN
  .S01_AXIS_TVALID(s_axis_roce_role_tx_meta.valid),            // input wire S01_AXIS_TVALID
  .S01_AXIS_TREADY(s_axis_roce_role_tx_meta.ready),            // output wire S01_AXIS_TREADY
  .S01_AXIS_TDATA(s_axis_roce_role_tx_meta.data),              // input wire [159 : 0] S01_AXIS_TDATA
  .M00_AXIS_ACLK(net_clk),                // input wire M00_AXIS_ACLK
  .M00_AXIS_ARESETN(net_aresetn),          // input wire M00_AXIS_ARESETN
  .M00_AXIS_TVALID(axis_tx_metadata.valid),            // output wire M00_AXIS_TVALID
  .M00_AXIS_TREADY(axis_tx_metadata.ready),            // input wire M00_AXIS_TREADY
  .M00_AXIS_TDATA(axis_tx_metadata.data),              // output wire [159 : 0] M00_AXIS_TDATA
  .S00_ARB_REQ_SUPPRESS(1'b0),  // input wire S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(1'b0)  // input wire S01_ARB_REQ_SUPPRESS
);


/*
 * Width alignment
 */
axi_stream #(.WIDTH(WIDTH) )   axis_rxwrite_data();
axi_stream #(.WIDTH(WIDTH) )   axis_rxread_data();
axi_stream #(.WIDTH(WIDTH) )   axis_txwrite_data();
axi_stream #(.WIDTH(WIDTH) )   axis_txread_data();
axi_stream #(.WIDTH(WIDTH) )   axis_roce_read_data();
axi_stream #(.WIDTH(WIDTH) )   axis_roce_write_data();
axi_stream #(.WIDTH(WIDTH) )  axis_tx_data();
generate
if (WIDTH==64) begin
//TCP Data Path
`ifndef RX_DDR_BYPASS
axis_512_to_64_converter tcp_rxread_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_read_data[ddrPortNetworkRx].valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_read_data[ddrPortNetworkRx].ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_read_data[ddrPortNetworkRx].data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_read_data[ddrPortNetworkRx].keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_read_data[ddrPortNetworkRx].last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_rxread_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_rxread_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_rxread_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(axis_rxread_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(axis_rxread_data.last)    // output wire m_axis_tlast
);

axis_64_to_512_converter tcp_rxwrite_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(axis_rxwrite_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(axis_rxwrite_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(axis_rxwrite_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axis_rxwrite_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axis_rxwrite_data.last),    // input wire s_axis_tlast
  .s_axis_tdest(1'b0),    // input wire s_axis_tlast
  .m_axis_tvalid(m_axis_write_data[ddrPortNetworkRx].valid),  // output wire m_axis_tvalid
  .m_axis_tready(m_axis_write_data[ddrPortNetworkRx].ready),  // input wire m_axis_tready
  .m_axis_tdata(m_axis_write_data[ddrPortNetworkRx].data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(m_axis_write_data[ddrPortNetworkRx].keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(m_axis_write_data[ddrPortNetworkRx].last),    // output wire m_axis_tlast
  .m_axis_tdest()    // output wire m_axis_tlast
);
`endif
axis_512_to_64_converter tcp_txread_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_read_data[ddrPortNetworkTx].valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_read_data[ddrPortNetworkTx].ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_read_data[ddrPortNetworkTx].data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_read_data[ddrPortNetworkTx].keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_read_data[ddrPortNetworkTx].last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_txread_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_txread_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_txread_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(axis_txread_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(axis_txread_data.last)    // output wire m_axis_tlast
);

axis_64_to_512_converter tcp_txwrite_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(axis_txwrite_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(axis_txwrite_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(axis_txwrite_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axis_txwrite_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axis_txwrite_data.last),    // input wire s_axis_tlast
  .s_axis_tdest(1'b0),    // input wire s_axis_tlast
  .m_axis_tvalid(m_axis_write_data[ddrPortNetworkTx].valid),  // output wire m_axis_tvalid
  .m_axis_tready(m_axis_write_data[ddrPortNetworkTx].ready),  // input wire m_axis_tready
  .m_axis_tdata(m_axis_write_data[ddrPortNetworkTx].data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(m_axis_write_data[ddrPortNetworkTx].keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(m_axis_write_data[ddrPortNetworkTx].last),    // output wire m_axis_tlast
  .m_axis_tdest()    // output wire m_axis_tlast
);

//RoCE Data Path
axis_512_to_64_converter roce_read_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_roce_read_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_roce_read_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_roce_read_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_roce_read_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_roce_read_data.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_roce_read_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_roce_read_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_roce_read_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(axis_roce_read_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(axis_roce_read_data.last)    // output wire m_axis_tlast
);

axis_512_to_64_converter roce_tx_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(s_axis_roce_role_tx_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(s_axis_roce_role_tx_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(s_axis_roce_role_tx_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(s_axis_roce_role_tx_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(s_axis_roce_role_tx_data.last),    // input wire s_axis_tlast
  .m_axis_tvalid(axis_tx_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(axis_tx_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(axis_tx_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(axis_tx_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(axis_tx_data.last)    // output wire m_axis_tlast
);

axis_64_to_512_converter roce_write_data_converter (
  .aclk(net_clk),                    // input wire aclk
  .aresetn(net_aresetn),              // input wire aresetn
  .s_axis_tvalid(axis_roce_write_data.valid),  // input wire s_axis_tvalid
  .s_axis_tready(axis_roce_write_data.ready),  // output wire s_axis_tready
  .s_axis_tdata(axis_roce_write_data.data),    // input wire [63 : 0] s_axis_tdata
  .s_axis_tkeep(axis_roce_write_data.keep),    // input wire [7 : 0] s_axis_tkeep
  .s_axis_tlast(axis_roce_write_data.last),    // input wire s_axis_tlast
  .s_axis_tdest(axis_roce_write_data.dest),    // input wire s_axis_tlast
  .m_axis_tvalid(m_axis_roce_write_data.valid),  // output wire m_axis_tvalid
  .m_axis_tready(m_axis_roce_write_data.ready),  // input wire m_axis_tready
  .m_axis_tdata(m_axis_roce_write_data.data),    // output wire [511 : 0] m_axis_tdata
  .m_axis_tkeep(m_axis_roce_write_data.keep),    // output wire [63 : 0] m_axis_tkeep
  .m_axis_tlast(m_axis_roce_write_data.last),    // output wire m_axis_tlast
  .m_axis_tdest(m_axis_roce_write_data.dest)    // output wire m_axis_tlast
);
end
if (WIDTH==512) begin
//TCP Data Path
assign axis_rxread_data.valid = s_axis_txread_data.valid;
assign s_axis_rxread_data.ready = axis_txread_data.ready;
assign axis_rxread_data.data = s_axis_txread_data.data;
assign axis_rxread_data.keep = s_axis_txread_data.keep;
assign axis_rxread_data.last = s_axis_txread_data.last;

assign m_axis_rxwrite_data.valid = axis_txwrite_data.valid;
assign axis_rxwrite_data.ready = m_axis_txwrite_data.ready;
assign m_axis_rxwrite_data.data = axis_txwrite_data.data;
assign m_axis_rxwrite_data.keep = axis_txwrite_data.keep;
assign m_axis_rxwrite_data.last = axis_txwrite_data.last;

assign axis_txread_data.valid = s_axis_txread_data.valid;
assign s_axis_txread_data.ready = axis_txread_data.ready;
assign axis_txread_data.data = s_axis_txread_data.data;
assign axis_txread_data.keep = s_axis_txread_data.keep;
assign axis_txread_data.last = s_axis_txread_data.last;

assign m_axis_txwrite_data.valid = axis_txwrite_data.valid;
assign axis_txwrite_data.ready = m_axis_txwrite_data.ready;
assign m_axis_txwrite_data.data = axis_txwrite_data.data;
assign m_axis_txwrite_data.keep = axis_txwrite_data.keep;
assign m_axis_txwrite_data.last = axis_txwrite_data.last;


//RoCE Data Path
assign axis_roce_read_data.valid = s_axis_roce_read_data.valid; 
assign s_axis_roce_read_data.ready = axis_roce_read_data.ready;
assign axis_roce_read_data.data = s_axis_roce_read_data.data;
assign axis_roce_read_data.keep = s_axis_roce_read_data.keep;
assign axis_roce_read_data.last = s_axis_roce_read_data.last;

assign axis_tx_data.valid = s_axis_roce_role_tx_data.valid;
assign s_axis_roce_role_tx_data.ready = axis_tx_data.ready;
assign axis_tx_data.data = s_axis_roce_role_tx_data.data;
assign axis_tx_data.keep = s_axis_roce_role_tx_data.keep;
assign axis_tx_data.last = s_axis_roce_role_tx_data.last;

assign m_axis_roce_write_data.valid = axis_roce_write_data.valid;
assign axis_roce_write_data.ready = m_axis_roce_write_data.ready;
assign m_axis_roce_write_data.data = axis_roce_write_data.data;
assign m_axis_roce_write_data.keep = axis_roce_write_data.keep;
assign m_axis_roce_write_data.last = axis_roce_write_data.last;
end
endgenerate
/*
 * Statistics
 */
logic[31:0] rx_word_counter; 
logic[31:0] rx_pkg_counter; 
logic[31:0] tx_word_counter; 
logic[31:0] tx_pkg_counter;

logic[31:0] tcp_rx_pkg_counter;
logic[31:0] tcp_tx_pkg_counter;

logic[31:0] roce_rx_pkg_counter;
logic[31:0] roce_tx_pkg_counter;

logic[31:0] roce_data_rx_word_counter;
logic[31:0] roce_data_rx_pkg_counter;
logic[31:0] roce_data_tx_role_word_counter;
logic[31:0] roce_data_tx_role_pkg_counter;
logic[31:0] roce_data_tx_host_word_counter;
logic[31:0] roce_data_tx_host_pkg_counter;

logic[31:0] arp_rx_pkg_counter;
logic[31:0] arp_tx_pkg_counter;

reg[7:0]  axis_stream_down_counter;
(* mark_debug = "true" *)reg axis_stream_down;
reg[7:0]  output_stream_down_counter;
reg output_stream_down;

always @(posedge net_clk) begin
    if (~net_aresetn) begin
        rx_word_counter <= '0;
        rx_pkg_counter <= '0;
        tx_word_counter <= '0;
        tx_pkg_counter <= '0;

        tcp_rx_pkg_counter <= '0;
        tcp_tx_pkg_counter <= '0;

        roce_data_rx_word_counter <= '0;
        roce_data_rx_pkg_counter <= '0;
        roce_data_tx_role_word_counter <= '0;
        roce_data_tx_role_pkg_counter <= '0;
        roce_data_tx_host_word_counter <= '0;
        roce_data_tx_host_pkg_counter <= '0;
        
        arp_rx_pkg_counter <= '0;
        arp_tx_pkg_counter <= '0;
        
        roce_rx_pkg_counter <= '0;
        roce_tx_pkg_counter <= '0;

        axis_stream_down_counter <= '0;
        axis_stream_down <= 1'b0;
    end
    else begin
        if (s_axis_net.ready) begin
            axis_stream_down_counter <= '0;
        end
        if (s_axis_net.valid && ~s_axis_net.ready) begin
            axis_stream_down_counter <= axis_stream_down_counter + 1;
        end
        if (axis_stream_down_counter > 2) begin
            axis_stream_down <= 1'b1;
        end
        if (s_axis_net.valid && s_axis_net.ready) begin
            rx_word_counter <= rx_word_counter + 1;
            if (s_axis_net.last) begin
                rx_pkg_counter <= rx_pkg_counter + 1;
            end
        end
        if (m_axis_net.valid && m_axis_net.ready) begin
            tx_word_counter <= tx_word_counter + 1;
            if (m_axis_net.last) begin
                tx_pkg_counter <= tx_pkg_counter + 1;
            end
        end
        //arp
        if (axis_arp_slice_to_arp.valid && axis_arp_slice_to_arp.ready) begin
            if (axis_arp_slice_to_arp.last) begin
                arp_rx_pkg_counter <= arp_rx_pkg_counter + 1;
            end
        end
        if (axis_arp_to_arp_slice.valid && axis_arp_to_arp_slice.ready) begin
            if (axis_arp_to_arp_slice.last) begin
                arp_tx_pkg_counter <= arp_tx_pkg_counter + 1;
            end
        end
        //tcp
        if (axis_toe_slice_to_toe.valid && axis_toe_slice_to_toe.ready) begin
            if (axis_toe_slice_to_toe.last) begin
                tcp_rx_pkg_counter <= tcp_rx_pkg_counter + 1;
            end
        end
        if (axis_toe_to_toe_slice.valid && axis_toe_to_toe_slice.ready) begin
            if (axis_toe_to_toe_slice.last) begin
                tcp_tx_pkg_counter <= tcp_tx_pkg_counter + 1;
            end
        end
        //roce
        if (axis_udp_slice_to_udp.valid && axis_udp_slice_to_udp.ready) begin
            if (axis_udp_slice_to_udp.last) begin
                roce_rx_pkg_counter <= rx_pkg_counter + 1;
            end
        end
        if (axis_udp_to_udp_slice.valid && axis_udp_to_udp_slice.ready) begin
            if (axis_udp_to_udp_slice.last) begin
                roce_tx_pkg_counter <= roce_tx_pkg_counter + 1;
            end
        end
        //roce data
        if (m_axis_roce_write_data.valid && m_axis_roce_write_data.ready) begin
            roce_data_rx_word_counter <= roce_data_rx_word_counter + 1;
            if (m_axis_roce_write_data.last) begin
                roce_data_rx_pkg_counter <= roce_data_rx_pkg_counter + 1;
            end
        end
        if (s_axis_roce_read_data.valid && s_axis_roce_read_data.ready) begin
            roce_data_tx_host_word_counter <= roce_data_tx_host_word_counter + 1;
            if (s_axis_roce_read_data.last) begin
                roce_data_tx_host_pkg_counter <= roce_data_tx_host_pkg_counter + 1;
            end
        end
        if (s_axis_roce_role_tx_data.valid && s_axis_roce_role_tx_data.ready) begin
            roce_data_tx_role_word_counter <= roce_data_tx_role_word_counter + 1;
            if (s_axis_roce_role_tx_data.last) begin
                roce_data_tx_role_pkg_counter <= roce_data_tx_role_pkg_counter + 1;
            end
        end
    end
end

endmodule

`default_nettype wire
