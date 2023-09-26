# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(rocev2
        VERSION 0.82)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")
set(FNS_ROCE_STACK_MAX_QPS 500 CACHE STRING "Maximum number of queue pairs the RoCE stack can support")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "../axi_utils.cpp"
                    "rocev2.cpp"
                    "rocev2.hpp"
                    "../ipv4/ipv4_utils.cpp"
                    "../ipv4/ipv4.cpp"
                    "../ipv6/ipv6.cpp"
                    "../udp/udp.cpp"
                    "../ib_transport_protocol/ib_utils.cpp"
                    "../ib_transport_protocol/ib_transport_protocol.cpp"
                    "../ib_transport_protocol/read_req_table.cpp"
                    "../ib_transport_protocol/transport_timer.cpp"
                    "../ib_transport_protocol/retransmitter/retransmitter.cpp"
                    "../ib_transport_protocol/state_table.cpp"
                    "../ib_transport_protocol/conn_table.cpp"
                    "../ib_transport_protocol/msn_table.cpp"
                 TB_FILES
                    "test_rocev2.cpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH} -DROCE_STACK_MAX_QPS=${FNS_ROCE_STACK_MAX_QPS}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "RoCEv2 Endpoint"
                 DESCRIPTION ""
                 VERSION ${PROJECT_VERSION})

# TODO
# csim_design -argv {${CMAKE_CURRENT_SOURCE_DIR}/write_read_read_large_receiver.in ${CMAKE_CURRENT_SOURCE_DIR}/write_read_read_large_receiver.out ${CMAKE_CURRENT_SOURCE_DIR}/rdma_txwriteread.in ${CMAKE_CURRENT_SOURCE_DIR}/rdma_txwriteread.out}
