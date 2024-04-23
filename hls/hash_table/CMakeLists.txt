# Author: David Sidler (david.sidler@inf.ethz.ch)

cmake_minimum_required(VERSION 3.0)

project(hash_table
        VERSION 1.0)

# Target options
set(FNS_PLATFORM_PART "xcu55c-fsvh2892-2L-e" CACHE STRING "FPGA platform part")
set(FNS_TCP_STACK_MAX_SESSIONS 1000 CACHE STRING "Max number of entries")

# Find Vitis
find_package(Vitis REQUIRED)

add_vitis_ip(${PROJECT_NAME}
                 FILES
                    "hash_table.cpp"
                    "hash_table.hpp"
                 TB_FILES
                    "test_hash_table.cpp"
                 HLS_FLAGS
                    "-DTCP_STACK_MAX_SESSIONS=${FNS_TCP_STACK_MAX_SESSIONS}"
                    "-DFNS_ROCE_STACK_MAX_QPS=${FNS_ROCE_STACK_MAX_QPS}"
                 PLATFORM_PART ${FNS_PLATFORM_PART}
                 VENDOR "ethz.systems.fpga"
                 DISPLAY_NAME "Hash Table (cuckoo)"
                 DESCRIPTION ""
                 VERSION ${PROJECT_VERSION})
