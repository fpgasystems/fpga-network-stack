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

#include "crc.hpp"
#include <rocev2_config.hpp>

//TODO is packet aligned to 32bytes??
//TODO move onto memory write path or where CRC is checked
template <int WIDTH, int INSTID = 0>
void extract_icrc(	stream<net_axis<WIDTH> >&		input,
#ifdef DISABLE_CRC_CHECK
					stream<net_axis<WIDTH> >&		output)
#else
					stream<net_axis<WIDTH> >&		output,
					stream<ap_uint<32> >&	rx_crcFifo)
#endif
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum stateType {FIRST, PKG, LAST};
	static stateType ei_state = FIRST;
	static net_axis<WIDTH> ei_prevWord;
	ap_uint<32> crc;
	net_axis<WIDTH> currWord;

	switch (ei_state)
	{
	case FIRST:
		if (!input.empty())
		{
			input.read(currWord);
            //std::cout << "[ NODE: " << INSTID << ", EXTRACT_CRC(), FIRST_STATE ]:  Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
			ei_prevWord = currWord;
			ei_state = PKG;
			if (currWord.last)
			{
				ei_state = LAST;
			}
		}
		break;
	case PKG:
		if (!input.empty())
		{
			input.read(currWord);
            //std::cout << "[ NODE: " << INSTID << ", EXTRACT_CRC(), PKG STATE ]:  Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
			if (currWord.last)
			{
				if (currWord.keep[4] == 0)
				{
					ei_prevWord.last = 0x1;
					crc = currWord.data(31, 0);
#ifndef DISABLE_CRC_CHECK
					rx_crcFifo.write(crc);
#endif
					ei_state = FIRST;
				}
				else
				{
					ei_state = LAST;
				}
			}
			output.write(ei_prevWord);
			ei_prevWord = currWord;
		}
		break;
	case LAST:
        //std::cout << "[ NODE: " << INSTID << ", EXTRACT_CRC(), LAST STATE ]:  Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
		ap_uint<64> keep = ei_prevWord.keep; //this is required to make the case statement work for all widths
		switch(keep)
		{
		case 0xF:
			//This should not occur
			crc = ei_prevWord.data(31, 0);
			break;
		case 0xFF:
			crc = ei_prevWord.data(63, 32);
			ei_prevWord.keep(7,4) = 0x0;
			break;
		case 0xFFF:
			crc = ei_prevWord.data(95, 64);
			ei_prevWord.keep(11,8) = 0x0;
			break;
		case 0xFFFF:
			crc = ei_prevWord.data(127, 96);
			ei_prevWord.keep(15,12) = 0x0;
			break;
		case 0xFFFFF:
			crc = ei_prevWord.data(159, 128);
			ei_prevWord.keep(19,16) = 0x0;
			break;
		case 0xFFFFFF:
			crc = ei_prevWord.data(191, 160);
			ei_prevWord.keep(23,20) = 0x0;
			break;
		case 0xFFFFFFF:
			crc = ei_prevWord.data(223, 192);
			ei_prevWord.keep(27,24) = 0x0;
			break;
		case 0xFFFFFFFF:
			crc = ei_prevWord.data(255, 224);
			ei_prevWord.keep(31,28) = 0x0;
			break;
		case 0xFFFFFFFFF:
			crc = ei_prevWord.data(287, 256);
			ei_prevWord.keep(35,32) = 0x0;
			break;
		case 0xFFFFFFFFFF:
			crc = ei_prevWord.data(319, 288);
			ei_prevWord.keep(39,36) = 0x0;
			break;
		case 0xFFFFFFFFFFF:
			crc = ei_prevWord.data(351, 320);
			ei_prevWord.keep(43,40) = 0x0;
			break;
		case 0xFFFFFFFFFFFF:
			crc = ei_prevWord.data(383, 352);
			ei_prevWord.keep(47,44) = 0x0;
			break;
		case 0xFFFFFFFFFFFFF:
			crc = ei_prevWord.data(415, 384);
			ei_prevWord.keep(51,48) = 0x0;
			break;
		case 0xFFFFFFFFFFFFFF:
			crc = ei_prevWord.data(447, 416);
			ei_prevWord.keep(55,52) = 0x0;
			break;
		case 0xFFFFFFFFFFFFFFF:
			crc = ei_prevWord.data(479, 448);
			ei_prevWord.keep(59,56) = 0x0;
			break;
		case 0xFFFFFFFFFFFFFFFF:
			crc = ei_prevWord.data(511, 480);
			ei_prevWord.keep(63,60) = 0x0;
			break;
		} //switch
		output.write(ei_prevWord);
#ifndef DISABLE_CRC_CHECK
		rx_crcFifo.write(crc);
#endif
		ei_state = FIRST;
		break;
	}

}

/*
 * Append ICRC over GRH, IBA Headers & Payload
 * RoCEv2: Replace in IPv6: traffic class, flow label, hop_limit, udp checksum with '1'
 * BTH: ignore Resv8a
 * hop limit is always FF
 */
template <int WIDTH, int INSTID = 0>
void mask_header_fields(stream<net_axis<WIDTH> >& input,
						stream<net_axis<WIDTH> >& dataOut,
						stream<net_axis<WIDTH> >& maskedDataOut)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	//mask containig all of these fields
	const static ap_uint<424> one_mask = 0;
	// traffic class
	one_mask(3, 0) = 0xF;
	one_mask(11, 8) = 0xF;
	// flow label
	one_mask(31, 12) = 0xFFFFF;
	// hop limit TODO: should already be xFF
	one_mask(63, 56) = 0xFF;

	// UDP checksum
	one_mask(383, 368) = 0xFFFF;
	// BTH Resv8a
	one_mask(423,416) = 0xFF;
	const static ap_uint<3> header_length = (424/WIDTH);
	static ap_uint<8> ai_wordCount = 0;

	net_axis<WIDTH> crcWord;
	net_axis<WIDTH> currWord;


	if (!input.empty())
	{
		input.read(currWord);
		crcWord = currWord;

		if (ai_wordCount < header_length)
		{
            std::cout << "[ NODE: " << INSTID << ", MASK_HEADER() ]: Less than header" << std::endl;
			//std::cout << "applied mask: " << ai_wordCount << ", range: (" << std::dec << (int) ((ai_wordCount+1)*WIDTH)-1 << "," << (int) (ai_wordCount*WIDTH) << ")" << std::endl;
			crcWord.data = crcWord.data | one_mask(((ai_wordCount+1)*WIDTH)-1, ai_wordCount*WIDTH);
		}
		else if (ai_wordCount == header_length)
		{
            std::cout << "[ NODE: " << INSTID << ", MASK_HEADER() ]: Equal header" << std::endl;
			//std::cout << "aaapplied mask: " << ai_wordCount << ", range: (" << std::dec << (int) 423 << "," << (int) (ai_wordCount*WIDTH) << ")" << std::endl;
			crcWord.data((424%WIDTH)-1 , 0) = crcWord.data((424%WIDTH)-1 , 0) | one_mask(423, ai_wordCount*WIDTH);
		}
		maskedDataOut.write(crcWord);
		dataOut.write(currWord);
		ai_wordCount++;
        std::cout << "[ NODE: " << INSTID << ", MASK_HEADER() ]: Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
		if (currWord.last)
		{
            std::cout << "[ NODE: " << INSTID << ", MASK_HEADER() ]: Written last" << std::endl;
			ai_wordCount = 0;
		}
	}
}

template <int WIDTH, int INSTID = 0>
void drop_invalid_crc(	stream<net_axis<WIDTH> >& input,
						stream<ap_uint<32> >& crcFifo,
						stream<ap_uint<32> >& calcCrcFifo,
						stream<net_axis<WIDTH> >& output,
						ap_uint<32>& 	 regCrcDropPkgCount)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum fsmType {META, FWD, DROP};
	static fsmType state = META;
	static ap_uint<32> droppedPackts = 0;

	ap_uint<32> act_crc;
	ap_uint<32> calc_crc;
	net_axis<WIDTH> currWord;

	switch (state)
	{
		case META:
			if (!crcFifo.empty() && !calcCrcFifo.empty())
			{
				crcFifo.read(act_crc);
				calcCrcFifo.read(calc_crc);
				calc_crc = ~calc_crc;
				std::cout << std::hex << "actual crc: " << act_crc << " exp crc: " << calc_crc << std::endl;
				if (act_crc == calc_crc)
				{
					state = FWD;
				}
				else
				{
					droppedPackts++;
					regCrcDropPkgCount = droppedPackts;
					std::cout << "CRC PKG DROPED" << std::endl;
					state = FWD; ///TODO hack to avoid CRC drops
				}
			}
			break;
		case FWD:
			if (!input.empty())
			{
				input.read(currWord);
				output.write(currWord);
				if (currWord.last)
				{
					state = META;
				}
			}
			break;
		case DROP:
			if (!input.empty())
			{
				input.read(currWord);
				if (currWord.last)
				{
					state = META;
				}
			}
			break;
	} //switch
}

template <int WIDTH, int INSTID = 0>
void compute_crc32(	stream<net_axis<WIDTH> >& input,
					stream<ap_uint<32> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	enum crcFsmStateType {FIRST, SECOND};
	static crcFsmStateType crcState = FIRST;
	const unsigned int polynomial = 0xEDB88320; //Ethernet polynomial: 0x04C11DB7 reversed
	static unsigned int crc = 0xdebb20e3; // 8 bytes of 0xFF with init crc 0xFFFFFFFF
	//static unsigned int crc = 0xFFFFFFFF;
	static unsigned int mask = 0;

	static net_axis<WIDTH> currWord;

	switch (crcState)
	{
	case FIRST:
		if (!input.empty())
		{
			input.read(currWord);
            std::cout << "[ NODE: " << INSTID << ", CRC() ]: State FIRST, Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
			/*std::cout << "CRC32:";
			print(std::cout, currWord);
			std::cout << std::endl;*/

			//std::cout << "byte: " << std::hex << (uint16_t) byte;// << std::endl;
			for (int i = 0; i < (WIDTH/8/2); i++)
			{
				#pragma HLS UNROLL
				if (currWord.keep[i])
				{
					crc ^= currWord.data(i*8+7, i*8);
					//std::cout << std::hex << std::setw(2) << (uint16_t) currWord.data(i*8+7, i*8);// << " ";
					//std::cout << std::dec <<  ((int) currWord.data(i*8+7, i*8)) << " ";

					for (int j = 0; j < 8; j++)
					{
						#pragma HLS UNROLL
						//crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
						mask = -(crc & 1);
						crc = (crc >> 1) ^ (polynomial & mask);
					}
				}
			}
			crcState = SECOND;
		}
		break;
	case SECOND:
		for (int i = (WIDTH/8/2); i < (WIDTH/8); i++)
		{
            //std::cout << "[ NODE: " << INSTID << ", CRC() ]: State SECOND, Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
			#pragma HLS UNROLL
			if (currWord.keep[i])
			{
				crc ^= currWord.data(i*8+7, i*8);
				for (int j = 0; j < 8; j++)
				{
					#pragma HLS UNROLL
					mask = -(crc & 1);
					crc = (crc >> 1) ^ (polynomial & mask);
				}
			}
		}
		if (currWord.last)
		{
			output.write(crc);
            std::cout << "[ NODE: " << INSTID << ", CRC() ]: CRC computed " << std::hex << crc << std::dec << std::endl;
			//std::cout << std::endl;
			//std::cout << "CRC["<< DUMMY << "]: "<< std::hex << ~crc << std::endl;
			//reset
			crc = 0xdebb20e3;
			mask = 0;
		}
		crcState = FIRST;
		break;
		//std::cout << std::endl;
	} //switch

}

//packets are multiple of 4 bytes, crc is 4 bytes
template <int WIDTH, int INSTID = 0>
void insert_icrc(
#ifndef DISABLE_CRC_CHECK
					stream<ap_uint<32> >& crcIn,
#endif
					stream<net_axis<WIDTH> >& input,
					stream<net_axis<WIDTH> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

#ifndef DISABLE_CRC_CHECK
	enum fsmState {CRC, FWD, POST};
	static fsmState ii_state = CRC;
#else
	enum fsmState {FWD, POST};
	static fsmState ii_state = FWD;
#endif
	static ap_uint<32> crc = 0xdeadbeef;

	net_axis<WIDTH> currWord;
	net_axis<WIDTH> sendWord;

	switch(ii_state)
	{
#ifndef DISABLE_CRC_CHECK
	case CRC:
		if (!crcIn.empty())
		{
			crcIn.read(crc);
            std::cout << "[ NODE: " << INSTID << ", INSERT_CRC() ]:  CRC " << std::hex << crc  << std::dec << std::endl;
			crc = ~crc;
			ii_state = FWD;
		}
		break;
#endif
	case FWD:
		if (!input.empty())
		{
			input.read(currWord);
            std::cout << "[ NODE: " << INSTID << ", INSERT_CRC() ]:  Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
			if (currWord.last)
			{
				//Check if word is full
				if (currWord.keep[(WIDTH/8)-1] == 1)
				{
					currWord.last = 0;
					ii_state = POST;
				}
				else
				{
#ifndef DISABLE_CRC_CHECK
					ii_state = CRC;
#endif
					ap_uint<64> keep = currWord.keep; //this is required to make the case statement work for all widths
					switch(keep)
					{
					case 0xF:
						currWord.data(63, 32) = crc;
						currWord.keep(7,4) = 0xF;
						break;
					case 0xFF:
						currWord.data(95, 64) = crc;
						currWord.keep(11,8) = 0xF;
						break;
					case 0xFFF:
						currWord.data(127, 96) = crc;
						currWord.keep(15,12) = 0xF;
						break;
					case 0xFFFF:
						currWord.data(159, 128) = crc;
						currWord.keep(19,16) = 0xF;
						break;
					case 0xFFFFF:
						currWord.data(191, 160) = crc;
						currWord.keep(23,20) = 0xF;
						break;
					case 0xFFFFFF:
						currWord.data(223, 192) = crc;
						currWord.keep(27,24) = 0xF;
						break;
					case 0xFFFFFFF:
						currWord.data(255, 224) = crc;
						currWord.keep(31,28) = 0xF;
						break;
					case 0xFFFFFFFF:
						currWord.data(287, 256) = crc;
						currWord.keep(35,32) = 0xF;
						break;
					case 0xFFFFFFFFF:
						currWord.data(319, 288) = crc;
						currWord.keep(39,36) = 0xF;
						break;
					case 0xFFFFFFFFFF:
						currWord.data(351, 320) = crc;
						currWord.keep(43,40) = 0xF;
						break;
					case 0xFFFFFFFFFFF:
						currWord.data(383, 352) = crc;
						currWord.keep(47,44) = 0xF;
						break;
					case 0xFFFFFFFFFFFF:
						currWord.data(415, 384) = crc;
						currWord.keep(51,48) = 0xF;
						break;
					case 0xFFFFFFFFFFFFF:
						currWord.data(447, 416) = crc;
						currWord.keep(55,52) = 0xF;
						break;
					case 0xFFFFFFFFFFFFFF:
						currWord.data(479, 448) = crc;
						currWord.keep(59,56) = 0xF;
						break;
					case 0xFFFFFFFFFFFFFFF:
						currWord.data(511, 480) = crc;
						currWord.keep(63,60) = 0xF;
						break;
					//case 0xFFFFFFFF:
						//TODO should not be reached
						//break;
					} //switch
				} //keep
			} //last
			output.write(currWord);
		}
		break;
	case POST:
		sendWord.data(31, 0) = crc;
		sendWord.data(WIDTH-1, 32) = 0;
		sendWord.keep(3, 0) = 0xF;
		sendWord.keep((WIDTH/8)-1, 4) = 0;
		sendWord.last = 1;
		output.write(sendWord);
#ifndef DISABLE_CRC_CHECK
		ii_state = CRC;
#else
		ii_state = FWD;
#endif
		break;
	} //switch
}

template <int WIDTH, int INSTID = 0>
void round_robin_arbiter(stream<net_axis<WIDTH> >& in,
						stream<net_axis<WIDTH> >& out1,
						stream<net_axis<WIDTH> >& out2)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static bool one = true;
	net_axis<WIDTH> currWord;

	if (!in.empty())
	{
		in.read(currWord);
		if (one)
		{
			out1.write(currWord);
            //std::cout << "[ NODE: " << INSTID << ", ROUND_ROBIN_ARBITER() ]: Send to 1, Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
		}
		else
		{
			out2.write(currWord);
            //std::cout << "[ NODE: " << INSTID << ", ROUND_ROBIN_ARBITER() ]: Send to 2, Data " << std::hex << currWord.data << ", Last " << currWord.last << std::dec << std::endl;
		}
		if (currWord.last)
		{
			one = !one;
            //std::cout << "[ NODE: " << INSTID << ", ROUND_ROBIN_ARBITER() ]: Swap" << std::endl;
		}
	}
}

template <int INSTID = 0>
void round_robin_merger(stream<ap_uint<32> >& in1,
						stream<ap_uint<32> >& in2,
						stream<ap_uint<32> >& out)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

	static bool one_m = true;

	if (one_m)
	{
		if (!in1.empty())
		{
			//std::cout << "[ NODE: " << INSTID << ", ROUND_ROBIN_MERGER() ]: Merge from 1" << std::endl;
            out.write(in1.read());
            
			one_m = !one_m;
		}
	}
	else
	{
		if (!in2.empty())
		{
			//std::cout << "[ NODE: " << INSTID << ", ROUND_ROBIN_MERGER() ]: Merge from 2" << std::endl;
            out.write(in2.read());
            
			one_m = !one_m;
		}
	}
}

template <int WIDTH, int INSTID = 0>
void crc(	
	// RX
	stream<net_axis<WIDTH> >&   s_axis_rx_data,
	stream<net_axis<WIDTH> >&   m_axis_rx_data,

    stream<net_axis<WIDTH> >&   s_axis_tx_data,
	stream<net_axis<WIDTH> >&   m_axis_tx_data,

	ap_uint<32>&                regCrcDropPkgCount
) {
#pragma HLS INLINE

	/*
	 * RX
	 */
#ifndef DISABLE_CRC_CHECK
	static stream<net_axis<WIDTH> > rx_dataFifo("rx_dataFifo");
	static stream<net_axis<WIDTH> > rx_maskedDataFifo("rx_maskedDataFifo");
	static stream<net_axis<WIDTH> > rx_maskedDataFifo1("rx_maskedDataFifo1");
	static stream<net_axis<WIDTH> > rx_maskedDataFifo2("rx_maskedDataFifo2");
	static stream<net_axis<WIDTH> > rx_crcDataFifo("rx_crcDataFifo");
	static stream<ap_uint<32> > rx_crcFifo("rx_crcFifo");
	static stream<ap_uint<32> > rx_calcCrcFifo("rx_calcCrcFifo");
	static stream<ap_uint<32> > rx_calcCrcFifo1("rx_calcCrcFifo1");
	static stream<ap_uint<32> > rx_calcCrcFifo2("rx_calcCrcFifo2");
	#pragma HLS STREAM depth=4 variable=rx_dataFifo
	#pragma HLS STREAM depth=4 variable=rx_maskedDataFifo
	#pragma HLS STREAM depth=256 variable=rx_maskedDataFifo1 //TODO maybe increase for better TP
	#pragma HLS STREAM depth=256 variable=rx_maskedDataFifo2
	#pragma HLS STREAM depth=512 variable=rx_crcDataFifo // 1536 bytes, 48 for WIDTH = 256
	#pragma HLS STREAM depth=32 variable=rx_crcFifo
	#pragma HLS STREAM depth=32 variable=rx_calcCrcFifo
	#pragma HLS STREAM depth=2 variable=rx_calcCrcFifo1
	#pragma HLS STREAM depth=2 variable=rx_calcCrcFifo2
#endif

#ifdef DISABLE_CRC_CHECK
	regCrcDropPkgCount = 0;
	extract_icrc<WIDTH, INSTID>(s_axis_rx_data, m_axis_rx_data);
#else
	extract_icrc<WIDTH, INSTID>(s_axis_rx_data, rx_dataFifo, rx_crcFifo);
	mask_header_fields<WIDTH, INSTID>(rx_dataFifo, rx_crcDataFifo, rx_maskedDataFifo);
	round_robin_arbiter<WIDTH, INSTID>(rx_maskedDataFifo, rx_maskedDataFifo1, rx_maskedDataFifo2);
	compute_crc32<WIDTH, INSTID>(rx_maskedDataFifo1, rx_calcCrcFifo1);
	compute_crc32<WIDTH, INSTID>(rx_maskedDataFifo2, rx_calcCrcFifo2);
	round_robin_merger<INSTID>(rx_calcCrcFifo1, rx_calcCrcFifo2, rx_calcCrcFifo);
	drop_invalid_crc<WIDTH, INSTID>(rx_crcDataFifo, rx_crcFifo, rx_calcCrcFifo, m_axis_rx_data, regCrcDropPkgCount);
#endif

	/*
	 * TX
	 */
#ifndef DISABLE_CRC_CHECK
	static stream<net_axis<WIDTH> > tx_maskedDataFifo("tx_maskedDataFifo");
	static stream<net_axis<WIDTH> > tx_maskedDataFifo1("tx_maskedDataFifo1");
	static stream<net_axis<WIDTH> > tx_maskedDataFifo2("tx_maskedDataFifo2");
	static stream<net_axis<WIDTH> > tx_crcDataFifo("tx_crcDataFifo");
	static stream<ap_uint<32> > crcFifo("crcFifo");
	static stream<ap_uint<32> > crcFifo1("crcFifo1");
	static stream<ap_uint<32> > crcFifo2("crcFifo2");
	#pragma HLS STREAM depth=4 variable=tx_maskedDataFifo
	#pragma HLS STREAM depth=128 variable=tx_maskedDataFifo1 //increase size for better TP
	#pragma HLS STREAM depth=128 variable=tx_maskedDataFifo2
	#pragma HLS STREAM depth=384 variable=tx_crcDataFifo // 1536 bytes, 48 for WIDTH = 256
	#pragma HLS STREAM depth=2 variable=crcFifo
	#pragma HLS STREAM depth=2 variable=crcFifo1
	#pragma HLS STREAM depth=2 variable=crcFifo2
#endif

#ifdef DISABLE_CRC_CHECK
	insert_icrc<WIDTH, INSTID>(s_axis_tx_data, m_axis_tx_data);
#else
	mask_header_fields<WIDTH, INSTID>(s_axis_tx_data, tx_crcDataFifo, tx_maskedDataFifo);
	round_robin_arbiter<WIDTH, INSTID>(tx_maskedDataFifo, tx_maskedDataFifo1, tx_maskedDataFifo2);
	compute_crc32<WIDTH, INSTID>(tx_maskedDataFifo1, crcFifo1);
	compute_crc32<WIDTH, INSTID>(tx_maskedDataFifo2, crcFifo2);
	round_robin_merger<INSTID>(crcFifo1, crcFifo2, crcFifo);
	insert_icrc<WIDTH, INSTID>(crcFifo, tx_crcDataFifo, m_axis_tx_data);
#endif

}

#define crc_spec_decla(ninst)                                   \
template void crc<DATA_WIDTH, ninst>(		    	            \
	stream<net_axis<DATA_WIDTH> >& s_axis_rx_data,		        \
    stream<net_axis<DATA_WIDTH> >& m_axis_rx_data,		        \
    stream<net_axis<DATA_WIDTH> >& s_axis_tx_data,		        \
    stream<net_axis<DATA_WIDTH> >& m_axis_tx_data,		        \
	ap_uint<32>& regCrcDropPkgCount		                        \
);

crc_spec_decla(0);
crc_spec_decla(1);