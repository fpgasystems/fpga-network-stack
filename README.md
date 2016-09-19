
TCP/IP Stack Design Using Vivado HLS
======================================

This readme file contains these sections:

1. INTRODUCTION
2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS
3. DESIGN FILE HIERARCHY
4. INSTALLATION AND OPERATING INSTRUCTIONS
5. PUBLICATIONS
6. SUPPORT
7. LICENSE
8. CONTRIBUTING
9. ACKNOWLEDGEMENTS


1. INTRODUCTION

The TCP/IP stack released in this repository was originally developed in collaboration with Xilinx Research Dublin. Xilinx open sourced the original version which came out of this collaboartion on their [github](https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip). In this repository we release our ongoing development of the TCP/IP stack based on the version released by Xilinx. We optimized the orginal version to reduce latency, and resource consumption. Further we addded support for the Xilinx VC709 board.


2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS

	* Xilinx Vivado 2015.3 or 2016.2 (most recent version should work)
	* License for Xilinx 10G MAC IP
	* Linux OS
	* Xilinx VC709 or ADM-PCIE-7V3 FPGA card from Alpha Data

3. DESIGN FILE HIERARCHY 

```
\
  | 
  +------ \hls
  |     Contains all the Vivado HLS projects for implementing the TCP Offload engine and related modules.
  +------ \rtl
  |			Contains all the rtl code used to create Vivado project.
  +------ \ip
  |			Contains all the netlist files used to create Vivado project.
  +------ \constraints
  |			Contains all the constraint files for generating bitstream
  +------ \scripts
  | 		Contain all scripts for building Vivado project, generating bitstream and testing bitstream
  +------ \doc
  |			Contain the design document for the TCP offload engine			Contain the design document for the TCP offload engine
```

4. INSTALLATION AND OPERATING INSTRUCTIONS

To create a Vivado project for the VC709 or ADM-PCIE-7V3 and generate a bitstream,  follow the steps below:

	1. make sure Xilinx Vivado HLS 2016.2 is set in your PATH environment variable (Vivado 2015.3 for the ADM-PCIE-7V3)
	2. make sure bash is installed on your computer
	3. navigate to tcp_ip/scripts
	4. edit the first line in make_tcp_ip.sh to point to the bash installed on your computer
	5. run "./make_tcp_ip.sh"
	6. run "vivado -mode batch -source create_vc709_proj.tcl" (create_adm7v3_proj.tcl for the ADM-PCIE-7V3)
	7. at the end of step 7, an Vivao project will created and opened in the Vivado gui.
	8. Set the IP address, DefaultGateway and MAC address if required at the top of the tcp_ip_top.v file
	9. Click "Generate Bitstream" to generate a bitstream for the FPGA on ADM-PCIE-7V3 board.

To test the generated .bit file, please follow the steps below:
	
	1. deploy the generated .bit file on the Xilinx VC709 or Alpha Data 7V3 board. This .bit file loops back both UDP and TCP packetssent to the FPGA.
	2. connect the 10G ETH port on another PC to the 10G ETH port on the FPGA board	and set the IP address of this PC 10G to be in the same subnet as the FPGA.
	3. from the PC console enter following command to check if you can ping the FPGA successfully by running following command:
	ping <FPGA-ip-address>
	4. navigate to  tcp_ip/scripts and enter following command to send packets which will be looped back.
			python ./client_length_mt_fixed_char.py --ip=<FPGA-ip-address>
		This python script basically will create two sessions and send packets to the TOE and the close the session.

		if you run following command, UDP packets will be looped back.
			python ./client_fixed_length.py --ip=<FPGA-ip-address>
		

## 5. PUBLICATIONS

D. Sidler, G. Alonso, M. Blott, K. Karras et al., *Scalable 10Gbps
TCP/IP Stack Architecture for Reconfigurable Hardware,* in FCCM’15, [Paper](http://ieeexplore.ieee.org/document/7160037/?reload=true&arnumber=7160037), [Slides](http://fccm.org/2015/pdfs/M2_P1.pdf)

D. Sidler, Z. Istvan, G. Alonso, *Low-Latency TCP/IP Stack for Data Center Applications,* in FPL'16, [Paper](http://davidsidler.ch/files/fpl16-lowlatencytcpip.pdf), [Slides]

## 6. SUPPORT

For questions and to get help on this project please contact david.sidler@inf.ethz.ch. 

## 7. License
The source for this project is licensed under the [3-Clause BSD License][]

## 8. Contributing code
TBD

## 9. Acknowledgements
The orginal version of this TCP/IP stack was developed in collaboration with [Xilinx](http://www.xilinx.com/) Research, Dublin, Ireland. This orignal version was open sourced at https://github.com/Xilinx/HLx_Examples/tree/master/Acceleration/tcp_ip

[3-Clause BSD License]: LICENSE.md
