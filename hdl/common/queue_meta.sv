import lynxTypes::*;

module queue_meta #(
    parameter QDEPTH = 8
) (
    input  logic        aclk,
    input  logic        aresetn,

    metaIntf.s          s_meta,
    metaIntf.m          m_meta
);

logic val_rd;
logic rdy_rd;

fifo #(
    .DATA_BITS($bits(s_meta.data)),
    .FIFO_SIZE(QDEPTH)
) inst_fifo (
    .aclk       (aclk),
    .aresetn    (aresetn),
    .rd         (val_rd),
    .wr         (s_meta.valid),
    .ready_rd   (rdy_rd),
    .ready_wr   (s_meta.ready),
    .data_in    (s_meta.data),
    .data_out   (m_meta.data)
);

assign m_meta.valid = rdy_rd;
assign val_rd = m_meta.valid & m_meta.ready;

endmodule