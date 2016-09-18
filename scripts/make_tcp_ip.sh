#!/bin/bash
################################################################################
# Author: Lisa Liu
# Date:	2016/07/29
#
# Usage:
#			./make_tcp_ip.sh
# Vivado_hls version:
#			2015.1
################################################################################
BUILDDIR="$PWD/build"

echo "BUILDDIR is $BUILDDIR"

if [ -d "$BUILDDIR" ]; then
	eval cd "$BUILDDIR"
	echo "$PWD"
else
	mkdir "$BUILDDIR"
	eval cd "$BUILDDIR"
	echo "PWD"
fi

eval cp -r ../../hls ./

eval cd ./hls/arp_server
eval vivado_hls -f run_hls.tcl

eval cd ./hls/arp_server_subnet
eval vivado_hls -f run_hls.tcl

eval cd ../dhcp_client
eval vivado_hls -f run_hls.tcl

eval cd ../echo_server_application
eval vivado_hls -f run_hls.tcl

eval cd ../icmp_server
eval vivado_hls -f run_hls.tcl

eval cd ../ip_handler
eval vivado_hls -f run_hls.tcl

eval cd ../mac_ip_encode
eval vivado_hls -f run_hls.tcl

eval cd ../toe
eval vivado_hls -f run_hls.tcl

eval cd ../udp/udpCore
eval vivado_hls -f run_hls.tcl

eval cd ../../udp/udpLoopback
eval vivado_hls -f run_hls.tcl

eval cd ../../udp/udpAppMux
eval vivado_hls -f run_hls.tcl

echo "Finished HLS kernel synthesis"
echo "Create ipRepository"

eval cd "$BUILDDIR"
IPREPOSITORYDIR="$BUILDDIR/ipRepository"

if [ -d "$IPREPOSITORYDIR" ]; then
	eval cd "$IPREPOSITORYDIR"
	echo "$PWD"
else
	mkdir "$IPREPOSITORYDIR"
	eval cd "$IPREPOSITORYDIR"
	echo "PWD"
fi

eval cp -R ../hls/arp_server/arp_server_prj/solution1/impl/ip ./arp_server
eval cp -R ../hls/arp_server_subnet/arp_server_subnet_prj/solution1/impl/ip ./arp_server_subnet
eval cp -R ../hls/dhcp_client/dhcp_prj/solution1/impl/ip ./dhcp_client
eval cp -R ../hls/echo_server_application/echo_server_prj/solution1/impl/ip ./echo_server
eval cp -R ../hls/icmp_server/icmpServer_prj/solution1/impl/ip ./icmp_server
eval cp -R ../hls/ip_handler/ipHandler_prj/solution1/impl/ip ./ip_handler
eval cp -R ../hls/mac_ip_encode/macIpEncode_prj/solution1/impl/ip ./mac_ip_encode
eval cp -R ../hls/toe/toe_prj/solution1/impl/ip ./toe_ip
eval cp -R ../hls/udp/udpCore/udp_prj/solution1/impl/ip ./udpCore
eval cp -R ../hls/udp/udpLoopback/udpLoopback_prj/solution1/impl/ip ./udpLoopback
eval cp -R ../hls/udp/udpAppMux/udpAppMux_prj/solution1/impl/ip ./

echo "finished preparation for creating vivado project, please run vivado -mode batch -source create.tcl to create vivado project" 

