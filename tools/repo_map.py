#!/usr/bin/env python3
"""Generate a compact machine-readable map of the iggy3d repository.

The tool is read-only unless --output is supplied. It scans source-like files,
classifies ownership by path, extracts lightweight symbols/includes, maps CMake
target references, and emits JSON or Markdown for planner/reviewer orientation.
It is intentionally static and deterministic; it is not a compiler.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, TextIO


DEFAULT_SCAN_ROOTS = ("apps", "src", "tests", "tools", "cmake")
IGNORED_DIRS = {".cache", ".git", "build", "vendor"}
SOURCE_SUFFIXES = {
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".md",
    ".py",
    ".sh",
    ".toml",
    ".txt",
}

BRANCH_RE = re.compile(
    r"\b(if|else\s+if|elif|switch|case|for|while|catch)\b|\?"
)
CPP_INCLUDE_RE = re.compile(r'^\s*#include\s+[<"]([^>"]+)[>"]')
PY_INCLUDE_RE = re.compile(r"^\s*(?:from\s+([A-Za-z_][A-Za-z0-9_.]*)\s+import|import\s+([A-Za-z_][A-Za-z0-9_.]*))")
CMAKE_INCLUDE_RE = re.compile(r"^\s*include\s*\(\s*([^) \t]+)")
SH_INCLUDE_RE = re.compile(r"^\s*(?:source|\.)\s+([^#\s]+)")

CPP_TYPE_RE = re.compile(
    r"^\s*(?:template\s*<[^>]+>\s*)?(struct|class|enum\s+class|enum)\s+"
    r"([A-Za-z_][A-Za-z0-9_]*)"
)
CPP_FUNCTION_RE = re.compile(
    r"^\s*(?:[A-Za-z_][A-Za-z0-9_:<>,~&*\s]*\s+)"
    r"([A-Za-z_][A-Za-z0-9_:]*)\s*\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?[\{;]"
)
PY_SYMBOL_RE = re.compile(r"^\s*(class|def)\s+([A-Za-z_][A-Za-z0-9_]*)")
SH_FUNCTION_RE = re.compile(
    r"^\s*(?:function\s+)?([A-Za-z_][A-Za-z0-9_-]*)\s*(?:\(\))?\s*\{"
)
CMAKE_FUNCTION_RE = re.compile(r"^\s*(function|macro)\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)")
MD_HEADING_RE = re.compile(r"^\s{0,3}#{1,3}\s+(.+)")

CMAKE_CALL_NAMES = (
    "add_executable",
    "add_library",
    "add_test",
    "target_sources",
    "iggy3d_add_acceptance_test",
    "iggy3d_add_product_app_automation_smoke",
    "iggy3d_add_render_packet4_unit_test",
    "iggy3d_add_render_packet6_unit_test",
    "iggy3d_add_unit_test",
)
CMAKE_SOURCE_RE = re.compile(
    r"[\w./${}:<>+-]+\.(?:cmake|cpp|cxx|hpp|hxx|toml|txt|cc|hh|md|py|sh|c|h)"
)
TOKEN_RE = re.compile(r'"([^"]+)"|([A-Za-z0-9_.$\{\}<>\-:/]+)')


@dataclass(frozen=True)
class LayerRule:
  prefix: str
  layer: str


@dataclass(frozen=True)
class DomainRule:
  token: str
  domain: str


@dataclass
class FileRow:
  path: str
  layer: str
  domain: str
  purpose: str
  suffix: str
  nonblank_loc: int
  branch_hits: int
  include_count: int
  includes: list[str]
  symbols: list[str]
  test_targets: list[str] = field(default_factory=list)
  changed: bool = False


@dataclass
class TargetRow:
  name: str
  kind: str
  source_refs: list[str]
  domains: list[str]


LAYER_RULES = (
    LayerRule("src/app/", "app"),
    LayerRule("src/runtime/", "runtime"),
    LayerRule("src/projection/", "projection"),
    LayerRule("src/render/", "render"),
    LayerRule("src/content/", "content"),
    LayerRule("tests/", "tests"),
    LayerRule("tools/", "tools"),
    LayerRule("cmake/", "cmake"),
    LayerRule("apps/", "apps"),
    LayerRule("docs/", "docs"),
)

DOMAIN_RULES = (
    DomainRule("/physics/", "physics"),
    DomainRule("physics_", "physics"),
    DomainRule("Physics", "physics"),
    DomainRule("/player/", "player"),
    DomainRule("Player", "player"),
    DomainRule("/movement/", "movement"),
    DomainRule("Movement", "movement"),
    DomainRule("/session/", "session"),
    DomainRule("Session", "session"),
    DomainRule("/ai/", "ai"),
    DomainRule("Npc", "ai"),
    DomainRule("Behavior", "ai"),
    DomainRule("/collision/", "collision"),
    DomainRule("Collision", "collision"),
    DomainRule("/save/", "save"),
    DomainRule("Save", "save"),
    DomainRule("/frontend/", "frontend"),
    DomainRule("/menu/", "frontend"),
    DomainRule("Frontend", "frontend"),
    DomainRule("Menu", "frontend"),
    DomainRule("/gameplay/", "gameplay"),
    DomainRule("Gameplay", "gameplay"),
    DomainRule("/room_editor/", "room_editor"),
    DomainRule("RoomEditor", "room_editor"),
    DomainRule("/ascii_room/", "ascii_room"),
    DomainRule("AsciiRoom", "ascii_room"),
    DomainRule("/room/", "room"),
    DomainRule("Room", "room"),
    DomainRule("/vulkan/", "vulkan"),
    DomainRule("Vulkan", "vulkan"),
    DomainRule("/debug/", "debug"),
    DomainRule("Debug", "debug"),
    DomainRule("/automation/", "automation"),
    DomainRule("Automation", "automation"),
    DomainRule("/package", "package"),
    DomainRule("Package", "package"),
    DomainRule("/object/", "object"),
    DomainRule("Object", "object"),
    DomainRule("/tools", "tools"),
)


def read_text(path: Path) -> str:
  try:
    return path.read_text(encoding="utf-8")
  except UnicodeDecodeError:
    return path.read_text(encoding="utf-8", errors="replace")


def normalize_rel(path: Path, root: Path) -> str:
  return path.resolve().relative_to(root).as_posix()


def output_path(path: str) -> str:
  return path.replace("\\", "/")


def is_source_like(path: Path) -> bool:
  return path.name == "CMakeLists.txt" or path.suffix in SOURCE_SUFFIXES


def source_files(root: Path, scan_roots: Iterable[str]) -> list[Path]:
  files: list[Path] = []
  seen: set[Path] = set()
  for scan_root in scan_roots:
    base = (root / scan_root).resolve()
    if not base.exists():
      continue
    if base.is_file():
      if is_source_like(base) and base not in seen:
        files.append(base)
        seen.add(base)
      continue
    for dirpath, dirnames, filenames in os.walk(base):
      dirnames[:] = sorted(name for name in dirnames if name not in IGNORED_DIRS)
      for filename in sorted(filenames):
        path = Path(dirpath) / filename
        if is_source_like(path) and path not in seen:
          files.append(path)
          seen.add(path)
  root_cmake = root / "CMakeLists.txt"
  if root_cmake.exists() and root_cmake not in seen:
    files.append(root_cmake)
  return sorted(files, key=lambda item: normalize_rel(item, root))


def classify_layer(rel_path: str) -> str:
  for rule in LAYER_RULES:
    if rel_path.startswith(rule.prefix):
      return rule.layer
  if rel_path == "CMakeLists.txt":
    return "cmake"
  return "unknown"


def classify_domain(rel_path: str) -> str:
  haystack = f"/{rel_path} {Path(rel_path).stem}"
  for rule in DOMAIN_RULES:
    if rule.token in haystack:
      return rule.domain
  layer = classify_layer(rel_path)
  if layer in {"tests", "tools"}:
    return layer
  return "unknown"


def strip_comment_for_branches(line: str, suffix: str) -> str:
  if suffix in {".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx", ".c", ".h", ".py", ".sh"}:
    return line.split("//", 1)[0].split("#", 1)[0]
  return line


def extract_includes(lines: list[str], suffix: str) -> list[str]:
  includes: list[str] = []
  for line in lines:
    match = CPP_INCLUDE_RE.match(line)
    if match:
      includes.append(match.group(1))
      continue
    if suffix == ".py":
      py_match = PY_INCLUDE_RE.match(line)
      if py_match:
        includes.append(py_match.group(1) or py_match.group(2))
        continue
    if suffix in {".cmake", ".txt"}:
      cmake_match = CMAKE_INCLUDE_RE.match(line)
      if cmake_match:
        includes.append(cmake_match.group(1).strip('"'))
        continue
    if suffix == ".sh":
      shell_match = SH_INCLUDE_RE.match(line)
      if shell_match:
        includes.append(shell_match.group(1).strip('"'))
  return sorted(dict.fromkeys(includes))


def extract_symbols(lines: list[str], suffix: str) -> list[str]:
  symbols: list[str] = []
  for line in lines:
    if suffix in {".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx", ".c", ".h"}:
      type_match = CPP_TYPE_RE.match(line)
      if type_match:
        symbols.append(type_match.group(2))
        continue
      function_match = CPP_FUNCTION_RE.match(line)
      if function_match:
        symbols.append(function_match.group(1).split("::")[-1])
        continue
    if suffix == ".py":
      py_match = PY_SYMBOL_RE.match(line)
      if py_match:
        symbols.append(py_match.group(2))
        continue
    if suffix == ".sh":
      shell_match = SH_FUNCTION_RE.match(line)
      if shell_match:
        symbols.append(shell_match.group(1))
        continue
    if suffix in {".cmake", ".txt"}:
      cmake_match = CMAKE_FUNCTION_RE.match(line)
      if cmake_match:
        symbols.append(cmake_match.group(2))
        continue
    if suffix == ".md":
      heading_match = MD_HEADING_RE.match(line)
      if heading_match:
        symbols.append(heading_match.group(1).strip())
  return sorted(dict.fromkeys(symbols))


def inspect_file(root: Path, path: Path, changed_paths: set[str]) -> FileRow:
  rel_path = normalize_rel(path, root)
  suffix = ".txt" if path.name == "CMakeLists.txt" else path.suffix
  text = read_text(path)
  lines = text.splitlines()
  nonblank_loc = sum(1 for line in lines if line.strip())
  branch_hits = sum(
      len(BRANCH_RE.findall(strip_comment_for_branches(line, suffix)))
      for line in lines
  )
  includes = extract_includes(lines, suffix)
  symbols = extract_symbols(lines, suffix)
  layer = classify_layer(rel_path)
  domain = classify_domain(rel_path)
  purpose = f"{layer} {domain}"
  return FileRow(
      path=rel_path,
      layer=layer,
      domain=domain,
      purpose=purpose,
      suffix=suffix,
      nonblank_loc=nonblank_loc,
      branch_hits=branch_hits,
      include_count=len(includes),
      includes=includes,
      symbols=symbols,
      changed=rel_path in changed_paths,
  )


def call_blocks(text: str, name: str) -> list[str]:
  blocks: list[str] = []
  pattern = re.compile(rf"\b{name}\s*\(")
  for match in pattern.finditer(text):
    start = match.end() - 1
    depth = 0
    for index in range(start, len(text)):
      character = text[index]
      if character == "(":
        depth += 1
      elif character == ")":
        depth -= 1
        if depth == 0:
          blocks.append(text[start + 1:index])
          break
  return blocks


def cmake_tokens(block: str) -> list[str]:
  tokens: list[str] = []
  for match in TOKEN_RE.finditer(block):
    token = match.group(1) or match.group(2)
    if token:
      tokens.append(token)
  return tokens


def normalize_source_ref(ref: str) -> str:
  return output_path(ref.strip().strip('"'))


def source_refs_from_block(block: str) -> list[str]:
  refs = [normalize_source_ref(match.group(0)) for match in CMAKE_SOURCE_RE.finditer(block)]
  return sorted(dict.fromkeys(ref for ref in refs if not ref.startswith("$<")))


def target_kind(call_name: str) -> str:
  if call_name in {
      "add_test",
      "iggy3d_add_acceptance_test",
      "iggy3d_add_product_app_automation_smoke",
      "iggy3d_add_render_packet4_unit_test",
      "iggy3d_add_render_packet6_unit_test",
      "iggy3d_add_unit_test",
  }:
    return "test"
  if call_name == "add_library":
    return "library"
  if call_name == "target_sources":
    return "sources"
  return "executable"


def target_name_from_call(call_name: str, block: str) -> str | None:
  if call_name == "add_test":
    match = re.search(r'\bNAME\s+"?([^"\s)]+)"?', block)
    name = match.group(1) if match else None
    return name if name and "$" not in name else None
  tokens = cmake_tokens(block)
  if not tokens or "$" in tokens[0]:
    return None
  return tokens[0]


def merge_target(targets: dict[str, TargetRow],
                 name: str,
                 kind: str,
                 source_refs: list[str]) -> None:
  if name not in targets:
    targets[name] = TargetRow(name=name, kind=kind, source_refs=[], domains=[])
  row = targets[name]
  if row.kind != kind and row.kind != "test":
    row.kind = kind
  row.source_refs = sorted(dict.fromkeys(row.source_refs + source_refs))


def cmake_files(root: Path) -> list[Path]:
  files: list[Path] = []
  root_cmake = root / "CMakeLists.txt"
  if root_cmake.exists():
    files.append(root_cmake)
  cmake_root = root / "cmake"
  if cmake_root.exists():
    files.extend(sorted(cmake_root.rglob("*.cmake")))
  return files


def parse_targets(root: Path) -> list[TargetRow]:
  targets: dict[str, TargetRow] = {}
  for path in cmake_files(root):
    text = read_text(path)
    for call_name in CMAKE_CALL_NAMES:
      for block in call_blocks(text, call_name):
        name = target_name_from_call(call_name, block)
        if not name:
          continue
        merge_target(targets, name, target_kind(call_name), source_refs_from_block(block))
  for row in targets.values():
    domains = [classify_domain(ref) for ref in row.source_refs]
    row.domains = sorted(domain for domain in dict.fromkeys(domains) if domain != "unknown")
  return sorted(targets.values(), key=lambda item: item.name)


def camel_to_snake(value: str) -> str:
  first = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", value)
  second = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", first)
  return re.sub(r"[^a-zA-Z0-9]+", "_", second).strip("_").lower()


def attach_test_targets(files: list[FileRow], targets: list[TargetRow]) -> None:
  test_targets = [target for target in targets if target.kind == "test"]
  for row in files:
    stem_key = camel_to_snake(Path(row.path).stem)
    refs = []
    for target in test_targets:
      direct = row.path in target.source_refs
      by_name = stem_key and stem_key in target.name
      by_source = any(stem_key and stem_key in ref for ref in target.source_refs)
      if direct or by_name or by_source:
        refs.append(target.name)
    row.test_targets = sorted(dict.fromkeys(refs))


def changed_paths_for_range(root: Path, diff_range: str | None) -> set[str]:
  if not diff_range:
    return set()
  result = subprocess.run(
      ["git", "-C", str(root), "diff", "--name-only", diff_range],
      check=False,
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
  )
  if result.returncode != 0:
    raise RuntimeError(result.stderr.strip() or f"git diff failed for {diff_range}")
  return {output_path(line.strip()) for line in result.stdout.splitlines() if line.strip()}


def file_to_json(row: FileRow) -> dict[str, object]:
  return {
      "path": row.path,
      "layer": row.layer,
      "domain": row.domain,
      "purpose": row.purpose,
      "suffix": row.suffix,
      "nonblank_loc": row.nonblank_loc,
      "branch_hits": row.branch_hits,
      "include_count": row.include_count,
      "includes": row.includes,
      "symbols": row.symbols,
      "test_targets": row.test_targets,
      "changed": row.changed,
  }


def target_to_json(row: TargetRow) -> dict[str, object]:
  return {
      "name": row.name,
      "kind": row.kind,
      "source_refs": row.source_refs,
      "domains": row.domains,
  }


def build_payload(root: Path,
                  scan_roots: tuple[str, ...],
                  changed_range: str | None) -> dict[str, object]:
  changed_paths = changed_paths_for_range(root, changed_range)
  files = [inspect_file(root, path, changed_paths) for path in source_files(root, scan_roots)]
  targets = parse_targets(root)
  attach_test_targets(files, targets)
  return {
      "schema": "iggy3d.repo_map.v1",
      "generated_by": "tools/repo_map.py",
      "scan_roots": list(scan_roots),
      "summary": {
          "file_count": len(files),
          "nonblank_loc": sum(row.nonblank_loc for row in files),
          "branch_hits": sum(row.branch_hits for row in files),
          "symbol_count": sum(len(row.symbols) for row in files),
          "test_target_count": sum(1 for row in targets if row.kind == "test"),
      },
      "files": [file_to_json(row) for row in files],
      "targets": [target_to_json(row) for row in targets],
  }


def domain_summary(files: list[dict[str, object]]) -> list[tuple[str, int, int, int]]:
  grouped: dict[str, tuple[int, int, int]] = {}
  for row in files:
    domain = str(row["domain"])
    count, loc, branches = grouped.get(domain, (0, 0, 0))
    grouped[domain] = (
        count + 1,
        loc + int(row["nonblank_loc"]),
        branches + int(row["branch_hits"]),
    )
  return [(domain, *values) for domain, values in sorted(grouped.items())]


def write_json(payload: dict[str, object], out: TextIO) -> None:
  json.dump(payload, out, indent=2, sort_keys=False)
  out.write("\n")


def write_markdown(payload: dict[str, object], out: TextIO, top: int) -> None:
  summary = payload["summary"]
  files = list(payload["files"])
  targets = list(payload["targets"])

  print("# Repo Map", file=out)
  print("", file=out)
  print("Generated by `tools/repo_map.py`.", file=out)
  print("", file=out)
  print("## Summary", file=out)
  print("", file=out)
  print(f"- schema: `{payload['schema']}`", file=out)
  print(f"- scan roots: {', '.join(f'`{root}`' for root in payload['scan_roots'])}", file=out)
  print(f"- files: {summary['file_count']}", file=out)
  print(f"- nonblank LOC: {summary['nonblank_loc']}", file=out)
  print(f"- branch hits: {summary['branch_hits']}", file=out)
  print(f"- symbols: {summary['symbol_count']}", file=out)
  print(f"- test targets: {summary['test_target_count']}", file=out)
  print("", file=out)

  print("## Top Branch Pressure", file=out)
  print("", file=out)
  print("| File | Layer | Domain | LOC | Branches | Tests |", file=out)
  print("| --- | --- | --- | ---: | ---: | --- |", file=out)
  for row in sorted(files, key=lambda item: int(item["branch_hits"]), reverse=True)[:top]:
    tests = ", ".join(f"`{name}`" for name in row["test_targets"]) or "-"
    print(
        f"| `{row['path']}` | {row['layer']} | {row['domain']} | "
        f"{row['nonblank_loc']} | {row['branch_hits']} | {tests} |",
        file=out,
    )
  print("", file=out)

  print("## Domain Summary", file=out)
  print("", file=out)
  print("| Domain | Files | LOC | Branches |", file=out)
  print("| --- | ---: | ---: | ---: |", file=out)
  for domain, count, loc, branches in domain_summary(files):
    print(f"| {domain} | {count} | {loc} | {branches} |", file=out)
  print("", file=out)

  changed_rows = [row for row in files if row["changed"]]
  if changed_rows:
    print("## Changed Files", file=out)
    print("", file=out)
    print("| File | Layer | Domain | LOC | Branches | Tests |", file=out)
    print("| --- | --- | --- | ---: | ---: | --- |", file=out)
    for row in changed_rows:
      tests = ", ".join(f"`{name}`" for name in row["test_targets"]) or "-"
      print(
          f"| `{row['path']}` | {row['layer']} | {row['domain']} | "
          f"{row['nonblank_loc']} | {row['branch_hits']} | {tests} |",
          file=out,
      )
    print("", file=out)

  print("## Test Target Map", file=out)
  print("", file=out)
  print("| Target | Kind | Domains | Source refs |", file=out)
  print("| --- | --- | --- | --- |", file=out)
  for row in targets:
    if row["kind"] != "test":
      continue
    domains = ", ".join(f"`{domain}`" for domain in row["domains"]) or "-"
    refs = ", ".join(f"`{ref}`" for ref in row["source_refs"][:3]) or "-"
    if len(row["source_refs"]) > 3:
      refs += f", +{len(row['source_refs']) - 3} more"
    print(f"| `{row['name']}` | {row['kind']} | {domains} | {refs} |", file=out)


def write_payload(payload: dict[str, object],
                  output_format: str,
                  out: TextIO,
                  top: int) -> None:
  if output_format == "json":
    write_json(payload, out)
    return
  write_markdown(payload, out, top)


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--root", default=".", help="repository root")
  parser.add_argument("--format", choices=("json", "markdown"), default="json")
  parser.add_argument("--top", type=int, default=20, help="top branch-pressure rows")
  parser.add_argument("--output", help="write output to this file")
  parser.add_argument("--changed", help="git diff range to mark changed files")
  parser.add_argument("scan_roots", nargs="*", help="optional scan roots")
  args = parser.parse_args()

  root = Path(args.root).resolve()
  scan_roots = tuple(args.scan_roots) if args.scan_roots else DEFAULT_SCAN_ROOTS
  try:
    payload = build_payload(root, scan_roots, args.changed)
  except RuntimeError as exc:
    print(f"repo_map: {exc}", file=sys.stderr)
    return 1

  if args.output:
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8", newline="") as out:
      write_payload(payload, args.format, out, args.top)
    return 0

  write_payload(payload, args.format, sys.stdout, args.top)
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
