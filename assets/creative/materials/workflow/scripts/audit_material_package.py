#!/usr/bin/env python3
"""Audit a final material package for quality-contract and packaging drift."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from types import SimpleNamespace
from typing import Any, Iterable


ALLOWED_CLAIM_STATUS = {"measured", "proxy", "authored", "inherited"}
TIERS_REQUIRING_REFERENCE_DELTA = {"hero-master", "reusable-family"}
WORKFLOW_STATE_SCHEMA = "iggy3d.material_workflow_state.v1"


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def json_pointer(document: Any, pointer: str) -> Any:
    if pointer == "":
        return document
    if not pointer.startswith("/"):
        raise ValueError(f"JSON pointer must begin with '/': {pointer}")
    current = document
    for raw in pointer[1:].split("/"):
        token = raw.replace("~1", "/").replace("~0", "~")
        if isinstance(current, list):
            current = current[int(token)]
        elif isinstance(current, dict):
            current = current[token]
        else:
            raise ValueError(f"Pointer traverses a scalar: {pointer}")
    return current


def numeric_leaf_pointers(value: Any, pointer: str = "") -> list[str]:
    if isinstance(value, bool):
        return []
    if isinstance(value, (int, float)):
        return [pointer]
    if isinstance(value, list):
        result: list[str] = []
        for index, child in enumerate(value):
            result.extend(
                numeric_leaf_pointers(child, f"{pointer}/{index}")
            )
        return result
    if isinstance(value, dict):
        result = []
        for key, child in value.items():
            escaped = str(key).replace("~", "~0").replace("/", "~1")
            result.extend(
                numeric_leaf_pointers(child, f"{pointer}/{escaped}")
            )
        return result
    return []


def claim_covers(claim_pointer: str, leaf_pointer: str) -> bool:
    return (
        leaf_pointer == claim_pointer
        or leaf_pointer.startswith(claim_pointer.rstrip("/") + "/")
    )


def check_measurement_claims(
    contract: dict[str, Any],
    documents: dict[str, dict[str, Any]],
) -> tuple[bool, dict[str, Any]]:
    claims = contract.get("measurement_claims", [])
    claim_roots = contract.get("claim_roots", [])
    errors: list[str] = []
    seen_ids: set[str] = set()
    for claim in claims:
        claim_id = claim.get("id")
        if not isinstance(claim_id, str) or claim_id == "":
            errors.append("Measurement claim has no stable id")
            continue
        if claim_id in seen_ids:
            errors.append(f"Duplicate measurement claim id: {claim_id}")
        seen_ids.add(claim_id)
        document_name = claim.get("document")
        pointer = claim.get("pointer")
        status = claim.get("status")
        if document_name not in documents:
            errors.append(f"{claim_id}: unknown document {document_name!r}")
            continue
        if not isinstance(pointer, str):
            errors.append(f"{claim_id}: missing JSON pointer")
            continue
        if status not in ALLOWED_CLAIM_STATUS:
            errors.append(f"{claim_id}: unsupported status {status!r}")
        try:
            actual = json_pointer(documents[document_name], pointer)
        except (KeyError, IndexError, ValueError) as error:
            errors.append(f"{claim_id}: {error}")
            continue
        if "value" in claim and actual != claim["value"]:
            errors.append(
                f"{claim_id}: declared value {claim['value']!r} "
                f"does not match {actual!r}"
            )
        if status in {"measured", "proxy", "inherited"} and not claim.get(
            "source_id"
        ):
            errors.append(f"{claim_id}: {status} claim has no source_id")
        if status == "authored" and not claim.get("reason"):
            errors.append(f"{claim_id}: authored claim has no reason")
    for root in claim_roots:
        document_name = root.get("document")
        root_pointer = root.get("pointer")
        if document_name not in documents or not isinstance(root_pointer, str):
            errors.append(f"Invalid claim root: {root!r}")
            continue
        try:
            value = json_pointer(documents[document_name], root_pointer)
        except (KeyError, IndexError, ValueError) as error:
            errors.append(f"Claim root {root_pointer}: {error}")
            continue
        leaves = numeric_leaf_pointers(value, root_pointer)
        document_claims = [
            claim["pointer"]
            for claim in claims
            if claim.get("document") == document_name
            and isinstance(claim.get("pointer"), str)
        ]
        for leaf in leaves:
            if not any(
                claim_covers(pointer, leaf) for pointer in document_claims
            ):
                errors.append(
                    f"Unsupported numeric claim: {document_name}:{leaf}"
                )
    return not errors, {
        "claim_count": len(claims),
        "root_count": len(claim_roots),
        "errors": errors,
    }


def unique_image_count(nodes: Iterable[dict[str, Any]]) -> int:
    return len(
        {
            node.get("image")
            for node in nodes
            if node.get("bl_idname") == "ShaderNodeTexImage"
            and node.get("image") is not None
        }
    )


def check_performance(
    contract: dict[str, Any],
    texture_manifest: dict[str, Any],
    blender_manifest: dict[str, Any],
) -> tuple[bool, dict[str, Any]]:
    budget = contract.get("performance_budget", {})
    geometry = blender_manifest["geometry"]
    nodes = blender_manifest["shader"]["nodes"]
    actual = {
        "total_texture_bytes": sum(
            int(entry["bytes"])
            for entry in texture_manifest["files"].values()
        ),
        "product_objects": int(geometry["object_count"]),
        "total_vertices": (
            int(geometry["object_count"])
            * int(geometry["vertices_per_object"])
        ),
        "total_polygons": (
            int(geometry["object_count"])
            * int(geometry["polygons_per_object"])
        ),
        "shader_nodes": int(blender_manifest["shader"]["node_count"]),
        "shader_links": int(blender_manifest["shader"]["link_count"]),
        "unique_images": unique_image_count(nodes),
        "proof_count": len(blender_manifest["proofs"]),
    }
    errors = []
    for key, value in actual.items():
        maximum_key = f"max_{key}"
        if maximum_key not in budget:
            errors.append(f"Performance budget omits {maximum_key}")
            continue
        maximum = int(budget[maximum_key])
        if value > maximum:
            errors.append(f"{key}={value} exceeds {maximum}")
    return not errors, {
        "actual": actual,
        "budget": budget,
        "errors": errors,
    }


def check_layer_provenance(
    contract: dict[str, Any],
    blender_manifest: dict[str, Any],
) -> tuple[bool, dict[str, Any]]:
    required = {
        "construction",
        "macro",
        "medium",
        "edge_event",
        "micro",
        "cumulative_color",
        "height_normal",
        "response",
        "stylization",
    }
    entries = contract.get("layer_provenance", [])
    errors = []
    if not isinstance(entries, list):
        return False, {"errors": ["layer_provenance must be a list"]}
    entry_ids = [
        entry.get("id")
        for entry in entries
        if isinstance(entry, dict)
    ]
    valid_ids = {value for value in entry_ids if isinstance(value, str)}
    if len(entry_ids) != len(valid_ids):
        errors.append("Layer provenance IDs are missing or duplicated")
    if contract.get("tier") in {"hero-master", "reusable-family"}:
        missing = sorted(required - valid_ids)
        if missing:
            errors.append(
                "Required layer provenance is missing: " + ", ".join(missing)
            )
    claim_ids = {
        claim.get("id")
        for claim in contract.get("measurement_claims", [])
        if isinstance(claim, dict)
    }
    available_proofs = set(blender_manifest.get("proofs", {}))
    required_fields = (
        "physical_meaning",
        "sources",
        "outputs",
        "consumers",
        "proof_ids",
        "rest_rule",
        "claim_ids",
    )
    for entry in entries:
        if not isinstance(entry, dict):
            errors.append("Layer provenance entry is not an object")
            continue
        layer_id = entry.get("id", "<missing>")
        for field in required_fields:
            value = entry.get(field)
            if field in {
                "sources",
                "outputs",
                "consumers",
                "proof_ids",
                "claim_ids",
            }:
                if not isinstance(value, list):
                    errors.append(f"{layer_id}: {field} must be a list")
                elif field not in {"claim_ids"} and not value:
                    errors.append(f"{layer_id}: {field} cannot be empty")
            elif not isinstance(value, str) or not value.strip():
                errors.append(f"{layer_id}: {field} cannot be empty")
        unknown_claims = sorted(
            claim_id
            for claim_id in entry.get("claim_ids", [])
            if claim_id not in claim_ids
        )
        if unknown_claims:
            errors.append(
                f"{layer_id}: unknown claim IDs: "
                + ", ".join(unknown_claims)
            )
        unknown_proofs = sorted(
            proof_id
            for proof_id in entry.get("proof_ids", [])
            if proof_id not in available_proofs
        )
        if unknown_proofs:
            errors.append(
                f"{layer_id}: unknown proof IDs: "
                + ", ".join(unknown_proofs)
            )
    return not errors, {
        "required_layer_ids": sorted(required),
        "declared_layer_ids": sorted(valid_ids),
        "errors": errors,
    }


def check_surface_method_contract(
    contract: dict[str, Any],
    blender_manifest: dict[str, Any],
) -> tuple[bool, dict[str, Any]]:
    method = contract.get("surface_method_contract")
    if not isinstance(method, dict):
        return False, {
            "declared": False,
            "errors": ["surface_method_contract must be an object"],
        }
    errors = []
    if method.get("version") != 1:
        errors.append("surface_method_contract version must be 1")
    effects = method.get("effect_stack")
    if not isinstance(effects, list) or not effects:
        errors.append("effect_stack must be a non-empty list")
        effects = []
    ids = [
        effect.get("id")
        for effect in effects
        if isinstance(effect, dict)
    ]
    valid_ids = {value for value in ids if isinstance(value, str) and value}
    if len(ids) != len(valid_ids):
        errors.append("Effect IDs are missing or duplicated")
    orders = [
        effect.get("order")
        for effect in effects
        if isinstance(effect, dict)
    ]
    if any(
        isinstance(order, bool) or not isinstance(order, int)
        for order in orders
    ):
        errors.append("Every effect order must be an integer")
    elif len(orders) != len(set(orders)):
        errors.append("Effect orders must be unique")
    elif orders != sorted(orders):
        errors.append("Effect stack must be recorded in execution order")
    available_proofs = set(blender_manifest.get("proofs", {}))
    text_fields = (
        "physical_role",
        "operation",
        "coordinate_frame",
        "quiet_rule",
    )
    list_fields = (
        "inputs",
        "mask_sources",
        "outputs",
        "consumers",
        "proof_ids",
    )
    for effect in effects:
        if not isinstance(effect, dict):
            errors.append("Effect stack entry is not an object")
            continue
        effect_id = effect.get("id", "<missing>")
        for field in text_fields:
            value = effect.get(field)
            if not isinstance(value, str) or not value.strip():
                errors.append(f"{effect_id}: {field} cannot be empty")
        for field in list_fields:
            value = effect.get(field)
            if not isinstance(value, list) or not value:
                errors.append(f"{effect_id}: {field} must be a non-empty list")
        unknown_proofs = sorted(
            proof_id
            for proof_id in effect.get("proof_ids", [])
            if proof_id not in available_proofs
        )
        if unknown_proofs:
            errors.append(
                f"{effect_id}: unknown proof IDs: "
                + ", ".join(unknown_proofs)
            )
    transfers = method.get("geometry_to_map_transfer")
    if not isinstance(transfers, list):
        errors.append("geometry_to_map_transfer must be a list")
        transfers = []
    for index, transfer in enumerate(transfers):
        if not isinstance(transfer, dict):
            errors.append(f"Transfer {index} is not an object")
            continue
        transfer_id = transfer.get("id", f"transfer-{index}")
        for field in (
            "source_geometry",
            "retained_as_geometry",
            "baked_outputs",
            "boundary_reason",
            "proof_ids",
        ):
            value = transfer.get(field)
            if field in {
                "retained_as_geometry",
                "baked_outputs",
                "proof_ids",
            }:
                if not isinstance(value, list):
                    errors.append(f"{transfer_id}: {field} must be a list")
            elif not isinstance(value, str) or not value.strip():
                errors.append(f"{transfer_id}: {field} cannot be empty")
        unknown_proofs = sorted(
            proof_id
            for proof_id in transfer.get("proof_ids", [])
            if proof_id not in available_proofs
        )
        if unknown_proofs:
            errors.append(
                f"{transfer_id}: unknown proof IDs: "
                + ", ".join(unknown_proofs)
            )
    damage = method.get("damage_placement")
    if not isinstance(damage, dict) or not isinstance(
        damage.get("enabled"), bool
    ):
        errors.append("damage_placement must declare boolean enabled")
        damage = {"enabled": False}
    if damage["enabled"]:
        for field in (
            "eligible_semantics",
            "protected_semantics",
            "event_families",
            "density_or_count_bounds",
            "proof_ids",
        ):
            value = damage.get(field)
            if not isinstance(value, list) or not value:
                errors.append(
                    f"damage_placement: {field} must be a non-empty list"
                )
        quiet_rule = damage.get("quiet_rule")
        if not isinstance(quiet_rule, str) or not quiet_rule.strip():
            errors.append("damage_placement: quiet_rule cannot be empty")
        unknown_proofs = sorted(
            proof_id
            for proof_id in damage.get("proof_ids", [])
            if proof_id not in available_proofs
        )
        if unknown_proofs:
            errors.append(
                "damage_placement: unknown proof IDs: "
                + ", ".join(unknown_proofs)
            )
    return not errors, {
        "declared": True,
        "version": method.get("version"),
        "effect_ids": sorted(valid_ids),
        "effect_count": len(effects),
        "geometry_transfer_count": len(transfers),
        "damage_enabled": damage["enabled"],
        "errors": errors,
    }


def check_output_inventory(
    output_root: Path,
    texture_manifest_path: Path,
    texture_manifest: dict[str, Any],
    blender_manifest_path: Path,
    blender_manifest: dict[str, Any],
) -> tuple[bool, dict[str, Any]]:
    expected = {
        texture_manifest_path.resolve(),
        blender_manifest_path.resolve(),
        (output_root / blender_manifest["blend"]["filename"]).resolve(),
    }
    for entry in texture_manifest["files"].values():
        expected.add((output_root / entry["filename"]).resolve())
    for entry in blender_manifest["proofs"].values():
        expected.add((output_root / "proofs" / entry["filename"]).resolve())
    actual = {
        path.resolve()
        for path in output_root.rglob("*")
        if path.is_file()
    }
    missing = sorted(str(path) for path in expected - actual)
    unexpected = sorted(str(path) for path in actual - expected)
    return not missing and not unexpected, {
        "expected_count": len(expected),
        "actual_count": len(actual),
        "missing": missing,
        "unexpected": unexpected,
    }


def check_routes(
    contract: dict[str, Any],
    blender_manifest: dict[str, Any],
) -> tuple[bool, dict[str, Any]]:
    nodes = blender_manifest["shader"]["nodes"]
    links = blender_manifest["shader"]["links"]
    outgoing = {
        link["from_node"]
        for link in links
    }
    dormant_images = sorted(
        node["name"]
        for node in nodes
        if node["bl_idname"] == "ShaderNodeTexImage"
        and node["name"] not in outgoing
    )
    dormant_attribute_nodes = sorted(
        node["name"]
        for node in nodes
        if node["bl_idname"] == "ShaderNodeAttribute"
        and node["name"] not in outgoing
    )
    present = set(blender_manifest["attributes"]["present"])
    consumed = set(blender_manifest["shader"]["attribute_nodes"])
    storage_only = set(contract.get("storage_only_attributes", []))
    unowned_geometry_attributes = sorted(present - consumed - storage_only)
    unknown_storage = sorted(storage_only - present)
    errors = []
    if dormant_images:
        errors.append("Dormant texture nodes: " + ", ".join(dormant_images))
    if dormant_attribute_nodes:
        errors.append(
            "Dormant Attribute nodes: " + ", ".join(dormant_attribute_nodes)
        )
    if unowned_geometry_attributes:
        errors.append(
            "Unconsumed geometry attributes: "
            + ", ".join(unowned_geometry_attributes)
        )
    if unknown_storage:
        errors.append(
            "Declared storage-only attributes are absent: "
            + ", ".join(unknown_storage)
        )
    return not errors, {
        "dormant_image_nodes": dormant_images,
        "dormant_attribute_nodes": dormant_attribute_nodes,
        "unowned_geometry_attributes": unowned_geometry_attributes,
        "unknown_storage_only_attributes": unknown_storage,
        "errors": errors,
    }


def load_renderer(path: Path) -> Any:
    spec = importlib.util.spec_from_file_location(
        "material_coded_demand_renderer",
        path,
    )
    if spec is None or spec.loader is None:
        raise ValueError(f"Cannot load coded-demand renderer: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_document_drift(args: argparse.Namespace) -> tuple[bool, dict[str, Any]]:
    renderer = load_renderer(args.renderer)
    render_args = SimpleNamespace(
        package=args.package,
        profile=args.profile,
        pattern=args.pattern,
        texture_manifest=args.texture_manifest,
        blender_manifest=args.blender_manifest,
        generator=args.generator,
        builder=args.builder,
        generator_test=args.generator_test,
        blend_test=args.blend_test,
        output=args.coded_demands,
    )
    expected = renderer.render(render_args)
    actual = args.coded_demands.read_text()
    return actual == expected, {
        "coded_demands": str(args.coded_demands),
        "actual_sha256": sha256(args.coded_demands),
        "expected_sha256": hashlib.sha256(expected.encode()).hexdigest(),
    }


def check_hashes(
    output_root: Path,
    texture_manifest: dict[str, Any],
    blender_manifest: dict[str, Any],
) -> tuple[bool, dict[str, Any]]:
    errors = []
    for lane, entry in texture_manifest["files"].items():
        path = output_root / entry["filename"]
        if not path.is_file() or sha256(path) != entry["sha256"]:
            errors.append(f"Texture hash mismatch: {lane}")
    blend_path = output_root / blender_manifest["blend"]["filename"]
    if (
        not blend_path.is_file()
        or sha256(blend_path) != blender_manifest["blend"]["sha256"]
    ):
        errors.append("Blend hash mismatch")
    for proof_id, entry in blender_manifest["proofs"].items():
        path = output_root / "proofs" / entry["filename"]
        if not path.is_file() or sha256(path) != entry["sha256"]:
            errors.append(f"Proof hash mismatch: {proof_id}")
    return not errors, {"errors": errors}


def check_workflow_state(
    workflow_state_path: Path,
    package: Path,
    contract: dict[str, Any],
    coded_demands: Path,
    required_targets: list[Path],
) -> tuple[bool, dict[str, Any]]:
    errors = []
    if not workflow_state_path.is_file():
        return False, {
            "workflow_state": str(workflow_state_path),
            "errors": ["WORKFLOW_STATE.json is missing"],
        }
    state = read_json(workflow_state_path)
    if state.get("schema") != WORKFLOW_STATE_SCHEMA:
        errors.append("Unexpected workflow-state schema")
    if state.get("package") != str(package.resolve()):
        errors.append("Workflow state belongs to another package")
    revisions = state.get("revisions", [])
    current_number = state.get("current_revision")
    current = next(
        (
            revision
            for revision in revisions
            if revision.get("revision") == current_number
        ),
        None,
    )
    if current is None:
        errors.append("Current workflow revision is missing")
        return False, {
            "workflow_state": str(workflow_state_path),
            "errors": errors,
        }
    if current.get("tier") != contract.get("tier"):
        errors.append("Workflow-state tier does not match profile")
    frozen_coded = current.get("coded_demands", {})
    if frozen_coded.get("path") != str(coded_demands.resolve()):
        errors.append("Workflow state names another coded-demand document")
    actual_coded_hash = sha256(coded_demands)
    if current.get("coded_demands_sha256") != actual_coded_hash:
        errors.append("Coded demands changed after blueprint freeze")
    entry = current.get("build_entry")
    if not isinstance(entry, dict) or not entry.get(
        "target_baselines_verified"
    ):
        errors.append("No verified open build entry exists")
    elif entry.get("coded_demands_sha256") != actual_coded_hash:
        errors.append("Open build entry does not match frozen coded demands")
    declared_targets = {
        Path(record["path"]).resolve()
        for record in current.get("target_baselines", [])
        if isinstance(record, dict) and isinstance(record.get("path"), str)
    }
    omitted_targets = sorted(
        str(path.resolve())
        for path in required_targets
        if path.resolve() not in declared_targets
    )
    if omitted_targets:
        errors.append(
            "Production targets omitted from frozen baseline: "
            + ", ".join(omitted_targets)
        )
    return not errors, {
        "workflow_state": str(workflow_state_path),
        "revision": current.get("revision"),
        "tier": current.get("tier"),
        "declared_target_count": len(declared_targets),
        "required_targets": [str(path.resolve()) for path in required_targets],
        "omitted_targets": omitted_targets,
        "errors": errors,
    }


def check_acceptance(
    contract: dict[str, Any],
    state: str,
    actual_target_proofs: list[Path],
    manual_review_records: list[Path],
    reference_delta: Path,
) -> tuple[bool, dict[str, Any]]:
    tier = contract.get("tier")
    errors = []
    if tier in TIERS_REQUIRING_REFERENCE_DELTA:
        if not reference_delta.is_file():
            errors.append("REFERENCE_DELTA.md is required")
        elif "## Unresolved differences" not in reference_delta.read_text():
            errors.append(
                "REFERENCE_DELTA.md must contain Unresolved differences"
            )
    requires_target = bool(
        contract.get("acceptance", {}).get(
            "actual_target_required_for_acceptance",
            tier in TIERS_REQUIRING_REFERENCE_DELTA,
        )
    )
    target_identifier = contract.get("acceptance", {}).get(
        "actual_target_identifier"
    )
    existing_target_proofs = [
        str(path.resolve())
        for path in actual_target_proofs
        if path.is_file()
    ]
    if (
        state == "accepted"
        and requires_target
        and (
            not isinstance(target_identifier, str)
            or not target_identifier.strip()
            or target_identifier.startswith("pending-selection:")
        )
    ):
        errors.append("Accepted state requires a resolved target identifier")
    if state == "accepted" and requires_target and not existing_target_proofs:
        errors.append("Accepted state requires an actual-target proof")
    requires_review = bool(
        contract.get("acceptance", {}).get(
            "user_review_required_for_acceptance",
            tier in TIERS_REQUIRING_REFERENCE_DELTA,
        )
    )
    existing_review_records = [
        str(path.resolve())
        for path in manual_review_records
        if path.is_file()
    ]
    if state == "accepted" and requires_review and not existing_review_records:
        errors.append("Accepted state requires a manual-review record")
    return not errors, {
        "tier": tier,
        "requested_state": state,
        "actual_target_required_for_acceptance": requires_target,
        "actual_target_identifier": target_identifier,
        "actual_target_proofs": existing_target_proofs,
        "manual_review_required_for_acceptance": requires_review,
        "manual_review_records": existing_review_records,
        "errors": errors,
    }


def run_audit(args: argparse.Namespace) -> tuple[bool, dict[str, Any]]:
    profile = read_json(args.profile)
    pattern = read_json(args.pattern)
    texture_manifest = read_json(args.texture_manifest)
    blender_manifest = read_json(args.blender_manifest)
    contract = profile.get("workflow_contract")
    if not isinstance(contract, dict):
        raise ValueError("Profile has no workflow_contract")
    if contract.get("tier") not in {
        "hero-master",
        "reusable-family",
        "bounded-variation",
    }:
        raise ValueError("Profile workflow tier is missing or invalid")
    checks: dict[str, dict[str, Any]] = {}
    check_results: list[bool] = []

    def record(
        name: str,
        result: tuple[bool, dict[str, Any]],
    ) -> None:
        passed, detail = result
        checks[name] = {"passed": passed, **detail}
        check_results.append(passed)

    record(
        "workflow_state",
        check_workflow_state(
            args.workflow_state,
            args.package,
            contract,
            args.coded_demands,
            [
                args.profile,
                args.pattern,
                args.generator,
                args.builder,
                args.generator_test,
                args.blend_test,
            ],
        ),
    )
    record(
        "measurement_claims",
        check_measurement_claims(
            contract,
            {"profile": profile, "pattern": pattern},
        ),
    )
    record(
        "performance",
        check_performance(contract, texture_manifest, blender_manifest),
    )
    record(
        "layer_provenance",
        check_layer_provenance(contract, blender_manifest),
    )
    if "surface_method_contract" in contract:
        record(
            "surface_method_contract",
            check_surface_method_contract(contract, blender_manifest),
        )
    record(
        "canonical_output",
        check_output_inventory(
            args.output_root,
            args.texture_manifest,
            texture_manifest,
            args.blender_manifest,
            blender_manifest,
        ),
    )
    record("live_routes", check_routes(contract, blender_manifest))
    record("documentation_drift", check_document_drift(args))
    record(
        "artifact_hashes",
        check_hashes(args.output_root, texture_manifest, blender_manifest),
    )
    record(
        "acceptance",
        check_acceptance(
            contract,
            args.state,
            args.actual_target_proof,
            args.manual_review_record,
            args.reference_delta,
        ),
    )
    report = {
        "schema": "iggy3d.material_package_audit.v1",
        "package": str(args.package.resolve()),
        "profile_schema": profile.get("schema"),
        "pattern_schema": pattern.get("schema"),
        "requested_state": args.state,
        "passed": all(check_results),
        "checks": checks,
    }
    return all(check_results), report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--profile", type=Path, required=True)
    parser.add_argument("--pattern", type=Path, required=True)
    parser.add_argument("--texture-manifest", type=Path, required=True)
    parser.add_argument("--blender-manifest", type=Path, required=True)
    parser.add_argument("--coded-demands", type=Path, required=True)
    parser.add_argument("--workflow-state", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--renderer", type=Path, required=True)
    parser.add_argument("--generator", type=Path, required=True)
    parser.add_argument("--builder", type=Path, required=True)
    parser.add_argument("--generator-test", type=Path, required=True)
    parser.add_argument("--blend-test", type=Path, required=True)
    parser.add_argument("--reference-delta", type=Path, required=True)
    parser.add_argument(
        "--state",
        choices=("candidate", "accepted"),
        required=True,
    )
    parser.add_argument(
        "--actual-target-proof",
        type=Path,
        action="append",
        default=[],
    )
    parser.add_argument(
        "--manual-review-record",
        type=Path,
        action="append",
        default=[],
    )
    parser.add_argument("--report", type=Path)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        passed, report = run_audit(args)
    except (KeyError, TypeError, ValueError) as error:
        raise SystemExit(f"material-package-audit: {error}")
    output = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.report is not None:
        args.report.write_text(output)
    sys.stdout.write(output)
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
