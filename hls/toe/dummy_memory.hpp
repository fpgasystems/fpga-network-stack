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
#ifndef MEM_H_
#define MEM_H_

#include "toe.hpp"
#include <map>

class dummyMemory {
public:
	void setReadCmd(mmCmd cmd);
	void setWriteCmd(mmCmd cmd);
	void readWord(axiWord& word);
	void writeWord(axiWord& word);
private:
	std::map<ap_uint<16>, ap_uint<8>*>::iterator createBuffer(ap_uint<16> id);
	void shuffleWord(ap_uint<64>& );
	bool* getBitMask(ap_uint<4> keep);
	ap_uint<16> readAddr; //<8>
	ap_uint<16> readId;
	int readLen;
	ap_uint<16> writeAddr; //<8>
	ap_uint<16> writeId;
	//ap_uint<16> writeLen;
	std::map<ap_uint<16>, ap_uint<8>*> storage;
	std::map<ap_uint<16>, ap_uint<8>*>::iterator readStorageIt;
	std::map<ap_uint<16>, ap_uint<8>*>::iterator writeStorageIt;
};
#endif
