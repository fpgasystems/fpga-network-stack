# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(toe
        VERSION 1.6)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")
set(FNS_TCP_STACK_FAST_RETRANSMIT_EN 1 CACHE STRING "Enabling TCP fast retransmit")
set(FNS_TCP_STACK_NODELAY_EN 1 CACHE STRING "Toggles Nagle's Algorithm on/off")
set(FNS_TCP_STACK_MAX_SESSIONS 1000 CACHE STRING "Maximum number of sessions the TCP/IP stack can support")
set(FNS_TCP_STACK_MSS 4096 CACHE STRING "Maximum Segment Size (MSS)")
set(FNS_TCP_STACK_RX_DDR_BYPASS_EN 1 CACHE STRING "Enabling DDR bypass on the RX path")
set(FNS_TCP_STACK_WINDOW_SCALING_EN 1 CACHE STRING "Enabling TCP window scaling option")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "../axi_utils.cpp"
                    "ack_delay/ack_delay.cpp"
                    "close_timer/close_timer.cpp"
                    "event_engine/event_engine.cpp"
                    "port_table/port_table.cpp"
                    "probe_timer/probe_timer.cpp"
                    "retransmit_timer/retransmit_timer.cpp"
                    "rx_app_if/rx_app_if.cpp"
                    "rx_app_stream_if/rx_app_stream_if.cpp"
                    "rx_engine/rx_engine.cpp"
                    "rx_sar_table/rx_sar_table.cpp"
                    "session_lookup_controller/session_lookup_controller.cpp"
                    "state_table/state_table.cpp"
                    "tx_app_if/tx_app_if.cpp"
                    "tx_app_stream_if/tx_app_stream_if.cpp"
                    "tx_engine/tx_engine.cpp"
                    "tx_sar_table/tx_sar_table.cpp"
                    "tx_app_interface/tx_app_interface.cpp"
                    "toe.cpp"
                 TB_FILES
                    "toe_tb.cpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH}"
                    "-DTCP_NODELAY=${FNS_TCP_STACK_NODELAY_EN}"
                    "-DTCP_MSS=${FNS_TCP_STACK_MSS}"
                    "-DTCP_STACK_MAX_SESSIONS=${FNS_TCP_STACK_MAX_SESSIONS}"
                    "-DRX_DDR_BYPASS=${FNS_TCP_STACK_RX_DDR_BYPASS_EN}"
                    "-DFAST_RETRANSMIT=${FNS_TCP_STACK_FAST_RETRANSMIT_EN}"
                    "-DWINDOW_SCALE=${FNS_TCP_STACK_WINDOW_SCALING_EN}"
                    "-DFNS_ROCE_STACK_MAX_QPS=${FNS_ROCE_STACK_MAX_QPS}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "10G TCP Offload Engine"
                 DESCRIPTION "TCP Offload Engine supporting 10Gbps line rate, up to 10K concurrent sessions."
                 VERSION ${PROJECT_VERSION})

# TODO
# csim_design -clean -argv {0 ${CMAKE_CURRENT_SOURCE_DIR}/testVectors/io_fin_5.dat ${CMAKE_CURRENT_SOURCE_DIR}/testVectors/rxOutput.dat ${CMAKE_CURRENT_SOURCE_DIR}/testVectors/txOutput.dat ${CMAKE_CURRENT_SOURCE_DIR}/testVectors/rx_io_fin_5.gold}
