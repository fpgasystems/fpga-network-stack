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
#include "rocev2_config.hpp"
#include "rocev2.hpp"

template <int WIDTH>
void rocev2(
	hls::stream<net_axis<WIDTH> >& s_axis_rx_data,
	hls::stream<net_axis<WIDTH> >&	m_axis_tx_data,
				
	// S(R)Q
	hls::stream<txMeta>& s_axis_sq_meta,

	// ACKs
	hls::stream<ackMeta>& m_axis_rx_ack_meta,
				
	// RDMA
	hls::stream<memCmd>& m_axis_mem_write_cmd,
	hls::stream<memCmd>& m_axis_mem_read_cmd,
	hls::stream<net_axis<WIDTH> >& m_axis_mem_write_data,
	hls::stream<net_axis<WIDTH> >& s_axis_mem_read_data,

	// QP
	hls::stream<qpContext>&	s_axis_qp_interface,
	hls::stream<ifConnReq>&	s_axis_qp_conn_interface,
	ap_uint<128> local_ip_address,

	//Debug output
#ifdef DBG_IBV
	hls::stream<psnPkg>& m_axis_dbg,
#endif
	ap_uint<32>& regCrcDropPkgCount,
	ap_uint<32>& regInvalidPsnDropCount,
    ap_uint<32>& regRetransCount,
	ap_uint<32>& regIbvCountRx,
    ap_uint<32>& regIbvCountTx
) {
#pragma HLS INLINE

	//metadata fifos
	static stream<ipUdpMeta>	rx_ipUdpMetaFifo("rx_ipUdpMetaFifo");
	static stream<ipUdpMeta>	tx_ipUdpMetaFifo("tx_ipUdpMetaFifo");
	#pragma HLS STREAM depth=8 variable=rx_ipUdpMetaFifo
	#pragma HLS STREAM depth=2 variable=tx_ipUdpMetaFifo

	//IP
	static stream<net_axis<WIDTH> >		rx_crc2ipFifo("rx_crc2ipFifo");
	static stream<net_axis<WIDTH> >		rx_udp2ibFifo("rx_udp2ibFifo");
	static stream<net_axis<WIDTH> >		tx_ib2udpFifo("tx_ib2udpFifo");
	static stream<net_axis<WIDTH> >		tx_ip2crcFifo("tx_ip2crcFifo");
	#pragma HLS STREAM depth=2 variable=rx_crc2ipFifo
	#pragma HLS STREAM depth=2 variable=rx_udp2ibFifo
	#pragma HLS STREAM depth=2 variable=tx_ib2udpFifo
	#pragma HLS STREAM depth=2 variable=tx_ip2crcFifo

	static stream<ipMeta>	rx_ip2udpMetaFifo("rx_ip2udpMetaFifo");
	static stream<net_axis<WIDTH> >	rx_ip2udpFifo("rx_ip2udpFifo");
	//static stream<net_axis<WIDTH> >	rx_ip2udpFifo("rx_ip2udpFifo");
	static stream<ipMeta>	tx_udp2ipMetaFifo("tx_udp2ipMetaFifo");
	static stream<net_axis<WIDTH> >	tx_udp2ipFifo("tx_udp2ipFifo");
	#pragma HLS STREAM depth=2 variable=rx_ip2udpMetaFifo
	#pragma HLS STREAM depth=2 variable=rx_ip2udpFifo
	#pragma HLS STREAM depth=2 variable=tx_udp2ipMetaFifo
	#pragma HLS STREAM depth=2 variable=tx_udp2ipFifo

    /*
     * CRC
     */
    crc<WIDTH, 0>(
        s_axis_rx_data,
        rx_crc2ipFifo,
        tx_ip2crcFifo,
        m_axis_tx_data,
        regCrcDropPkgCount
    );

	/*
	 * IPv6 & UDP
	 */
#if IP_VERSION == 6
	ipv6(	
		rx_crc2ipFifo,
		rx_ip2udpMetaFifo,
		rx_ip2udpFifo,
		tx_udp2ipMetaFifo,
		tx_udp2ipFifo,
		tx_ip2crcFifo,
		local_ip_address
	);

	/*
	 * IPv4 & UDP
	 */
#else
	ipv4<WIDTH>(	
		rx_crc2ipFifo,
		rx_ip2udpMetaFifo,
		rx_ip2udpFifo,
		tx_udp2ipMetaFifo,
		tx_udp2ipFifo,
		tx_ip2crcFifo,
		local_ip_address,
		UDP_PROTOCOL
	);
#endif

	udp<WIDTH>(
		rx_ip2udpMetaFifo,
		rx_ip2udpFifo,
		rx_ipUdpMetaFifo,
		rx_udp2ibFifo,
		tx_ipUdpMetaFifo,
		tx_ib2udpFifo,
		tx_udp2ipMetaFifo,
		tx_udp2ipFifo,
		local_ip_address,
		RDMA_DEFAULT_PORT
	);

	/*
	 * IB PROTOCOL
	 */
	ib_transport_protocol<WIDTH, 0>(	
		rx_ipUdpMetaFifo,
		rx_udp2ibFifo,
		tx_ipUdpMetaFifo,
		tx_ib2udpFifo,
		s_axis_sq_meta,
		m_axis_rx_ack_meta,
		m_axis_mem_write_cmd,
		m_axis_mem_read_cmd,
		m_axis_mem_write_data,
		s_axis_mem_read_data,
		s_axis_qp_interface,
		s_axis_qp_conn_interface,
#ifdef DBG_IBV
		m_axis_dbg,
#endif
		regInvalidPsnDropCount,
        regRetransCount,
		regIbvCountRx,
        regIbvCountTx
	);
    
}

#if defined( __VITIS_HLS__)
void rocev2_top(
	stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >& s_axis_rx_data,
	stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >& m_axis_tx_data,
				
	stream<txMeta>&	s_axis_sq_meta,

	stream<ackMeta>& m_axis_rx_ack_meta,
				
	//Memory
	stream<memCmd>& m_axis_mem_write_cmd,
	stream<memCmd>& m_axis_mem_read_cmd,
	stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >& m_axis_mem_write_data,
	stream<ap_axiu<DATA_WIDTH, 0, 0, 0> >& s_axis_mem_read_data,

	//Interface
	stream<qpContext>& s_axis_qp_interface,
	stream<ifConnReq>& s_axis_qp_conn_interface,
	ap_uint<128> local_ip_address,

	//Debug output
#ifdef DBG_IBV
	stream<psnPkg>& m_axis_dbg,
#endif 
	ap_uint<32>& regCrcDropPkgCount,
	ap_uint<32>& regInvalidPsnDropCount,
    ap_uint<32>& regRetransCount,
	ap_uint<32>& regIbvCountRx,
    ap_uint<32>& regIbvCountTx
) {
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	// NET
	#pragma HLS INTERFACE axis register port=s_axis_rx_data
	#pragma HLS INTERFACE axis register port=m_axis_tx_data

	// S(R)Q
	#pragma HLS INTERFACE axis register port=s_axis_sq_meta
	#pragma HLS aggregate  variable=s_axis_sq_meta compact=bit

	// ACKs
	#pragma HLS INTERFACE axis register port=m_axis_rx_ack_meta
	#pragma HLS aggregate  variable=m_axis_rx_ack_meta compact=bit

	// RDMA
	#pragma HLS INTERFACE axis register port=m_axis_mem_write_cmd
	#pragma HLS INTERFACE axis register port=m_axis_mem_read_cmd
	#pragma HLS aggregate  variable=m_axis_mem_write_cmd compact=bit
	#pragma HLS aggregate  variable=m_axis_mem_read_cmd compact=bit
	#pragma HLS INTERFACE axis register port=m_axis_mem_write_data
	#pragma HLS INTERFACE axis register port=s_axis_mem_read_data

	// QP
	#pragma HLS INTERFACE axis register port=s_axis_qp_interface
	#pragma HLS INTERFACE axis register port=s_axis_qp_conn_interface
	#pragma HLS aggregate  variable=s_axis_qp_interface compact=bit
	#pragma HLS aggregate  variable=s_axis_qp_conn_interface compact=bit

	#pragma HLS INTERFACE ap_none register port=local_ip_address

	//DEBUG
#ifdef DBG_IBV
	#pragma HLS INTERFACE axis register port=m_axis_dbg
	#pragma HLS aggregate  variable=m_axis_dbg compact=bit
#endif 
	#pragma HLS INTERFACE ap_vld port=regCrcDropPkgCount

	static hls::stream<net_axis<DATA_WIDTH> > s_axis_rx_data_internal;
	#pragma HLS STREAM depth=2 variable=s_axis_rx_data_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_tx_data_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_tx_data_internal
	static hls::stream<net_axis<DATA_WIDTH> > m_axis_mem_write_data_internal;
	#pragma HLS STREAM depth=2 variable=m_axis_mem_write_data_internal
	static hls::stream<net_axis<DATA_WIDTH> > s_axis_mem_read_data_internal;
	#pragma HLS STREAM depth=2 variable=s_axis_mem_read_data_internal

	convert_axis_to_net_axis<DATA_WIDTH>(s_axis_rx_data, s_axis_rx_data_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_tx_data_internal, m_axis_tx_data);

	convert_axis_to_net_axis<DATA_WIDTH>(s_axis_mem_read_data, s_axis_mem_read_data_internal);

	convert_net_axis_to_axis<DATA_WIDTH>(m_axis_mem_write_data_internal, m_axis_mem_write_data);

   	rocev2<DATA_WIDTH>(			
	   	s_axis_rx_data_internal,
		m_axis_tx_data_internal,
								
		s_axis_sq_meta,
		m_axis_rx_ack_meta,
								
		m_axis_mem_write_cmd,
		m_axis_mem_read_cmd,
		m_axis_mem_write_data_internal,
		s_axis_mem_read_data_internal,

		s_axis_qp_interface,
		s_axis_qp_conn_interface,
		local_ip_address,

#ifdef DBG_IBV
		m_axis_dbg,
#endif 
		regCrcDropPkgCount,
		regInvalidPsnDropCount,
        regRetransCount,
		regIbvCountRx,
        regIbvCountTx
	);
	
#else
void rocev2_top(
	stream<net_axis<DATA_WIDTH> >&	s_axis_rx_data,
	stream<net_axis<DATA_WIDTH> >& m_axis_tx_data,
				
	stream<txMeta>&	s_axis_sq_meta,

	stream<ackMeta>& m_axis_rx_ack_meta,
				
	//Memory
	stream<memCmd>& m_axis_mem_write_cmd,
	stream<memCmd>& m_axis_mem_read_cmd,
	stream<net_axis<DATA_WIDTH> >& m_axis_mem_write_data,
	stream<net_axis<DATA_WIDTH> >& s_axis_mem_read_data,

	//Interface
	stream<qpContext>& s_axis_qp_interface,
	stream<ifConnReq>& s_axis_qp_conn_interface,
	ap_uint<128> local_ip_address,

	//Debug output
#ifdef DBG_IBV
	stream<psnPkg>& m_axis_dbg,
#endif 
	ap_uint<32>& regCrcDropPkgCount,
	ap_uint<32>& regInvalidPsnDropCount,
    ap_uint<32>& regRetransCount,
	ap_uint<32>& regIbvCountRx,
    ap_uint<32>& regIbvCountTx
) {
	#pragma HLS DATAFLOW disable_start_propagation
	#pragma HLS INTERFACE ap_ctrl_none port=return

	// NET
	#pragma HLS INTERFACE axis register port=s_axis_rx_data
	#pragma HLS INTERFACE axis register port=m_axis_tx_data

	// S(R)Q
	#pragma HLS INTERFACE axis register port=s_axis_sq_meta
	#pragma HLS DATA_PACK variable=s_axis_sq_meta

	// ACKs
	#pragma HLS INTERFACE axis register port=m_axis_rx_ack_meta
	#pragma HLS DATA_PACK variable=m_axis_rx_ack_meta

	// RDMA
	#pragma HLS INTERFACE axis register port=m_axis_mem_write_cmd
	#pragma HLS INTERFACE axis register port=m_axis_mem_read_cmd
	#pragma HLS DATA_PACK variable=m_axis_mem_write_cmd
	#pragma HLS DATA_PACK variable=m_axis_mem_read_cmd
	#pragma HLS INTERFACE axis register port=m_axis_mem_write_data
	#pragma HLS INTERFACE axis register port=s_axis_mem_read_data

	// QP
	#pragma HLS INTERFACE axis register port=s_axis_qp_interface
	#pragma HLS INTERFACE axis register port=s_axis_qp_conn_interface
	#pragma HLS DATA_PACK variable=s_axis_qp_interface
	#pragma HLS DATA_PACK variable=s_axis_qp_conn_interface

	#pragma HLS INTERFACE ap_none register port=local_ip_address

	//DEBUG
#ifdef DBG_IBV
	#pragma HLS INTERFACE axis register port=m_axis_dbg
	#pragma HLS DATA_PACK variable=m_axis_dbg 
#endif 

	#pragma HLS INTERFACE ap_vld port=regCrcDropPkgCount

   rocev2<DATA_WIDTH>(			
	   	s_axis_rx_data,
		m_axis_tx_data,
								
		s_axis_sq_meta,
		m_axis_rx_ack_meta,
								
		m_axis_mem_write_cmd,
		m_axis_mem_read_cmd,
		m_axis_mem_write_data,
		s_axis_mem_read_data,

		s_axis_qp_interface,
		s_axis_qp_conn_interface,
		local_ip_address,

#ifdef DBG_IBV
		m_axis_dbg,
#endif 
		regCrcDropPkgCount,
		regInvalidPsnDropCount,
        regRetransCount,
		regIbvCountRx,
        regIbvCountTx
);
#endif

}
