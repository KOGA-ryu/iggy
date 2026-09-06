#!/usr/bin/env python3
"""Build the selected B response as a separate noncanonical v004 candidate."""

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
SELECTION_SCRIPT = MATERIAL_ROOT / "compare_connected_oxide_luster_v1.py"
OUTPUT_ROOT = MATERIAL_ROOT / "output" / "forged_iron_connected_oxide_candidate_v004"
CANDIDATE_BLEND = OUTPUT_ROOT / "forged_iron_connected_oxide_candidate_v004.blend"
MANIFEST_PATH = OUTPUT_ROOT / "manifest.json"
GROUP_NAME = "IGGY_SH_ConnectedOxideForgedIron_v004"
MATERIAL_PREFIX = "IGGY_MAT_ConnectedOxideForgedIron_v004"
SELECTED_RECIPE_ID = "B_connected_oxide_no_luster"
EXPOSED_CONDUCTOR = 0.0
LOCKED_ABSENCES = (
    "rust",
    "condition",
    "contact_polish",
    "soot",
    "dirt",
    "blood",
    "cat_face_motifs",
    "diamond_stamps",
)


def _load_selection():
    spec = importlib.util.spec_from_file_location(
        "iggy_v004_connected_oxide_selection",
        SELECTION_SCRIPT,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load selected response code: {SELECTION_SCRIPT}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


selection = _load_selection()
base = selection.base


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


def new_socket(
    group: bpy.types.NodeTree,
    name: str,
    in_out: str,
    socket_type: str,
    *,
    default: Any | None = None,
) -> None:
    socket = group.interface.new_socket(
        name=name,
        in_out=in_out,
        socket_type=socket_type,
    )
    if default is not None:
        socket.default_value = default


def set_node_location(node: bpy.types.Node, x: float, y: float) -> bpy.types.Node:
    node.location = (x, y)
    return node


def create_shared_response_group() -> tuple[bpy.types.NodeTree, dict[str, Any]]:
    previous = bpy.data.node_groups.get(GROUP_NAME)
    if previous is not None:
        bpy.data.node_groups.remove(previous, do_unlink=True)
    group = bpy.data.node_groups.new(GROUP_NAME, "ShaderNodeTree")
    group.description = (
        "Selected B connected intact oxide with broad forging normal, independent "
        "roughness, and explicitly disabled worked luster."
    )
    group.color_tag = "SHADER"
    for name, socket_type, default in (
        ("Base Color", "NodeSocketColor", (0.008, 0.009, 0.011, 1.0)),
        ("Roughness", "NodeSocketFloat", 0.715),
        ("Normal Color", "NodeSocketColor", (0.5, 0.5, 1.0, 1.0)),
        ("Luster Control", "NodeSocketColor", (0.0, 0.5, 1.0, 1.0)),
        ("Component Tangent", "NodeSocketVector", (1.0, 0.0, 0.0)),
    ):
        new_socket(group, name, "INPUT", socket_type, default=default)
    for name, socket_type in (
        ("Shader", "NodeSocketShader"),
        ("Base Color", "NodeSocketColor"),
        ("Roughness", "NodeSocketFloat"),
        ("Normal Color", "NodeSocketColor"),
        ("Metalness", "NodeSocketFloat"),
        ("Luster Response", "NodeSocketColor"),
    ):
        new_socket(group, name, "OUTPUT", socket_type)

    nodes = group.nodes
    links = group.links
    group_input = set_node_location(nodes.new("NodeGroupInput"), -900.0, 120.0)
    group_input.name = "Group_Input"
    group_output = set_node_location(nodes.new("NodeGroupOutput"), 900.0, 120.0)
    group_output.name = "Group_Output"

    normal_map = set_node_location(nodes.new("ShaderNodeNormalMap"), -420.0, -80.0)
    normal_map.name = "Broad_Forging_Normal_Decode"
    normal_map.space = "TANGENT"
    normal_map.uv_map = base.FIELD_UV
    normal_map.inputs["Strength"].default_value = 1.0

    separate = set_node_location(nodes.new("ShaderNodeSeparateColor"), -420.0, -330.0)
    separate.name = "Separate_Luster_Amount_Rotation_Rest"
    separate.mode = "RGB"
    disabled_luster = set_node_location(nodes.new("ShaderNodeMath"), -140.0, -320.0)
    disabled_luster.name = "Worked_Luster_Disabled"
    disabled_luster.operation = "MULTIPLY"
    disabled_luster.inputs[1].default_value = 0.0

    oxide_bsdf = set_node_location(nodes.new("ShaderNodeBsdfPrincipled"), 160.0, 270.0)
    oxide_bsdf.name = "Complete_Intact_Oxide_Response"
    base.set_principled_input(oxide_bsdf, ("Metallic",), 0.0)
    base.set_principled_input(oxide_bsdf, ("IOR",), 2.10)
    base.set_principled_input(oxide_bsdf, ("Coat Weight", "Coat"), 0.0)
    oxide_anisotropy = base.set_principled_input(
        oxide_bsdf,
        ("Anisotropic IOR Level", "Anisotropic"),
        0.0,
    )
    oxide_rotation = base.set_principled_input(
        oxide_bsdf,
        ("Anisotropic Rotation",),
        0.5,
    )

    conductor_bsdf = set_node_location(nodes.new("ShaderNodeBsdfPrincipled"), 160.0, -70.0)
    conductor_bsdf.name = "Complete_Hidden_Iron_Response"
    base.set_principled_input(
        conductor_bsdf,
        ("Base Color",),
        (*base.CONDUCTOR_F0_LINEAR, 1.0),
    )
    base.set_principled_input(conductor_bsdf, ("Metallic",), 1.0)
    base.set_principled_input(conductor_bsdf, ("Roughness",), base.CONDUCTOR_ROUGHNESS)
    base.set_principled_input(
        conductor_bsdf,
        ("Anisotropic IOR Level", "Anisotropic"),
        base.ANISOTROPY,
    )

    exposure = set_node_location(nodes.new("ShaderNodeValue"), 160.0, -430.0)
    exposure.name = "Explicit_Exposed_Conductor_Only_Owner"
    exposure.outputs[0].default_value = EXPOSED_CONDUCTOR
    physical_mix = set_node_location(nodes.new("ShaderNodeMixShader"), 570.0, 150.0)
    physical_mix.name = "Physical_Oxide_And_Hidden_Conductor"

    links.new(group_input.outputs["Normal Color"], normal_map.inputs["Color"])
    links.new(group_input.outputs["Luster Control"], separate.inputs["Color"])
    links.new(separate.outputs[0], disabled_luster.inputs[0])
    links.new(disabled_luster.outputs[0], oxide_bsdf.inputs[oxide_anisotropy])
    links.new(separate.outputs[1], oxide_bsdf.inputs[oxide_rotation])
    for shader in (oxide_bsdf, conductor_bsdf):
        links.new(group_input.outputs["Component Tangent"], shader.inputs["Tangent"])
        links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    links.new(group_input.outputs["Base Color"], oxide_bsdf.inputs["Base Color"])
    links.new(group_input.outputs["Roughness"], oxide_bsdf.inputs["Roughness"])
    links.new(exposure.outputs[0], physical_mix.inputs["Fac"])
    links.new(oxide_bsdf.outputs["BSDF"], physical_mix.inputs[1])
    links.new(conductor_bsdf.outputs["BSDF"], physical_mix.inputs[2])
    links.new(physical_mix.outputs["Shader"], group_output.inputs["Shader"])
    links.new(group_input.outputs["Base Color"], group_output.inputs["Base Color"])
    links.new(group_input.outputs["Roughness"], group_output.inputs["Roughness"])
    links.new(group_input.outputs["Normal Color"], group_output.inputs["Normal Color"])
    links.new(exposure.outputs[0], group_output.inputs["Metalness"])
    links.new(group_input.outputs["Luster Control"], group_output.inputs["Luster Response"])

    counts = {
        "principled": sum(node.bl_idname == "ShaderNodeBsdfPrincipled" for node in nodes),
        "mix_shader": sum(node.bl_idname == "ShaderNodeMixShader" for node in nodes),
        "normal_map": sum(node.bl_idname == "ShaderNodeNormalMap" for node in nodes),
        "value": sum(node.bl_idname == "ShaderNodeValue" for node in nodes),
    }
    if counts != {"principled": 2, "mix_shader": 1, "normal_map": 1, "value": 1}:
        raise RuntimeError(f"Shared response topology drifted: {counts}")
    group["iggy_status"] = "NONCANONICAL_CYCLES_PROOF_CANDIDATE"
    group["iggy_selected_recipe"] = SELECTED_RECIPE_ID
    group["iggy_worked_luster_amount"] = 0.0
    return group, counts


def add_image_node(
    tree: bpy.types.NodeTree,
    name: str,
    image: bpy.types.Image,
    uv: bpy.types.Node,
    location: tuple[float, float],
) -> bpy.types.Node:
    node = tree.nodes.new("ShaderNodeTexImage")
    node.name = name
    node.label = name.replace("_", " ")
    node.image = image
    node.interpolation = "Linear"
    node.extension = "EXTEND"
    node.location = location
    tree.links.new(uv.outputs["UV"], node.inputs["Vector"])
    return node


def build_component_material(
    obj: bpy.types.Object,
    group: bpy.types.NodeTree,
    images: dict[str, bpy.types.Image],
) -> tuple[bpy.types.Material, dict[str, int]]:
    material = bpy.data.materials.new(f"{MATERIAL_PREFIX}__{obj.name}")
    material.use_nodes = True
    tree = material.node_tree
    tree.nodes.clear()
    output = set_node_location(tree.nodes.new("ShaderNodeOutputMaterial"), 720.0, 100.0)
    output.name = "Material_Output"
    response = set_node_location(tree.nodes.new("ShaderNodeGroup"), 400.0, 100.0)
    response.name = "Selected_B_Connected_Oxide_Response"
    response.node_tree = group
    uv = set_node_location(tree.nodes.new("ShaderNodeUVMap"), -850.0, 160.0)
    uv.name = "Nonrepeating_Component_Field_UV"
    uv.uv_map = base.FIELD_UV
    tangent = set_node_location(tree.nodes.new("ShaderNodeTangent"), -100.0, -390.0)
    tangent.name = "Component_Owned_Material_Tangent"
    tangent.direction_type = "UV_MAP"
    tangent.uv_map = base.TANGENT_UV
    color = add_image_node(
        tree,
        "Continuous_Oxide_Optical_Colour",
        images["oxide_base"],
        uv,
        (-590.0, 330.0),
    )
    roughness = add_image_node(
        tree,
        "Independent_Oxide_Roughness",
        images["oxide_roughness"],
        uv,
        (-590.0, 110.0),
    )
    normal = add_image_node(
        tree,
        "Broad_Forging_Normal_Only",
        images["combined_normal"],
        uv,
        (-590.0, -120.0),
    )
    luster = add_image_node(
        tree,
        "Explicit_Zero_Worked_Luster_Control",
        images["luster_control"],
        uv,
        (-590.0, -350.0),
    )
    tree.links.new(color.outputs["Color"], response.inputs["Base Color"])
    tree.links.new(roughness.outputs["Color"], response.inputs["Roughness"])
    tree.links.new(normal.outputs["Color"], response.inputs["Normal Color"])
    tree.links.new(luster.outputs["Color"], response.inputs["Luster Control"])
    tree.links.new(tangent.outputs["Tangent"], response.inputs["Component Tangent"])
    tree.links.new(response.outputs["Shader"], output.inputs["Surface"])
    material["iggy_status"] = "NONCANONICAL_CYCLES_PROOF_CANDIDATE"
    material["iggy_selected_recipe"] = SELECTED_RECIPE_ID
    material["iggy_component"] = obj.name
    counts = {
        "image_texture": sum(node.bl_idname == "ShaderNodeTexImage" for node in tree.nodes),
        "tangent": sum(node.bl_idname == "ShaderNodeTangent" for node in tree.nodes),
        "group": sum(node.bl_idname == "ShaderNodeGroup" for node in tree.nodes),
        "output": sum(node.bl_idname == "ShaderNodeOutputMaterial" for node in tree.nodes),
    }
    if counts != {"image_texture": 4, "tangent": 1, "group": 1, "output": 1}:
        raise RuntimeError(f"Component material topology drifted: {counts}")
    return material, counts


def scalar_range(value: np.ndarray) -> list[float]:
    return [float(value.min()), float(value.max())]


def reopen_candidate_contract(candidate_blend: Path) -> dict[str, Any]:
    bpy.ops.wm.open_mainfile(filepath=str(candidate_blend))
    group = bpy.data.node_groups.get(GROUP_NAME)
    if group is None:
        raise RuntimeError("Saved candidate lost its shared response group")
    target_contract: dict[str, Any] = {}
    shared_users = 0
    for name in base.TARGET_OBJECTS:
        obj = bpy.data.objects.get(name)
        if obj is None or len(obj.data.materials) != 1:
            raise RuntimeError(f"Saved target material contract failed: {name}")
        material = obj.data.materials[0]
        group_nodes = [
            node
            for node in material.node_tree.nodes
            if node.bl_idname == "ShaderNodeGroup" and node.node_tree == group
        ]
        images = [
            node.image
            for node in material.node_tree.nodes
            if node.bl_idname == "ShaderNodeTexImage"
        ]
        shared_users += len(group_nodes)
        target_contract[name] = {
            "material": material.name,
            "shared_group_instances": len(group_nodes),
            "image_texture_count": len(images),
            "all_images_packed": all(
                image is not None and image.packed_file is not None for image in images
            ),
            "uv_layers": sorted(layer.name for layer in obj.data.uv_layers),
        }
    if shared_users != len(base.TARGET_OBJECTS):
        raise RuntimeError(f"Expected eight shared-group users, found {shared_users}")
    return {
        "opened_path": str(Path(bpy.data.filepath).resolve()),
        "shared_group": group.name,
        "shared_group_users": shared_users,
        "targets": target_contract,
    }


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    candidate_blend = output_root / CANDIDATE_BLEND.name
    manifest_path = output_root / MANIFEST_PATH.name
    if not base.SOURCE_BLEND.is_file():
        raise FileNotFoundError(base.SOURCE_BLEND)
    selected_recipe = next(
        recipe for recipe in selection.CANDIDATES if recipe.candidate_id == SELECTED_RECIPE_ID
    )
    if selected_recipe.luster_mode != "none":
        raise RuntimeError("Selected v004 route unexpectedly enables worked luster")

    source_hash_before = sha256_file(base.SOURCE_BLEND)
    build_script_hash = sha256_file(Path(__file__).resolve())
    selection_script_hash = sha256_file(SELECTION_SCRIPT)
    bpy.ops.wm.open_mainfile(filepath=str(base.SOURCE_BLEND))
    targets = [bpy.data.objects[name] for name in base.TARGET_OBJECTS]
    frames = {obj.name: base.install_component_frames(obj) for obj in targets}
    shared_group, shared_counts = create_shared_response_group()

    component_manifest: dict[str, Any] = {}
    for obj in targets:
        fields = selection.build_candidate_fields(frames[obj.name], obj.name, selected_recipe)
        slug = obj.name.replace(".", "_")
        prefix = f"IGGY_IMG_ConnectedOxide_v004_{slug}"
        images = {
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
        material, material_counts = build_component_material(obj, shared_group, images)
        base.set_material(obj, material)
        luster_amount = fields["luster_control"][..., 0]
        component_manifest[obj.name] = {
            "material": material.name,
            "images": {key: image.name for key, image in images.items()},
            "all_images_packed": all(image.packed_file is not None for image in images.values()),
            "material_node_counts": material_counts,
            "field_resolution": list(fields["oxide_roughness"].shape[::-1]),
            "coverage_range": scalar_range(fields["oxide_coverage"]),
            "metalness_maximum": float(fields["oxide_metalness"].max()),
            "exposure_maximum": float(fields["exposure"].max()),
            "oxide_surface_height_absolute_maximum_m": float(
                np.abs(fields["oxide_surface_height_m"]).max()
            ),
            "roughness_range": scalar_range(fields["oxide_roughness"]),
            "luster_amount_range": scalar_range(luster_amount),
            "broad_forging_height_range_m": scalar_range(fields["broad_forging_height_m"]),
        }

    scene = bpy.context.scene
    scene["iggy_material_candidate"] = "forged_iron_connected_oxide_candidate_v004"
    scene["iggy_status"] = "NONCANONICAL_CYCLES_PROOF_CANDIDATE"
    scene["iggy_selected_recipe"] = SELECTED_RECIPE_ID
    scene["iggy_manual_acceptance_established"] = False
    bpy.ops.wm.save_as_mainfile(filepath=str(candidate_blend), check_existing=False)
    candidate_hash = sha256_file(candidate_blend)
    reopen_contract = reopen_candidate_contract(candidate_blend)

    source_hash_after = sha256_file(base.SOURCE_BLEND)
    canonical_source_unchanged = source_hash_before == source_hash_after
    if not canonical_source_unchanged:
        raise RuntimeError("v004 candidate build changed the frozen v003 source")
    manifest = {
        "schema": "iggy3d.forged_iron_connected_oxide_candidate.v004",
        "status": "NONCANONICAL_CYCLES_PROOF_CANDIDATE",
        "candidate": {
            "id": "forged_iron_connected_oxide_candidate_v004",
            "blend": str(candidate_blend),
            "blend_sha256": candidate_hash,
            "selected_recipe": SELECTED_RECIPE_ID,
            "shared_group": GROUP_NAME,
            "material_prefix": MATERIAL_PREFIX,
        },
        "source": {
            "path": str(base.SOURCE_BLEND),
            "sha256_before": source_hash_before,
            "sha256_after": source_hash_after,
        },
        "scripts": {
            "build": {"path": str(Path(__file__).resolve()), "sha256": build_script_hash},
            "selection": {"path": str(SELECTION_SCRIPT), "sha256": selection_script_hash},
        },
        "consumer": {
            "collection": base.TARGET_COLLECTION,
            "objects": list(base.TARGET_OBJECTS),
        },
        "shared_group_contract": shared_counts,
        "components": component_manifest,
        "reopen_contract": reopen_contract,
        "canonical_source_unchanged": canonical_source_unchanged,
        "locked_absences": list(LOCKED_ABSENCES),
        "uses_ai_generated_imagery": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "promotion_boundary": (
            "Candidate may advance only to the paired Cycles actual-hinge proof. "
            "It may not replace v003 or register as a donor."
        ),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(
        "FORGED_IRON_V004_CANDIDATE="
        f"components:{len(component_manifest)},shared_users:{reopen_contract['shared_group_users']},"
        f"source_unchanged:{canonical_source_unchanged}"
    )
    print(f"FORGED_IRON_V004_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
