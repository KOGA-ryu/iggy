#!/usr/bin/env python3
"""Reject new source branch statements without explicit approval.

Default usage checks unstaged/staged working tree changes:
  tools/check_branch_gate.py

Common review usage checks one committed slice:
  tools/check_branch_gate.py --diff HEAD~1..HEAD
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


BRANCH_RE = re.compile(r"\b(else\s+if|if|switch)\s*\(")
APPROVAL_RE = re.compile(r"\bbranch-gate:\s*(BG-[0-9]{4,})\b")
RELOCATION_RE = re.compile(
    r"\bbranch-gate-relocation:\s*(BG-[0-9]{4,})\s+"
    r"from=([^\s]+)"
)
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
DEFAULT_SOURCE_PREFIXES = ("src/", "apps/")
DEFAULT_IGNORED_PREFIXES = ("build/",)
LEDGER_PATH = Path("docs/branch_gate_approvals.tsv")
APPROVAL_WINDOW_LINES = 3


@dataclass
class Finding:
  path: str
  new_line: int
  statement: str
  reason: str


def run_git(args: list[str]) -> str:
  result = subprocess.run(
      ["git", *args],
      check=False,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
  )
  if result.returncode != 0:
    sys.stderr.write(result.stderr)
    raise SystemExit(result.returncode)
  return result.stdout


def load_approval_ids(path: Path) -> set[str]:
  if not path.exists():
    return set()
  ids: set[str] = set()
  for line_number, raw_line in enumerate(path.read_text().splitlines(), start=1):
    line = raw_line.strip()
    if not line or line.startswith("#"):
      continue
    if line_number == 1 and line.startswith("id\t"):
      continue
    ids.add(line.split("\t", 1)[0])
  return ids


def source_path(path: str, include_tests: bool) -> bool:
  if not any(path.endswith(suffix) for suffix in SOURCE_SUFFIXES):
    return False
  if path.startswith(DEFAULT_IGNORED_PREFIXES):
    return False
  if include_tests and (path.startswith(DEFAULT_SOURCE_PREFIXES) or path.startswith("tests/")):
    return True
  return path.startswith(DEFAULT_SOURCE_PREFIXES)


def added_code(line: str) -> str:
  if not line.startswith("+") or line.startswith("+++"):
    return ""
  code = line[1:].strip()
  if not code or code.startswith("//") or code.startswith("*"):
    return ""
  return code


def has_ternary(code: str) -> bool:
  code_before_comment = code.split("//", 1)[0]
  return "?" in code_before_comment and ":" in code_before_comment


def branch_statement_name(code: str) -> str | None:
  match = BRANCH_RE.search(code)
  if match:
    return match.group(1).replace(" ", "_")
  if has_ternary(code):
    return "ternary"
  return None


def branch_fingerprint(code: str) -> tuple[str, str] | None:
  code_without_comment = code.split("//", 1)[0]
  statement = branch_statement_name(code_without_comment)
  if statement is None:
    return None
  normalized = " ".join(code_without_comment.split())
  return statement, normalized


def diff_text(diff_range: str | None, staged: bool) -> str:
  if diff_range:
    return run_git(["diff", "--unified=0", diff_range])
  if staged:
    return run_git(["diff", "--cached", "--unified=0"])
  return run_git(["diff", "--unified=0"])


def scan_diff(diff: str, approval_ids: set[str], include_tests: bool) -> list[Finding]:
  removed_branches: dict[str, Counter[tuple[str, str]]] = defaultdict(Counter)
  relocation_markers: dict[str, list[tuple[str, str]]] = defaultdict(list)
  additions: list[tuple[str, int, tuple[str, str], list[tuple[str, int]]]] = []
  current_path = ""
  new_line = 0
  recent_approvals: list[tuple[str, int]] = []

  for raw_line in diff.splitlines():
    if raw_line.startswith("diff --git "):
      current_path = ""
      recent_approvals.clear()
      continue
    if raw_line.startswith("+++ b/"):
      current_path = raw_line.removeprefix("+++ b/")
      continue
    if raw_line.startswith("@@"):
      match = re.search(r"\+([0-9]+)(?:,([0-9]+))?", raw_line)
      if match:
        new_line = int(match.group(1)) - 1
      recent_approvals.clear()
      continue

    if raw_line.startswith("+") and not raw_line.startswith("+++"):
      new_line += 1
      approval_match = APPROVAL_RE.search(raw_line[1:])
      if approval_match:
        recent_approvals.append((approval_match.group(1), new_line))
      recent_approvals = [
          approval for approval in recent_approvals
          if 0 <= new_line - approval[1] <= APPROVAL_WINDOW_LINES
      ]

      if not current_path or not source_path(current_path, include_tests):
        continue

      code = raw_line[1:]
      relocation_match = RELOCATION_RE.search(code)
      if relocation_match:
        relocation_markers[current_path].append(
            (relocation_match.group(1), relocation_match.group(2)))

      code = added_code(raw_line)
      if not code:
        continue

      fingerprint = branch_fingerprint(code)
      if fingerprint is not None:
        additions.append((current_path, new_line, fingerprint,
                          list(recent_approvals)))
      continue
    elif raw_line.startswith("-") and not raw_line.startswith("---"):
      if current_path and source_path(current_path, include_tests):
        code = raw_line[1:].strip()
        fingerprint = branch_fingerprint(code)
        if fingerprint is not None:
          removed_branches[current_path][fingerprint] += 1
      continue
    else:
      if raw_line and current_path:
        new_line += 1
      continue

  findings: list[Finding] = []
  for path, line, fingerprint, recent_approvals in additions:
    if removed_branches[path][fingerprint] > 0:
      removed_branches[path][fingerprint] -= 1
      continue

    relocated = False
    for approval_id, source_path_name in relocation_markers[path]:
      if approval_id not in approval_ids:
        continue
      if removed_branches[source_path_name][fingerprint] <= 0:
        continue
      removed_branches[source_path_name][fingerprint] -= 1
      relocated = True
      break
    if relocated:
      continue

    approved_id = next((approval_id for approval_id, _ in reversed(recent_approvals)
                        if approval_id in approval_ids), "")
    if approved_id:
      continue

    if recent_approvals:
      reason = f"approval id not found in {LEDGER_PATH}"
    else:
      reason = "missing branch-gate approval id"
    findings.append(Finding(path, line, fingerprint[0], reason))

  return findings


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--diff", help="git diff range to check, for example HEAD~1..HEAD")
  parser.add_argument("--cached", action="store_true", help="check staged changes")
  parser.add_argument("--include-tests", action="store_true", help="also gate tests/")
  parser.add_argument("--ledger", default=str(LEDGER_PATH), help="approval TSV path")
  args = parser.parse_args()

  ledger = Path(args.ledger)
  findings = scan_diff(diff_text(args.diff, args.cached),
                       load_approval_ids(ledger),
                       args.include_tests)
  if not findings:
    print("branch gate: ok")
    return 0

  print("branch gate: rejected new unapproved source branch statements", file=sys.stderr)
  for finding in findings:
    print(f"{finding.path}:{finding.new_line}: {finding.statement}: {finding.reason}",
          file=sys.stderr)
  print(f"Add an approval to {ledger} and a nearby `// branch-gate: BG-0000` comment, "
        "or replace the branch with a table/controller/result path.",
        file=sys.stderr)
  return 1


if __name__ == "__main__":
  raise SystemExit(main())
