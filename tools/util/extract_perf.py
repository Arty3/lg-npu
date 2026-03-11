#!/usr/bin/env python3

"""
extract_perf.py - Parse simulation logs for performance counter values.

Usage:
    python3 tools/util/extract_perf.py <sim-log> [--csv]

Searches for lines matching the pattern:
    PERF: <name> = <value>
and prints a summary table (or CSV with --csv).
"""

from pathlib import Path

import argparse
import sys
import re

PERF_RE = re.compile(r"PERF:\s+(\S+)\s*=\s*(\d+)")


def extract(log_path: Path) -> list[tuple[str, int]]:
    results: list[tuple[str, int]] = []
    with open(log_path) as fh:
        for line in fh:
            m = PERF_RE.search(line)
            if m:
                results.append((m.group(1), int(m.group(2))))
    return results


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract perf counters from sim log")
    parser.add_argument("log", type=Path, help="Simulation log file")
    parser.add_argument("--csv", action="store_true", help="Output CSV format")
    args = parser.parse_args()

    if not args.log.exists():
        print(f"ERROR: file not found: {args.log}", file=sys.stderr)
        return 1

    results = extract(args.log)
    if not results:
        print("No performance counter data found.", file=sys.stderr)
        return 0

    if args.csv:
        print("counter,value")
        for name, val in results:
            print(f"{name},{val}")
    else:
        max_name = max(len(n) for n, _ in results)
        for name, val in results:
            print(f"  {name:<{max_name}}  {val:>12}")

    return 0

if __name__ == "__main__":
    raise SystemExit(main())
