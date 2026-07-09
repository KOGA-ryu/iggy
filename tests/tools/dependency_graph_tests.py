#!/usr/bin/env python3
"""Synthetic tests for the report-only dependency graph scanner."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "dependency_graph.py"
DEPARTMENTS = ("core", "config", "content", "runtime", "projection", "render", "app")
PREFIXES = {
    "core": "src/core",
    "config": "src/config",
    "content": "src/content",
    "runtime": "src/runtime",
    "projection": "src/projection",
    "render": "src/render",
    "app": "src/app",
}


def write_file(root: Path, relative: str, contents: str) -> None:
    path = root / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(contents, encoding="utf-8")


def run_tool(root: Path, *extra: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(TOOL), "--repo-root", str(root), *extra],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def load_json(result: subprocess.CompletedProcess[str]) -> dict[str, object]:
    assert result.returncode == 0, result.stderr + result.stdout
    return json.loads(result.stdout)


def build_graph_fixture(root: Path) -> None:
    write_file(root, "src/core/types.hpp", "#pragma once\n")
    write_file(root, "src/content/content.hpp", '#include <core/types.hpp>\n')
    write_file(root, "src/runtime/runtime.hpp", '#include "../content/content.hpp"\n#include <content/content.hpp>\n')
    write_file(
        root,
        "src/projection/projection.hpp",
        '#include <render/render.hpp>\n#include "projection_local.hpp"\n// #include <core/ignored.hpp>\n/* #include <core/ignored.hpp> */\n',
    )
    write_file(root, "src/projection/projection_local.hpp", "#include <core/types.hpp>\n")
    write_file(root, "src/render/render.hpp", '#include <projection/projection.hpp>\n')
    write_file(
        root,
        "src/app/main.cpp",
        '#include <runtime/runtime.hpp>\n#include <runtime/runtime.hpp>\n#include "local.hpp"\n',
    )
    write_file(root, "src/app/local.hpp", '#include <core/types.hpp>\n')


def write_policy(root: Path, **overrides: object) -> Path:
    policy: dict[str, object] = {
        "schema": "iggy3d.architecture_dependency_policy.v1",
        "source_root": "src",
        "suffixes": [".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"],
        "departments": [
            {"name": department, "prefix": PREFIXES[department]}
            for department in DEPARTMENTS
        ],
        "allowed_pairs": [
            {"source": source, "target": target}
            for source in DEPARTMENTS
            for target in DEPARTMENTS
            if source != target
        ],
        "restricted_targets": [],
        "ratchets": {
            "max_direct_edges": 100,
            "departments": {
                department: {
                    "max_direct_edge_out": 100,
                    "max_unique_department_out": 100,
                }
                for department in DEPARTMENTS
            },
        },
        "allowed_exceptions": [],
    }
    policy.update(overrides)
    path = root / "policy.json"
    path.write_text(json.dumps(policy, indent=2), encoding="utf-8")
    return path


def test_graph_facts_and_diagnostics() -> None:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        report = load_json(run_tool(root, "--format", "json"))
        assert report["scan"]["direct_edge_count"] == 7
        assert report["scan"]["directed_pair_count"] == 7
        assert report["scan"]["unordered_pair_count"] == 6
        assert report["strongly_connected_components"] == [
            {"departments": ["projection", "render"]}
        ]
        assert report["fan_out"]["app"] == {"edge_count": 2, "department_count": 2}
        assert report["fan_in"]["core"]["edge_count"] == 3
        assert any(item["code"] == "duplicate_include" for item in report["diagnostics"])
        closure = {item["source"]: item["targets"] for item in report["transitive_closure"]}
        assert "runtime" in closure["app"]
        assert "projection" in closure["render"]
        text_result = run_tool(root, "--format", "text")
        assert text_result.returncode == 0
        assert "direct cross-department edges: 7" in text_result.stdout


def test_deterministic_json_and_output_path() -> None:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        first = run_tool(root, "--format", "json")
        second = run_tool(root, "--format", "json")
        assert first.stdout == second.stdout
        parsed = json.loads(first.stdout)
        assert list(parsed) == [
            "schema",
            "scan",
            "departments",
            "cross_department_edges",
            "directed_pairs",
            "fan_out",
            "fan_in",
            "transitive_closure",
            "strongly_connected_components",
            "diagnostics",
            "policy",
        ]
        output = root / "report.json"
        written = run_tool(root, "--format", "json", "--output", str(output))
        assert written.returncode == 0
        assert output.read_text(encoding="utf-8") == first.stdout


def test_unreliable_scan_diagnostics() -> None:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        write_file(root, "src/core/types.hpp", "")
        write_file(root, "src/app/bad.cpp", '#include <core/missing.hpp>\n')
        result = run_tool(root, "--format", "json")
        assert result.returncode == 3
        report = json.loads(result.stdout)
        assert report["diagnostics"][0]["code"] == "unresolved_project_include"

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        write_file(root, "src/app/bad.cpp", '#include "core/\n')
        result = run_tool(root, "--format", "json")
        assert result.returncode == 3
        assert any(
            item["code"] == "malformed_include"
            for item in json.loads(result.stdout)["diagnostics"]
        )

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        write_file(root, "src/misc/unclassified.cpp", "")
        result = run_tool(root, "--format", "json")
        assert result.returncode == 3
        assert any(
            item["code"] == "unclassified_source"
            for item in json.loads(result.stdout)["diagnostics"]
        )

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        write_file(root, "src/app/main.cpp", '#include "../misc/missing.hpp"\n')
        write_file(root, "src/misc/target.hpp", "")
        write_file(root, "src/app/other.cpp", '#include "../misc/target.hpp"\n')
        result = run_tool(root, "--format", "json")
        assert result.returncode == 3
        assert any(
            item["code"] == "unclassified_target"
            for item in json.loads(result.stdout)["diagnostics"]
        )


def test_policy_checks_and_cli_errors() -> None:
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        write_file(root, "src/core/types.hpp", "")
        write_file(root, "src/app/main.cpp", '#include <core/types.hpp>\n')
        policy_path = write_policy(root)
        result = run_tool(root, "--policy", str(policy_path), "--check-policy", "--format", "json")
        assert result.returncode == 0
        assert json.loads(result.stdout)["policy"]["passed"]

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        policy_path = write_policy(
            root,
            allowed_pairs=[{"source": "app", "target": "core"}],
        )
        result = run_tool(root, "--policy", str(policy_path), "--check-policy", "--format", "json")
        assert result.returncode == 4
        policy = json.loads(result.stdout)["policy"]
        assert not policy["passed"]
        assert any(item["code"] == "forbidden_edge" for item in policy["violations"])
        assert any(item["code"] == "cycle" for item in policy["violations"])

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        policy_path = write_policy(
            root,
            restricted_targets=[
                {"source": "content", "target": "core", "targets": ["src/core/not_used.hpp"]}
            ],
        )
        result = run_tool(root, "--policy", str(policy_path), "--check-policy", "--format", "json")
        assert result.returncode == 4
        assert any(
            item["code"] == "restricted_target"
            for item in json.loads(result.stdout)["policy"]["violations"]
        )

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        policy_path = write_policy(
            root,
            allowed_exceptions=[
                {"source": "src/app/not_seen.cpp", "target": "src/core/types.hpp"}
            ],
        )
        result = run_tool(root, "--policy", str(policy_path), "--check-policy", "--format", "json")
        assert result.returncode == 4
        assert any(
            item["code"] == "exception_mismatch"
            for item in json.loads(result.stdout)["policy"]["violations"]
        )

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        policy_path = write_policy(
            root,
            ratchets={
                "max_direct_edges": 1,
                "departments": {
                    department: {
                        "max_direct_edge_out": 100,
                        "max_unique_department_out": 100,
                    }
                    for department in DEPARTMENTS
                },
            },
        )
        result = run_tool(root, "--policy", str(policy_path), "--check-policy", "--format", "json")
        assert result.returncode == 4
        assert any(
            item["code"] == "ratchet_total"
            for item in json.loads(result.stdout)["policy"]["violations"]
        )

    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        build_graph_fixture(root)
        malformed = root / "policy.json"
        malformed.write_text("[]", encoding="utf-8")
        result = run_tool(root, "--policy", str(malformed), "--check-policy")
        assert result.returncode == 2
        assert result.stdout == ""
        result = run_tool(root, "--check-policy")
        assert result.returncode == 2


def main() -> int:
    test_graph_facts_and_diagnostics()
    test_deterministic_json_and_output_path()
    test_unreliable_scan_diagnostics()
    test_policy_checks_and_cli_errors()
    print("dependency_graph_tests: passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
