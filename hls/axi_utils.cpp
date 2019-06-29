#include "axi_utils.hpp"

/*bool checkIfResponse(ibOpCode code)
{
	return (code == RC_RDMA_READ_RESP_FIRST || code == RC_RDMA_READ_RESP_MIDDLE ||
			code == RC_RDMA_READ_RESP_LAST || code == RC_RDMA_READ_RESP_ONLY ||
			code == RC_ACK);
}

bool checkIfWriteOrPartReq(ibOpCode code)
{
	return (code == RC_RDMA_WRITE_FIRST || code == RC_RDMA_WRITE_MIDDLE ||
			code == RC_RDMA_WRITE_LAST || code == RC_RDMA_WRITE_ONLY ||
			code == RC_RDMA_PART_FIRST || code == RC_RDMA_PART_MIDDLE ||
			code == RC_RDMA_PART_LAST || code == RC_RDMA_PART_ONLY);
}

bool checkIfAethHeader(ibOpCode code)
{
	return (code == RC_RDMA_READ_RESP_ONLY || code == RC_RDMA_READ_RESP_FIRST ||
			code == RC_RDMA_READ_RESP_LAST || code == RC_ACK);
}

bool checkIfRethHeader(ibOpCode code)
{
	return (code == RC_RDMA_WRITE_ONLY || code == RC_RDMA_WRITE_FIRST ||
			code == RC_RDMA_PART_ONLY || code == RC_RDMA_PART_FIRST ||
			code == RC_RDMA_READ_REQUEST || code == RC_RDMA_READ_CONSISTENT_REQUEST);
}*/

/*template <>
void assignDest<routedAxiWord>(routedAxiWord& d, routedAxiWord& s)
{
	d.dest = s.dest;
}*/

ap_uint<64> lenToKeep(ap_uint<32> length)
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

