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

// Unit test for the requester's handling of an RNR NAK. rx_process_exh and rx_ibh_fsm decode ACK
// packets, and what they send drives the retransmitter and the transport timer, wired as in the
// stack; the test plays the state table and the sending side. An RNR NAK must leave the request it
// refused outstanding, and the timer must resend that request for as long as the responder refuses
// it.
//
// Build and run from the repository root, with the Vitis HLS headers:
//   mkdir -p build && sed -e 's/${DATA_WIDTH}/64/' -e 's/${ROCE_STACK_MAX_QPS}/256/' \
//       -e 's/${PMTU_BYTES}/4096/' hls/rocev2/rocev2_config.hpp.in > build/rocev2_config.hpp
//   g++ -std=c++14 -I$XILINX_HLS/include -Ihls -Ihls/ib_transport_protocol -Ibuild \
//       hls/ib_transport_protocol/test_rnr_nak.cpp -o build/test_rnr_nak && build/test_rnr_nak

// The functions under test, and the opcode helpers they use
#include "ib_transport_protocol.cpp"
#include "ib_utils.cpp"

#include <cstdio>
#include <map>
#include <vector>

namespace
{

const int W = 512;

// rx_process_exh
stream<net_axis<W> > exh_input("exh_input");
stream<ibOpCode> exh_opcode("exh_opcode");
stream<exhMeta> exh_meta("exh_meta");
stream<ExHeader<W> > exh_header("exh_header");
stream<net_axis<W> > exh_output("exh_output");

// rx_ibh_fsm
#ifdef DBG_IBV
stream<psnPkg> ibh_dbg("ibh_dbg"); // never read: psnPkg has no default constructor
#endif
stream<ibhMeta> ibh_meta("ibh_meta");
stream<rxStateRsp> state_rsp("state_rsp");
stream<rxStateReq> state_req("state_req");
stream<ibhMeta> ibh_out("ibh_out");
stream<ackEvent> ibh_event("ibh_event");
stream<bool> ibh_drop("ibh_drop");
stream<fwdPolicy> ibh_drop_meta("ibh_drop_meta");
ap_uint<32> invalid_psn_drops;

// retransmitter and transport timer
stream<retransUpdate> retrans_upd("retrans_upd");
stream<retransRdInit> retrans_init("retrans_init");
stream<retransmission> retrans_nak("retrans_nak");
stream<retransmission> timer_to_retrans("timer_to_retrans");
stream<retransEntry> retrans_insert("retrans_insert");
stream<retransEvent> retrans_event("retrans_event");
stream<rxTimerUpdate> timer_clear("timer_clear");
stream<ap_uint<24> > timer_set("timer_set");

// The responder side of the state table, per QP: the oldest unacknowledged request and the last
// one sent
struct QpState
{
    uint32_t epsn;
    uint32_t max_forward;
};
std::map<uint32_t, QpState> qps;

int failures = 0;

void check(bool ok, const char* what)
{
    std::printf("%s %s\n", ok ? "ok:  " : "FAIL:", what);
    failures += ok ? 0 : 1;
}

void show(const std::vector<uint32_t>& psns)
{
    std::printf("       resent:");
    if (psns.empty())
        std::printf(" nothing");
    for (uint32_t psn : psns)
        std::printf(" %u", psn);
    std::printf("\n");
}

void state_table()
{
    while (!state_req.empty())
    {
        rxStateReq req = state_req.read();
        QpState& qp = qps[req.qpn.to_uint()];
        if (req.write)
            qp.epsn = req.epsn.to_uint();
        else
            state_rsp.write(rxStateRsp(qp.epsn, qp.epsn, qp.max_forward));
    }
}

void step(int cycles)
{
    for (int i = 0; i < cycles; ++i)
    {
        rx_process_exh<W>(exh_input, exh_opcode, exh_meta, exh_header, exh_output);
        rx_ibh_fsm(
#ifdef DBG_IBV
                   ibh_dbg,
#endif
                   ibh_meta, exh_meta, state_rsp, state_req, ibh_out, ibh_event, ibh_drop,
                   ibh_drop_meta, timer_clear, retrans_upd, invalid_psn_drops);
        state_table();
        retransmitter(retrans_upd, retrans_init, retrans_nak, timer_to_retrans, retrans_insert,
                      retrans_event);
        transport_timer(timer_clear, timer_set, timer_to_retrans);

        // Outputs this test does not look at
        while (!exh_header.empty()) exh_header.read();
        while (!exh_output.empty()) exh_output.read();
        while (!ibh_out.empty()) ibh_out.read();
        while (!ibh_event.empty()) ibh_event.read();
        while (!ibh_drop.empty()) ibh_drop.read();
        while (!ibh_drop_meta.empty()) ibh_drop_meta.read();
        while (!retrans_init.empty()) retrans_init.read();
    }
}

std::vector<uint32_t> retransmitted()
{
    std::vector<uint32_t> psns;
    while (!retrans_event.empty())
        psns.push_back(retrans_event.read().psn.to_uint());
    return psns;
}

// Sends SENDs with PSNs first..last on a QP: each is recorded for retransmission and arms the timer
void send(uint32_t qpn, uint32_t first, uint32_t last)
{
    qps[qpn] = QpState{first, last};
    for (uint32_t psn = first; psn <= last; ++psn)
    {
        retrans_insert.write(retransEntry(qpn, psn, RC_SEND_ONLY, 0x1000, 0x2000, 1024, 1, 0, 1));
        timer_set.write(qpn);
        step(20);
    }
    retransmitted();
}

// An ACK packet (AETH) for `psn` on a QP; syndrome 0x00 is an ACK, 0x2c an RNR NAK (RNR timer 12)
void ack(uint32_t qpn, uint32_t psn, uint8_t syndrome)
{
    AckExHeader<W> aeth;
    aeth.setSyndrome(syndrome);
    aeth.setMsn(0);
    net_axis<W> word;
    word.data = 0;
    aeth.consumeWord(word.data);
    word.keep = 0xf;
    word.last = 1;
    exh_opcode.write(RC_ACK);
    exh_input.write(word);
    ibh_meta.write(ibhMeta(RC_ACK, 0, qpn, psn, true));
    step(50);
}

const uint8_t ACK = 0x00;
const uint8_t RNR_NAK = 0x2c;

// Lets the transport timer run until it expires (it visits every QP once per MAX_QPS cycles) and
// returns what the retransmitter sent again
std::vector<uint32_t> expire()
{
    step((int(TIME_64ms.to_uint()) + 2) * MAX_QPS + 200);
    return retransmitted();
}

} // namespace

int main()
{
    // Requests 100..103 in flight: 100 is acknowledged, then the responder refuses 101
    send(1, 100, 103);
    ack(1, 100, ACK);
    ack(1, 101, RNR_NAK);
    {
        const std::vector<uint32_t> psns = expire();
        check(!psns.empty() && psns.front() == 101,
              "after an RNR NAK on 101, the timer resends from 101");
        show(psns);
    }

    // The responder refuses 200 again and again, far past the transport retry count
    send(2, 200, 203);
    {
        int resent = 0;
        for (int round = 0; round < 3 * int(RETRANS_RETRY_CNT); ++round)
        {
            ack(2, 200, RNR_NAK);
            const std::vector<uint32_t> psns = expire();
            resent += !psns.empty() && psns.front() == 200;
        }
        check(resent == 3 * int(RETRANS_RETRY_CNT), "36 RNR NAKs in a row on 200: 200 is resent after each one");
        std::printf("       200 resent after %d of them\n", resent);
    }
    // ... until it has room: the ACK of 203 acknowledges everything and stops the timer
    ack(2, 203, ACK);
    check(expire().empty(), "the ACK of 203 then stops the timer");

    // An RNR NAK on the last request in flight
    send(3, 300, 301);
    ack(3, 300, ACK);
    ack(3, 301, RNR_NAK);
    {
        const std::vector<uint32_t> psns = expire();
        check(!psns.empty() && psns.front() == 301,
              "after an RNR NAK on the last request, 301, the timer keeps running and resends it");
        show(psns);
    }

    if (failures)
        std::printf("%d FAILURE(S)\n", failures);
    else
        std::printf("ALL PASSED\n");
    return failures ? 1 : 0;
}
