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
#pragma once
#include "../axi_utils.hpp"
using namespace hls;

// #define N_NODE_4
#define PORT_LOC 5000
#define PORT_RMT 5001
#define PKT_LEN  64
#define PKT_LEN2 64
#define CLK_SIM 5000

#ifdef N_NODE_4
#define FWDMETA(srcNode)                                                        \
    if (!s_axis_tx_meta_n##srcNode.empty() && !udpMetaVldN##srcNode){           \
        s_axis_tx_meta_n##srcNode.read(udpMetaN##srcNode);                      \
        udpMetaVldN##srcNode = true;                                            \
        cntPacketN##srcNode++;                                                  \
        if(dropEveryNPacket)                                                    \
            dropPacketN##srcNode = (cntPacketN##srcNode % dropEveryNPacket)==0; \
        if(dropPacketN##srcNode);                                               \
        else if(udpMetaN##srcNode.their_address==ip_address_n0)                 \
            m_axis_rx_meta_n0.write(udpMetaN##srcNode);                         \
        else if(udpMetaN##srcNode.their_address==ip_address_n1)                 \
            m_axis_rx_meta_n1.write(udpMetaN##srcNode);                         \
        else if(udpMetaN##srcNode.their_address==ip_address_n2)                 \
            m_axis_rx_meta_n2.write(udpMetaN##srcNode);                         \
        else                                                                    \
            m_axis_rx_meta_n3.write(udpMetaN##srcNode);                         \
    }
#else
#define FWDMETA(srcNode)                                                        \
    if (!s_axis_tx_meta_n##srcNode.empty() && !udpMetaVldN##srcNode){           \
        s_axis_tx_meta_n##srcNode.read(udpMetaN##srcNode);                      \
        udpMetaVldN##srcNode = true;                                            \
        cntPacketN##srcNode++;                                                  \
        if(dropEveryNPacket)                                                    \
            dropPacketN##srcNode = (cntPacketN##srcNode % dropEveryNPacket)==0; \
        if(dropPacketN##srcNode);                                               \
        else if(udpMetaN##srcNode.their_address==ip_address_n0)                 \
            m_axis_rx_meta_n0.write(udpMetaN##srcNode);                         \
        else if(udpMetaN##srcNode.their_address==ip_address_n1)                 \
            m_axis_rx_meta_n1.write(udpMetaN##srcNode);                         \
        else                                                                    \
            std::cout << "[ERROR] Non-existing IP:" << std::hex                 \
                << udpMetaN##srcNode.their_address << std::endl;                \
    }                                                                   
#endif

#ifdef N_NODE_4
#define FWDDATA(srcNode)                                                \
    if (!s_axis_tx_data_n##srcNode.empty() && udpMetaVldN##srcNode){    \
        s_axis_tx_data_n##srcNode.read(currWordN##srcNode);             \
        if(dropPacketN##srcNode);                                        \
        else if(udpMetaN##srcNode.their_address==ip_address_n0)         \
            m_axis_rx_data_n0.write(currWordN##srcNode);                \
        else if(udpMetaN##srcNode.their_address==ip_address_n1)         \
            m_axis_rx_data_n1.write(currWordN##srcNode);                \
        else if(udpMetaN##srcNode.their_address==ip_address_n2)         \
            m_axis_rx_data_n2.write(currWordN##srcNode);                \
        else                                                            \
            m_axis_rx_data_n3.write(currWordN##srcNode);                \
        if(currWordN##srcNode.last){                                    \
            udpMetaVldN##srcNode = false;                               \
            dropPacketN##srcNode = false;                               \
        }                                                               \
    }
#else
#define FWDDATA(srcNode)                                                \
    if (!s_axis_tx_data_n##srcNode.empty() && udpMetaVldN##srcNode){    \
        s_axis_tx_data_n##srcNode.read(currWordN##srcNode);             \
        if(dropPacketN##srcNode);                                       \
        else if(udpMetaN##srcNode.their_address==ip_address_n0)         \
            m_axis_rx_data_n0.write(currWordN##srcNode);                \
        else                                                            \
            m_axis_rx_data_n1.write(currWordN##srcNode);                \
        if(currWordN##srcNode.last){                                    \
            udpMetaVldN##srcNode = false;                               \
            dropPacketN##srcNode = false;                               \
        }                                                               \
    }
#endif


// NOTE: print function will eat the data on bus
#define PRTRXMETA(node)                                                                         \
    if (!s_axis_rx_meta_n##node.empty()){                                                       \
        ipUdpMeta udpMeta;                                                                      \
        s_axis_rx_meta_n##node.read(udpMeta);                                                   \
        std::cout << "[s_axis_rx_meta_n" << node << "]:\t"                                      \
            << "dstIP:" << std::hex << udpMeta.their_address << std::dec                        \
            << ", dstPort:" << udpMeta.their_port                                               \
            << ", srcPort:" << udpMeta.my_port                                                  \
            << ", Len:" << udpMeta.length << std::endl;                                         \
    }

#define PRTRXDATA(node)                                                                         \
        if (!s_axis_rx_data_n##node.empty()){                                                   \
            net_axis<DATA_WIDTH> curWord;                                                       \
            s_axis_rx_data_n##node.read(curWord);                                               \
            std::cout << "[s_axis_rx_data_n" << node << "]:\t"                                  \
                << "Data:" << std::hex << curWord.data                                          \
                << ", Keep:" << curWord.keep << ", isLast:" << curWord.last << std::endl;       \
        }

#define PRTTXMETA(node)                                                                         \
    if (!m_axis_tx_meta_n##node.empty()){                                                       \
        ipUdpMeta udpMeta;                                                                      \
        m_axis_tx_meta_n##node.read(udpMeta);                                                   \
        std::cout << "[m_axis_tx_meta_n" << node << "]:\t"                                      \
            << "dstIP:" << std::hex << udpMeta.their_address << std::dec                        \
            << ", dstPort:" << udpMeta.their_port                                               \
            << ", srcPort:" << udpMeta.my_port                                                  \
            << ", Len:" << udpMeta.length << std::endl;                                         \
    }

#define PRTTXDATA(node)                                                                         \
        if (!m_axis_tx_data_n##node.empty()){                                                   \
            net_axis<DATA_WIDTH> curWord;                                                       \
            m_axis_tx_data_n##node.read(curWord);                                               \
            std::cout << "[m_axis_tx_data_n" << node << "]:\t"                                  \
                << "Data:" << std::hex << curWord.data                                          \
                << ", Keep:" << curWord.keep << ", isLast:" << curWord.last << std::endl;       \
        }


// ------------------------------------------------------------------------------------------------
// simulate switch behavior with udp packets
// ------------------------------------------------------------------------------------------------
template <int WIDTH>
void simSwitch( 
    // RX - net module
    stream<ipUdpMeta>& m_axis_rx_meta_n0,
    stream<net_axis<WIDTH> >& m_axis_rx_data_n0,
    // TX - net module
    stream<ipUdpMeta>& s_axis_tx_meta_n0,
    stream<net_axis<WIDTH> >& s_axis_tx_data_n0,

    stream<ipUdpMeta>& m_axis_rx_meta_n1,
    stream<net_axis<WIDTH> >& m_axis_rx_data_n1,
    stream<ipUdpMeta>& s_axis_tx_meta_n1,
    stream<net_axis<WIDTH> >& s_axis_tx_data_n1,

#ifdef N_NODE_4
    stream<ipUdpMeta>& m_axis_rx_meta_n2,
    stream<net_axis<WIDTH> >& m_axis_rx_data_n2,
    stream<ipUdpMeta>& s_axis_tx_meta_n2,
    stream<net_axis<WIDTH> >& s_axis_tx_data_n2,

    stream<ipUdpMeta>& m_axis_rx_meta_n3,
    stream<net_axis<WIDTH> >& m_axis_rx_data_n3,
    stream<ipUdpMeta>& s_axis_tx_meta_n3,
    stream<net_axis<WIDTH> >& s_axis_tx_data_n3,
#endif

    ap_uint<128> ip_address_n0,
    ap_uint<128> ip_address_n1,
#ifdef N_NODE_4
    ap_uint<128> ip_address_n2,
    ap_uint<128> ip_address_n3,
#endif

    ap_uint<8> dropEveryNPacket // 0 means no drop
){

#pragma HLS inline off
#pragma HLS pipeline II=1

    static ipUdpMeta udpMetaN0, udpMetaN1;
    static bool udpMetaVldN0, udpMetaVldN1 = false;
    static bool dropPacketN0, dropPacketN1 = false;
    static uint32_t cntPacketN0, cntPacketN1 = 0;    
#ifdef N_NODE_4
    static ipUdpMeta udpMetaN2, udpMetaN3;
    static bool udpMetaVldN2, udpMetaVldN3 = false;
    static bool dropPacketN2, dropPacketN3 = false;
    static uint32_t cntPacketN2, cntPacketN3 = 0;    
#endif

    net_axis<WIDTH> currWordN0, currWordN1;
#ifdef N_NODE_4
    net_axis<WIDTH> currWordN2, currWordN3;
#endif

    FWDMETA(0);
    FWDMETA(1);
#ifdef N_NODE_4
    FWDMETA(2);
    FWDMETA(3);
#endif

    FWDDATA(0);
    FWDDATA(1);
#ifdef N_NODE_4
    FWDDATA(2);
    FWDDATA(3);
#endif

}


int testSimSwitch(int dropEveryNPacket){
#pragma HLS inline region off

    // RX - net module
    stream<ipUdpMeta> s_axis_rx_meta_n0;
    stream<net_axis<DATA_WIDTH> > s_axis_rx_data_n0;
    stream<ipUdpMeta> s_axis_rx_meta_n1;
    stream<net_axis<DATA_WIDTH> > s_axis_rx_data_n1;

    // TX - net module
    stream<ipUdpMeta> m_axis_tx_meta_n0;
    stream<net_axis<DATA_WIDTH> > m_axis_tx_data_n0;
    stream<ipUdpMeta> m_axis_tx_meta_n1;
    stream<net_axis<DATA_WIDTH> > m_axis_tx_data_n1;

    ap_uint<128> ip_address_n0, ip_address_n1;
    ip_address_n0(127, 64) = 0xfe80000000000000;
    ip_address_n0(63, 0)   = 0x92e2baff0b01d4d2;
    ip_address_n1(127, 64) = 0xfe80000000000000;
    ip_address_n1(63, 0)   = 0x92e2baff0b01d4d3;

    ipUdpMeta metaN0 = ipUdpMeta(ip_address_n1, PORT_RMT, PORT_LOC, PKT_LEN);
    ipUdpMeta metaN1 = ipUdpMeta(ip_address_n0, PORT_RMT, PORT_LOC, PKT_LEN2);

    // write test packets to n0 tx
    for (int i=0; i<8; i++){
        m_axis_tx_meta_n0.write(metaN0);
        for (int j=0; j<PKT_LEN; j+=DATA_WIDTH/8){
            bool isLast = ((j+DATA_WIDTH/8)>=PKT_LEN) ? true : false;
            m_axis_tx_data_n0.write(net_axis<DATA_WIDTH>((0x0000<<16)+(i<<8)+j/(DATA_WIDTH/8), lenToKeep(isLast ? PKT_LEN-j : DATA_WIDTH/8), isLast));
        }
    }

    // write test packets to n1 tx
    for (int i=0; i<8; i++){
        m_axis_tx_meta_n1.write(metaN1);
        for (int j=0; j<PKT_LEN2; j+=DATA_WIDTH/8){
            bool isLast = ((j+DATA_WIDTH/8)>=PKT_LEN2) ? true : false;
            m_axis_tx_data_n1.write(net_axis<DATA_WIDTH>((0x0001<<16)+(i<<8)+j/(DATA_WIDTH/8), lenToKeep(isLast ? PKT_LEN2-j : DATA_WIDTH/8), isLast));
        }
    }

    for (int i=0; i<CLK_SIM; i++){
        simSwitch<DATA_WIDTH>(
            s_axis_rx_meta_n0,
            s_axis_rx_data_n0,
            m_axis_tx_meta_n0,
            m_axis_tx_data_n0,
            s_axis_rx_meta_n1,
            s_axis_rx_data_n1,
            m_axis_tx_meta_n1,
            m_axis_tx_data_n1,
            ip_address_n0,
            ip_address_n1,
            dropEveryNPacket
        );

        // monitor the n1 rx
        PRTRXMETA(1);
        PRTRXDATA(1);

        // monitor the n0 rx
        PRTRXMETA(0);
        PRTRXDATA(0);

    }

    return 0;
}



