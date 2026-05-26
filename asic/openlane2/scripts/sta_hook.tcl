# ============================================================================
# sta_hook.tcl - Custom STA reporting executed after OpenLane2's signoff STA.
#
# OpenLane2 invokes STA at multiple stages; this hook is sourced after
# routing for the worst-corner check. It tightens reporting so the CI log
# carries an unambiguous PASS/FAIL summary.
# ============================================================================

# Configurable thresholds (slack in ns)
set SETUP_SLACK_MIN  0.0
set HOLD_SLACK_MIN   0.0

report_checks -path_delay max -fields {slew cap input nets fanout} -group_count 5
report_checks -path_delay min -fields {slew cap input nets fanout} -group_count 5
report_check_types -max_slew -max_capacitance -max_fanout -violators

set wns_setup [sta::worst_slack -max]
set wns_hold  [sta::worst_slack -min]

puts "STA_HOOK: WNS setup = ${wns_setup} ns"
puts "STA_HOOK: WNS hold  = ${wns_hold} ns"

if {${wns_setup} < ${SETUP_SLACK_MIN}} {
    puts "STA_HOOK: FAIL - setup slack ${wns_setup} below ${SETUP_SLACK_MIN}"
    exit 1
}
if {${wns_hold} < ${HOLD_SLACK_MIN}} {
    puts "STA_HOOK: FAIL - hold slack ${wns_hold} below ${HOLD_SLACK_MIN}"
    exit 1
}
puts "STA_HOOK: PASS"
