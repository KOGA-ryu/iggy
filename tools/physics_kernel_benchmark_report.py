#!/usr/bin/env python3
"""Report iggy3d physics kernel benchmark JSON as compact Markdown or CSV.

The tool is read-only. It accepts the stable JSON packets emitted by
iggy3d_physics_kernel_bench, normalizes suite/case inputs into rows, and can
compare a candidate packet against a baseline packet by kernel/scenario key.
When --baseline is supplied with --format csv, the CSV output contains only the
comparison columns.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, TextIO


CASE_SCHEMA = "iggy3d.physics_kernel_benchmark.case.v1"
SUITE_SCHEMA = "iggy3d.physics_kernel_benchmark.suite.v1"

SUMMARY_COLUMNS = (
    "kernel",
    "scenario",
    "iterations",
    "elapsed_ns",
    "colliders",
    "candidates",
    "tested",
    "duplicate_rejected",
    "overlaps",
    "contacts",
    "solve_plans",
    "position_corrections",
    "velocity_impulses",
    "friction_impulses",
    "kinematic_iterations",
    "kinematic_hits",
    "max_penetration_m",
    "normal_impulse_total",
    "friction_impulse_total",
    "status",
)

COMPARE_COLUMNS = (
    "kernel",
    "scenario",
    "elapsed_ns_delta",
    "candidates_delta",
    "tested_delta",
    "contacts_delta",
    "solve_plans_delta",
    "kinematic_hits_delta",
    "status_change",
)


@dataclass(frozen=True)
class BenchmarkRow:
  kernel: str
  scenario: str
  iterations: int
  elapsed_ns: int
  colliders: int
  candidates: int
  tested: int
  duplicate_rejected: int
  overlaps: int
  contacts: int
  solve_plans: int
  position_corrections: int
  velocity_impulses: int
  friction_impulses: int
  kinematic_iterations: int
  kinematic_hits: int
  max_penetration_m: float
  normal_impulse_total: float
  friction_impulse_total: float
  status: str


@dataclass(frozen=True)
class PacketRows:
  schema: str
  rows: list[BenchmarkRow]
  failed_case_count: int | None = None
  total_elapsed_ns: int | None = None


def int_value(data: dict[str, object], key: str) -> int:
  value = data.get(key, 0)
  if isinstance(value, bool):
    return int(value)
  if isinstance(value, int):
    return value
  if isinstance(value, float):
    return int(value)
  return 0


def float_value(data: dict[str, object], key: str) -> float:
  value = data.get(key, 0.0)
  if isinstance(value, (int, float)) and not isinstance(value, bool):
    return float(value)
  return 0.0


def string_value(data: dict[str, object], key: str) -> str:
  value = data.get(key, "")
  return value if isinstance(value, str) else ""


def row_from_case(case: dict[str, object]) -> BenchmarkRow:
  counters = case.get("counters")
  if not isinstance(counters, dict):
    counters = {}
  extrema = case.get("extrema")
  if not isinstance(extrema, dict):
    extrema = {}
  return BenchmarkRow(
      kernel=string_value(case, "kernel"),
      scenario=string_value(case, "scenario"),
      iterations=int_value(case, "iterations"),
      elapsed_ns=int_value(case, "elapsed_nanoseconds"),
      colliders=int_value(counters, "collider_count"),
      candidates=int_value(counters, "broadphase_candidate_pair_count"),
      tested=int_value(counters, "broadphase_tested_pair_count"),
      duplicate_rejected=int_value(
          counters,
          "broadphase_duplicate_pair_rejected_count",
      ),
      overlaps=int_value(counters, "broadphase_overlapping_pair_count"),
      contacts=int_value(counters, "contact_count"),
      solve_plans=int_value(counters, "solve_plan_count"),
      position_corrections=int_value(
          counters,
          "position_correction_applied_count",
      ),
      velocity_impulses=int_value(counters, "velocity_impulse_applied_count"),
      friction_impulses=int_value(counters, "friction_impulse_applied_count"),
      kinematic_iterations=int_value(counters, "kinematic_iteration_count"),
      kinematic_hits=int_value(counters, "kinematic_hit_count"),
      max_penetration_m=float_value(extrema, "max_penetration_meters"),
      normal_impulse_total=float_value(extrema, "total_normal_impulse"),
      friction_impulse_total=float_value(extrema, "total_friction_impulse"),
      status=string_value(case, "status") or string_value(case, "reason_code"),
  )


def load_packet(path: Path) -> dict[str, object]:
  try:
    with path.open("r", encoding="utf-8") as handle:
      packet = json.load(handle)
  except json.JSONDecodeError as exc:
    raise ValueError(f"malformed json: {exc.msg}") from exc
  except OSError as exc:
    raise ValueError(f"read failed: {exc}") from exc
  if not isinstance(packet, dict):
    raise ValueError("top-level json value must be an object")
  return packet


def rows_from_packet(packet: dict[str, object]) -> PacketRows:
  schema = string_value(packet, "schema")
  if schema == CASE_SCHEMA:
    return PacketRows(schema=schema, rows=[row_from_case(packet)])
  if schema != SUITE_SCHEMA:
    raise ValueError(f"unknown schema: {schema or '<missing>'}")
  cases = packet.get("cases")
  if not isinstance(cases, list):
    raise ValueError("suite json missing cases array")
  rows = []
  for case in cases:
    if not isinstance(case, dict):
      raise ValueError("suite cases must be objects")
    rows.append(row_from_case(case))
  return PacketRows(
      schema=schema,
      rows=rows,
      failed_case_count=int_value(packet, "failed_case_count"),
      total_elapsed_ns=int_value(packet, "total_elapsed_nanoseconds"),
  )


def row_to_dict(row: BenchmarkRow) -> dict[str, object]:
  return {
      "kernel": row.kernel,
      "scenario": row.scenario,
      "iterations": row.iterations,
      "elapsed_ns": row.elapsed_ns,
      "colliders": row.colliders,
      "candidates": row.candidates,
      "tested": row.tested,
      "duplicate_rejected": row.duplicate_rejected,
      "overlaps": row.overlaps,
      "contacts": row.contacts,
      "solve_plans": row.solve_plans,
      "position_corrections": row.position_corrections,
      "velocity_impulses": row.velocity_impulses,
      "friction_impulses": row.friction_impulses,
      "kinematic_iterations": row.kinematic_iterations,
      "kinematic_hits": row.kinematic_hits,
      "max_penetration_m": row.max_penetration_m,
      "normal_impulse_total": row.normal_impulse_total,
      "friction_impulse_total": row.friction_impulse_total,
      "status": row.status,
  }


def metric(row: BenchmarkRow | None, name: str) -> int:
  if row is None:
    return 0
  value = getattr(row, name)
  return int(value)


def compare_rows(
    candidate_rows: Iterable[BenchmarkRow],
    baseline_rows: Iterable[BenchmarkRow],
) -> list[dict[str, object]]:
  candidates = {(row.kernel, row.scenario): row for row in candidate_rows}
  baselines = {(row.kernel, row.scenario): row for row in baseline_rows}
  rows: list[dict[str, object]] = []
  for key in sorted(set(candidates) | set(baselines)):
    candidate = candidates.get(key)
    baseline = baselines.get(key)
    if baseline is None:
      status_change = "new"
    elif candidate is None:
      status_change = "removed"
    elif candidate.status == baseline.status:
      status_change = "same"
    else:
      status_change = f"{baseline.status}->{candidate.status}"
    rows.append({
        "kernel": key[0],
        "scenario": key[1],
        "elapsed_ns_delta": metric(candidate, "elapsed_ns")
        - metric(baseline, "elapsed_ns"),
        "candidates_delta": metric(candidate, "candidates")
        - metric(baseline, "candidates"),
        "tested_delta": metric(candidate, "tested") - metric(baseline, "tested"),
        "contacts_delta": metric(candidate, "contacts")
        - metric(baseline, "contacts"),
        "solve_plans_delta": metric(candidate, "solve_plans")
        - metric(baseline, "solve_plans"),
        "kinematic_hits_delta": metric(candidate, "kinematic_hits")
        - metric(baseline, "kinematic_hits"),
        "status_change": status_change,
    })
  return rows


def markdown_text(value: object) -> str:
  return str(value).replace("\n", " ").replace("|", "\\|")


def write_markdown_table(
    out: TextIO,
    columns: tuple[str, ...],
    rows: Iterable[dict[str, object]],
) -> None:
  out.write("| " + " | ".join(columns) + " |\n")
  out.write("| " + " | ".join("---" for _ in columns) + " |\n")
  for row in rows:
    out.write(
        "| "
        + " | ".join(markdown_text(row[column]) for column in columns)
        + " |\n"
    )


def write_markdown(
    out: TextIO,
    packet_rows: PacketRows,
    comparison: list[dict[str, object]] | None = None,
) -> None:
  out.write("# Physics Kernel Benchmark Report\n\n")
  out.write(f"- schema: {packet_rows.schema}\n")
  out.write(f"- case count: {len(packet_rows.rows)}\n")
  if packet_rows.failed_case_count is not None:
    out.write(f"- failed case count: {packet_rows.failed_case_count}\n")
  if packet_rows.total_elapsed_ns is not None:
    out.write(f"- total elapsed ns: {packet_rows.total_elapsed_ns}\n")
  out.write("\n")
  write_markdown_table(
      out,
      SUMMARY_COLUMNS,
      [row_to_dict(row) for row in packet_rows.rows],
  )
  if comparison is not None:
    out.write("\n## Comparison\n\n")
    write_markdown_table(out, COMPARE_COLUMNS, comparison)


def write_csv(
    out: TextIO,
    packet_rows: PacketRows,
    comparison: list[dict[str, object]] | None = None,
) -> None:
  writer = csv.DictWriter(
      out,
      fieldnames=COMPARE_COLUMNS if comparison is not None else SUMMARY_COLUMNS,
      lineterminator="\n",
  )
  writer.writeheader()
  if comparison is not None:
    writer.writerows(comparison)
  else:
    writer.writerows(row_to_dict(row) for row in packet_rows.rows)


def parse_args(argv: list[str]) -> argparse.Namespace:
  parser = argparse.ArgumentParser(
      description="Print compact reports for iggy3d physics kernel benchmark JSON.",
      epilog=(
          "With --baseline and --format csv, output contains comparison "
          "columns only."
      ),
  )
  parser.add_argument("benchmark_json", type=Path, help="candidate benchmark JSON packet")
  parser.add_argument("--baseline", type=Path, help="baseline benchmark JSON packet")
  parser.add_argument(
      "--format",
      choices=("markdown", "csv"),
      default="markdown",
      help="output format (default: markdown)",
  )
  return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
  args = parse_args(argv or sys.argv[1:])
  try:
    candidate = rows_from_packet(load_packet(args.benchmark_json))
    comparison = None
    if args.baseline is not None:
      baseline = rows_from_packet(load_packet(args.baseline))
      comparison = compare_rows(candidate.rows, baseline.rows)
  except ValueError as exc:
    print(f"error: {exc}", file=sys.stderr)
    return 1

  if args.format == "markdown":
    write_markdown(sys.stdout, candidate, comparison)
  else:
    write_csv(sys.stdout, candidate, comparison)
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
