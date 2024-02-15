/**
  * Copyright (c) 2021, Systems Group, ETH Zurich
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *
  * 1. Redistributions of source code must retain the above copyright notice,
  * this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  * this list of conditions and the following disclaimer in the documentation
  * and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holder nor the names of its contributors
  * may be used to endorse or promote products derived from this software
  * without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
  * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

`ifndef LYNX_INTF_SV_
`define LYNX_INTF_SV_

`timescale 1ns / 1ps

import lynxTypes::*;

// ----------------------------------------------------------------------------
// Generic meta interface
// ----------------------------------------------------------------------------
interface metaIntf #(
	parameter type STYPE = logic[63:0]
) (
	input  logic aclk
);

logic valid;
logic ready;
STYPE data;

task tie_off_s ();
	ready = 1'b0;
endtask

task tie_off_m ();
	data = 0;
	valid = 1'b0;
endtask

modport s (
	import tie_off_s,
	input  valid,
	output ready,
	input  data
);

modport m (
	import tie_off_m,
	output valid,
	input  ready,
	output data
);

endinterface

// ----------------------------------------------------------------------------
// TLB interface
// ----------------------------------------------------------------------------
interface tlbIntf #(
	parameter TLB_INTF_DATA_BITS = TLB_DATA_BITS
);

typedef logic [VADDR_BITS-1:0] addr_t;
typedef logic [TLB_INTF_DATA_BITS-1:0] data_t;
typedef logic [PID_BITS-1:0] pid_t;

addr_t 			addr;
data_t 			data;

logic 			valid;
logic 			wr;
pid_t			pid;
logic 			hit;

modport s (
	input valid,
	input wr,
	input pid,
	input addr,
	output hit,
	output data
);

modport m (
	output valid,
	output wr,
	output pid,
	output addr,
	input hit,
	input data
);

endinterface

// ----------------------------------------------------------------------------
// DMA interface
// ----------------------------------------------------------------------------
interface dmaIntf (
	input  logic aclk
);

dma_req_t   			req;
dma_rsp_t				rsp;
logic 					valid;
logic 					ready;

task tie_off_s ();
	rsp = 0;
	ready = 1'b0;
endtask

task tie_off_m ();
	req = 0;
	valid = 1'b0;
endtask

// Slave
modport s (
	import tie_off_s,
	input  req,
	input  valid,
	output ready,
	output rsp
);

// Master
modport m (
	import tie_off_m,
	output req,
	output valid,
	input  ready,
	input  rsp
);

endinterface

// ----------------------------------------------------------------------------
// DMA ISR interface
// ----------------------------------------------------------------------------
interface dmaIsrIntf ();

dma_isr_req_t 			req;
dma_isr_rsp_t			rsp;
logic 					valid;
logic 					ready;

task tie_off_s ();
	rsp = 0;
	ready = 1'b0;
endtask

task tie_off_m ();
	req = 0;
	valid = 1'b0;
endtask

modport s (
    import tie_off_s,
	input  req,
	input  valid,
	output ready,
	output rsp
);

modport m (
    import tie_off_m,
	output req,
	output valid,
	input  ready,
	input  rsp
);

endinterface

// ---------------------------------------------------------------------------- 
// Multiplexer interface 
// ---------------------------------------------------------------------------- 
interface muxIntf #(
	parameter integer N_ID_BITS = N_REGIONS_BITS,
	parameter integer ARB_DATA_BITS = AXI_DATA_BITS
);

localparam integer BEAT_LOG_BITS = $clog2(ARB_DATA_BITS/8);

logic [N_ID_BITS-1:0]			    vfid;
logic [LEN_BITS-BEAT_LOG_BITS-1:0]	len;
logic 								ctl;

logic 							   	ready;
logic 							   	valid;
logic 								done;

task tie_off_s ();
	vfid = 0;
	len = 0;
	ctl = 1'b0;
	ready = 1'b0;
endtask

task tie_off_m ();
	valid = 1'b0;
	done = 1'b0;
endtask

modport s (
    import tie_off_s,
	output vfid,
	output len,
	output ctl,
	output ready,
	input  valid,
	input  done
);

modport m (
    import tie_off_m,
	input  vfid,
	input  len,
	input  ctl,
	input  ready,
	output valid,
	output done
);

endinterface

// ---------------------------------------------------------------------------- 
// XDMA bypass
// ---------------------------------------------------------------------------- 
interface xdmaIntf ();

logic [63:0] h2c_addr;
logic [27:0] h2c_len;
logic [15:0] h2c_ctl;
logic h2c_valid;
logic h2c_ready;

logic [63:0] c2h_addr;
logic [27:0] c2h_len;
logic [15:0] c2h_ctl;
logic c2h_valid;
logic c2h_ready;

logic [7:0] h2c_status;
logic [7:0] c2h_status;

// Slave
modport s (
	input h2c_addr,
	input h2c_len,
	input h2c_ctl,
	input h2c_valid,
	output h2c_ready,
	input c2h_addr,
	input c2h_len,
	input c2h_ctl,
	input c2h_valid,
	output c2h_ready,
	output h2c_status,
	output c2h_status
);

// Master
modport m (
	output h2c_addr,
	output h2c_len,
	output h2c_ctl,
	output h2c_valid,
	input h2c_ready,
	output c2h_addr,
	output c2h_len,
	output c2h_ctl,
	output c2h_valid,
	input c2h_ready,
	input h2c_status,
	input c2h_status
);

endinterface


`endif
