#!/usr/bin/env python3
"""Audit iggy3d product module shape and consolidation candidates.

The script is read-only. It scans source/header files, classifies them into
coarse product domains, counts branch pressure, and emits either Markdown or
CSV so review work can happen from repo artifacts instead of chat.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


SOURCE_SUFFIXES = {".cpp", ".hpp", ".cc", ".hh", ".cxx", ".hxx", ".c", ".h"}
DEFAULT_SCAN_ROOTS = (
    "src/app/iggy3d",
    "src/app/frontend",
    "src/projection",
    "src/render",
    "src/content",
)

BRANCH_RE = re.compile(r"\b(if|else\s+if|switch|case|for|while)\b|\?")
INCLUDE_RE = re.compile(r'^\s*#include\s+[<"]([^>"]+)[>"]')
TYPE_EXPORT_RE = re.compile(
    r"^\s*(struct|class|enum\s+class|enum)\s+([A-Za-z_][A-Za-z0-9_]*)"
)
FUNCTION_EXPORT_RE = re.compile(
    r"^\s*(?:[A-Za-z_][A-Za-z0-9_:<>,&*\s]*\s+)"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*;"
)


@dataclass(frozen=True)
class DomainRule:
  domain: str
  target: str
  keywords: tuple[str, ...]


DOMAIN_RULES = (
    DomainRule("app-shell", "ProductShell", ("AppShell",)),
    DomainRule("automation", "ProductAutomation", ("Automation",)),
    DomainRule("save-flow", "ProductSaveFlow", ("Save", "SavedRoom")),
    DomainRule("world-flow", "ProductWorldFlow", ("World", "DungeonDraft", "BuiltinDungeon")),
    DomainRule("frontend-flow", "ProductFrontendFlow", ("Frontend", "Menu", "Starter", "Pause", "Settings", "DevTools", "SaveBrowser", "Selector", "Confirm")),
    DomainRule("room-authoring", "ProductRoomAuthoring", ("RoomEditor", "RoomEditing", "RoomAuthoring", "EditableRoom")),
    DomainRule("room-pipeline", "ProductRoomPipeline", ("AsciiRoom", "RoomAssetText", "RoomTo", "ActiveRoom")),
    DomainRule("projection-render", "ProductProjection", ("Projection", "PrimitiveDraw", "RenderBridge", "Viewport", "MovementDebugHud", "NpcBehaviorDebugHud")),
    DomainRule("receipt", "ProductReceipt", ("Receipt",)),
    DomainRule("gameplay", "ProductGameplay", ("Gameplay", "Camera", "Scripted")),
    DomainRule("render-backend", "RenderBackend", ("src/render/", "render/vulkan")),
    DomainRule("runtime", "Runtime", ("runtime/",)),
    DomainRule("content", "Content", ("content/", "Package", "Material", "Mesh", "Fixture")),
)

KEEP_NAMES = {
    "AppShell",
    "ProductAppOptions",
    "ProductSaveCatalog",
    "SaveBridge",
    "ProductWorldCreation",
    "ProductFrontendRouter",
    "ReceiptBuilder",
}

MERGE_TARGET_OVERRIDES = {
    "ProductRoomEditorCursor": "ProductRoomAuthoring",
    "ProductRoomEditorOverlay": "ProductRoomAuthoring",
    "ProductRoomEditorActionController": "ProductRoomAuthoring",
    "ProductRoomEditingState": "ProductRoomAuthoring",
    "ProductRoomAuthoringController": "ProductRoomAuthoring",
    "AsciiRoomSource": "ProductRoomPipeline",
    "AsciiRoomGrid": "ProductRoomPipeline",
    "AsciiRoomToAuthoredRoom": "ProductRoomPipeline",
    "AsciiRoomToEditableRoom": "ProductRoomPipeline",
    "AsciiRoomToRoomAsset": "ProductRoomPipeline",
    "AsciiRoomAssetText": "ProductRoomPipeline",
    "ProductAsciiRoomActivation": "ProductRoomPipeline",
    "ProductAsciiRoomAuthoring": "ProductRoomPipeline",
    "ProductAsciiRoomEditing": "ProductRoomPipeline",
    "ProductAsciiRoomPackage": "ProductRoomPipeline",
    "ProductAsciiRoomPreview": "ProductRoomPipeline",
    "ProductActiveRoomState": "ProductRoomPipeline",
    "ProductActiveRoomCollision": "ProductRoomPipeline",
    "ProductPrimitiveDrawList": "ProductProjection",
    "ProductRenderBridge": "ProductProjection",
    "ProductViewportFraming": "ProductProjection",
    "ProductMovementDebugHud": "ProductProjection",
    "ProductNpcBehaviorDebugHud": "ProductProjection",
    "ProductMenuTransitions": "ProductFrontendFlow",
    "FrontendActionExecutor": "ProductFrontendFlow",
    "ProductAutomationCommandRegistry": "ProductAutomation",
    "ProductAppOperations": "SplitByDomain",
}


@dataclass
class ModuleRow:
  path: str
  stem: str
  loc: int
  branch_hits: int
  include_count: int
  export_count: int
  domain: str
  target: str
  recommendation: str
  reason: str


def read_text(path: Path) -> str:
  try:
    return path.read_text(encoding="utf-8")
  except UnicodeDecodeError:
    return path.read_text(encoding="utf-8", errors="replace")


def source_files(root: Path, scan_roots: Iterable[str]) -> list[Path]:
  files: list[Path] = []
  for scan_root in scan_roots:
    base = root / scan_root
    if not base.exists():
      continue
    files.extend(
        path for path in base.rglob("*")
        if path.is_file() and path.suffix in SOURCE_SUFFIXES
    )
  return sorted(files)


def classify(rel_path: str, stem: str) -> tuple[str, str]:
  if stem in MERGE_TARGET_OVERRIDES:
    target = MERGE_TARGET_OVERRIDES[stem]
    for rule in DOMAIN_RULES:
      if rule.target == target:
        return rule.domain, target
    return "product", target

  haystack = f"{rel_path} {stem}"
  for rule in DOMAIN_RULES:
    if any(keyword in haystack for keyword in rule.keywords):
      return rule.domain, rule.target
  return "uncategorized", "Review"


def count_exports(path: Path, lines: list[str]) -> int:
  if path.suffix not in {".hpp", ".h", ".hh", ".hxx"}:
    return 0
  count = 0
  for line in lines:
    stripped = line.strip()
    if not stripped or stripped.startswith("//"):
      continue
    if TYPE_EXPORT_RE.match(line) or FUNCTION_EXPORT_RE.match(line):
      count += 1
  return count


def recommendation(stem: str,
                   loc: int,
                   branch_hits: int,
                   export_count: int,
                   domain: str,
                   target: str) -> tuple[str, str]:
  if stem == "AppShell":
    return (
        "extract",
        "lifecycle shell should delegate product policy to domain modules",
    )
  if stem in KEEP_NAMES:
    return "keep", "stable owner file already has durable domain responsibility"
  if stem == "ProductAppOperations":
    return (
        "split",
        "mixed save/world/session orchestration should move into durable domain modules",
    )
  if stem in MERGE_TARGET_OVERRIDES:
    return "merge", f"one-shot product file should fold into {target}"
  if loc <= 180 and export_count <= 4 and domain.startswith("room"):
    return "merge-review", f"small {domain} file may belong in {target}"
  if loc <= 160 and export_count <= 3 and domain in {
      "frontend-flow",
      "projection-render",
      "automation",
  }:
    return "merge-review", f"small {domain} file may belong in {target}"
  if branch_hits >= 80:
    return "split-or-table", "branch-heavy file needs table/controller extraction"
  return "keep-review", "not an obvious one-shot file from static shape alone"


def inspect_file(root: Path, path: Path) -> ModuleRow:
  rel_path = path.relative_to(root).as_posix()
  text = read_text(path)
  lines = text.splitlines()
  loc = sum(1 for line in lines if line.strip())
  branch_hits = sum(len(BRANCH_RE.findall(line.split("//", 1)[0])) for line in lines)
  include_count = sum(1 for line in lines if INCLUDE_RE.match(line))
  export_count = count_exports(path, lines)
  stem = path.stem
  domain, target = classify(rel_path, stem)
  action, reason = recommendation(stem, loc, branch_hits, export_count, domain, target)
  return ModuleRow(
      path=rel_path,
      stem=stem,
      loc=loc,
      branch_hits=branch_hits,
      include_count=include_count,
      export_count=export_count,
      domain=domain,
      target=target,
      recommendation=action,
      reason=reason,
  )


def rows_by_domain(rows: list[ModuleRow]) -> dict[str, list[ModuleRow]]:
  grouped: dict[str, list[ModuleRow]] = {}
  for row in rows:
    grouped.setdefault(row.domain, []).append(row)
  return dict(sorted(grouped.items()))


def write_csv(rows: list[ModuleRow], out) -> None:
  writer = csv.writer(out)
  writer.writerow([
      "path",
      "stem",
      "loc",
      "branch_hits",
      "include_count",
      "export_count",
      "domain",
      "target",
      "recommendation",
      "reason",
  ])
  for row in rows:
    writer.writerow([
        row.path,
        row.stem,
        row.loc,
        row.branch_hits,
        row.include_count,
        row.export_count,
        row.domain,
        row.target,
        row.recommendation,
        row.reason,
    ])


def write_markdown(rows: list[ModuleRow], out, top: int) -> None:
  grouped = rows_by_domain(rows)
  total_loc = sum(row.loc for row in rows)
  total_branches = sum(row.branch_hits for row in rows)
  merge_count = sum(1 for row in rows if row.recommendation.startswith("merge"))
  extract_count = sum(1 for row in rows if row.recommendation == "extract")
  heavy_count = sum(1 for row in rows if row.recommendation == "split-or-table")

  print("# Product Module Shape Audit", file=out)
  print("", file=out)
  print("Generated by `tools/audit_product_module_shape.py`.", file=out)
  print("", file=out)
  print("## Summary", file=out)
  print("", file=out)
  print(f"- scanned files: {len(rows)}", file=out)
  print(f"- nonblank LOC: {total_loc}", file=out)
  print(f"- branch hits: {total_branches}", file=out)
  print(f"- merge candidates: {merge_count}", file=out)
  print(f"- extraction roots: {extract_count}", file=out)
  print(f"- split/table candidates: {heavy_count}", file=out)
  print("", file=out)

  print("## Top Branch Pressure", file=out)
  print("", file=out)
  print("| File | Domain | LOC | Branch hits | Recommendation | Target |", file=out)
  print("| --- | --- | ---: | ---: | --- | --- |", file=out)
  for row in sorted(rows, key=lambda item: item.branch_hits, reverse=True)[:top]:
    print(
        f"| `{row.path}` | {row.domain} | {row.loc} | {row.branch_hits} | "
        f"{row.recommendation} | {row.target} |",
        file=out,
    )
  print("", file=out)

  print("## Domain Summary", file=out)
  print("", file=out)
  print("| Domain | Files | LOC | Branch hits | Merge/review candidates | Target |", file=out)
  print("| --- | ---: | ---: | ---: | ---: | --- |", file=out)
  for domain, domain_rows in grouped.items():
    merge_like = sum(1 for row in domain_rows
                     if row.recommendation in {"merge", "merge-review"})
    target = domain_rows[0].target if domain_rows else "Review"
    print(
        f"| {domain} | {len(domain_rows)} | {sum(row.loc for row in domain_rows)} | "
        f"{sum(row.branch_hits for row in domain_rows)} | {merge_like} | {target} |",
        file=out,
    )
  print("", file=out)

  print("## Merge Candidates", file=out)
  print("", file=out)
  print("| File | Target | LOC | Exports | Reason |", file=out)
  print("| --- | --- | ---: | ---: | --- |", file=out)
  for row in sorted(rows, key=lambda item: (item.target, item.path)):
    if row.recommendation not in {"merge", "merge-review"}:
      continue
    print(
        f"| `{row.path}` | {row.target} | {row.loc} | {row.export_count} | "
        f"{row.reason} |",
        file=out,
    )
  print("", file=out)

  print("## Full File Rows", file=out)
  print("", file=out)
  print("| File | Domain | Target | LOC | Branches | Includes | Exports | Recommendation |", file=out)
  print("| --- | --- | --- | ---: | ---: | ---: | ---: | --- |", file=out)
  for row in rows:
    print(
        f"| `{row.path}` | {row.domain} | {row.target} | {row.loc} | "
        f"{row.branch_hits} | {row.include_count} | {row.export_count} | "
        f"{row.recommendation} |",
        file=out,
    )


def main() -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--root", default=".", help="repository root")
  parser.add_argument("--format", choices=("markdown", "csv"), default="markdown")
  parser.add_argument("--top", type=int, default=20, help="top branch-pressure rows")
  parser.add_argument("--output", help="write output to this file")
  parser.add_argument("scan_roots", nargs="*", help="optional scan roots")
  args = parser.parse_args()

  root = Path(args.root).resolve()
  scan_roots = tuple(args.scan_roots) if args.scan_roots else DEFAULT_SCAN_ROOTS
  rows = [inspect_file(root, path) for path in source_files(root, scan_roots)]

  if args.output:
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8", newline="") as out:
      if args.format == "csv":
        write_csv(rows, out)
      else:
        write_markdown(rows, out, args.top)
    return 0

  if args.format == "csv":
    write_csv(rows, sys.stdout)
  else:
    write_markdown(rows, sys.stdout, args.top)
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
