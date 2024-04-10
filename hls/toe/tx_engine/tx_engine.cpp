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

#include "toe_config.hpp"
#include "tx_engine.hpp"
#include "../../ipv4/ipv4.hpp"
#include "../two_complement_subchecksums.hpp"


/** @ingroup tx_engine
 *  @name metaLoader
 *  The metaLoader reads the Events from the EventEngine then it loads all the necessary MetaData from the data
 *  structures (RX & TX Sar Table). Depending on the Event type it generates the necessary MetaData for the
 *  ipHeaderConstruction and the pseudoHeaderConstruction.
 *  Additionally it requests the IP Tuples from the Session. In some special cases the IP Tuple is delivered directly
 *  from @ref rx_engine and does not have to be loaded from the Session Table. The isLookUpFifo indicates this special cases.
 *  Lookup Table for the current session.
 *  Depending on the Event Type the retransmit or/and probe Timer is set.
 *  @param[in]		eventEng2txEng_event
 *  @param[in]		rxSar2txEng_upd_rsp
 *  @param[in]		txSar2txEng_upd_rsp
 *  @param[out]		txEng2rxSar_upd_req
 *  @param[out]		txEng2txSar_upd_req
 *  @param[out]		txEng2timer_setRetransmitTimer
 *  @param[out]		txEng2timer_setProbeTimer
 *  @param[out]		txEng_ipMetaFifoOut
 *  @param[out]		txEng_tcpMetaFifoOut
 *  @param[out]		txBufferReadCmd
 *  @param[out]		txEng2sLookup_rev_req
 *  @param[out]		txEng_isLookUpFifoOut
 *  @param[out]		txEng_tupleShortCutFifoOut
 */
void metaLoader(hls::stream<extendedEvent>&				eventEng2txEng_event,
				hls::stream<rxSarReply>&				rxSar2txEng_rsp,
				hls::stream<txTxSarReply>&				txSar2txEng_upd_rsp,
				hls::stream<ap_uint<16> >&				txEng2rxSar_req,
				hls::stream<txTxSarQuery>&				txEng2txSar_upd_req,
				hls::stream<txRetransmitTimerSet>&		txEng2timer_setRetransmitTimer,
				hls::stream<ap_uint<16> >&				txEng2timer_setProbeTimer,
				hls::stream<ap_uint<16> >&				txEng_ipMetaFifoOut,
				hls::stream<tx_engine_meta>&			txEng_tcpMetaFifoOut,
				hls::stream<mmCmd>&						txBufferReadCmd,
				hls::stream<ap_uint<16> >&				txEng2sLookup_rev_req,
				hls::stream<bool>&						txEng_isLookUpFifoOut,
#if (TCP_NODELAY)
				hls::stream<bool>&						txEng_isDDRbypass,
#endif
				hls::stream<fourTuple>&					txEng_tupleShortCutFifoOut,
				hls::stream<ap_uint<1> >&				readCountFifo)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	static ap_uint<1> ml_FsmState = 0;
	static bool ml_sarLoaded = false;
	static extendedEvent ml_curEvent;
	static ap_uint<32> ml_randomValue= 0x562301af; //Random seed initialization

	static ap_uint<2> ml_segmentCount = 0;
	static rxSarReply	rxSar;
	static txTxSarReply	txSarReg;
	ap_uint<WINDOW_BITS> slowstart_threshold;
	static tx_engine_meta meta;
	rstEvent resetEvent;

	switch (ml_FsmState)
	{
	case 0:
		if (!eventEng2txEng_event.empty())
		{
			eventEng2txEng_event.read(ml_curEvent);
			readCountFifo.write(1);
			ml_sarLoaded = false;
			//NOT necessary for SYN/SYN_ACK only needs one
			switch (ml_curEvent.type)
			{
			case RT:
				txEng2rxSar_req.write(ml_curEvent.sessionID);
				txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				break;
			case TX:
				txEng2rxSar_req.write(ml_curEvent.sessionID);
				txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				break;
			case SYN_ACK:
				txEng2rxSar_req.write(ml_curEvent.sessionID);
				txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				break;
			case FIN:
				txEng2rxSar_req.write(ml_curEvent.sessionID);
				txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				break;
			case RST:
				// Get txSar for SEQ numb
				resetEvent = ml_curEvent;
				if (resetEvent.hasSessionID())
				{
					txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				}
				break;
			case ACK_NODELAY:
			case ACK:
				txEng2rxSar_req.write(ml_curEvent.sessionID);
				txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				break;
			case SYN:
				if (ml_curEvent.rt_count != 0)
				{
					txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID));
				}
				break;
			default:
				break;
			}
			ml_FsmState = 1;
			ml_randomValue++; //make sure it doesn't become zero TODO move this out of if, but breaks my testsuite
		} //if not empty
		ml_segmentCount = 0;
		break;
	case 1:
		switch(ml_curEvent.type)
		{
		// When Nagle's algorithm disabled
		// Can bypass DDR
#if (TCP_NODELAY)
		case TX:
			if ((!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty()))// || ml_sarLoaded)
			{
				rxSar2txEng_rsp.read(rxSar);
				txTxSarReply txSar = txSar2txEng_upd_rsp.read();

				meta.ackNumb = rxSar.recvd;
				meta.seqNumb = txSar.not_ackd;
				meta.window_size = rxSar.windowSize; //Precalcualted in rx_sar_table ((rxSar.appd - rxSar.recvd) - 1)
				meta.ack = 1; // ACK is always set when established
				meta.rst = 0;
				meta.syn = 0;
				meta.fin = 0;
				meta.length = ml_curEvent.length;

				//this is hack, makes sure that probeTimer is never set.
				//ProbeTimer is not used, since application checks space before transmitting
				if (0x7FFF < ml_curEvent.length) // Ox7FFF = 32767 
				{
					txEng2timer_setProbeTimer.write(ml_curEvent.sessionID);
				}

				//TODO some checking
				txSar.not_ackd += ml_curEvent.length;

				txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1));
				//This state is always left
				ml_FsmState = 0;


				// Send a packet only if there is data or we want to send an empty probing message
				if (meta.length != 0)// || ml_curEvent.retransmit) //TODO retransmit boolean currently not set, should be removed
				{
					txEng_ipMetaFifoOut.write(meta.length);
					txEng_tcpMetaFifoOut.write(meta);
					txEng_isLookUpFifoOut.write(true);
					txEng_isDDRbypass.write(true);
					txEng2sLookup_rev_req.write(ml_curEvent.sessionID);

					// Only set RT timer if we actually send sth, TODO only set if we change state and sent sth
					txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID));
				}

				std::cout<<"TX";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;		
			}

			break;
#else
		case TX:
			// Sends everyting between txSar.not_ackd and txSar.app
			if ((!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty()) || ml_sarLoaded)
			{
				if (!ml_sarLoaded)
				{
					rxSar2txEng_rsp.read(rxSar);
					txSar2txEng_upd_rsp.read(txSar);
				}

				//Compute our space, Advertise at least a quarter/half, otherwise 0
				meta.ackNumb = rxSar.recvd;
				meta.seqNumb = txSar.not_ackd;
				meta.window_size = rxSar.windowSize; //Precalcualted in rx_sar_table ((rxSar.appd - rxSar.recvd) - 1)
				meta.ack = 1; // ACK is always set when established
				meta.rst = 0;
				meta.syn = 0;
				meta.fin = 0;
				meta.length = 0;

				currLength = (txSar.app - ((ap_uint<WINDOW_BITS>)txSar.not_ackd));
				// Construct address before modifying txSar.not_ackd
				ap_uint<32> pkgAddr;

				//pkgAddr(31, 30) = 0x01;
				pkgAddr(31, 30) = 0x00; //TODO: this is combined with addr offset calculation in mem_inf to separate rx and tx
				pkgAddr(29, WINDOW_BITS) = ml_curEvent.sessionID(13, 0);
				pkgAddr(WINDOW_BITS-1, 0) = txSar.not_ackd(WINDOW_BITS-1, 0); //ml_curEvent.address;


				// Check length, if bigger than Usable Window or MMS
				//precomputed in txSar Table: usableWindow = (min(recv_window, cong_windwo) < usedLength ? (min(recv_window, cong_window) - usedLength) : 0)
				if (currLength <= txSar.usableWindow)
				{
					if (currLength >= MSS) //TODO change to >= MSS, use maxSegmentCount
					{
						// We stay in this state and sent immediately another packet
						txSar.not_ackd += MSS;
						meta.length = MSS;
					}
					else
					{
						// If we sent all data, there might be a fin we have to sent too
						if (txSar.finReady && (txSar.ackd == txSar.not_ackd || currLength == 0))
						{
							ml_curEvent.type = FIN;
						}
						else
						{
							ml_FsmState = 0;
						}
						// Check if small segment and if unacknowledged data in pipe (Nagle)
//#if !(TCP_NODELAY)
						if (txSar.ackd == txSar.not_ackd)
//#endif
						{
							txSar.not_ackd += currLength;
							meta.length = currLength;
						}
//#if !(TCP_NODELAY)
						else
						{
							txEng2timer_setProbeTimer.write(ml_curEvent.sessionID);
						}
//#endif
						// Write back txSar not_ackd pointer
						txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1));
					}
				}
				else
				{
					// code duplication, but better timing..
					if (txSar.usableWindow >= MSS)
					{
						// We stay in this state and sent immediately another packet
						txSar.not_ackd += MSS;
						meta.length = MSS;
					}
					else
					{
						// Check if we sent >= MSS data
//#if !(TCP_NODELAY)
						if (txSar.ackd == txSar.not_ackd)
//#endif
						{
							txSar.not_ackd += txSar.usableWindow;
							meta.length = txSar.usableWindow;
						}
						// Set probe Timer to try again later
						txEng2timer_setProbeTimer.write(ml_curEvent.sessionID);
						txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1));
						ml_FsmState = 0;
					}
				}

				if (meta.length != 0)
				{
					txBufferReadCmd.write(mmCmd(pkgAddr, meta.length));
				}
				// Send a packet only if there is data or we want to send an empty probing message
				if (meta.length != 0)// || ml_curEvent.retransmit) //TODO retransmit boolean currently not set, should be removed
				{
					txEng_ipMetaFifoOut.write(meta.length);
					txEng_tcpMetaFifoOut.write(meta);
					txEng_isLookUpFifoOut.write(true);
					txEng2sLookup_rev_req.write(ml_curEvent.sessionID);

					// Only set RT timer if we actually send sth, TODO only set if we change state and sent sth
					txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID));
				}//TODO if probe send msg length 1
				ml_sarLoaded = true;

				std::cout<<"TX";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;	
			}
			break;
#endif
		case RT:
			if ((!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty()) || ml_sarLoaded)
			{
				txTxSarReply txSar;
				if (!ml_sarLoaded)
				{
					rxSar2txEng_rsp.read(rxSar);
					txSar = txSar2txEng_upd_rsp.read();
				}
				else
				{
					txSar = txSarReg;
				}

				//TODO use  usedLengthWithFIN
				ap_uint<WINDOW_BITS> currLength = txSar.usedLength; //((ap_uint<WINDOW_BITS>) txSar.not_ackd - txSar.ackd);

				meta.ackNumb = rxSar.recvd;
				meta.seqNumb = txSar.ackd;
				meta.window_size = rxSar.windowSize; //Precalcualted in rx_sar_table ((rxSar.appd - rxSar.recvd) - 1)
				meta.ack = 1; // ACK is always set when session is established
				meta.rst = 0;
				meta.syn = 0;
				meta.fin = 0;

				// Construct address before modifying txSar.ackd
				ap_uint<32> pkgAddr;
				pkgAddr(31, 30) = 0x00; //TODO: this is combined with addr offset calculation in mem_inf to separate rx and tx
				pkgAddr(29, WINDOW_BITS) = ml_curEvent.sessionID(13, 0);
				pkgAddr(WINDOW_BITS-1, 0) = txSar.ackd(WINDOW_BITS-1, 0); //ml_curEvent.address;

				// Decrease Slow Start Threshold, only on first RT from retransmitTimer
				if (!ml_sarLoaded && (ml_curEvent.rt_count == 1))
				{
					if (currLength > (4*MSS)) // max( FlightSize/2, 2*MSS) RFC:5681
					{
						slowstart_threshold = currLength/2;
					}
					else
					{
						slowstart_threshold = (2 * MSS);
					}
					txEng2txSar_upd_req.write(txTxSarRtQuery(ml_curEvent.sessionID, slowstart_threshold));
				}


				// Since we are retransmitting from txSar.ackd to txSar.not_ackd, this data is already inside the usableWindow
				// => no check is required
				// Only check if length is bigger than MMS
				if (currLength > MSS)
				{
					// We stay in this state and sent immediately another packet
					meta.length = MSS;
					txSar.ackd += MSS;
					txSar.usedLength -= MSS;
					// TODO replace with dynamic count, remove this
					if (ml_segmentCount == 3)
					{
						// Should set a probe or sth??
						//txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1));
						ml_FsmState = 0;
					}
					ml_segmentCount++;
				}
				else
				{
					meta.length = currLength;
					if (txSar.finSent)
					{
						ml_curEvent.type = FIN;
					}
					else
					{
						// set RT here???
						ml_FsmState = 0;
					}
				}

				// Only send a packet if there is data
				if (meta.length != 0)
				{
					txBufferReadCmd.write(mmCmd(pkgAddr, meta.length));
					txEng_ipMetaFifoOut.write(meta.length);
					txEng_tcpMetaFifoOut.write(meta);
					txEng_isLookUpFifoOut.write(true);
#if (TCP_NODELAY)
					txEng_isDDRbypass.write(false);
#endif
					txEng2sLookup_rev_req.write(ml_curEvent.sessionID);

					// Only set RT timer if we actually send sth
					txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID));
				}
				ml_sarLoaded = true;
				txSarReg = txSar;

				std::cout<<"RT";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;	
			}
			break;
		case ACK:
		case ACK_NODELAY:
			if (!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty())
			{
				rxSar2txEng_rsp.read(rxSar);
				txTxSarReply txSar = txSar2txEng_upd_rsp.read();

				meta.ackNumb = rxSar.recvd;
				meta.seqNumb = txSar.not_ackd; //Always send SEQ
				meta.window_size = rxSar.windowSize; //Precalcualted in rx_sar_table ((rxSar.appd - rxSar.recvd) - 1)
				meta.length = 0;
				meta.ack = 1;
				meta.rst = 0;
				meta.syn = 0;
				meta.fin = 0;
				txEng_ipMetaFifoOut.write(meta.length);
				txEng_tcpMetaFifoOut.write(meta);
				txEng_isLookUpFifoOut.write(true);
				txEng2sLookup_rev_req.write(ml_curEvent.sessionID);
				ml_FsmState = 0;

				std::cout<<"TX_ACK";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;	
			}
			break;
		case SYN:
			if (((ml_curEvent.rt_count != 0) && !txSar2txEng_upd_rsp.empty()) || (ml_curEvent.rt_count == 0))
			{
				txTxSarReply txSar;
				if (ml_curEvent.rt_count != 0)
				{
					txSar = txSar2txEng_upd_rsp.read();
					meta.seqNumb = txSar.ackd;
				}
				else
				{
					// txSar.not_ackd = ml_randomValue; // FIXME better rand()
					// ml_randomValue = (ml_randomValue* 8) xor ml_randomValue;
					// meta.seqNumb = txSar.not_ackd;
					txSar.not_ackd = ((ml_randomValue << 6) - 1); 
					meta.seqNumb = txSar.not_ackd;
					ml_randomValue = (ml_randomValue* 8) xor ml_randomValue;
					txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd+1, 1, 1));
				}
				meta.ackNumb = 0;
				//meta.seqNumb = txSar.not_ackd;
				meta.window_size = 0xFFFF;
				meta.length = 4; // For MSS Option, 4 bytes
				meta.ack = 0;
				meta.rst = 0;
				meta.syn = 1;
				meta.fin = 0;
#if (WINDOW_SCALE)
				meta.length = 8;
				meta.win_shift = WINDOW_SCALE_BITS;
#endif

				txEng_ipMetaFifoOut.write(meta.length);
				txEng_tcpMetaFifoOut.write(meta);
				txEng_isLookUpFifoOut.write(true);
				txEng2sLookup_rev_req.write(ml_curEvent.sessionID);
				// set retransmit timer
				txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID, SYN));
				ml_FsmState = 0;

				std::cout<<"TX_SYN";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;	
			}
			break;
		case SYN_ACK:
			if (!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty())
			{
				rxSar2txEng_rsp.read(rxSar);
				txTxSarReply txSar = txSar2txEng_upd_rsp.read();

				// construct SYN_ACK message
				meta.ackNumb = rxSar.recvd;
				meta.window_size = 0xFFFF;
				meta.length = 4; // For MSS Option, 4 bytes
				meta.ack = 1;
				meta.rst = 0;
				meta.syn = 1;
				meta.fin = 0;
				if (ml_curEvent.rt_count != 0)
				{
					meta.seqNumb = txSar.ackd;
				}
				else
				{
					// txSar.not_ackd = ml_randomValue; // FIXME better rand();
					// ml_randomValue = (ml_randomValue* 8) xor ml_randomValue;
					// meta.seqNumb = txSar.not_ackd;
					txSar.not_ackd = ((ml_randomValue << 6) - 1); 
					meta.seqNumb = txSar.not_ackd;
					ml_randomValue = (ml_randomValue* 8) xor ml_randomValue;
					txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd+1, 1, 1));
				}

#if (WINDOW_SCALE)
				// An additional 4 bytes for WScale option -> total: 8 bytes
				if (rxSar.win_shift != 0)
				{
					meta.length = 8;
				}
				meta.win_shift = rxSar.win_shift;
#endif

				txEng_ipMetaFifoOut.write(meta.length);
				txEng_tcpMetaFifoOut.write(meta);
				txEng_isLookUpFifoOut.write(true);
				txEng2sLookup_rev_req.write(ml_curEvent.sessionID);

				// set retransmit timer
				txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID, SYN_ACK));
				ml_FsmState = 0;

				std::cout<<"TX_SYN_ACK";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;	
			}
			break;
		case FIN:
			if ((!rxSar2txEng_rsp.empty() && !txSar2txEng_upd_rsp.empty()) || ml_sarLoaded)
			{
				txTxSarReply txSar;
				if (!ml_sarLoaded)
				{
					rxSar2txEng_rsp.read(rxSar);
					txSar = txSar2txEng_upd_rsp.read();
				}
				else
				{
					txSar = txSarReg;
				}

				//construct FIN message
				meta.ackNumb = rxSar.recvd;
				//meta.seqNumb = txSar.not_ackd;
				meta.window_size = rxSar.windowSize; //Precalcualted in rx_sar_table ((rxSar.appd - rxSar.recvd) - 1)
				meta.length = 0;
				meta.ack = 1; // has to be set for FIN message as well
				meta.rst = 0;
				meta.syn = 0;
				meta.fin = 1;

				// Check if retransmission, in case of RT, we have to reuse not_ackd number
				if (ml_curEvent.rt_count != 0)
				{
					meta.seqNumb = txSar.not_ackd-1; //Special case, or use ackd?
				}
				else
				{
					meta.seqNumb = txSar.not_ackd;
					// Check if all data is sent, otherwise we have to delay FIN message
					// Set fin flag, such that probeTimer is informed
#if !(TCP_NODELAY)
					if (txSar.app == txSar.not_ackd(WINDOW_BITS-1, 0))
#endif
					{
						txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd+1, 1, 0, true, true));
					}
#if !(TCP_NODELAY)
					else
					{
						txEng2txSar_upd_req.write(txTxSarQuery(ml_curEvent.sessionID, txSar.not_ackd, 1, 0, true, false));
					}
#endif
				}

				// Check if there is a FIN to be sent //TODO maybe restruce this
#if !(TCP_NODELAY)
				if (meta.seqNumb(WINDOW_BITS-1, 0) == txSar.app)
#endif
				{
					txEng_ipMetaFifoOut.write(meta.length);
					txEng_tcpMetaFifoOut.write(meta);
					txEng_isLookUpFifoOut.write(true);
					txEng2sLookup_rev_req.write(ml_curEvent.sessionID);
					// set retransmit timer
					//txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID, FIN));
					txEng2timer_setRetransmitTimer.write(txRetransmitTimerSet(ml_curEvent.sessionID));
				}

				ml_FsmState = 0;

				std::cout<<"TX_FIN";
				std::cout<<std::dec<<" session id:"<<ml_curEvent.sessionID;
				std::cout<<" seqNum:"<<txSar.not_ackd;
				std::cout<<" ackNum:"<<rxSar.recvd;
				std::cout<<" window_size:"<<rxSar.windowSize;
				std::cout<<" ack:"<<meta.ack;
				std::cout<<" rst:"<<meta.rst;
				std::cout<<" syn:"<<meta.syn;
				std::cout<<" fin:"<<meta.fin;
				std::cout<<" length:"<<ml_curEvent.length<<std::endl;	
			}
			break;
		case RST:
			// Assumption RST length == 0
			resetEvent = ml_curEvent;
			if (!resetEvent.hasSessionID())
			{
				txEng_ipMetaFifoOut.write(0);
				txEng_tcpMetaFifoOut.write(tx_engine_meta(0, resetEvent.getAckNumb(), 1, 1, 0, 0));
				txEng_isLookUpFifoOut.write(false);
				txEng_tupleShortCutFifoOut.write(ml_curEvent.tuple);
				ml_FsmState = 0;
			}
			else if (!txSar2txEng_upd_rsp.empty())
			{
				txTxSarReply txSar = txSar2txEng_upd_rsp.read();
				txEng_ipMetaFifoOut.write(0);
				txEng_isLookUpFifoOut.write(true);
				txEng2sLookup_rev_req.write(resetEvent.sessionID); //there is no sessionID??
				//if (resetEvent.getAckNumb() != 0)
				//{
					txEng_tcpMetaFifoOut.write(tx_engine_meta(txSar.not_ackd, resetEvent.getAckNumb(), 1, 1, 0, 0));
				/*}
				/else
				{
					metaDataFifoOut.write(tx_engine_meta(txSar.not_ackd, rxSar.recvd, 1, 1, 0, 0));
				}*/
					ml_FsmState = 0;
			}
			break;
		} //switch
		break;
	} //switch
}

/** @ingroup tx_engine
 *  Forwards the incoming tuple from the SmartCam or RX Engine to the 2 header construction modules
 *  @param[in]	sLookup2txEng_rev_rsp
 *  @param[in]	txEng_tupleShortCutFifoIn
 *  @param[in]	txEng_isLookUpFifoIn
 *  @param[out]	txEng_ipTupleFifoOut
 *  @param[out]	txEng_tcpTupleFifoOut
 */
void tupleSplitter(	stream<fourTuple>&		sLookup2txEng_rev_rsp,
					stream<fourTuple>&		txEng_tupleShortCutFifoIn,
					stream<bool>&			txEng_isLookUpFifoIn,
					stream<twoTuple>&		txEng_ipTupleFifoOut,
					stream<fourTuple>&		txEng_tcpTupleFifoOut)
{
//#pragma HLS INLINE off
#pragma HLS pipeline II=1
	static bool ts_getMeta = true;
	static bool ts_isLookUp;

	fourTuple tuple;

	if (ts_getMeta)
	{
		if (!txEng_isLookUpFifoIn.empty())
		{
			txEng_isLookUpFifoIn.read(ts_isLookUp);
			ts_getMeta = false;
		}
	}
	else
	{
		if (!sLookup2txEng_rev_rsp.empty() && ts_isLookUp)
		{
			sLookup2txEng_rev_rsp.read(tuple);
			txEng_ipTupleFifoOut.write(twoTuple(tuple.srcIp, tuple.dstIp));
			txEng_tcpTupleFifoOut.write(tuple);
			ts_getMeta = true;
		}
		else if(!txEng_tupleShortCutFifoIn.empty() && !ts_isLookUp)
		{
			txEng_tupleShortCutFifoIn.read(tuple);
			txEng_ipTupleFifoOut.write(twoTuple(tuple.srcIp, tuple.dstIp));
			txEng_tcpTupleFifoOut.write(tuple);
			ts_getMeta = true;
		}
	}
}

/** @ingroup tx_engine
 * 	Reads the IP header metadata and the IP addresses. From this data it generates the IP header and streams it out.
 *  @param[in]		txEng_ipMetaDataFifoIn
 *  @param[in]		txEng_ipTupleFifoIn
 *  @param[out]		txEng_ipHeaderBufferOut
 */
/*void ipHeaderConstruction(stream<ap_uint<16> >&				txEng_ipMetaDataFifoIn,
							stream<twoTuple>&				txEng_ipTupleFifoIn,
							stream<axiWord>&				txEng_ipHeaderBufferOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum { WORD_0, WORD_1, WORD_2, WORD_3, WORD_4 };
	static ap_uint<2> ihc_currWord = 0;
	static twoTuple ihc_tuple;

	axiWord sendWord;
	ap_uint<16> length = 0;

	switch(ihc_currWord)
	{
	case WORD_0:
		if (!txEng_ipMetaDataFifoIn.empty())
		{
			txEng_ipMetaDataFifoIn.read(length);
			sendWord.data.range(7, 0) = 0x45;
			sendWord.data.range(15, 8) = 0;
			length = length + 40;
			sendWord.data.range(23, 16) = length(15, 8); //length
			sendWord.data.range(31, 24) = length(7, 0);
			sendWord.data.range(47, 32) = 0;
			sendWord.data.range(50, 48) = 0; //Flags
			sendWord.data.range(63, 51) = 0x0;//Fragment Offset //FIXME why is this here
			sendWord.keep = 0xFF;
			sendWord.last = 0;
			txEng_ipHeaderBufferOut.write(sendWord);
			ihc_currWord++;
		}
		break;
	case WORD_1:
		if (!txEng_ipTupleFifoIn.empty())
		{
			txEng_ipTupleFifoIn.read(ihc_tuple);
			sendWord.data.range(7, 0) = 0x40;
			sendWord.data.range(15, 8) = 0x06; // TCP
			sendWord.data.range(31, 16) = 0; // CS
			sendWord.data.range(63, 32) = ihc_tuple.srcIp; // srcIp
			sendWord.keep = 0xFF;
			sendWord.last = 0;
			txEng_ipHeaderBufferOut.write(sendWord);
			ihc_currWord++;
		}
		break;
	case WORD_2:
		sendWord.data.range(31, 0) = ihc_tuple.dstIp; // dstIp
		sendWord.keep = 0x0F;
		sendWord.last = 1;
		txEng_ipHeaderBufferOut.write(sendWord);
		ihc_currWord = 0;
		break;
	} //switch
}*/

//IMPORT INSTEAD of COPY //TODO or move to IP Output Handler
template <int WIDTH>
void generate_ipv4( //stream<ipv4Meta>&    txEng_ipMetaDataFifoIn,
			stream<ap_uint<16> >&				txEng_ipMetaDataFifoIn,
			stream<twoTuple>&				txEng_ipTupleFifoIn,
          stream<net_axis<WIDTH> >&  tx_shift2ipv4Fifo,
          stream<net_axis<WIDTH> >&  m_axis_tx_data)
          //ap_uint<32>      local_ipv4_address)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

  enum fsmStateType {META, HEADER, PARTIAL_HEADER, BODY};
  static fsmStateType gi_state=META;
  static ipv4Header<WIDTH> header;

  switch (gi_state)
  {
  case META:
    //if (!txEng_ipMetaDataFifoIn.empty())
	if (!txEng_ipMetaDataFifoIn.empty() && !txEng_ipTupleFifoIn.empty())
    {
      //txEng_ipMetaDataFifoIn.read();
	  ap_uint<16> metaLength = txEng_ipMetaDataFifoIn.read();
	  twoTuple tuples = txEng_ipTupleFifoIn.read();
      header.clear();

      //length = meta.length + 20; //was adding +40
	  ap_uint<16> length = metaLength + 40;
	  // std::cout << "length: " << length << std::endl;
      header.setLength(length);
      header.setDstAddr(tuples.dstIp);
      header.setSrcAddr(tuples.srcIp);
      header.setProtocol(TCP_PROTOCOL);
      if (IPV4_HEADER_SIZE >= WIDTH)
      {
		// std::cout << "switched to IP HEADER" << std::endl;
        gi_state = HEADER;
      }
      else
      {
		// std::cout << "switched to IP PARTIAL HEADER" << std::endl;
        gi_state = PARTIAL_HEADER;
      }
    }
    break;
  case HEADER:
  {
	net_axis<WIDTH> sendWord;
    if (header.consumeWord(sendWord.data) < (WIDTH/8))
    {
      /*currWord.keep = 0xFFFFFFFF; //TODO, set as much as required
      currWord.last = 0;
      m_axis_tx_data.write(currWord);*/
		// std::cout << "switched to IP PARTIAL HEADER" << std::endl;
      gi_state = PARTIAL_HEADER;
    }
    //else
    {
      sendWord.keep = 0xFFFFFFFF; //TODO, set as much as required
      sendWord.last = 0;
      m_axis_tx_data.write(sendWord);
      //gi_state = PARTIAL_HEADER;
    }
    break;
  }
  case PARTIAL_HEADER:
    if (!tx_shift2ipv4Fifo.empty())
    {
      net_axis<WIDTH> currWord = tx_shift2ipv4Fifo.read();
      header.consumeWord(currWord.data);
      m_axis_tx_data.write(currWord);
      gi_state = BODY;
	  // std::cout << "IP PARTIAL: ";
	  // printLE(std::cout, currWord);
	  // std::cout << std::endl;

      if (currWord.last)
      {
        gi_state = META;
      }
    }
    break;
  case BODY:
    if (!tx_shift2ipv4Fifo.empty())
    {
    	net_axis<WIDTH> currWord = tx_shift2ipv4Fifo.read();
		
		// std::cout << "IP BODY: ";// << std::endl;
		// printLE(std::cout, currWord);
	 //  std::cout << std::endl;

      m_axis_tx_data.write(currWord);
      if (currWord.last)
      {
        gi_state = META;
      }
    }
    break;
  }
}


/** @ingroup tx_engine
 * 	Reads the TCP header metadata and the IP tuples. From this data it generates the TCP pseudo header and streams it out.
 *  @param[in]		tcpMetaDataFifoIn
 *  @param[in]		tcpTupleFifoIn
 *  @param[out]		dataOut
 *  @TODO this should be better, cleaner
 */
//FIXME clean this up, code duplication
/*void pseudoHeaderConstruction(stream<tx_engine_meta>&		tcpMetaDataFifoIn,
								stream<fourTuple>&			tcpTupleFifoIn,
								stream<axiWord>&			dataOut)
{
#pragma HLS INLINE off
#pragma HLS pipeline II=1

	enum { WORD_0, WORD_1, WORD_2, WORD_3, WORD_4 };
	static ap_uint<3> phc_currWord = 0;
	axiWord sendWord;
	static tx_engine_meta phc_meta;
	static fourTuple phc_tuple;
	//static bool phc_done = true;
	ap_uint<16> length = 0;

	/*if (phc_done && !tcpMetaDataFifoIn.empty())
	{
		tcpMetaDataFifoIn.read(phc_meta);
		phc_done = false;
		//Adapt window, if too small close completely
		/*if(phc_meta.window_size < (MY_MSS/2))
		{
			phc_meta.window_size = 0;
		}*//*
	}
	else if (!phc_done)
	{*/
		/*switch(phc_currWord)
		{
		case WORD_0:
			if (!tcpTupleFifoIn.empty() && !tcpMetaDataFifoIn.empty())
			{
				tcpTupleFifoIn.read(phc_tuple);
				tcpMetaDataFifoIn.read(phc_meta);
				sendWord.data.range(31, 0) = phc_tuple.srcIp;
				sendWord.data.range(63, 32) = phc_tuple.dstIp;
				sendWord.keep = 0xFF;
				sendWord.last = 0;
				dataOut.write(sendWord);
				phc_currWord++;
			}
			break;
		case WORD_1:
			sendWord.data.range(7, 0) = 0x00;
			sendWord.data.range(15, 8) = 0x06; // TCP
			length = phc_meta.length + 0x14;  // 20 bytes for the header
			sendWord.data.range(23, 16) = length(15, 8);
			sendWord.data.range(31, 24) = length(7, 0);
			sendWord.data.range(47, 32) = phc_tuple.srcPort; // srcPort
			sendWord.data.range(63, 48) = phc_tuple.dstPort; // dstPort
			sendWord.keep = 0xFF;
			sendWord.last = 0;
			dataOut.write(sendWord);
			phc_currWord++;
			break;
		case WORD_2:
			// Insert SEQ number
			sendWord.data(7, 0)   = phc_meta.seqNumb(31, 24);
			sendWord.data(15, 8)  = phc_meta.seqNumb(23, 16);
			sendWord.data(23, 16) = phc_meta.seqNumb(15, 8);
			sendWord.data(31, 24) = phc_meta.seqNumb(7, 0);
			// Insert ACK number
			sendWord.data(39, 32) = phc_meta.ackNumb(31, 24);
			sendWord.data(47, 40) = phc_meta.ackNumb(23, 16);
			sendWord.data(55, 48) = phc_meta.ackNumb(15, 8);
			sendWord.data(63, 56) = phc_meta.ackNumb(7, 0);
			sendWord.keep = 0xFF;
			sendWord.last = 0;
			dataOut.write(sendWord);
			phc_currWord++;
			break;
		case WORD_3:
			sendWord.data(3,1) = 0; // reserved
			sendWord.data(7, 4) = (0x5 + phc_meta.syn); //data offset
			/* Control bits:
			 * [8] == FIN
			 * [9] == SYN
			 * [10] == RST
			 * [11] == PSH
			 * [12] == ACK
			 * [13] == URG
			 */
			/*sendWord.data[0] = 0; //NS bit
			sendWord.data[8] = phc_meta.fin; //control bits
			sendWord.data[9] = phc_meta.syn;
			sendWord.data[10] = phc_meta.rst;
			sendWord.data[11] = 0;
			sendWord.data[12] = phc_meta.ack;
			sendWord.data(15, 13) = 0; //some other bits
			//sendWord.data.range(31, 16) = phc_meta.window_size; //check if at least half the size FIXME
			sendWord.data.range(23, 16) = phc_meta.window_size(15, 8);
			sendWord.data.range(31, 24) = phc_meta.window_size(7, 0);
			sendWord.data.range(63, 32) = 0; //urgPointer & checksum
			sendWord.keep = 0xFF;
			sendWord.last = (phc_meta.length == 0);
			dataOut.write(sendWord);
			if (!phc_meta.syn)
			{
				phc_currWord = 0;
			}
			else
			{
				phc_currWord++;
			}
			//phc_done = true;
			break;
		case WORD_4: // Only used for SYN and MSS negotiation
			sendWord.data(7, 0) = 0x02; // Option Kind
			sendWord.data(15, 8) = 0x04; // Option length
			sendWord.data(31, 16) = 0xB405; // 0x05B4 = 1460
			sendWord.data(63, 32) = 0;
			sendWord.keep = 0x0F;
			sendWord.last = 1;//(phc_meta.length == 0x04); //OR JUST SET TO 1
			dataOut.write(sendWord);
			phc_currWord = 0;
			break;
		} //switch
	//} //else
}*/

template <int WIDTH>
void pseudoHeaderConstructionNew(stream<tx_engine_meta>&		tcpMetaDataFifoIn,
								stream<fourTuple>&			tcpTupleFifoIn,
								stream<net_axis<WIDTH> >&		dataIn,
								stream<net_axis<WIDTH> >&		dataOut)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum fsmState {META, HEADER, PARTIAL_HEADER, BODY, HEADER_MSS_OPTION};
	static fsmState state = META;
	static tcpFullPseudoHeader<WIDTH> header; //size 256Bit
	static bool hasBody = false;
	static bool isSYN = false;
	static ap_uint<4> win_shift = 0;

	switch (state)
	{
	case META:
		if (!tcpMetaDataFifoIn.empty() && !tcpTupleFifoIn.empty())
		{
			tx_engine_meta meta = tcpMetaDataFifoIn.read();
			fourTuple tuple = tcpTupleFifoIn.read();

			header.clear();
			header.setSrcAddr(tuple.srcIp);
			header.setDstAddr(tuple.dstIp);
			header.setLength(meta.length + 0x14); // +20 bytes for the header without options
			header.setSrcPort(tuple.srcPort);
			header.setDstPort(tuple.dstPort);
			header.setSeqNumb(meta.seqNumb);
			header.setAckNumb(meta.ackNumb);
#if !(WINDOW_SCALE)
			header.setDataOffset(0x5 + meta.syn);
#else
			win_shift = meta.win_shift;
			if (meta.syn)
			{
				if (meta.win_shift != 0)
				{
					header.setDataOffset(0x7);
				}
				else
				{
					header.setDataOffset(0x6);
				}
			}
			else
			{
				header.setDataOffset(0x5);
			}
#endif
			//flags
			header.setFinFlag(meta.fin);
			header.setSynFlag(meta.syn);
			header.setRstFlag(meta.rst);
			header.setAckFlag(meta.ack);

			hasBody = meta.length != 0 && !meta.syn;
			isSYN = meta.syn;


			header.setWindowSize(meta.window_size);
			header.setChecksum(0);

			if (WIDTH > 256 && hasBody)
			{
				state = PARTIAL_HEADER;
			}
			else
			{
				// std::cout << "switch to TCP HEADER" << std::endl;
				//TODO handle 512
				state = HEADER;
			}
		}
		break;
	case HEADER:
	{
		net_axis<WIDTH> sendWord;
		sendWord.last = 0;
		ap_uint<8> remainingLength = header.consumeWord(sendWord.data);
		// std::cout << std::dec << "remainingLenght: " << remainingLength << std::endl;
		if (remainingLength < (WIDTH/8))
		//if (header.consumeWord(sendWord.data) < WIDTH)
		{
			if (hasBody)
			{
				// std::cout << "switch to TCP BODY" << std::endl;
				state = BODY; //PARTIAL_HEADER;
			}
			else
			{
				if (isSYN && WIDTH <= 256)
				{
					// std::cout << "switch to MSS" << std::endl;
					state = HEADER_MSS_OPTION;
				}
				else
				{
					sendWord.last = 1;
					state = META;
				}
			}
		}
		sendWord.keep = 0xffffffff; //Keep for 256bit (size of the header)
		// std::cout << "TCP HEADER: ";
		// printLE(std::cout, sendWord);
		// std::cout << std::endl;

		//In case of WIDTH == 512, we can add the MSS option into the first word
		if (isSYN && WIDTH == 512)
		{
			// MSS negotiation is only used in SYN packets
			sendWord.data(263, 256) = 0x02; // Option Kind
			sendWord.data(271, 264) = 0x04; // Option length
			sendWord.data(287, 272) = reverse((ap_uint<16>)MSS); //0xB405; // 0x05B4 = 1460
#ifndef __SYNTHESIS__
			sendWord.data(511, 288) = 0;
#endif
			sendWord.keep(35,32) = 0xF;
#if (WINDOW_SCALE)
			// WSopt negotiation, only send in SYN-ACK if received with SYN as in RFC 7323 1.3
			// std::cout << "PSEUDO NEW: " << win_shift << std::endl;
			if (win_shift != 0)
			{
				sendWord.data(295, 288) = 0x03; // Option Kind
				sendWord.data(303, 296) = 0x03; // Option length
				sendWord.data(311, 304) = win_shift;
				sendWord.data(319, 312) = 0x0; // End of option list
				sendWord.keep(39, 36) = 0xF;
			}
#endif
		}

		dataOut.write(sendWord);
		break;
	}
	//TODO PARTIAL HEADER only required for 512
	case PARTIAL_HEADER:
		if (!dataIn.empty())
		{
			net_axis<WIDTH> currWord = dataIn.read();
			header.consumeWord(currWord.data);
			dataOut.write(currWord);
			state = BODY;
			if (currWord.last)
			{
				state = META;
			}
		}
		break;
	case BODY:
		if (!dataIn.empty())
		{
			net_axis<WIDTH> currWord = dataIn.read();
			dataOut.write(currWord);
			if (currWord.last)
			{
				state = META;
			}
		}
		break;
	case HEADER_MSS_OPTION: //TODO Rename
	{
		net_axis<WIDTH> sendWord;
		// MSS negotiation is only used in SYN packets
		sendWord.data(7, 0) = 0x02; // Option Kind
		sendWord.data(15, 8) = 0x04; // Option length
		sendWord.data(31, 16) = reverse((ap_uint<16>)MSS); //0xB405; // 0x05B4 = 1460
		sendWord.keep = 0x0F;
#ifndef __SYNTHESIS__
		sendWord.data(63, 32) = 0;
#endif
#if (WINDOW_SCALE)
		// WSopt negotiation, only send in SYN-ACK if received with SYN as in RFC 7323 1.3
		// std::cout << "PSEUDO NEW: " << win_shift << std::endl;
		if (win_shift != 0)
		{
			sendWord.data(39, 32) = 0x03; // Option Kind
			sendWord.data(47, 40) = 0x03; // Option length
			sendWord.data(55, 48) = win_shift;
			sendWord.data(63, 56) = 0x0; // End of option list
			sendWord.keep = 0xFF;
		}
#endif
		sendWord.last = 1;

		dataOut.write(sendWord);
		state = META;
		break;
	}//HEADER_LAST
	}//switch
}


/** @ingroup tx_engine
 * In case of the payload had to bread using two DDR commands, it it concatened by this module
 */
template <int WIDTH>
void read_data_stitching(	hls::stream<bool>&			memAccessBreakdown2readPkgStitcher,
							hls::stream<net_axis<WIDTH> >&		readDataIn,
							hls::stream<net_axis<WIDTH> >&		readDataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static net_axis<WIDTH> prevWord;
	static bool pkgNeedsMerge = false;
	enum fsmStateType {IDLE, FIRST_PART, STITCH, ATTACH_ALIGNED, RESIDUE};
	static fsmStateType state = IDLE;
	static ap_uint<8> offset = 0;

	switch(state)
	{
	case IDLE:
		if (!memAccessBreakdown2readPkgStitcher.empty() && !readDataIn.empty())
		{
			memAccessBreakdown2readPkgStitcher.read(pkgNeedsMerge);
			net_axis<WIDTH> currWord = readDataIn.read();

			offset = keepToLen(currWord.keep);

			//Check if packet has only 1 word
			if (currWord.last)
			{
				if (pkgNeedsMerge)
				{
					currWord.last = 0;
					//Check if next packet has to be aligned or not
					if (currWord.keep[WIDTH/8-1] == 0)
					{
						//currWord is stored in prevWord
						state = STITCH;
					}
					else
					{
						readDataOut.write(currWord);
						state = ATTACH_ALIGNED;
					}
					
				}
				else //packet does not have to be merged
				{
					readDataOut.write(currWord);
					//We remain in this state
				}
				
			}
			else //packet contains more than 1 word
			{
				readDataOut.write(currWord);
				state = FIRST_PART;
			}
			prevWord = currWord;
		}
		break;
	case FIRST_PART:	// This state outputs the all the data words in the 1st memory access of a segment but the 1st one.
		if (!readDataIn.empty())
		{
			net_axis<WIDTH> currWord = readDataIn.read();
			offset = keepToLen(currWord.keep);

			//Check if end of packet
			if (currWord.last)
			{
				if (pkgNeedsMerge)
				{
					currWord.last = 0;
					//Check if next packet has to be aligned or not
					if (currWord.keep[WIDTH/8-1] == 0)
					{
						//currWord is stored in prevWord
						state = STITCH;
					}
					else
					{
						readDataOut.write(currWord);
						state = ATTACH_ALIGNED;
					}
					
				}
				else //packet does not have to be merged
				{
					readDataOut.write(currWord);
					state = IDLE;
				}
				
			}
			else 
			{
				readDataOut.write(currWord);
				//Remain in this state until last
			}
			prevWord = currWord;
		}
		break;
	case ATTACH_ALIGNED:													// This state handles 2nd mem.access data when no realignment is required
		if (!readDataIn.empty())
		{
			net_axis<WIDTH> currWord = readDataIn.read();
			readDataOut.write(currWord);
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	case STITCH:
		if (!readDataIn.empty())
		{
			net_axis<WIDTH> sendWord;
			sendWord.last = 0;
			net_axis<WIDTH> currWord = readDataIn.read();

			//Create new word consisting of current and previous word
			// offset specifies how many bytes of prevWord are valid
			sendWord.data((offset*8) -1, 0) = prevWord.data((offset*8) -1, 0);
			sendWord.keep(offset-1, 0) = prevWord.keep(offset -1, 0);

			sendWord.data(WIDTH-1, (offset*8)) = currWord.data(WIDTH - (offset*8) - 1, 0);
			sendWord.keep(WIDTH/8-1, offset) = currWord.keep(WIDTH/8 - offset - 1, 0);
			sendWord.last = (currWord.keep[WIDTH/8 - offset] == 0);

			readDataOut.write(sendWord);
			prevWord.data((offset*8) - 1, 0) = currWord.data(WIDTH-1, WIDTH - (offset*8));
			prevWord.keep(offset - 1, 0) = currWord.keep(WIDTH/8 - 1, WIDTH/8 - offset);
			if (currWord.last)
			{
				state = IDLE;
				if (!sendWord.last)
				{
					state = RESIDUE;
				}
			}
		}
		break;
	case RESIDUE:
		{
			net_axis<WIDTH> sendWord;
			sendWord.data((offset*8) -1, 0) = prevWord.data((offset*8) -1, 0);
			sendWord.keep(offset-1, 0) = prevWord.keep(offset -1, 0);

#ifndef __SYNTHESIS__
			sendWord.data(WIDTH-1, (offset*8)) = 0;
#endif
			sendWord.keep(WIDTH/8-1, offset) = 0;
			sendWord.last = 1;

			readDataOut.write(sendWord);
			state = IDLE;
		}
		break;
	}
}



//TODO call function only when NODELAY
template <int WIDTH>
void read_data_arbiter(stream<net_axis<WIDTH> >&		txBufferReadData,
#if (TCP_NODELAY)
					stream<bool>&			txEng_isDDRbypass,
					stream<net_axis<WIDTH> >&		txApp2txEng_data_stream,
#endif
					stream<net_axis<WIDTH> >&		txEng_tcpSegOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ap_uint<2>	tps_state = 0; //TODO rename

	switch (tps_state)
	{
	case 0:
#if (TCP_NODELAY)
		if (!txEng_isDDRbypass.empty())
		{
			bool isBypass = txEng_isDDRbypass.read();
			if (isBypass)
			{
				tps_state = 2;
			}
			else
			{
				tps_state = 1;
			}
			
		}
#else
		tps_state = 1
#endif
		break;
	case 1:
		if (!txBufferReadData.empty())
		{
			net_axis<WIDTH> currWord = txBufferReadData.read();
			txEng_tcpSegOut.write(currWord);
			if (currWord.last)
			{
				tps_state = 0;
			}
		}
		break;
#if (TCP_NODELAY)
	case 2:
		if (!txApp2txEng_data_stream.empty())
		{
			net_axis<WIDTH> currWord = txApp2txEng_data_stream.read();
			// std::cout << "ARBITER: ";
			// printLE(std::cout, currWord);
			// std::cout << std::endl;
			txEng_tcpSegOut.write(currWord);
			if (currWord.last)
			{
				tps_state = 0;
			}
		}
		break;
#endif
	} // switch
}



//TODO rename & reuse
void txEngMemAccessBreakdown(	hls::stream<mmCmd>&	inputMemAccess,
										hls::stream<mmCmd>&	outputMemAccess,
										hls::stream<bool>&	memAccessBreakdown2txPkgStitcher)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static ap_uint<1> txEngBreakdownState = 0;
	static mmCmd cmd;
	static ap_uint<WINDOW_BITS> lengthFirstAccess;

	switch (txEngBreakdownState)
	{
	case 0:
		if (!inputMemAccess.empty())
		{
			inputMemAccess.read(cmd);
			std::cout << "TX read cmd: " << cmd.saddr << ", " << cmd.bbt << std::endl;
			if ((cmd.saddr(WINDOW_BITS-1, 0) + cmd.bbt) > BUFFER_SIZE)
			{
				lengthFirstAccess = BUFFER_SIZE - cmd.saddr;
				
				memAccessBreakdown2txPkgStitcher.write(true);
				outputMemAccess.write(mmCmd(cmd.saddr, lengthFirstAccess));
				txEngBreakdownState = 1;
			}
			else
			{
				memAccessBreakdown2txPkgStitcher.write(false);
				outputMemAccess.write(cmd);
			}
		}
		break;
	case 1:
			outputMemAccess.write(mmCmd(0, cmd.bbt - lengthFirstAccess));
			txEngBreakdownState = 0;
		break;
	}
}

// Removes the first word in case the WIDTH == 64
template <int WIDTH>
void remove_pseudo_header(hls::stream<net_axis<WIDTH> >&	dataIn,
							hls::stream<net_axis<WIDTH> >&	dataOut)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	static bool firstWord = true;

	if (!dataIn.empty())
	{
		net_axis<WIDTH> word = dataIn.read();
		// std::cout << "REMOVE: ";
		// printLE(std::cout, word);
		// std::cout << std::endl;

		if (!firstWord || WIDTH > 64)
		{
			dataOut.write(word);
		}
		firstWord = false;
		if (word.last)
		{
			firstWord = true;
		}
	}
}

template<int WIDTH>
void insert_checksum(	hls::stream<ap_uint<16> >&			checksumIn,
						hls::stream<net_axis<WIDTH> >&		dataIn,
						hls::stream<net_axis<WIDTH> >&		dataOut)
{
#pragma HLS PIPELINE II=1

	static ap_uint<2> state = (WIDTH > 128 ? 1 : 0);
	static ap_uint<3> wordCount = 0;

	switch(state)
	{
		case 0:
			if (!dataIn.empty())
			{
				net_axis<WIDTH> word = dataIn.read();
				dataOut.write(word);
				wordCount++;
				if (wordCount == 128/WIDTH)
				{
					wordCount = 0;
					state = 1;
				}
			}
			break;
		case 1:
			//Insert checksum
			if (!checksumIn.empty() && !dataIn.empty())
			{
				ap_uint<16> checksum = checksumIn.read();
				// std::cout << "INSERTING checksum: " << checksum << std::endl;
				net_axis<WIDTH> word = dataIn.read();
				// printLE(std::cout, word);
				// std::cout << std::endl;
				if (WIDTH > 128)
				{
					word.data(143, 128) = reverse(checksum);
				}
				else
				{
					word.data(15, 0) = reverse(checksum);
				}
				dataOut.write(word);
				// std::cout << "after: ";
				// printLE(std::cout, word);
				// std::cout << std::endl;

				state = 2;
				if (word.last)
				{
					state = (WIDTH > 128 ? 1 : 0);
				}
			}
			break;
		case 2:
			if (!dataIn.empty())
			{
				net_axis<WIDTH> word = dataIn.read();
				dataOut.write(word);
				if (word.last)
				{
					state = (WIDTH > 128 ? 1 : 0);
				}
			}
			break;
	}
}

/** @ingroup tx_engine
 *  @param[in]		eventEng2txEng_event
 *  @param[in]		rxSar2txEng_upd_rsp
 *  @param[in]		txSar2txEng_upd_rsp
 *  @param[in]		txBufferReadData
 *  @param[in]		sLookup2txEng_rev_rsp
 *  @param[out]		txEng2rxSar_upd_req
 *  @param[out]		txEng2txSar_upd_req
 *  @param[out]		txEng2timer_setRetransmitTimer
 *  @param[out]		txEng2timer_setProbeTimer
 *  @param[out]		txBufferReadCmd
 *  @param[out]		txEng2sLookup_rev_req
 *  @param[out]		ipTxData
 */
template <int WIDTH>
void tx_engine(	stream<extendedEvent>&			eventEng2txEng_event,
				stream<rxSarReply>&				rxSar2txEng_rsp,
				stream<txTxSarReply>&			txSar2txEng_upd_rsp,
				stream<net_axis<WIDTH> >&			txBufferReadData,
#if (TCP_NODELAY)
				stream<net_axis<WIDTH> >&			txApp2txEng_data_stream,
#endif
				stream<fourTuple>&				sLookup2txEng_rev_rsp,
				stream<ap_uint<16> >&			txEng2rxSar_req,
				stream<txTxSarQuery>&			txEng2txSar_upd_req,
				stream<txRetransmitTimerSet>&	txEng2timer_setRetransmitTimer,
				stream<ap_uint<16> >&			txEng2timer_setProbeTimer,
				stream<mmCmd>&					txBufferReadCmd,
				stream<ap_uint<16> >&			txEng2sLookup_rev_req,
				stream<net_axis<WIDTH> >&				ipTxData,
				stream<ap_uint<1> >&			readCountFifo)
{
// #pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none port=return
//#pragma HLS PIPELINE II=1
#pragma HLS INLINE //off

	// Memory Read delay around 76 cycles, 10 cycles/packet, so keep meta of at least 8 packets
	//static stream<tx_engine_meta>		txEng_metaDataFifo("txEng_metaDataFifo");
	static stream<ap_uint<16> >			txEng_ipMetaFifo("txEng_ipMetaFifo");
	static stream<tx_engine_meta>		txEng_tcpMetaFifo("txEng_tcpMetaFifo");
	//#pragma HLS stream variable=txEng_metaDataFifo depth=16
	#pragma HLS stream variable=txEng_ipMetaFifo depth=32
	#pragma HLS stream variable=txEng_tcpMetaFifo depth=32
#if defined( __VITIS_HLS__)
	//#pragma HLS aggregate  variable=txEng_metaDataFifo compact=bit
	//#pragma HLS aggregate  variable=txEng_ipMetaFifo compact=bit
	#pragma HLS aggregate  variable=txEng_tcpMetaFifo compact=bit
#else
	//#pragma HLS DATA_PACK variable=txEng_metaDataFifo
	//#pragma HLS DATA_PACK variable=txEng_ipMetaFifo
	#pragma HLS DATA_PACK variable=txEng_tcpMetaFifo
#endif

	static hls::stream<net_axis<WIDTH> >		txBufferReadDataStitched("txBufferReadDAtaStitched");
	static hls::stream<net_axis<WIDTH> >		txEng_shift2pseudoFifo("txEng_shift2pseudoFifo");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer0("txEng_tcpPkgBuffer0");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer1("txEng_tcpPkgBuffer1");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer2("txEng_tcpPkgBuffer2");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer3("txEng_tcpPkgBuffer3");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer4("txEng_tcpPkgBuffer4");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer5("txEng_tcpPkgBuffer5");
	static hls::stream<net_axis<WIDTH> >		txEng_tcpPkgBuffer6("txEng_tcpPkgBuffer6");

	#pragma HLS stream variable=txBufferReadDataStitched  depth=2
	#pragma HLS stream variable=txEng_shift2pseudoFifo depth=2
	#pragma HLS stream variable=txEng_tcpPkgBuffer0 depth=2
	#pragma HLS stream variable=txEng_tcpPkgBuffer1 depth=16   // is forwarded immediately, size is not critical
	#pragma HLS stream variable=txEng_tcpPkgBuffer2 depth=256  // critical, has to keep complete packet for checksum computation
	#pragma HLS stream variable=txEng_tcpPkgBuffer3 depth=2
	#pragma HLS stream variable=txEng_tcpPkgBuffer4 depth=2
	#pragma HLS stream variable=txEng_tcpPkgBuffer5 depth=2
	#pragma HLS stream variable=txEng_tcpPkgBuffer6 depth=2

#if defined( __VITIS_HLS__)
	#pragma HLS aggregate  variable=txBufferReadDataStitched compact=bit
	#pragma HLS aggregate  variable=txEng_shift2pseudoFifo compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer0 compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer1 compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer2 compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer3 compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer4 compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer5 compact=bit
	#pragma HLS aggregate  variable=txEng_tcpPkgBuffer6 compact=bit

	static stream<subSums<WIDTH/16> >			txEng_subChecksumsFifo("txEng_subChecksumsFifo");
	static stream<ap_uint<16> >			txEng_tcpChecksumFifo("txEng_tcpChecksumFifo");
	#pragma HLS stream variable=txEng_subChecksumsFifo depth=2
	#pragma HLS stream variable=txEng_tcpChecksumFifo depth=4
	#pragma HLS aggregate  variable=txEng_subChecksumsFifo compact=bit

	static stream<fourTuple> 		txEng_tupleShortCutFifo("txEng_tupleShortCutFifo");
	static stream<bool>				txEng_isLookUpFifo("txEng_isLookUpFifo");
	static stream<twoTuple>			txEng_ipTupleFifo("txEng_ipTupleFifo");
	static stream<fourTuple>		txEng_tcpTupleFifo("txEng_tcpTupleFifo");
	#pragma HLS stream variable=txEng_tupleShortCutFifo depth=2
	#pragma HLS stream variable=txEng_isLookUpFifo depth=4
	#pragma HLS stream variable=txEng_ipTupleFifo depth=32
	#pragma HLS stream variable=txEng_tcpTupleFifo depth=32
	#pragma HLS aggregate  variable=txEng_tupleShortCutFifo compact=bit
	#pragma HLS aggregate  variable=txEng_ipTupleFifo compact=bit
	#pragma HLS aggregate  variable=txEng_tcpTupleFifo compact=bit

	static stream<mmCmd> txMetaloader2memAccessBreakdown("txMetaloader2memAccessBreakdown");
	#pragma HLS stream variable=txMetaloader2memAccessBreakdown depth=32
	#pragma HLS aggregate  variable=txMetaloader2memAccessBreakdown compact=bit
	static stream<bool> memAccessBreakdown2txPkgStitcher("memAccessBreakdown2txPkgStitcher");
	#pragma HLS stream variable=memAccessBreakdown2txPkgStitcher depth=32
	
	static stream<bool> txEng_isDDRbypass("txEng_isDDRbypass");
	#pragma HLS stream variable=txEng_isDDRbypass depth=32
#else
	#pragma HLS DATA_PACK variable=txBufferReadDataStitched
	#pragma HLS DATA_PACK variable=txEng_shift2pseudoFifo
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer0
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer1
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer2
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer3
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer4
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer5
	#pragma HLS DATA_PACK variable=txEng_tcpPkgBuffer6

	static stream<subSums<WIDTH/16> >			txEng_subChecksumsFifo("txEng_subChecksumsFifo");
	static stream<ap_uint<16> >			txEng_tcpChecksumFifo("txEng_tcpChecksumFifo");
	#pragma HLS stream variable=txEng_subChecksumsFifo depth=2
	#pragma HLS stream variable=txEng_tcpChecksumFifo depth=4
	#pragma HLS DATA_PACK variable=txEng_subChecksumsFifo

	static stream<fourTuple> 		txEng_tupleShortCutFifo("txEng_tupleShortCutFifo");
	static stream<bool>				txEng_isLookUpFifo("txEng_isLookUpFifo");
	static stream<twoTuple>			txEng_ipTupleFifo("txEng_ipTupleFifo");
	static stream<fourTuple>		txEng_tcpTupleFifo("txEng_tcpTupleFifo");
	#pragma HLS stream variable=txEng_tupleShortCutFifo depth=2
	#pragma HLS stream variable=txEng_isLookUpFifo depth=4
	#pragma HLS stream variable=txEng_ipTupleFifo depth=32
	#pragma HLS stream variable=txEng_tcpTupleFifo depth=32
	#pragma HLS DATA_PACK variable=txEng_tupleShortCutFifo
	#pragma HLS DATA_PACK variable=txEng_ipTupleFifo
	#pragma HLS DATA_PACK variable=txEng_tcpTupleFifo

	static stream<mmCmd> txMetaloader2memAccessBreakdown("txMetaloader2memAccessBreakdown");
	#pragma HLS stream variable=txMetaloader2memAccessBreakdown depth=32
	#pragma HLS DATA_PACK variable=txMetaloader2memAccessBreakdown
	static stream<bool> memAccessBreakdown2txPkgStitcher("memAccessBreakdown2txPkgStitcher");
	#pragma HLS stream variable=memAccessBreakdown2txPkgStitcher depth=32
	
	static stream<bool> txEng_isDDRbypass("txEng_isDDRbypass");
	#pragma HLS stream variable=txEng_isDDRbypass depth=32
#endif

	metaLoader(	eventEng2txEng_event,
				rxSar2txEng_rsp,
				txSar2txEng_upd_rsp,
				txEng2rxSar_req,
				txEng2txSar_upd_req,
				txEng2timer_setRetransmitTimer,
				txEng2timer_setProbeTimer,
				txEng_ipMetaFifo,
				txEng_tcpMetaFifo,
				txMetaloader2memAccessBreakdown,
				txEng2sLookup_rev_req,
				txEng_isLookUpFifo,
#if (TCP_NODELAY)
				txEng_isDDRbypass,
#endif
				txEng_tupleShortCutFifo,
				readCountFifo);

	txEngMemAccessBreakdown(txMetaloader2memAccessBreakdown, txBufferReadCmd, memAccessBreakdown2txPkgStitcher);

	tupleSplitter(	sLookup2txEng_rev_rsp,
					txEng_tupleShortCutFifo,
					txEng_isLookUpFifo,
					txEng_ipTupleFifo,
					txEng_tcpTupleFifo);

	//Stitches splitted reads back toghether
	read_data_stitching<WIDTH>(memAccessBreakdown2txPkgStitcher, txBufferReadData, txBufferReadDataStitched);
	//Arbitrates between DRAM and bypass data, concatenas DRAM data if necessary
	read_data_arbiter<WIDTH>(txBufferReadDataStitched,
#if (TCP_NODELAY)
					txEng_isDDRbypass,
					txApp2txEng_data_stream,
#endif
					txEng_tcpPkgBuffer0);

	lshiftWordByOctet<WIDTH, 51>(((TCP_FULL_PSEUDO_HEADER_SIZE%WIDTH)/8), txEng_tcpPkgBuffer0, txEng_shift2pseudoFifo);

	pseudoHeaderConstructionNew<WIDTH>(txEng_tcpMetaFifo, txEng_tcpTupleFifo, txEng_shift2pseudoFifo, txEng_tcpPkgBuffer1);
	
	two_complement_subchecksums<WIDTH, 22>(txEng_tcpPkgBuffer1, txEng_tcpPkgBuffer2, txEng_subChecksumsFifo);
	finalize_ipv4_checksum<WIDTH/16>(txEng_subChecksumsFifo, txEng_tcpChecksumFifo);

	// Removes one word in case of WIDTH=64bit (IP part of pseudo header is 96bit)
	remove_pseudo_header<WIDTH>(txEng_tcpPkgBuffer2, txEng_tcpPkgBuffer3);

	//TODO merge these two shifts
	rshiftWordByOctet<net_axis<WIDTH>, WIDTH, 53>(((96%WIDTH)/8), txEng_tcpPkgBuffer3, txEng_tcpPkgBuffer4);

	insert_checksum<WIDTH>(txEng_tcpChecksumFifo, txEng_tcpPkgBuffer4, txEng_tcpPkgBuffer5);
	
	lshiftWordByOctet<WIDTH, 52>(((IPV4_HEADER_SIZE%WIDTH)/8), txEng_tcpPkgBuffer5, txEng_tcpPkgBuffer6);

	generate_ipv4<WIDTH>(txEng_ipMetaFifo, txEng_ipTupleFifo, txEng_tcpPkgBuffer6, ipTxData);

}

template void tx_engine<DATA_WIDTH>(	stream<extendedEvent>&			eventEng2txEng_event,
				stream<rxSarReply>&				rxSar2txEng_rsp,
				stream<txTxSarReply>&			txSar2txEng_upd_rsp,
				stream<net_axis<DATA_WIDTH> >&			txBufferReadData,
#if (TCP_NODELAY)
				stream<net_axis<DATA_WIDTH> >&			txApp2txEng_data_stream,
#endif
				stream<fourTuple>&				sLookup2txEng_rev_rsp,
				stream<ap_uint<16> >&			txEng2rxSar_req,
				stream<txTxSarQuery>&			txEng2txSar_upd_req,
				stream<txRetransmitTimerSet>&	txEng2timer_setRetransmitTimer,
				stream<ap_uint<16> >&			txEng2timer_setProbeTimer,
				stream<mmCmd>&					txBufferReadCmd,
				stream<ap_uint<16> >&			txEng2sLookup_rev_req,
				stream<net_axis<DATA_WIDTH> >&				ipTxData,
				stream<ap_uint<1> >&			readCountFifo);
