#!/usr/bin/env python3
"""Build and validate the repository department map.

Human-maintained inputs:
  docs/departments/registry.tsv
  docs/departments/ownership.tsv
  docs/departments/<department>/DEPARTMENT.md
  docs/departments/<department>/TODO.md
  docs/departments/<department>/TESTING.md

Generated outputs:
  docs/departments/INDEX.md
  docs/departments/DEPENDENCIES.md
  docs/departments/TEST_QUEUE.md
  docs/departments/<department>/FILES.md
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
DOC_ROOT = ROOT / "docs" / "departments"
REGISTRY_PATH = DOC_ROOT / "registry.tsv"
OWNERSHIP_PATH = DOC_ROOT / "ownership.tsv"

GOVERNANCE_PATHS = (
    "docs/departments/",
    "tools/repo_departments.py",
)

MATURITY_VALUES = {
    "Missing",
    "Prototype",
    "Stable Recipe",
    "UI Bindable",
    "Integrated",
    "Support",
    "Deferred",
}
DELIVERY_VALUES = {
    "Needs Audit",
    "Planned",
    "In Progress",
    "Automated Green",
    "Manual Test Needed",
    "Accepted",
    "Blocked",
    "Deferred",
}
TEST_STATUS_VALUES = {"Pending", "Passed", "Blocked", "Deferred", "Not Required"}
AUDIT_STATE_VALUES = {"Not Started", "In Progress", "Complete", "Reaudit Needed"}
AUDIT_CLASSIFICATION_VALUES = {
    "Canonical Owner",
    "Required Adapter",
    "Duplicate Implementation",
    "Legacy Reachable",
    "Test-only Production",
    "Unreachable",
    "Ownership Undecided",
    "Contract Risk",
}
AUDIT_DISPOSITION_VALUES = {
    "Keep",
    "Consolidate",
    "Delete Candidate",
    "Investigate",
    "Move",
    "Repair",
}
AUDIT_PRIORITY_VALUES = {"P0", "P1", "P2", "P3"}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


@dataclass(frozen=True)
class Department:
    department_id: str
    title: str
    priority: int
    focus_tier: str
    audit_state: str
    summary: str


@dataclass(frozen=True)
class WorkItem:
    department_id: str
    work_id: str
    capability: str
    maturity: str
    delivery: str
    evidence: str
    next_action: str


@dataclass(frozen=True)
class ManualTest:
    department_id: str
    test_id: str
    work_id: str
    scenario: str
    command: str
    status: str
    last_verified: str


@dataclass(frozen=True)
class AuditItem:
    department_id: str
    audit_id: str
    surface: str
    classification: str
    evidence: str
    disposition: str
    priority: str


def run_git(*args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout


def tracked_files() -> list[str]:
    return sorted(line for line in run_git("ls-files").splitlines() if line)


def governed_files() -> list[str]:
    return [
        path
        for path in tracked_files()
        if not any(path == prefix or path.startswith(prefix) for prefix in GOVERNANCE_PATHS)
    ]


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def write_tsv(path: Path, fieldnames: list[str], rows: Iterable[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(
            handle, fieldnames=fieldnames, delimiter="\t", lineterminator="\n"
        )
        writer.writeheader()
        writer.writerows(rows)


def load_departments() -> list[Department]:
    rows = read_tsv(REGISTRY_PATH)
    return [
        Department(
            department_id=row["department_id"].strip(),
            title=row["title"].strip(),
            priority=int(row["priority"].strip()),
            focus_tier=row["focus_tier"].strip(),
            audit_state=row["audit_state"].strip(),
            summary=row["summary"].strip(),
        )
        for row in rows
    ]


def load_ownership() -> dict[str, str]:
    result: dict[str, str] = {}
    for row in read_tsv(OWNERSHIP_PATH):
        path = row["path"].strip()
        department_id = row["department_id"].strip()
        if path in result:
            raise ValueError(f"duplicate ownership row: {path}")
        result[path] = department_id
    return result


def parse_markdown_table(path: Path, expected_header: list[str]) -> list[list[str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    expected = [cell.strip() for cell in expected_header]
    for index, line in enumerate(lines):
        if not line.startswith("|"):
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if cells != expected:
            continue
        rows: list[list[str]] = []
        for row_line in lines[index + 2 :]:
            if not row_line.startswith("|"):
                break
            row = [cell.strip() for cell in row_line.strip().strip("|").split("|")]
            if len(row) != len(expected):
                raise ValueError(f"{path}: malformed table row: {row_line}")
            rows.append(row)
        return rows
    raise ValueError(f"{path}: missing table with header {' | '.join(expected)}")


def load_work_items(departments: list[Department]) -> list[WorkItem]:
    items: list[WorkItem] = []
    for department in departments:
        path = DOC_ROOT / department.department_id / "TODO.md"
        rows = parse_markdown_table(
            path, ["ID", "Capability", "Maturity", "Delivery", "Evidence", "Next"]
        )
        for row in rows:
            items.append(
                WorkItem(
                    department_id=department.department_id,
                    work_id=row[0],
                    capability=row[1],
                    maturity=row[2],
                    delivery=row[3],
                    evidence=row[4],
                    next_action=row[5],
                )
            )
    return items


def load_manual_tests(departments: list[Department]) -> list[ManualTest]:
    tests: list[ManualTest] = []
    for department in departments:
        path = DOC_ROOT / department.department_id / "TESTING.md"
        rows = parse_markdown_table(
            path,
            ["Test ID", "Work ID", "Scenario", "Command", "Status", "Last Verified"],
        )
        for row in rows:
            tests.append(
                ManualTest(
                    department_id=department.department_id,
                    test_id=row[0],
                    work_id=row[1],
                    scenario=row[2],
                    command=row[3],
                    status=row[4],
                    last_verified=row[5],
                )
            )
    return tests


def load_audit_items(departments: list[Department]) -> list[AuditItem]:
    items: list[AuditItem] = []
    for department in departments:
        path = DOC_ROOT / department.department_id / "AUDIT.md"
        rows = parse_markdown_table(
            path,
            [
                "ID",
                "Surface",
                "Classification",
                "Evidence",
                "Disposition",
                "Priority",
            ],
        )
        for row in rows:
            items.append(
                AuditItem(
                    department_id=department.department_id,
                    audit_id=row[0],
                    surface=row[1],
                    classification=row[2],
                    evidence=row[3],
                    disposition=row[4],
                    priority=row[5],
                )
            )
    return items


def infer_test_department(name: str) -> str:
    lowered = name.lower()
    groups = (
        (
            "building-layout",
            (
                "world_layout",
                "building",
                "room_",
                "opening",
                "wall_",
                "door",
                "window",
                "stair",
                "ramp",
                "roof",
                "architecture",
                "two_storey",
                "map_maker",
            ),
        ),
        (
            "terrain-site",
            (
                "terrain",
                "contour",
                "road",
                "bridge",
                "watercourse",
                "retaining",
                "topography",
            ),
        ),
        (
            "assets-objects",
            (
                "asset",
                "static_mesh",
                "placement",
                "attachment",
                "scatter",
                "authored",
                "volume",
                "voxel",
                "brush",
                "pattern",
                "surface_extrude",
                "connected_fill",
                "ghost",
                "snap_",
                "catalog",
                "object_descriptor",
                "calibration_bay",
                "framing_bay",
            ),
        ),
        (
            "interaction-controls",
            (
                "control",
                "input",
                "action_hint",
                "select",
                "transform",
                "group",
                "clipboard",
                "measure",
                "camera_mode",
                "grid_footprint",
                "creative_interaction",
                "creative_tools",
            ),
        ),
        (
            "editor-shell-ui",
            (
                "desktop",
                "toolbox",
                "tool_glyph",
                "drafting_style",
                "viewport_layout",
                "content_viewport",
                "ui_widget",
            ),
        ),
        (
            "rendering-preview",
            (
                "render",
                "projection",
                "vulkan",
                "swapchain",
                "mesh",
                "frustum",
                "frame_capture",
                "shader",
                "vertex_format",
                "debug_hud",
                "wireframe",
            ),
        ),
        (
            "persistence-validation",
            (
                "save",
                "hash",
                "codec",
                "replay",
                "diagnostic",
                "validation",
                "package_runtime",
            ),
        ),
        (
            "playtest-simulation",
            (
                "play",
                "npc",
                "physics",
                "movement",
                "session",
                "ability",
                "combat",
                "player",
                "guard",
                "reasoning",
                "objective",
                "collision",
                "inventory",
                "target",
                "clamber",
                "reachability",
                "segment_occlusion",
                "entity_hit",
                "world_state",
                "clock",
                "command_admission",
                "interaction_system",
                "logic_link",
                "moving_platform",
                "runtime_sandbox",
                "map_demo",
                "scenario_seed",
            ),
        ),
        (
            "authoring-core",
            (
                "document",
                "mutation",
                "history",
                "recipe",
                "provenance",
                "map_template",
                "world_service",
                "creator_task",
                "creative_core",
                "facade",
                "creative_geometry",
            ),
        ),
    )
    for department_id, needles in groups:
        if any(needle in lowered for needle in needles):
            return department_id
    return "foundation-build"


def infer_creative_app_department(path: str) -> str:
    name = Path(path).stem.lower()
    if name == "main" or name in {"editordraftingstyle", "editortoolglyphs"}:
        return "editor-shell-ui"
    if name == "editorworldtargeting":
        return "interaction-controls"
    if "worldlayout" in name:
        if any(token in name for token in ("terrain", "topography", "contour")):
            return "terrain-site"
        return "building-layout"
    if name.startswith("editorterrain"):
        return "terrain-site"
    if name.startswith("editordesktop"):
        if "worldlayout" in name or "generated" in name:
            return "building-layout"
        if "terrain" in name:
            return "terrain-site"
        if "asset" in name:
            return "assets-objects"
        if "measurement" in name or "transform" in name:
            return "interaction-controls"
        return "editor-shell-ui"
    if any(
        token in name
        for token in (
            "asset",
            "catalog",
            "attachment",
            "authored",
            "placement",
            "roomplacement",
            "structuralplacement",
            "volume",
            "pattern",
            "connectedfill",
            "surfaceextrude",
            "shapepreview",
        )
    ):
        return "assets-objects"
    if any(
        token in name
        for token in (
            "playtest",
            "playerspawn",
            "logiclink",
        )
    ):
        return "playtest-simulation"
    if "mapvalidation" in name:
        return "persistence-validation"
    if any(
        token in name
        for token in (
            "preview",
            "overlay",
            "wireframe",
            "frustum",
            "capture",
            "scenepreview",
        )
    ):
        return "rendering-preview"
    if "persistence" in name:
        return "persistence-validation"
    if any(
        token in name
        for token in (
            "control",
            "gamepad",
            "interaction",
            "picking",
            "pickframe",
            "gizmo",
            "transform",
            "group",
            "objectaction",
            "edits",
            "measurement",
            "tool",
            "actionhint",
            "commandinput",
            "helditem",
            "materialstroke",
            "pathediting",
        )
    ):
        return "interaction-controls"
    if any(token in name for token in ("desktop", "frame", "bootstrap", "state")):
        return "editor-shell-ui"
    return "authoring-core"


def infer_creative_source_department(path: str) -> str:
    relative = path.removeprefix("src/app/iggy3d/creative/")
    area = relative.split("/", 1)[0]
    name = Path(path).stem.lower()
    if area == "document":
        if "terrain" in name:
            return "terrain-site"
        if any(token in name for token in ("voxel", "patternrecipe", "objectdescriptor")):
            return "assets-objects"
        if "measurement" in name or "snap" in name:
            return "interaction-controls"
        if "wireframe" in name:
            return "rendering-preview"
        if "logiclink" in name:
            return "playtest-simulation"
        return "authoring-core"
    if area in {"history", "mutation", "adapters"}:
        return "authoring-core"
    if area == "bridge":
        return "editor-shell-ui"
    if area == "assets":
        return "assets-objects"
    if area in {"camera", "input"}:
        return "interaction-controls"
    if area in {"overlay", "render"}:
        return "rendering-preview"
    if area == "play":
        return "playtest-simulation"
    if area == "ui":
        return "editor-shell-ui"
    if area == "validation":
        return "persistence-validation"
    if area == "spatial":
        return "assets-objects"
    if area == "recipes":
        if any(
            token in name
            for token in (
                "terrain",
                "road",
                "bridge",
                "retaining",
                "watercourse",
            )
        ):
            return "terrain-site"
        if any(
            token in name
            for token in (
                "building",
                "door",
                "window",
                "stair",
                "ramp",
                "roof",
                "wall",
                "structuralsurface",
            )
        ):
            return "building-layout"
        if any(token in name for token in ("object", "pattern")):
            return "assets-objects"
        return "authoring-core"
    if area == "world":
        if "terrain" in name:
            return "terrain-site"
        if name.startswith("mapdemo"):
            return "playtest-simulation"
        if name.startswith("documentsection") or name.startswith("worldservice"):
            return "authoring-core"
        return "building-layout"
    if area == "tools":
        if "terrain" in name:
            return "terrain-site"
        if any(
            token in name
            for token in (
                "asset",
                "attachment",
                "material",
                "shape",
                "connected",
                "surface",
                "volume",
                "pattern",
                "structuralplacement",
                "selectionplacement",
            )
        ):
            return "assets-objects"
        return "interaction-controls"
    return "authoring-core"


def infer_department(path: str) -> str:
    lowered = path.lower()
    if path.startswith("tests/"):
        return infer_test_department(Path(path).name)
    if path.startswith("apps/iggy3d_creative/"):
        return infer_creative_app_department(path)
    if path.startswith("apps/iggy3d_playtest/"):
        return "playtest-simulation"
    if path.startswith("src/app/iggy3d/creative/"):
        return infer_creative_source_department(path)
    if path.startswith("src/app/iggy3d/save/"):
        return "persistence-validation"
    if path.startswith("src/app/iggy3d/map_maker/"):
        return "building-layout"
    if path.startswith("src/runtime/"):
        if any(
            path.startswith(prefix)
            for prefix in (
                "src/runtime/save/",
                "src/runtime/replay/",
                "src/runtime/diagnostics/",
            )
        ):
            return "persistence-validation"
        return "playtest-simulation"
    if path.startswith(("src/render/", "src/projection/", "shaders/")):
        return "rendering-preview"
    if path.startswith("src/content/assets/"):
        return "assets-objects"
    if path.startswith("src/content/"):
        return "playtest-simulation"
    if path.startswith("src/app/frontend/"):
        return "persistence-validation"
    if path.startswith(("assets/", "tools/blender/")):
        return "assets-objects"
    if path.startswith("tools/") and any(
        token in lowered for token in ("boulder", "walkway", "blender")
    ):
        return "assets-objects"
    if path.startswith("docs/"):
        if "creative_assets/" in lowered or "future_object" in lowered:
            return "assets-objects"
        if any(token in lowered for token in ("desktop_ui", "creative_ui/")):
            return "editor-shell-ui"
        if "control" in lowered:
            return "interaction-controls"
        if "terrain" in lowered:
            return "terrain-site"
        if "creative_maps/" in lowered or "physics/" in lowered:
            return "playtest-simulation"
        if "placement" in lowered:
            return "assets-objects"
        if "tool_completion" in lowered:
            return "authoring-core"
        return "foundation-build"
    if path.startswith("fixtures/"):
        return "playtest-simulation"
    return "foundation-build"


def bootstrap_ownership() -> None:
    department_ids = {department.department_id for department in load_departments()}
    rows: list[dict[str, str]] = []
    counts: Counter[str] = Counter()
    for path in governed_files():
        department_id = infer_department(path)
        if department_id not in department_ids:
            raise ValueError(f"{path}: inferred unknown department {department_id}")
        rows.append({"path": path, "department_id": department_id})
        counts[department_id] += 1
    write_tsv(OWNERSHIP_PATH, ["path", "department_id"], rows)
    print(f"wrote {OWNERSHIP_PATH.relative_to(ROOT)} with {len(rows)} paths")
    for department_id in sorted(counts):
        print(f"  {department_id}: {counts[department_id]}")


def resolve_include(source: str, include: str, known_paths: set[str]) -> str | None:
    source_parent = Path(source).parent
    candidates = (
        source_parent / include,
        Path("src") / include,
        Path("apps/iggy3d_creative") / include,
        Path("third_party") / include,
        Path(include),
    )
    for candidate in candidates:
        normalized = candidate.as_posix()
        if normalized in known_paths:
            return normalized
    return None


def build_include_graph(
    ownership: dict[str, str],
) -> tuple[dict[str, set[str]], dict[str, set[str]]]:
    known_paths = set(ownership)
    outgoing: dict[str, set[str]] = defaultdict(set)
    incoming: dict[str, set[str]] = defaultdict(set)
    for path in ownership:
        if Path(path).suffix not in {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}:
            continue
        absolute = ROOT / path
        try:
            contents = absolute.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for include in INCLUDE_RE.findall(contents):
            resolved = resolve_include(path, include, known_paths)
            if resolved is None or resolved == path:
                continue
            outgoing[path].add(resolved)
            incoming[resolved].add(path)
    return outgoing, incoming


def relative_link(from_path: Path, target: str) -> str:
    target_path = ROOT / target
    relative = Path(os.path.relpath(target_path, from_path.parent))
    return relative.as_posix()


def file_role(path: str) -> str:
    suffix = Path(path).suffix.lower()
    stem = Path(path).stem.replace("_", " ")
    if path.startswith("tests/"):
        return f"Automated proof for {stem}."
    if suffix in {".cpp", ".cc", ".c"}:
        return f"Implementation owner for {stem}."
    if suffix in {".hpp", ".hh", ".h"}:
        return f"Interface or shared declarations for {stem}."
    if suffix == ".md":
        return f"Product or engineering documentation for {stem}."
    if suffix in {".py", ".sh"}:
        return f"Generation or validation tool for {stem}."
    if path.startswith("assets/"):
        return f"Creative content or asset-pipeline input for {stem}."
    if suffix in {".cmake", ".txt"} or Path(path).name == "CMakeLists.txt":
        return f"Build configuration for {stem}."
    return f"Repository resource for {stem}."


def markdown_cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\n", " ")


def generate_files_docs(
    departments: list[Department],
    ownership: dict[str, str],
    outgoing: dict[str, set[str]],
    incoming: dict[str, set[str]],
) -> None:
    department_by_id = {department.department_id: department for department in departments}
    paths_by_department: dict[str, list[str]] = defaultdict(list)
    for path, department_id in ownership.items():
        paths_by_department[department_id].append(path)

    for department_id, paths in paths_by_department.items():
        output_path = DOC_ROOT / department_id / "FILES.md"
        lines = [
            f"# {department_by_id[department_id].title} File Map",
            "",
            "<!-- Generated by tools/repo_departments.py. Do not edit by hand. -->",
            "",
            f"Assigned files: **{len(paths)}**.",
            "",
            "| File | Role | Direct Repo Dependencies | Dependency Departments | Direct Consumers | Direct Test Consumers |",
            "| --- | --- | --- | --- | ---: | --- |",
        ]
        for path in sorted(paths):
            direct_dependencies = sorted(outgoing.get(path, set()))
            dependency_labels = ", ".join(
                f"[{Path(target).name}]({relative_link(output_path, target)})"
                for target in direct_dependencies[:4]
            )
            if len(direct_dependencies) > 4:
                dependency_labels += f", +{len(direct_dependencies) - 4}"
            if not dependency_labels:
                dependency_labels = "-"
            target_departments = sorted(
                {
                    ownership[target]
                    for target in outgoing.get(path, set())
                    if ownership[target] != department_id
                }
            )
            target_labels = ", ".join(
                department_by_id[target].title for target in target_departments
            )
            if not target_labels:
                target_labels = "-"
            direct_tests = sorted(
                consumer
                for consumer in incoming.get(path, set())
                if consumer.startswith("tests/")
            )
            test_labels = ", ".join(
                f"[{Path(test).name}]({relative_link(output_path, test)})"
                for test in direct_tests[:3]
            )
            if len(direct_tests) > 3:
                test_labels += f", +{len(direct_tests) - 3}"
            if not test_labels:
                test_labels = "-"
            file_link = relative_link(output_path, path)
            lines.append(
                "| "
                f"[{markdown_cell(path)}]({file_link}) | "
                f"{markdown_cell(file_role(path))} | "
                f"{dependency_labels} | "
                f"{markdown_cell(target_labels)} | "
                f"{len(incoming.get(path, set()))} | "
                f"{test_labels} |"
            )
        output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def generate_dependencies(
    departments: list[Department],
    ownership: dict[str, str],
    outgoing: dict[str, set[str]],
) -> None:
    department_by_id = {department.department_id: department for department in departments}
    pair_edges: Counter[tuple[str, str]] = Counter()
    pair_sources: dict[tuple[str, str], set[str]] = defaultdict(set)
    cross_edges_by_source: Counter[str] = Counter()
    cross_departments_by_source: dict[str, set[str]] = defaultdict(set)
    for source, targets in outgoing.items():
        source_department = ownership[source]
        for target in targets:
            target_department = ownership[target]
            if source_department == target_department:
                continue
            pair = (source_department, target_department)
            pair_edges[pair] += 1
            pair_sources[pair].add(source)
            cross_edges_by_source[source] += 1
            cross_departments_by_source[source].add(target_department)

    output_path = DOC_ROOT / "DEPENDENCIES.md"
    lines = [
        "# Department Dependencies",
        "",
        "<!-- Generated by tools/repo_departments.py. Do not edit by hand. -->",
        "",
        "Counts are resolved direct repository `#include` edges. They are an "
        "observability baseline, not yet an allow-or-deny policy.",
        "",
        "| From | To | Direct Include Edges | Source Files |",
        "| --- | --- | ---: | ---: |",
    ]
    for pair, edge_count in sorted(
        pair_edges.items(), key=lambda item: (-item[1], item[0])
    ):
        source_department, target_department = pair
        lines.append(
            f"| {department_by_id[source_department].title} | "
            f"{department_by_id[target_department].title} | {edge_count} | "
            f"{len(pair_sources[pair])} |"
        )

    lines.extend(
        [
            "",
            "## Highest Cross-Department Fan-Out",
            "",
            "These files deserve manual ownership review before physical moves.",
            "",
            "| File | Owner | Outgoing Departments | Cross-Department Edges |",
            "| --- | --- | --- | ---: |",
        ]
    )
    for source, edge_count in cross_edges_by_source.most_common(30):
        departments_out = ", ".join(
            department_by_id[department_id].title
            for department_id in sorted(cross_departments_by_source[source])
        )
        lines.append(
            f"| [{source}]({relative_link(output_path, source)}) | "
            f"{department_by_id[ownership[source]].title} | "
            f"{departments_out} | {edge_count} |"
        )
    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def generate_index(
    departments: list[Department],
    ownership: dict[str, str],
    work_items: list[WorkItem],
    manual_tests: list[ManualTest],
    audit_items: list[AuditItem],
) -> None:
    file_counts = Counter(ownership.values())
    code_counts: Counter[str] = Counter()
    test_counts: Counter[str] = Counter()
    for path, department_id in ownership.items():
        if path.startswith("tests/"):
            test_counts[department_id] += 1
        elif Path(path).suffix in {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}:
            code_counts[department_id] += 1
    work_by_department: dict[str, list[WorkItem]] = defaultdict(list)
    tests_by_department: dict[str, list[ManualTest]] = defaultdict(list)
    audits_by_department: dict[str, list[AuditItem]] = defaultdict(list)
    for item in work_items:
        work_by_department[item.department_id].append(item)
    for test in manual_tests:
        tests_by_department[test.department_id].append(test)
    for item in audit_items:
        audits_by_department[item.department_id].append(item)

    lines = [
        "# Repository Departments",
        "",
        "<!-- Generated by tools/repo_departments.py. Do not edit by hand. -->",
        "",
        "This is the operational dashboard for the Creative-only repository. "
        "Maturity describes implementation shape; delivery describes whether the "
        "product result has actually been accepted.",
        "",
        "| Rank | Tier | Department | Audit State | Files | Code | Tests | Needs Audit | Active | Manual Tests Pending | Retire or Consolidate | Blocked |",
        "| ---: | --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |",
    ]
    active_values = {"Planned", "In Progress", "Automated Green", "Manual Test Needed"}
    for department in sorted(departments, key=lambda item: item.priority):
        items = work_by_department[department.department_id]
        tests = tests_by_department[department.department_id]
        audits = audits_by_department[department.department_id]
        lines.append(
            f"| {department.priority} | {department.focus_tier} | "
            f"[{department.title}]({department.department_id}/DEPARTMENT.md) "
            f"| [{department.audit_state}]({department.department_id}/AUDIT.md) "
            f"| {file_counts[department.department_id]} "
            f"| {code_counts[department.department_id]} "
            f"| {test_counts[department.department_id]} "
            f"| {sum(item.delivery == 'Needs Audit' for item in items)} "
            f"| {sum(item.delivery in active_values for item in items)} "
            f"| {sum(test.status == 'Pending' for test in tests)} "
            f"| {sum(item.disposition in {'Delete Candidate', 'Consolidate'} for item in audits)} "
            f"| {sum(item.delivery == 'Blocked' for item in items)} |"
        )
    lines.extend(
        [
            "",
            "## Working Rules",
            "",
            "- Every governed tracked file has exactly one department owner.",
            "- Domain-specific UI belongs to its product department; only shared shell "
            "and widget code belongs to Editor Shell and Drafting UI.",
            "- Tests belong to the department whose behavior they prove.",
            "- A capability is not accepted until automated evidence and any required "
            "manual product test are recorded.",
            "- Ownership or wiring changes update this map in the same change.",
            "",
            "See [TEST_QUEUE.md](TEST_QUEUE.md) for work awaiting hands-on inspection.",
            "See [DEPENDENCIES.md](DEPENDENCIES.md) for current direct include flow and "
            "the highest cross-department fan-out files.",
        ]
    )
    (DOC_ROOT / "INDEX.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def generate_test_queue(
    departments: list[Department], manual_tests: list[ManualTest]
) -> None:
    department_by_id = {department.department_id: department for department in departments}
    pending = [test for test in manual_tests if test.status == "Pending"]
    lines = [
        "# Manual Test Queue",
        "",
        "<!-- Generated by tools/repo_departments.py. Do not edit by hand. -->",
        "",
        "These checks require a person to judge the visible or interactive result. "
        "Automated-green work remains incomplete while it is listed here.",
        "",
        "| Test | Department | Work | Scenario | Launch | Last Verified |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for test in sorted(pending, key=lambda item: item.test_id):
        department = department_by_id[test.department_id]
        testing_link = f"{test.department_id}/TESTING.md"
        lines.append(
            f"| [{test.test_id}]({testing_link}) | {department.title} | "
            f"{test.work_id} | {markdown_cell(test.scenario)} | "
            f"{markdown_cell(test.command)} | {markdown_cell(test.last_verified)} |"
        )
    if not pending:
        lines.append("| - | - | - | No manual tests are currently pending. | - | - |")
    (DOC_ROOT / "TEST_QUEUE.md").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def validate(
    departments: list[Department],
    ownership: dict[str, str],
    work_items: list[WorkItem],
    manual_tests: list[ManualTest],
) -> list[str]:
    errors: list[str] = []
    department_ids = {department.department_id for department in departments}
    if len(department_ids) != len(departments):
        errors.append("registry.tsv contains duplicate department ids")
    priorities = {department.priority for department in departments}
    expected_priorities = set(range(1, len(departments) + 1))
    if priorities != expected_priorities:
        errors.append(
            "registry.tsv priorities must be unique and contiguous from 1 "
            f"through {len(departments)}"
        )
    for department in departments:
        if department.focus_tier not in AUDIT_PRIORITY_VALUES:
            errors.append(
                f"{department.department_id}: invalid focus tier "
                f"{department.focus_tier}"
            )
        if department.audit_state not in AUDIT_STATE_VALUES:
            errors.append(
                f"{department.department_id}: invalid audit state "
                f"{department.audit_state}"
            )

    governed = set(governed_files())
    owned = set(ownership)
    for path in sorted(governed - owned):
        errors.append(f"unowned tracked file: {path}")
    for path in sorted(owned - governed):
        errors.append(f"ownership row is stale or untracked: {path}")
    for path, department_id in ownership.items():
        if department_id not in department_ids:
            errors.append(f"{path}: unknown department {department_id}")

    work_ids: dict[str, WorkItem] = {}
    for item in work_items:
        if item.work_id in work_ids:
            errors.append(f"duplicate work id: {item.work_id}")
        work_ids[item.work_id] = item
        if item.maturity not in MATURITY_VALUES:
            errors.append(f"{item.work_id}: invalid maturity {item.maturity}")
        if item.delivery not in DELIVERY_VALUES:
            errors.append(f"{item.work_id}: invalid delivery {item.delivery}")

    test_ids: set[str] = set()
    pending_work_ids: set[str] = set()
    for test in manual_tests:
        if test.test_id in test_ids:
            errors.append(f"duplicate manual test id: {test.test_id}")
        test_ids.add(test.test_id)
        if test.work_id not in work_ids:
            errors.append(f"{test.test_id}: unknown work id {test.work_id}")
        elif work_ids[test.work_id].department_id != test.department_id:
            errors.append(
                f"{test.test_id}: work item {test.work_id} belongs to another department"
            )
        if test.status not in TEST_STATUS_VALUES:
            errors.append(f"{test.test_id}: invalid status {test.status}")
        if test.status == "Pending":
            pending_work_ids.add(test.work_id)
    for item in work_items:
        if item.delivery == "Manual Test Needed" and item.work_id not in pending_work_ids:
            errors.append(f"{item.work_id}: manual test needed but no pending test exists")

    for department in departments:
        department_root = DOC_ROOT / department.department_id
        for filename in (
            "DEPARTMENT.md",
            "TODO.md",
            "TESTING.md",
            "AUDIT.md",
            "FILES.md",
        ):
            if not (department_root / filename).is_file():
                errors.append(
                    f"{department.department_id}: missing required {filename}"
                )
    return errors


def validate_audits(
    departments: list[Department], audit_items: list[AuditItem]
) -> list[str]:
    errors: list[str] = []
    department_ids = {department.department_id for department in departments}
    audit_ids: set[str] = set()
    for item in audit_items:
        if item.department_id not in department_ids:
            errors.append(
                f"{item.audit_id}: unknown department {item.department_id}"
            )
        if item.audit_id in audit_ids:
            errors.append(f"duplicate audit id: {item.audit_id}")
        audit_ids.add(item.audit_id)
        if item.classification not in AUDIT_CLASSIFICATION_VALUES:
            errors.append(
                f"{item.audit_id}: invalid classification {item.classification}"
            )
        if item.disposition not in AUDIT_DISPOSITION_VALUES:
            errors.append(
                f"{item.audit_id}: invalid disposition {item.disposition}"
            )
        if item.priority not in AUDIT_PRIORITY_VALUES:
            errors.append(f"{item.audit_id}: invalid priority {item.priority}")
    return errors


def generate() -> None:
    departments = load_departments()
    ownership = load_ownership()
    work_items = load_work_items(departments)
    manual_tests = load_manual_tests(departments)
    audit_items = load_audit_items(departments)
    outgoing, incoming = build_include_graph(ownership)
    generate_files_docs(departments, ownership, outgoing, incoming)
    generate_dependencies(departments, ownership, outgoing)
    generate_index(departments, ownership, work_items, manual_tests, audit_items)
    generate_test_queue(departments, manual_tests)
    print(
        f"generated dashboard and {len(departments)} file maps for "
        f"{len(ownership)} governed files"
    )


def check_generated_outputs() -> list[str]:
    before: dict[Path, bytes] = {}
    generated_paths = [
        DOC_ROOT / "INDEX.md",
        DOC_ROOT / "DEPENDENCIES.md",
        DOC_ROOT / "TEST_QUEUE.md",
    ]
    for department in load_departments():
        generated_paths.append(DOC_ROOT / department.department_id / "FILES.md")
    for path in generated_paths:
        if path.is_file():
            before[path] = path.read_bytes()
    generate()
    errors: list[str] = []
    for path in generated_paths:
        previous = before.get(path)
        current = path.read_bytes() if path.is_file() else None
        if previous is None:
            errors.append(f"generated output was missing: {path.relative_to(ROOT)}")
            if path.is_file():
                path.unlink()
        elif current != previous:
            errors.append(f"generated output was stale: {path.relative_to(ROOT)}")
            path.write_bytes(previous)
    return errors


def check() -> int:
    departments = load_departments()
    ownership = load_ownership()
    work_items = load_work_items(departments)
    manual_tests = load_manual_tests(departments)
    audit_items = load_audit_items(departments)
    errors = validate(departments, ownership, work_items, manual_tests)
    errors.extend(validate_audits(departments, audit_items))
    if not errors:
        errors.extend(check_generated_outputs())
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(
        f"department map valid: {len(departments)} departments, "
        f"{len(ownership)} governed files, {len(work_items)} work items, "
        f"{len(manual_tests)} manual tests, {len(audit_items)} audit findings"
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "command", choices=("bootstrap", "generate", "check"), help="operation to run"
    )
    args = parser.parse_args()
    if args.command == "bootstrap":
        bootstrap_ownership()
        return 0
    if args.command == "generate":
        generate()
        return 0
    return check()


if __name__ == "__main__":
    raise SystemExit(main())
