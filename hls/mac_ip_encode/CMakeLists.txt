# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(mac_ip_encode
        VERSION 2.0)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "../axi_utils.cpp"
                    "../ipv4/ipv4_utils.cpp"
                    "../ethernet/ethernet.hpp"
                    "mac_ip_encode.cpp"
                    "mac_ip_encode.hpp"
                 TB_FILES
                    "test_mac_ip_encode.cpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH}"
                    "-DFNS_ROCE_STACK_MAX_QPS=${FNS_ROCE_STACK_MAX_QPS}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "MAC IP Encoder"
                 DESCRIPTION ""
                 VERSION ${PROJECT_VERSION})

# TODO
# csim_design -argv {${CMAKE_CURRENT_SOURCE_DIR}/in.dat ${CMAKE_CURRENT_SOURCE_DIR}/tcp.out}
