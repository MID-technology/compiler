#!/usr/bin/env python3
"""
Run every VALID test through its full pipeline and report.

- lexer/valid    -> lexer_demo must exit cleanly
- parser/valid   -> parser   must exit cleanly
- semantic/valid -> semantic must exit cleanly
- codegen/*      -> olc emits .ll, clang links with runtime.o, binary is executed

For each test prints [OK] or [FAIL] with the captured output when it failed.
"""

import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build" / "codegen"
TESTS = ROOT / "tests"

LEXER  = BUILD / "lexer" / "lexer_demo"
PARSER = BUILD / "parser" / "parser"
SEMA   = BUILD / "semantic" / "semantic"
OLC    = BUILD / "olc"
RUNTIME_OBJ = BUILD / "runtime.o"

ERROR_MARKERS = ("Lexer error", "Parser error", "Semantic Errors", "Error:")


@dataclass
class Result:
    name: str
    stage: str
    ok: bool
    detail: str = ""
    stdout: str = ""


def find_clang() -> str:
    for c in ("/opt/homebrew/opt/llvm/bin/clang", "/usr/local/opt/llvm/bin/clang", "clang"):
        if shutil.which(c) or Path(c).exists():
            return c
    return "clang"


def run(cmd, **kw) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, timeout=30, **kw)


def check_stage(binary: Path, src: Path) -> Result:
    name = str(src.relative_to(ROOT))
    if not binary.exists():
        return Result(name, binary.parent.name, False, f"missing binary {binary}")
    p = run([str(binary), str(src)])
    out = p.stdout + p.stderr
    bad = any(m in out for m in ERROR_MARKERS) or p.returncode != 0
    return Result(name, binary.parent.name, not bad, out if bad else "")


def check_codegen(src: Path, workdir: Path, clang: str) -> Result:
    name = str(src.relative_to(ROOT))
    stem = src.stem
    ll = workdir / f"{stem}.ll"
    binp = workdir / f"{stem}.bin"

    cc = run([str(OLC), str(src), "-emit-llvm", "-o", str(ll)])
    if cc.returncode != 0 or not ll.exists():
        return Result(name, "codegen/compile", False, (cc.stdout + cc.stderr).strip())

    link = run([clang, "-o", str(binp), str(ll), str(RUNTIME_OBJ)])
    if link.returncode != 0 or not binp.exists():
        return Result(name, "codegen/link", False, (link.stdout + link.stderr).strip())

    try:
        exe = subprocess.run([str(binp)], capture_output=True, text=True, timeout=10)
    except subprocess.TimeoutExpired:
        return Result(name, "codegen/run", False, "timeout after 10s")

    if exe.returncode != 0:
        detail = f"exit={exe.returncode}\nstdout:\n{exe.stdout}\nstderr:\n{exe.stderr}"
        return Result(name, "codegen/run", False, detail.strip())

    return Result(name, "codegen/run", True, "", exe.stdout)


def main() -> int:
    for tool in (LEXER, PARSER, SEMA, OLC, RUNTIME_OBJ):
        if not tool.exists():
            print(f"missing build artifact: {tool}", file=sys.stderr)
            return 2

    clang = find_clang()
    results: list[Result] = []

    for src in sorted((TESTS / "lexer" / "valid").glob("*.ol")):
        results.append(check_stage(LEXER, src))
    for src in sorted((TESTS / "parser" / "valid").glob("*.ol")):
        results.append(check_stage(PARSER, src))
    for src in sorted((TESTS / "semantic" / "valid").glob("*.ol")):
        results.append(check_stage(SEMA, src))

    with tempfile.TemporaryDirectory(prefix="olrun_") as td:
        wd = Path(td)
        for src in sorted((TESTS / "codegen").glob("*.ol")):
            results.append(check_codegen(src, wd, clang))

    ok = fail = 0
    for r in results:
        if r.ok:
            ok += 1
            tail = r.stdout.strip().replace("\n", " | ")
            preview = f" -> {tail[:80]}" if tail else ""
            print(f"[OK  ] ({r.stage:<16}) {r.name}{preview}")
        else:
            fail += 1
            print(f"[FAIL] ({r.stage:<16}) {r.name}")
            for line in r.detail.splitlines()[:10]:
                print(f"        {line}")

    print()
    print(f"{ok}/{ok + fail} passed" + (f", {fail} failed" if fail else ""))
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
