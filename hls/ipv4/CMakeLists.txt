# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(ipv4
        VERSION 0.1)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "../packet.hpp"
                    "../axi_utils.hpp"
                    "ipv4.cpp"
                    "ipv4.hpp"
                    "ipv4_utils.cpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "IPv4"
                 DESCRIPTION ""
                 VERSION ${PROJECT_VERSION})
