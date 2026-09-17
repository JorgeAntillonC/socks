#!/usr/bin/env python3
"""
Socks test harness.

Usage:
    python sockschecker.py record    capture a baseline of every program
    python sockschecker.py verify    re-run and compare against the baseline

Programs are found under ./examples, ./features, ./errors (relative to
this script's directory). Every *.sk file is executed through
../../build/main.exe. Both stdout and stderr, along with the process
exit code, are compared.

The baseline is stored under ./_baseline/, one .bin file per program,
with the source path encoded into the filename.

Timing output emitted by the compiler ("done time:" block) is stripped
before comparison so it never causes false positives.
"""

import argparse
import difflib
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
COMPILER   = (SCRIPT_DIR / ".." / ".." / "build" / "main.exe").resolve()
SUBDIRS    = ["examples", "features", "errors"]
BASELINE   = SCRIPT_DIR / "_baseline"
TIMEOUT    = 30      # seconds per program

TIMING_MARKER = b"done time:"


def find_programs():
    programs = []
    for sub in SUBDIRS:
        d = SCRIPT_DIR / sub
        if not d.is_dir():
            continue
        programs.extend(sorted(d.glob("*.sk")))
    return programs


def run_program(prog):
    rel = prog.relative_to(SCRIPT_DIR)
    try:
        r = subprocess.run(
            [str(COMPILER), str(rel)],
            cwd=str(SCRIPT_DIR),
            stdin=subprocess.DEVNULL,
            capture_output=True,
            timeout=TIMEOUT,
        )
        return prog, r.returncode, r.stdout, r.stderr
    except subprocess.TimeoutExpired as e:
        return prog, -1, e.stdout or b"", (e.stderr or b"") + b"\n<timeout>\n"
    except FileNotFoundError:
        print(f"compiler not found at {COMPILER}", file=sys.stderr)
        sys.exit(2)


def baseline_path(prog):
    rel = prog.relative_to(SCRIPT_DIR)
    flat = str(rel).replace("\\", "_").replace("/", "_")
    return BASELINE / (flat + ".bin")


def strip_timing(blob):
    """Remove the trailing 'done time:' block from a stream."""
    i = blob.find(TIMING_MARKER)
    if i == -1:
        return blob
    return blob[:i].rstrip() + b"\n"


def pack(rc, out, err):
    out = strip_timing(out)
    err = strip_timing(err)
    return b"RC=%d\n---STDOUT---\n" % rc + out + b"\n---STDERR---\n" + err


def show_diff(expected, actual):
    e = expected.decode("utf-8", "replace").splitlines()
    a = actual.decode("utf-8", "replace").splitlines()
    for line in difflib.unified_diff(e, a, "expected", "actual", lineterm=""):
        print("    " + line)


def record(workers):
    if not COMPILER.exists():
        print(f"compiler not found at {COMPILER}", file=sys.stderr)
        sys.exit(2)
    BASELINE.mkdir(exist_ok=True)
    programs = find_programs()
    print(f"recording {len(programs)} program(s) with {workers} workers")

    with ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(run_program, programs))

    # write baselines serially, in program order, so output is stable
    for prog, rc, out, err in results:
        baseline_path(prog).write_bytes(pack(rc, out, err))
        rel = prog.relative_to(SCRIPT_DIR)
        print(f"  {str(rel):<40} rc={rc:<4} out={len(out):>6}b err={len(err):>6}b")


def verify(workers):
    if not COMPILER.exists():
        print(f"compiler not found at {COMPILER}", file=sys.stderr)
        sys.exit(2)
    if not BASELINE.is_dir():
        print("no baseline; run `record` first", file=sys.stderr)
        sys.exit(2)
    programs = find_programs()
    print(f"verifying {len(programs)} program(s) with {workers} workers")

    with ThreadPoolExecutor(max_workers=workers) as pool:
        results = list(pool.map(run_program, programs))

    failures = []
    for prog, rc, out, err in results:
        bp = baseline_path(prog)
        if not bp.exists():
            failures.append((prog, "no baseline", None, None))
            continue
        expected = bp.read_bytes()
        actual = pack(rc, out, err)
        if actual != expected:
            failures.append((prog, "mismatch", expected, actual))

    if not failures:
        print("GREEN")
        return 0

    print("RED")
    for prog, why, exp, act in failures:
        rel = prog.relative_to(SCRIPT_DIR)
        print(f"\n  {rel}: {why}")
        if exp is not None:
            show_diff(exp, act)
    return 1


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("mode", choices=["record", "verify"])
    ap.add_argument("-j", "--jobs", type=int, default=8,
                    help="parallel compiler invocations (default 8)")
    args = ap.parse_args()

    if args.mode == "record":
        record(args.jobs)
    else:
        sys.exit(verify(args.jobs))


if __name__ == "__main__":
    main()