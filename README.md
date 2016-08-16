
TCP_IP Offload Engine Design Using Vivado High-Level Synthesis and Vivado
======================================

This readme file contains these sections:

1. OVERVIEW
2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS
3. DESIGN FILE HIERARCHY
4. INSTALLATION AND OPERATING INSTRUCTIONS
5. OTHER INFORMATION (OPTIONAL)
6. SUPPORT
7. LICENSE
8. CONTRIBUTING
9. Acknowledgements
10. REVISION HISTORY

1. OVERVIEW

This is readme file provides a basic explanation of the file structure in $the TCP Offload Engine repository, including instructions on how to compile, synthesize and test the HLS part of the design as well as how to integrate it with the provided RTL infrastructure and test it on the Xilinx VC709 development board. The TCP Offload engine repo includes:

- Vivado HLS projects of the TCP Offload engine itself along with any peripheral modules necessary for it to operate.
- A simple test client which can be executed on any linux-based system to open sessions on the FPGA.


2. SOFTWARE TOOLS AND SYSTEM REQUIREMENTS

	* Xilinx Vivado HLS 2015.1
	* Xilinx Vivado 2015.3
	* License for Xilinx 10G MAC IP
	* Linux OS
	* ADM-PCIE-7V3 FPGA card from Alpha Data

3. DESIGN FILE HIERARCHY 

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
	|			Contain the design document for the TCP offload engine

4. INSTALLATION AND OPERATING INSTRUCTIONS

To create Vivado project and generate bitstream for the TCP_IP offload engine, please follow the steps below:

	1. make sure Xilinx Vivado HLS 2015.1 is set in your PATH environment variable
	2. make sure bash is installed on your computer
	3. navigate to tcp_ip/scripts
	4. edit the first line in make_tcp_ip.sh to point to the bash installed on your computer
	5. run "./make_tcp_ip.sh"
	6. after step 5, make sure Vivado 2015.3 is set in your PATH environemnt variable
	7. run "vivado -mode batch -source create.tcl"
	8. at the end of step 7, an Vivao project will created and opened in the Vivado gui.
	9. Click "Generate Bitstream" to generate a bitstream for the FPGA on ADM-PCIE-7V3 board.

To test the generated .bit file, please follow the steps below:
	
	1. deploy the generated .bit file to Alpha Data 7V3 board. This .bit file loops back both UDP and TCP packetssent to FPGA.
	2. connect the 10G ETH port on another PC to the 10G ETH port on the FPGA board	and set the IP address of this PC 10G as 1.1.1.20.
	3. from the PC console enter following command to check if you can ping the FPGA successfully by running following command:
	ping 1.1.1.1
	4. navigate to  tcp_ip/scripts and enter following command to send packets which will be looped back.
			python ./client_length_mt_fixed_char.py
		This python script basically will create two sessions and send packets to the TOE and the close the session.

		if you run following command, UDP packets will be looped back.
			python ./client_fixed_length.py
		

## 5. OTHER INFORMATION


## 6. SUPPORT

For questions and to get help on this project or your own projects, visit the [Vivado HLS Forums][]. 

## 7. License
The source for this project is licensed under the [3-Clause BSD License][]

## 8. Contributing code
Please refer to and read the [Contributing][] document for guidelines on how to contribute code to this open source project. The code in the `/master` branch is considered to be stable, and all pull-requests should be made against the `/develop` branch.

## 9. Acknowledgements
This project is written by developers at [Xilinx](http://www.xilinx.com/) with other contributors listed below:

## 10. REVISION HISTORY

Date		|	Readme Version		|	Revision Description
------------|-----------------------|-------------------------
Aug2016		|	1.0					|	Initial Xilinx release
