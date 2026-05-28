# ============================================================================
# base.sdc - Top-level timing constraints for npu_shell on sky130A
#
# Strategy:
#   - Single clock domain (clk) at 100 MHz (10 ns period).
#   - Symmetric IO budgets at 20% of the clock period.
#   - rst_async_n is async (driven by an external reset controller) and is
#     synchronised internally by rtl/platform/reset_sync.sv. Cut it from STA.
#   - irq is a level-sensitive output to the host; budget it at 20% as well.
#   - ext_mem_* is the DMA bus to host RAM. We constrain it with the same
#     20% budget; if the host bridge sits in another domain, group it
#     separately once that path is known.
# ============================================================================

set CLK_PORT      clk
set CLK_PERIOD    10.0
set IO_FRAC       0.20
set IO_DELAY      [expr {$CLK_PERIOD * $IO_FRAC}]
set CLK_UNCERT    0.25

create_clock -name clk -period $CLK_PERIOD [get_ports $CLK_PORT]
set_clock_uncertainty $CLK_UNCERT [get_clocks clk]
set_clock_transition  0.15        [get_clocks clk]

# All non-clock primary inputs / outputs share one budget for now.
# OpenSTA does not implement `remove_from_collection`; filter by name instead.
set ALL_INPUTS  [all_inputs -no_clocks]
set ALL_OUTPUTS [all_outputs]

set_input_delay  -clock clk -max $IO_DELAY $ALL_INPUTS
set_input_delay  -clock clk -min 0.0       $ALL_INPUTS
set_output_delay -clock clk -max $IO_DELAY $ALL_OUTPUTS
set_output_delay -clock clk -min 0.0       $ALL_OUTPUTS

# Drive / load model so STA has a realistic IO context.
set_driving_cell -lib_cell sky130_fd_sc_hd__inv_2 -pin Y $ALL_INPUTS
set_load 0.05 $ALL_OUTPUTS

# rst_async_n is async; it is synchronised by reset_sync before use.
# Cut it from setup/hold checks but keep it visible to the tool.
set_false_path -from [get_ports rst_async_n]

# No async clock groups (single clock domain). When DDR / host clock arrives
# in a future revision, add:
#   set_clock_groups -asynchronous -group {clk} -group {host_clk}
