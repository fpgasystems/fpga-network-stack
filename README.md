# Scalable Network Stack supporting TCP/IP, RoCEv2, UDP/IP at 10-100Gbit/s

## Getting Started

### Prerequisites
- Xilinx Vivado 2019.1
- cmake 3.0 or higher

Supported boards (out of the box)
- Xilinx VC709
- Xilinx VCU118
- Alpha Data ADM-PCIE-7V3


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


For an example project including the TCP/IP stack or the RoCEv2 stack with DMA to host memory checkout our Distributed Accelerator OS (DavOS).


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

## Benchmarks



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
@PHDTHESIS{sidler2019innetworkdataprocessing,
	author = {Sidler, David},
	publisher = {ETH Zurich},
	year = {2019-09},
	copyright = {In Copyright - Non-Commercial Use Permitted},
	title = {In-Network Data Processing using FPGAs},
}

## Contributors
- [David Sidler](http://github.com/dsidler), [Systems Group] (http://systems.ethz.ch), ETH Zurich
- [Mario Ruiz](https://github.com/mariodruiz), HPCN Group of UAM, Spain
- [Kimon Karras] (http://github.com/kimonk), former Researcher at Xilinx Research, Dublin
- [Lisa Liu](http://github.com/lisaliu1), Xilinx Research, Dublin
```
