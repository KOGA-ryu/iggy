#!/usr/bin/env python3
"""Deterministic report-only production include graph scanner."""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx")
IGNORED_PARTS = frozenset(("build", "generated", "vendor", "third_party"))
DEPARTMENTS = (
    ("core", "src/core"),
    ("config", "src/config"),
    ("content", "src/content"),
    ("runtime", "src/runtime"),
    ("projection", "src/projection"),
    ("render", "src/render"),
    ("app", "src/app"),
)
DEPARTMENT_NAMES = tuple(name for name, _ in DEPARTMENTS)
DEPARTMENT_ROOTS = frozenset(name for name, _ in DEPARTMENTS)

INCLUDE_START_RE = re.compile(r"^\s*#\s*include\b")
INCLUDE_RE = re.compile(
    r'^\s*#\s*include\b\s*(?P<open>[<"])(?P<name>[^<>"\n]+)(?P<close>[>"])\s*$'
)

FATAL_DIAGNOSTICS = frozenset(
    {
        "malformed_include",
        "unresolved_project_include",
        "unclassified_source",
        "unclassified_target",
    }
)


def _relative_path(path: Path, repo_root: Path) -> str:
    return path.relative_to(repo_root).as_posix()


def _is_within(path: Path, root: Path) -> bool:
    try:
        path.relative_to(root)
    except ValueError:
        return False
    return True


def _department_for(path: Path, repo_root: Path) -> str | None:
    relative = _relative_path(path, repo_root)
    matches = [
        name
        for name, prefix in DEPARTMENTS
        if relative == prefix or relative.startswith(prefix + "/")
    ]
    if not matches:
        return None
    return max(matches, key=lambda name: len(dict(DEPARTMENTS)[name]))


def _strip_comments(text: str) -> str:
    """Remove C/C++ comments while retaining strings and source line numbers."""

    output: list[str] = []
    index = 0
    state = "normal"
    while index < len(text):
        char = text[index]
        next_char = text[index + 1] if index + 1 < len(text) else ""
        if state == "normal":
            if char == "/" and next_char == "/":
                output.append(" ")
                output.append(" ")
                index += 2
                state = "line_comment"
                continue
            if char == "/" and next_char == "*":
                output.append(" ")
                output.append(" ")
                index += 2
                state = "block_comment"
                continue
            if char == '"':
                state = "string"
            elif char == "'":
                state = "character"
            output.append(char)
            index += 1
            continue
        if state == "line_comment":
            if char == "\n":
                output.append(char)
                state = "normal"
            else:
                output.append(" ")
            index += 1
            continue
        if state == "block_comment":
            if char == "*" and next_char == "/":
                output.append(" ")
                output.append(" ")
                index += 2
                state = "normal"
            else:
                output.append("\n" if char == "\n" else " ")
                index += 1
            continue
        output.append(char)
        if char == "\\" and index + 1 < len(text):
            output.append(text[index + 1])
            index += 2
            continue
        if (state == "string" and char == '"') or (
            state == "character" and char == "'"
        ):
            state = "normal"
        index += 1
    return "".join(output)


def _diagnostic(
    code: str,
    source: str | None,
    line: int | None,
    message: str,
    target: str | None = None,
) -> dict[str, object]:
    result: dict[str, object] = {"code": code, "message": message}
    if source is not None:
        result["source"] = source
    if line is not None:
        result["line"] = line
    if target is not None:
        result["target"] = target
    return result


def _project_candidate(
    include_name: str,
    source_path: Path,
    repo_root: Path,
    src_root: Path,
    opener: str,
) -> tuple[Path | None, str | None]:
    normalized = include_name.replace("\\", "/")
    parts = Path(normalized).parts
    is_department_root = bool(parts) and parts[0] in DEPARTMENT_ROOTS
    is_src_root = normalized == "src" or normalized.startswith("src/")
    candidate: Path | None = None
    project_named = is_department_root or is_src_root
    if is_department_root:
        candidate = src_root.joinpath(*parts)
    elif is_src_root:
        candidate = repo_root.joinpath(*parts)
    elif opener == '"':
        candidate = source_path.parent.joinpath(*parts)

    if candidate is None:
        return None, None
    candidate = candidate.resolve()
    if not _is_within(candidate, src_root):
        return None, None
    if opener == '"':
        project_named = True
    if not candidate.exists() or not candidate.is_file():
        if project_named:
            return None, "unresolved_project_include"
        return None, None
    if candidate.suffix not in SOURCE_SUFFIXES:
        return None, "unclassified_target"
    if _department_for(candidate, repo_root) is None:
        return candidate, "unclassified_target"
    return candidate, None


def _scan_sources(repo_root: Path) -> tuple[list[Path], list[dict[str, object]]]:
    src_root = repo_root / "src"
    diagnostics: list[dict[str, object]] = []
    if not src_root.is_dir():
        diagnostics.append(
            _diagnostic(
                "unresolved_project_include",
                None,
                None,
                "production source root does not exist: src",
            )
        )
        return [], diagnostics
    sources: list[Path] = []
    for path in sorted(src_root.rglob("*")):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue
        relative_parts = path.relative_to(repo_root).parts
        if IGNORED_PARTS.intersection(relative_parts):
            continue
        sources.append(path.resolve())
        if _department_for(path, repo_root) is None:
            diagnostics.append(
                _diagnostic(
                    "unclassified_source",
                    _relative_path(path, repo_root),
                    None,
                    "production source is outside the configured departments",
                )
            )
    return sources, diagnostics


def _build_report(repo_root: Path) -> tuple[dict[str, object], bool]:
    repo_root = repo_root.resolve()
    src_root = repo_root / "src"
    sources, diagnostics = _scan_sources(repo_root)
    edges: set[tuple[str, str, str, str]] = set()
    for source_path in sources:
        source_name = _relative_path(source_path, repo_root)
        source_department = _department_for(source_path, repo_root)
        if source_department is None:
            continue
        try:
            text = _strip_comments(source_path.read_text(encoding="utf-8"))
        except (OSError, UnicodeError) as error:
            diagnostics.append(
                _diagnostic(
                    "unresolved_project_include",
                    source_name,
                    None,
                    f"could not read production source: {error}",
                )
            )
            continue
        seen_targets: set[str] = set()
        for line_number, line in enumerate(text.splitlines(), start=1):
            if not INCLUDE_START_RE.match(line):
                continue
            match = INCLUDE_RE.match(line)
            if match is None or match.group("open") == "<" and match.group("close") != ">" or match.group("open") == '"' and match.group("close") != '"':
                diagnostics.append(
                    _diagnostic(
                        "malformed_include",
                        source_name,
                        line_number,
                        "malformed project include directive",
                    )
                )
                continue
            opener = match.group("open")
            include_name = match.group("name").strip()
            candidate, error_code = _project_candidate(
                include_name, source_path, repo_root, src_root, opener
            )
            if error_code is not None:
                diagnostics.append(
                    _diagnostic(
                        error_code,
                        source_name,
                        line_number,
                        f"project include cannot be resolved: {include_name}",
                        include_name,
                    )
                )
                continue
            if candidate is None:
                continue
            target_name = _relative_path(candidate, repo_root)
            target_department = _department_for(candidate, repo_root)
            if target_department is None:
                diagnostics.append(
                    _diagnostic(
                        "unclassified_target",
                        source_name,
                        line_number,
                        "resolved project target is outside the configured departments",
                        target_name,
                    )
                )
                continue
            if target_name in seen_targets:
                diagnostics.append(
                    _diagnostic(
                        "duplicate_include",
                        source_name,
                        line_number,
                        "duplicate resolved include was de-duplicated",
                        target_name,
                    )
                )
                continue
            seen_targets.add(target_name)
            if source_department != target_department:
                edges.add((source_name, target_name, source_department, target_department))

    sorted_edges = [
        {
            "source": source,
            "target": target,
            "source_department": source_department,
            "target_department": target_department,
        }
        for source, target, source_department, target_department in sorted(edges)
    ]
    pair_data: dict[tuple[str, str], dict[str, object]] = {}
    for edge in sorted_edges:
        pair = (edge["source_department"], edge["target_department"])
        row = pair_data.setdefault(pair, {"edge_count": 0, "source_files": set()})
        row["edge_count"] = int(row["edge_count"]) + 1
        row["source_files"].add(edge["source"])
    directed_pairs = [
        {
            "source": source,
            "target": target,
            "edge_count": int(row["edge_count"]),
            "source_file_count": len(row["source_files"]),
        }
        for (source, target), row in sorted(
            pair_data.items(),
            key=lambda item: (DEPARTMENT_NAMES.index(item[0][0]), DEPARTMENT_NAMES.index(item[0][1])),
        )
    ]

    adjacency = {department: set() for department in DEPARTMENT_NAMES}
    for source, target in pair_data:
        adjacency[source].add(target)
    fan_out: dict[str, dict[str, int]] = {}
    fan_in: dict[str, dict[str, int]] = {}
    for department in DEPARTMENT_NAMES:
        outgoing = [row for row in directed_pairs if row["source"] == department]
        incoming = [row for row in directed_pairs if row["target"] == department]
        fan_out[department] = {
            "edge_count": sum(int(row["edge_count"]) for row in outgoing),
            "department_count": len(outgoing),
        }
        fan_in[department] = {
            "edge_count": sum(int(row["edge_count"]) for row in incoming),
            "department_count": len(incoming),
        }

    closure: list[dict[str, object]] = []
    for department in DEPARTMENT_NAMES:
        reachable: set[str] = set()
        pending = list(sorted(adjacency[department], key=DEPARTMENT_NAMES.index))
        while pending:
            target = pending.pop(0)
            if target in reachable:
                continue
            reachable.add(target)
            pending.extend(
                child
                for child in sorted(adjacency[target], key=DEPARTMENT_NAMES.index)
                if child not in reachable
            )
        closure.append(
            {"source": department, "targets": sorted(reachable, key=DEPARTMENT_NAMES.index)}
        )

    def strongly_connected_components() -> list[list[str]]:
        index = 0
        stack: list[str] = []
        on_stack: set[str] = set()
        indices: dict[str, int] = {}
        low_links: dict[str, int] = {}
        components: list[list[str]] = []

        def visit(node: str) -> None:
            nonlocal index
            indices[node] = index
            low_links[node] = index
            index += 1
            stack.append(node)
            on_stack.add(node)
            for child in sorted(adjacency[node], key=DEPARTMENT_NAMES.index):
                if child not in indices:
                    visit(child)
                    low_links[node] = min(low_links[node], low_links[child])
                elif child in on_stack:
                    low_links[node] = min(low_links[node], indices[child])
            if low_links[node] != indices[node]:
                return
            component: list[str] = []
            while True:
                child = stack.pop()
                on_stack.remove(child)
                component.append(child)
                if child == node:
                    break
            if len(component) > 1:
                components.append(sorted(component, key=DEPARTMENT_NAMES.index))

        for department in DEPARTMENT_NAMES:
            if department not in indices:
                visit(department)
        return sorted(components, key=lambda component: DEPARTMENT_NAMES.index(component[0]))

    diagnostics = sorted(
        diagnostics,
        key=lambda item: (
            str(item.get("source", "")),
            int(item.get("line", 0) or 0),
            str(item.get("code", "")),
            str(item.get("target", "")),
            str(item.get("message", "")),
        ),
    )
    fatal = any(item["code"] in FATAL_DIAGNOSTICS for item in diagnostics)
    report: dict[str, object] = {
        "schema": "iggy3d.dependency_graph.v1",
        "scan": {
            "source_root": "src",
            "suffixes": list(SOURCE_SUFFIXES),
            "excluded_roots": ["build", "generated", "vendor", "third_party", "tests", "docs", "tools", "apps"],
            "source_file_count": len(sources),
            "direct_edge_count": len(sorted_edges),
            "directed_pair_count": len(directed_pairs),
            "unordered_pair_count": len({frozenset((row["source"], row["target"])) for row in directed_pairs}),
        },
        "departments": [
            {"name": name, "root": prefix} for name, prefix in DEPARTMENTS
        ],
        "cross_department_edges": sorted_edges,
        "directed_pairs": directed_pairs,
        "fan_out": fan_out,
        "fan_in": fan_in,
        "transitive_closure": closure,
        "strongly_connected_components": [
            {"departments": component} for component in strongly_connected_components()
        ],
        "diagnostics": diagnostics,
        "policy": None,
    }
    return report, not fatal


class PolicyError(ValueError):
    """Raised when a policy file does not satisfy the machine schema."""


def _policy_pair(value: object) -> tuple[str, str]:
    if isinstance(value, str):
        parts = [part.strip() for part in value.split("->")]
        if len(parts) == 2:
            return parts[0], parts[1]
    if isinstance(value, dict) and isinstance(value.get("source"), str) and isinstance(value.get("target"), str):
        return value["source"], value["target"]
    raise PolicyError("allowed pair must be a 'source->target' string or source/target object")


def _load_policy(path: Path) -> dict[str, object]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise PolicyError(f"could not read policy: {error}") from error
    if not isinstance(data, dict) or data.get("schema") != "iggy3d.architecture_dependency_policy.v1":
        raise PolicyError("policy schema must be iggy3d.architecture_dependency_policy.v1")
    if data.get("source_root") != "src":
        raise PolicyError("policy source_root must be src")
    if data.get("suffixes") != list(SOURCE_SUFFIXES):
        raise PolicyError("policy suffixes do not match the scanner suffixes")
    departments = data.get("departments")
    if not isinstance(departments, list) or [item.get("name") for item in departments if isinstance(item, dict)] != list(DEPARTMENT_NAMES):
        raise PolicyError("policy departments must be the ordered seven-department map")
    for expected, item in zip(DEPARTMENTS, departments):
        if not isinstance(item, dict) or item.get("name") != expected[0] or item.get("prefix") != expected[1]:
            raise PolicyError("policy department prefixes do not match the scanner map")

    allowed_pairs_value = data.get(
        "allowed_pairs",
        data.get("allowed_directed_pairs", data.get("allowed_edges")),
    )
    if not isinstance(allowed_pairs_value, list):
        raise PolicyError("policy allowed_pairs must be a list")
    allowed_pairs = {_policy_pair(value) for value in allowed_pairs_value}
    if any(source not in DEPARTMENT_NAMES or target not in DEPARTMENT_NAMES or source == target for source, target in allowed_pairs):
        raise PolicyError("policy allowed_pairs contain an unknown or self-directed department")

    restricted: dict[tuple[str, str], tuple[str, ...]] = {}
    restricted_value = data.get("restricted_targets", [])
    if isinstance(restricted_value, list):
        for item in restricted_value:
            if not isinstance(item, dict) or not isinstance(item.get("source"), str) or not isinstance(item.get("target"), str) or not isinstance(item.get("targets"), list) or not all(isinstance(target, str) for target in item["targets"]):
                raise PolicyError("restricted_targets list entries must contain source, target, and string targets")
            restricted[(item["source"], item["target"])] = tuple(sorted(item["targets"]))
    elif isinstance(restricted_value, dict):
        for pair_name, targets in restricted_value.items():
            source, target = _policy_pair(pair_name)
            if not isinstance(targets, list) or not all(isinstance(item, str) for item in targets):
                raise PolicyError("restricted_targets map values must be string lists")
            restricted[(source, target)] = tuple(sorted(targets))
    else:
        raise PolicyError("policy restricted_targets must be a list or map")
    if any(source not in DEPARTMENT_NAMES or target not in DEPARTMENT_NAMES for source, target in restricted):
        raise PolicyError("policy restricted_targets contain an unknown department")

    ratchets = data.get("ratchets")
    if not isinstance(ratchets, dict):
        raise PolicyError("policy ratchets must be an object")
    max_direct_edges = ratchets.get(
        "max_direct_edges",
        ratchets.get(
            "max_cross_department_edges",
            ratchets.get("max_direct_cross_department_edges", ratchets.get("total_cross_department_edges")),
        ),
    )
    if not isinstance(max_direct_edges, int) or max_direct_edges < 0:
        raise PolicyError("policy ratchets.max_direct_edges must be a non-negative integer")
    department_ratchets = ratchets.get("departments")
    if not isinstance(department_ratchets, dict):
        raise PolicyError("policy ratchets.departments must be an object")
    normalized_ratchets: dict[str, dict[str, int]] = {}
    for department in DEPARTMENT_NAMES:
        values = department_ratchets.get(department)
        if not isinstance(values, dict) or not isinstance(values.get("max_direct_edge_out"), int) or not isinstance(values.get("max_unique_department_out"), int):
            if isinstance(values, dict) and isinstance(values.get("max_direct_edges_out"), int) and isinstance(values.get("max_unique_edges_out"), int):
                values = {
                    "max_direct_edge_out": values["max_direct_edges_out"],
                    "max_unique_department_out": values["max_unique_edges_out"],
                }
            else:
                raise PolicyError(f"policy ratchet is missing for {department}")
        if values["max_direct_edge_out"] < 0 or values["max_unique_department_out"] < 0:
            raise PolicyError("policy ratchet ceilings must be non-negative")
        normalized_ratchets[department] = {
            "max_direct_edge_out": values["max_direct_edge_out"],
            "max_unique_department_out": values["max_unique_department_out"],
        }

    exceptions_value = data.get("allowed_exceptions")
    if not isinstance(exceptions_value, list):
        raise PolicyError("policy allowed_exceptions must be a list")
    exceptions: set[tuple[str, str]] = set()
    for item in exceptions_value:
        if isinstance(item, str):
            exception = _policy_pair(item)
        elif isinstance(item, dict) and isinstance(item.get("source"), str) and isinstance(item.get("target"), str):
            exception = (item["source"], item["target"])
        else:
            raise PolicyError("allowed_exceptions entries must contain source and target")
        if exception in exceptions:
            raise PolicyError("allowed_exceptions must not contain duplicates")
        exceptions.add(exception)
    return {
        "schema": data["schema"],
        "allowed_pairs": allowed_pairs,
        "restricted_targets": restricted,
        "max_direct_edges": max_direct_edges,
        "department_ratchets": normalized_ratchets,
        "allowed_exceptions": exceptions,
    }


def _apply_policy(report: dict[str, object], policy: dict[str, object]) -> dict[str, object]:
    violations: list[dict[str, object]] = []
    allowed_pairs = policy["allowed_pairs"]
    exceptions = policy["allowed_exceptions"]
    observed_edges = {(edge["source"], edge["target"]) for edge in report["cross_department_edges"]}
    for edge in report["cross_department_edges"]:
        pair = (edge["source_department"], edge["target_department"])
        edge_key = (edge["source"], edge["target"])
        if pair not in allowed_pairs and edge_key not in exceptions:
            violations.append({
                "code": "forbidden_edge",
                "source": edge["source"],
                "target": edge["target"],
                "source_department": edge["source_department"],
                "target_department": edge["target_department"],
            })
        targets = policy["restricted_targets"].get(pair)
        if targets is not None and edge["target"] not in targets:
            violations.append({
                "code": "restricted_target",
                "source": edge["source"],
                "target": edge["target"],
                "allowed_targets": list(targets),
            })
    for source, target in sorted(exceptions):
        if (source, target) not in observed_edges:
            violations.append({
                "code": "exception_mismatch",
                "source": source,
                "target": target,
            })
    for component in report["strongly_connected_components"]:
        violations.append({
            "code": "cycle",
            "departments": component["departments"],
        })
    if report["scan"]["direct_edge_count"] > policy["max_direct_edges"]:
        violations.append({
            "code": "ratchet_total",
            "actual": report["scan"]["direct_edge_count"],
            "maximum": policy["max_direct_edges"],
        })
    for department, ceiling in policy["department_ratchets"].items():
        actual = report["fan_out"][department]
        if actual["edge_count"] > ceiling["max_direct_edge_out"]:
            violations.append({
                "code": "ratchet_direct_edge_out",
                "department": department,
                "actual": actual["edge_count"],
                "maximum": ceiling["max_direct_edge_out"],
            })
        if actual["department_count"] > ceiling["max_unique_department_out"]:
            violations.append({
                "code": "ratchet_unique_department_out",
                "department": department,
                "actual": actual["department_count"],
                "maximum": ceiling["max_unique_department_out"],
            })
    violations.sort(key=lambda item: json.dumps(item, sort_keys=True))
    return {
        "schema": policy["schema"],
        "checked": True,
        "passed": not violations,
        "violations": violations,
    }


def _render_text(report: dict[str, object]) -> str:
    scan = report["scan"]
    lines = [
        f"schema: {report['schema']}",
        f"source files: {scan['source_file_count']}",
        f"direct cross-department edges: {scan['direct_edge_count']}",
        f"directed pairs: {scan['directed_pair_count']}",
        f"unordered pairs: {scan['unordered_pair_count']}",
        "departments:",
    ]
    lines.extend(f"  {row['name']}: {row['root']}" for row in report["departments"])
    lines.append("cross_department_edges:")
    lines.extend(
        f"  {edge['source']} -> {edge['target']} ({edge['source_department']} -> {edge['target_department']})"
        for edge in report["cross_department_edges"]
    )
    lines.append("directed pairs:")
    lines.extend(
        f"  {row['source']} -> {row['target']}: edges={row['edge_count']} source_files={row['source_file_count']}"
        for row in report["directed_pairs"]
    )
    lines.append("fan_out:")
    lines.extend(
        f"  {name}: edges={row['edge_count']} departments={row['department_count']}"
        for name, row in report["fan_out"].items()
    )
    lines.append("fan_in:")
    lines.extend(
        f"  {name}: edges={row['edge_count']} departments={row['department_count']}"
        for name, row in report["fan_in"].items()
    )
    lines.append("transitive_closure:")
    lines.extend(f"  {row['source']}: {', '.join(row['targets']) or '-'}" for row in report["transitive_closure"])
    lines.append("strongly_connected_components:")
    lines.extend(f"  {', '.join(row['departments'])}" for row in report["strongly_connected_components"])
    lines.append("diagnostics:")
    if report["diagnostics"]:
        lines.extend(f"  {json.dumps(item, sort_keys=True)}" for item in report["diagnostics"])
    else:
        lines.append("  none")
    if report["policy"] is None:
        lines.append("policy: null")
    else:
        lines.append(f"policy: {'pass' if report['policy']['passed'] else 'fail'}")
        lines.extend(f"  {json.dumps(item, sort_keys=True)}" for item in report["policy"]["violations"])
    return "\n".join(lines) + "\n"


def _render_json(report: dict[str, object]) -> str:
    return json.dumps(report, indent=2, sort_keys=False) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--policy", type=Path)
    parser.add_argument("--check-policy", action="store_true")
    parser.add_argument("--format", choices=("text", "json"), default="text")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    if args.check_policy and args.policy is None:
        parser.error("--check-policy requires --policy")
    if not args.repo_root.is_dir():
        print(f"dependency graph scan failed: repo root is not a directory: {args.repo_root}", file=sys.stderr)
        return 3
    try:
        policy = _load_policy(args.policy) if args.policy is not None else None
        report, reliable = _build_report(args.repo_root)
        if args.check_policy and policy is not None:
            report["policy"] = _apply_policy(report, policy)
        rendered = _render_json(report) if args.format == "json" else _render_text(report)
        if args.output is not None:
            args.output.write_text(rendered, encoding="utf-8")
        else:
            sys.stdout.write(rendered)
    except PolicyError as error:
        print(f"dependency graph policy failed: {error}", file=sys.stderr)
        return 2
    except (OSError, UnicodeError, ValueError) as error:
        print(f"dependency graph scan failed: {error}", file=sys.stderr)
        return 3
    if not reliable:
        return 3
    if args.check_policy and report["policy"] is not None and not report["policy"]["passed"]:
        return 4
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
