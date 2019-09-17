# TCP/IP Stack Design Using Vivado HLS

## Getting Started

### Prerequisites
- Xilinx Vivado 2018.1
- License for Xilinx 10G MAC IP
- Linux OS

Supported boards (out of the box)
- Xilinx VC709
- Xilinx VCU118
- Alpha Data ADM-PCIE-7V3

### Installation

Make sure that Vivado and Vivado HLS are in your PATH. Use version 2018.1

Navigate to the _hls_ directory:

    cd hls

Execute the script generate the HLS IP cores for your board:

    ./generate_hls vc709

For the VCU118 run

    ./generate_hls vcu118


Navigate to the _projects_ directory:

    cd ../projects

Create the example project for your board.

For the Xilinx VC709:

    vivado -mode batch -source create_vc709_proj.tcl

For the Alpha DATA ADM-PCIE-7V3:

    vivado -mode batch -source reate_adm7v3_proj.tcl

For the Xilinx VCU118:

    vivado -mode batch -source create_vcu118_proj.tcl


After the previous command executed, a Vivado project will be created and the Vivado GUI is started.

Click "Generate Bitstream" to generate a bitstream for the FPGA.

## Testing the example project

The default configuration deploys a TCP echo server and a UDP iperf client. The default IP address the board is 10.1.212.209. Make sure the testing machine conencted to the FPGA board is in the same subnet 10.1.212.*

As an intial connectivity test ping the FPGA board by running 

    ping 10.1.212.209

After reprogramming the FPGA the first ping message is lost due to a missing ARP entry in the ARP table. However, the FPGA should reply to all following ping messages.

  
For the TCP echo server you can use netcat:

    echo 'hello world' | netcat -q 1 11.1.212.209 7

Alternatively, you can use the _echoping_ linux commandline tool.

For the TCP and UDP iperf test, see [here](http://github.com/dsidler/fpga-network-stack/wiki/iPerf-Benchmark). 


## Configuration

Coming soon

## Publications
- D. Sidler, G. Alonso, M. Blott, K. Karras et al., *Scalable 10Gbps
TCP/IP Stack Architecture for Reconfigurable Hardware,* in FCCM’15, [Paper](http://davidsidler.ch/files/fccm2015-tcpip.pdf), [Slides](http://fccm.org/2015/pdfs/M2_P1.pdf)

- D. Sidler, Z. Istvan, G. Alonso, *Low-Latency TCP/IP Stack for Data Center Applications,* in FPL'16, [Paper](http://davidsidler.ch/files/fpl16-lowlatencytcpip.pdf)

## Citation
If you use the TCP/IP stack in your project please cite one of the following papers and/or link to the github project:
```
@INPROCEEDINGS{sidler2015tcp, 
	author={D. Sidler and G. Alonso and M. Blott and K. Karras and others}, 
	booktitle={FCCM'15}, 
	title={{Scalable 10Gbps TCP/IP Stack Architecture for Reconfigurable Hardware}}, 
}
@INPROCEEDINGS{sidler2016lowlatencytcp, 
	author={D. Sidler and Z. Istvan and G. Alonso}, 
	booktitle={FPL'16}, 
	title={{Low-Latency TCP/IP Stack for Data Center Applications}}, 
}
```

For more information please visit the [wiki](http://github.com/dsidler/fpga-network-stack/wiki)
