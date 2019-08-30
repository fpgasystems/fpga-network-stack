`timescale 1ns / 1ps
`default_nettype none

module register_slice_wrapper #(
    parameter WIDTH = 64
) (
    input wire          aclk,
    input wire          aresetn,
    axi_stream.slave    s_axis,
    axi_stream.master   m_axis
);


generate
if(WIDTH==64) begin
axis_register_slice_64 slice_inst(
 .aclk(aclk),
 .aresetn(aresetn),
 .s_axis_tvalid(s_axis.valid),
 .s_axis_tready(s_axis.ready),
 .s_axis_tdata(s_axis.data),
 .s_axis_tkeep(s_axis.keep),
 .s_axis_tlast(s_axis.last),
 .m_axis_tvalid(m_axis.valid),
 .m_axis_tready(m_axis.ready),
 .m_axis_tdata(m_axis.data),
 .m_axis_tkeep(m_axis.keep),
 .m_axis_tlast(m_axis.last)
);
end
if(WIDTH==128) begin
axis_register_slice_128 slice_inst(
 .aclk(aclk),
 .aresetn(aresetn),
 .s_axis_tvalid(s_axis.valid),
 .s_axis_tready(s_axis.ready),
 .s_axis_tdata(s_axis.data),
 .s_axis_tkeep(s_axis.keep),
 .s_axis_tlast(s_axis.last),
 .m_axis_tvalid(m_axis.valid),
 .m_axis_tready(m_axis.ready),
 .m_axis_tdata(m_axis.data),
 .m_axis_tkeep(m_axis.keep),
 .m_axis_tlast(m_axis.last)
);
end
if(WIDTH==256) begin
axis_register_slice_256 slice_inst(
 .aclk(aclk),
 .aresetn(aresetn),
 .s_axis_tvalid(s_axis.valid),
 .s_axis_tready(s_axis.ready),
 .s_axis_tdata(s_axis.data),
 .s_axis_tkeep(s_axis.keep),
 .s_axis_tlast(s_axis.last),
 .m_axis_tvalid(m_axis.valid),
 .m_axis_tready(m_axis.ready),
 .m_axis_tdata(m_axis.data),
 .m_axis_tkeep(m_axis.keep),
 .m_axis_tlast(m_axis.last)
);
end
if(WIDTH==512) begin
axis_register_slice_512 slice_inst(
 .aclk(aclk),
 .aresetn(aresetn),
 .s_axis_tvalid(s_axis.valid),
 .s_axis_tready(s_axis.ready),
 .s_axis_tdata(s_axis.data),
 .s_axis_tkeep(s_axis.keep),
 .s_axis_tlast(s_axis.last),
 .m_axis_tvalid(m_axis.valid),
 .m_axis_tready(m_axis.ready),
 .m_axis_tdata(m_axis.data),
 .m_axis_tkeep(m_axis.keep),
 .m_axis_tlast(m_axis.last)
);
end
endgenerate

endmodule
`default_nettype wire