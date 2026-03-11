#!/usr/bin/env python3
"""Use sv2v + Yosys + netlistsvg to generate per-component block diagrams.

Produces one SVG per component via netlistsvg (digital-logic style).

Requires: sv2v, yosys, netlistsvg (npm install -g netlistsvg).
"""

import subprocess
import argparse
import tempfile
import pathlib
import shutil
import sys

REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
RTL_F = REPO_ROOT / "tools" / "lint" / "rtl.f"

# Map of logical name -> top module
COMPONENTS = {
    "control":            "npu_reg_block",
    "cmd_fetch":          "npu_cmd_fetch",
    "cmd_decode":         "npu_cmd_decode",
    "queue":              "npu_queue",
    "irq":                "npu_irq_ctrl",
    "reset":              "npu_reset_ctrl",
    "perf":               "npu_perf_counters",
    "conv_backend":       "conv_backend",
    "conv_ctrl":          "conv_ctrl",
    "conv_pe":            "conv_pe",
    "conv_array":         "conv_array",
    "gemm_backend":       "gemm_backend",
    "gemm_ctrl":          "gemm_ctrl",
    "vec_backend":        "vec_backend",
    "softmax_composite":  "softmax_composite",
    "lnorm_composite":    "lnorm_composite",
    "pool_composite":     "pool_composite",
    "postproc":           "postproc",
    "activation":         "activation",
    "memory":             "npu_mem_top",
    "weight_buf":         "npu_weight_buffer",
    "act_buf":            "npu_act_buffer",
    "buffer_router":      "npu_buffer_router",
    "dma_frontend":       "npu_dma_frontend",
    "core":               "npu_core",
    "scheduler":          "npu_scheduler",
    "dispatch":           "npu_dispatch",
    "completion":         "npu_completion",
    "status":             "npu_status",
}


def collect_files() -> list[str]:
    files: list[str] = []
    for line in RTL_F.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("//") or line.startswith("-"):
            continue
        files.append(str(REPO_ROOT / line))
    return files


def sv2v_convert(files: list[str]) -> str:
    include = REPO_ROOT / "include"
    cmd = [
        "sv2v",
        f"-I{include}",
        f"-I{include / 'pkg'}",
        f"-I{include / 'defines'}",
        *files,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("sv2v failed:", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        sys.exit(1)
    return result.stdout


def run_yosys(top: str, prefix: pathlib.Path, tmp_path: str) -> bool:
    json_path = pathlib.Path(f"{prefix}.json")
    svg_path = pathlib.Path(f"{prefix}.svg")

    yosys_cmds = (
        f"read_verilog {tmp_path}; "
        f"hierarchy -top {top}; "
        f"proc; opt_clean; "
        f"write_json {json_path}"
    )
    result = subprocess.run(
        ["yosys", "-p", yosys_cmds],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(f"  FAILED (yosys): {top}", file=sys.stderr)
        print(result.stderr[-500:] if len(result.stderr) > 500 else result.stderr, file=sys.stderr)
        return False

    res = subprocess.run(
        ["netlistsvg", str(json_path), "-o", str(svg_path)],
        capture_output=True,
        text=True,
    )
    if res.returncode != 0:
        print(f"  FAILED (netlistsvg): {top}", file=sys.stderr)
        return False

    return True


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate per-component SVG diagrams via Yosys + netlistsvg")
    parser.add_argument("--out-dir", type=pathlib.Path,
                        default=REPO_ROOT / "docs" / "diagrams" / "components")
    parser.add_argument("--component", choices=list(COMPONENTS.keys()) + ["all"],
                        default="all", help="Which component to generate (default: all)")
    args = parser.parse_args()

    if not shutil.which("netlistsvg"):
        print("ERROR: netlistsvg not found (install: npm install -g netlistsvg)", file=sys.stderr)
        sys.exit(1)

    args.out_dir.mkdir(parents=True, exist_ok=True)

    files = collect_files()
    print("Running sv2v ...")
    verilog = sv2v_convert(files)

    with tempfile.NamedTemporaryFile(mode="w", suffix=".v", delete=False) as tmp:
        tmp.write(verilog)
        tmp_path = tmp.name

    targets = COMPONENTS if args.component == "all" else {args.component: COMPONENTS[args.component]}

    ok = 0
    fail = 0
    for name, top in targets.items():
        prefix = args.out_dir / name
        print(f"Generating {name} ({top}) ...")
        if run_yosys(top, prefix, tmp_path):
            print(f"  -> {prefix}.svg")
            ok += 1
        else:
            fail += 1

    pathlib.Path(tmp_path).unlink(missing_ok=True)
    print(f"\nDone: {ok} generated, {fail} failed")
    if fail:
        sys.exit(1)


if __name__ == "__main__":
    main()
