# ============================================================================
# yosys_hook.tcl - Optional pre/post-synthesis Yosys commands.
#
# OpenLane2 owns the main synthesis recipe. Add only project-specific
# instructions here (blackboxes, set_attribute keep, hierarchical reads).
# Wire it into config.json via `"SYNTH_READ_BLACKBOX_LIB"` or a custom
# `"SYNTH_PRE_SCRIPT"` once macros land.
# ============================================================================

# Mark SRAM wrappers as blackboxes during synthesis; replaced with real
# macros at floorplan. Uncomment once LEFs are available.
# blackbox sky130_sram_1rw_blackbox
# blackbox sky130_sram_1rw1r_blackbox

# Preserve reset synchroniser flop chain so CTS does not merge the stages.
# setattr -mod -set keep 1 reset_sync

# Report area before optimisation passes for trend tracking.
stat
