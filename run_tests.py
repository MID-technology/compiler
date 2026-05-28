#!/usr/bin/env python3
"""
Test runner for the O-language compiler.

Layout:
  tests/lexer/valid/        .ol files the lexer must accept
  tests/lexer/invalid/      .ol files where lexer must report an error
  tests/parser/valid/       .ol files the parser must accept
  tests/parser/invalid/     .ol files where parser must report an error
  tests/semantic/valid/     .ol files the semantic analyzer must accept
  tests/semantic/invalid/   .ol files where semantic analyzer must report an error
  tests/codegen/            .ol files that must go end-to-end without errors

Usage:
  python3 run_tests.py                       # run everything
  python3 run_tests.py codegen               # run one stage
  python3 run_tests.py semantic/invalid      # run a sub-stage
  python3 run_tests.py codegen/fibonacci.ol  # run one file
  python3 run_tests.py -v codegen            # verbose
  python3 run_tests.py --build-dir build     # override build dir
"""

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DEFAULT_BUILD = ROOT / "build" / "codegen"
TESTS_DIR = ROOT / "tests"


@dataclass
class Stage:
    binary_rel: str          
    expect_error: bool       
    error_marker: str       


def stages(build_dir: Path) -> dict[str, Stage]:
    return {
        "lexer/valid":      Stage("lexer/lexer_demo",   False, ""),
        "lexer/invalid":    Stage("lexer/lexer_demo",   True,  "Lexer error"),
        "parser/valid":     Stage("parser/parser",      False, ""),
        "parser/invalid":   Stage("parser/parser",      True,  "Parser error"),
        "semantic/valid":   Stage("semantic/semantic",  False, ""),
        "semantic/invalid": Stage("semantic/semantic",  True,  "Semantic Errors"),
        "codegen":          Stage("olc",                False, ""),
    }


ERROR_MARKERS_ANY_STAGE = ("Lexer error", "Parser error", "Semantic Errors", "Error:")


def discover(selector: str | None) -> list[tuple[str, Path]]:
    all_stage_keys = [
        "lexer/valid", "lexer/invalid",
        "parser/valid", "parser/invalid",
        "semantic/valid", "semantic/invalid",
        "codegen",
    ]

    if selector is None:
        targets = all_stage_keys
        files: list[tuple[str, Path]] = []
        for key in targets:
            files.extend(_collect(key))
        return files

    selector = selector.strip("/")

    if selector in all_stage_keys:
        return _collect(selector)

    
    matches = [k for k in all_stage_keys if k.split("/")[0] == selector]
    if matches:
        files = []
        for k in matches:
            files.extend(_collect(k))
        return files

    
    candidate = TESTS_DIR / selector
    if candidate.is_file():
        stage_key = _stage_for_path(candidate)
        return [(stage_key, candidate)]

    
    if candidate.is_dir():
        return [(_stage_for_path(p), p) for p in sorted(candidate.rglob("*.ol"))]

    raise SystemExit(f"Unknown selector: {selector!r}")


def _collect(stage_key: str) -> list[tuple[str, Path]]:
    stage_dir = TESTS_DIR / stage_key
    if not stage_dir.is_dir():
        return []
    return [(stage_key, p) for p in sorted(stage_dir.rglob("*.ol"))]


def _stage_for_path(path: Path) -> str:
    rel = path.relative_to(TESTS_DIR).as_posix()
    for key in (
        "lexer/valid", "lexer/invalid",
        "parser/valid", "parser/invalid",
        "semantic/valid", "semantic/invalid",
    ):
        if rel.startswith(key + "/"):
            return key
    return "codegen"


def run_one(file: Path, stage: Stage, build_dir: Path, verbose: bool) -> bool:
    binary = build_dir / stage.binary_rel
    if not binary.exists():
        print(f"  missing binary: {binary}", file=sys.stderr)
        return False

    proc = subprocess.run(
        [str(binary), str(file)],
        capture_output=True,
        text=True,
        timeout=30,
    )
    output = proc.stdout + proc.stderr

    if stage.expect_error:
        ok = stage.error_marker in output
    else:
        ok = not any(marker in output for marker in ERROR_MARKERS_ANY_STAGE)

    status = "OK  " if ok else "FAIL"
    rel = file.relative_to(ROOT)
    print(f"[{status}] {rel}", flush=True)
    if not ok or verbose:
        tail_lines = output.strip().splitlines()[-6:]
        for line in tail_lines:
            print(f"    {line}")

    return ok


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("selector", nargs="?", default=None,
                    help="stage, sub-stage, directory, or single .ol file")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="also print compiler output tail for every test")
    ap.add_argument("--build-dir", type=Path, default=DEFAULT_BUILD,
                    help=f"directory containing built binaries (default: {DEFAULT_BUILD})")
    args = ap.parse_args()

    stage_map = stages(args.build_dir)
    targets = discover(args.selector)
    if not targets:
        print("No test files found.")
        return 1

    passed = failed = 0
    for stage_key, file in targets:
        stage = stage_map[stage_key]
        if run_one(file, stage, args.build_dir, args.verbose):
            passed += 1
        else:
            failed += 1

    total = passed + failed
    print(f"\n{passed}/{total} passed" + (f", {failed} failed" if failed else ""))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
