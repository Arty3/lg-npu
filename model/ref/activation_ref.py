# activation_ref.py - Reference activation functions for NPU verification

LEAKY_SHIFT = 3  # alpha = 1/2^3 = 0.125

ACT_RELU = 0
ACT_NONE = 1
ACT_LEAKY_RELU = 2


def relu(x: int) -> int:
    """ReLU on a signed INT32 value."""
    return max(0, x)


def leaky_relu(x: int) -> int:
    """Leaky ReLU with fixed negative slope 1/2^LEAKY_SHIFT."""
    return x >> LEAKY_SHIFT if x < 0 else x


def activate(x: int, mode: int = ACT_RELU) -> int:
    """Apply the selected activation function."""
    if mode == ACT_RELU:
        return relu(x)
    if mode == ACT_LEAKY_RELU:
        return leaky_relu(x)
    return x  # ACT_NONE
