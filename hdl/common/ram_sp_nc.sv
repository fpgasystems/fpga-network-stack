module ram_sp_nc
  #(
    parameter ADDR_BITS = 10,
    parameter DATA_BITS = 64
  )
  (
    input  logic                          clk,
    input  logic                          a_en,
    input  logic [(DATA_BITS/8)-1:0]      a_we,
    input  logic [ADDR_BITS-1:0]          a_addr,
    input  logic [DATA_BITS-1:0]          a_data_in,
    output logic [DATA_BITS-1:0]          a_data_out
  );

  localparam DEPTH = 2**ADDR_BITS;

  (* ram_style = "block" *) reg [DATA_BITS-1:0] ram[DEPTH];
  reg [DATA_BITS-1:0] a_data_reg;

  always_ff @(posedge clk) begin
    if(a_en) begin
      for (int i = 0; i < (DATA_BITS/8); i++) begin
        if(a_we[i]) begin
          ram[a_addr][(i*8)+:8] <= a_data_in[(i*8)+:8];
        end
      end
      a_data_reg <= ram[a_addr];
    end
  end

  assign a_data_out = a_data_reg;

endmodule // ram_sp_nc