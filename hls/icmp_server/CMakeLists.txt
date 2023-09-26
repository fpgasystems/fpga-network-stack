# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(icmp_server
        VERSION 1.67)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "icmp_server.cpp"
                    "icmp_server.hpp"
                 TB_FILES
                    "test_icmp_server.cpp"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "xilinx.labs"
                 DISPLAY_NAME "ICMP Server"
                 DESCRIPTION "Replies to ARP queries and resolves IP addresses."
                 VERSION ${PROJECT_VERSION})
