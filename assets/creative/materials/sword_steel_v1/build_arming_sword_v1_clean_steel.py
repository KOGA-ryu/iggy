#!/usr/bin/env python3
"""Install sword_steel_v1 on the measured BATMAN arming-sword blade."""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Vector
import numpy as np


PACKAGE = Path(__file__).resolve().parent
PROFILE_PATH = PACKAGE / "profiles" / "arming_sword_v1_clean_steel.json"
CANONICAL_BUILDER_PATH = PACKAGE / "build_sword_steel_v1.py"
DEFAULT_OUTPUT_ROOT = PACKAGE / "output" / "arming_sword_v1_consumer"
MATERIAL_NAME = "IGGY_MAT_ArmingSword_CleanSteel_v001"
REGION_ATTRIBUTE = "sinc_sword_region"
BLADE_UV = "IGGY_BladeUV"
SAVED_BLEND_NAME = "arming_sword_v1_clean_steel.blend"
MANIFEST_NAME = "arming_sword_v1_clean_steel_manifest.json"


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--factory-root",
        type=Path,
        default=Path("/home/kogaRyu/blender-refs"),
    )
    parser.add_argument("--profile", type=Path, default=PROFILE_PATH)
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT_ROOT)
    parser.add_argument("--resolution-x", type=int, default=720)
    parser.add_argument("--resolution-y", type=int, default=1080)
    parser.add_argument("--samples", type=int, default=48)
    parser.add_argument("--validate-only", action="store_true")
    parser.add_argument("--blend-path", type=Path)
    parser.add_argument("--manifest-path", type=Path)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_canonical_builder():
    spec = importlib.util.spec_from_file_location(
        "iggy_sword_steel_v1_canonical",
        CANONICAL_BUILDER_PATH,
    )
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load canonical sword builder: {CANONICAL_BUILDER_PATH}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


canonical = load_canonical_builder()


def load_profile(path: Path) -> dict[str, Any]:
    profile = json.loads(path.expanduser().resolve().read_text())
    if profile["geometry"]["has_fuller"]:
        raise RuntimeError("the measured arming sword may not gain an invented fuller")
    if profile["finish"]["regions"]["fuller"]["enabled"]:
        raise RuntimeError("the absent fuller region must stay disabled")
    return profile


def source_paths(factory_root: Path, profile: dict[str, Any]) -> dict[str, Path]:
    consumer = profile["consumer"]
    paths = {
        "factory": factory_root / consumer["source_factory_relative_path"],
        "finish": factory_root / consumer["source_finish_relative_path"],
        "blade": factory_root / consumer["source_blade_relative_path"],
    }
    expected = {
        "factory": consumer["source_factory_sha256"],
        "finish": consumer["source_finish_sha256"],
        "blade": consumer["source_blade_sha256"],
    }
    for name, path in paths.items():
        if not path.is_file():
            raise FileNotFoundError(f"factory source is missing: {path}")
        actual = sha256_file(path)
        if actual != expected[name]:
            raise RuntimeError(
                f"{name} source drifted: expected {expected[name]}, got {actual}"
            )
    return paths


def load_donor_contracts(profile: dict[str, Any]) -> tuple[dict[str, Any], dict[str, Any]]:
    conductor = json.loads(canonical.CONDUCTOR_MANIFEST.read_text())
    tangent = json.loads(canonical.TANGENT_MANIFEST.read_text())
    if (
        conductor["comparison_contract"]["shared_linear_rgb_value"]
        != profile["donors"]["expected_f0_linear"]
    ):
        raise RuntimeError("clean conductor F0 drifted")
    if (
        conductor["comparison_contract"]["shared_roughness"]
        != profile["donors"]["body_roughness"]
    ):
        raise RuntimeError("clean conductor roughness drifted")
    if (
        tangent["selection_contract"]["selected_default"]
        != profile["donors"]["body_anisotropy"]
    ):
        raise RuntimeError("accepted tangent anisotropy drifted")
    return conductor, tangent


def execute_factory_prefix(factory_root: Path) -> dict[str, Any]:
    assembly_path = factory_root / "tools" / "assemble_arming_sword.py"
    for name in ("Cube", "Light", "Camera"):
        obj = bpy.data.objects.get(name)
        if obj is not None:
            bpy.data.objects.remove(obj, do_unlink=True)
    namespace: dict[str, Any] = {
        "__name__": "__iggy_arming_sword_factory__",
        "__file__": str(assembly_path),
    }
    exec(
        compile(assembly_path.read_text(), str(assembly_path), "exec"),
        namespace,
    )
    return namespace


def select_generated_blade(namespace: dict[str, Any]) -> bpy.types.Object:
    candidates = [
        obj
        for obj in bpy.data.objects
        if obj.type == "MESH" and obj.name.startswith("SW_blade")
    ]
    if not candidates:
        raise RuntimeError("factory produced no SW_blade mesh")
    selected = max(candidates, key=lambda obj: len(obj.data.polygons))
    if namespace.get("blade_ob") is not selected or selected.name != "SW_blade.001":
        raise RuntimeError("factory blade reference or object identity drifted")
    if (len(selected.data.vertices), len(selected.data.polygons)) != (270, 268):
        raise RuntimeError("factory blade topology drifted")
    return selected


def apply_factory_blade_baseline(
    blade: bpy.types.Object,
    source_zones: list[str],
    factory_root: Path,
) -> bpy.types.Material:
    from sg import stylize

    swatches = {
        "blade": "iron",
        "blade_edge": "stone_light",
        "ricasso": "iron",
    }
    stylize._set_uvs(blade, source_zones, swatches)
    old_col = blade.data.color_attributes.get("Col")
    if old_col is not None:
        blade.data.color_attributes.remove(old_col)
    col = blade.data.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    for datum in col.data:
        datum.color = (1.0, 1.0, 1.0, 1.0)
    stylize._dirty(blade, strength=0.30)
    image = bpy.data.images.load(
        str(factory_root / "textures" / "T_Palette_Master_01.png")
    )
    material = bpy.data.materials.new("M_ArmingSword_BladeBaseline")
    material.use_nodes = True
    tree = material.node_tree
    principled = tree.nodes["Principled BSDF"]
    texture = tree.nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    vertex_colour = tree.nodes.new("ShaderNodeVertexColor")
    vertex_colour.layer_name = "Col"
    multiply = tree.nodes.new("ShaderNodeMix")
    multiply.data_type = "RGBA"
    multiply.blend_type = "MULTIPLY"
    multiply.inputs["Factor"].default_value = 1.0
    tree.links.new(texture.outputs["Color"], multiply.inputs["A"])
    tree.links.new(vertex_colour.outputs["Color"], multiply.inputs["B"])
    tree.links.new(multiply.outputs["Result"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.8
    blade.data.materials.clear()
    blade.data.materials.append(material)
    for polygon in blade.data.polygons:
        polygon.material_index = 0
        polygon.use_smooth = True
    return material


def profile_half_width_m(t: float, profile: dict[str, Any]) -> float:
    consumer = profile["consumer"]
    exponent = profile["geometry"]["profile_width_exponent"]
    t = min(max(t, 0.0), 1.0)
    return 0.5 * (
        consumer["shoulder_width_m"]
        + (consumer["pre_tip_width_m"] - consumer["shoulder_width_m"])
        * (t**exponent)
    )


def author_blade_semantics(
    blade: bpy.types.Object,
    source_zones: list[str],
    profile: dict[str, Any],
) -> dict[str, Any]:
    mesh = blade.data
    if len(source_zones) != len(mesh.polygons):
        raise RuntimeError("factory zone list no longer matches blade polygons")
    geometry = profile["geometry"]
    consumer = profile["consumer"]
    z_min = min(vertex.co.z for vertex in mesh.vertices)
    z_max = max(vertex.co.z for vertex in mesh.vertices)
    length_m = z_max - z_min
    if abs(length_m - consumer["blade_length_m"]) > 1.0e-6:
        raise RuntimeError("factory blade length drifted")

    old_uv = mesh.uv_layers.get(geometry["blade_uv"])
    if old_uv is not None:
        mesh.uv_layers.remove(old_uv)
    uv = mesh.uv_layers.new(name=geometry["blade_uv"])
    old_region = mesh.color_attributes.get(geometry["region_attribute"])
    if old_region is not None:
        mesh.color_attributes.remove(old_region)
    region = mesh.color_attributes.new(
        name=geometry["region_attribute"],
        type="FLOAT_COLOR",
        domain="CORNER",
    )

    counts = {"body": 0, "fuller": 0, "bevel": 0, "edge": 0}
    source_mismatches = 0
    vectors = {
        "body": (1.0, 0.0, 0.0, 0.0),
        "fuller": (0.0, 1.0, 0.0, 0.0),
        "bevel": (0.0, 0.0, 1.0, 0.0),
        "edge": (0.0, 0.0, 0.0, 1.0),
    }
    bevel_start = 1.0 - geometry["secondary_bevel_half_width_fraction"]
    for polygon, source_zone in zip(mesh.polygons, source_zones):
        t_center = min(max((polygon.center.z - z_min) / length_m, 0.0), 1.0)
        ratio = abs(polygon.center.x) / max(
            profile_half_width_m(t_center, profile),
            1.0e-8,
        )
        is_cap = abs(polygon.normal.z) > 0.9
        if is_cap:
            semantic = (
                "edge" if polygon.center.z > z_min + length_m * 0.5 else "body"
            )
        elif ratio >= 0.95:
            semantic = "edge"
        elif ratio >= bevel_start:
            semantic = "bevel"
        else:
            semantic = "body"
        if not is_cap and ((source_zone == "blade_edge") != (semantic == "edge")):
            source_mismatches += 1
        counts[semantic] += 1
        for loop_index in polygon.loop_indices:
            vertex = mesh.vertices[mesh.loops[loop_index].vertex_index]
            u = min(max((vertex.co.z - z_min) / length_m, 0.0), 1.0)
            vertex_half_width_m = profile_half_width_m(u, profile)
            v = min(
                max(0.5 + vertex.co.x / (2.0 * vertex_half_width_m), 0.0),
                1.0,
            )
            if polygon.normal.y < -0.1:
                v = 1.0 - v
            uv.data[loop_index].uv = (u, v)
            region.data[loop_index].color = vectors[semantic]

    if counts != geometry["expected_region_polygon_counts"]:
        raise RuntimeError(f"actual blade region counts drifted: {counts}")
    if source_mismatches != geometry["expected_source_zone_mismatch_polygon_count"]:
        raise RuntimeError(
            "source zone mismatch count drifted: "
            f"expected {geometry['expected_source_zone_mismatch_polygon_count']}, "
            f"got {source_mismatches}"
        )
    quad_count = sum(len(polygon.vertices) == 4 for polygon in mesh.polygons)
    if quad_count != consumer["expected_quad_count"]:
        raise RuntimeError("factory blade is no longer all quads")
    blade["iggy_material_consumer_id"] = profile["integration_id"]
    blade["iggy_source_zone_mismatch_polygon_count"] = source_mismatches
    mesh.update()
    return {
        "vertex_count": len(mesh.vertices),
        "polygon_count": len(mesh.polygons),
        "quad_count": quad_count,
        "dimensions_m": [round(float(value), 6) for value in blade.dimensions],
        "region_polygon_counts": counts,
        "source_zone_counts": dict(sorted(Counter(source_zones).items())),
        "source_zone_mismatch_polygon_count": source_mismatches,
        "has_fuller": False,
        "cross_section": geometry["cross_section"],
        "region_attribute": geometry["region_attribute"],
        "blade_uv": geometry["blade_uv"],
    }


def configure_cycles_device(scene: bpy.types.Scene) -> str:
    try:
        preferences = bpy.context.preferences.addons["cycles"].preferences
        preferences.compute_device_type = "OPTIX"
        preferences.get_devices()
        for device in preferences.devices:
            device.use = device.type != "CPU"
        scene.cycles.device = "GPU"
        return "OPTIX_GPU"
    except (KeyError, RuntimeError, TypeError):
        scene.cycles.device = "CPU"
        return "CPU"


def configure_scene(
    resolution_x: int,
    resolution_y: int,
    samples: int,
) -> tuple[bpy.types.Scene, bpy.types.Object, dict[str, bpy.types.Object], str]:
    for obj in list(bpy.data.objects):
        if obj.type in {"LIGHT", "CAMERA"}:
            bpy.data.objects.remove(obj, do_unlink=True)
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    device = configure_cycles_device(scene)
    scene.render.resolution_x = resolution_x
    scene.render.resolution_y = resolution_y
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.view_settings.exposure = -1.25
    if scene.world is None:
        scene.world = bpy.data.worlds.new("IGGY_ArmingSword_ProofWorld")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.008, 0.011, 0.017, 1.0)
    background.inputs["Strength"].default_value = 0.035

    camera_data = bpy.data.cameras.new("IGGY_ArmingSword_ProofCamera")
    camera = bpy.data.objects.new("IGGY_ArmingSword_ProofCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    lights = {
        "key": canonical.create_area_light(
            scene, "IGGY_ArmingSword_Key", (-0.24, -0.34, 0.42), 22.0, 0.06, 0.68
        ),
        "fill": canonical.create_area_light(
            scene, "IGGY_ArmingSword_Fill", (0.34, -0.22, 0.48), 5.0, 0.12, 0.50
        ),
        "grazing": canonical.create_area_light(
            scene, "IGGY_ArmingSword_Grazing", (-0.18, -0.11, 0.46), 8.0, 0.018, 0.70
        ),
    }
    return scene, camera, lights, device


def point_at(obj: bpy.types.Object, target: tuple[float, float, float]) -> None:
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def configure_view(
    camera: bpy.types.Object,
    lights: dict[str, bpy.types.Object],
    view: str,
) -> None:
    for light in lights.values():
        light.hide_render = True
    full_target = (0.0, 0.0, 0.305)
    if view in {"baseline", "front", "regions"}:
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = 1.12
        camera.location = (0.0, -1.25, 0.305)
        target = full_target
    elif view in {"three_quarter", "clay"}:
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = 1.12
        camera.location = (0.48, -1.10, 0.305)
        target = full_target
    elif view == "gameplay":
        camera.data.type = "PERSP"
        camera.data.lens = 64.0
        camera.location = (0.72, -2.35, 0.36)
        target = full_target
    else:
        camera.data.type = "ORTHO"
        camera.data.ortho_scale = 0.31
        camera.location = (0.0, -0.62, 0.46)
        target = (0.0, 0.0, 0.46)
    point_at(camera, target)

    if view == "grazing":
        lights["grazing"].hide_render = False
        lights["fill"].hide_render = False
        lights["grazing"].data.energy = 8.0
        lights["fill"].data.energy = 0.4
    elif view not in {"regions", "macro", "finish"}:
        lights["key"].hide_render = False
        lights["fill"].hide_render = False
        lights["key"].data.energy = 22.0
        lights["fill"].data.energy = 5.0
    for light in lights.values():
        point_at(light, target)


def render_still(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return {"path": path.name, "bytes": path.stat().st_size, "sha256": sha256_file(path)}


def build_comparison_board(
    output_root: Path,
    renders: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    order = (
        "baseline",
        "front",
        "three_quarter",
        "grazing",
        "gameplay",
        "clay",
        "regions",
        "macro",
        "finish",
    )
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for the actual-sword proof board")
    inputs: list[str] = []
    filters: list[str] = []
    for index, name in enumerate(order):
        inputs.extend(("-i", str(output_root / renders[name]["path"])))
        filters.append(f"[{index}:v]pad=iw+4:ih+4:2:2:color=0x181b22[t{index}]")
    filters.extend(
        (
            "[t0][t1][t2]hstack=inputs=3[r0]",
            "[t3][t4][t5]hstack=inputs=3[r1]",
            "[t6][t7][t8]hstack=inputs=3[r2]",
            "[r0][r1][r2]vstack=inputs=3[board]",
        )
    )
    board_path = output_root / "arming_sword_v1_clean_steel_comparison_board.png"
    command = [
        ffmpeg,
        "-y",
        "-hide_banner",
        "-loglevel",
        "error",
        *inputs,
        "-filter_complex",
        ";".join(filters),
        "-map",
        "[board]",
        "-frames:v",
        "1",
        str(board_path),
    ]
    completed = subprocess.run(command, capture_output=True, text=True)
    if completed.returncode != 0:
        raise RuntimeError("actual-sword proof board failed: " + completed.stderr)
    return {
        "path": board_path.name,
        "bytes": board_path.stat().st_size,
        "sha256": sha256_file(board_path),
        "order": list(order),
    }


def output_record(path: Path) -> dict[str, Any]:
    return {"path": path.name, "bytes": path.stat().st_size, "sha256": sha256_file(path)}


def validate_saved_blend(blend_path: Path, manifest_path: Path) -> None:
    blend_path = blend_path.expanduser().resolve()
    manifest_path = manifest_path.expanduser().resolve()
    if not blend_path.is_file() or not manifest_path.is_file():
        raise FileNotFoundError("actual-sword blend or manifest is missing")
    bpy.ops.wm.open_mainfile(filepath=str(blend_path))
    blade = bpy.data.objects.get("SW_blade.001")
    material = bpy.data.materials.get(MATERIAL_NAME)
    if blade is None or material is None:
        raise RuntimeError("saved actual sword blade or material is missing")
    dimensions = [round(float(value), 6) for value in blade.dimensions]
    if dimensions != [0.048, 0.006, 0.78]:
        raise RuntimeError(f"saved actual blade dimensions drifted: {dimensions}")
    if len(blade.data.polygons) != 268:
        raise RuntimeError("saved actual blade polygon count drifted")
    if REGION_ATTRIBUTE not in blade.data.color_attributes:
        raise RuntimeError("saved actual blade region attribute is missing")
    if BLADE_UV not in blade.data.uv_layers:
        raise RuntimeError("saved actual blade UV is missing")
    region = blade.data.color_attributes[REGION_ATTRIBUTE]
    counts = {"body": 0, "fuller": 0, "bevel": 0, "edge": 0}
    for polygon in blade.data.polygons:
        color = region.data[polygon.loop_start].color
        semantic = ("body", "fuller", "bevel", "edge")[
            max(range(4), key=lambda index: color[index])
        ]
        counts[semantic] += 1
    if counts != {"body": 176, "fuller": 0, "bevel": 56, "edge": 36}:
        raise RuntimeError(f"saved actual blade region counts drifted: {counts}")
    tree = material.node_tree
    if sum(node.bl_idname == "ShaderNodeBsdfPrincipled" for node in tree.nodes) != 1:
        raise RuntimeError("saved actual steel must have one Principled BSDF")
    if any(node.bl_idname == "ShaderNodeMixShader" for node in tree.nodes):
        raise RuntimeError("saved actual steel unexpectedly contains a Mix Shader")
    image_nodes = [node for node in tree.nodes if node.bl_idname == "ShaderNodeTexImage"]
    if len(image_nodes) != 2 or any(node.image is None for node in image_nodes):
        raise RuntimeError("saved actual steel does not have two live finish maps")
    if any(node.image.packed_file is None for node in image_nodes):
        raise RuntimeError("saved actual steel finish maps are not packed")
    manifest = json.loads(manifest_path.read_text())
    manifest["reopen_validated"] = True
    manifest["reopen_validation"] = {
        "blend_sha256": sha256_file(blend_path),
        "object": blade.name,
        "dimensions_m": dimensions,
        "region_polygon_counts": counts,
        "material": material.name,
        "packed_images": sorted(node.image.name for node in image_nodes),
    }
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print("ARMING_SWORD_CLEAN_STEEL_REOPEN_VALIDATED=true")


def build(args: argparse.Namespace) -> None:
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    profile = load_profile(args.profile)
    sources = source_paths(args.factory_root.expanduser().resolve(), profile)
    source_hashes_before = {name: sha256_file(path) for name, path in sources.items()}
    conductor, tangent = load_donor_contracts(profile)
    macro = canonical.generate_macro_polish_field(profile)
    grind = canonical.generate_grind_field(profile)
    macro_path = output_root / "arming_sword_v1_macro_polish.png"
    grind_path = output_root / "arming_sword_v1_grind_field.png"
    canonical.common.write_png_gray16(macro_path, macro)
    canonical.common.write_png_gray16(grind_path, grind)

    namespace = execute_factory_prefix(args.factory_root.expanduser().resolve())
    blade = select_generated_blade(namespace)
    source_zones = list(namespace["bz"])
    baseline_material = apply_factory_blade_baseline(
        blade,
        source_zones,
        args.factory_root.expanduser().resolve(),
    )
    if "SW_ALL" in bpy.data.objects:
        bpy.data.objects["SW_ALL"].hide_render = True

    scene, camera, lights, render_device = configure_scene(
        args.resolution_x,
        args.resolution_y,
        args.samples,
    )
    renders: dict[str, dict[str, Any]] = {}
    configure_view(camera, lights, "baseline")
    renders["baseline"] = render_still(
        scene,
        output_root / "arming_sword_v1_baseline.png",
    )

    geometry_contract = author_blade_semantics(blade, source_zones, profile)
    macro_image = bpy.data.images.load(str(macro_path), check_existing=False)
    macro_image.name = "IGGY_IMG_ArmingSword_MacroPolish_v001"
    macro_image.colorspace_settings.name = "Non-Color"
    macro_image.pack()
    grind_image = bpy.data.images.load(str(grind_path), check_existing=False)
    grind_image.name = "IGGY_IMG_ArmingSword_LongitudinalGrind_v001"
    grind_image.colorspace_settings.name = "Non-Color"
    grind_image.pack()
    canonical.MATERIAL_NAME = MATERIAL_NAME
    physical, material_contract = canonical.build_sword_steel_material(
        profile,
        conductor,
        tangent,
        macro_image,
        grind_image,
    )
    clay = canonical.build_clay_material()
    regions = canonical.build_region_material()
    macro_proof = canonical.build_finish_material(
        macro_image,
        "IGGY_MAT_ArmingSword_MacroPolishProof",
        (float(macro.min()), float(macro.max())),
    )
    finish_proof = canonical.build_finish_material(
        grind_image,
        "IGGY_MAT_ArmingSword_MediumGrindProof",
    )

    for view in ("front", "three_quarter", "grazing", "gameplay"):
        canonical.set_material(blade, physical)
        configure_view(camera, lights, view)
        renders[view] = render_still(
            scene,
            output_root / f"arming_sword_v1_{view}.png",
        )
    for view, proof_material in (
        ("clay", clay),
        ("regions", regions),
        ("macro", macro_proof),
        ("finish", finish_proof),
    ):
        canonical.set_material(blade, proof_material)
        configure_view(camera, lights, view)
        renders[view] = render_still(
            scene,
            output_root / f"arming_sword_v1_{view}.png",
        )
    canonical.set_material(blade, physical)
    blade["iggy_baseline_materials"] = [baseline_material.name]
    blade["iggy_baseline_outline_disabled_for_material_proof"] = True
    bpy.ops.file.pack_all()
    blend_path = output_root / SAVED_BLEND_NAME
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    board = build_comparison_board(output_root, renders)

    source_hashes_after = {name: sha256_file(path) for name, path in sources.items()}
    if source_hashes_after != source_hashes_before:
        raise RuntimeError("actual-sword integration changed the source factory")
    macro_centered = macro - 0.5
    grind_centered = grind - 0.5
    manifest = {
        "schema": "iggy3d.arming-sword-clean-steel-build.v1",
        "status": "ACTUAL_ASSET_PRODUCTION_CANDIDATE",
        "blender_version": bpy.app.version_string,
        "consumer": {
            **profile["consumer"],
            "source_object": blade.name,
        },
        "geometry": geometry_contract,
        "material": material_contract,
        "finish": {
            "macro_polish": {
                "resolution": [macro.shape[1], macro.shape[0]],
                "map_span_m": profile["finish"]["macro_polish"]["map_span_m"],
                "minimum": float(macro.min()),
                "maximum": float(macro.max()),
                "mean": float(macro.mean()),
                "signed_abs_p95": float(np.percentile(np.abs(macro_centered), 95.0)),
            },
            "medium_grind": {
                "resolution": [grind.shape[1], grind.shape[0]],
                "map_span_m": profile["finish"]["medium_grind"]["map_span_m"],
                "track_count": profile["finish"]["medium_grind"]["track_count"],
                "minimum": float(grind.min()),
                "maximum": float(grind.max()),
                "mean": float(grind.mean()),
                "signed_abs_p95": float(np.percentile(np.abs(grind_centered), 95.0)),
            },
            "micro_response": profile["finish"]["micro_response"],
        },
        "source_files": {
            name: {
                "path": str(path),
                "sha256_before": source_hashes_before[name],
                "sha256_after": source_hashes_after[name],
            }
            for name, path in sources.items()
        },
        "proof_contract": {
            "matched_baseline_and_front": True,
            "factory_finish_script_executable": False,
            "factory_finish_failure": "SW_grip is missing the expected zone attribute",
            "blade_outline_show_render_before": False,
            "blade_outline_show_render_during_proofs": False,
            "render_engine": scene.render.engine,
            "render_device": render_device,
            "samples": args.samples,
            "resolution": [args.resolution_x, args.resolution_y],
            "view_transform": scene.view_settings.view_transform,
            "look": scene.view_settings.look,
        },
        "outputs": {
            "macro_polish_field": output_record(macro_path),
            "grind_field": output_record(grind_path),
            "saved_blend": output_record(blend_path),
            "comparison_board": board,
            "renders": renders,
        },
        "saved_blend_images_packed": all(
            image.packed_file is not None for image in (macro_image, grind_image)
        ),
        "reopen_validated": False,
        "uses_ai_generated_imagery": False,
        "uses_damage": False,
        "unreal_parity_verified": False,
        "limitations": [
            "Abrasive-track population and finish amplitudes are authored transfers, not measurements from the factory blade.",
            "The source zone labels contain a recorded 28-polygon side-face mismatch; the integration semantic is geometry-derived.",
            "The clean-steel pass changes only the blade; guard, grip, pommel, fittings, and their source styling remain context.",
            "Blender Cycles proof does not establish Unreal anisotropy parity.",
        ],
    }
    manifest_path = output_root / MANIFEST_NAME
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(
        "ARMING_SWORD_CLEAN_STEEL_BUILD="
        f"regions:{geometry_contract['region_polygon_counts']},"
        f"source_zone_mismatches:{geometry_contract['source_zone_mismatch_polygon_count']},"
        f"material:{material_contract['material_name']}"
    )
    print(f"ARMING_SWORD_CLEAN_STEEL_OUTPUT={output_root}")


def main() -> None:
    args = parse_args()
    if args.validate_only:
        blend_path = args.blend_path or (args.output_root / SAVED_BLEND_NAME)
        manifest_path = args.manifest_path or (args.output_root / MANIFEST_NAME)
        validate_saved_blend(blend_path, manifest_path)
    else:
        build(args)


if __name__ == "__main__":
    main()
