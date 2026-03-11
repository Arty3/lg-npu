#!/usr/bin/env python3

"""
Generate exp LUT values for softmax_exp_lut.sv
"""

import math

print("// exp_int table: round(65535 * exp(-k)) for k=0..7")
for k in range(8):
    v = round(65535 * math.exp(-k))
    print(f"            3'd{k}: exp_int = 16'd{v};")

print()
print("// exp_frac table: round(65535 * exp(-f/32)) for f=0..31")
for f in range(32):
    v = round(65535 * math.exp(-f / 32.0))
    print(f"            5'd{f:>2}: exp_frac = 16'd{v};")
