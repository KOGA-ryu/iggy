#!/usr/bin/env python3
"""Freeze and enforce the pre-implementation material blueprint boundary."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
from typing import Any


SCHEMA = "iggy3d.material_workflow_state.v1"
TIERS = {"hero-master", "reusable-family", "bounded-variation"}
DEMAND_PATTERN = re.compile(r"^##\s+(DEM-[A-Z-]+-\d+):", re.MULTILINE)
STAGE_PATTERN = re.compile(
    r"^\|\s*04\b[^|]*\|\s*(?:complete|verified-existing)\s*\|",
    re.MULTILINE,
)
FORBIDDEN_PLACEHOLDERS = (
    re.compile(r"^\s*pass\s*(?:#.*)?$", re.MULTILINE),
    re.compile(r"^\s*TODO\b", re.MULTILINE),
    re.compile(r"^\s*\.\.\.\s*$", re.MULTILINE),
    re.compile(r"Write complete executable .* here", re.IGNORECASE),
    re.compile(r'"write"\s*:\s*"complete valid JSON here"'),
)


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def file_record(path: Path) -> dict[str, Any]:
    resolved = path.resolve()
    return {
        "path": str(resolved),
        "exists": resolved.is_file(),
        "sha256": sha256(resolved) if resolved.is_file() else None,
        "bytes": resolved.stat().st_size if resolved.is_file() else None,
    }


def read_state(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ValueError(f"Workflow state does not exist: {path}")
    payload = json.loads(path.read_text())
    if payload.get("schema") != SCHEMA:
        raise ValueError("Unexpected material workflow state schema")
    return payload


def atomic_write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        dir=path.parent,
        prefix=f".{path.name}.",
        suffix=".tmp",
        delete=False,
    ) as handle:
        handle.write(json.dumps(payload, indent=2, sort_keys=True) + "\n")
        temporary = Path(handle.name)
    os.replace(temporary, path)


def validate_blueprint(
    workstream: Path,
    coded_demands: Path,
) -> tuple[list[str], str]:
    if not workstream.is_file():
        raise ValueError(f"Workstream dossier is missing: {workstream}")
    if not coded_demands.is_file():
        raise ValueError(f"Coded demands are missing: {coded_demands}")
    workstream_text = workstream.read_text()
    if STAGE_PATTERN.search(workstream_text) is None:
        raise ValueError(
            "Stage 04 must be complete or verified-existing before freeze"
        )
    coded_text = coded_demands.read_text()
    demand_ids = DEMAND_PATTERN.findall(coded_text)
    if not demand_ids:
        raise ValueError("No coded demand IDs were found")
    if len(demand_ids) != len(set(demand_ids)):
        raise ValueError("Coded demand IDs must be unique")
    if coded_text.count("### Test code") < len(demand_ids):
        raise ValueError("Every coded demand must contain Test code")
    production_headings = (
        coded_text.count("### Generator code")
        + coded_text.count("### Blender builder")
        + coded_text.count("### Production code")
    )
    if production_headings < len(demand_ids):
        raise ValueError("Every coded demand must contain production code")
    for pattern in FORBIDDEN_PLACEHOLDERS:
        match = pattern.search(coded_text)
        if match is not None:
            raise ValueError(
                "Coded demands contain a forbidden placeholder: "
                f"{match.group(0)!r}"
            )
    if coded_text.count("```") < len(demand_ids) * 4:
        raise ValueError("Coded demands do not contain enough executable blocks")
    return demand_ids, sha256(coded_demands)


def command_freeze(args: argparse.Namespace) -> int:
    if args.tier not in TIERS:
        raise ValueError(f"Unsupported workflow tier: {args.tier}")
    package = args.package.resolve()
    state_path = package / "WORKFLOW_STATE.json"
    workstream = args.workstream.resolve()
    coded_demands = args.coded_demands.resolve()
    demand_ids, coded_hash = validate_blueprint(workstream, coded_demands)
    target_records = [
        file_record(path.resolve())
        for path in args.target
    ]
    previous = (
        read_state(state_path)
        if state_path.is_file()
        else {
            "schema": SCHEMA,
            "package": str(package),
            "revisions": [],
        }
    )
    if previous["package"] != str(package):
        raise ValueError("Workflow state belongs to another package")
    revision_number = len(previous["revisions"]) + 1
    revision = {
        "revision": revision_number,
        "tier": args.tier,
        "frozen_at": utc_now(),
        "revision_note": args.revision_note,
        "red_gate_evidence": list(args.red_gate_evidence),
        "workstream": file_record(workstream),
        "coded_demands": file_record(coded_demands),
        "coded_demands_sha256": coded_hash,
        "demand_ids": demand_ids,
        "target_baselines": target_records,
        "build_entry": None,
    }
    previous["revisions"].append(revision)
    previous["current_revision"] = revision_number
    previous["updated_at"] = utc_now()
    atomic_write_json(state_path, previous)
    print(
        json.dumps(
            {
                "status": "frozen",
                "revision": revision_number,
                "tier": args.tier,
                "demand_ids": demand_ids,
                "target_count": len(target_records),
                "state": str(state_path),
            },
            sort_keys=True,
        )
    )
    return 0


def current_revision(state: dict[str, Any]) -> dict[str, Any]:
    number = int(state["current_revision"])
    for revision in state["revisions"]:
        if int(revision["revision"]) == number:
            return revision
    raise ValueError("Current workflow revision is missing")


def command_open_build(args: argparse.Namespace) -> int:
    state_path = args.package.resolve() / "WORKFLOW_STATE.json"
    state = read_state(state_path)
    revision = current_revision(state)
    coded_path = Path(revision["coded_demands"]["path"])
    if not coded_path.is_file():
        raise ValueError("Frozen coded-demand document is missing")
    actual_coded_hash = sha256(coded_path)
    if actual_coded_hash != revision["coded_demands_sha256"]:
        raise ValueError(
            "Coded demands changed after freeze; review and freeze a revision"
        )
    changed_targets = []
    for baseline in revision["target_baselines"]:
        path = Path(baseline["path"])
        current = file_record(path)
        if (
            current["exists"] != baseline["exists"]
            or current["sha256"] != baseline["sha256"]
        ):
            changed_targets.append(str(path))
    if changed_targets:
        raise ValueError(
            "Production targets changed before build entry opened: "
            + ", ".join(changed_targets)
        )
    if revision["build_entry"] is not None:
        raise ValueError("Build entry is already open for this revision")
    revision["build_entry"] = {
        "opened_at": utc_now(),
        "coded_demands_sha256": actual_coded_hash,
        "target_baselines_verified": True,
    }
    state["updated_at"] = utc_now()
    atomic_write_json(state_path, state)
    print(
        json.dumps(
            {
                "status": "build-entry-open",
                "revision": revision["revision"],
                "tier": revision["tier"],
                "state": str(state_path),
            },
            sort_keys=True,
        )
    )
    return 0


def command_verify_entry(args: argparse.Namespace) -> int:
    state_path = args.package.resolve() / "WORKFLOW_STATE.json"
    state = read_state(state_path)
    revision = current_revision(state)
    entry = revision.get("build_entry")
    if entry is None or entry.get("target_baselines_verified") is not True:
        raise ValueError("No verified open build entry exists")
    coded_path = Path(revision["coded_demands"]["path"])
    if not coded_path.is_file():
        raise ValueError("Frozen coded-demand document is missing")
    actual_coded_hash = sha256(coded_path)
    if (
        actual_coded_hash != revision["coded_demands_sha256"]
        or actual_coded_hash != entry.get("coded_demands_sha256")
    ):
        raise ValueError(
            "Coded demands changed after build entry; freeze a revision"
        )
    print(
        json.dumps(
            {
                "status": "verified",
                "revision": revision["revision"],
                "tier": revision["tier"],
                "opened_at": entry["opened_at"],
            },
            sort_keys=True,
        )
    )
    return 0


def command_status(args: argparse.Namespace) -> int:
    state_path = args.package.resolve() / "WORKFLOW_STATE.json"
    state = read_state(state_path)
    revision = current_revision(state)
    print(
        json.dumps(
            {
                "schema": state["schema"],
                "package": state["package"],
                "revision_count": len(state["revisions"]),
                "current_revision": revision,
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    freeze = subparsers.add_parser("freeze")
    freeze.add_argument("--package", type=Path, required=True)
    freeze.add_argument("--workstream", type=Path, required=True)
    freeze.add_argument("--coded-demands", type=Path, required=True)
    freeze.add_argument("--tier", choices=sorted(TIERS), required=True)
    freeze.add_argument("--target", type=Path, action="append", default=[])
    freeze.add_argument("--red-gate-evidence", action="append", default=[])
    freeze.add_argument("--revision-note", required=True)
    freeze.set_defaults(handler=command_freeze)

    open_build = subparsers.add_parser("open-build")
    open_build.add_argument("--package", type=Path, required=True)
    open_build.set_defaults(handler=command_open_build)

    verify = subparsers.add_parser("verify-entry")
    verify.add_argument("--package", type=Path, required=True)
    verify.set_defaults(handler=command_verify_entry)

    status = subparsers.add_parser("status")
    status.add_argument("--package", type=Path, required=True)
    status.set_defaults(handler=command_status)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        return int(args.handler(args))
    except ValueError as error:
        raise SystemExit(f"material-workflow-gate: {error}")


if __name__ == "__main__":
    raise SystemExit(main())
