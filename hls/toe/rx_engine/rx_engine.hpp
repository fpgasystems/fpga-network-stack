/*
 * Copyright (c) 2019, Systems Group, ETH Zurich
 * Copyright (c) 2016, Xilinx, Inc.
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
#include "../toe.hpp"
#include "../toe_internals.hpp"

struct optionalFieldsMeta
{
	ap_uint<4>	dataOffset;
	ap_uint<1>	syn;
	optionalFieldsMeta() {}
	optionalFieldsMeta(ap_uint<4> offset, ap_uint<1> syn)
		:dataOffset(offset), syn(syn) {}
};

struct pseudoMeta
{
	ap_uint<32> their_address;
	ap_uint<32> our_address;
	ap_uint<16> length;
	pseudoMeta() {}
	pseudoMeta(ap_uint<32> src_addr, ap_uint<32> dst_addr, ap_uint<16> len)
		:their_address(src_addr), our_address(dst_addr), length(len) {}
};

/** @ingroup rx_engine
 *  @TODO check if same as in Tx engine
 */
struct rxEngineMetaData
{
	//ap_uint<16> sessionID;
	ap_uint<32> seqNumb;
	ap_uint<32> ackNumb;
	ap_uint<16> winSize;
//#if (WINDOW_SCALE)
	ap_uint<4>	winScale;
//#endif
	ap_uint<16> length;
	ap_uint<1>	ack;
	ap_uint<1>	rst;
	ap_uint<1>	syn;
	ap_uint<1>	fin;
	ap_uint<4>	dataOffset;
	//ap_uint<16> dstPort;
};

/** @ingroup rx_engine
 *
 */
struct rxFsmMetaData
{
	ap_uint<16>			sessionID;
	ap_uint<32>			srcIpAddress;
	ap_uint<16>			dstIpPort;
	rxEngineMetaData	meta; //check if all needed
	ap_uint<16> 		srcIpPort;
	rxFsmMetaData() {}
	rxFsmMetaData(ap_uint<16> id, ap_uint<32> ipAddr, ap_uint<16> ipPort, rxEngineMetaData meta, ap_uint<16> srcIpPort)
				:sessionID(id), srcIpAddress(ipAddr), dstIpPort(ipPort), meta(meta), srcIpPort(srcIpPort) {}
};

/** @defgroup rx_engine RX Engine
 *  @ingroup tcp_module
 *  RX Engine
 */
template <int WIDTH>
void rx_engine(	stream<net_axis<WIDTH> >&					ipRxData,
				stream<sessionLookupReply>&			sLookup2rxEng_rsp,
				stream<sessionState>&				stateTable2rxEng_upd_rsp,
				stream<bool>&						portTable2rxEng_rsp,
				stream<rxSarEntry>&					rxSar2rxEng_upd_rsp,
				stream<rxTxSarReply>&				txSar2rxEng_upd_rsp,
#if !(RX_DDR_BYPASS)
				stream<mmStatus>&					rxBufferWriteStatus,
#endif
				stream<net_axis<WIDTH> >&					rxBufferWriteData,
				stream<sessionLookupQuery>&			rxEng2sLookup_req,
				stream<stateQuery>&					rxEng2stateTable_upd_req,
				stream<ap_uint<16> >&				rxEng2portTable_req,
				stream<rxSarRecvd>&					rxEng2rxSar_upd_req,
				stream<rxTxSarQuery>&				rxEng2txSar_upd_req,
				stream<rxRetransmitTimerUpdate>&	rxEng2timer_clearRetransmitTimer,
				stream<ap_uint<16> >&				rxEng2timer_clearProbeTimer,
				stream<ap_uint<16> >&				rxEng2timer_setCloseTimer,
				stream<openStatus>&					openConStatusOut, //TODO remove
				stream<extendedEvent>&				rxEng2eventEng_setEvent,
#if !(RX_DDR_BYPASS)
				stream<mmCmd>&						rxBufferWriteCmd,
				stream<appNotification>&			rxEng2rxApp_notification);
#else
				stream<appNotification>&			rxEng2rxApp_notification,
				ap_uint<16>					rxbuffer_data_count,
				ap_uint<16>					rxbuffer_max_data_count);
#endif
