/************************************************
Copyright (c) 2016, Xilinx, Inc.
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
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.// Copyright (c) 2015 Xilinx, Inc.
************************************************/
#ifndef TOE_H_
#define TOE_H_

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>
#include <vector>

static const uint16_t MAX_SESSIONS = 10000;

//#include "session_lookup_controller/session_lookup_controller.hpp"

#define noOfTxSessions 1 // Number of Tx Sessions to open for testing
extern uint32_t packetCounter;
extern uint32_t cycleCounter;
extern unsigned int	simCycleCounter;
// Forward declarations.
struct rtlSessionUpdateRequest;
struct rtlSessionUpdateReply;
struct rtlSessionLookupReply;
struct rtlSessionLookupRequest;


#define OOO_N 4 	// number of OOO blocks accepted
#define OOO_W 4288	// window {max(offset + length)} of sequence numbers beyond recvd accepted

static const ap_uint<16> MMS=536;
static const ap_uint<16> MY_MSS=576;

// OOO Parameters
//static const int OOO_N = 4;		// number of OOO blocks accepted
//static const int OOO_W = 4288;	// window {max(offset + length)} of sequence numbers beyond recvd accepted
static const int OOO_N_BITS = 3;		// bits required to represent OOO_N+1, need 0 to show no OOO blocks are valid
static const int OOO_W_BITS = 13;		// bits required to represent OOO_W
static const int OOO_L_BITS = 13;		// max length in bits of OOO blocks allowed

//static const int OOO_N_BITS = ceil(log10(OOO_N+1)/log10(2));		// (3) bits required to represent OOO_N
//static const int OOO_W_BITS = ceil(log10(OOO_W)  /log10(2));		// (13) bits required to represent OOO_W

static const ap_uint<32> SEQ_mid = 2147483648; // used in Modulo Arithmetic Comparison 2^(32-1) of sequence numbers etc.



using namespace hls;

#ifndef __SYNTHESIS__
static const ap_uint<32> TIME_1ms		= 1;
static const ap_uint<32> TIME_5ms		= 1;
static const ap_uint<32> TIME_25ms		= 1;
static const ap_uint<32> TIME_50ms		= 1;
static const ap_uint<32> TIME_100ms		= 1;
static const ap_uint<32> TIME_250ms		= 1;
static const ap_uint<32> TIME_1s		= 1;
static const ap_uint<32> TIME_5s		= 1;
static const ap_uint<32> TIME_7s		= 2;
static const ap_uint<32> TIME_10s		= 3;
static const ap_uint<32> TIME_15s		= 4;
static const ap_uint<32> TIME_20s		= 5;
static const ap_uint<32> TIME_30s		= 6;
static const ap_uint<32> TIME_60s		= 1;
static const ap_uint<32> TIME_120s		= 120;
#else
static const ap_uint<32> TIME_1ms		= 15;
static const ap_uint<32> TIME_5ms		= 75;
static const ap_uint<32> TIME_25ms		= 375;
static const ap_uint<32> TIME_50ms		= 750;
static const ap_uint<32> TIME_100ms		= 1515;
static const ap_uint<32> TIME_250ms		= 3780;
static const ap_uint<32> TIME_1s		= 15150;
static const ap_uint<32> TIME_5s		= 75075;
static const ap_uint<32> TIME_7s		= 105105;
static const ap_uint<32> TIME_10s		= 150150;
static const ap_uint<32> TIME_15s		= 225225;
static const ap_uint<32> TIME_20s		= 300300;
static const ap_uint<32> TIME_30s		= 450450;
static const ap_uint<32> TIME_60s		= 900900;
static const ap_uint<32> TIME_120s		= 1801801;
#endif

#define BROADCASTCHANNELS 2
enum eventType {TX, RT, ACK, SYN, SYN_ACK, FIN, RST, ACK_NODELAY};

/*
 * There is no explicit LISTEN state
 * CLOSE-WAIT state is not used, since the FIN is sent out immediately after we receive a FIN, the application is simply notified
 * FIN_WAIT_2 is also unused
 * LISTEN is merged into CLOSED
 */
enum sessionState {CLOSED, SYN_SENT, SYN_RECEIVED, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, CLOSING, TIME_WAIT, LAST_ACK};
enum notificationType {PKG, CLOSE, TIME_OUT, RESET};
enum { WORD_0, WORD_1, WORD_2, WORD_3, WORD_4 };


/* (adapted from Linux /net/tcp.h line 292)
* The next routines deal with comparing 32 bit unsigned ints
* and worry about wraparound (automatic with unsigned arithmetic).
*
* These functions are equivalent to the following operators (modulo 2^32)
* before()	<
* !after()	<=
* after()	>
* !before()	>=
*
*/
static inline bool before(ap_uint<32> seq1, ap_uint<32> seq2) {
	return (ap_int<32>)(seq1-seq2) < 0;
}
#define after(seq2, seq1)       before(seq1, seq2)

struct axiWord
{
	ap_uint<64>		data;
	ap_uint<8>		keep;
	ap_uint<1>		last;
	axiWord() {}
	axiWord(ap_uint<64>	 data, ap_uint<8> keep, ap_uint<1> last)
				: data(data), keep(keep), last(last) {}
};

struct fourTuple
{
	ap_uint<32> srcIp;
	ap_uint<32> dstIp;
	ap_uint<16> srcPort;
	ap_uint<16> dstPort;
	fourTuple() {}
	fourTuple(ap_uint<32> srcIp, ap_uint<32> dstIp, ap_uint<16> srcPort, ap_uint<16> dstPort)
			  : srcIp(srcIp), dstIp(dstIp), srcPort(srcPort), dstPort(dstPort) {}
};

inline bool operator < (fourTuple const& lhs, fourTuple const& rhs) {
		return lhs.dstIp < rhs.dstIp || (lhs.dstIp == rhs.dstIp && lhs.srcIp < rhs.srcIp);
	}
struct ipTuple
{
	ap_uint<32>	ip_address;
	ap_uint<16>	ip_port;
};

struct sessionLookupQuery
{
	fourTuple	tuple;
	bool		allowCreation;
	sessionLookupQuery() {}
	sessionLookupQuery(fourTuple tuple, bool allowCreation)
			:tuple(tuple), allowCreation(allowCreation) {}
};

struct sessionLookupReply
{
	ap_uint<16> sessionID;
	bool		hit;
	sessionLookupReply() {}
	sessionLookupReply(ap_uint<16> id, bool hit)
			:sessionID(id), hit(hit) {}
};


struct stateQuery
{
	ap_uint<16> 	sessionID;
	sessionState 	state;
	ap_uint<1>		write;
	stateQuery() {}
	stateQuery(ap_uint<16> id)
			:sessionID(id), state(CLOSED), write(0){}
	stateQuery(ap_uint<16> id, sessionState state, ap_uint<1> write)
			:sessionID(id), state(state), write(write) {}
};

/** @ingroup rx_sar_table
 *  @ingroup rx_engine
 *  @ingroup tx_engine
 *  This struct defines an entry of the @ref rx_sar_table
 */
struct rxSarEntry
{
	ap_uint<32> recvd;
	ap_uint<16> appd;
};

struct rxSarRecvd
{
	ap_uint<16> sessionID;
	ap_uint<32> recvd;
	ap_uint<1> write;
	ap_uint<1> init;
	rxSarRecvd() {}
	rxSarRecvd(ap_uint<16> id)
				:sessionID(id), recvd(0), write(0), init(0) {}
	rxSarRecvd(ap_uint<16> id, ap_uint<32> recvd, ap_uint<1> write)
				:sessionID(id), recvd(recvd), write(write), init(0) {}
	rxSarRecvd(ap_uint<16> id, ap_uint<32> recvd, ap_uint<1> write, ap_uint<1> init)
					:sessionID(id), recvd(recvd), write(write), init(init) {}
};

struct rxSarAppd
{
	ap_uint<16> sessionID;
	ap_uint<16> appd;
	// ap_uint<32> recvd; // for comparison with application data request - ensure appd + length < recvd
	ap_uint<1>	write;
	rxSarAppd() {}
	rxSarAppd(ap_uint<16> id)
			:sessionID(id), appd(0), write(0) {}
	rxSarAppd(ap_uint<16> id, ap_uint<16> appd)
			:sessionID(id), appd(appd), write(1) {}
};

/*struct rxSarOffset
{
	ap_uint<16> sessionID;
	ap_uint<16> offset[OOO_N];
	rxSarOffset() {}
	rxSarOffset(ap_uint<16> id, ap_uint<4> index, ap_uint<16> offset)
			:sessionID(id), {}


};

// Original rxSarRecvd, before OOO change
struct rxSarRecvd
{
	ap_uint<16> sessionID;
	ap_uint<32> recvd;
	ap_uint<1> write;
	ap_uint<1> init;
	rxSarRecvd() {}
	rxSarRecvd(ap_uint<16> id)
			:sessionID(id), recvd(0), write(0), init(0) {}
	rxSarRecvd(ap_uint<16> id, ap_uint<32> recvd, ap_uint<1> write)
			:sessionID(id), recvd(recvd), write(write), init(0) {}
	rxSarRecvd(ap_uint<16> id, ap_uint<32> recvd, ap_uint<1> write, ap_uint<1> init)
			:sessionID(id), recvd(recvd), write(write), init(init) {}
};*/

struct txSarEntry
{
	ap_uint<32> ackd;
	ap_uint<32> not_ackd;
	ap_uint<16> recv_window;
	ap_uint<16> cong_window;
	ap_uint<16> slowstart_threshold;
	ap_uint<16> app;
	ap_uint<2>	count;
	bool		finReady;
	bool		finSent;
};

struct rxTxSarQuery
{
	ap_uint<16> sessionID;
	ap_uint<32> ackd;
	ap_uint<16> recv_window;
	ap_uint<16>	cong_window;
	ap_uint<2>  count;
	ap_uint<1> write;
	ap_uint<1>	init;
	rxTxSarQuery () {}
	rxTxSarQuery(ap_uint<16> id)
				:sessionID(id), ackd(0), recv_window(0), count(0), write(0) {}
	rxTxSarQuery(ap_uint<16> id, ap_uint<32> ackd, ap_uint<16> recv_win, ap_uint<16> cong_win, ap_uint<2> count, ap_uint<1> init)
				:sessionID(id), ackd(ackd), recv_window(recv_win), cong_window(cong_win), count(count), write(1), init(init) {}
};

struct txTxSarQuery
{
	ap_uint<16> sessionID;	
	ap_uint<32> not_ackd;
	ap_uint<1>	write;
	ap_uint<1>	init;
	bool		finReady;
	bool		finSent;
	bool		isRtQuery;
	txTxSarQuery() {}
	txTxSarQuery(ap_uint<16> id)
				:sessionID(id), not_ackd(0), write(0), init(0), finReady(false), finSent(false), isRtQuery(false) {}
	//txTxSarQuery(ap_uint<16> id, ap_uint<1> lock)
	//			:sessionID(id), not_ackd(0), write(0), init(0), finReady(false), finSent(false), isRtQuery(false) {}
	txTxSarQuery(ap_uint<16> id, ap_uint<32> not_ackd, ap_uint<1> write)
				:sessionID(id), not_ackd(not_ackd), write(write), init(0), finReady(false), finSent(false), isRtQuery(false) {}
	txTxSarQuery(ap_uint<16> id, ap_uint<32> not_ackd, ap_uint<1> write, ap_uint<1> init)
				:sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(false), finSent(false), isRtQuery(false) {}
	txTxSarQuery(ap_uint<16> id, ap_uint<32> not_ackd, ap_uint<1> write, ap_uint<1> init, bool finReady, bool finSent)
				:sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(false) {}
	txTxSarQuery(ap_uint<16> id, ap_uint<32> not_ackd, ap_uint<1> write, ap_uint<1> init, bool finReady, bool finSent, bool isRt)
				:sessionID(id), not_ackd(not_ackd), write(write), init(init), finReady(finReady), finSent(finSent), isRtQuery(isRt) {}
};

struct txTxSarRtQuery : public txTxSarQuery
{
	txTxSarRtQuery() {}
	txTxSarRtQuery(const txTxSarQuery& q)
			:txTxSarQuery(q.sessionID, q.not_ackd, q.write, q.init, q.finReady, q.finSent, q.isRtQuery) {}
	txTxSarRtQuery(ap_uint<16> id, ap_uint<16> ssthresh)
			:txTxSarQuery(id, ssthresh, 1, 0, false, false, true) {}
	ap_uint<16> getThreshold()
	{
	return not_ackd(15, 0);
	}
};

struct txAppTxSarQuery
{
	ap_uint<16> sessionID;
	//ap_uint<16> ackd;
	ap_uint<16> mempt;
	bool		write;
	txAppTxSarQuery() {}
	txAppTxSarQuery(ap_uint<16> id)
			:sessionID(id), mempt(0), write(false) {}
	txAppTxSarQuery(ap_uint<16> id, ap_uint<16> pt)
			:sessionID(id), mempt(pt), write(true) {}
};

struct rxTxSarReply
{
	ap_uint<32>	prevAck;
	ap_uint<32> nextByte;
	ap_uint<16>	cong_window;
	ap_uint<16> slowstart_threshold;
	ap_uint<2>	count;
	rxTxSarReply() {}
	rxTxSarReply(ap_uint<32> ack, ap_uint<32> next, ap_uint<16> cong_win, ap_uint<16> sstresh, ap_uint<2> count)
			:prevAck(ack), nextByte(next), cong_window(cong_win), slowstart_threshold(sstresh), count(count) {}
};

struct txAppTxSarReply
{
	ap_uint<16> sessionID;
	ap_uint<16> ackd;
	ap_uint<16> mempt;
	txAppTxSarReply() {}
	txAppTxSarReply(ap_uint<16> id, ap_uint<16> ackd, ap_uint<16> pt)
			:sessionID(id), ackd(ackd), mempt(pt) {}
};

struct txAppTxSarPush
{
	ap_uint<16> sessionID;
	ap_uint<16> app;
	txAppTxSarPush() {}
	txAppTxSarPush(ap_uint<16> id, ap_uint<16> app)
			:sessionID(id), app(app) {}
};

struct txSarAckPush
{
	ap_uint<16> sessionID;
	ap_uint<16> ackd;
	ap_uint<1>	init;
	txSarAckPush() {}
	txSarAckPush(ap_uint<16> id, ap_uint<16> ackd)
			:sessionID(id), ackd(ackd), init(0) {}
	txSarAckPush(ap_uint<16> id, ap_uint<16> ackd, ap_uint<1> init)
			:sessionID(id), ackd(ackd), init(init) {}
};

struct txTxSarReply
{
	ap_uint<32> ackd;
	ap_uint<32> not_ackd;
	ap_uint<16> min_window;
	ap_uint<16> app;
	bool		finReady;
	bool		finSent;
	txTxSarReply() {}
	txTxSarReply(ap_uint<32> ack, ap_uint<32> nack, ap_uint<16> min_window, ap_uint<16> app, bool finReady, bool finSent)
		:ackd(ack), not_ackd(nack), min_window(min_window), app(app), finReady(finReady), finSent(finSent) {}
};

struct rxRetransmitTimerUpdate {
	ap_uint<16> sessionID;
	bool		stop;
	rxRetransmitTimerUpdate() {}
	rxRetransmitTimerUpdate(ap_uint<16> id)
			:sessionID(id), stop(0) {}
	rxRetransmitTimerUpdate(ap_uint<16> id, bool stop)
			:sessionID(id), stop(stop) {}
};

struct txRetransmitTimerSet {
	ap_uint<16> sessionID;
	eventType	type;
	txRetransmitTimerSet() {}
	txRetransmitTimerSet(ap_uint<16> id)
			:sessionID(id), type(RT) {} //FIXME??
	txRetransmitTimerSet(ap_uint<16> id, eventType type)
			:sessionID(id), type(type) {}
};

struct event
{
	eventType	type;
	ap_uint<16>	sessionID;
	//eventType	type;
	//ap_uint<16>	sessionID;
	ap_uint<16> address;
	ap_uint<16> length;
	ap_uint<3>	rt_count;
	//bool		retransmit;
	event() {}
	//event(const event&) {}
	event(eventType type, ap_uint<16> id)
			:type(type), sessionID(id), address(0), length(0), rt_count(0) {}
	event(eventType type, ap_uint<16> id, ap_uint<3> rt_count)
			:type(type), sessionID(id), address(0), length(0), rt_count(rt_count) {}
	event(eventType type, ap_uint<16> id, ap_uint<16> addr, ap_uint<16> len)
			:type(type), sessionID(id), address(addr), length(len), rt_count(0) {}
	event(eventType type, ap_uint<16> id, ap_uint<16> addr, ap_uint<16> len, ap_uint<3> rt_count)
			:type(type), sessionID(id), address(addr), length(len), rt_count(rt_count) {}
};

struct extendedEvent : public event
{
	fourTuple	tuple;
	extendedEvent() {}
	extendedEvent(const event& ev)
			:event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
	extendedEvent(const event& ev, fourTuple tuple)
			:event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count), tuple(tuple) {}
};

struct rstEvent : public event
{
	rstEvent() {}
	rstEvent(const event& ev)
		:event(ev.type, ev.sessionID, ev.address, ev.length, ev.rt_count) {}
	rstEvent(ap_uint<32> seq)
			//:event(RST, 0, false), seq(seq) {}
			:event(RST, 0, seq(31, 16), seq(15, 0), 0) {}
	rstEvent(ap_uint<16> id, ap_uint<32> seq)
			:event(RST, id, seq(31, 16), seq(15, 0), 1) {}
			//:event(RST, id, true), seq(seq) {}
	rstEvent(ap_uint<16> id, ap_uint<32> seq, bool hasSessionID)
			:event(RST, id, seq(31, 16), seq(15, 0), hasSessionID) {}
		//:event(RST, id, hasSessionID), seq(seq) {}
	ap_uint<32> getAckNumb()
	{
		ap_uint<32> seq;
		seq(31, 16) = address;
		seq(15, 0) = length;
		return seq;
	}
	bool hasSessionID()
	{
		return (rt_count != 0);
	}
};

struct mmCmd
{
	ap_uint<23>	bbt;
	ap_uint<1>	type;
	ap_uint<6>	dsa;
	ap_uint<1>	eof;
	ap_uint<1>	drr;
	ap_uint<32>	saddr;
	ap_uint<4>	tag;
	ap_uint<4>	rsvd;
	mmCmd() {}
	mmCmd(ap_uint<32> addr, ap_uint<16> len)
			:bbt(len), type(1), dsa(0), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}
	/*mm_cmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<1> last)
			:bbt(len), type(1), dsa(0), eof(last), drr(1), saddr(addr), tag(0), rsvd(0) {}*/
	/*mm_cmd(ap_uint<32> addr, ap_uint<16> len, ap_uint<4> dsa)
			:bbt(len), type(1), dsa(dsa), eof(1), drr(1), saddr(addr), tag(0), rsvd(0) {}*/
};

struct mmStatus
{
	ap_uint<4>	tag;
	ap_uint<1>	interr;
	ap_uint<1>	decerr;
	ap_uint<1>	slverr;
	ap_uint<1>	okay;
};

//TODO is this required??
struct mm_ibtt_status
{
	ap_uint<4>	tag;
	ap_uint<1>	interr;
	ap_uint<1>	decerr;
	ap_uint<1>	slverr;
	ap_uint<1>	okay;
	ap_uint<22>	brc_vd;
	ap_uint<1>	eop;
};

struct openStatus
{
	ap_uint<16>	sessionID;
	bool		success;
	openStatus() {}
	openStatus(ap_uint<16> id, bool success)
		:sessionID(id), success(success) {}
};

struct appNotification
{
	ap_uint<16>			sessionID;
	ap_uint<16>			length;
	ap_uint<32>			ipAddress;
	ap_uint<16>			dstPort;
	bool				closed;
	appNotification() {}
	appNotification(ap_uint<16> id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port)
				:sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(false) {}
	appNotification(ap_uint<16> id, bool closed)
				:sessionID(id), length(0), ipAddress(0),  dstPort(0), closed(closed) {}
	appNotification(ap_uint<16> id, ap_uint<32> addr, ap_uint<16> port, bool closed)
				:sessionID(id), length(0), ipAddress(addr),  dstPort(port), closed(closed) {}
	appNotification(ap_uint<16> id, ap_uint<16> len, ap_uint<32> addr, ap_uint<16> port, bool closed)
			:sessionID(id), length(len), ipAddress(addr), dstPort(port), closed(closed) {}
};


struct appReadRequest
{
	ap_uint<16> sessionID;
	//ap_uint<16> address;
	ap_uint<16> length;
	appReadRequest() {}
	appReadRequest(ap_uint<16> id, ap_uint<16> len)
			:sessionID(id), length(len) {}
};
//ap_uint<8> length2keep_mapping(uint16_t lengthValue);
ap_uint<8> returnKeep(ap_uint<4> length);
ap_uint<16> byteSwap16(ap_uint<16> inputVector);
ap_uint<32> byteSwap32(ap_uint<32> inputVector);
ap_uint<4> keepMapping(ap_uint<8> keepValue);		// This function counts the number of 1s in an 8-bit value

//template<class type> void streamBroadcaster (stream<type> &input, stream<type> &output1, stream<type> &output2);
//template<class type> void streamMerger(stream<type> &input1, stream<type>& input2, stream<type>& output);
template<typename T> void mergeFunction(stream<T>& in1, stream<T>& in2, stream<T>& out);

void toe(	// Data & Memory Interface
			stream<axiWord>&						ipRxData,
			stream<mmStatus>&						rxBufferWriteStatus,
			stream<mmStatus>&						txBufferWriteStatus,
			stream<axiWord>&						rxBufferReadData,
			stream<axiWord>&						txBufferReadData,
			stream<axiWord>&						ipTxData,
			stream<mmCmd>&							rxBufferWriteCmd,
			stream<mmCmd>&							rxBufferReadCmd,
			stream<mmCmd>&							txBufferWriteCmd,
			stream<mmCmd>&							txBufferReadCmd,
			stream<axiWord>&						rxBufferWriteData,
			stream<axiWord>&						txBufferWriteData,
			// SmartCam Interface
			stream<rtlSessionLookupReply>&			sessionLookup_rsp,
			stream<rtlSessionUpdateReply>&			sessionUpdate_rsp,
			//stream<ap_uint<14> >&					readFinSessionId,
			stream<rtlSessionLookupRequest>&		sessionLookup_req,
			stream<rtlSessionUpdateRequest>&		sessionUpdate_req,
			//stream<rtlSessionUpdateRequest>&		sessionInsert_req,
			//stream<rtlSessionUpdateRequest>&		sessionDelete_req,
			//stream<ap_uint<14> >&					writeNewSessionId,
			// Application Interface
			stream<ap_uint<16> >&					listenPortReq,
			// This is disabled for the time being, due to complexity concerns
			//stream<ap_uint<16> >&					appClosePortIn,
			stream<appReadRequest>&					rxDataReq,
			stream<ipTuple>&						openConnReq,
			stream<ap_uint<16> >&					closeConnReq,
			stream<ap_uint<16> >&					txDataReqMeta,
			stream<axiWord>&						txDataReq,

			stream<bool>&							listenPortRsp,
			stream<appNotification>&				notification,
			stream<ap_uint<16> >&					rxDataRspMeta,
			stream<axiWord>&						rxDataRsp,
			stream<openStatus>&						openConnRsp,
			stream<ap_int<17> >&					txDataRsp,
			//IP Address Input
			ap_uint<32>								regIpAddress,
			//statistic
			ap_uint<16>&							relSessionCount,
			ap_uint<16>&							regSessionCount);

#endif
