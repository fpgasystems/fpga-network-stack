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
#pragma once

#include <hls_stream.h>
#include "ap_int.h"
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "ap_axi_sdata.h"
//Adaptation of ap_axiu<>

/*template <int D>
struct axis
{
	ap_uint<D>		data;
	ap_uint<D/8>	keep;
	ap_uint<1>		last;
	axis() {}
	axis(ap_uint<D> data, ap_uint<D/8> keep, ap_uint<1> last)
		:data(data), keep(keep), last(last) {}
};*/

template <int D>
struct net_axis
{
	ap_uint<D>		data;
	ap_uint<D/8>	keep;
	ap_uint<1>		last;
	net_axis() {}
	net_axis(ap_uint<D> data, ap_uint<D/8> keep, ap_uint<1> last)
		:data(data), keep(keep), last(last) {}
};

template <int D, int R=1>
struct routed_net_axis
{
	ap_uint<D>		data;
	ap_uint<D/8>	keep;
	ap_uint<1>		last;
	ap_uint<R>		dest;
	routed_net_axis() {}
	routed_net_axis(net_axis<D> w, ap_uint<R> r)
		:data(w.data), keep(w.keep), last(w.last), dest(r) {}
};

template <int WIDTH>
void convert_net_axis_to_axis(hls::stream<net_axis<WIDTH> >& input,
							hls::stream<ap_axiu<WIDTH, 0, 0, 0> >& output)
{
#pragma HLS pipeline II=1

	net_axis<WIDTH> inputWord;
	ap_axiu<WIDTH, 0, 0, 0> outputWord;

	if (!input.empty())
	{
		inputWord = input.read();
		outputWord.data = inputWord.data;
		outputWord.keep = inputWord.keep;
		outputWord.last = inputWord.last;
		output.write(outputWord);
	}
}


template <int WIDTH>
void convert_axis_to_net_axis(hls::stream<ap_axiu<WIDTH, 0, 0, 0> >& input,
							hls::stream<net_axis<WIDTH> >& output)
{
#pragma HLS pipeline II=1

	ap_axiu<WIDTH, 0, 0, 0> inputWord;
	net_axis<WIDTH> outputWord;
	
	if (!input.empty())
	{
		inputWord = input.read();
		outputWord.data = inputWord.data;
		outputWord.keep = inputWord.keep;
		outputWord.last = inputWord.last;
		output.write(outputWord);
	}
}

template <int WIDTH, int DST=1>
void convert_routed_net_axis_to_axis(hls::stream<routed_net_axis<WIDTH, DST> >& input,
							hls::stream<ap_axiu<WIDTH, 0, 0, DST> >& output)
{
#pragma HLS pipeline II=1

	routed_net_axis<WIDTH, DST> inputWord;
	ap_axiu<WIDTH, 0, 0, DST> outputWord;

	if (!input.empty())
	{
		inputWord = input.read();
		outputWord.data = inputWord.data;
		outputWord.keep = inputWord.keep;
		outputWord.last = inputWord.last;
		outputWord.dest = inputWord.dest;
		output.write(outputWord);
	}
}

template<int D>
ap_uint<D> reverse(const ap_uint<D>& w)
{
	ap_uint<D> temp;
	for (int i = 0; i < D/8; i++)
	{
		#pragma HLS UNROLL
		temp(i*8+7, i*8) = w(D-(i*8)-1, D-(i*8)-8);
	}
	return temp;
}

template<int D>
ap_uint<D> reverse_bits(const ap_uint<D>& w)
{
	ap_uint<D> temp;
	for (int i = 0; i < D; i++)
	{
		#pragma HLS UNROLL
		temp[i] = w[D-i-1];
	}
	return temp;
}

template<int D>
bool scan(std::istream& inputFile, ap_uint<D>& data)
{
	uint16_t temp;
	for (int i = 0; i < D/8; i++)
	{
		if (inputFile >> std::hex >> temp)
		{
			data(i*8+7, i*8) = temp;
		}
		else
		{
			//std::cerr << "[ERROR]: could not scan input" << std::endl;
			return false;
		}
	}
	return (bool) inputFile;
}

template<int D>
bool scan(std::istream& inputFile, net_axis<D>& word)
{
	uint16_t temp;
	uint32_t keepTemp;
	uint16_t lastTemp;
	for (int i = 0; i < D/8; i++)
	{
		if (inputFile >> std::hex >> temp)
		{
			word.data(i*8+7, i*8) = temp;
		}
		else
		{
			//std::cerr << "[ERROR]: could not scan input" << std::endl;
			return false;
		}
	}
	inputFile >> keepTemp;
	inputFile >> lastTemp;
	word.keep = keepTemp;
	word.last = lastTemp;
	//if (!inputFile)
	//	std::cerr << "[ERROR]: could not scan input" << std::endl;
	return (bool) inputFile;
}

template<int D>
bool scanLE(std::istream& inputFile, ap_uint<D>& data)
{
	uint16_t temp;
	for (int i = (D/8)-1; i >= 0; i--)
	{
		if (inputFile >> std::hex >> temp)
		{
			data(i*8+7, i*8) = temp;
		}
		else
		{
			//std::cerr << "[ERROR]: could not scan input" << std::endl;
			return false;
		}
	}
	return (bool) inputFile;
}

template<int D>
bool scanLE(std::istream& inputFile, net_axis<D>& word)
{
	uint16_t temp;
	uint32_t keepTemp;
	uint16_t lastTemp;
	for (int i = (D/8)-1; i >= 0; i--)
	{
		if (inputFile >> std::hex >> temp)
		{
			word.data(i*8+7, i*8) = temp;
		}
		else
		{
			//std::cerr << "[ERROR]: could not scan input" << std::endl;
			return false;
		}
	}
	inputFile >> keepTemp;
	inputFile >> lastTemp;
	word.keep = keepTemp;
	word.last = lastTemp;
	return (bool) inputFile;
}

template<int D>
void print(std::ostream& output, ap_uint<D> data)
{
	output << std::hex;
	output << std::setfill('0');
	for (int i = 0; i < D/8; i++)
	{
		output << std::noshowbase << std::setw(2) << (uint16_t) data(i*8+7, i*8) << " ";
	}
}

template<int D>
void print(std::ostream& output, net_axis<D>& word)
{
#ifndef __SYNTHESIS__
	output << std::hex;
	output << std::setfill('0');
	for (int i = 0; i < D/8; i++)
	{
		output << std::noshowbase << std::setw(2) << (uint16_t) word.data(i*8+7, i*8) << " ";
	}
	output << std::setw(D/8/4) << (uint64_t) word.keep << " ";
	output << std::setw(1) << (uint16_t)word.last;
#endif
}

template<int D>
void printLE(std::ostream& output, ap_uint<D>& data)
{
#ifndef __SYNTHESIS__
	output << std::hex;
	output << std::setfill('0') ;
	for (int i = (D/8)-1; i >= 0; i--)
	{
		output << std::noshowbase << std::setw(2) << (uint16_t) data(i*8+7, i*8) << " ";
	}
#endif
}

template<int D>
void printLE(std::ostream& output, net_axis<D>& word)
{
#ifndef __SYNTHESIS__
	output << std::hex;
	output << std::setfill('0') ;
	for (int i = (D/8)-1; i >= 0; i--)
	{
		output << std::noshowbase << std::setw(2) << (uint16_t) word.data(i*8+7, i*8) << " ";
	}
	output << std::setw(D/8/4) << (uint64_t) word.keep << " ";
	output << std::setw(1) << (uint16_t)word.last;
#endif
}

template<int D, int R>
void printLE(std::ostream& output, routed_net_axis<D, R>& word)
{
#ifndef __SYNTHESIS__
	output << std::hex;
	output << std::setfill('0') ;
	for (int i = (D/8)-1; i >= 0; i--)
	{
		output << std::noshowbase << std::setw(2) << (uint16_t) word.data(i*8+7, i*8) << " ";
	}
	output << std::setw(D/8/4) << (uint64_t) word.keep << " ";
	output << std::setw(1) << (uint16_t)word.last;
	output << std::setw(R) << " TDEST:" << (uint16_t)word.dest;
#endif
}

template <int W, int D, int DUMMY>
void increaseStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W*D> >&output)
{
#pragma HLS INLINE

	static int count = 0;

	static net_axis<W*D> temp;

	if (!input.empty())
	{
		net_axis<W> currWord = input.read();
		temp.data((W*count)+W-1, (W*count)) = currWord.data;
		temp.keep(((W/8)*count+(W/8)-1), ((W/8)*count)) = currWord.keep;
		temp.last = currWord.last;

		count++;
		if (currWord.last || count == D)
		{
			output.write(temp);
			count = 0;
#ifndef __SYNTHESIS__
			temp.data = 0;
#endif
			temp.keep = 0;
		}
	}

}

template <int W, int D, int DUMMY>
void reduceStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W/D> >&output)
{
#pragma HLS INLINE

	enum fsmStateType {FIRST, SECOND};
	static fsmStateType fsmState = FIRST;
	static int count = 0;

	static net_axis<W> currWord;
	net_axis<W/D> temp;

	switch (fsmState)
	{
		case FIRST:
			if (!input.empty())
			{
				input.read(currWord);
				temp.data = currWord.data((W/D)-1, 0);
				temp.keep = currWord.keep(((W/D)/8)-1, 0);
				temp.last = (currWord.keep[(W/8)/D] == 0); //(currWord.keep((W/8)-1, (W/8)/2) == 0);
				output.write(temp);

				if (currWord.keep[(W/8)/D])
				{
					count = 1;
					fsmState = SECOND;
				}

				//shift word
				currWord.data(W-(W/D)-1, 0) = currWord.data(W-1, W/D);
				currWord.keep((W/8)-((W/8)/D)-1, 0) = currWord.keep((W/8)-1, (W/8)/D);
				currWord.keep((W/8)-1, (W/8)-((W/8)/D)) = 0;
			}
			break;
		case SECOND:
			temp.data = currWord.data((W/D)-1, 0);
			temp.keep = currWord.keep(((W/D)/8)-1, 0);
			if (count < D-1)
			{
				temp.last = (currWord.keep[(W/8)/D] == 0); //(currWord.keep((W/8)-1, (W/8)/2) == 0);
			}
			else
			{
				temp.last = currWord.last;
			}
			output.write(temp);
			//shift word
			currWord.data(W-(W/D)-1, 0) = currWord.data(W-1, W/D);
			currWord.keep((W/8)-((W/8)/D)-1, 0) = currWord.keep((W/8)-1, (W/8)/D);
			currWord.keep((W/8)-1, (W/8)-((W/8)/D)) = 0;


			count++;
			if (count == D || temp.last)
			{
				
				fsmState = FIRST;
			}
			break;
	}
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	if (!input.empty())
	{
		output.write(input.read());
	}
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W*2> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	increaseStreamWidth<W,2,DUMMY>(input, output);
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W*4> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	increaseStreamWidth<W,4,DUMMY>(input, output);
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W*8> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	increaseStreamWidth<W,8,DUMMY>(input, output);
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W/2> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	reduceStreamWidth<W,2,DUMMY>(input, output);
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W/4> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	reduceStreamWidth<W,4,DUMMY>(input, output);
}

template <int W, int DUMMY>
void convertStreamWidth(hls::stream<net_axis<W> >& input, hls::stream<net_axis<W/8> >&output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	reduceStreamWidth<W,8,DUMMY>(input, output);
}


template <class T>
void assignDest(T& d, T& s) {}

template <>
void assignDest<routed_net_axis<64> >(routed_net_axis<64>& d, routed_net_axis<64>& s);
template <>
void assignDest<routed_net_axis<128> >(routed_net_axis<128>& d, routed_net_axis<128>& s);
template <>
void assignDest<routed_net_axis<256> >(routed_net_axis<256>& d, routed_net_axis<256>& s);
template <>
void assignDest<routed_net_axis<512> >(routed_net_axis<512>& d, routed_net_axis<512>& s);

// The 2nd template parameter is a hack to use this function multiple times
template <typename T, int W, int whatever>
void rshiftWordByOctet(	uint16_t offset,
						hls::stream<T>& input,
						hls::stream<T>& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1 //TODO this has a bug, the bug might come from how it is used

	enum fsmStateType {PKG, REMAINDER};
	static fsmStateType fsmState = PKG;
	static bool rs_firstWord = (offset != 0);
	static T prevWord;

	T currWord;
	T sendWord;

	sendWord.last = 0;
	switch (fsmState)
	{
	case PKG:
		if (!input.empty())
		{
			input.read(currWord);

			if (!rs_firstWord)
			{
				if (offset == 0)
				{
					sendWord = currWord;
				}
				else
				{
					sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
					sendWord.data((W-1), W-(8*offset)) = currWord.data((8*offset)-1, 0);

					sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
					sendWord.keep((W/8-1), (W/8)-offset) = currWord.keep(offset-1, 0);

					sendWord.last = (currWord.keep((W/8-1), offset) == 0);
					//sendWord.dest = currWord.dest;
					assignDest(sendWord, currWord);
				}//else offset
				output.write(sendWord);
			}

			prevWord = currWord;
			rs_firstWord = false;
			if (currWord.last)
			{
				rs_firstWord = (offset != 0);
				//rs_writeRemainder = (sendWord.last == 0);
				if (!sendWord.last)
				{
					fsmState = REMAINDER;
				}
			}
			//}//else offset
		}
		break;
	case REMAINDER:
		sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
		sendWord.data((W-1), W-(8*offset)) = 0;
		sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
		sendWord.keep((W/8-1), (W/8)-offset) = 0;
		sendWord.last = 1;
		//sendWord.dest = prevWord.dest;
		assignDest(sendWord, currWord);

		output.write(sendWord);
		fsmState = PKG;
		break;
	}
}

// The 2nd template parameter is a hack to use this function multiple times
template <int W, int whatever>
void lshiftWordByOctet(	uint16_t offset,
						hls::stream<net_axis<W> >& input,
						hls::stream<net_axis<W> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1
	static bool ls_firstWord = true;
	static bool ls_writeRemainder = false;
	static net_axis<W> prevWord;

	net_axis<W> currWord;
	net_axis<W> sendWord;

	//TODO use states
	if (ls_writeRemainder)
	{
		sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
		sendWord.data((W-1), (8*offset)) = 0;
		sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
		sendWord.keep((W/8-1), offset) = 0;
		sendWord.last = 1;

		output.write(sendWord);
		ls_writeRemainder = false;
	}
	else if (!input.empty())
	{
		input.read(currWord);

		if (offset == 0)
		{
			output.write(currWord);
		}
		else
		{

			if (ls_firstWord)
			{
				sendWord.data((8*offset)-1, 0) = 0;
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);
				sendWord.keep(offset-1, 0) = 0xFFFFFFFF;
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);
				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);
			}
			else
			{
				sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);

				sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);

				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);

			}
			output.write(sendWord);

			prevWord = currWord;
			ls_firstWord = false;
			if (currWord.last)
			{
				ls_firstWord = true;
				ls_writeRemainder = !sendWord.last;
			}
		} //else offset
	}

}

//TODO move to utils
template <typename T>
void stream_merger(hls::stream<T>& in1, hls::stream<T>& in2, hls::stream<T>& out)
{
#pragma HLS PIPELINE II=1
#pragma HLS inline off

	if (!in1.empty())
	{
		out.write(in1.read());
	}
	else if (!in2.empty())
	{
		out.write(in2.read());
	}
}

template <typename T>
void stream_merger(	hls::stream<ap_uint<1> >&	originIn,
					hls::stream<T>&	input0,
					hls::stream<T>&	input1,
					hls::stream<T>&	output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum stateType {IDLE, FWD0, FWD1};
	static stateType state = IDLE;
	T word;
	ap_uint<1> origin;

	switch (state)
	{
	case IDLE:
		if (!originIn.empty())
		{
			originIn.read(origin);
			if (origin == 0)
			{
				if (!input0.empty())
				{
					input0.read(word);
					output.write(word);
				}
				else
				{
					state = FWD0;
				}
			}
			else
			{
				if (!input1.empty())
				{
					input1.read(word);
					output.write(word);
				}
				else
				{
					state = FWD1;
				}
			}
		}
		break;
	case FWD0:
		if (!input0.empty())
		{
			input0.read(word);
			output.write(word);
			state = IDLE;
		}
		break;
	case FWD1:
		if (!input1.empty())
		{
			input1.read(word);
			output.write(word);
			state = IDLE;
		}
		break;
	}//switch
}

template <class T, int DUMMY>
void fair_merger(hls::stream<T>& in0, hls::stream<T>& in1, hls::stream<T>& out)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	static bool merge_pref = true;

	if (merge_pref)
	{
		if (!in0.empty())
		{
			out.write(in0.read());
			merge_pref = false;
		}
		else if(!in1.empty())
		{
			out.write(in1.read());
		}
	}
	else
	{
		if(!in1.empty())
		{
			out.write(in1.read());
			merge_pref = true;
		}
		else if (!in0.empty())
		{
			out.write(in0.read());
		}
	}
}

template <int W>
void fair_pkg_merger(hls::stream<net_axis<W> >& in0, hls::stream<net_axis<W> >& in1, hls::stream<net_axis<W> >& out)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum fpmStateType{IDLE, FWD0, FWD1};
	static fpmStateType state = IDLE;
	static bool merge_pref = true;
	net_axis<W> currWord;

	currWord.last = 0;
	switch (state)
	{
	case IDLE:
		if (merge_pref)
		{
			if (!in0.empty())
			{
				in0.read(currWord);
				out.write(currWord);
				state = FWD1;

			}
			else if(!in1.empty())
			{
				in1.read(currWord);
				out.write(currWord);
				state = FWD1;
			}
		}
		else
		{
			if(!in1.empty())
			{
				in1.read(currWord);
				out.write(currWord);
				state = FWD1;

			}
			else if (!in0.empty())
			{
				in0.read(currWord);
				out.write(currWord);
				state = FWD0;
			}
		}
		if (currWord.last)
		{
			state = IDLE;
		}
		break;
	case FWD0:
		if (!in0.empty())
		{
			in0.read(currWord);
			out.write(currWord);
			if (currWord.last)
			{
				merge_pref = false;
				state = IDLE;
			}
		}
		break;
	case FWD1:
		if (!in1.empty())
		{
			in1.read(currWord);
			out.write(currWord);
			if (currWord.last)
			{
				merge_pref = true;
				state = IDLE;
			}
		}
		break;
	}//switch
}

template <int W>
void stream_pkg_merger(	hls::stream<ap_uint<1> >&	originIn,
						hls::stream<net_axis<W> >&	input0,
						hls::stream<net_axis<W> >&	input1,
						hls::stream<net_axis<W> >&	output)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum stateType {IDLE, FWD0, FWD1};
	static stateType state = IDLE;
	net_axis<W> currWord;
	ap_uint<1> origin;

	switch (state)
	{
	case IDLE:
		if (!originIn.empty())
		{
			originIn.read(origin);
			if (origin == 0)
			{
				if (!input0.empty())
				{
					input0.read(currWord);
					output.write(currWord);
					if (!currWord.last)
					{
						state = FWD0;
					}
				}
				else
				{
					state = FWD0;
				}
			}
			else
			{
				if (!input1.empty())
				{
					input1.read(currWord);
					output.write(currWord);
					if (!currWord.last)
					{
						state = FWD1;
					}
				}
				else
				{
					state = FWD1;
				}
			}
		}
		break;
	case FWD0:
		if (!input0.empty())
		{
			input0.read(currWord);
			output.write(currWord);
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	case FWD1:
		if (!input1.empty())
		{
			input1.read(currWord);
			output.write(currWord);
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	}//switch
}

template <int W>
void stream_pkg_splitter(	hls::stream<ap_uint<1> >&	destIn,
							hls::stream<net_axis<W> >&	input,
							hls::stream<net_axis<W> >&	output0,
							hls::stream<net_axis<W> >&	output1)
{
#pragma HLS PIPELINE II=1
#pragma HLS INLINE off

	enum stateType {IDLE, FWD0, FWD1};
	static stateType state = IDLE;
	net_axis<W> currWord;
	ap_uint<1> dest;

	switch (state)
	{
	case IDLE:
		if (!destIn.empty())
		{
			destIn.read(dest);
			state = (dest == 0) ? FWD0 : FWD1;
			if (!input.empty())
			{
				input.read(currWord);
				if (dest == 0)
				{
					output0.write(currWord);
				}
				else
				{
					output1.write(currWord);

				}
				if (currWord.last)
				{
					state = IDLE;
				}
			}
		}
		break;
	case FWD0:
		if (!input.empty())
		{
			input.read(currWord);
			output0.write(currWord);
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	case FWD1:
		if (!input.empty())
		{
			input.read(currWord);
			output1.write(currWord);
			if (currWord.last)
			{
				state = IDLE;
			}
		}
		break;
	}//switch
}


template <int W>
void pass_valid_pkg(hls::stream<bool>&				pkgValidIn,
					hls::stream<net_axis<W> >&		input,
					hls::stream<net_axis<W> >&		output)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	enum fsmStateType {VALID, FWD, DROP};
	static fsmStateType state = VALID;

	switch (state)
	{
	case VALID:
		if (!pkgValidIn.empty() && !input.empty())
		{
			bool valid = pkgValidIn.read();
			net_axis<W> word = input.read();

			if (valid)
			{
				output.write(word);
				if (!word.last)
				{
					state = FWD;
				}
			}
			else
			{
				if (!word.last)
				{
					state = DROP;
				}
			}
			
		}
		break;
	case FWD:
		if (!input.empty())
		{
			net_axis<W> word = input.read();
			output.write(word);
			if (word.last)
			{
				state = VALID;
			}
		}
		break;
	case DROP:
		if (!input.empty())
		{
			net_axis<W> word = input.read();
			if (word.last)
			{
				state = VALID;
			}
		}
		break;
	} //switch
}


template <class T>
void toe_duplicate_stream(	hls::stream<T>&	in,
								hls::stream<T>&	out0,
								hls::stream<T>& 	out1)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	if (!in.empty())
	{
		T item = in.read();
		out0.write(item);
		out1.write(item);
	}
}

template <class T>
void ip_handler_duplicate_stream(	hls::stream<T>&	in,
								hls::stream<T>&	out0,
								hls::stream<T>& 	out1)
{
	#pragma HLS PIPELINE II=1
	#pragma HLS INLINE off

	if (!in.empty())
	{
		T item = in.read();
		out0.write(item);
		out1.write(item);
	}
}


ap_uint<64> lenToKeep(ap_uint<6> length);
ap_uint<8> keepToLen(ap_uint<64> keepValue);

template<int WIDTH>
net_axis<WIDTH> alignWords(ap_uint<6> offset, net_axis<WIDTH>	prevWord, net_axis<WIDTH> currWord)
{

   net_axis<WIDTH> alignedWord;

		alignedWord.data(WIDTH-1, WIDTH - (offset*8)) = currWord.data(offset*8-1, 0);
		alignedWord.keep(WIDTH/8-1, WIDTH/8 - offset) = currWord.keep(offset - 1, 0);
		alignedWord.data(WIDTH - (offset*8) -1, 0) = prevWord.data(WIDTH-1, offset*8);
		alignedWord.keep(WIDTH/8 - offset - 1, 0)  = prevWord.keep(WIDTH/8-1, offset);
		//alignedWord.last = (currWord.keep[offset] == 0);

   return alignedWord;
}


// SOLUTION NEEDED --------------------------------------------------------------------------------------------------------------------------------------------------

// The 2nd template parameter is a hack to use this function multiple times
template <int W, int whatever>
void udp_lshiftWordByOctet(	uint16_t offset,
						hls::stream<net_axis<W> >& input,
						hls::stream<net_axis<W> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1
	static bool ls_firstWord = true;
	static bool ls_writeRemainder = false;
	static net_axis<W> prevWord;

	net_axis<W> currWord;
	net_axis<W> sendWord;

	//TODO use states
	if (ls_writeRemainder)
	{
		sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
		sendWord.data((W-1), (8*offset)) = 0;
		sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
		sendWord.keep((W/8-1), offset) = 0;
		sendWord.last = 1;

		output.write(sendWord);
		ls_writeRemainder = false;
	}
	else if (!input.empty())
	{
		input.read(currWord);

		if (offset == 0)
		{
			output.write(currWord);
		}
		else
		{

			if (ls_firstWord)
			{
				sendWord.data((8*offset)-1, 0) = 0;
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);
				sendWord.keep(offset-1, 0) = 0xFFFFFFFF;
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);
				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);
			}
			else
			{
				sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);

				sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);

				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);

			}
			output.write(sendWord);

			prevWord = currWord;
			ls_firstWord = false;
			if (currWord.last)
			{
				ls_firstWord = true;
				ls_writeRemainder = !sendWord.last;
			}
		} //else offset
	}

}

// The 2nd template parameter is a hack to use this function multiple times
template <int W, int whatever>
void mac_lshiftWordByOctet(	uint16_t offset,
						hls::stream<net_axis<W> >& input,
						hls::stream<net_axis<W> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1
	static bool ls_firstWord = true;
	static bool ls_writeRemainder = false;
	static net_axis<W> prevWord;

	net_axis<W> currWord;
	net_axis<W> sendWord;

	//TODO use states
	if (ls_writeRemainder)
	{
		sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
		sendWord.data((W-1), (8*offset)) = 0;
		sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
		sendWord.keep((W/8-1), offset) = 0;
		sendWord.last = 1;

		output.write(sendWord);
		ls_writeRemainder = false;
	}
	else if (!input.empty())
	{
		input.read(currWord);

		if (offset == 0)
		{
			output.write(currWord);
		}
		else
		{

			if (ls_firstWord)
			{
				sendWord.data((8*offset)-1, 0) = 0;
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);
				sendWord.keep(offset-1, 0) = 0xFFFFFFFF;
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);
				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);
			}
			else
			{
				sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);

				sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);

				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);

			}
			output.write(sendWord);

			prevWord = currWord;
			ls_firstWord = false;
			if (currWord.last)
			{
				ls_firstWord = true;
				ls_writeRemainder = !sendWord.last;
			}
		} //else offset
	}

}

// The 2nd template parameter is a hack to use this function multiple times
template <int W, int whatever>
void ipv4_lshiftWordByOctet(	uint16_t offset,
						hls::stream<net_axis<W> >& input,
						hls::stream<net_axis<W> >& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1
	static bool ls_firstWord = true;
	static bool ls_writeRemainder = false;
	static net_axis<W> prevWord;

	net_axis<W> currWord;
	net_axis<W> sendWord;

	//TODO use states
	if (ls_writeRemainder)
	{
		sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
		sendWord.data((W-1), (8*offset)) = 0;
		sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
		sendWord.keep((W/8-1), offset) = 0;
		sendWord.last = 1;

		output.write(sendWord);
		ls_writeRemainder = false;
	}
	else if (!input.empty())
	{
		input.read(currWord);

		if (offset == 0)
		{
			output.write(currWord);
		}
		else
		{

			if (ls_firstWord)
			{
				sendWord.data((8*offset)-1, 0) = 0;
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);
				sendWord.keep(offset-1, 0) = 0xFFFFFFFF;
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);
				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);
			}
			else
			{
				sendWord.data((8*offset)-1, 0) = prevWord.data((W-1), W-(8*offset));
				sendWord.data((W-1), (8*offset)) = currWord.data((W-1)-(8*offset), 0);

				sendWord.keep(offset-1, 0) = prevWord.keep((W/8-1), (W/8)-offset);
				sendWord.keep((W/8-1), offset) = currWord.keep((W/8-1)-offset, 0);

				sendWord.last = (currWord.keep((W/8-1), (W/8)-offset) == 0);

			}
			output.write(sendWord);

			prevWord = currWord;
			ls_firstWord = false;
			if (currWord.last)
			{
				ls_firstWord = true;
				ls_writeRemainder = !sendWord.last;
			}
		} //else offset
	}

}

// The 2nd template parameter is a hack to use this function multiple times
template <typename T, int W, int whatever>
void ip_handler_rshiftWordByOctet(	uint16_t offset,
						hls::stream<T>& input,
						hls::stream<T>& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1 //TODO this has a bug, the bug might come from how it is used

	enum fsmStateType {PKG, REMAINDER};
	static fsmStateType fsmState = PKG;
	static bool rs_firstWord = (offset != 0);
	static T prevWord;

	T currWord;
	T sendWord;

	sendWord.last = 0;
	switch (fsmState)
	{
	case PKG:
		if (!input.empty())
		{
			input.read(currWord);

			if (!rs_firstWord)
			{
				if (offset == 0)
				{
					sendWord = currWord;
				}
				else
				{
					sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
					sendWord.data((W-1), W-(8*offset)) = currWord.data((8*offset)-1, 0);

					sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
					sendWord.keep((W/8-1), (W/8)-offset) = currWord.keep(offset-1, 0);

					sendWord.last = (currWord.keep((W/8-1), offset) == 0);
					//sendWord.dest = currWord.dest;
					assignDest(sendWord, currWord);
				}//else offset
				output.write(sendWord);
			}

			prevWord = currWord;
			rs_firstWord = false;
			if (currWord.last)
			{
				rs_firstWord = (offset != 0);
				//rs_writeRemainder = (sendWord.last == 0);
				if (!sendWord.last)
				{
					fsmState = REMAINDER;
				}
			}
			//}//else offset
		}
		break;
	case REMAINDER:
		sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
		sendWord.data((W-1), W-(8*offset)) = 0;
		sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
		sendWord.keep((W/8-1), (W/8)-offset) = 0;
		sendWord.last = 1;
		//sendWord.dest = prevWord.dest;
		assignDest(sendWord, currWord);

		output.write(sendWord);
		fsmState = PKG;
		break;
	}
}

// The 2nd template parameter is a hack to use this function multiple times
template <typename T, int W, int whatever>
void udp_rshiftWordByOctet(	uint16_t offset,
						hls::stream<T>& input,
						hls::stream<T>& output)
{
#pragma HLS inline off
#pragma HLS pipeline II=1 //TODO this has a bug, the bug might come from how it is used

	enum fsmStateType {PKG, REMAINDER};
	static fsmStateType fsmState = PKG;
	static bool rs_firstWord = (offset != 0);
	static T prevWord;

	T currWord;
	T sendWord;

	sendWord.last = 0;
	switch (fsmState)
	{
	case PKG:
		if (!input.empty())
		{
			input.read(currWord);

			if (!rs_firstWord)
			{
				if (offset == 0)
				{
					sendWord = currWord;
				}
				else
				{
					sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
					sendWord.data((W-1), W-(8*offset)) = currWord.data((8*offset)-1, 0);

					sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
					sendWord.keep((W/8-1), (W/8)-offset) = currWord.keep(offset-1, 0);

					sendWord.last = (currWord.keep((W/8-1), offset) == 0);
					//sendWord.dest = currWord.dest;
					assignDest(sendWord, currWord);
				}//else offset
				output.write(sendWord);
			}

			prevWord = currWord;
			rs_firstWord = false;
			if (currWord.last)
			{
				rs_firstWord = (offset != 0);
				//rs_writeRemainder = (sendWord.last == 0);
				if (!sendWord.last)
				{
					fsmState = REMAINDER;
				}
			}
			//}//else offset
		}
		break;
	case REMAINDER:
		sendWord.data((W-1)-(8*offset), 0) = prevWord.data((W-1), 8*offset);
		sendWord.data((W-1), W-(8*offset)) = 0;
		sendWord.keep((W/8-1)-offset, 0) = prevWord.keep((W/8-1), offset);
		sendWord.keep((W/8-1), (W/8)-offset) = 0;
		sendWord.last = 1;
		//sendWord.dest = prevWord.dest;
		assignDest(sendWord, currWord);

		output.write(sendWord);
		fsmState = PKG;
		break;
	}
}