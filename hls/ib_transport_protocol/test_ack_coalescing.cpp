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

// Unit test for acknowledgements in the requester. An ACK acknowledges every request up to its PSN
// (a responder may coalesce ACKs: a ConnectX acknowledges several requests with one). Each
// acknowledged WRITE must complete, with its own routing bits, and must leave the QP's
// retransmission list, so that a later READ response finds its READ there and its data goes to
// the READ's buffer. Drives the whole ib_transport_protocol; the
// test plays the remote node, reading the requests the stack sends and answering them with
// packets of its choosing, and the memory.
//
// Build and run from the repository root, with the Vitis HLS headers:
//   mkdir -p build && sed -e 's/${DATA_WIDTH}/64/' -e 's/${ROCE_STACK_MAX_QPS}/256/' \
//       -e 's/${PMTU_BYTES}/4096/' hls/rocev2/rocev2_config.hpp.in > build/rocev2_config.hpp
//   g++ -std=c++14 -I$XILINX_HLS/include -Ihls -Ihls/ib_transport_protocol -Ibuild \
//       hls/ib_transport_protocol/test_ack_coalescing.cpp -o build/test_ack_coalescing \
//       && build/test_ack_coalescing

// The stack, and the opcode helpers it uses
#include "ib_transport_protocol.cpp"
#include "ib_utils.cpp"

#include <cstdio>
#include <vector>

namespace
{

const int W = 512;

stream<ipUdpMeta> rx_meta("rx_meta");
stream<net_axis<W> > rx_data("rx_data");
stream<ipUdpMeta> tx_meta("tx_meta");
stream<net_axis<W> > tx_data("tx_data");
stream<txMeta> sq("sq");
stream<ackMeta> completions("completions");
stream<memCmd> mem_wr_cmd("mem_wr_cmd");
stream<memCmd> mem_rd_cmd("mem_rd_cmd");
stream<net_axis<W> > mem_wr_data("mem_wr_data");
stream<net_axis<W> > mem_rd_data("mem_rd_data");
stream<qpContext> qp_ctx("qp_ctx");
stream<ifConnReq> qp_conn("qp_conn");
#ifdef DBG_IBV
// Never read: psnPkg has no default constructor
stream<psnPkg> dbg0("dbg0");
stream<psnPkg> dbg1("dbg1");
stream<psnPkg> dbg2("dbg2");
#endif
ap_uint<32> reg_drops, reg_retrans, reg_rx, reg_tx;

const ap_uint<128> PEER_IP("0xfe8000000000000092e2baff0b01d4d3");

// What the stack sent, what it completed, and where it wrote READ data
struct Sent
{
    ibOpCode op;
    uint32_t qpn;
    uint32_t psn;
};
std::vector<Sent> sent;
std::vector<ackMeta> done;
std::vector<memCmd> written;

bool tx_in_packet = false;
int failures = 0;

void check(bool ok, const char* what)
{
    std::printf("%s %s\n", ok ? "ok:  " : "FAIL:", what);
    failures += ok ? 0 : 1;
}

// tkeep of a word holding `bytes` bytes
ap_uint<W / 8> keep_of(uint32_t bytes)
{
    ap_uint<W / 8> keep = 0;
    for (uint32_t i = 0; i < bytes; ++i)
        keep[i] = 1;
    return keep;
}

void step(int cycles)
{
    for (int i = 0; i < cycles; ++i)
    {
        ib_transport_protocol<W, 0>(rx_meta, rx_data, tx_meta, tx_data, sq, completions,
                                    mem_wr_cmd, mem_rd_cmd, mem_wr_data, mem_rd_data, qp_ctx, qp_conn,
#ifdef DBG_IBV
                                    dbg0, dbg1, dbg2,
#endif
                                    reg_drops, reg_retrans, reg_rx, reg_tx);

        // The remote node: note each packet's opcode and PSN from its BTH
        while (!tx_meta.empty())
            tx_meta.read();
        while (!tx_data.empty())
        {
            net_axis<W> word = tx_data.read();
            if (!tx_in_packet)
            {
                BaseTransportHeader<W> bth;
                bth.parseWord(word.data);
                sent.push_back(Sent{bth.getOpCode(), bth.getDstQP().to_uint(), bth.getPsn().to_uint()});
            }
            tx_in_packet = !word.last;
        }

        // The memory: payload for every read, and note where READ data is written
        while (!mem_rd_cmd.empty())
        {
            memCmd cmd = mem_rd_cmd.read();
            uint32_t len = cmd.len.to_uint();
            for (uint32_t offs = 0; offs < len; offs += W / 8)
            {
                net_axis<W> word;
                word.data = 0;
                const uint32_t bytes = len - offs < W / 8 ? len - offs : W / 8;
                word.keep = keep_of(bytes);
                word.last = offs + W / 8 >= len;
                mem_rd_data.write(word);
            }
        }
        while (!mem_wr_cmd.empty())
            written.push_back(mem_wr_cmd.read());
        while (!mem_wr_data.empty())
            mem_wr_data.read();
        while (!completions.empty())
            done.push_back(completions.read());
    }
}

// A packet from the remote node to local QP `qpn`: BTH, an AETH (syndrome ACK) for acknowledgements and
// READ responses, `payload` bytes, and the ICRC's 4 bytes
void receive(ibOpCode op, uint32_t qpn, uint32_t psn, uint32_t payload)
{
    BaseTransportHeader<W> bth;
    bth.setOpCode(op);
    bth.setPartitionKey(0xffff);
    bth.setDstQP(qpn);
    bth.setPsn(psn);
    AckExHeader<W> aeth;
    aeth.setSyndrome(0);
    aeth.setMsn(0);

    ap_uint<W> header = 0;
    bth.consumeWord(header);
    ap_uint<W> ack = 0;
    aeth.consumeWord(ack);
    header(BTH_SIZE + AETH_SIZE - 1, BTH_SIZE) = ack(AETH_SIZE - 1, 0);

    const uint32_t len = (BTH_SIZE + AETH_SIZE) / 8 + payload + 4;
    rx_meta.write(ipUdpMeta(PEER_IP, RDMA_DEFAULT_PORT, RDMA_DEFAULT_PORT, 8 + len));
    for (uint32_t offs = 0; offs < len; offs += W / 8)
    {
        net_axis<W> word;
        word.data = offs == 0 ? header : ap_uint<W>(0);
        const uint32_t bytes = len - offs < W / 8 ? len - offs : W / 8;
        word.keep = keep_of(bytes);
        word.last = offs + W / 8 >= len;
        rx_data.write(word);
    }
    step(600);
}

// Local address of a request whose completion is to go to stream `dest` of the vFPGA
ap_uint<64> laddr(uint64_t base, uint32_t dest)
{
    return (ap_uint<64>(dest) << 48) | base;
}

std::vector<ackMeta> completions_of(uint32_t qpn)
{
    std::vector<ackMeta> out;
    for (const ackMeta& c : done)
        if (c.qpn == qpn)
            out.push_back(c);
    return out;
}

// PSNs of the packets a local QP sent (their BTH carries the remote node's QP number)
std::vector<uint32_t> psns_sent(uint32_t qpn, ibOpCode op)
{
    std::vector<uint32_t> out;
    for (const Sent& s : sent)
        if (s.qpn == 0x10 + qpn && s.op == op)
            out.push_back(s.psn);
    return out;
}

} // namespace

int main()
{
    // Two QPs to the remote node, 1 and 2 locally
    for (uint32_t qpn = 1; qpn <= 2; ++qpn)
    {
        qp_ctx.write(qpContext(READY_SEND, qpn, 0x100 * qpn, 0x1000 * qpn, 0, 0));
        qp_conn.write(ifConnReq(qpn, 0x10 + qpn, PEER_IP, RDMA_DEFAULT_PORT));
    }
    step(100);

    // Three WRITEs on QP 1, each to be completed on its own stream (dest 1, 2, 3); the remote node
    // acknowledges all three with one ACK, for the last PSN
    for (uint32_t i = 0; i < 3; ++i)
        sq.write(txMeta(RC_RDMA_WRITE_ONLY, 1, 1, 1, 0, laddr(0x10000 + i * 0x1000, i + 1), 0x80000, 64, 0));
    step(600);
    {
        const std::vector<uint32_t> psns = psns_sent(1, RC_RDMA_WRITE_ONLY);
        check(psns.size() == 3, "QP 1 sends three WRITEs");
        if (psns.size() == 3)
            receive(RC_ACK, 1, psns[2], 0);
        const std::vector<ackMeta> c = completions_of(1);
        std::printf("       %zu completion(s) on QP 1, dest:", c.size());
        for (const ackMeta& m : c)
            std::printf(" %u", m.dst.to_uint());
        std::printf("\n");
        check(c.size() == 3, "one ACK for the third WRITE completes all three");
        check(c.size() == 3 && c[0].dst == 1 && c[1].dst == 2 && c[2].dst == 3,
              "each completion goes to its own WRITE's stream");
    }

    // Two WRITEs on QP 2, acknowledged by one ACK for the second, then a READ: its response must
    // find the READ at the head of the QP's retransmission list, not a WRITE left there
    sq.write(txMeta(RC_RDMA_WRITE_ONLY, 2, 1, 1, 0, laddr(0x40000, 1), 0x90000, 64, 0));
    sq.write(txMeta(RC_RDMA_WRITE_ONLY, 2, 1, 1, 0, laddr(0x41000, 1), 0x91000, 64, 0));
    step(600);
    {
        const std::vector<uint32_t> writes = psns_sent(2, RC_RDMA_WRITE_ONLY);
        check(writes.size() == 2, "QP 2 sends two WRITEs");
        if (writes.size() == 2)
            receive(RC_ACK, 2, writes[1], 0);
    }
    sq.write(txMeta(RC_RDMA_READ_REQUEST, 2, 1, 1, 0, laddr(0x50000, 2), 0xA0000, 64, 0));
    step(600);
    {
        const std::vector<uint32_t> reads = psns_sent(2, RC_RDMA_READ_REQUEST);
        check(reads.size() == 1, "QP 2 then sends a READ");
        if (reads.size() == 1)
            receive(RC_RDMA_READ_RESP_ONLY, 2, reads[0], 64);
        std::printf("       READ data written to:");
        for (const memCmd& w : written)
            std::printf(" 0x%llx", (unsigned long long) w.addr(31, 0).to_uint64());
        std::printf("\n");
        check(written.size() == 1 && written[0].addr(31, 0) == 0x50000,
              "the READ response's data goes to the READ's buffer");
        const std::vector<ackMeta> c = completions_of(2);
        std::printf("       %zu completion(s) on QP 2\n", c.size());
        check(c.size() == 3, "two WRITE completions and the READ's");
    }

    if (failures)
        std::printf("%d FAILURE(S)\n", failures);
    else
        std::printf("ALL PASSED\n");
    return failures ? 1 : 0;
}
