/************************************************
Copyright (c) 2019, Systems Group, ETH Zurich.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
************************************************/
#pragma once

#include <iostream>
#include "../axi_utils.hpp"
#include "../mem_utils.hpp"
#include "../packet.hpp"
#include "../ipv6/ipv6.hpp"
#include "../udp/udp.hpp"

using namespace hls;

const uint32_t BTH_SIZE = 96;
const uint32_t RETH_SIZE = 128;
const uint32_t AETH_SIZE = 32;
const uint32_t IMMDT_SIZE = 32;
const uint32_t RPCH_SIZE = 192;

const ap_uint<16> RDMA_DEFAULT_PORT = 0x12B7; //4791 --> 0x12B7

#define RETRANS_EN 0

// QP/EE states, page 473
typedef enum {RESET, INIT, READY_RECV, READY_SEND, SQ_ERROR, ERROR} qpState;

typedef enum {AETH, RETH, RAW} pkgType;
typedef enum {SHIFT_AETH, SHIFT_RETH, SHIFT_NONE} pkgShiftType;

//See page 246
typedef enum {
	RC_RDMA_WRITE_FIRST = 0x06,
	RC_RDMA_WRITE_MIDDLE = 0x07,
	RC_RDMA_WRITE_LAST = 0x08,
	RC_RDMA_WRITE_LAST_WITH_IMD = 0x09,
	RC_RDMA_WRITE_ONLY = 0x0A,
	RC_RDMA_WRITE_ONLY_WIT_IMD = 0x0B,
	RC_RDMA_READ_REQUEST = 0x0C,
	RC_RDMA_READ_RESP_FIRST = 0x0D,
	RC_RDMA_READ_RESP_MIDDLE = 0x0E,
	RC_RDMA_READ_RESP_LAST = 0x0F,
	RC_RDMA_READ_RESP_ONLY = 0x10,
	RC_ACK = 0x11,
	// RPC
	RC_RDMA_RPC_REQUEST = 0x18,
} ibOpCode;

bool checkIfResponse(ibOpCode code);
bool checkIfWrite(ibOpCode code);
bool checkIfAethHeader(ibOpCode code);
bool checkIfRethHeader(ibOpCode code);

/* QP context */
struct qpContext
{
	qpState		newState;
	ap_uint<4>  local_reg;
	ap_uint<24> qp_num;
	ap_uint<24> remote_psn;
	ap_uint<24> local_psn;
	ap_uint<16> r_key;
	ap_uint<48> virtual_address;
};

/* QP connection */
struct ifConnReq
{
	ap_uint<16> qpn;
	ap_uint<24> remote_qpn;
	ap_uint<128> remote_ip_address; //TODO make variable
	ap_uint<16> remote_udp_port; //TODO what is this used for
};

struct readRequest
{
	ap_uint<24> qpn;
	ap_uint<48> vaddr;
	ap_uint<32> dma_length;
	ap_uint<24> psn;
	ap_uint<4>  local_reg;
	ap_uint<1>  host;
	
	readRequest() {}
	readRequest(ap_uint<24> qpn, ap_uint<48> vaddr, ap_uint<32> len, ap_uint<24> psn, ap_uint<4> local_reg)
//		:qpn(qpn), vaddr(vaddr), dma_length(len), psn(psn) {}
		:qpn(qpn), vaddr(vaddr), dma_length(len), psn(psn), local_reg(local_reg), host(1) {}
};

struct fwdPolicy
{
	bool isDrop;
	bool ackOnly;
	fwdPolicy()
		:isDrop(false), ackOnly(false) {}
	fwdPolicy(bool drop, bool ack)
		:isDrop(drop), ackOnly(ack) {}
};

//TODO move to UDP/IP
struct dstTuple
{
	ap_uint<128> their_address;
	ap_uint<16> their_port;
	dstTuple() {}
	dstTuple(ap_uint<128> addr, ap_uint<16> port)
		:their_address(addr), their_port(port) {}
};

/* TX */
struct txPacketInfo
{
	bool isAETH;
	bool hasHeader;
	bool hasPayload;
};

struct txMeta
{
	ibOpCode 	 op_code;
	ap_uint<24>  qpn;
	ap_uint<4>   local_reg;
	ap_uint<1>   host;
	ap_uint<30>  rsrvd;
	ap_uint<192> params;
	txMeta()
		:op_code(RC_RDMA_WRITE_ONLY) {}
	txMeta(ibOpCode op, ap_uint<24> qp, ap_uint<4> local_reg, ap_uint<1> host, ap_uint<30> rsrvd, ap_uint<192> params)
				:op_code(op), qpn(qp), local_reg(local_reg), host(host), rsrvd(rsrvd), params(params) {}
};

struct routedTxMeta
{
	txMeta      data;
	routedTxMeta() {}
	routedTxMeta(ibOpCode op, ap_uint<24> qp, ap_uint<4> local_reg, ap_uint<1> host, ap_uint<30> rsrvd, ap_uint<192> params)
		:data(op, qp, local_reg, host, rsrvd, params) {}
};

/* Internal read */
struct memCmdInternal
{
	ibOpCode op_code;
	ap_uint<16> qpn; //TODO required
	ap_uint<64> addr;
	ap_uint<32> len;
	ap_uint<4>  local_reg;
	ap_uint<1>  host;
	memCmdInternal() {}
	memCmdInternal(ibOpCode op, ap_uint<16> qpn, ap_uint<64> addr, ap_uint<32> len, ap_uint<4> local_reg, ap_uint<1> host)
		: op_code(op), qpn(qpn), addr(addr), len(len), local_reg(local_reg), host(host) {}
};

/* Event */
struct ackEvent
{
	ap_uint<24> qpn;
	ap_uint<24>	psn;
	bool		validPsn;
	bool		isNak;
	ackEvent() {}
	ackEvent(ap_uint<24> qpn)
		:qpn(qpn), psn(0), validPsn(false), isNak(false) {}
	ackEvent(ap_uint<24> qpn, bool nak)
			:qpn(qpn), psn(0), validPsn(false), isNak(nak) {}
	ackEvent(ap_uint<24> qp, ap_uint<24> psn, bool nak)
			:qpn(qp), psn(psn), validPsn(true), isNak(nak) {}
};

//TODO create readEvent
//TODO event for writes addr + len, no psn
struct event
{
	ibOpCode 	op_code;
	ap_uint<24> qpn;
	ap_uint<48> addr;
	ap_uint<32> length;
	ap_uint<24>	psn;
	bool		validPsn;
	bool		isNak;
	event()
		:op_code(RC_ACK), validPsn(false), isNak(false) {}
	event(ibOpCode op, ap_uint<24> qp)
		:op_code(op), qpn(qp), validPsn(false), isNak(false) {}
	event(ackEvent& aev)
		:op_code(RC_ACK), qpn(aev.qpn), psn(aev.psn), validPsn(aev.validPsn), isNak(aev.isNak) {}
	/*event(ibOpCode op, ap_uint<24> qp, ap_uint<24> psn, bool nak)
		:op_code(op), qpn(qp), psn(psn), validPsn(true), isNak(nak) {}*/
	event(ibOpCode op, ap_uint<24> qp, ap_uint<48> addr, ap_uint<32> len)
		:op_code(op), qpn(qp), addr(addr), length(len), psn(0), validPsn(false), isNak(false) {}
	event(ibOpCode op, ap_uint<24> qp, ap_uint<32> len, ap_uint<24> psn)
		:op_code(op), qpn(qp), addr(0), length(len), psn(psn), validPsn(true), isNak(false) {}
	event(ibOpCode op, ap_uint<24> qp, ap_uint<48> addr, ap_uint<32> len, ap_uint<24> psn)
		:op_code(op), qpn(qp), addr(addr), length(len), psn(psn), validPsn(true), isNak(false) {}
};

/* Pakage info */
struct pkgSplitType
{
	ibOpCode op_code;
	pkgSplitType() {}
	pkgSplitType(ibOpCode op)
			:op_code(op) {}
};

struct pkgInfo
{
	pkgType		type;
	ap_uint<29> words;
	pkgInfo() {}
	pkgInfo(pkgType t, ap_uint<29> words)
		:type(t), words(words) {}
};

struct retransEvent
{
	ibOpCode 	op_code;
	ap_uint<24> qpn;
	ap_uint<48> localAddr;
	ap_uint<48> remoteAddr;
	ap_uint<32> length;
	ap_uint<24>	psn;
	bool		validPsn;  //TODO remove?
	bool		isNak; //TODO remove?
	retransEvent()
		:op_code(RC_ACK), validPsn(false), isNak(false) {}
	retransEvent(ibOpCode op, ap_uint<24> qp, ap_uint<48> laddr, ap_uint<48> raddr, ap_uint<32> len, ap_uint<24> psn)
		:op_code(op), qpn(qp), localAddr(laddr), remoteAddr(raddr),length(len), psn(psn), validPsn(true), isNak(false) {}
};

struct memMeta
{
	bool write;
	ap_uint<48> address;
	ap_uint<32> length;
	memMeta() {}
	memMeta(bool w, ap_uint<48> addr, ap_uint<32> len)
		:write(w), address(addr), length(len) {}
};


/**
 * BaseTransportHeader
 * header(7, 0) = op_code; // OpCode (8bit), IBA packet type, which extension headers follow
 * header[8] = solicited_event;
 * header[9] = mig_req;
 * header(11,10) = pad_count;
 * header(15, 12) = t_ver;
 * header(31, 16) = partition_key;
 * header[32] = fr;
 * header[33] = br;
 * header(39, 34) = reserved_var;
 * header(63, 40) = reverse(dest_qp);
 * header[71] = ack_req;
 * header(70, 64) = reserved;
 * header(95, 72) = reverse(psn);
 */

template <int W>
class BaseTransportHeader : public packetHeader<W, BTH_SIZE> {
	using packetHeader<W, BTH_SIZE>::header;

public:
	BaseTransportHeader() {} //TODO set initialize some fields

	void setOpCode(ibOpCode opcode)
	{
		header(7, 0) =  (uint8_t) opcode;
	}
	ibOpCode getOpCode()
	{
		return ibOpCode((uint16_t) header(7, 0));
	}
	void setPartitionKey(ap_uint<16> key)
	{
		header(31, 16) = key;
	}
	ap_uint<16> getPartitionKey()
	{
		return header(31, 16);
	}
	void setPsn(ap_uint<24> psn)
	{
		header(95, 72) = reverse(psn);
	}
	ap_uint<24> getPsn()
	{
		return reverse((ap_uint<24>) header(95, 72));
	}
	void setDstQP(ap_uint<24> qp)
	{
		header(63, 40) = reverse(qp);
	}
	ap_uint<24> getDstQP()
	{
		return reverse((ap_uint<24>) header(63, 40));
	}
	void setAckReq(bool bit)
	{
		header[71] = bit;
	}
};

struct ibhMeta
{
	ibOpCode op_code;
	ap_uint<16> partition_key;
	ap_uint<24> dest_qp;
	ap_uint<24> psn;
	ap_uint<4>  local_reg;
	bool		validPSN;
	ap_uint<22> numPkg; //TODO does not really fit here //TODO how many bits does this need?
	ibhMeta()
		:op_code(RC_ACK) {} //TODO
	ibhMeta(ibOpCode op, ap_uint<16> key, ap_uint<24> qp)
			:op_code(op), partition_key(key), dest_qp(qp), psn(0), validPSN(false), numPkg(1) {}
	ibhMeta(ibOpCode op, ap_uint<16> key, ap_uint<24> qp, ap_uint<22> numPkg)
			:op_code(op), partition_key(key), dest_qp(qp), psn(0), validPSN(false), numPkg(numPkg) {}
	ibhMeta(ibOpCode op, ap_uint<16> key, ap_uint<24> qp, ap_uint<24> psn, ap_uint<4> local_reg, bool vp)
			:op_code(op), partition_key(key), dest_qp(qp), psn(psn), local_reg(local_reg), validPSN(vp), numPkg(1) {}
};

struct exhMeta
{
	bool		isNak;
	ap_uint<22> numPkg; //TODO how many bits does this need?
	exhMeta() {}
	exhMeta(bool isNak)
		:isNak(isNak), numPkg(1) {}
	exhMeta(bool isNak, ap_uint<22> numPkg)
		:isNak(isNak), numPkg(numPkg) {}
};

/**
 *  see page 167
 *	ap_uint<64> virtual_address;
 *	ap_uint<32> r_key;
 *	ap_uint<32> dma_len;
 */
template <int W>
class RdmaExHeader : public packetHeader<W, RETH_SIZE>//RETH
{
	using packetHeader<W, RETH_SIZE>::header;

public:
	RdmaExHeader() {}

	void setVirtualAddress(ap_uint<64> addr) //TODO & or not??
	{
		header(63, 0) = reverse(addr); //TODO or reverseByte
	}
	ap_uint<64> getVirtualAddress()
	{
		return reverse((ap_uint<64>) header(63, 0));
	}
	void setRemoteKey(ap_uint<32> key)
	{
		header(95, 64) = reverse(key);
	}
	ap_uint<32> getRemoteKey()
	{
		return reverse((ap_uint<32>) header(95,64));
	}
	void setLength(ap_uint<32> len)
	{
		header(127,96) = reverse(len);
	}
	ap_uint<32> getLength()
	{
		return reverse((ap_uint<32>) header(127,96));
	}
};

/**
 * AETH, see page 254, page format on 337, NAK codes page 338
 * [7:0] syndrome, indicates if this is an ACK or NAK
 * [31:8] msn, number of last message completed
 */
template <int W>
class AckExHeader : public packetHeader<W, AETH_SIZE> //AETH
{
	using packetHeader<W, AETH_SIZE>::header;

public:
	AckExHeader() {}

	void setSyndrome(ap_uint<8> syn)
	{
		header(7, 0) = syn;
	}
	ap_uint<8> getSyndrome()
	{
		return ((ap_uint<8>) header(7, 0));
	}
	void setMsn(ap_uint<24> msn)
	{
		header(31, 8) = reverse(msn);
	}
	ap_uint<24> getMsn()
	{
		return reverse((ap_uint<32>) header(31, 8));
	}
	bool isNAK()
	{
		return (header(6,5) == 0x3);
	}
};

/**
 *  Custom RPC Header
 *  ap_uint<192> params;
 */
template <int W>
class RdmaRpcHeader : public packetHeader<W, RPCH_SIZE>//RCTH
{
	using packetHeader<W, RPCH_SIZE>::header;

public:
	RdmaRpcHeader() {}

	void setParams(ap_uint<192> params) //TODO & or not??
	{
		header(191, 0) = reverse(params); //TODO or reverseByte
	}
	ap_uint<192> getParams()
	{
		return reverse((ap_uint<192>) header(191, 0));
	}
};

template <int W>
class ExHeader: public packetHeader<W, RPCH_SIZE>
{
	using packetHeader<W, RPCH_SIZE>::header;

public:
	ExHeader() {}

	ExHeader(RdmaExHeader<W>& h)
	{
		header = h.getRawHeader();
	}
	ExHeader(AckExHeader<W>& h)
	{
		header = h.getRawHeader();
	}
	ExHeader(RdmaRpcHeader<W>& h)
	{
		header = h.getRawHeader();
	}
	RdmaExHeader<W> getRdmaHeader()
	{
		RdmaExHeader<W> rethHeader;
		rethHeader.setRawHeader(header);
		return rethHeader;
	}

	AckExHeader<W> getAckHeader()
	{
		AckExHeader<W> aethHeader;
		aethHeader.setRawHeader(header);
		return aethHeader;
	}
	RdmaRpcHeader<W> getRpcHeader()
	{
		RdmaRpcHeader<W> pcHeader;
		pcHeader.setRawHeader(header);
		return pcHeader;
	}
};

struct ImmDt
{
	ap_uint<32> immdt;
};

struct InvalidateExHeader //IETH
{
	ap_uint<32> r_key;
};

template <int WIDTH>
void ib_transport_protocol(	// RX - net module
							hls::stream<ipUdpMeta>&	s_axis_rx_meta,
							hls::stream<net_axis<WIDTH> >&	s_axis_rx_data,

							// TX - net module
							hls::stream<ipUdpMeta>&	m_axis_tx_meta,
							hls::stream<net_axis<WIDTH> >& m_axis_tx_data,

							// RDMA command
							hls::stream<txMeta>& s_axis_tx_meta,

							// RPC
							hls::stream<routedTxMeta>& m_axis_rx_rpc_params,

							// Memory
							hls::stream<routedMemCmd>& m_axis_mem_write_cmd,
							hls::stream<routedMemCmd>& m_axis_mem_read_cmd,
							hls::stream<net_axis<WIDTH> >& m_axis_mem_write_data,
							hls::stream<net_axis<WIDTH> >& s_axis_mem_read_data,

							// QP intf
							hls::stream<qpContext>&	s_axis_qp_interface,
							hls::stream<ifConnReq>&	s_axis_qp_conn_interface,

							// Debug
							ap_uint<32>& regInvalidPsnDropCount);