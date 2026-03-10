"""INT8 convolution reference model matching RTL behaviour.

Pipeline: conv -> bias_add -> relu -> quantize(>>>shift) -> clamp[-128,127]

Layout
  Activations : NHWC  (stored H*W*C contiguous)
  Weights     : KRSC  (K filters, each R*S*C)
  Output      : NHWC  (OH*OW*K)

Loop nest (matches conv_ctrl.sv):
  oh -> ow -> k -> r -> s -> c   (outer to inner)
"""

from __future__ import annotations

import numpy as np


def conv2d_int8(
    act: np.ndarray,
    weight: np.ndarray,
    bias: np.ndarray | None = None,
    stride_h: int = 1,
    stride_w: int = 1,
    pad_h: int = 0,
    pad_w: int = 0,
    quant_shift: int = 0,
) -> np.ndarray:
    """Compute INT8 2-D convolution matching RTL exactly.

    Parameters
    ----------
    act : int8 array [H, W, C]
    weight : int8 array [K, R, S, C]
    bias : int8 array [K] or None (treated as 0)
    stride_h, stride_w : spatial strides
    pad_h, pad_w : zero-padding (symmetric)
    quant_shift : arithmetic right-shift applied after bias+ReLU

    Returns
    -------
    int8 array [OH, OW, K]
    """
    in_h, in_w, in_c = act.shape
    out_k, filt_r, filt_s, filt_c = weight.shape
    assert filt_c == in_c

    out_h = (in_h + 2 * pad_h - filt_r) // stride_h + 1
    out_w = (in_w + 2 * pad_w - filt_s) // stride_w + 1

    out = np.zeros((out_h, out_w, out_k), dtype=np.int8)

    for oh in range(out_h):
        for ow in range(out_w):
            for k in range(out_k):
                acc = np.int32(0)
                for r in range(filt_r):
                    for s in range(filt_s):
                        for c in range(in_c):
                            ih = oh * stride_h + r - pad_h
                            iw = ow * stride_w + s - pad_w
                            if ih < 0 or iw < 0 or ih >= in_h or iw >= in_w:
                                a = np.int8(0)
                            else:
                                a = np.int8(act[ih, iw, c])
                            w = np.int8(weight[k, r, s, c])
                            acc += np.int32(a) * np.int32(w)

                # Bias (sign-extended INT8 -> INT32)
                if bias is not None:
                    acc += np.int32(bias[k])

                # ReLU
                if acc < 0:
                    acc = np.int32(0)

                # Arithmetic right shift
                acc = np.int32(int(acc) >> quant_shift)

                # Clamp to INT8
                acc = max(-128, min(127, int(acc)))
                out[oh, ow, k] = np.int8(acc)

    return out
