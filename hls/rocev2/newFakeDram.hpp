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
#include "../axi_utils.hpp"
#include "../mem_utils.hpp"

template <int WIDTH>
class newFakeDRAM {
	struct MemChunk {
		uint64_t size;
		unsigned char* data;
	};

public:
	newFakeDRAM() {}//:numChunks(0) {}
	~newFakeDRAM()
	{
		/*for (int i = 0; i < numChunks; ++i)
		{
			delete chunks[i]->data;
			delete chunks[i];
		}*/
	}

	unsigned char* createChunk(uint64_t size)
	{
		/*MemChunk* chunk = new MemChunk();
		chunk->size = size;
		chunk->data = (unsigned char*) malloc(size);
		chunks.push_back(chunk);
		numChunks++;
		return chunk->data;*/
	}

	void processWrite(memCmd cmd, hls::stream<routed_net_axis<WIDTH> >& dataIn)
	{
		std::cout << "Write command, address: " << cmd.addr << ", length: " << cmd.len << std::endl;

		routed_net_axis<WIDTH> currWord;
		do
		{
			dataIn.read(currWord);
		} while(!currWord.last);
	}
	void processWrite(routedMemCmd cmd, hls::stream<routed_net_axis<WIDTH> >& dataIn)
	{
		std::cout << "Write command, address: " << cmd.data.addr << ", length: " << cmd.data.len << std::endl;

		routed_net_axis<WIDTH> currWord;
		do
		{
			dataIn.read(currWord);
		} while(!currWord.last);
	}

	void processRead(memCmd cmd, hls::stream<net_axis<WIDTH> >& dataOut)
	{
		
		//uint64_t offset = cmd.addr - chunk->baseAddr;
		uint64_t* memPtr = (uint64_t*)(uint64_t) cmd.addr; //(uint64_t*) (chunk->data + offset);
		uint32_t lengthCount = 0;
		net_axis<WIDTH> currWord;
		currWord.keep = 0;
		currWord.last = 0;
		uint32_t idx = 0;
		do
		{
			lengthCount += 8;
			currWord.data(idx*64+63, idx*64) = *memPtr;
			currWord.keep(idx*8+7, idx*8) = 0xFF;
			idx++;
			if (lengthCount == cmd.len || idx == (WIDTH/64))
			{
				currWord.last = (lengthCount == cmd.len);
				dataOut.write(currWord);
				currWord.keep = 0x0;
				idx = 0;
			}
			memPtr++;

		} while(lengthCount != cmd.len);
		std::cout << "total data read: " << std::dec << lengthCount << std::endl;
	}
   void processRead(routedMemCmd cmd, hls::stream<net_axis<WIDTH> >& dataOut)
	{
		
		//uint64_t offset = cmd.addr - chunk->baseAddr;
		uint64_t* memPtr = (uint64_t*)(uint64_t) cmd.data.addr; //(uint64_t*) (chunk->data + offset);
		uint32_t lengthCount = 0;
		net_axis<WIDTH> currWord;
		currWord.keep = 0;
		currWord.last = 0;
		uint32_t idx = 0;
		do
		{
			lengthCount += 8;
			currWord.data(idx*64+63, idx*64) = *memPtr;
			currWord.keep(idx*8+7, idx*8) = 0xFF;
			idx++;
			if (lengthCount == cmd.data.len || idx == (WIDTH/64))
			{
				currWord.last = (lengthCount == cmd.data.len);
				dataOut.write(currWord);
				currWord.keep = 0x0;
				idx = 0;
			}
			memPtr++;

		} while(lengthCount != cmd.data.len);
		std::cout << "total data read: " << std::dec << lengthCount << std::endl;
	}

private:
	//int numChunks;
	//std::vector<MemChunk*>	chunks;
};
