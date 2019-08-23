open_project toe_prj

set_top toe

add_files ../axi_utils.cpp
add_files ack_delay/ack_delay.cpp
add_files close_timer/close_timer.cpp
add_files event_engine/event_engine.cpp
add_files port_table/port_table.cpp
add_files probe_timer/probe_timer.cpp
add_files retransmit_timer/retransmit_timer.cpp
add_files rx_app_if/rx_app_if.cpp
add_files rx_app_stream_if/rx_app_stream_if.cpp
add_files rx_engine/rx_engine.cpp
add_files rx_sar_table/rx_sar_table.cpp
add_files session_lookup_controller/session_lookup_controller.cpp
#add_files session_lookup_controller/session_lookup_controller/stub_session_lookup.cpp
add_files state_table/state_table.cpp
add_files tx_app_if/tx_app_if.cpp
add_files tx_app_stream_if/tx_app_stream_if.cpp
add_files tx_engine/tx_engine.cpp
add_files tx_sar_table/tx_sar_table.cpp
add_files tx_app_interface/tx_app_interface.cpp
add_files dummy_memory.cpp
add_files toe.cpp
add_files -tb toe_tb.cpp

open_solution "solution1"
set_part {xc7vx690tffg1761-2}
create_clock -period 6.66 -name default

#csim_design -clean -argv {../../../../testVectors/io_finwp_5.dat ../../../../testVectors/rxOutput.dat ../../../../testVectors/txOutput.dat ../../../../testVectors/rx_io_finwp_5.gold}
csim_design -clean -argv {0 ../../../../testVectors/io_fin_5.dat ../../../../testVectors/rxOutput.dat ../../../../testVectors/txOutput.dat ../../../../testVectors/rx_io_fin_5.gold}
#csim_design -clean -argv {0 ../../../../testVectors/mysyn.dat ../../../../testVectors/rxOutput.dat ../../../../testVectors/txOutput.dat ../../../../testVectors/rx_io_fin_5.gold}
#csim_design -clean -setup
#csynth_design
#cosim_design -tool modelsim -rtl verilog -trace_level all -argv {../../../../testVectors/in9.dat ../../../../testVectors/rxOutput.dat ../../../../testVectors/txOutput.dat}
#export_design -format ip_catalog -display_name "10G TCP Offload Engine" -description "TCP Offload Engine supporting 10Gbps line rate, up to 10K concurrent sessions & Out-Of-Order segments." -vendor "xilinx.labs" -version "2.06"
exit
