# Author:  David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(ethernet_frame_padding
        VERSION 0.2)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Domain options
set(FNS_DATA_WIDTH 8 CACHE STRING "Width of data path in bytes")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "ethernet_frame_padding.cpp"
                    "ethernet_frame_padding.hpp"
                 HLS_FLAGS
                    "-DFNS_DATA_WIDTH=${FNS_DATA_WIDTH}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "Ethernet Frame Padding"
                 DESCRIPTION ""
                 VERSION ${PROJECT_VERSION})
