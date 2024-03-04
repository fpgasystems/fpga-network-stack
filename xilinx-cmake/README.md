# xilinx-cmake

This project is entirely based on [github.com/definelicht/hlslib](https://github.com/definelicht/hlslib). Please refer to the original repository for more information about documentation, support, contributing, etc.

# Feature overview

This cmake has an additional function `add_vitis_ip` to assist you to simulate, synthesise and export Vitis HLS IPs.

```cmake
add_vitis_ip(${PROJECT_NAME}
            FILES
              "../axi_utils.cpp"
              "arp_server_subnet.cpp"
              "arp_server_subnet.hpp"
            TB_FILES
              "test_arp_server_subnet.cpp"
            HLS_FLAGS
              "-DMY_IMPORTANT_DEFINITION -O2"
            PLATFORM_PART "xcu55c-fsvh2892-2L-e"
            VENDOR "ethz.systems.fpga"
            DISPLAY_NAME "ARP Subnet Server"
            DESCRIPTION "Replies to ARP queries and resolves IP addresses."
            VERSION ${PROJECT_VERSION}
            IP_DIR "./iprepo")
```

New targets:

```bash
make csim # C-Simulation (csim_design)
make synth # Synthesis (csynth_design)
make cosim # Co-Simulation (cosim_design)
make ip # Export IP (export_design)
```

# Projects using xilinx-cmake

 * [github.com/fpgasystems/strega](https://github.com/fpgasystems/strega) - Strega: An HTTP Server for FPGAs
 * [github.com/fpgasystems/Vitis_with_100Gbps_TCP-IP](https://github.com/fpgasystems/Vitis_with_100Gbps_TCP-IP) - Vitis TCP/IP stack
 * [github.com/fpgasystems/fpga-network-stack](https://github.com/fpgasystems/fpga-network-stack) - FPGA Network stack

## License

This software is copyrighted under the BSD 3-Clause License.
