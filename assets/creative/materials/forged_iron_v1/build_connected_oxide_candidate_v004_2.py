#!/usr/bin/env python3
"""Integrate the selected uniform directional response as saved v004.2."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from typing import Any

import bpy
import numpy as np


MATERIAL_ROOT = Path(__file__).resolve().parent
SOURCE_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_1"
SOURCE_BLEND = SOURCE_ROOT / "forged_iron_connected_oxide_candidate_v004_1.blend"
SOURCE_MANIFEST = SOURCE_ROOT / "manifest.json"
SOURCE_BUILD_SCRIPT = MATERIAL_ROOT / "build_connected_oxide_candidate_v004_1.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004_2"
CANDIDATE_BLEND = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004_2.blend"
MANIFEST_PATH = OUTPUT_ROOT / "manifest.json"

SOURCE_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004"
TARGET_GROUP = "IGGY_SH_ConnectedOxideForgedIron_v004_2"
SOURCE_MATERIAL_PREFIX = "IGGY_MAT_ConnectedOxideForgedIron_v004_1"
TARGET_MATERIAL_PREFIX = "IGGY_MAT_ConnectedOxideForgedIron_v004_2"
EXPECTED_TOPOLOGY_SHA256 = (
    "ce47658c611f97e6b4032596b223a7016fb05bf1daf30646b89c72b04227523b"
)
MULTIPLIER_NODE = "Worked_Luster_Disabled"
TANGENT_UV = "IGGY_IronTangentUV_v001"
DIRECTIONAL_RESPONSE_AMOUNT = 0.28
DIRECTIONAL_RESPONSE_ROTATION = 0.50
DIRECTIONAL_RESPONSE_REST = 1.00
SOURCE_MULTIPLIER = 0.00
TARGET_MULTIPLIER = 1.00

IMAGE_NODE_NAMES = {
    "oxide_base": "Continuous_Oxide_Optical_Colour",
    "oxide_roughness": "Independent_Oxide_Roughness",
    "combined_normal": "Broad_Forging_Normal_Only",
    "luster_control": "Explicit_Zero_Worked_Luster_Control",
}
FROZEN_LANES = ("oxide_base", "oxide_roughness", "combined_normal")
LOCKED_ABSENCES = (
    "new_base_colour",
    "new_roughness",
    "new_normal",
    "height",
    "oxide_height",
    "exposed_conductor",
    "rust",
    "damage",
    "scratches",
    "dents",
    "contact_polish",
    "soot",
    "dirt",
    "blood",
    "visible_brushing_pattern",
    "grooves",
    "cat_face_motifs",
    "diamond_stamps",
    "baked_light_direction",
)


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


source_build = _load_module("iggy_v004_2_source_build", SOURCE_BUILD_SCRIPT)
base = source_build.base


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


def sha256_array(value: np.ndarray) -> str:
    return hashlib.sha256(
        np.ascontiguousarray(value, dtype="<f4").tobytes()
    ).hexdigest()


def image_values(image: bpy.types.Image) -> np.ndarray:
    rgba = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(rgba)
    return rgba.reshape(image.size[1], image.size[0], 4)


def image_pixel_sha256(image: bpy.types.Image) -> str:
    return sha256_array(image_values(image))


def scalar_range(value: np.ndarray) -> list[float]:
    return [float(value.min()), float(value.max())]


def material_images(material: bpy.types.Material) -> dict[str, bpy.types.Image]:
    if not material.use_nodes or material.node_tree is None:
        raise RuntimeError(f"{material.name} has no live node tree")
    images: dict[str, bpy.types.Image] = {}
    for lane, node_name in IMAGE_NODE_NAMES.items():
        node = material.node_tree.nodes.get(node_name)
        if node is None or node.bl_idname != "ShaderNodeTexImage" or node.image is None:
            raise RuntimeError(f"{material.name} lost image owner {node_name}")
        if node.image.packed_file is None:
            raise RuntimeError(f"{material.name}/{node_name} is not packed")
        images[lane] = node.image
    return images


def group_default_values(group: bpy.types.NodeTree) -> dict[str, Any]:
    values: dict[str, Any] = {}
    for node in group.nodes:
        for index, socket in enumerate(node.inputs):
            if not hasattr(socket, "default_value"):
                continue
            default = socket.default_value
            if hasattr(default, "__len__"):
                value: Any = [float(item) for item in default]
            elif isinstance(default, (float, int, bool)):
                value = float(default) if isinstance(default, float) else default
            else:
                continue
            values[f"{node.name}:{index}:{socket.name}"] = value
    return values


def group_default_differences(
    source: bpy.types.NodeTree,
    target: bpy.types.NodeTree,
) -> list[dict[str, Any]]:
    source_values = group_default_values(source)
    target_values = group_default_values(target)
    if source_values.keys() != target_values.keys():
        raise RuntimeError("Copied group changed its default-bearing socket inventory")
    return [
        {"socket": key, "source": source_values[key], "candidate": target_values[key]}
        for key in source_values
        if source_values[key] != target_values[key]
    ]


def mesh_sha256(obj: bpy.types.Object) -> str:
    mesh = obj.data
    coordinates = np.empty(len(mesh.vertices) * 3, dtype=np.float32)
    mesh.vertices.foreach_get("co", coordinates)
    loop_vertices = np.empty(len(mesh.loops), dtype=np.int32)
    mesh.loops.foreach_get("vertex_index", loop_vertices)
    polygon_starts = np.empty(len(mesh.polygons), dtype=np.int32)
    polygon_totals = np.empty(len(mesh.polygons), dtype=np.int32)
    mesh.polygons.foreach_get("loop_start", polygon_starts)
    mesh.polygons.foreach_get("loop_total", polygon_totals)
    digest = hashlib.sha256()
    for array in (coordinates, loop_vertices, polygon_starts, polygon_totals):
        digest.update(np.ascontiguousarray(array).tobytes())
    digest.update(
        np.ascontiguousarray(np.asarray(obj.matrix_world, dtype="<f4")).tobytes()
    )
    return digest.hexdigest()


def make_uniform_luster_image(component_name: str) -> bpy.types.Image:
    values = np.empty((4, 4, 3), dtype=np.float32)
    values[..., 0] = DIRECTIONAL_RESPONSE_AMOUNT
    values[..., 1] = DIRECTIONAL_RESPONSE_ROTATION
    values[..., 2] = DIRECTIONAL_RESPONSE_REST
    image = base.make_float_image(
        f"IGGY_IMG_ConnectedOxide_v004_2_{component_name}_LusterControl",
        values,
        color=True,
    )
    if list(image.size) != [4, 4] or image.packed_file is None:
        raise RuntimeError(f"Failed to create packed 4x4 luster control for {component_name}")
    if image.colorspace_settings.name != "Non-Color":
        raise RuntimeError(f"Luster control is not Non-Color: {component_name}")
    return image


def component_contract(obj: bpy.types.Object) -> dict[str, Any]:
    material = obj.data.materials[0]
    images = material_images(material)
    luster = image_values(images["luster_control"])
    return {
        "material": material.name,
        "mesh_sha256": mesh_sha256(obj),
        "uv_layers": sorted(layer.name for layer in obj.data.uv_layers),
        "images": {lane: image.name for lane, image in images.items()},
        "image_pixel_sha256": {
            lane: image_pixel_sha256(image) for lane, image in images.items()
        },
        "all_images_packed": all(image.packed_file is not None for image in images.values()),
        "luster_control_resolution": list(images["luster_control"].size),
        "luster_amount_range": [
            round(value, 6) for value in scalar_range(luster[..., 0])
        ],
        "luster_rotation_range": [
            round(value, 6) for value in scalar_range(luster[..., 1])
        ],
        "luster_rest_range": [
            round(value, 6) for value in scalar_range(luster[..., 2])
        ],
    }


def validate_group(group: bpy.types.NodeTree, expected_multiplier: float) -> dict[str, Any]:
    signature = source_build.shared_group_signature(group)
    expected_shape = {
        "node_count": 9,
        "link_count": 20,
        "interface_socket_count": 11,
        "topology_sha256": EXPECTED_TOPOLOGY_SHA256,
    }
    for key, value in expected_shape.items():
        if signature[key] != value:
            raise RuntimeError(f"{group.name} drifted at {key}: {signature[key]} != {value}")
    multiplier = group.nodes.get(MULTIPLIER_NODE)
    if (
        multiplier is None
        or multiplier.bl_idname != "ShaderNodeMath"
        or multiplier.operation != "MULTIPLY"
        or float(multiplier.inputs[1].default_value) != expected_multiplier
    ):
        raise RuntimeError(f"{group.name} has the wrong directional multiplier")
    return signature


def reopen_in_process(candidate_blend: Path) -> dict[str, Any]:
    bpy.ops.wm.open_mainfile(filepath=str(candidate_blend))
    source_group = bpy.data.node_groups.get(SOURCE_GROUP)
    target_group = bpy.data.node_groups.get(TARGET_GROUP)
    if source_group is None or target_group is None:
        raise RuntimeError("Reopened candidate lost a required response group")
    source_signature = validate_group(source_group, SOURCE_MULTIPLIER)
    target_signature = validate_group(target_group, TARGET_MULTIPLIER)
    materials: set[str] = set()
    target_group_users = 0
    components: dict[str, Any] = {}
    for name in base.TARGET_OBJECTS:
        obj = bpy.data.objects[name]
        material = obj.data.materials[0]
        if not material.name.startswith(TARGET_MATERIAL_PREFIX):
            raise RuntimeError(f"Reopened {name} has the wrong material {material.name}")
        materials.add(material.name)
        target_group_users += sum(
            node.bl_idname == "ShaderNodeGroup" and node.node_tree == target_group
            for node in material.node_tree.nodes
        )
        components[name] = component_contract(obj)
    if len(materials) != len(base.TARGET_OBJECTS) or target_group_users != len(base.TARGET_OBJECTS):
        raise RuntimeError("Reopened candidate lost unique materials or group consumers")
    return {
        "opened_path": str(Path(bpy.data.filepath).resolve()),
        "source_group": source_signature,
        "candidate_group": target_signature,
        "unique_material_count": len(materials),
        "candidate_group_users": target_group_users,
        "components": components,
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    candidate_blend = output_root / CANDIDATE_BLEND.name
    manifest_path = output_root / MANIFEST_PATH.name
    for required in (SOURCE_BLEND, SOURCE_MANIFEST, SOURCE_BUILD_SCRIPT):
        if not required.is_file():
            raise FileNotFoundError(required)

    source_blend_before = sha256_file(SOURCE_BLEND)
    source_manifest_hash = sha256_file(SOURCE_MANIFEST)
    build_script_hash = sha256_file(Path(__file__).resolve())
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE_BLEND))

    source_group = bpy.data.node_groups.get(SOURCE_GROUP)
    if source_group is None:
        raise RuntimeError("v004.1 is missing the frozen shared group")
    source_signature = validate_group(source_group, SOURCE_MULTIPLIER)
    if bpy.data.node_groups.get(TARGET_GROUP) is not None:
        raise RuntimeError("v004.1 unexpectedly already contains the v004.2 group")

    before: dict[str, Any] = {}
    for name in base.TARGET_OBJECTS:
        obj = bpy.data.objects[name]
        material = obj.data.materials[0]
        if not material.name.startswith(SOURCE_MATERIAL_PREFIX):
            raise RuntimeError(f"{name} is not using the saved v004.1 material")
        contract = component_contract(obj)
        luster = image_values(material_images(material)["luster_control"])
        if scalar_range(luster[..., 0]) != [0.0, 0.0]:
            raise RuntimeError(f"{name} source luster amount is not frozen at zero")
        before[name] = contract

    target_group = source_group.copy()
    target_group.name = TARGET_GROUP
    target_group["iggy_status"] = "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
    target_group["iggy_direction_response_amount"] = DIRECTIONAL_RESPONSE_AMOUNT
    target_group["iggy_direction_response_rotation"] = DIRECTIONAL_RESPONSE_ROTATION
    target_multiplier = target_group.nodes[MULTIPLIER_NODE]
    target_multiplier.inputs[1].default_value = TARGET_MULTIPLIER
    # The source group has no live material consumers after retargeting. Keep
    # it only as an explicit saved-file comparison record so the reopen gate
    # can prove the exact zero-to-one default delta instead of trusting the
    # build-time observation.
    source_group.use_fake_user = True
    target_signature = validate_group(target_group, TARGET_MULTIPLIER)
    differences = group_default_differences(source_group, target_group)
    expected_difference_key = f"{MULTIPLIER_NODE}:1:Value"
    if differences != [{
        "socket": expected_difference_key,
        "source": SOURCE_MULTIPLIER,
        "candidate": TARGET_MULTIPLIER,
    }]:
        raise RuntimeError(f"v004.2 changed more than the multiplier: {differences}")

    components: dict[str, Any] = {}
    for name in base.TARGET_OBJECTS:
        obj = bpy.data.objects[name]
        material = obj.data.materials[0]
        group_nodes = [
            node
            for node in material.node_tree.nodes
            if node.bl_idname == "ShaderNodeGroup" and node.node_tree == source_group
        ]
        if len(group_nodes) != 1:
            raise RuntimeError(f"{material.name} does not have one source-group owner")
        group_nodes[0].node_tree = target_group
        material.name = f"{TARGET_MATERIAL_PREFIX}__{name}"
        material["iggy_status"] = "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
        material["iggy_direction_response_amount"] = DIRECTIONAL_RESPONSE_AMOUNT
        material["iggy_direction_response_rotation"] = DIRECTIONAL_RESPONSE_ROTATION

        images = material_images(material)
        old_luster = images["luster_control"]
        luster_node = material.node_tree.nodes[IMAGE_NODE_NAMES["luster_control"]]
        luster_node.image = make_uniform_luster_image(name)
        if old_luster.users != 0:
            raise RuntimeError(f"Old zero-luster image still has consumers: {old_luster.name}")
        bpy.data.images.remove(old_luster)

        after = component_contract(obj)
        frozen_hashes_match = all(
            before[name]["image_pixel_sha256"][lane]
            == after["image_pixel_sha256"][lane]
            for lane in FROZEN_LANES
        )
        if not frozen_hashes_match:
            raise RuntimeError(f"v004.2 changed a frozen image for {name}")
        if before[name]["mesh_sha256"] != after["mesh_sha256"]:
            raise RuntimeError(f"v004.2 changed geometry or transform for {name}")
        after["source_frozen_lane_pixel_sha256"] = {
            lane: before[name]["image_pixel_sha256"][lane] for lane in FROZEN_LANES
        }
        after["frozen_lane_hashes_match_source"] = frozen_hashes_match
        components[name] = after

    scene = bpy.context.scene
    scene["iggy_material_candidate"] = "forged_iron_connected_oxide_candidate_v004_2"
    scene["iggy_status"] = "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE"
    scene["iggy_direction_response_amount"] = DIRECTIONAL_RESPONSE_AMOUNT
    scene["iggy_direction_response_rotation"] = DIRECTIONAL_RESPONSE_ROTATION
    scene["iggy_manual_acceptance_established"] = False
    bpy.ops.wm.save_as_mainfile(filepath=str(candidate_blend), check_existing=False)
    candidate_hash = sha256_file(candidate_blend)
    in_process_reopen = reopen_in_process(candidate_blend)

    for name, reopened in in_process_reopen["components"].items():
        for lane in FROZEN_LANES:
            if (
                reopened["image_pixel_sha256"][lane]
                != components[name]["source_frozen_lane_pixel_sha256"][lane]
            ):
                raise RuntimeError(f"Reopened candidate drifted {name}/{lane}")

    source_blend_after = sha256_file(SOURCE_BLEND)
    source_unchanged = source_blend_before == source_blend_after
    if not source_unchanged:
        raise RuntimeError("v004.2 build changed the saved v004.1 source")

    manifest = {
        "schema": "iggy3d.forged_iron_connected_oxide_candidate.v004_2",
        "status": "NONCANONICAL_ACTUAL_ASSET_PROOF_CANDIDATE",
        "candidate": {
            "id": "forged_iron_connected_oxide_candidate_v004_2",
            "blend": str(candidate_blend),
            "blend_sha256": candidate_hash,
            "shared_group": TARGET_GROUP,
            "material_prefix": TARGET_MATERIAL_PREFIX,
        },
        "source_v004_1": {
            "blend": str(SOURCE_BLEND),
            "blend_sha256_before": source_blend_before,
            "blend_sha256_after": source_blend_after,
            "manifest": str(SOURCE_MANIFEST),
            "manifest_sha256": source_manifest_hash,
            "shared_group": SOURCE_GROUP,
            "material_prefix": SOURCE_MATERIAL_PREFIX,
        },
        "source_v004_1_unchanged": source_unchanged,
        "build_script": {
            "path": str(Path(__file__).resolve()),
            "sha256": build_script_hash,
        },
        "consumer": {
            "collection": base.TARGET_COLLECTION,
            "objects": list(base.TARGET_OBJECTS),
            "tangent_uv": TANGENT_UV,
        },
        "source_group_contract": source_signature,
        "shared_group_contract": target_signature,
        "group_default_differences": differences,
        "direction_response": {
            "amount": DIRECTIONAL_RESPONSE_AMOUNT,
            "rotation": DIRECTIONAL_RESPONSE_ROTATION,
            "rest": DIRECTIONAL_RESPONSE_REST,
            "source_multiplier": SOURCE_MULTIPLIER,
            "candidate_multiplier": TARGET_MULTIPLIER,
            "visible_texture_pattern": False,
        },
        "components": components,
        "in_process_reopen": in_process_reopen,
        "separate_process_reopen": {
            "verified": False,
            "contract": None,
            "verifier": None,
        },
        "locked_absences": list(LOCKED_ABSENCES),
        "uses_ai_generated_imagery": False,
        "uses_condition": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "promotion_boundary": (
            "v004.2 requires a separate-process reopen and paired v004.1 proof; "
            "it cannot become an accepted material or donor in this build step."
        ),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(
        "FORGED_IRON_V004_2_CANDIDATE="
        f"components:{len(components)},group_users:{in_process_reopen['candidate_group_users']},"
        f"source_unchanged:{source_unchanged}"
    )
    print(f"FORGED_IRON_V004_2_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
