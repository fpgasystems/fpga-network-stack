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
#include "retransmitter.hpp"
#include <rocev2_config.hpp> //defines MAX_QPS

using namespace hls;


//TODO maybe introduce seperate request streams
void retrans_pointer_table(	stream<pointerReq>&					pointerReqFifo,
					stream<pointerUpdate>&				pointerUpdFifo,
					stream<retransPointerEntry>& 		pointerRspFifo)
					//stream<event>&			retrans2event)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static retransPointerEntry ptr_table[MAX_QPS];
	#pragma HLS bind_storage variable=ptr_table type=RAM_T2P impl=BRAM
	//#pragma HLS DEPENDENCE variable=ptr_table inter false

	static ap_uint<16> pt_lockedQP;
	static bool pt_isLocked = false;
	static bool pt_wait = false;
	static pointerReq pt_req;

	pointerUpdate upd;

	if (!pointerUpdFifo.empty())
	{
		pointerUpdFifo.read(upd);
		ptr_table[upd.qpn] = upd.entry;
		if (pt_lockedQP == upd.qpn)
		{
			pt_isLocked = false;
		}
	}
	else if (!pointerReqFifo.empty() && !pt_wait)
	{
		pointerReqFifo.read(pt_req);
		if (pt_req.lock && pt_isLocked)
		{
			pt_wait = true;
		}
		else
		{
			pointerRspFifo.write(ptr_table[pt_req.qpn]);
			if (pt_req.lock)
			{
				pt_isLocked = true;
				pt_lockedQP = pt_req.qpn;
			}
		}
	}
	else if (pt_wait)
	{
		if (!pt_isLocked)
		{
			pointerRspFifo.write(ptr_table[pt_req.qpn]);
			pt_isLocked = true;
			pt_lockedQP = pt_req.qpn;
			pt_wait = false;
		}
	}
}

/*void pointer_table(	stream<retransRelease>&			rx2retrans_release_upd,
					stream<retransmission>& 		rx2retrans_req,
					stream<retransmission>& 		timer2retrans_req, //TODO this requires psn??
					stream<retransEntry>&			tx2retrans_insertRequest,
					stream<pointerMeta>&	ptrMetaFifo)
					//stream<event>&			retrans2event)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static retransPointerEntry ptr_table[MAX_QPS];
	#pragma HLS bind_storage variable=ptr_table type=RAM_T2P impl=BRAM
	#pragma HLS DEPENDENCE variable=ptr_table inter false

	retransRelease release;
	//static ap_uint<16> curr;
	//static ap_uint<16> newPtr;
	retransEntry insert;
	retransmission retrans;
	retransPointerEntry ptrs;
	//static retransMetaEntry meta;

	if (!rx2retrans_release_upd.empty())
	{
		//TODO lock qpn
		rx2retrans_release_upd.read(release);
		ptrs = ptr_table[release.qpn];
		//curr = ptrs.head;
		if (ptrs.valid)
		{
			ptrMetaFifo.write(pointerMeta(RELEASE, ptrs));
		}
	}
	else if (!tx2retrans_insertRequest.empty())// && !freeListFifo.empty())
	{
		//TODO lock qpn
		newPtr = freeListFifo.read();
		tx2retrans_insertRequest.read(insert);
		ptrs = ptr_table[insert.qpn];
		ptrMetaFifo.write(pointerMeta(INSERT, insert.qpn, ptrs));
	}
	else if (!rx2retrans_req.empty())
	{
		rx2retrans_req.read(retrans);
		ptrs = ptr_table[retrans.qpn];
		//curr = ptrs.head;
		if (ptrs.valid)
		{
			ptrMetaFifo.write(pointerMeta(RX_RETRANS, retrans.psn, ptrs));
		}
	}
	else if (!timer2retrans_req.empty())
	{
		timer2retrans_req.read(retrans);
		//Uses always head psn
		ptrs = ptr_table[retrans.qpn];
		if (ptrs.valid)
		{
			ptrMetaFifo.write(pointerMeta(TIMER_RETRANS, ptrs));
		}

	}
}*/

/*void retrans_meta_table(stream<pointerMeta>&	ptrMetaFifo,
						stream<pointerUpdate>&		pointerUpdateFifo,
						stream<event>&			retrans2event)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static retransMetaEntry meta_table[META_TABLE_SIZE];
	#pragma HLS bind_storage variable=meta_table type=RAM_T2P impl=BRAM
	#pragma HLS DEPENDENCE variable=meta_table inter false

	static stream<ap_uint<16> > freeListFifo("freeListFifo");
	#pragma HLS STREAM depth=META_TABLE_SIZE variable=freeListFifo

	enum retransStateType {INIT, GET_META, PROCESS};
	static retransStateType rt_state = INIT;

	static ap_uint<16> freeListCounter = 0;
	static pointerMeta ptrMeta;
	static ap_uint<16> curr;

	ap_uint<16> newPtr;
	retransMetaEntry meta;

	switch (rt_state)
	{
	case INIT:
		freeListFifo.write(freeListCounter);
		if (freeListCounter == META_TABLE_SIZE-1)
		{
			rt_state = GET_META;
		}
		freeListCounter++;
		break;
	case GET_META:
		if (!ptrMetaFifo.empty())
		{
			ptrMetaFifo.read(ptrMeta);
			curr = ptrMeta.entry.head;
			rt_state = PROCESS;
		}
		break;
	case PROCESS:
		switch(ptrMeta.entry)
		{
		case INSERT:
			if (!freeListFifo.empty())
			{
			newPtr = freeListFifo.read();
				if (!ptrMeta.entry.valid)
				{
					//init entry
					ptrMeta.entry.valid = true;
					ptrMeta.entry.head = newPtr;
					ptrMeta.entry.tail = newPtr;
				}
				else
				{
					meta_table[ptrMeta.entry.tail].next = newPtr;
					meta_table[ptrMeta.entry.tail].isTail = false;
					meta_table[newPtr] = insert;
					ptrMeta.entry.tail = newPtr;
				}
				//Write back
				pointerUpdateFifo.write(pointerUpdate(qpn, ptrMeta.entry));
				//ptr_table[insert.qpn] = ptrs;
				rt_state = GET_META;
			}
			break;
		/*case RELEASE:
			rt_state = MAIN;
			if (ptrs.valid)
			{
				meta = meta_table[curr];
				rt_state = RELEASE_1;
			}
			break;*/
		/*case RELEASE:
			meta = meta_table[curr];
			if (!meta.valid || (meta.psn ==  release.latest_acked_req))
			{
				ptrMeta.entry.head = curr;
				pointerUpdateFifo.write(qpn, ptrMeta.entry);
				//ptr_table[release.qpn] = ptrs;
				rt_state = GET_META;
			}
			else
			{
				if (meta.isTail)
				{
					ptrMeta.entry.valid = false;
					ptrMeta.entry.head = curr;
					pointerUpdateFifo.write(qpn, ptrMeta.entry);
					//ptr_table[release.qpn] = ptrs;
					rt_state = GET_META;
				}
				curr = meta.next;
				//meta = meta_table[curr];
			}
			break;
		/*case RETRANS_0:
			rt_state = MAIN;
			if (ptrs.valid)
			{
				meta = meta_table[curr];
				rt_state = RETRANS_1;
			}
			break;*/
		/*case RX_RETRANS:
			meta = meta_table[curr];
			if (!meta.valid || meta.isTail)
			{
				rt_state = GET_META;
			}
			else
			{
				if (meta.psn == ptrMeta.psn)
				{
					//Generate even
					retrans2event.write(event(meta.opCode, ptrMeta.qpn, meta.vaddr, meta.length, meta.psn));
					rt_state = GET_META;
				}
				curr = meta.next;
				meta = meta_table[curr];
			}
			break;
		case TIMER_RETRANS:
			meta = meta_table[curr];
			retrans2event.write(event(meta.opCode, ptrMeta.qpn, meta.vaddr, meta.length, meta.psn));
			break;
		}//switch entry
		break;
	}//switch state
}*/

void retrans_meta_table(stream<retransMetaReq>&		meta_upd_req,
						stream<retransMetaEntry>&		meta_rsp)
						//stream<bool>& stopFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static retransMetaEntry meta_table[META_TABLE_SIZE];
	#pragma HLS bind_storage variable=meta_table type=RAM_T2P impl=BRAM
	#pragma HLS DEPENDENCE variable=meta_table inter false
	//enum rmtFsmStateType {READ, CONTINOUS};// CONTINOUS_2, IMD};
	//static rmtFsmStateType rmt_state = READ;

	//static ap_uint<24> checkPsn;
	//static bool prevTrigger = false;
	retransMetaReq req;
	//static ap_uint<16> curr_idx;

	//static ap_uint<16> next_idx;
	//retransMetaEntry entry;
	//static bool continuousMode = false;


	//static ap_uint<1> alternater = 0;

	/*switch (alternater)
	{
	case 0:
		if (continuousMode)
		{
			std::cout << "next_idx: " << next_idx << std::endl;
			entry = meta_table[curr_idx];
			bool last = false;
			next_idx = entry.next;
			//rmt_state = IMD; //TODO stay
			if (!stopFifo.empty())
			{
				stopFifo.read();
				last = true;
				//rmt_state = READ;
				continuousMode = false;
			}
			meta_rsp.write(retransMetaRsp(entry, last));
		}
		else*/ if (!meta_upd_req.empty())
		{
			meta_upd_req.read(req);
			if (req.write)
			{
				meta_table[req.idx] = req.entry;
			}
			else if (req.append)
			{
				meta_table[req.idx].next = req.entry.next;
				meta_table[req.idx].isTail = false;
			}
			else
			{
				//entry = meta_table[req.idx];
				meta_rsp.write(meta_table[req.idx]);
				//next_idx = entry.next;
				/*if (req.continous)
				{
					continuousMode = true;
					//not sure this is necessary
					/*if (!entry.valid || entry.isTail)
					{
						next_idx = 0;
					}*/
					//rmt_state = CONTINOUS;
				//}
			}

		}
		/*alternater = 1;
		break;
	case 1:
		curr_idx = entry.next;
		alternater = 0;
		break;
	} //switch


	/*switch (rmt_state)
	{
	case READ:
		if (!meta_upd_req.empty())
		{
			meta_upd_req.read(req);
			if (req.write)
			{
				meta_table[req.idx] = req.entry;
			}
			else if (req.append)
			{
				meta_table[req.idx].next = req.entry.next;
				meta_table[req.idx].isTail = false;
			}
			else
			{
				entry = meta_table[req.idx];
				meta_rsp.write(retransMetaRsp(entry));
				next_idx = entry.next;
				if (req.continous)
				{
					//not sure this is necessary
					/*if (!entry.valid || entry.isTail)
					{
						next_idx = 0;
					}*/
					/*rmt_state = CONTINOUS;
				}
			}

		}
		break;
	case CONTINOUS:
		{
			std::cout << "next_idx: " << next_idx << std::endl;
			entry = meta_table[next_idx];
			bool last = false;
			next_idx = entry.next;
			//rmt_state = IMD; //TODO stay
			if (!stopFifo.empty())
			{
				stopFifo.read();
				last = true;
				rmt_state = READ;
			}
			meta_rsp.write(retransMetaRsp(entry, last));
			/*isEnd = (!entry.valid || entry.psn == checkPsn || entry.isTail);
			if (!entry.valid || entry.psn == checkPsn || entry.isTail)
			{
				//prevTrigger = true;
				//rmt_state = READ;
			}*/
		/*}
		break;
	/*case IMD:
		/*if (isEnd)
		{
			rmt_state = READ;
		}
		else
		{
			rmt_state = CONTINOUS;
		}*/
		/*rmt_state = CONTINOUS_2;
		break;
	/*case CONTINOUS_2:
		if (isEnd)
		{
			//prevTrigger = true;
			rmt_state = READ;
		}
		else
		{
			rmt_state = CONTINOUS;
		}
		break;*/
	//}//switch
}

//TODO HLS is failing so bad with II, such that this module has a ton of states
void process_retransmissions(	stream<retransRelease>&	rx2retrans_release_upd,
					stream<retransmission>& rx2retrans_req,
					stream<retransmission>& timer2retrans_req,
					stream<retransEntry>&	tx2retrans_insertRequest,
					stream<pointerReq>&					pointerReqFifo,
					stream<pointerUpdate>&				pointerUpdFifo,
					stream<retransPointerEntry>& 		pointerRspFifo, //TODO reorder
					stream<retransMetaReq>&				metaReqFifo,
					stream<retransMetaEntry>&			metaRspFifo,
					stream<ap_uint<16> >&				freeListFifo,
					stream<ap_uint<16> >&				releaseFifo,
					//stream<bool>&	stopFifo,
					stream<retransEvent>&				retrans2event)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	/*static retransPointerEntry ptr_table[MAX_QPS];
	#pragma HLS bind_storage variable=ptr_table type=RAM_T2P impl=BRAM
	#pragma HLS DEPENDENCE variable=ptr_table inter false*/
	/*static retransMetaEntry meta_table[META_TABLE_SIZE];
	#pragma HLS bind_storage variable=meta_table type=RAM_T2P impl=BRAM
	#pragma HLS DEPENDENCE variable=meta_table inter false*/


	enum retransStateType {MAIN, INSERT, INSERT_2, RELEASE_0, RELEASE_1, RETRANS_0, RETRANS_1, RETRANS_2, TIMER_RETRANS, TIMER_RETRANS_2}; //FLUSH_META_PIPE};
	static retransStateType rt_state = MAIN;
	//static ap_uint<16> freeListCounter = 0;
	static retransRelease release;
	static ap_uint<16> curr;
	//static ap_uint<16> relCurr;
	static ap_uint<16> newMetaIdx;
	static retransEntry insert;
	static retransmission retrans;
	static retransMetaEntry meta; //TODO register needed??
	static retransPointerEntry ptrMeta;
	//static ap_uint<16> prevIdx;

	switch (rt_state)
	{
	/*case INIT:
		//TODO move to free handler
		releaseFifo.write(freeListCounter); //TODO
		if (freeListCounter == META_TABLE_SIZE-1)
		{
			rt_state = MAIN;
		}
		freeListCounter++;
		break;*/
	case MAIN:
		if (!rx2retrans_release_upd.empty())
		{
			rx2retrans_release_upd.read(release);
			pointerReqFifo.write(pointerReq(release.qpn, true));
			rt_state = RELEASE_0;
		}
		else if (!tx2retrans_insertRequest.empty() && !freeListFifo.empty())
		{
			newMetaIdx = freeListFifo.read();
			tx2retrans_insertRequest.read(insert);
			pointerReqFifo.write(pointerReq(insert.qpn, true));
			rt_state = INSERT;
		}
		else if (!rx2retrans_req.empty())
		{
			rx2retrans_req.read(retrans);
			std::cout << "RX Retransmit triggered!! , psn: " << std::hex << retrans.psn << std::endl;
			pointerReqFifo.write(pointerReq(release.qpn));
			rt_state = RETRANS_0;
		}
		else if (!timer2retrans_req.empty())
		{
			std::cout << "TIMER Retransmit triggered!!\n";
			timer2retrans_req.read(retrans);
			//Uses always head psn
			pointerReqFifo.write(pointerReq(retrans.qpn));
			rt_state = TIMER_RETRANS;
		}
		break;
	case INSERT:
		if (!pointerRspFifo.empty())
		{
			pointerRspFifo.read(ptrMeta);
			if (!ptrMeta.valid)
			{
				//init entry
				ptrMeta.valid = true;
				ptrMeta.head = newMetaIdx;
				ptrMeta.tail = newMetaIdx;
				metaReqFifo.write(retransMetaReq(newMetaIdx, retransMetaEntry(insert)));
				pointerUpdFifo.write(pointerUpdate(insert.qpn, ptrMeta));
				rt_state = MAIN;
			}
			else
			{
				//meta_table[ptrMeta.tail].next = newPtr;
				//meta_table[ptrMeta.tail].isTail = false;
				//Append new pointer to tail
				metaReqFifo.write(retransMetaReq(ptrMeta.tail, newMetaIdx));
				ptrMeta.tail = newMetaIdx;
				rt_state = INSERT_2;
			}
			//Write back, TODO move to insert2??
			//pointerUpdFifo.write(pointerUpdate(insert.qpn, ptrMeta));
		}
		break;
	case INSERT_2:
		metaReqFifo.write(retransMetaReq(newMetaIdx, retransMetaEntry(insert)));
		pointerUpdFifo.write(pointerUpdate(insert.qpn, ptrMeta));
		//meta_table[newPtr] = insert;
		rt_state = MAIN;
		break;
	case RELEASE_0:
		if (!pointerRspFifo.empty())
		{
			std::cout << "releasing: " << release.latest_acked_req << std::endl;
			pointerRspFifo.read(ptrMeta);
			if (ptrMeta.valid)
			{
				//Get continuous stream of meta entries
				metaReqFifo.write(retransMetaReq(ptrMeta.head));
				curr = ptrMeta.head;
				//meta = meta_table[curr];
				rt_state = RELEASE_1;
			}
			else
			{
				std::cout << "RELEASE_0: invalid meta entry\n";
				//Release lock
				pointerUpdFifo.write(pointerUpdate(release.qpn, ptrMeta));
				rt_state = MAIN;
			}
		}
		break;
	case RELEASE_1:
		if (!metaRspFifo.empty())
		{
			//TODO rearrange this thing
			metaRspFifo.read(meta);

			//TODO this should never occur: meta.entry.valid
			std::cout << "meta.psn: " << meta.psn << ", latest acked req: " << release.latest_acked_req << std::endl;
			if (!meta.valid || (meta.psn == release.latest_acked_req))
			{
				if (meta.psn == release.latest_acked_req)
				{
					std::cout << "release success" << std::endl;
				}
				ptrMeta.head = meta.next;
				ptrMeta.valid = !meta.isTail;
				pointerUpdFifo.write(pointerUpdate(release.qpn, ptrMeta));
				//ptr_table[release.qpn] = ptrs;
				//stopFifo.write(true);
				rt_state = MAIN;
			}
			else
			{
				if (meta.isTail)
				{
					ptrMeta.valid = false;
					ptrMeta.head = curr;
					pointerUpdFifo.write(pointerUpdate(release.qpn, ptrMeta));
					//ptr_table[release.qpn] = ptrs;
					//stopFifo.write(true);
					rt_state = MAIN;
				}
				else
				{
               metaReqFifo.write(retransMetaReq(meta.next));
					//rt_freeListFifo.write(curr); //TODO check correctness
					//rt_state = RELEASE_2;
				}
				curr = meta.next;
			}
			if (meta.valid)
			{
				releaseFifo.write(curr);
			}
		}
		break;
	/*case RELEASE_2:
		//relCurr = relMeta.next;
		relMeta2 = meta_table[relCurr];
		rt_state = RELEASE_1;
		break;*/
	case RETRANS_0:
		if (!pointerRspFifo.empty())
		{
			pointerRspFifo.read(ptrMeta);
			//curr = ptrMeta.head;
			rt_state = MAIN;
			if (ptrMeta.valid)
			{
				//Get continuous stream of meta entries
				metaReqFifo.write(retransMetaReq(ptrMeta.head));
				curr = ptrMeta.head;
				//meta = meta_table[curr];
				rt_state = RETRANS_1;
			}
		}
		break;
	case RETRANS_1:
		//Find PSN of interest and start retransmitting
		if (!metaRspFifo.empty())
		{
			metaRspFifo.read(meta);
			if (meta.valid)
			{
				if (!meta.isTail)
				{
					metaReqFifo.write(retransMetaReq(meta.next));
				}
				else
				{
					rt_state = MAIN;
				}
				//Check if we should start retransmitting
				if (meta.psn == retrans.psn)
				{
					//Generate event
					std::cout << std::hex << "retransmitting opcode: " << meta.opCode << ", local addr: " << meta.localAddr << ", remote addr: " << meta.remoteAddr << ", length: " << meta.length << std::endl;
					retrans2event.write(retransEvent(meta.opCode, retrans.qpn, meta.localAddr, meta.remoteAddr, meta.length, meta.psn));
					if (!meta.isTail)
					{
						rt_state = RETRANS_2;
					}
				}
			}
			else
			{
				rt_state = MAIN;
			}
		}
		break;
	case RETRANS_2:
		//Retransmit everything until we reach tail
		if (!metaRspFifo.empty())
		{
			metaRspFifo.read(meta);
			if (meta.valid) // && meta.psn == retrans.psn)
			{
				if (!meta.isTail)
				{
					metaReqFifo.write(retransMetaReq(meta.next));
				}
				else
				{
					rt_state = MAIN;
				}
				//Generate event
				std::cout << std::hex << "retransmitting opcode: " << meta.opCode << ", local addr: " << meta.localAddr << ", remote addr: " << meta.remoteAddr << ", length: " << meta.length << std::endl;
				retrans2event.write(retransEvent(meta.opCode, retrans.qpn, meta.localAddr, meta.remoteAddr, meta.length, meta.psn));
			}
			else
			{
				rt_state = MAIN;
			}
		}
		break;
	case TIMER_RETRANS:
		if (!pointerRspFifo.empty())
		{
			pointerRspFifo.read(ptrMeta);
			if (ptrMeta.valid)
			{
				metaReqFifo.write(retransMetaReq(ptrMeta.head));
				rt_state = TIMER_RETRANS_2;
				//meta = meta_table[ptrMeta.head];
				//retrans2event.write(event(meta.opCode, retrans.qpn, meta.vaddr, meta.length, meta.psn));
			}
			else
			{
				rt_state = MAIN;
			}
		}
		break;
	case TIMER_RETRANS_2:
		if (!metaRspFifo.empty())
		{
			meta = metaRspFifo.read();
			if (meta.valid)
			{
				retrans2event.write(retransEvent(meta.opCode, retrans.qpn, meta.localAddr, meta.remoteAddr, meta.length, meta.psn));
			}
			rt_state = MAIN;
		}
		break;
	/*case FLUSH_META_PIPE:
		if (!metaRspFifo.empty())
		{
			metaRspFifo.read(meta);
			if (meta.last)
			{
				rt_state = MAIN;
			}
		}
		break;*/
	}//switch
}


void freelist_handler(	stream<ap_uint<16> >& rt_releaseFifo,
						stream<ap_uint<16> >& rt_freeListFifo)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static ap_uint<16> freeListCounter = 0;
	#pragma HLS reset variable=freeListCounter

	if (!rt_releaseFifo.empty())
	{
		rt_freeListFifo.write(rt_releaseFifo.read());
	}
	else if (freeListCounter < META_TABLE_SIZE && !rt_freeListFifo.full())
	{
		rt_freeListFifo.write(freeListCounter);
		freeListCounter++;
	}
}


void retransmitter(	stream<retransRelease>&	rx2retrans_release_upd,
					stream<retransmission>& rx2retrans_req,
					stream<retransmission>& timer2retrans_req,
					stream<retransEntry>&	tx2retrans_insertRequest,
					stream<retransEvent>&	retrans2event)
{
//#pragma HLS DATAFLOW
//#pragma HLS INTERFACE ap_ctrl_none register port=return
#pragma HLS INLINE

	static stream<pointerReq>				rt_pointerReqFifo("rt_pointerReqFifo");
	static stream<pointerUpdate>			rt_pointerUpdFifo("rt_pointerUpdFifo");
	static stream<retransPointerEntry> 		rt_pointerRspFifo("rt_pointerRspFifo"); //TODO reorder
	#pragma HLS STREAM depth=2 variable=rt_pointerReqFifo
	#pragma HLS STREAM depth=2 variable=rt_pointerUpdFifo
	#pragma HLS STREAM depth=2 variable=rt_pointerRspFifo

	static stream<retransMetaReq>				rt_metaReqFifo("rt_metaReqFifo");
	static stream<retransMetaEntry>			rt_metaRspFifo("rt_metaRspFifo");
	//static stream<ap_uint<16> > rt_freeListFifo("rt_freeListFifo");
	#pragma HLS STREAM depth=2 variable=rt_metaReqFifo
	#pragma HLS STREAM depth=2 variable=rt_metaRspFifo
	//static stream<bool> rt_stopFifo("rt_stopFifo");
	//#pragma HLS STREAM depth=2 variable=rt_stopFifo

	static stream<ap_uint<16> > rt_freeListFifo("rt_freeListFifo");
	#pragma HLS STREAM depth=META_TABLE_SIZE variable=rt_freeListFifo
	static stream<ap_uint<16> > rt_releaseFifo("rt_releaseFifo");
	#pragma HLS STREAM depth=2 variable=rt_releaseFifo

	freelist_handler(rt_releaseFifo, rt_freeListFifo);

	retrans_pointer_table(rt_pointerReqFifo, rt_pointerUpdFifo, rt_pointerRspFifo);

	retrans_meta_table(	rt_metaReqFifo,
						rt_metaRspFifo);
						//rt_stopFifo);

	process_retransmissions(rx2retrans_release_upd,
							rx2retrans_req,
							timer2retrans_req,
							tx2retrans_insertRequest,
							rt_pointerReqFifo,
							rt_pointerUpdFifo,
							rt_pointerRspFifo,
							rt_metaReqFifo,
							rt_metaRspFifo,
							rt_freeListFifo,
							rt_releaseFifo,
							//rt_stopFifo,
							retrans2event);
}
