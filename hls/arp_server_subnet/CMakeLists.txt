# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(arp_server_subnet
        VERSION 1.1)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "../axi_utils.cpp"
                    "arp_server_subnet.cpp"
                    "arp_server_subnet.hpp"
                 TB_FILES
                    "test_arp_server_subnet.cpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "ARP Subnet Server"
                 DESCRIPTION "Replies to ARP queries and resolves IP addresses."
                 VERSION ${PROJECT_VERSION})

# TODO
# csim_design -clean -argv {${CMAKE_CURRENT_SOURCE_DIR}/in.dat ${CMAKE_CURRENT_SOURCE_DIR}/out.dat}
