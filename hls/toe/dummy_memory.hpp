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
#ifndef MEM_H_
#define MEM_H_

#include "toe.hpp"
#include <map>

template <int WIDTH>
class dummyMemory {
public:
	void setReadCmd(mmCmd cmd);
	void setWriteCmd(mmCmd cmd);
	void readWord(net_axis<WIDTH>& word);
	void writeWord(net_axis<WIDTH>& word);
private:
	std::map<ap_uint<16>, ap_uint<8>*>::iterator createBuffer(ap_uint<16> id);
	//void shuffleWord(ap_uint<64>& );
	//bool* getBitMask(ap_uint<4> keep);
	ap_uint<16> readAddr; //<8>
	ap_uint<16> readId;
	int readLen;
	ap_uint<16> writeAddr; //<8>
	ap_uint<16> writeId;
	ap_uint<16> writeLen;
	std::map<ap_uint<16>, ap_uint<8>*> storage;
	std::map<ap_uint<16>, ap_uint<8>*>::iterator readStorageIt;
	std::map<ap_uint<16>, ap_uint<8>*>::iterator writeStorageIt;
};

template <int WIDTH>
void dummyMemory<WIDTH>::setReadCmd(mmCmd cmd)
{
//	readAddr = cmd.saddr(7, 0);
	readAddr = cmd.saddr(15, 0);
	readId = cmd.saddr(31, 16);
	uint16_t tempLen = (uint16_t) cmd.bbt(15, 0);
	readLen = (int) tempLen;
	//std::cout << readLen << std::endl;
}

template <int WIDTH>
void dummyMemory<WIDTH>::setWriteCmd(mmCmd cmd)
{
//	writeAddr = cmd.saddr(7, 0);
	writeAddr = cmd.saddr(15, 0);
	writeId = cmd.saddr(31, 16);
	uint16_t tempLen = (uint16_t) cmd.bbt(15, 0);
	writeLen = (int) tempLen;
	//std::cout << "WRITE command: " << std::hex << cmd.saddr(15, 0) << " " << std::dec << cmd.bbt << std::endl;
}

template <int WIDTH>
void dummyMemory<WIDTH>::readWord(net_axis<WIDTH>& word)
{
	readStorageIt = storage.find(readId);
	if (readStorageIt == storage.end())
	{
		readStorageIt = createBuffer(readId);
		// check it?
	}
	int i = 0;
	word.keep = 0;
	while (readLen > 0 && i < (WIDTH/8))
	{
		word.data((i*8)+7, i*8) = (readStorageIt->second)[readAddr];
		word.keep = (word.keep << 1);
		word.keep++;
		readLen--;
		readAddr++;
		i++;
	}
	if (readLen == 0)
	{
		word.last = 1;
	}
	else
	{
		word.last = 0;
	}
}

template <int WIDTH>
void dummyMemory<WIDTH>::writeWord(net_axis<WIDTH>& word)
{
	writeStorageIt = storage.find(writeId);
	if (writeStorageIt == storage.end())
	{
		writeStorageIt = createBuffer(writeId);
		// check it?
	}
	//shuffleWord(word.data);
	for (int i = 0; i < (WIDTH/8); i++)
	{
		if (word.keep[i])
		{
			(writeStorageIt->second)[writeAddr] = word.data((i*8)+7, i*8);
			writeAddr++;
			writeLen--;
		}
		else
		{
			break;
		}
	}
	if (word.last)
	{
		assert(writeLen == 0);
	}
}

template <int WIDTH>
std::map<ap_uint<16>, ap_uint<8>*>::iterator dummyMemory<WIDTH>::createBuffer(ap_uint<16> id)
{
	ap_uint<8>* array = new ap_uint<8>[65536]; // [255] default
	std::pair<std::map<ap_uint<16>, ap_uint<8>*>::iterator, bool> ret;
	ret = storage.insert(std::make_pair(id, array));
	if (ret.second)
	{
		return ret.first;
	}
	return storage.end();
}

#endif
