# Scalable Network Stack supporting TCP/IP, RoCEv2, UDP/IP at 10-100Gbit/s

#### Table of contents
1. [Getting Started](#gettingstarted)
2. [Compiling HLS modules](#compiling)
3.	[Interfaces](#interfaces)
	1.	[TCP/IP](#tcp-interface)
	2. [ROCE](#roce-interface)
4. [Benchmarks](#benchmarks)
5.	[Publications](#publications)
6. [Citation](#citation)
7. [Contributors](#contributors)


<a name="gettingstarted"></a>
## Getting Started

### Prerequisites
- Xilinx Vivado 2019.1
- cmake 3.0 or higher

Supported boards (out of the box)
- Xilinx VC709
- Xilinx VCU118
- Alpha Data ADM-PCIE-7V3

<a name="compiling"></a>
## Compiling (all) HLS modules and install them to your IP repository

0. Optionally specify the location of your IP repository:
```
export $IPREPO_DIR=/home/myname/iprepo
```

1. Create a build directory
```
mkdir build
cd build
```

2.a) Configure build
```    
cmake .. -DDATA_WIDTH=64 -DCLOCK_PERIOD=3.1 -DFPGA_PART=xcvu9p-flga2104-2L-e -DFPGA_FAMILY=ultraplus -DVIVADO_HLS_ROOT_DIR=/opt/Xilinx/Vivado//2019.1/bin/
```

2.b)Alternatively you can use one the board name ot configure your build
```
cmake .. -DDEVICE_NAME=vcu118
```

All cmake  options:

| Name                        | Values                | Desription                                                              |
| --------------------------- | --------------------- | ----------------------------------------------------------------------- |
| DEVICE_NAME                 | <vc709,vcu118,adm7v3> | Supported devices                                                       |
| NETWORK_BANDWIDTH           | <10,100>              | Bandwidth of the Ethernet interface in Gbit/s, default depends on board |
| FPGA_PART                   | <name>                | Name of the FPGA part, e.g. xc7vx690tffg1761-2                          |
| FPGA_FAMILY                 | <7series,ultraplus>   | Name of the FPGA part family                                            |
| DATA_WIDTH                  | <8,16,32,64>          | Data width of the network stack in bytes                                |
| CLOCK_PERIOD                | <nanoseconds>         | Clock period in nanoseconds, e.g. 3.1 for 100G, 6.4 for 10G             |
| TCP_STACK_MSS               | <value>               | Maximum segment size of the TCP/IP stack                                |
| TCP_STACK_WINDOW_SCALING_EN | <0,1>                 | Enalbing TCP Window scaling option           |
| VIVADO_HLS_ROOT_DIR         | <path>                | Path to Vivado HLS directory, e.g. /opt/Xilinx/Vivado/2019.1            |

3. Build HLS IP cores and install them into IP repository
```
make installip
``` 


For an example project including the TCP/IP stack or the RoCEv2 stack with DMA to host memory checkout our Distributed Accelerator OS [DavOS](https://github.com/fpgasystems/davos).


## Working with individual HLS modules

1. Setup build directory, e.g. for the TCP module

```
$ cd hls/toe
$ mkdir build
$ cd build
$ cmake .. -DFPGA_PART=xcvu9p-flga2104-2L-e -DDATA_WIDTH=8 -DCLOCK_PERIOD=3.1
```

2. Run c-simulation
```
$ make csim
```

3. Run c-synthesis
```
$ make synthesis
```

4. Generate HLS IP core
```
$ make ip
```

5. Install HLS IP core into the IP repository
```
$ make installip
```

<a name="interfaces"></a>
## Interfaces
All interfaces are using the AXI4-Stream protocol. For AXI4-Streams carrying network/data packets, we use the following definition in HLS:
```
template <int D>
struct net_axis
{
	ap_uint<D>    data;
	ap_uint<D/8>  keep;
	ap_uint<1>    last;
};
```

<a name="tcp-interface"></a>
### TCP/IP

#### Open Connection
To open a connection the destination IP address and TCP port have to provided through the `s_axis_open_conn_req` interface. The TCP stack provides an answer to this request through the `m_axis_open_conn_rsp` interface which provides the sessionID and a boolean indicating if the connection was openend successfully.

Interface definition in HLS:
```
struct ipTuple
{
	ap_uint<32>	ip_address;
	ap_uint<16>	ip_port;
};
struct openStatus
{
	ap_uint<16>	sessionID;
	bool		success;
};

void toe(...
	hls::stream<ipTuple>& openConnReq,
	hls::stream<openStatus>& openConnRsp,
	...);
```




#### Close Connection
To close a connection the sessionID has to be provided to the `s_axis_close_conn_req` interface. The TCP/IP stack does not provide a notification upon completion of this request, however it is guranteeed that the connection is closed eventually.

Interface definition in HLS:
```
hls::stream<ap_uint<16> >& closeConnReq,

```

#### Open a TCP port to listen on
To open a port to listen on (e.g. as a server), the port number has to be provided to `s_axis_listen_port_req`. The port number has to be in range of active ports: 0 - 32767. The TCP stack will respond through the `m_axis_listen_port_rsp` interface indicating if the port was set to the listen state succesfully.

Interface definition in HLS:
```
hls::stream<ap_uint<16> >& listenPortReq,
hls::stream<bool>& listenPortRsp,
```

#### Receiving notifications from the TCP stack
The application using the TCP stack can receive notifications through the `m_axis_notification` interface. The notifications either indicate that new data is available or that a connection was closed.

Interface definition in HLS:
```
struct appNotification
{
	ap_uint<16>			sessionID;
	ap_uint<16>			length;
	ap_uint<32>			ipAddress;
	ap_uint<16>			dstPort;
	bool				closed;
};

hls::stream<appNotification>& notification,
```

#### Receiving data
If data is available on a TCP/IP session, i.e. a notification was received. Then this data can be requested through the `s_axis_rx_data_req` interface. The data as well as the sessionID are then received through the `m_axis_rx_data_rsp_metadata` and `m_axis_rx_data_rsp` interface.

Interface definition in HLS:
```
struct appReadRequest
{
	ap_uint<16> sessionID;
	ap_uint<16> length;
};

hls::stream<appReadRequest>& rxDataReq,
hls::stream<ap_uint<16> >& rxDataRspMeta,
hls::stream<net_axis<WIDTH> >& rxDataRsp,
```

Waveform of receiving a (data) notification, requesting data, and receiving the data:

![signal tcp-rx-handshake](https://svg.wavedrom.com/github/fpgasystems/fpga-network-stack/master/waveforms/tcp-rx-handshake.json5)


#### Transmitting data
When an application wants to transmit data on a TCP connection, it first has to check if enough buffer space is available. This check/request is done through the `s_axis_tx_data_req_metadata` interface. If the response through the `m_axis_tx_data_rsp` interface from the TCP stack is positive. The application can send the data through the `s_axis_tx_data_req` interface. If the response from the TCP stack is negative the application can retry by sending another request on the `s_axis_tx_data_req_metadata` interface.

Interface definition in HLS:
```
struct appTxMeta
{
	ap_uint<16> sessionID;
	ap_uint<16> length;
};
struct appTxRsp
{
	ap_uint<16> sessionID;
	ap_uint<16> length;
	ap_uint<30> remaining_space;
	ap_uint<2>  error;
};

hls::stream<appTxMeta>& txDataReqMeta,
hls::stream<appTxRsp>& txDataRsp,
hls::stream<net_axis<WIDTH> >& txDataReq,
```

Waveform of requesting a data transmit and transmitting the data. 
![signal tcp-tx-handshake](https://svg.wavedrom.com/github/fpgasystems/fpga-network-stack/master/waveforms/tcp-tx-handshake.json5)


<a name="roce-interface"></a>
### RoCE (RDMA over Converged Ethernet)

#### Load Queue Pair (QP)
Before any RDMA operations can be executed the Queue Pairs have to established out-of-band (e.g. over TCP/IP) by the hosts. The host can the load the QP into the RoCE stack through the `s_axis_qp_interface` and `s_axis_qp_conn_interface` interface.

Interface definition in HLS:
```
typedef enum {RESET, INIT, READY_RECV, READY_SEND, SQ_ERROR, ERROR} qpState;

struct qpContext
{
	qpState		newState;
	ap_uint<24> qp_num;
	ap_uint<24> remote_psn;
	ap_uint<24> local_psn;
	ap_uint<16> r_key;
	ap_uint<48> virtual_address;
};
struct ifConnReq
{
	ap_uint<16> qpn;
	ap_uint<24> remote_qpn;
	ap_uint<128> remote_ip_address;
	ap_uint<16> remote_udp_port;
};

hls::stream<qpContext>&	s_axis_qp_interface,
hls::stream<ifConnReq>&	s_axis_qp_conn_interface,
```

#### Issue RDMA commands
RDMA commands can be issued to RoCE stack through the `s_axis_tx_meta` interface. In case the commands transmits data. This data can be either originate from the host memory as specified by the `local_vaddr` or can originate from the application on the FPGA. In the latter case the `local_vaddr` is set to 0 and the data is provided through the `s_axis_tx_data` interface.

Interface definition in HLS:
```
typedef enum {APP_READ, APP_WRITE, APP_PART, APP_POINTER, APP_READ_CONSISTENT} appOpCode;

struct txMeta
{
	appOpCode 	op_code;
	ap_uint<24> qpn;
	ap_uint<48> local_vaddr;
	ap_uint<48> remote_vaddr;
	ap_uint<32> length;
};
hls::stream<txMeta>& s_axis_tx_meta,
hls::stream<net_axis<WIDTH> >& s_axis_tx_data,
```
Waveform of issuing a RDMA read request:
![signal roce-read-handshake](https://svg.wavedrom.com/github/fpgasystems/fpga-network-stack/master/waveforms/roce-read-handshake.json5)

Waveform of issuing an RDMA write request where data on the FPGA is transmitted:
![signal roce-write-handshake](https://svg.wavedrom.com/github/fpgasystems/fpga-network-stack/master/waveforms/roce-write-handshake.json5)



<a name="interfaces"></a>
## Benchmarks
(Coming soon)


<a name="publications"></a>
## Publications
- D. Sidler, G. Alonso, M. Blott, K. Karras et al., *Scalable 10Gbps
TCP/IP Stack Architecture for Reconfigurable Hardware,* in FCCM’15, [Paper](http://davidsidler.ch/files/fccm2015-tcpip.pdf), [Slides](http://fccm.org/2015/pdfs/M2_P1.pdf)

- D. Sidler, Z. Istvan, G. Alonso, *Low-Latency TCP/IP Stack for Data Center Applications,* in FPL'16, [Paper](http://davidsidler.ch/files/fpl16-lowlatencytcpip.pdf)

- D. Sidler, Z. Wang, M. Chiosa, A. Kulkarni, G. Alonso, *StRoM: smart remote memory,* in EuroSys'20, [Paper](https://dl.acm.org/doi/abs/10.1145/3342195.3387519)


<a name="citation"></a>
## Citation
If you use the TCP/IP stack or the RDMA stack in your project please cite one of the following papers and/or link to the github project:
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
@PHDTHESIS{sidler2019innetworkdataprocessing,
	author = {Sidler, David},
	publisher = {ETH Zurich},
	year = {2019-09},
	copyright = {In Copyright - Non-Commercial Use Permitted},
	title = {In-Network Data Processing using FPGAs},
}
@INPROCEEDINGS{sidler2020strom,
	author = {Sidler, David and Wang, Zeke and Chiosa, Monica and Kulkarni, Amit and Alonso, Gustavo},
	booktitle = {Proceedings of the Fifteenth European Conference on Computer Systems},
	title = {StRoM: Smart Remote Memory},
	doi = {10.1145/3342195.3387519},
}
```

<a name="contributors"></a>
## Contributors
- [David Sidler](http://github.com/dsidler), [Systems Group](http://systems.ethz.ch), ETH Zurich
- [Monica Chiosa](http://github.com/chipet), [Systems Group](http://systems.ethz.ch), ETH Zurich
- [Zhenhao He](http://github.com/zhenhaohe), [Systems Group](http://systems.ethz.ch), ETH Zurich
- [Mario Ruiz](https://github.com/mariodruiz), HPCN Group of UAM, Spain
- [Kimon Karras](http://github.com/kimonk), former Researcher at Xilinx Research, Dublin
- [Lisa Liu](http://github.com/lisaliu1), Xilinx Research, Dublin
