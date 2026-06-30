#!/usr/bin/env python3
"""Report iggy3d product frame metrics JSON as Markdown or CSV.

The tool is read-only. It accepts the stable JSON packets emitted by
iggy3d_product_frame_metrics, normalizes suite and single-scenario outputs into
rows, and can compare a candidate packet against a baseline packet by
scenario/debug_overlay key. Positive deltas are reported as facts, not failures.
"""

from __future__ import annotations

import argparse
import csv
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, TextIO


ROOT_SCHEMA = "iggy3d.product_frame_metrics.v1"
SCENARIO_SCHEMA = "iggy3d.product_frame_metrics.scenario.v1"

SUMMARY_COLUMNS = (
    "scenario",
    "debug_overlay",
    "frames",
    "total_ns",
    "projection_or_frame_build_ns",
    "gameplay_update_ns",
    "input_or_automation_ns",
    "receipt_or_metrics_build_ns",
    "draw_item_count_max",
    "primitive_count_max",
    "triangle_count_max",
    "collision_surface_count_max",
    "physics_candidate_pair_count_max",
    "physics_sweep_count_max",
    "physics_contact_count_max",
    "wall_run_active_frame_count",
    "debug_hud_line_count_max",
    "movement_state_last",
    "status",
)

COMPARE_COLUMNS = (
    "scenario",
    "debug_overlay",
    "total_ns_delta",
    "projection_or_frame_build_ns_delta",
    "gameplay_update_ns_delta",
    "input_or_automation_ns_delta",
    "receipt_or_metrics_build_ns_delta",
    "draw_item_count_max_delta",
    "primitive_count_max_delta",
    "triangle_count_max_delta",
    "collision_surface_count_max_delta",
    "physics_candidate_pair_count_max_delta",
    "physics_sweep_count_max_delta",
    "physics_contact_count_max_delta",
    "wall_run_active_frame_count_delta",
    "debug_hud_line_count_max_delta",
    "status_change",
)


@dataclass(frozen=True)
class ProductFrameMetricRow:
  scenario: str
  debug_overlay: bool
  frames: int
  total_ns: int
  projection_or_frame_build_ns: int
  gameplay_update_ns: int
  input_or_automation_ns: int
  receipt_or_metrics_build_ns: int
  draw_item_count_max: int
  primitive_count_max: int
  triangle_count_max: int
  collision_surface_count_max: int
  physics_candidate_pair_count_max: int
  physics_sweep_count_max: int
  physics_contact_count_max: int
  wall_run_active_frame_count: int
  debug_hud_line_count_max: int
  movement_state_last: str
  status: str


@dataclass(frozen=True)
class ProductFrameMetricRows:
  schema: str
  rows: list[ProductFrameMetricRow]
  scenario_count: int | None = None
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


def bool_value(data: dict[str, object], key: str) -> bool:
  value = data.get(key, False)
  return value if isinstance(value, bool) else False


def string_value(data: dict[str, object], key: str) -> str:
  value = data.get(key, "")
  return value if isinstance(value, str) else ""


def object_value(data: dict[str, object], key: str) -> dict[str, object]:
  value = data.get(key)
  return value if isinstance(value, dict) else {}


def row_from_scenario(scenario: dict[str, object]) -> ProductFrameMetricRow:
  schema = string_value(scenario, "schema")
  if schema != SCENARIO_SCHEMA:
    raise ValueError(f"unknown scenario schema: {schema or '<missing>'}")
  timings = object_value(scenario, "timings_ns")
  counters = object_value(scenario, "counters")
  movement = object_value(scenario, "movement")
  return ProductFrameMetricRow(
      scenario=string_value(scenario, "scenario"),
      debug_overlay=bool_value(scenario, "debug_overlay"),
      frames=int_value(scenario, "frames"),
      total_ns=int_value(timings, "total_ns"),
      projection_or_frame_build_ns=int_value(
          timings,
          "projection_or_frame_build_ns",
      ),
      gameplay_update_ns=int_value(timings, "tick_or_gameplay_update_ns"),
      input_or_automation_ns=int_value(timings, "input_or_automation_ns"),
      receipt_or_metrics_build_ns=int_value(
          timings,
          "receipt_or_metrics_build_ns",
      ),
      draw_item_count_max=int_value(counters, "draw_item_count_max"),
      primitive_count_max=int_value(counters, "primitive_count_max"),
      triangle_count_max=int_value(counters, "triangle_count_max"),
      collision_surface_count_max=int_value(counters, "collision_surface_count"),
      physics_candidate_pair_count_max=int_value(
          counters,
          "physics_candidate_pair_count_max",
      ),
      physics_sweep_count_max=int_value(
          counters,
          "physics_movement_sweep_count_max",
      ),
      physics_contact_count_max=int_value(counters, "physics_contact_count_max"),
      wall_run_active_frame_count=int_value(
          movement,
          "wall_run_active_frame_count",
      ),
      debug_hud_line_count_max=int_value(counters, "debug_hud_line_count_max"),
      movement_state_last=string_value(movement, "movement_state_last"),
      status=string_value(scenario, "status")
      or string_value(scenario, "reason_code"),
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


def rows_from_packet(packet: dict[str, object]) -> ProductFrameMetricRows:
  schema = string_value(packet, "schema")
  if schema == SCENARIO_SCHEMA:
    return ProductFrameMetricRows(schema=schema, rows=[row_from_scenario(packet)])
  if schema != ROOT_SCHEMA:
    raise ValueError(f"unknown schema: {schema or '<missing>'}")
  scenarios = packet.get("scenarios")
  if not isinstance(scenarios, list):
    raise ValueError("metrics json missing scenarios array")
  rows = []
  for scenario in scenarios:
    if not isinstance(scenario, dict):
      raise ValueError("scenario entries must be objects")
    rows.append(row_from_scenario(scenario))
  return ProductFrameMetricRows(
      schema=schema,
      rows=rows,
      scenario_count=int_value(packet, "scenario_count"),
      total_elapsed_ns=int_value(packet, "total_elapsed_ns"),
  )


def row_to_dict(row: ProductFrameMetricRow) -> dict[str, object]:
  return {
      "scenario": row.scenario,
      "debug_overlay": row.debug_overlay,
      "frames": row.frames,
      "total_ns": row.total_ns,
      "projection_or_frame_build_ns": row.projection_or_frame_build_ns,
      "gameplay_update_ns": row.gameplay_update_ns,
      "input_or_automation_ns": row.input_or_automation_ns,
      "receipt_or_metrics_build_ns": row.receipt_or_metrics_build_ns,
      "draw_item_count_max": row.draw_item_count_max,
      "primitive_count_max": row.primitive_count_max,
      "triangle_count_max": row.triangle_count_max,
      "collision_surface_count_max": row.collision_surface_count_max,
      "physics_candidate_pair_count_max": row.physics_candidate_pair_count_max,
      "physics_sweep_count_max": row.physics_sweep_count_max,
      "physics_contact_count_max": row.physics_contact_count_max,
      "wall_run_active_frame_count": row.wall_run_active_frame_count,
      "debug_hud_line_count_max": row.debug_hud_line_count_max,
      "movement_state_last": row.movement_state_last,
      "status": row.status,
  }


def metric(row: ProductFrameMetricRow | None, name: str) -> int:
  if row is None:
    return 0
  return int(getattr(row, name))


def compare_rows(
    candidate_rows: Iterable[ProductFrameMetricRow],
    baseline_rows: Iterable[ProductFrameMetricRow],
) -> list[dict[str, object]]:
  candidates = {
      (row.scenario, row.debug_overlay): row for row in candidate_rows
  }
  baselines = {
      (row.scenario, row.debug_overlay): row for row in baseline_rows
  }
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
        "scenario": key[0],
        "debug_overlay": key[1],
        "total_ns_delta": metric(candidate, "total_ns")
        - metric(baseline, "total_ns"),
        "projection_or_frame_build_ns_delta": metric(
            candidate,
            "projection_or_frame_build_ns",
        ) - metric(baseline, "projection_or_frame_build_ns"),
        "gameplay_update_ns_delta": metric(candidate, "gameplay_update_ns")
        - metric(baseline, "gameplay_update_ns"),
        "input_or_automation_ns_delta": metric(
            candidate,
            "input_or_automation_ns",
        ) - metric(baseline, "input_or_automation_ns"),
        "receipt_or_metrics_build_ns_delta": metric(
            candidate,
            "receipt_or_metrics_build_ns",
        ) - metric(baseline, "receipt_or_metrics_build_ns"),
        "draw_item_count_max_delta": metric(candidate, "draw_item_count_max")
        - metric(baseline, "draw_item_count_max"),
        "primitive_count_max_delta": metric(candidate, "primitive_count_max")
        - metric(baseline, "primitive_count_max"),
        "triangle_count_max_delta": metric(candidate, "triangle_count_max")
        - metric(baseline, "triangle_count_max"),
        "collision_surface_count_max_delta": metric(
            candidate,
            "collision_surface_count_max",
        ) - metric(baseline, "collision_surface_count_max"),
        "physics_candidate_pair_count_max_delta": metric(
            candidate,
            "physics_candidate_pair_count_max",
        ) - metric(baseline, "physics_candidate_pair_count_max"),
        "physics_sweep_count_max_delta": metric(
            candidate,
            "physics_sweep_count_max",
        ) - metric(baseline, "physics_sweep_count_max"),
        "physics_contact_count_max_delta": metric(
            candidate,
            "physics_contact_count_max",
        ) - metric(baseline, "physics_contact_count_max"),
        "wall_run_active_frame_count_delta": metric(
            candidate,
            "wall_run_active_frame_count",
        ) - metric(baseline, "wall_run_active_frame_count"),
        "debug_hud_line_count_max_delta": metric(
            candidate,
            "debug_hud_line_count_max",
        ) - metric(baseline, "debug_hud_line_count_max"),
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
    packet_rows: ProductFrameMetricRows,
    comparison: list[dict[str, object]] | None = None,
) -> None:
  out.write("# Product Frame Metrics Report\n\n")
  out.write(f"- schema: {packet_rows.schema}\n")
  out.write(f"- row count: {len(packet_rows.rows)}\n")
  if packet_rows.scenario_count is not None:
    out.write(f"- scenario count: {packet_rows.scenario_count}\n")
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
    packet_rows: ProductFrameMetricRows,
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
      description="Print compact reports for iggy3d product frame metrics JSON.",
      epilog=(
          "With --baseline and --format csv, output contains comparison "
          "columns only."
      ),
  )
  parser.add_argument("metrics_json", type=Path, help="candidate metrics JSON packet")
  parser.add_argument("--baseline", type=Path, help="baseline metrics JSON packet")
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
    candidate = rows_from_packet(load_packet(args.metrics_json))
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
