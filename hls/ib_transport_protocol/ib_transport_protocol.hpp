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
#include "../packet.hpp"
#include "../ipv6/ipv6.hpp"
#include "../udp/udp.hpp"
#include "rocev2_config.hpp"

using namespace hls;

//#define DBG_IBV_IBH_PROCESS
#define DBG_IBV_IBH_FSM

#if defined(DBG_IBV_IBH_PROCESS) || defined(DBG_IBV_IBH_FSM)
#define DBG_IBV
#endif

const uint32_t BTH_SIZE = 96;
const uint32_t RETH_SIZE = 128;
const uint32_t AETH_SIZE = 32;
const uint32_t IMMDT_SIZE = 32;

const uint32_t RETRANS_RETRY_CNT = 12;
const uint32_t RETRANS_S1 = 3;
const uint32_t RETRANS_S2 = 3;
const uint32_t RETRANS_S3 = 3;

const ap_uint<16> RDMA_DEFAULT_PORT = 0x12B7; //4791 --> 0x12B7

// QP/EE states, page 473
typedef enum {RESET, INIT, READY_RECV, READY_SEND, SQ_ERROR, ERROR} qpState;

typedef enum {AETH, RETH, RAW, IMMED} pkgType;
typedef enum {SHIFT_AETH, SHIFT_RETH, SHIFT_NONE} pkgShiftType;
typedef enum {PKG_SEND, PKG_WRITE} pkgOper;

typedef enum {
	PKG_NF = 0,
	PKG_F = 1
} pkgCtlType;

typedef enum {
	PKG_INT = 0,
	PKG_HOST = 1
} pkgHostType;

typedef enum {
    WR_ACK = 0,
    RD_ACK = 1
} ackType;

typedef enum {
    NO_SYNC = 0,
    SYNC = 1
} syncType;

typedef enum {
    NO_STRM = 0,
    STRM = 1
} strmType;

//See page 246
typedef enum {
    RC_SEND_FIRST = 0x00,
    RC_SEND_MIDDLE = 0x01,
    RC_SEND_LAST = 0x02,
    RC_SEND_LAST_WITH_IMD = 0x03,
	RC_SEND_ONLY = 0x04,
    RC_SEND_ONLY_WITH_IMD = 0x05,
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
} ibOpCode;

bool checkIfResponse(ibOpCode code);
bool checkIfWrite(ibOpCode code);
bool checkIfAethHeader(ibOpCode code);
bool checkIfRethHeader(ibOpCode code);

/* QP context */
// Was 3 + 3*24 + 16 + 64 = 155 Bit, is now 171 bit (with full 32-bit rkey)
struct qpContext
{
	// I'm not sure which role the order of these fields play in here when receiving values from s_axis_qp_interface
	qpState		newState;
	ap_uint<24> qp_num;
	ap_uint<24> remote_psn;
	ap_uint<24> local_psn;
	// Needs to be 32 Bit according to specification - changed that 
	// ap_uint<32> r_key;
	ap_uint<48> virtual_address;
	ap_uint<32> r_key;
	qpContext() {}
	qpContext(qpState newState, ap_uint<24> qp_num, ap_uint<24> remote_psn, ap_uint<24> local_psn, ap_uint<16> r_key, ap_uint<64> virtual_address)
				:newState(newState), qp_num(qp_num), remote_psn(remote_psn), local_psn(local_psn), r_key(r_key), virtual_address(virtual_address) {}
};

/* QP connection */
struct ifConnReq
{
	ap_uint<16> qpn;
	ap_uint<24> remote_qpn;
	ap_uint<128> remote_ip_address; //TODO make variable
	ap_uint<16> remote_udp_port; //TODO what is this used for
	ifConnReq() {}
	ifConnReq(ap_uint<16> qpn, ap_uint<24> remote_qpn, ap_uint<128> remote_ip_address, ap_uint<16> remote_udp_port)
				:qpn(qpn), remote_qpn(remote_qpn), remote_ip_address(remote_ip_address), remote_udp_port(remote_udp_port) {}
};

struct readRequest
{
	ap_uint<24> qpn;
	ap_uint<64> vaddr;
	ap_uint<32> dma_length;
	ap_uint<24> psn;
	ap_uint<1>  host;
	
	readRequest() {}
	readRequest(ap_uint<24> qpn, ap_uint<64> vaddr, ap_uint<32> len, ap_uint<24> psn)
//		:qpn(qpn), vaddr(vaddr), dma_length(len), psn(psn) {}
		:qpn(qpn), vaddr(vaddr), dma_length(len), psn(psn), host(1) {}
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

/* Internal read */
struct memCmdInternal
{
	ibOpCode op_code;
	ap_uint<16> qpn; //TODO required
	ap_uint<64> addr;
	ap_uint<32> len;
	ap_uint<1>  host;
    ap_uint<1>  sync;
    ap_uint<4>  offs;
	memCmdInternal() {}
	memCmdInternal(ibOpCode op, ap_uint<16> qpn, ap_uint<64> addr, ap_uint<32> len, ap_uint<1> host)
		: op_code(op), qpn(qpn), addr(addr), len(len), host(host), sync(0), offs(0) {}
    memCmdInternal(ibOpCode op, ap_uint<16> qpn, ap_uint<64> addr, ap_uint<32> len, ap_uint<1> host, ap_uint<4> offs)
		: op_code(op), qpn(qpn), addr(addr), len(len), host(host), sync(1), offs(offs) {}
    // TODO: need to set some default value?
    memCmdInternal(ap_uint<16> qpn, ap_uint<64> addr, ap_uint<32> len)
        : qpn(qpn), addr(addr), len(len), host(1), sync(0) {}
};

/* Mem command */
struct memCmd
{
	ap_uint<48> addr;
	ap_uint<28> len;
	ap_uint<1>  ctl;
    ap_uint<1>  strm;
    ap_uint<1>  sync;
	ap_uint<1>  host;
    ap_uint<4>  tdst;
	ap_uint<6>  pid;
	ap_uint<4>  vfid; 
	memCmd() {}

	memCmd(ap_uint<64> addr, ap_uint<28> len, ap_uint<1> ctl, ap_uint<1> host, ap_uint<6> pid, ap_uint<4> vfid)
		:addr(addr(47,0)), len(len), ctl(ctl), strm(addr(52,52)), sync(0), host(host), tdst(addr(51,48)), pid(pid), vfid(vfid) {}
	memCmd(ap_uint<64> addr, ap_uint<28> len, ap_uint<1> ctl, ap_uint<1> host, ap_uint<24> qpn)
        :addr(addr(47,0)), len(len), ctl(ctl), strm(addr(52,52)), sync(0), host(host), tdst(addr(51,48)), pid(qpn(5,0)), vfid(qpn(9,6)) {}

    memCmd(ap_uint<64> addr, ap_uint<28> len, ap_uint<1> ctl, ap_uint<1> sync, ap_uint<1> host, ap_uint<4> tdst, ap_uint<6> pid, ap_uint<4> vfid)
		:addr(addr(47,0)), len(len), ctl(ctl), strm(0), sync(sync), host(host), tdst(tdst), pid(pid), vfid(vfid) {}
    memCmd(ap_uint<64> addr, ap_uint<28> len, ap_uint<1> ctl, ap_uint<1> sync, ap_uint<1> host, ap_uint<4> tdst, ap_uint<24> qpn)
		:addr(addr(47,0)), len(len), ctl(ctl), strm(0), sync(sync), host(host), tdst(tdst), pid(qpn(5,0)), vfid(qpn(9,6)) {}
};

struct routedMemCmd
{
	memCmd      data;
	routedMemCmd() {}
	routedMemCmd(ap_uint<64> addr, ap_uint<32> len, ap_uint<1> ctl, ap_uint<1> host, ap_uint<24> qpn)
		:data(addr, len(27,0), ctl, host, qpn(5,0), qpn(9,6)) {}
};


/* TX */
struct txPacketInfo
{
	bool isAETH;
	bool hasHeader;
	bool hasPayload;
};

// Probably rkey should be installed as part of the params in this structure - this comes from externally and carries all other relevant information 
struct txMeta
{
	ibOpCode 	 op_code; // 32
	ap_uint<10>  qpn; // vfid, pid
	ap_uint<1>   host;
	ap_uint<1>   lst;
	ap_uint<4>   offs;
	ap_uint<192> params;
	txMeta()
		:op_code(RC_RDMA_WRITE_ONLY) {}
	txMeta(ibOpCode op, ap_uint<10> qp, ap_uint<1> host, ap_uint<1> lst, ap_uint<4> offs, ap_uint<192> params)
				:op_code(op), qpn(qp), host(host), lst(lst), offs(offs), params(params) {}
};

/* ACK meta */
struct ackMeta 
{
	ap_uint<1> rd;
	ap_uint<10> qpn;
	ap_uint<24> psn;
	ackMeta() {}
	ackMeta(bool rd, ap_uint<10> qpn, ap_uint<24> psn)
		: rd(rd), qpn(qpn), psn(psn) {}
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
	ap_uint<64> addr;
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
	event(ibOpCode op, ap_uint<24> qp, ap_uint<32> len)
		:op_code(op), qpn(qp), addr(0), length(len), psn(0), validPsn(false), isNak(false) {}
	event(ibOpCode op, ap_uint<24> qp, ap_uint<64> addr, ap_uint<32> len)
		:op_code(op), qpn(qp), addr(addr), length(len), psn(0), validPsn(false), isNak(false) {}
	event(ibOpCode op, ap_uint<24> qp, ap_uint<32> len, ap_uint<24> psn)
		:op_code(op), qpn(qp), addr(0), length(len), psn(psn), validPsn(true), isNak(false) {}
	event(ibOpCode op, ap_uint<24> qp, ap_uint<64> addr, ap_uint<32> len, ap_uint<24> psn)
		:op_code(op), qpn(qp), addr(addr), length(len), psn(psn), validPsn(true), isNak(false) {}
};

/* Pakage info */
struct pkgSplit
{
	ibOpCode op_code;
	pkgSplit() {}
	pkgSplit(ibOpCode op)
		:op_code(op) {}
};

struct pkgShift
{
	pkgShiftType type;
	ap_uint<24> qpn;
	pkgShift() {}
	pkgShift(pkgShiftType type, ap_uint<24> qpn) 
		:type(type), qpn(qpn) {}
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
	ap_uint<64> localAddr;
	ap_uint<64> remoteAddr;
	ap_uint<32> length;
	ap_uint<24>	psn;
    ap_uint<1>  lst;
    ap_uint<4>  offs;
	bool		validPsn;  //TODO remove?
	bool		isNak; //TODO remove?
	retransEvent()
		:op_code(RC_ACK), validPsn(false), isNak(false) {}
	retransEvent(ibOpCode op, ap_uint<24> qp, ap_uint<64> laddr, ap_uint<64> raddr, ap_uint<32> len, ap_uint<24> psn, ap_uint<1> lst, ap_uint<4> offs)
		:op_code(op), qpn(qp), localAddr(laddr), remoteAddr(raddr),length(len), psn(psn), lst(lst), offs(offs), validPsn(true), isNak(false) {}
};

struct memMeta
{
	bool write;
	ap_uint<64> address;
	ap_uint<32> length;
	memMeta() {}
	memMeta(bool w, ap_uint<64> addr, ap_uint<32> len)
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
	
	// New setter-methods for the Opcode-Bits - order is somehow reversed compared to the comments given above this function 
	void setSolicitedEvent(bool bit)
	{
		header[15] = bit;
	}
	void setMigReq(bool bit)
	{
		header[14] = bit;
	}
	void setPadCount(ap_uint<2> pad_count)
	{
		header(13, 12) = pad_count;
	}
	void setHeaderVersion(ap_uint<4> header_version)
	{
		header(11, 8) = header_version;
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
	bool		validPSN;
	ap_uint<22> numPkg; //TODO does not really fit here //TODO how many bits does this need?
	ibhMeta()
		:op_code(RC_ACK) {} //TODO
	ibhMeta(ibOpCode op, ap_uint<16> key, ap_uint<24> qp)
			:op_code(op), partition_key(key), dest_qp(qp), psn(0), validPSN(false), numPkg(1) {}
	ibhMeta(ibOpCode op, ap_uint<16> key, ap_uint<24> qp, ap_uint<22> numPkg)
			:op_code(op), partition_key(key), dest_qp(qp), psn(0), validPSN(false), numPkg(numPkg) {}
	ibhMeta(ibOpCode op, ap_uint<16> key, ap_uint<24> qp, ap_uint<24> psn, bool vp)
			:op_code(op), partition_key(key), dest_qp(qp), psn(psn), validPSN(vp), numPkg(1) {}
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
		return reverse((ap_uint<24>) header(31, 8));
	}
	bool isNAK()
	{
		return (header(6,5) == 0x3);
	}
};

template <int W>
class ExHeader: public packetHeader<W, RETH_SIZE>
{
	using packetHeader<W, RETH_SIZE>::header;

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
};

struct ImmDt
{
	ap_uint<32> immdt;
};

struct InvalidateExHeader //IETH
{
	ap_uint<32> r_key;
};

struct psnPkg
{
	ap_uint<4> ctl;
	ap_uint<24> psn;
	ap_uint<24> epsn;
	ibOpCode 	op_code;


	psnPkg(ap_uint<4> ctl, ap_uint<24> psn, ap_uint<24> epsn, ibOpCode op_code) 
		: ctl(ctl), psn(psn), epsn(epsn), op_code(op_code)  {}
};

struct rtrPkg
{
	ap_uint<24> r1;
	ap_uint<24> r2;
	ap_uint<4> ctl;

	rtrPkg(ap_uint<24> r1, ap_uint<24> r2, ap_uint<4> ctl) 
		: r1(r1), r2(r2), ctl(ctl) {}
};

struct tmrPkg
{
	ap_uint<16> qpn;
	ap_uint<4> retries;

	tmrPkg(ap_uint<16> qpn, ap_uint<4> retries) 
		: qpn(qpn), retries(retries) {}
};

template <int WIDTH, int INSTID>
void ib_transport_protocol(	
	// RX - net module
	hls::stream<ipUdpMeta>&	s_axis_rx_meta,
	hls::stream<net_axis<WIDTH> >& s_axis_rx_data,

	// TX - net module
	hls::stream<ipUdpMeta>&	m_axis_tx_meta,
	hls::stream<net_axis<WIDTH> >& m_axis_tx_data,

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

	// Debug
#ifdef DBG_IBV
	hls::stream<psnPkg>& m_axis_dbg_0, 
#endif
	ap_uint<32>& regInvalidPsnDropCount,
    ap_uint<32>& regRetransCount,
	ap_uint<32>& regIbvCountRx,
    ap_uint<32>& regIbvCountTx
);
