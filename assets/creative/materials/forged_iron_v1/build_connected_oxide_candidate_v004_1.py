#!/usr/bin/env python3
"""Integrate the selected C calibration into a reopenable v004.1 candidate."""

from __future__ import annotations

import argparse
from dataclasses import asdict
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any

import bpy
import numpy as np


MATERIAL_ROOT = Path(__file__).resolve().parent
CALIBRATION_SCRIPT = MATERIAL_ROOT / "compare_v004_optical_response_calibration.py"
CALIBRATION_ROOT = MATERIAL_ROOT / "output" / "forged_iron_v004_optical_calibration_v1"
CALIBRATION_MANIFEST = CALIBRATION_ROOT / "manifest.json"
SOURCE_V004 = (
    MATERIAL_ROOT
    / "output"
    / "forged_iron_connected_oxide_candidate_v004"
    / "forged_iron_connected_oxide_candidate_v004.blend"
)
SOURCE_V003 = MATERIAL_ROOT / "output" / "forged_iron_v1.blend"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_1"
CANDIDATE_BLEND = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004_1.blend"
MANIFEST_PATH = OUTPUT_ROOT / "manifest.json"
GROUP_NAME = "IGGY_SH_ConnectedOxideForgedIron_v004"
MATERIAL_PREFIX = "IGGY_MAT_ConnectedOxideForgedIron_v004_1"
SELECTED_RECIPE_ID = "C_audited_compressed_cool"

# DEM-PRODUCTION-010: exact selected-C integration envelope. These values are
# intentionally repeated here instead of inferred from the manifest so drift
# in either source fails closed before the saved candidate is written.
THIN_HEX = "#252a31"
THICK_HEX = "#353b46"
ROUGHNESS_BASE = 0.680
THERMAL_AMPLITUDE = 0.035
MEDIUM_AMPLITUDE = 0.055
GRAIN_AMPLITUDE = 0.012
ROUGHNESS_MINIMUM = 0.600
ROUGHNESS_MAXIMUM = 0.760
BROAD_NORMAL_MULTIPLIER = 1.75

LOCKED_ABSENCES = (
    "rust",
    "condition",
    "contact_polish",
    "soot",
    "dirt",
    "blood",
    "worked_luster",
    "oxide_height",
    "exposed_conductor",
    "cat_face_motifs",
    "diamond_stamps",
)


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


calibration = _load_module("iggy_v004_1_calibration", CALIBRATION_SCRIPT)
builder = calibration.builder
base = calibration.base


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=OUTPUT_ROOT)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def selected_recipe():
    recipe = next(
        item for item in calibration.CANDIDATES
        if item.candidate_id == SELECTED_RECIPE_ID
    )
    expected = {
        "candidate_id": SELECTED_RECIPE_ID,
        "thin_hex": THIN_HEX,
        "thick_hex": THICK_HEX,
        "roughness_base": ROUGHNESS_BASE,
        "thermal_amplitude": THERMAL_AMPLITUDE,
        "medium_amplitude": MEDIUM_AMPLITUDE,
        "grain_amplitude": GRAIN_AMPLITUDE,
        "roughness_minimum": ROUGHNESS_MINIMUM,
        "roughness_maximum": ROUGHNESS_MAXIMUM,
        "broad_normal_multiplier": BROAD_NORMAL_MULTIPLIER,
    }
    actual = asdict(recipe)
    for key, value in expected.items():
        if actual[key] != value:
            raise RuntimeError(
                f"Selected-C integration drifted at {key}: {actual[key]!r} != {value!r}"
            )
    return recipe


def scalar_range(value: np.ndarray) -> list[float]:
    return [float(value.min()), float(value.max())]


def shared_group_signature(group: bpy.types.NodeTree) -> dict[str, Any]:
    nodes = sorted((node.name, node.bl_idname) for node in group.nodes)
    links = sorted(
        (
            link.from_node.name,
            link.from_socket.name,
            link.to_node.name,
            link.to_socket.name,
        )
        for link in group.links
    )
    sockets = sorted(
        (item.name, item.in_out, item.socket_type)
        for item in group.interface.items_tree
        if getattr(item, "item_type", None) == "SOCKET"
    )
    payload = json.dumps(
        {"nodes": nodes, "links": links, "sockets": sockets},
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return {
        "name": group.name,
        "node_count": len(nodes),
        "link_count": len(links),
        "interface_socket_count": len(sockets),
        "topology_sha256": hashlib.sha256(payload).hexdigest(),
    }


def build_images(
    component_name: str,
    fields: dict[str, np.ndarray],
) -> dict[str, bpy.types.Image]:
    slug = component_name.replace(".", "_")
    prefix = f"IGGY_IMG_ConnectedOxide_v004_1_{slug}"
    return {
        "oxide_base": base.make_float_image(
            f"{prefix}_OxideBase",
            fields["oxide_base_linear"],
            color=True,
        ),
        "oxide_roughness": base.make_float_image(
            f"{prefix}_OxideRoughness",
            fields["oxide_roughness"],
            color=False,
        ),
        "combined_normal": base.make_float_image(
            f"{prefix}_BroadForgingNormal",
            fields["combined_normal"] * 0.5 + 0.5,
            color=True,
        ),
        "luster_control": base.make_float_image(
            f"{prefix}_LusterControl",
            fields["luster_control"],
            color=True,
        ),
    }


def reopen_candidate_contract(candidate_blend: Path) -> dict[str, Any]:
    contract = builder.reopen_candidate_contract(candidate_blend)
    group = bpy.data.node_groups.get(GROUP_NAME)
    if group is None:
        raise RuntimeError("Reopened v004.1 lost the unchanged v004 shared group")
    unique_materials: set[str] = set()
    for name in base.TARGET_OBJECTS:
        obj = bpy.data.objects[name]
        material = obj.data.materials[0]
        if not material.name.startswith(MATERIAL_PREFIX):
            raise RuntimeError(f"Reopened {name} has the wrong material: {material.name}")
        unique_materials.add(material.name)
        images = [
            node.image
            for node in material.node_tree.nodes
            if node.bl_idname == "ShaderNodeTexImage"
        ]
        if len(images) != 4 or not all(image and image.packed_file for image in images):
            raise RuntimeError(f"Reopened {name} lost a packed component field")
    if len(unique_materials) != len(base.TARGET_OBJECTS):
        raise RuntimeError("v004.1 does not own one unique material per component")
    contract["shared_group_signature"] = shared_group_signature(group)
    contract["unique_material_count"] = len(unique_materials)
    return contract


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    candidate_blend = output_root / CANDIDATE_BLEND.name
    manifest_path = output_root / MANIFEST_PATH.name
    for required in (SOURCE_V004, SOURCE_V003, CALIBRATION_MANIFEST):
        if not required.is_file():
            raise FileNotFoundError(required)

    recipe = selected_recipe()
    source_v004_before = sha256_file(SOURCE_V004)
    source_v003_before = sha256_file(SOURCE_V003)
    calibration_manifest_hash = sha256_file(CALIBRATION_MANIFEST)
    build_script_hash = sha256_file(Path(__file__).resolve())
    calibration_script_hash = sha256_file(CALIBRATION_SCRIPT)

    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_V004))
    shared_group = bpy.data.node_groups.get(GROUP_NAME)
    if shared_group is None:
        raise RuntimeError("Source v004 is missing its shared physical-response group")
    shared_before = shared_group_signature(shared_group)
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    frames = {obj.name: base.install_component_frames(obj) for obj in targets}

    component_manifest: dict[str, Any] = {}
    for obj in targets:
        fields = calibration.build_calibrated_fields(frames[obj.name], obj.name, recipe)
        images = build_images(obj.name, fields)
        material, material_counts = builder.build_component_material(
            obj,
            shared_group,
            images,
        )
        material.name = f"{MATERIAL_PREFIX}__{obj.name}"
        material["iggy_status"] = "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
        material["iggy_selected_recipe"] = SELECTED_RECIPE_ID
        base.set_material(obj, material)
        component_manifest[obj.name] = {
            "material": material.name,
            "images": {key: image.name for key, image in images.items()},
            "all_images_packed": all(
                image.packed_file is not None for image in images.values()
            ),
            "material_node_counts": material_counts,
            "field_resolution": list(fields["oxide_roughness"].shape[::-1]),
            "coverage_range": scalar_range(fields["oxide_coverage"]),
            "metalness_maximum": float(fields["oxide_metalness"].max()),
            "exposure_maximum": float(fields["exposure"].max()),
            "oxide_height_absolute_maximum_m": float(
                np.abs(fields["oxide_surface_height_m"]).max()
            ),
            "roughness_range": scalar_range(fields["oxide_roughness"]),
            "luster_amount_range": scalar_range(fields["luster_control"][..., 0]),
            "broad_forging_height_range_m": scalar_range(
                fields["broad_forging_height_m"]
            ),
        }

    shared_after_fields = shared_group_signature(shared_group)
    if shared_after_fields != shared_before:
        raise RuntimeError("v004.1 field integration changed shared-group topology")
    scene = bpy.context.scene
    scene["iggy_material_candidate"] = "forged_iron_connected_oxide_candidate_v004_1"
    scene["iggy_status"] = "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
    scene["iggy_selected_recipe"] = SELECTED_RECIPE_ID
    scene["iggy_manual_acceptance_established"] = False
    bpy.ops.wm.save_as_mainfile(filepath=str(candidate_blend), check_existing=False)
    candidate_hash = sha256_file(candidate_blend)
    reopen_contract = reopen_candidate_contract(candidate_blend)
    if reopen_contract["shared_group_signature"] != shared_before:
        raise RuntimeError("Reopened v004.1 changed shared-group topology")

    source_v004_after = sha256_file(SOURCE_V004)
    source_v003_after = sha256_file(SOURCE_V003)
    source_v004_unchanged = source_v004_before == source_v004_after
    source_v003_unchanged = source_v003_before == source_v003_after
    if not source_v004_unchanged or not source_v003_unchanged:
        raise RuntimeError("v004.1 build changed a frozen source blend")

    manifest = {
        "schema": "iggy3d.forged_iron_connected_oxide_candidate.v004_1",
        "status": "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE",
        "candidate": {
            "id": "forged_iron_connected_oxide_candidate_v004_1",
            "blend": str(candidate_blend),
            "blend_sha256": candidate_hash,
            "selected_recipe": SELECTED_RECIPE_ID,
            "shared_group": GROUP_NAME,
            "material_prefix": MATERIAL_PREFIX,
        },
        "integrated_recipe": asdict(recipe),
        "source_v004": {
            "path": str(SOURCE_V004),
            "sha256_before": source_v004_before,
            "sha256_after": source_v004_after,
        },
        "source_v003": {
            "path": str(SOURCE_V003),
            "sha256_before": source_v003_before,
            "sha256_after": source_v003_after,
        },
        "calibration": {
            "manifest": str(CALIBRATION_MANIFEST),
            "manifest_sha256": calibration_manifest_hash,
            "script": str(CALIBRATION_SCRIPT),
            "script_sha256": calibration_script_hash,
        },
        "build_script": {
            "path": str(Path(__file__).resolve()),
            "sha256": build_script_hash,
        },
        "consumer": {
            "collection": base.TARGET_COLLECTION,
            "objects": list(base.TARGET_OBJECTS),
        },
        "shared_group_contract": shared_before,
        "components": component_manifest,
        "reopen_contract": reopen_contract,
        "source_v004_unchanged": source_v004_unchanged,
        "source_v003_unchanged": source_v003_unchanged,
        "locked_absences": list(LOCKED_ABSENCES),
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "promotion_boundary": (
            "v004.1 may advance only to paired Cycles proof against saved v004; "
            "it may not replace the frozen source or register as a donor."
        ),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(
        "FORGED_IRON_V004_1_CANDIDATE="
        f"components:{len(component_manifest)},"
        f"shared_users:{reopen_contract['shared_group_users']},"
        f"sources_unchanged:{source_v004_unchanged and source_v003_unchanged}"
    )
    print(f"FORGED_IRON_V004_1_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
