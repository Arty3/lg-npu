#!/usr/bin/env python3

"""
Use sv2v + Yosys + netlistsvg to generate a top-level block diagram of npu_shell.

Produces a single SVG via netlistsvg (digital-logic style), showing only
the first level of hierarchy (sub-module boxes and their inter-connections)
without descending into child module internals.

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


def collect_files() -> list[str]:
    """Parse rtl.f and return absolute paths to .sv files."""
    files: list[str] = []
    for line in RTL_F.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("//") or line.startswith("-"):
            continue
        files.append(str(REPO_ROOT / line))
    return files


def sv2v_convert(files: list[str]) -> str:
    """Run sv2v to convert SystemVerilog to plain Verilog. Returns temp path."""
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


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate top-level npu_shell diagram via Yosys + netlistsvg")
    parser.add_argument("--out-dir", type=pathlib.Path, default=REPO_ROOT / "docs" / "diagrams")
    parser.add_argument("--top", default="npu_shell")
    args = parser.parse_args()

    if not shutil.which("netlistsvg"):
        print("ERROR: netlistsvg not found (install: npm install -g netlistsvg)", file=sys.stderr)
        sys.exit(1)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    json_path = args.out_dir / "top_view.json"
    svg_path = args.out_dir / "top_view.svg"

    files = collect_files()
    verilog = sv2v_convert(files)

    with tempfile.NamedTemporaryFile(mode="w", suffix=".v", delete=False) as tmp:
        tmp.write(verilog)
        tmp_path = tmp.name

    yosys_cmds = (
        f"read_verilog {tmp_path}; "
        f"hierarchy -top {args.top}; "
        f"proc; opt_clean; "
        f"write_json {json_path}"
    )

    print(f"Running Yosys to elaborate {args.top} ...")
    result = subprocess.run(
        ["yosys", "-p", yosys_cmds],
        capture_output=True,
        text=True,
    )
    pathlib.Path(tmp_path).unlink(missing_ok=True)

    if result.returncode != 0:
        print("Yosys failed:", file=sys.stderr)
        print(result.stderr[-1000:] if len(result.stderr) > 1000 else result.stderr, file=sys.stderr)
        sys.exit(1)

    print(f"Running netlistsvg to generate {svg_path} ...")
    result = subprocess.run(
        ["netlistsvg", str(json_path), "-o", str(svg_path)],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print("netlistsvg failed:", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        sys.exit(1)

    print(f"Generated {svg_path}")


if __name__ == "__main__":
    main()
