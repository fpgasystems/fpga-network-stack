/*
 * Copyright (c) 2022, Systems Group, ETH Zurich
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
#include "rocev2.hpp"
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h> /* Added for the nonblocking socket */
#include <cstdint>

#include "../axi_utils.hpp" //TODO why is this needed here
#include "../ib_transport_protocol/ib_transport_protocol.hpp"
#include "rocev2_config.hpp"

using namespace hls;
#include "newFakeDram.hpp"
#include "simSwitch.hpp"

#define IBTPORT(ninst)                                                   \
    static stream<txMeta> s_axis_sq_meta_n##ninst;                       \
    static stream<ackMeta> m_axis_rx_ack_meta_n##ninst;                  \
    static stream<qpContext> s_axis_qp_interface_n##ninst;               \
    static stream<ifConnReq> s_axis_qp_conn_interface_n##ninst;          \
    static stream<memCmd> m_axis_mem_write_cmd_n##ninst;                 \
    static stream<memCmd> m_axis_mem_read_cmd_n##ninst;                  \
    static stream<net_axis<DATA_WIDTH> > m_axis_mem_write_data_n##ninst; \
    static stream<net_axis<DATA_WIDTH> > s_axis_mem_read_data_n##ninst;  \
    ap_uint<32> regInvalidPsnDropCount_n##ninst;                         \
    ap_uint<32> regRetransCount_n##ninst;                                \
    ap_uint<32> regValidIbvCountRx_n##ninst;                             \
    ap_uint<32> regValidIbvCountTx_n##ninst;

#define IBTRUN(ninst)                               \
    ib_transport_protocol<DATA_WIDTH, ninst>(       \
        s_axis_rx_meta_n##ninst,                    \
        s_axis_rx_data_n##ninst,                    \
        m_axis_tx_meta_n##ninst,                    \
        m_axis_tx_data_n##ninst,                    \
        s_axis_sq_meta_n##ninst,                    \
        m_axis_rx_ack_meta_n##ninst,                \
        m_axis_mem_write_cmd_n##ninst,              \
        m_axis_mem_read_cmd_n##ninst,               \
        m_axis_mem_write_data_n##ninst,             \
        s_axis_mem_read_data_n##ninst,              \
        s_axis_qp_interface_n##ninst,               \
        s_axis_qp_conn_interface_n##ninst,          \
        regInvalidPsnDropCount_n##ninst,            \
        regRetransCount_n##ninst,                   \
        regValidIbvCountRx_n##ninst,                \
        regValidIbvCountTx_n##ninst                 \
    );

#define SWITCHPORT(port)                                    \
    stream<ipUdpMeta> s_axis_rx_meta_n##port;               \
    stream<net_axis<DATA_WIDTH> > s_axis_rx_data_n##port;   \
    stream<ipUdpMeta> m_axis_tx_meta_n##port;               \
    stream<net_axis<DATA_WIDTH> > m_axis_tx_data_n##port;

#define SWITCHRUN(dropEveryNPacket)                 \
    simSwitch<DATA_WIDTH>(                          \
        s_axis_rx_meta_n0,                          \
        s_axis_rx_data_n0,                          \
        m_axis_tx_meta_n0,                          \
        m_axis_tx_data_n0,                          \
        s_axis_rx_meta_n1,                          \
        s_axis_rx_data_n1,                          \
        m_axis_tx_meta_n1,                          \
        m_axis_tx_data_n1,                          \
        ipAddrN0,                                   \
        ipAddrN1,                                   \
        dropEveryNPacket                            \
    );

#define DRAMRUN(ninst)                                                                    \
if (!m_axis_mem_write_cmd_n##ninst.empty() && !writeCmdReady[ninst]){                     \
    m_axis_mem_write_cmd_n##ninst.read(writeCmd[ninst]);                                  \
    writeCmdReady[ninst] = true;                                                          \
    writeRemainLen[ninst] = writeCmd[ninst].len;                                          \
    std::cout << "[Memory]: Write command, address: " << writeCmd[ninst].addr              \
        << ", length: " << std::dec <<writeCmd[ninst].len << std::endl;                   \
}                                                                                         \
if (writeCmdReady[ninst] && !m_axis_mem_write_data_n##ninst.empty()){                     \
    net_axis<DATA_WIDTH> currWord;                                                        \
    m_axis_mem_write_data_n##ninst.read(currWord);                                        \
    writeRemainLen[ninst] -= (DATA_WIDTH/8);                                              \
    writeCmdReady[ninst] = (writeRemainLen[ninst] <= 0) ? false : true;                   \
}                                                                                         \
if (!m_axis_mem_read_cmd_n##ninst.empty()){                                               \
    m_axis_mem_read_cmd_n##ninst.read(readCmd[ninst]);                                    \
    memoryN##ninst.processRead(readCmd[ninst], s_axis_mem_read_data_n##ninst);            \
}                                                                                         \
if (!m_axis_rx_ack_meta_n##ninst.empty()){                                                \
    m_axis_rx_ack_meta_n##ninst.read(ackMeta[ninst]);                                     \
    std::cout << "[Ack " << ninst << "]: qpn: " << std::hex <<                            \
        ackMeta[ninst].qpn << std::dec <<                                                 \
        "\tisNak:" << 0                                                                   \
        << std::endl;                                                                     \
}

 //std::cout << "[Memory]: Write data: " << std::hex                                      \
 //       << currWord.data << std::dec << std::endl;                                        \


int main(int argc, char* argv[]){
    // testSimSwitch(8); // drop one packet for every 8; 0 means no drop

    // switch ports
    SWITCHPORT(0);
    SWITCHPORT(1);

    // interfaces
    IBTPORT(0);
    IBTPORT(1);

    // newFakeDRAM
    newFakeDRAM<DATA_WIDTH> memoryN0;
    newFakeDRAM<DATA_WIDTH> memoryN1;
    std::vector<bool> writeCmdReady {false, false};
    std::vector<memCmd> writeCmd(2);
    std::vector<memCmd> readCmd(2);
    std::vector<int> writeRemainLen(2);
    std::vector<ackMeta> ackMeta(2);

    // ipAddr
    ap_uint<128> ipAddrN0, ipAddrN1;
    ipAddrN0(127, 64) = 0xfe80000000000000;
    ipAddrN0(63, 0)   = 0x92e2baff0b01d4d2;
    ipAddrN1(127, 64) = 0xfe80000000000000;
    ipAddrN1(63, 0)   = 0x92e2baff0b01d4d3;

    // Create qp ctx
    qpContext ctxN00 = qpContext(READY_RECV, 0x00, 0xac701e, 0x2a19d6, 0, 0x00);
    qpContext ctxN01 = qpContext(READY_RECV, 0x01, 0xbc701e, 0x3a19d6, 0, 0x00);

    qpContext ctxN10 = qpContext(READY_RECV, 0x00, 0x3a19d6, 0xbc701e, 0, 0x00);
    qpContext ctxN11 = qpContext(READY_RECV, 0x01, 0x2a19d6, 0xac701e, 0, 0x00);
    
    ifConnReq connInfoN0 = ifConnReq(1, 0, ipAddrN1, 5000);
    ifConnReq connInfoN1 = ifConnReq(0, 1, ipAddrN0, 5000);

    s_axis_qp_interface_n0.write(ctxN00);
    s_axis_qp_interface_n0.write(ctxN01);
    s_axis_qp_interface_n1.write(ctxN10);
    s_axis_qp_interface_n1.write(ctxN11);

    s_axis_qp_conn_interface_n0.write(connInfoN0);
    s_axis_qp_conn_interface_n1.write(connInfoN1);

    int count = 0;
    //Make sure it is initialized
    while (count < 10)
    {
        IBTRUN(0);
        IBTRUN(1);
        count++;
    }

    for (int i=0; i<1; i++) {
        // s_axis_sq_meta_n0.write(txMeta(RC_RDMA_WRITE_FIRST, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_RDMA_WRITE_MIDDLE, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_RDMA_WRITE_MIDDLE, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_RDMA_WRITE_LAST, 0x01, 0, 1, 0, params));

        //s_axis_sq_meta_n0.write(txMeta(RC_SEND_ONLY, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_SEND_FIRST, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_SEND_MIDDLE, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_SEND_MIDDLE, 0x01, 0, 0, 0, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_SEND_LAST, 0x01, 0, 1, 0, params));

        s_axis_sq_meta_n0.write(txMeta(RC_RDMA_READ_REQUEST, 0x01, 0, 0, 0, 0x400, 0x500, 1 * 1024, 0));
        // s_axis_sq_meta_n0.write(txMeta(RC_RDMA_READ_REQUEST, 0x01, 0, 0, 1, params));
        // s_axis_sq_meta_n0.write(txMeta(RC_RDMA_READ_REQUEST, 0x01, 0, 1, 2, params));
    }

    while (count < 200000)
    {
        IBTRUN(0);
        IBTRUN(1);
        SWITCHRUN(0);
        DRAMRUN(0);
        DRAMRUN(1);
        count++;
    }

    return 0;
}
