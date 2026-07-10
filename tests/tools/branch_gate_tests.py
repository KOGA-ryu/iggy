#!/usr/bin/env python3
"""Synthetic oracle for the branch relocation gate."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import sys


CHECKER = Path(__file__).parents[2] / "tools" / "check_branch_gate.py"
SPEC = importlib.util.spec_from_file_location("check_branch_gate", CHECKER)
assert SPEC is not None and SPEC.loader is not None
CHECKER_MODULE = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = CHECKER_MODULE
SPEC.loader.exec_module(CHECKER_MODULE)


def findings(diff: str, approvals: set[str] | None = None):
    return CHECKER_MODULE.scan_diff(diff, approvals or set(), False)


def same_file_diff(old: str, new: str) -> str:
    return f"""diff --git a/src/app/example.cpp b/src/app/example.cpp
--- a/src/app/example.cpp
+++ b/src/app/example.cpp
@@ -1 +1 @@
-{old}
+{new}
"""


def cross_file_diff(source_line: str | None, destination: str,
                    marker: str, destination_lines: list[str]) -> str:
    source = ""
    if source_line is not None:
        source = f"""diff --git a/src/app/source.cpp b/src/app/source.cpp
--- a/src/app/source.cpp
+++ b/src/app/source.cpp
@@ -1 +1 @@
-{source_line}
+void unchanged() {{}}
"""
    additions = "\n".join(f"+{line}" for line in destination_lines)
    return source + f"""diff --git a/{destination} b/{destination}
--- a/{destination}
+++ b/{destination}
@@ -1,0 +1,{len(destination_lines)} @@
{additions}
"""


def main() -> int:
    assert not findings(same_file_diff("if (value) {", "if (value) {"))
    assert findings(same_file_diff("if (value) {", "if (other) {"))

    approved = same_file_diff("if (value) {", "// branch-gate: BG-1000\nif (new) {")
    assert not findings(approved, {"BG-1000"})

    marked = cross_file_diff(
        "if (value) {",
        "src/app/moved.cpp",
        "// branch-gate-relocation: BG-1001 from=src/app/source.cpp",
        ["// branch-gate-relocation: BG-1001 from=src/app/source.cpp",
         "if (value) {"])
    assert not findings(marked, {"BG-1001"})

    changed = cross_file_diff(
        "if (value) {",
        "src/app/moved.cpp",
        "// branch-gate-relocation: BG-1002 from=src/app/source.cpp",
        ["// branch-gate-relocation: BG-1002 from=src/app/source.cpp",
         "if (other) {"])
    assert findings(changed, {"BG-1002"})

    missing_ledger = cross_file_diff(
        "if (value) {",
        "src/app/moved.cpp",
        "// branch-gate-relocation: BG-1003 from=src/app/source.cpp",
        ["// branch-gate-relocation: BG-1003 from=src/app/source.cpp",
         "if (value) {"])
    assert findings(missing_ledger)

    missing_source = cross_file_diff(
        None,
        "src/app/moved.cpp",
        "// branch-gate-relocation: BG-1004 from=src/app/source.cpp",
        ["// branch-gate-relocation: BG-1004 from=src/app/source.cpp",
         "if (value) {"])
    assert findings(missing_source, {"BG-1004"})

    duplicate = cross_file_diff(
        "if (value) {",
        "src/app/moved.cpp",
        "// branch-gate-relocation: BG-1005 from=src/app/source.cpp",
        ["// branch-gate-relocation: BG-1005 from=src/app/source.cpp",
         "if (value) {", "if (value) {"])
    assert len(findings(duplicate, {"BG-1005"})) == 1

    print("branch gate synthetic cases: 8/8 passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
