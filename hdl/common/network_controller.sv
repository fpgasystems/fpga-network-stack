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

module network_controller
(
    //clk
    input  wire         pcie_clk,
    input  wire         pcie_aresetn,
    // user clk
    input wire          net_clk,
    input wire          net_aresetn,
    
    //Control interface
    axi_lite.slave      s_axil,
    axi_mm.slave        s_axim,

    output reg         m_axis_qp_interface_valid,
    input wire          m_axis_qp_interface_ready,
    output reg[143:0]  m_axis_qp_interface_data,
  
    output reg         m_axis_qp_conn_interface_valid,
    input wire          m_axis_qp_conn_interface_ready,
    output reg[183:0]  m_axis_qp_conn_interface_data,

    output wire        m_axis_tx_meta_valid,
    input wire         m_axis_tx_meta_ready,
    output wire[159:0] m_axis_tx_meta_data,

    output reg          m_axis_host_arp_lookup_request_TVALID,
    input wire          m_axis_host_arp_lookup_request_TREADY,
    output reg[31:0]    m_axis_host_arp_lookup_request_TDATA,
    input wire          s_axis_host_arp_lookup_reply_TVALID,
    output reg          s_axis_host_arp_lookup_reply_TREADY,
    input wire[55:0]    s_axis_host_arp_lookup_reply_TDATA,

    output reg          m_axis_pc_meta_valid,
    input wire          m_axis_pc_meta_ready,
    output reg[95:0]    m_axis_pc_meta_data,

    input wire[31:0]    roce_crc_pkg_drop_count,
    input wire[31:0]    roce_psn_pkg_drop_count,
    input wire[31:0]    rx_word_counter,
    input wire[31:0]    rx_pkg_counter,
    input wire[31:0]    tx_word_counter,
    input wire[31:0]    tx_pkg_counter,

    //arp
    input wire[31:0]    arp_rx_pkg_counter,
    input wire[31:0]    arp_tx_pkg_counter,
    input wire[15:0]    arp_request_pkg_counter,
    input wire[15:0]    arp_reply_pkg_counter,
    //icmp
    input wire[31:0]    icmp_rx_pkg_counter,
    input wire[31:0]    icmp_tx_pkg_counter,
    //tcp
    input wire[31:0]    tcp_rx_pkg_counter,
    input wire[31:0]    tcp_tx_pkg_counter,
    //roce
    input wire[31:0]    roce_rx_pkg_counter,
    input wire[31:0]    roce_tx_pkg_counter,
    //roce data
    input wire[31:0]    roce_data_rx_word_counter,
    input wire[31:0]    roce_data_rx_pkg_counter,
    input wire[31:0]    roce_data_tx_role_word_counter,
    input wire[31:0]    roce_data_tx_role_pkg_counter,
    input wire[31:0]    roce_data_tx_host_word_counter,
    input wire[31:0]    roce_data_tx_host_pkg_counter,

    input wire          axis_stream_down,

    output reg         set_ip_addr_valid,
    output reg[31:0]   set_ip_addr_data,
    
    output reg          set_board_number_valid,
    output reg[3:0]     set_board_number_data


);

localparam AXI_RESP_OK = 2'b00;
localparam AXI_RESP_SLVERR = 2'b10;

//TODO clean up and use enum

//WRITE states
localparam WRITE_IDLE = 0;
localparam WRITE_DATA = 1;
localparam WRITE_RESPONSE = 2;
localparam WRITE_CONTEXT = 3;
localparam WRITE_CONN = 4;
//localparam WRITE_TLB = 5;
localparam WRITE_POST = 6;
//localparam WRITE_PART_CMD = 7;
//localparam WRITE_PART_MAP = 8;
//localparam RESET_RX_PART = 9;
localparam WRITE_ARP_LOOKUP = 10;
localparam WRITE_PC_META = 11;

//READ states
localparam READ_IDLE = 0;
//localparam READ_DATA = 1;
localparam READ_RESPONSE = 1;
localparam READ_RESPONSE2 = 2;
localparam WAIT_BRAM = 3;

//ADDRESES
localparam GPIO_REG_CTX         = 8'h00;
localparam GPIO_REG_CONN        = 8'h01;
//localparam GPIO_REG_TLB         = 8'h02;
localparam GPIO_REG_POST        = 8'h03;
//localparam GPIO_REG_PART_CMD    = 8'h04;
//localparam GPIO_REG_PART_MAP    = 8'h05;
//localparam GPIO_REG_PART_RESET  = 8'h06;
localparam GPIO_REG_BOARDNUM    = 8'h07;
localparam GPIO_REG_IPADDR      = 8'h08;
localparam GPIO_REG_ARP         = 8'h09;
//localparam GPIO_REG_DMA_READS   = 8'h0A;
//localparam GPIO_REG_DMA_WRITES  = 8'h0B;
localparam GPIO_REG_DEBUG       = 8'h0C;
//localparam GPIO_REG_DEBUG2      = 8'h0D;
localparam GPIO_REG_CMDS        = 8'h0E;
localparam GPIO_REG_PC_META     = 8'h0F;
localparam GPIO_REG_PART_TUPLES = 8'h10;

//localparam GPIO_REG_CRC_DROPS   = 8'h11;

localparam DIRECTION_RX = 0;
localparam DIRECTION_TX = 1;

localparam NUM_DEBUG_REGS = 25;

localparam DEBUG_CRC_DROPS  = 8'h00;
localparam DEBUG_PSN_DROPS  = 8'h01;
localparam DEBUG_RX_WORDS   = 8'h02;
localparam DEBUG_RX_PKGS    = 8'h03;
localparam DEBUG_TX_WORDS   = 8'h04;
localparam DEBUG_TX_PKGS    = 8'h05;
localparam DEBUG_ARP_RX_PKG = 8'h06;
localparam DEBUG_ARP_TX_PKG = 8'h07;
localparam DEBUG_ARP_REQ_PKG = 8'h08;
localparam DEBUG_ARP_RSP_PKG = 8'h09;
localparam DEBUG_ICMP_RX_PKG = 8'h0A;
localparam DEBUG_ICMP_TX_PKG = 8'h0B;
localparam DEBUG_TCP_RX_PKG = 8'h0C;
localparam DEBUG_TCP_TX_PKG = 8'h0D;

localparam DEBUG_ROCE_RX_PKG = 8'h10;
localparam DEBUG_ROCE_TX_PKG = 8'h11;
localparam DEBUG_ROCE_DATA_RX_WORDS  = 8'h12;
localparam DEBUG_ROCE_DATA_RX_PKGS   = 8'h13;
localparam DEBUG_ROCE_DATA_HOST_TX_WORDS  = 8'h14;
localparam DEBUG_ROCE_DATA_HOST_TX_PKGS   = 8'h15;
localparam DEBUG_ROCE_DATA_ROLE_TX_WORDS  = 8'h16;
localparam DEBUG_ROCE_DATA_ROLE_TX_PKGS   = 8'h17;
localparam DEBUG_NET_DOWN       = 8'h18;

reg net_aresetn_reg;
always @(posedge net_clk)
begin
    net_aresetn_reg <= net_aresetn;
end

// REGISTER SLICE

wire  [31:0] axil_awaddr;
wire         axil_awvalid;
reg          axil_awready;
wire  [31:0] axil_wdata;
wire   [3:0] axil_wstrb;
wire         axil_wvalid;
reg          axil_wready;
reg   [1:0]  axil_bresp;
reg          axil_bvalid;
wire         axil_bready;
wire  [31:0] axil_araddr;
wire         axil_arvalid;
reg          axil_arready;
reg  [31:0]  axil_rdata;
reg   [1:0]  axil_rresp;
reg          axil_rvalid;
wire         axil_rready;


axil_clock_converter axi_lite_clock_converter (
  .s_axi_aclk(pcie_clk),                    // input wire aclk
  .s_axi_aresetn(pcie_aresetn),              // input wire aresetn
  .s_axi_awaddr(s_axil.awaddr),    // input wire [31 : 0] s_axi_awaddr
  .s_axi_awprot(3'b00),    // input wire [2 : 0] s_axi_awprot
  .s_axi_awvalid(s_axil.awvalid),  // input wire s_axi_awvalid
  .s_axi_awready(s_axil.awready),  // output wire s_axi_awready
  .s_axi_wdata(s_axil.wdata),      // input wire [31 : 0] s_axi_wdata
  .s_axi_wstrb(s_axil.wstrb),      // input wire [3 : 0] s_axi_wstrb
  .s_axi_wvalid(s_axil.wvalid),    // input wire s_axi_wvalid
  .s_axi_wready(s_axil.wready),    // output wire s_axi_wready
  .s_axi_bresp(s_axil.bresp),      // output wire [1 : 0] s_axi_bresp
  .s_axi_bvalid(s_axil.bvalid),    // output wire s_axi_bvalid
  .s_axi_bready(s_axil.bready),    // input wire s_axi_bready
  .s_axi_araddr(s_axil.araddr),    // input wire [31 : 0] s_axi_araddr
  .s_axi_arprot(3'b00),    // input wire [2 : 0] s_axi_arprot
  .s_axi_arvalid(s_axil.arvalid),  // input wire s_axi_arvalid
  .s_axi_arready(s_axil.arready),  // output wire s_axi_arready
  .s_axi_rdata(s_axil.rdata),      // output wire [31 : 0] s_axi_rdata
  .s_axi_rresp(s_axil.rresp),      // output wire [1 : 0] s_axi_rresp
  .s_axi_rvalid(s_axil.rvalid),    // output wire s_axi_rvalid
  .s_axi_rready(s_axil.rready),    // input wire s_axi_rready

  .m_axi_aclk(net_clk),        // input wire m_axi_aclk
  .m_axi_aresetn(net_aresetn),  // input wire m_axi_aresetn
  .m_axi_awaddr(axil_awaddr),    // output wire [31 : 0] m_axi_awaddr
  .m_axi_awprot(),    // output wire [2 : 0] m_axi_awprot
  .m_axi_awvalid(axil_awvalid),  // output wire m_axi_awvalid
  .m_axi_awready(axil_awready),  // input wire m_axi_awready
  .m_axi_wdata(axil_wdata),      // output wire [31 : 0] m_axi_wdata
  .m_axi_wstrb(axil_wstrb),      // output wire [3 : 0] m_axi_wstrb
  .m_axi_wvalid(axil_wvalid),    // output wire m_axi_wvalid
  .m_axi_wready(axil_wready),    // input wire m_axi_wready
  .m_axi_bresp(axil_bresp),      // input wire [1 : 0] m_axi_bresp
  .m_axi_bvalid(axil_bvalid),    // input wire m_axi_bvalid
  .m_axi_bready(axil_bready),    // output wire m_axi_bready
  .m_axi_araddr(axil_araddr),    // output wire [31 : 0] m_axi_araddr
  .m_axi_arprot(),    // output wire [2 : 0] m_axi_arprot
  .m_axi_arvalid(axil_arvalid),  // output wire m_axi_arvalid
  .m_axi_arready(axil_arready),  // input wire m_axi_arready
  .m_axi_rdata(axil_rdata),      // input wire [31 : 0] m_axi_rdata
  .m_axi_rresp(axil_rresp),      // input wire [1 : 0] m_axi_rresp
  .m_axi_rvalid(axil_rvalid),    // input wire m_axi_rvalid
  .m_axi_rready(axil_rready)    // output wire m_axi_rready
);

// handle AXI-MM read signals that are not used on system clock side
always @(posedge pcie_clk)
begin
    if (~pcie_aresetn) begin
       s_axim.arready <= 1'b0;
       s_axim.rid     <= {4{1'b0}};
       s_axim.rdata   <= {3{1'b0}};
       s_axim.rresp   <= {2{1'b0}};
       s_axim.rlast   <= 1'b0;
       s_axim.rvalid  <= 1'b0;
    end
    else begin
       s_axim.arready <= 1'b1;
    end
end

/*
// AXI-MM clock converter
axim_clock_converter axi_mm_clock_converter_inst(
//--- System Signals ---
.s_axi_aclk      (pcie_clk),
.s_axi_aresetn   (pcie_aresetn), 

//--- Write Address Channel Signals ---
.s_axi_awaddr    (s_axim.awaddr),
.s_axi_awprot    (s_axim.awprot),
.s_axi_awvalid   (s_axim.awvalid),
.s_axi_awready   (s_axim.awready),
.s_axi_awsize    (s_axim.awsize),
.s_axi_awburst   (s_axim.awburst),
.s_axi_awcache   (s_axim.awcache),
.s_axi_awlen     (s_axim.awlen),
.s_axi_awlock    (s_axim.awlock),
.s_axi_awqos     ({4{1'b0}}),
.s_axi_awregion  ({4{1'b0}}),
.s_axi_awid      (s_axim.awid),
//--- Write Data Channel Signals ---
.s_axi_wdata     (s_axim.wdata),
.s_axi_wstrb     (s_axim.wstrb),
.s_axi_wvalid    (s_axim.wvalid),
.s_axi_wready    (s_axim.wready),
.s_axi_wlast     (s_axim.wlast),
//--- Write Response Channel Signals ---
.s_axi_bresp     (s_axim.bresp),
.s_axi_bvalid    (s_axim.bvalid),
.s_axi_bready    (s_axim.bready),
.s_axi_bid       (s_axim.bid),

//--- Network Signals ---
.m_axi_aclk      (net_clk),
.m_axi_aresetn   (net_aresetn),
//--- Write Address Channel Signals ---
.m_axi_awaddr    (axim_net.awaddr),
.m_axi_awprot    (axim_net.awprot),
.m_axi_awvalid   (axim_net.awvalid),
.m_axi_awready   (axim_net.awready),
.m_axi_awsize    (axim_net.awsize),
.m_axi_awburst   (axim_net.awburst),
.m_axi_awcache   (axim_net.awcache),
.m_axi_awlen     (axim_net.awlen),
.m_axi_awlock    (axim_net.awlock),
.m_axi_awqos     (),
.m_axi_awregion  (),
.m_axi_awid      (axim_net.awid),
//--- Write Data Channel Signals ---
.m_axi_wdata     (axim_net.wdata),
.m_axi_wstrb     (axim_net.wstrb),
.m_axi_wvalid    (axim_net.wvalid),
.m_axi_wready    (axim_net.wready),
.m_axi_wlast     (axim_net.wlast),
//--- Write Response Channel Signals ---
.m_axi_bresp     (axim_net.bresp),
.m_axi_bvalid    (axim_net.bvalid),
.m_axi_bready    (axim_net.bready),
.m_axi_bid       (axim_net.bid)
);*/
  
//Fifo for tx metadata

reg         axis_by_tx_metadata_valid;
wire        axis_by_tx_metadata_ready;
reg[159:0]  axis_by_tx_metadata_data;
wire[31:0]  by_cmd_fifo_count;
wire        axis_by_merge_metadata_valid;
wire        axis_by_merge_metadata_ready;
wire[159:0] axis_by_merge_metadata_data;
reg         axis_legacy_tx_metadata_valid;
wire        axis_legacy_tx_metadata_ready;
reg[159:0]  axis_legacy_tx_metadata_data;
wire[31:0]  legacy_cmd_fifo_count;
wire        axis_legacy_merge_metadata_valid;
wire        axis_legacy_merge_metadata_ready;
wire[159:0] axis_legacy_merge_metadata_data;


// Two-clock cmd_fifo: 2 stage synchronization 
axis_data_fifo_160_cc cmd_fifo (
  .s_axis_aclk       (pcie_clk),                      // input wire s_axis_aresetn
  .s_axis_aresetn    (pcie_aresetn),                  // input wire s_axis_aclk
  .s_axis_tvalid     (axis_by_tx_metadata_valid),        // input wire s_axis_tvalid
  .s_axis_tready     (axis_by_tx_metadata_ready),        // output wire s_axis_tready
  .s_axis_tdata      (axis_by_tx_metadata_data),         // input wire [159 : 0] s_axis_tdata

  .m_axis_aclk       (net_clk),                       // input m_axis_clk
  .m_axis_tvalid     (axis_by_merge_metadata_valid),          // output wire m_axis_tvalid
  .m_axis_tready     (axis_by_merge_metadata_ready),          // input wire m_axis_tready
  .m_axis_tdata      (axis_by_merge_metadata_data),           // output wire [159 : 0] m_axis_tdata

  .axis_wr_data_count(),                              // output wire [31 : 0] axis_wr_data_count
  .axis_rd_data_count(by_cmd_fifo_count)                               // output wire [31 : 0] axis_rd_data_count
);


// Cmd fifo for the legacy interface (e.g. ARM CPUs which don't support AVX2)
axis_data_fifo_160 legacy_cmd_fifo (
  .s_axis_aclk       (net_clk),                      // input wire s_axis_aresetn
  .s_axis_aresetn    (net_aresetn),                  // input wire s_axis_aclk
  .s_axis_tvalid     (axis_legacy_tx_metadata_valid),        // input wire s_axis_tvalid
  .s_axis_tready     (axis_legacy_tx_metadata_ready),        // output wire s_axis_tready
  .s_axis_tdata      (axis_legacy_tx_metadata_data),         // input wire [159 : 0] s_axis_tdata
  .m_axis_tvalid     (axis_legacy_merge_metadata_valid),          // output wire m_axis_tvalid
  .m_axis_tready     (axis_legacy_merge_metadata_ready),          // input wire m_axis_tready
  .m_axis_tdata      (axis_legacy_merge_metadata_data),           // output wire [159 : 0] m_axis_tdata
  .axis_wr_data_count(),                              // output wire [31 : 0] axis_wr_data_count
  .axis_rd_data_count(legacy_cmd_fifo_count)                               // output wire [31 : 0] axis_rd_data_count
);

// Merge the two FIFOs
axis_interconnect_160_2to1 cmd_fifo_merge_inst (
  .ACLK(net_clk),                                  // input wire ACLK
  .ARESETN(net_aresetn),                            // input wire ARESETN
  .S00_AXIS_ACLK(net_clk),                // input wire S00_AXIS_ACLK
  .S00_AXIS_ARESETN(net_aresetn),          // input wire S00_AXIS_ARESETN
  .S00_AXIS_TVALID(axis_by_merge_metadata_valid),            // input wire S00_AXIS_TVALID
  .S00_AXIS_TREADY(axis_by_merge_metadata_ready),            // output wire S00_AXIS_TREADY
  .S00_AXIS_TDATA(axis_by_merge_metadata_data),              // input wire [159 : 0] S00_AXIS_TDATA.

  .S01_AXIS_ACLK(net_clk),                // input wire S01_AXIS_ACLK
  .S01_AXIS_ARESETN(net_aresetn),          // input wire S01_AXIS_ARESETN
  .S01_AXIS_TVALID(axis_legacy_merge_metadata_valid),            // input wire S01_AXIS_TVALID
  .S01_AXIS_TREADY(axis_legacy_merge_metadata_ready),            // output wire S01_AXIS_TREADY
  .S01_AXIS_TDATA(axis_legacy_merge_metadata_data),              // input wire [159 : 0] S01_AXIS_TDATA

  .M00_AXIS_ACLK(net_clk),                // input wire M00_AXIS_ACLK
  .M00_AXIS_ARESETN(net_aresetn),          // input wire M00_AXIS_ARESETN
  .M00_AXIS_TVALID(m_axis_tx_meta_valid),            // output wire M00_AXIS_TVALID
  .M00_AXIS_TREADY(m_axis_tx_meta_ready),            // input wire M00_AXIS_TREADY
  .M00_AXIS_TDATA(m_axis_tx_meta_data),              // output wire [159 : 0] M00_AXIS_TDATA

  .S00_ARB_REQ_SUPPRESS(0),  // input wire S00_ARB_REQ_SUPPRESS
  .S01_ARB_REQ_SUPPRESS(0)  // input wire S01_ARB_REQ_SUPPRESS
);

// ACTUAL LOGIC

reg[7:0] writeState;
reg[7:0] readState;

reg[31:0] writeAddr;
reg[31:0] readAddr;

reg[7:0]  word_counter;

reg[7:0] mmwriteState;
reg[63:0]mmwriteAddr;

//handle writes
//handle writes
always @(posedge net_clk)
begin
    if (~net_aresetn) begin
        axil_awready <= 1'b0;
        axil_wready <= 1'b0;
        axil_bvalid <= 1'b0;
        m_axis_qp_interface_valid <= 1'b0;
        m_axis_qp_conn_interface_valid <= 1'b0;
        axis_legacy_tx_metadata_valid <= 1'b0;
        
        m_axis_pc_meta_valid <= 1'b0;
        
        word_counter <= 0;
        set_ip_addr_valid <= 1'b0;
        set_board_number_valid <= 1'b0;
        
        writeState <= WRITE_IDLE;
    end
    else begin
        case (writeState)
            WRITE_IDLE: begin
                axil_awready <= 1'b1;
                axil_wready <= 1'b0;
                axil_bvalid <= 1'b0;
                m_axis_qp_interface_valid <= 1'b0;
                m_axis_qp_conn_interface_valid <= 1'b0;
                axis_legacy_tx_metadata_valid <= 1'b0;
                m_axis_pc_meta_valid <= 1'b0;
                
                m_axis_host_arp_lookup_request_TVALID <= 1'b0;
                s_axis_host_arp_lookup_reply_TREADY <= 1'b1;
                
                writeAddr <= (axil_awaddr[11:0] >> 5);
                if (axil_awvalid && axil_awready) begin
                    axil_awready <= 1'b0;
                    axil_wready <= 1'b1;
                    writeState <= WRITE_DATA;
                end
            end //WRITE_IDLE
            WRITE_DATA: begin
                axil_wready <= 1'b1;
                if (axil_wvalid && axil_wready) begin
                    axil_wready <= 0;
                    axil_bvalid <= 1'b1;
                    axil_bresp <= AXI_RESP_OK;
                    writeState <= WRITE_RESPONSE;
                    case (writeAddr)
                        GPIO_REG_CTX: begin
                            word_counter <= word_counter + 1;
                            case(word_counter)
                                0: begin
                                    m_axis_qp_interface_data[2:0] <= axil_wdata[2:0];
                                end
                                1: begin
                                    m_axis_qp_interface_data[26:3] <= axil_wdata[23:0];
                                end
                                2: begin
                                    m_axis_qp_interface_data[50:27] <= axil_wdata[23:0];
                                end
                                3: begin
                                    m_axis_qp_interface_data[74:51] <= axil_wdata[23:0];
                                end
                                4: begin
                                    m_axis_qp_interface_data[90:75] <= axil_wdata[15:0];
                                end
                                5: begin
                                    m_axis_qp_interface_data[122:91] <= axil_wdata[31:0];
                                end
                                6: begin
                                    m_axis_qp_interface_data[138:123] <= axil_wdata[15:0];
                                    axil_bvalid <= 1'b0;
                                    m_axis_qp_interface_valid <= 1'b1;
                                    word_counter <= 0;
                                    writeState <= WRITE_CONTEXT; 
                                end
                            endcase
                        end
                        GPIO_REG_CONN: begin
                            word_counter <= word_counter + 1;
                            case(word_counter)
                                0: begin
                                    m_axis_qp_conn_interface_data[15:0] <= axil_wdata[15:0];
                                end
                                1: begin
                                     m_axis_qp_conn_interface_data[39:16] <= axil_wdata[23:0];
                                end
                                2: begin
                                    m_axis_qp_conn_interface_data[71:40] <= axil_wdata[31:0];
                                end
                                3: begin
                                    m_axis_qp_conn_interface_data[103:72] <= axil_wdata[31:0];
                                end
                                4: begin
                                    m_axis_qp_conn_interface_data[135:104] <= axil_wdata[31:0];
                                end
                                5: begin
                                    m_axis_qp_conn_interface_data[167:136] <= axil_wdata[31:0];
                                end
                                6: begin
                                    m_axis_qp_conn_interface_data[183:168] <= axil_wdata[15:0];
                                    axil_bvalid <= 1'b0;
                                    m_axis_qp_conn_interface_valid <= 1'b1;
                                    word_counter <= 0;
                                    writeState <= WRITE_CONN;
                                end
                            endcase
                        end
                        GPIO_REG_POST: begin
                            word_counter <= word_counter + 1;
                            case(word_counter)
                                0: begin
                                    //lower part of qpn
                                    axis_legacy_tx_metadata_data[4:3] <= axil_wdata[1:0];
                                    //originAddr
                                    axis_legacy_tx_metadata_data[28:27] <= 0;
                                    axis_legacy_tx_metadata_data[58:29] <= axil_wdata[31:2];
                                end
                                1: begin
                                    //originAddr
                                    axis_legacy_tx_metadata_data[74:59] <= axil_wdata[15:0];
                                    //upper part of qpn
                                    axis_legacy_tx_metadata_data[6:5] <= axil_wdata[17:16];
                                    //targetAddr
                                    axis_legacy_tx_metadata_data[76:75] <= 0;
                                    axis_legacy_tx_metadata_data[90:77] <= axil_wdata[31:18];
                                end
                                2: begin
                                    //targetAddr
                                    axis_legacy_tx_metadata_data[122:91] <= axil_wdata[31:0];
                                end
                                3: begin
                                    //opCode 3bits
                                    axis_legacy_tx_metadata_data[2:0] <= axil_wdata[2:0];
                                    //length
                                    axis_legacy_tx_metadata_data[125:123] <= 0;
                                    axis_legacy_tx_metadata_data[154:126] <= axil_wdata[31:3];
                                    //set unused part of qpn to 0
                                    axis_legacy_tx_metadata_data[26:7] <= 0;
                                    axil_bvalid <= 1'b0;
                                    axis_legacy_tx_metadata_valid <= 1'b1;
                                    word_counter <= 0;
                                    writeState <= WRITE_POST;
                                end
                            endcase
                         end
                        GPIO_REG_PC_META: begin
                            word_counter <= word_counter + 1;
                            case (word_counter)
                                0: begin //key lower
                                    m_axis_pc_meta_data[31:0] <= axil_wdata[31:0];

                                end
                                1: begin //key lower
                                    m_axis_pc_meta_data[63:32] <= axil_wdata[31:0];
                                end
                                2: begin //rest
                                    m_axis_pc_meta_data[95:64] <= axil_wdata[31:0];
                                    axil_bvalid <= 1'b0;
                                    m_axis_pc_meta_valid <= 1'b1;
                                    word_counter <= 0;
                                    writeState <= WRITE_PC_META;
                                end
                            endcase
                        end
                        GPIO_REG_IPADDR: begin
                            set_ip_addr_valid <= 1'b1;
                            set_ip_addr_data <= axil_wdata[31:0];
                            axil_bvalid <= 1'b1;
                            axil_bresp <= AXI_RESP_OK;
                            writeState <= WRITE_RESPONSE;
                        end
                        GPIO_REG_BOARDNUM: begin
                            set_board_number_valid <= 1'b1;
                            set_board_number_data <= axil_wdata[3:0];
                            axil_bvalid <= 1'b1;
                            axil_bresp <= AXI_RESP_OK;
                            writeState <= WRITE_RESPONSE;
                        end
                        GPIO_REG_ARP: begin
                            m_axis_host_arp_lookup_request_TVALID <= 1'b1;
                            m_axis_host_arp_lookup_request_TDATA[7:0] <= axil_wdata[31:24];
                            m_axis_host_arp_lookup_request_TDATA[15:8] <= axil_wdata[23:16];
                            m_axis_host_arp_lookup_request_TDATA[23:16] <= axil_wdata[15:8];
                            m_axis_host_arp_lookup_request_TDATA[31:24] <= axil_wdata[7:0];
                            axil_bvalid <= 1'b0;
                            writeState <= WRITE_ARP_LOOKUP;
                        end
                    endcase
                end
            end //WRITE_DATA
            WRITE_RESPONSE: begin
                axil_bvalid <= 1'b1;
                if (axil_bvalid && axil_bready) begin
                    axil_bvalid <= 1'b0;
                    writeState <= WRITE_IDLE;
                end
            end//WRITE_RESPONSE
            WRITE_CONTEXT: begin
                m_axis_qp_interface_valid <= 1'b1;
                if (m_axis_qp_interface_valid && m_axis_qp_interface_ready) begin
                    axil_bvalid <= 1'b1;
                    axil_bresp <= AXI_RESP_OK;
                    m_axis_qp_interface_valid <= 1'b0;
                    writeState <= WRITE_RESPONSE;
                end
            end//WRITE_CONTEXT
            WRITE_CONN: begin
                m_axis_qp_conn_interface_valid <= 1'b1;
                if (m_axis_qp_conn_interface_valid && m_axis_qp_conn_interface_ready) begin
                    axil_bvalid <= 1'b1;
                    axil_bresp <= AXI_RESP_OK;
                    m_axis_qp_conn_interface_valid <= 1'b0;
                    writeState <= WRITE_RESPONSE;
                end
            end//WRITE_CONN
            WRITE_POST: begin
                axis_legacy_tx_metadata_valid <= 1'b1;
                if (axis_legacy_tx_metadata_valid && axis_legacy_tx_metadata_ready) begin
                    axil_bvalid <= 1'b1;
                    axil_bresp <= AXI_RESP_OK;
                    axis_legacy_tx_metadata_valid <= 1'b0;
                    writeState <= WRITE_RESPONSE;
                end
             end
            WRITE_PC_META: begin
                m_axis_pc_meta_valid <= 1'b1;
                if (m_axis_pc_meta_valid && m_axis_pc_meta_ready) begin
                    axil_bvalid <= 1'b1;
                    axil_bresp <= AXI_RESP_OK;
                    m_axis_pc_meta_valid <= 1'b0;
                    writeState <= WRITE_RESPONSE;
                end
            end
            WRITE_ARP_LOOKUP: begin
                m_axis_host_arp_lookup_request_TVALID <= 1'b1;
                if (m_axis_host_arp_lookup_request_TVALID && m_axis_host_arp_lookup_request_TREADY) begin
                    axil_bvalid <= 1'b1;
                    axil_bresp <= AXI_RESP_OK;
                    m_axis_host_arp_lookup_request_TVALID <= 1'b0;
                    writeState <= WRITE_RESPONSE;
                end
            end
        endcase
    end
end

//handle mm write
always @(posedge pcie_clk)
begin
    if (~pcie_aresetn) begin
        s_axim.awready         <= 1'b0;
        s_axim.wready          <= 1'b0;

        s_axim.bid             <= '0;
        s_axim.bresp           <= '0;
        s_axim.bvalid          <= 1'b0;

        axis_by_tx_metadata_data  <= '0;
        axis_by_tx_metadata_valid <= 1'b0;

        mmwriteState           <= WRITE_IDLE;
    end
    else begin
        s_axim.bid             <= '0; 
        s_axim.bresp           <= '0;

        case (mmwriteState)
            WRITE_IDLE: begin
                s_axim.awready         <= 1'b1;
                s_axim.wready          <= 1'b0;
                s_axim.bvalid          <= 1'b0;
                axis_by_tx_metadata_valid <= 1'b0;
                        
                mmwriteAddr = (s_axim.awaddr[11:0] >> 6); //64B aligned addresses for AVX -> no alignment issues with PCIe 8x or 16x

                if (s_axim.awvalid && s_axim.awready) begin
                   s_axim.awready      <= 1'b0;
                   s_axim.wready       <= 1'b1;

                   mmwriteState        <= WRITE_DATA;
                end
            end //WRITE_IDLE

            WRITE_DATA: begin
                s_axim.wready <= 1'b1;

                if (s_axim.wvalid && s_axim.wready) begin
                   s_axim.wready <= 1'b0;
                   s_axim.bvalid <= 1'b1;
                   s_axim.bresp  <= AXI_RESP_OK;

                   mmwriteState    <= WRITE_RESPONSE;

                   case (mmwriteAddr)

                        GPIO_REG_POST: begin
                    
                            // sw data is 32 bits aligned 
                            /* s_axim.wdata[31:0]    -> op         - sw: 32 bits
                                           [63:32]   -> qpn        - sw: 32 bits 
                                           [127:64]  -> originAddr - sw: 64 bits
                                           [191:128] -> targetAddr - sw: 64 bits
                                           [223:192] -> size       - sw: 32 bits
                            */
                            
                            // op - use only the 3 LSb that come from sw - [31:0]
                            axis_by_tx_metadata_data[2:0]     <= s_axim.wdata[2:0];

                            // qpn - use only the 4 LSb that come from sw  [63:32]
                            axis_by_tx_metadata_data[6:3]     <= s_axim.wdata[35:32];
                            axis_by_tx_metadata_data[26:7]    <= 0;                                        // 20 bits

                            // originAddr - use only 48 b from the sw  [127:64] 
                            axis_by_tx_metadata_data[74:27]   <= s_axim.wdata[113:64];

                            // targetAddr - use only 48 b from the sw  [191:128]
                            axis_by_tx_metadata_data[122:75]  <= s_axim.wdata[177:128];

                            // size  [255:192]
                            axis_by_tx_metadata_data[154:123] <= s_axim.wdata[223:192];

                            axis_by_tx_metadata_valid         <= 1'b1;
                            s_axim.bvalid                  <= 1'b0;
                            
                            mmwriteState <= WRITE_POST;

                         end
                     endcase
                 end
            end //WRITE_DATA

            WRITE_POST: begin
                if (axis_by_tx_metadata_valid && axis_by_tx_metadata_ready) begin
                    s_axim.bvalid          <= 1'b1;
                    s_axim.bresp           <= AXI_RESP_OK;
                    axis_by_tx_metadata_valid <= 1'b0;

                    s_axim.bvalid <= 1'b1;
                    mmwriteState  <= WRITE_RESPONSE;
                end
            end//WRITE_POST

            WRITE_RESPONSE: begin
                if (s_axim.bvalid && s_axim.bready) begin
                    s_axim.bvalid <= 1'b0;

                    mmwriteState <= WRITE_IDLE;
                end
            end//WRITE_RESPONSE
           
        endcase//mmwriteState
    end
end

//reads are currently not available
reg[7:0] debugRegAddr;

always @(posedge net_clk)
begin
    if (~net_aresetn) begin
        axil_arready <= 1'b0;
        axil_rresp <= 0;
        axil_rvalid <= 1'b0;
        readState <= READ_IDLE;
    end
    else begin      
        case (readState)
            READ_IDLE: begin
                axil_arready <= 1'b1;
                if (axil_arready && axil_arvalid) begin
                    readAddr <= (axil_araddr[11:0] >> 5);
                    axil_arready <= 1'b0;
                    readState <= READ_RESPONSE;
                end
                if (debugRegAddr == NUM_DEBUG_REGS) begin
                    debugRegAddr <= 0;
                end
            end
            READ_RESPONSE: begin
                axil_rvalid <= 1'b1;
                axil_rresp <= AXI_RESP_OK;
                case (readAddr)
                    GPIO_REG_CMDS: begin
                        axil_rdata <= (by_cmd_fifo_count | legacy_cmd_fifo_count);
                    end
                    GPIO_REG_PART_TUPLES: begin
                        axil_rdata <= '0; //part_tuples_counter;
                    end
                    /*GPIO_REG_CRC_DROPS: begin
                        //if (!ddr_bench_cycles_upper) begin
                            axil_rdata <= roce_crc_pkg_drop_count[31:0];*/
                        /*end
                        else begin
                            axil_rdata <= ddr_bench_execution_cycles[63:32];
                        end*/
                    /*end
                    GPIO_REG_PSN_DROPS: begin
                        //if (!dma_bench_cycles_upper) begin
                            axil_rdata <= roce_psn_pkg_drop_count[31:0];
                          end
                        else begin
                            axil_rdata <= dma_bench_execution_cycles[63:32];
                        end*/
                    //end
                    GPIO_REG_DEBUG: begin
                        case (debugRegAddr)
                            DEBUG_CRC_DROPS: begin
                                 axil_rdata <= roce_crc_pkg_drop_count;
                            end
                            DEBUG_PSN_DROPS: begin
                                 axil_rdata <= roce_psn_pkg_drop_count;
                            end
                            DEBUG_RX_WORDS: begin
                                 axil_rdata <= rx_word_counter;
                            end
                            DEBUG_RX_PKGS: begin
                                 axil_rdata <= rx_pkg_counter;
                            end
                            DEBUG_TX_WORDS: begin
                                 axil_rdata <= tx_word_counter;
                            end
                            DEBUG_TX_PKGS: begin
                                 axil_rdata <= tx_pkg_counter;
                            end
                            DEBUG_ARP_RX_PKG: begin
                                axil_rdata <= arp_rx_pkg_counter;
                            end
                            DEBUG_ARP_TX_PKG: begin
                                axil_rdata <= arp_tx_pkg_counter;
                            end
                            DEBUG_ARP_REQ_PKG: begin
                                axil_rdata <= arp_request_pkg_counter;
                            end
                            DEBUG_ARP_RSP_PKG: begin
                                axil_rdata <= arp_reply_pkg_counter;
                            end
                            DEBUG_ICMP_RX_PKG: begin
                                axil_rdata <= icmp_rx_pkg_counter;
                            end
                            DEBUG_ICMP_TX_PKG: begin
                                axil_rdata <= icmp_tx_pkg_counter;
                            end
                            DEBUG_TCP_RX_PKG: begin
                                axil_rdata <= tcp_rx_pkg_counter;
                            end
                            DEBUG_TCP_TX_PKG: begin
                                axil_rdata <= tcp_tx_pkg_counter;
                            end
                            DEBUG_ROCE_RX_PKG: begin
                                axil_rdata <= roce_rx_pkg_counter;
                            end
                            DEBUG_ROCE_TX_PKG: begin
                                axil_rdata <= roce_tx_pkg_counter;
                            end
                            DEBUG_ROCE_DATA_RX_WORDS: begin
                                 axil_rdata <= roce_data_rx_word_counter;
                            end
                            DEBUG_ROCE_DATA_RX_PKGS: begin
                                 axil_rdata <= roce_data_rx_pkg_counter;
                            end
                            DEBUG_ROCE_DATA_ROLE_TX_WORDS: begin
                                 axil_rdata <= roce_data_tx_role_word_counter;
                            end
                            DEBUG_ROCE_DATA_ROLE_TX_PKGS: begin
                                 axil_rdata <= roce_data_tx_role_pkg_counter;
                            end
                            DEBUG_ROCE_DATA_HOST_TX_WORDS: begin
                                 axil_rdata <= roce_data_tx_host_word_counter;
                            end
                            DEBUG_ROCE_DATA_HOST_TX_PKGS: begin
                                 axil_rdata <= roce_data_tx_host_pkg_counter;
                            end
                            DEBUG_NET_DOWN: begin
                                 axil_rdata <= axis_stream_down;
                            end
                            default: begin
                                axil_rresp <= AXI_RESP_SLVERR;
                                axil_rdata <= 0;
                            end
                        endcase
                    end
                    default: begin
                        axil_rresp <= AXI_RESP_SLVERR;
                        axil_rdata <= 32'hdeadbeef;
                    end
                endcase
                if (axil_rvalid && axil_rready) begin
                    axil_rvalid <= 1'b0;
                    if (readAddr == GPIO_REG_DEBUG) begin
                        debugRegAddr <= debugRegAddr + 1; 
                    end
                    readState <= READ_IDLE;
                end
            end
        endcase
    end
end

endmodule
`default_nettype wire
