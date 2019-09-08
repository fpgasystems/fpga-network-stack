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
#include "axi_utils.hpp"

template <>
void assignDest<routed_net_axis<64> >(routed_net_axis<64>& d, routed_net_axis<64>& s)
{
	d.dest = s.dest;
}
template <>
void assignDest<routed_net_axis<128> >(routed_net_axis<128>& d, routed_net_axis<128>& s)
{
	d.dest = s.dest;
}
template <>
void assignDest<routed_net_axis<256> >(routed_net_axis<256>& d, routed_net_axis<256>& s)
{
	d.dest = s.dest;
}
template <>
void assignDest<routed_net_axis<512> >(routed_net_axis<512>& d, routed_net_axis<512>& s)
{
	d.dest = s.dest;
}

ap_uint<64> lenToKeep(ap_uint<6> length)
{
	switch (length)
	{
	case 1:
	    return 0x01;
	  case 2:
	    return 0x03;
	  case 3:
	    return 0x07;
	  case 4:
	    return 0x0F;
	  case 5:
	    return 0x1F;
	  case 6:
	    return 0x3F;
	  case 7:
	    return 0x7F;
	  case 8:
	    return 0xFF;
	  case 9:
		return 0x01FF;
	  case 10:
		return 0x03FF;
	  case 11:
		return 0x07FF;
	  case 12:
		return 0x0FFF;
	  case 13:
		return 0x1FFF;
	  case 14:
		return 0x3FFF;
	  case 15:
		return 0x7FFF;
	  case 16:
		return 0xFFFF;
	  case 17:
		return 0x01FFFF;
	  case 18:
		return 0x03FFFF;
	  case 19:
		return 0x07FFFF;
	  case 20:
		return 0x0FFFFF;
	  case 21:
		return 0x1FFFFF;
	  case 22:
		return 0x3FFFFF;
	  case 23:
		return 0x7FFFFF;
	  case 24:
		return 0xFFFFFF;
	  case 25:
		return 0x01FFFFFF;
	  case 26:
		return 0x03FFFFFF;
	  case 27:
		return 0x07FFFFFF;
	  case 28:
		return 0x0FFFFFFF;
	  case 29:
		return 0x1FFFFFFF;
	  case 30:
		return 0x3FFFFFFF;
	  case 31:
		return 0x7FFFFFFF;
	  case 32:
		return 0xFFFFFFFF;
	  case 33:
		return 0x01FFFFFFFF;
	  case 34:
		return 0x03FFFFFFFF;
	  case 35:
		return 0x07FFFFFFFF;
	  case 36:
		return 0x0FFFFFFFFF;
	  case 37:
		return 0x1FFFFFFFFF;
	  case 38:
		return 0x3FFFFFFFFF;
	  case 39:
		return 0x7FFFFFFFFF;
	  case 40:
		return 0xFFFFFFFFFF;
	  case 41:
		return 0x01FFFFFFFFFF;
	  case 42:
		return 0x03FFFFFFFFFF;
	  case 43:
		return 0x07FFFFFFFFFF;
	  case 44:
		return 0x0FFFFFFFFFFF;
	  case 45:
		return 0x1FFFFFFFFFFF;
	  case 46:
		return 0x3FFFFFFFFFFF;
	  case 47:
		return 0x7FFFFFFFFFFF;
	  case 48:
		return 0xFFFFFFFFFFFF;
	  case 49:
		return 0x01FFFFFFFFFFFF;
	  case 50:
		return 0x03FFFFFFFFFFFF;
	  case 51:
		return 0x07FFFFFFFFFFFF;
	  case 52:
		return 0x0FFFFFFFFFFFFF;
	  case 53:
		return 0x1FFFFFFFFFFFFF;
	  case 54:
		return 0x3FFFFFFFFFFFFF;
	  case 55:
		return 0x7FFFFFFFFFFFFF;
	  case 56:
		return 0xFFFFFFFFFFFFFF;
	  case 57:
		return 0x01FFFFFFFFFFFFFF;
	  case 58:
		return 0x03FFFFFFFFFFFFFF;
	  case 59:
		return 0x07FFFFFFFFFFFFFF;
	  case 60:
		return 0x0FFFFFFFFFFFFFFF;
	  case 61:
		return 0x1FFFFFFFFFFFFFFF;
	  case 62:
		return 0x3FFFFFFFFFFFFFFF;
	  case 63:
		return 0x7FFFFFFFFFFFFFFF;
	  default:
		return 0xFFFFFFFFFFFFFFFF;
	}//switch
}

//Input argument cannot be templatized, otherwise the case statement leads to duplicate definitions
ap_uint<8> keepToLen(ap_uint<64> keepValue)
{
#pragma HLS INLINE

	switch (keepValue)
	{
	case 0x01:
		return 0x1;
	case 0x3:
		return 0x2;
	case 0x07:
		return 0x3;
	case 0x0F:
		return 0x4;
	case 0x1F:
		return 0x5;
	case 0x3F:
		return 0x6;
	case 0x7F:
		return 0x7;
	case 0xFF:
		return 0x8;
//#if W > 64
	case 0x01FF:
		return 0x9;
	case 0x3FF:
		return 0xA;
	case 0x07FF:
		return 0xB;
	case 0x0FFF:
		return 0xC;
	case 0x1FFF:
		return 0xD;
	case 0x3FFF:
		return 0xE;
	case 0x7FFF:
		return 0xF;
	case 0xFFFF:
		return 0x10;
//#if W > 128
	case 0x01FFFF:
		return 0x11;
	case 0x3FFFF:
		return 0x12;
	case 0x07FFFF:
		return 0x13;
	case 0x0FFFFF:
		return 0x14;
	case 0x1FFFFF:
		return 0x15;
	case 0x3FFFFF:
		return 0x16;
	case 0x7FFFFF:
		return 0x17;
	case 0xFFFFFF:
		return 0x18;
	case 0x01FFFFFF:
		return 0x19;
	case 0x3FFFFFF:
		return 0x1A;
	case 0x07FFFFFF:
		return 0x1B;
	case 0x0FFFFFFF:
		return 0x1C;
	case 0x1FFFFFFF:
		return 0x1D;
	case 0x3FFFFFFF:
		return 0x1E;
	case 0x7FFFFFFF:
		return 0x1F;
	case 0xFFFFFFFF:
		return 0x20;
//#if W > 256
	case 0x01FFFFFFFF:
		return 0x21;
	case 0x3FFFFFFFF:
		return 0x22;
	case 0x07FFFFFFFF:
		return 0x23;
	case 0x0FFFFFFFFF:
		return 0x24;
	case 0x1FFFFFFFFF:
		return 0x25;
	case 0x3FFFFFFFFF:
		return 0x26;
	case 0x7FFFFFFFFF:
		return 0x27;
	case 0xFFFFFFFFFF:
		return 0x28;
	case 0x01FFFFFFFFFF:
		return 0x29;
	case 0x3FFFFFFFFFF:
		return 0x2A;
	case 0x07FFFFFFFFFF:
		return 0x2B;
	case 0x0FFFFFFFFFFF:
		return 0x2C;
	case 0x1FFFFFFFFFFF:
		return 0x2D;
	case 0x3FFFFFFFFFFF:
		return 0x2E;
	case 0x7FFFFFFFFFFF:
		return 0x2F;
	case 0xFFFFFFFFFFFF:
		return 0x30;

	case 0x01FFFFFFFFFFFF:
		return 0x31;
	case 0x3FFFFFFFFFFFF:
		return 0x32;
	case 0x07FFFFFFFFFFFF:
		return 0x33;
	case 0x0FFFFFFFFFFFFF:
		return 0x34;
	case 0x1FFFFFFFFFFFFF:
		return 0x35;
	case 0x3FFFFFFFFFFFFF:
		return 0x36;
	case 0x7FFFFFFFFFFFFF:
		return 0x37;
	case 0xFFFFFFFFFFFFFF:
		return 0x38;
	case 0x01FFFFFFFFFFFFFF:
		return 0x39;
	case 0x3FFFFFFFFFFFFFF:
		return 0x3A;
	case 0x07FFFFFFFFFFFFFF:
		return 0x3B;
	case 0x0FFFFFFFFFFFFFFF:
		return 0x3C;
	case 0x1FFFFFFFFFFFFFFF:
		return 0x3D;
	case 0x3FFFFFFFFFFFFFFF:
		return 0x3E;
	case 0x7FFFFFFFFFFFFFFF:
		return 0x3F;
	case 0xFFFFFFFFFFFFFFFF:
		return 0x40;
//#endif
//#endif
//#endif
	}
}


