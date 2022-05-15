open_project toe_prj

set_top toe_top

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
#set_part {xc7vx690tffg1761-2}
set_part {xcvu9p-flga2104-2L-e}
create_clock -period 3.2 -name default

# config_rtl  -disable_start_propagation
csynth_design
export_design -format ip_catalog -display_name "10G TCP Offload Engine" -description "TCP Offload Engine supporting 10Gbps line rate, up to 10K concurrent sessions." -vendor "ethz.systems" -version "1.6"
exit
