open_project rxMemWriter_proj

set_top rxMemWriter

add_files rx_engine.cpp
#add_files -tb test_rx_engine.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

#csim_design -setup -clean
csynth_design
export_design -evaluate verilog
exit
