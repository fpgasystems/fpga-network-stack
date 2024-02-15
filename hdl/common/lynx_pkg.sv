`define EN_STRM
`define EN_BPSS
`define EN_AVX
`define EN_RDMA_0
`define EN_RDMA
`define EN_NET_0
`define EN_NET
`define EN_ACLK
`define EN_NCLK
`define EN_XCH_0
`define EN_STATS
`define VITIS_HLS
	
package lynxTypes;

    // -----------------------------------------------------------------
    // Functions
    // -----------------------------------------------------------------
    function integer clog2s;
    input [31:0] v;
    reg [31:0] value;
    begin
        value = v;
        if (value == 1) begin
            clog2s = 1;
        end
        else begin
            value = value-1;
            for (clog2s=0; value>0; clog2s=clog2s+1)
                value = value>>1;
        end
    end
    endfunction

    // -----------------------------------------------------------------
    // Static
    // -----------------------------------------------------------------

    // AXI
    parameter integer AXIL_DATA_BITS = 64;
    parameter integer AVX_DATA_BITS = 256;
    parameter integer AXI_DATA_BITS = 512;
    parameter integer AXI_ADDR_BITS = 64;
    parameter integer AXI_NET_BITS = 512;
    parameter integer AXI_DDR_BITS = 512;
    parameter integer AXI_TLB_BITS = 128;
    parameter integer AXI_ID_BITS = 6;

    // TLB ram
    parameter integer TLB_S_ORDER = 10;
    parameter integer PG_S_BITS = 12;
    parameter integer N_S_ASSOC = 4;

    parameter integer TLB_L_ORDER = 9;
    parameter integer PG_L_BITS = 21;
    parameter integer N_L_ASSOC = 2;

    parameter integer TLB_TMR_REF_CLR = 100000;

    // Data
    parameter integer ADDR_BITS = 64;
    parameter integer PADDR_BITS = 40;
    parameter integer VADDR_BITS = 48;
    parameter integer LEN_BITS = 28;
    parameter integer TLB_DATA_BITS = 96;
    parameter integer DEST_BITS = 4;
    parameter integer PID_BITS = 6;
    parameter integer USER_BITS = 4;

    // Queue depth
    parameter integer QUEUE_DEPTH = 8;

    // Slices
    parameter integer N_REG_DYN_HOST_S0 = 4; // 4
    parameter integer N_REG_DYN_HOST_S1 = 3; // 3
    parameter integer N_REG_DYN_HOST_S2 = 3; // 3
    parameter integer N_REG_DYN_CARD_S0 = 4; // 4
    parameter integer N_REG_DYN_CARD_S1 = 3; // 3
    parameter integer N_REG_DYN_CARD_S2 = 3; // 3
    parameter integer N_REG_DYN_NET_S0 = 4; // 4
    parameter integer N_REG_DYN_NET_S1 = 3; // 3
    parameter integer N_REG_DYN_NET_S2 = 4; // 3
    parameter integer N_REG_NET_S0 = 4; // 4
    parameter integer N_REG_NET_S1 = 3; // 3
    parameter integer N_REG_NET_S2 = 4; // 4
    parameter integer N_REG_CLK_CNVRT = 4; // 5
    parameter integer N_REG_ECI_S0 = 3; // 3
    parameter integer N_REG_ECI_S1 = 2; // 2
    parameter integer N_REG_DYN_DCPL = 4; // 4
    parameter integer N_REG_PR = 4; // 4
    parameter integer NET_STATS_DELAY = 4; // 4
    parameter integer XDMA_STATS_DELAY = 4; // 4

    // Network
    parameter integer ARP_LUP_REQ_BITS = 32;
    parameter integer ARP_LUP_RSP_BITS = 56;
    parameter integer IP_ADDR_BITS = 32;
    parameter integer MAC_ADDR_BITS = 48;
    parameter integer DEF_MAC_ADDRESS = 48'hE59D02350A00; // LSB first, 00:0A:35:02:9D:E5
    parameter integer DEF_IP_ADDRESS = 32'hD1D4010B; // LSB first, 0B:01:D4:D1
    parameter integer NET_STRM_DOWN_THRS = 256;

    // Network RDMA
    parameter integer APP_READ = 0;
    parameter integer APP_WRITE = 1;
    parameter integer APP_SEND = 2;
    parameter integer APP_IMMED = 3;

    parameter integer RC_SEND_FIRST = 5'h0;
    parameter integer RC_SEND_MIDDLE = 5'h1;
    parameter integer RC_SEND_LAST = 5'h2;
    parameter integer RC_SEND_ONLY = 5'h4;
    parameter integer RC_RDMA_WRITE_FIRST = 5'h6;
    parameter integer RC_RDMA_WRITE_MIDDLE = 5'h7;
    parameter integer RC_RDMA_WRITE_LAST = 5'h8;
    parameter integer RC_RDMA_WRITE_LAST_WITH_IMD = 5'h9;
    parameter integer RC_RDMA_WRITE_ONLY = 5'hA;
    parameter integer RC_RDMA_WRITE_ONLY_WIT_IMD = 5'hB;
    parameter integer RC_RDMA_READ_REQUEST = 5'hC;
    parameter integer RC_RDMA_READ_RESP_FIRST = 5'hD;
    parameter integer RC_RDMA_READ_RESP_MIDDLE = 5'hE;
    parameter integer RC_RDMA_READ_RESP_LAST = 5'hF;
    parameter integer RC_RDMA_READ_RESP_ONLY = 5'h10;
    parameter integer RC_ACK = 5'h11;

    parameter integer RDMA_ACK_BITS = 40;
    parameter integer RDMA_ACK_QPN_BITS = 10;
    parameter integer RDMA_ACK_SYNDROME_BITS = 8;
    parameter integer RDMA_ACK_PSN_BITS = 24;
    parameter integer RDMA_ACK_MSN_BITS = 24;
    parameter integer RDMA_BASE_REQ_BITS = 96;
    parameter integer RDMA_VADDR_BITS = 64;
    parameter integer RDMA_LEN_BITS = 32;
    parameter integer RDMA_REQ_BITS = 256;
    parameter integer RDMA_OPCODE_BITS = 5;
    parameter integer RDMA_QPN_BITS = 10;
    parameter integer RDMA_PARAMS_BITS = 32;
    parameter integer RDMA_MSG_BITS = 192;
    
    // Changed the bitwidth to 184 = 168 + 16 to account for rkey 32 Bit instead of 16 Bit
    parameter integer RDMA_QP_INTF_BITS = 184;
    parameter integer RDMA_QP_CONN_BITS = 184;
    parameter integer RDMA_LVADDR_OFFS = 0;
    parameter integer RDMA_RVADDR_OFFS = RDMA_VADDR_BITS;
    parameter integer RDMA_LEN_OFFS = 2*RDMA_VADDR_BITS;
    parameter integer RDMA_PARAMS_OFFS = 2*RDMA_VADDR_BITS + RDMA_LEN_BITS;
    parameter integer RDMA_MSN_BITS = 24;
    parameter integer RDMA_OFFS_BITS = 4;
    parameter integer RDMA_SNDRM_BITS = 8;
    parameter integer RDMA_MAX_OUTSTANDING = 32;
    parameter integer RDMA_MODE_PARSE = 0;
    parameter integer RDMA_MODE_RAW = 1;
    parameter integer RDMA_MAX_SINGLE_READ = 256 * 1024;

    // Network TCP/IP
    parameter integer N_TCP_CHANNELS = 2;
    parameter integer TCP_PORT_REQ_BITS = 16;
    parameter integer TCP_PORT_RSP_BITS = 8;
    parameter integer TCP_OPEN_CONN_REQ_BITS = 48;
    parameter integer TCP_OPEN_CONN_RSP_BITS = 72;
    parameter integer TCP_CLOSE_CONN_REQ_BITS = 16;
    parameter integer TCP_NOTIFY_BITS = 88;
    parameter integer TCP_RD_PKG_REQ_BITS = 32;
    parameter integer TCP_RX_META_BITS = 16;
    parameter integer TCP_TX_META_BITS = 32;
    parameter integer TCP_TX_STAT_BITS = 64;
    parameter integer TCP_MEM_CMD_BITS = 96;
    parameter integer TCP_MEM_STS_BITS = 32;

    parameter integer TCP_IP_ADDRESS_BITS = 32;
    parameter integer TCP_IP_PORT_BITS = 16;
    parameter integer TCP_SESSION_BITS = 16;
    parameter integer TCP_SUCCESS_BITS = 8;
    parameter integer TCP_LEN_BITS = 16;
    parameter integer TCP_REM_SPACE_BITS = 30;
    parameter integer TCP_ERROR_BITS = 2;

    // ECI
    parameter integer N_LANES = 12;
    parameter integer N_LANES_GRPS = 3;
    parameter integer ECI_DATA_BITS = 1024;
    parameter integer ECI_ADDR_BITS = 40;

    // -----------------------------------------------------------------
    // Dynamic
    // -----------------------------------------------------------------

    // Flow
    parameter integer PROBE_ID = 219540062;
    parameter integer N_CHAN = 1;
    parameter integer N_REGIONS = 1;
    parameter integer N_DDR_CHAN = 0;
    parameter integer N_MEM_CHAN = 0;
    parameter integer DDR_FRAG_SIZE = 1024;
    parameter integer DDR_CHAN_SIZE = 0;
    parameter integer PR_FLOW = 0;
    parameter integer AVX_FLOW = 1;
    parameter integer BPSS_FLOW = 1;
    parameter integer TLBF_FLOW = 0;
    parameter integer WB_FLOW = 0;
    parameter integer STRM_FLOW = 1;
    parameter integer MEM_FLOW = 0;
    parameter integer RDMA_0_FLOW = 1;
    parameter integer RDMA_1_FLOW = 0;
    parameter integer TCP_0_FLOW = 0;
    parameter integer TCP_1_FLOW = 0;
    parameter integer N_OUTSTANDING = 8;
    parameter integer N_CHAN_BITS = clog2s(1);
    parameter integer PMTU_BYTES = 4096;
    parameter integer N_REGIONS_BITS = clog2s(1);
    parameter integer N_DDR_CHAN_BITS = clog2s(0);
    parameter integer N_RDMA = 1;
    parameter integer N_TCP = 0;
    parameter integer RECONFIG_EOS_TIME = 1000000;
    parameter integer N_CARD_AXI = 1;
    parameter integer N_STRM_AXI = 1;
        
    // -----------------------------------------------------------------
    // Structs
    // -----------------------------------------------------------------
    typedef struct packed {
        logic [VADDR_BITS-1:0] vaddr;
        logic [LEN_BITS-1:0] len;
        logic stream;
        logic sync;
        logic ctl;
        logic host;
        logic [DEST_BITS-1:0] dest;
        logic [PID_BITS-1:0] pid;
        logic [N_REGIONS_BITS-1:0] vfid;
        logic [96-4-N_REGIONS_BITS-VADDR_BITS-LEN_BITS-DEST_BITS-PID_BITS-1:0] rsrvd;
    } req_t;

    typedef struct packed {
        logic [DEST_BITS-1:0] dest;
        logic [PID_BITS-1:0] pid;
        logic [USER_BITS-1:0] user;
    } cred_t;

    typedef struct packed {
        logic [VADDR_BITS-1:0] vaddr;
        logic [LEN_BITS-1:0] len;
        logic [PID_BITS-1:0] pid;
    } pfault_t;

    typedef struct packed {
        logic [PADDR_BITS-1:0] paddr;
        logic [31:0] value;
    } wback_t;

    typedef struct packed {
        logic [PADDR_BITS-1:0] paddr;
        logic [LEN_BITS-1:0] len;
        logic ctl;
        logic [DEST_BITS-1:0] dest;
        logic [PID_BITS-1:0] pid;
        logic stream;
        logic host;
        logic [96-PADDR_BITS-LEN_BITS-1-DEST_BITS-PID_BITS-1-1-1:0] rsrvd;
    } dma_req_t;

    typedef struct packed {
        logic done;
        logic host;
        logic stream;
        logic [DEST_BITS-1:0] dest;
        logic [PID_BITS-1:0] pid;
    } dma_rsp_t;

    typedef struct packed {
        logic [PADDR_BITS-1:0] paddr_card;
        logic [PADDR_BITS-1:0] paddr_host;
        logic [LEN_BITS-1:0] len;
        logic ctl;
        logic [DEST_BITS-1:0] dest;
        logic [PID_BITS-1:0] pid;
        logic isr;
        logic stream;
        logic host;
        logic [128-2*PADDR_BITS-LEN_BITS-1-DEST_BITS-PID_BITS-1-1-1-1:0] rsrvd;
    } dma_isr_req_t;

    typedef struct packed {
        logic done;
        logic host;
        logic stream;
        logic [DEST_BITS-1:0] dest;
        logic isr;
        logic [PID_BITS-1:0] pid;
    } dma_isr_rsp_t;

    typedef struct packed {
        logic [RDMA_OPCODE_BITS-1:0] opcode;
        logic [RDMA_QPN_BITS-1:0] qpn;
        logic host;
        logic mode;
        logic last;
        logic cmplt;
        logic [RDMA_MSN_BITS-1:0] ssn;
        logic [RDMA_OFFS_BITS-1:0] offs;
        logic [RDMA_MSG_BITS-1:0] msg;
        logic [RDMA_REQ_BITS-RDMA_MSG_BITS-RDMA_OFFS_BITS-RDMA_MSN_BITS-4-RDMA_QPN_BITS-RDMA_OPCODE_BITS-1:0] rsrvd;
    } rdma_req_t;

    typedef struct packed {
        logic rd;
        logic cmplt;
        logic [PID_BITS-1:0] pid;
        logic [DEST_BITS-1:0] vfid;
        logic [RDMA_ACK_MSN_BITS-1:0] ssn;
    } rdma_ack_t;

    typedef struct packed {
        logic [TCP_IP_PORT_BITS-1:0] ip_port;
    } tcp_listen_req_t;

    typedef struct packed {
        logic [TCP_SUCCESS_BITS-1:0] open_port_success;
    } tcp_listen_rsp_t;

    typedef struct packed {
        logic [TCP_IP_PORT_BITS-1:0] ip_port;
        logic [TCP_IP_ADDRESS_BITS-1:0] ip_address;
    } tcp_open_req_t;

    typedef struct packed {
        logic [TCP_IP_PORT_BITS-1:0] ip_port;
        logic [TCP_IP_ADDRESS_BITS-1:0] ip_address;
        logic [TCP_SUCCESS_BITS-1:0] success;
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_open_rsp_t;

    typedef struct packed {
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_close_req_t;

    typedef struct packed {
        logic [TCP_SUCCESS_BITS-1:0] closed;
        logic [TCP_IP_PORT_BITS-1:0] dst_port;
        logic [TCP_IP_ADDRESS_BITS-1:0] ip_address;
        logic [TCP_LEN_BITS-1:0] len;
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_notify_t;

    typedef struct packed {
        logic [PID_BITS-1:0] pid;
        logic [TCP_LEN_BITS-1:0] len;
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_rd_pkg_t;  

    typedef struct packed {
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_rx_meta_t;

    typedef struct packed {
        logic [PID_BITS-1:0] pid;
        logic [TCP_LEN_BITS-1:0] len;
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_tx_meta_t;

    typedef struct packed {
        logic [TCP_ERROR_BITS-1:0] error;
        logic [TCP_REM_SPACE_BITS-1:0] remaining_space;
        logic [TCP_LEN_BITS-1:0] len;
        logic [TCP_SESSION_BITS-1:0] sid;
    } tcp_tx_stat_t;

    typedef struct packed {
        logic [31:0] bpss_h2c_req_counter;
        logic [31:0] bpss_c2h_req_counter;
        logic [31:0] bpss_h2c_cmpl_counter;
        logic [31:0] bpss_c2h_cmpl_counter;
        logic [31:0] bpss_h2c_axis_counter;
        logic [31:0] bpss_c2h_axis_counter;
    } xdma_stat_t;

    typedef struct packed {
        logic [31:0] rx_pkg_counter;
        logic [31:0] tx_pkg_counter;
        logic [31:0] arp_rx_pkg_counter;
        logic [31:0] arp_tx_pkg_counter;
        logic [31:0] icmp_rx_pkg_counter;
        logic [31:0] icmp_tx_pkg_counter;
        logic [31:0] tcp_rx_pkg_counter;
        logic [31:0] tcp_tx_pkg_counter;
        logic [31:0] roce_rx_pkg_counter;
        logic [31:0] roce_tx_pkg_counter;
        logic [31:0] ibv_rx_pkg_counter;
        logic [31:0] ibv_tx_pkg_counter;
        logic [31:0] roce_psn_drop_counter;
        logic [31:0] roce_retrans_counter;
        logic [15:0] tcp_session_counter;
        logic axis_stream_down;
    } net_stat_t;

endpackage