#!/usr/bin/env python3
"""Generate INT8 convolution test vectors for the E2E simulation.

Produces files consumed by the C++ Verilator harness:
  <out-dir>/e2e_act.hex        -- input activations  (hex, one byte per line)
  <out-dir>/e2e_weights.hex    -- weight values       (hex, one byte per line)
  <out-dir>/e2e_expected.hex   -- expected outputs    (hex, one byte per line)
  <out-dir>/e2e_config.txt     -- test parameters     (key=value)

Usage:
  python model/vectors/gen_conv_vectors.py --out-dir tb/vectors
"""

from __future__ import annotations

import argparse
import os
import sys

import numpy as np

# Allow running from the repo root
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from ref.conv_ref import conv2d_int8


def _write_hex(path: str, data: np.ndarray) -> None:
    """Write an array of int8 values as hex (one byte per line)."""
    with open(path, "w") as f:
        for val in data.flat:
            f.write(f"{int(val) & 0xFF:02x}\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate conv test vectors")
    parser.add_argument("--out-dir", required=True, help="Output directory")
    args = parser.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)

    # Test parameters (small enough for quick simulation)
    in_h, in_w, in_c = 4, 4, 1
    out_k = 1
    filt_r, filt_s = 3, 3
    stride_h, stride_w = 1, 1
    pad_h, pad_w = 0, 0
    quant_shift = 0

    # Local SRAM addresses used by the command descriptor
    act_in_addr = 0
    act_out_addr = 4096  # second half of activation buffer
    weight_addr = 0
    bias_addr = 0  # unused (bias stubbed in RTL)

    # Build test data
    act = np.arange(1, in_h * in_w * in_c + 1, dtype=np.int8).reshape(
        (in_h, in_w, in_c)
    )
    weight = np.ones((out_k, filt_r, filt_s, in_c), dtype=np.int8)

    # Compute reference output
    expected = conv2d_int8(
        act, weight,
        bias=None,
        stride_h=stride_h, stride_w=stride_w,
        pad_h=pad_h, pad_w=pad_w,
        quant_shift=quant_shift,
    )

    out_h, out_w, _ = expected.shape

    # Write hex files
    _write_hex(os.path.join(args.out_dir, "e2e_act.hex"), act)
    _write_hex(os.path.join(args.out_dir, "e2e_weights.hex"), weight)
    _write_hex(os.path.join(args.out_dir, "e2e_expected.hex"), expected)

    # Write config
    cfg_path = os.path.join(args.out_dir, "e2e_config.txt")
    with open(cfg_path, "w") as f:
        f.write(f"in_h={in_h}\n")
        f.write(f"in_w={in_w}\n")
        f.write(f"in_c={in_c}\n")
        f.write(f"out_k={out_k}\n")
        f.write(f"filt_r={filt_r}\n")
        f.write(f"filt_s={filt_s}\n")
        f.write(f"stride_h={stride_h}\n")
        f.write(f"stride_w={stride_w}\n")
        f.write(f"pad_h={pad_h}\n")
        f.write(f"pad_w={pad_w}\n")
        f.write(f"quant_shift={quant_shift}\n")
        f.write(f"act_in_addr={act_in_addr}\n")
        f.write(f"act_out_addr={act_out_addr}\n")
        f.write(f"weight_addr={weight_addr}\n")
        f.write(f"bias_addr={bias_addr}\n")
        f.write(f"out_h={out_h}\n")
        f.write(f"out_w={out_w}\n")

    # Print summary
    print(f"Activations: {act.flatten().tolist()}")
    print(f"Weights:     {weight.flatten().tolist()}")
    print(f"Expected:    {expected.flatten().tolist()}")
    print(f"Output shape: ({out_h}, {out_w}, {out_k})")
    print(f"Files written to {args.out_dir}/")


if __name__ == "__main__":
    main()
