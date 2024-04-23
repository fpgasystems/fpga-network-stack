# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(dhcp_client
        VERSION 1.05)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "dhcp_client.cpp"
                    "dhcp_client.hpp"
                 TB_FILES
                    "test_dhcp_client.cpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH}"
                    "-DFNS_ROCE_STACK_MAX_QPS=${FNS_ROCE_STACK_MAX_QPS}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "xilinx.labs"
                 DISPLAY_NAME "DHCP Client"
                 DESCRIPTION "DHCP Client to be used with the Xilinx Labs TCP & UDP offload engines."
                 VERSION ${PROJECT_VERSION})
