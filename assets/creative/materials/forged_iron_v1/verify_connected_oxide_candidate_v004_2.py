#!/usr/bin/env python3
"""Verify saved v004.2 from a separate Blender startup process."""

from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any

import bpy


MATERIAL_ROOT = Path(__file__).resolve().parent
BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_2.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_2"
CANDIDATE_BLEND = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004_2.blend"
MANIFEST_PATH = OUTPUT_ROOT / "manifest.json"
REOPEN_CONTRACT_PATH = OUTPUT_ROOT / "reopen_contract.json"
REQUIRED_UV_LAYERS = {
    "IGGY_IronFieldUV_v001",
    "IGGY_IronTangentUV_v001",
    "IGGY_IronUV",
}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


build = _load_module("iggy_v004_2_reopen_build", BUILD_SCRIPT)
base = build.base


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def fail_unless(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def main() -> None:
    for required in (BUILD_SCRIPT, CANDIDATE_BLEND, MANIFEST_PATH, build.SOURCE_BLEND):
        if not required.is_file():
            raise FileNotFoundError(required)
    opened_path = Path(bpy.data.filepath).resolve()
    fail_unless(opened_path == CANDIDATE_BLEND.resolve(), f"Wrong startup blend: {opened_path}")

    manifest = json.loads(MANIFEST_PATH.read_text())
    candidate_hash = sha256_file(CANDIDATE_BLEND)
    source_hash = sha256_file(build.SOURCE_BLEND)
    fail_unless(
        candidate_hash == manifest["candidate"]["blend_sha256"],
        "Candidate blend changed after its build manifest was written",
    )
    fail_unless(
        source_hash == manifest["source_v004_1"]["blend_sha256_after"],
        "Saved v004.1 changed before the separate-process reopen",
    )

    source_group = bpy.data.node_groups.get(build.SOURCE_GROUP)
    target_group = bpy.data.node_groups.get(build.TARGET_GROUP)
    fail_unless(source_group is not None, "Saved candidate lost the v004 source group")
    fail_unless(target_group is not None, "Saved candidate lost the v004.2 group")
    source_signature = build.validate_group(source_group, build.SOURCE_MULTIPLIER)
    target_signature = build.validate_group(target_group, build.TARGET_MULTIPLIER)
    differences = build.group_default_differences(source_group, target_group)
    fail_unless(
        differences == manifest["group_default_differences"],
        f"Saved group default delta drifted: {differences}",
    )

    unique_materials: set[str] = set()
    candidate_group_users = 0
    components: dict[str, Any] = {}
    for name in base.TARGET_OBJECTS:
        obj = bpy.data.objects.get(name)
        fail_unless(obj is not None, f"Saved candidate lost {name}")
        material = obj.data.materials[0]
        fail_unless(
            material.name.startswith(build.TARGET_MATERIAL_PREFIX),
            f"{name} has the wrong material {material.name}",
        )
        unique_materials.add(material.name)
        group_instances = [
            node
            for node in material.node_tree.nodes
            if node.bl_idname == "ShaderNodeGroup" and node.node_tree == target_group
        ]
        fail_unless(len(group_instances) == 1, f"{name} lost its one v004.2 group owner")
        candidate_group_users += 1
        contract = build.component_contract(obj)
        recorded = manifest["components"][name]
        fail_unless(contract["mesh_sha256"] == recorded["mesh_sha256"], f"{name} mesh drift")
        fail_unless(
            contract["image_pixel_sha256"] == recorded["image_pixel_sha256"],
            f"{name} packed image pixels drifted",
        )
        fail_unless(contract["all_images_packed"], f"{name} lost a packed image")
        fail_unless(
            REQUIRED_UV_LAYERS.issubset(contract["uv_layers"]),
            f"{name} lost a construction UV layer",
        )
        fail_unless(
            contract["luster_control_resolution"] == [4, 4],
            f"{name} luster control is not 4x4",
        )
        fail_unless(
            contract["luster_amount_range"] == [0.28, 0.28]
            and contract["luster_rotation_range"] == [0.5, 0.5]
            and contract["luster_rest_range"] == [1.0, 1.0],
            f"{name} luster control is not uniform selected-C response",
        )
        for lane in build.FROZEN_LANES:
            fail_unless(
                contract["image_pixel_sha256"][lane]
                == recorded["source_frozen_lane_pixel_sha256"][lane],
                f"{name}/{lane} no longer matches v004.1",
            )
        components[name] = contract

    fail_unless(
        len(unique_materials) == len(base.TARGET_OBJECTS),
        "Saved candidate does not own one material per component",
    )
    fail_unless(
        candidate_group_users == len(base.TARGET_OBJECTS),
        "Saved v004.2 group does not have eight live material consumers",
    )
    fail_unless(sha256_file(build.SOURCE_BLEND) == source_hash, "Verifier changed v004.1")
    fail_unless(sha256_file(CANDIDATE_BLEND) == candidate_hash, "Verifier changed v004.2")

    contract = {
        "schema": "iggy3d.forged_iron_connected_oxide_candidate.v004_2.reopen.v1",
        "verified": True,
        "opened_path": str(opened_path),
        "candidate_blend_sha256": candidate_hash,
        "source_v004_1_sha256": source_hash,
        "source_group_contract": source_signature,
        "candidate_group_contract": target_signature,
        "group_default_differences": differences,
        "unique_material_count": len(unique_materials),
        "candidate_group_users": candidate_group_users,
        "components": components,
        "candidate_saved_by_verifier": False,
    }
    REOPEN_CONTRACT_PATH.write_text(json.dumps(contract, indent=2, sort_keys=False) + "\n")
    manifest["separate_process_reopen"] = {
        "verified": True,
        "contract": {
            "path": str(REOPEN_CONTRACT_PATH),
            "sha256": sha256_file(REOPEN_CONTRACT_PATH),
        },
        "verifier": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
    }
    MANIFEST_PATH.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(
        "FORGED_IRON_V004_2_REOPEN="
        f"verified:true,materials:{len(unique_materials)},group_users:{candidate_group_users}"
    )


if __name__ == "__main__":
    main()
