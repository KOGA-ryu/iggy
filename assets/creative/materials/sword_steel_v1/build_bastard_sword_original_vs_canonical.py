#!/usr/bin/env python3
"""Render one direct original-map versus canonical-steel sword comparison."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Matrix


PACKAGE = Path(__file__).resolve().parent
SOURCE = (
    PACKAGE
    / "output"
    / "historical_swords_set_study_v1"
    / "source"
    / "historical_swords_set.blend"
)
CANONICAL_BUILDER = PACKAGE / "build_sword_steel_v1.py"
STUDY_BUILDER = PACKAGE / "build_historical_swords_set_study_v1.py"
RESPONSE_BUILDER = PACKAGE / "build_historical_swords_clean_steel_response_v1.py"
DEFAULT_OUTPUT = PACKAGE / "output" / "bastard_sword_original_vs_canonical"
SOURCE_SHA256 = "b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de"
CANONICAL_SHA256 = "76030d14d442ad2c1d4550b6d150c82caa536a8ac63ee8a94208a9e51ead7bbb"
DIFFUSE_SHA256 = "61d60a44e3a026ad7fac430d7fd5e5ddf4a8cde12533d84b43ddef0324b19aac"
NORMAL_SHA256 = "9c7b5c73dad8a514507b36a228044f5b36f712aa8a3a0982d4652c0a59eeacfd"
MACRO_SHA256 = "dcffc56eed667746218c001503e923defcc9b9a8855b3ace371237d2cb084e35"
GRIND_SHA256 = "52c7b2f15c8a176d67d9c0d6e722f49f3b2be86c511eea7a6a438eca902f6c63"
PANEL_WIDTH = 720
PANEL_HEIGHT = 360
VIEWS = (
    ("original_neutral", "ORIGINAL MAPS / NEUTRAL", "neutral", "original"),
    ("canonical_neutral", "OUR STEEL / NEUTRAL", "neutral", "canonical"),
    ("original_grazing", "ORIGINAL MAPS / GRAZING", "grazing", "original"),
    ("canonical_grazing", "OUR STEEL / GRAZING", "grazing", "canonical"),
    ("original_gameplay", "ORIGINAL MAPS / GAMEPLAY", "gameplay", "original"),
    ("canonical_gameplay", "OUR STEEL / GAMEPLAY", "gameplay", "canonical"),
)


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--samples", type=int, default=24)
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def artifact(path: Path) -> dict[str, Any]:
    return {"path": str(path), "bytes": path.stat().st_size, "sha256": sha256_file(path)}


def load_module(path: Path, name: str) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def selected_polygons(source: bpy.types.Object, vertex_ids: set[int]) -> list[bpy.types.MeshPolygon]:
    return [
        polygon
        for polygon in source.data.polygons
        if all(int(vertex) in vertex_ids for vertex in polygon.vertices)
    ]


def uv_digest(mesh: bpy.types.Mesh, uv_name: str, polygons: list[bpy.types.MeshPolygon]) -> str:
    layer = mesh.uv_layers.get(uv_name)
    if layer is None:
        raise RuntimeError(f"missing {uv_name}")
    digest = hashlib.sha256()
    for polygon in polygons:
        for loop_index in polygon.loop_indices:
            uv = layer.data[loop_index].uv
            digest.update(struct.pack("<ff", float(uv.x), float(uv.y)))
    return digest.hexdigest()


def copy_blade_preserving_uv(
    source: bpy.types.Object,
    component: dict[str, Any],
) -> tuple[bpy.types.Object, dict[str, Any]]:
    vertex_ids = component["_vertex_ids"]
    ordered_vertices = sorted(vertex_ids)
    remap = {source_index: target_index for target_index, source_index in enumerate(ordered_vertices)}
    source_polygons = selected_polygons(source, vertex_ids)
    source_digest = uv_digest(source.data, "UVMap", source_polygons)
    vertices = [tuple(source.data.vertices[index].co) for index in ordered_vertices]
    faces = [tuple(remap[int(vertex)] for vertex in polygon.vertices) for polygon in source_polygons]
    mesh = bpy.data.meshes.new("IGGY_COMPARE_BastardSword_Blade_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    source_uv = source.data.uv_layers["UVMap"]
    copied_uv = mesh.uv_layers.new(name="UVMap")
    for source_polygon, target_polygon in zip(source_polygons, mesh.polygons, strict=True):
        if len(source_polygon.loop_indices) != len(target_polygon.loop_indices):
            raise RuntimeError("source face loop count changed")
        for source_loop, target_loop in zip(
            source_polygon.loop_indices,
            target_polygon.loop_indices,
            strict=True,
        ):
            copied_uv.data[target_loop].uv = source_uv.data[source_loop].uv
    target_digest = uv_digest(mesh, "UVMap", list(mesh.polygons))
    if target_digest != source_digest:
        raise RuntimeError("original source UV changed during blade isolation")
    obj = bpy.data.objects.new("IGGY_COMPARE_BastardSword_Blade", mesh)
    bpy.context.collection.objects.link(obj)
    obj.matrix_world = Matrix.Identity(4)
    return obj, {
        "source_uv_layer": "UVMap",
        "source_uv_digest_before": source_digest,
        "source_uv_digest_after": target_digest,
        "source_uv_loop_count": len(copied_uv.data),
    }


def extract_packed_image(name: str, path: Path, expected_hash: str) -> None:
    image = bpy.data.images.get(name)
    if image is None or not image.packed_files:
        raise RuntimeError(f"packed source image missing: {name}")
    path.write_bytes(bytes(image.packed_files[0].packed_file.data))
    if sha256_file(path) != expected_hash:
        raise RuntimeError(f"packed source image changed: {name}")


def set_input(node: bpy.types.Node, name: str, value: Any) -> None:
    socket = node.inputs.get(name)
    if socket is None:
        raise RuntimeError(f"{node.name} has no {name} input")
    socket.default_value = value


def build_original_map_material(
    diffuse_image: bpy.types.Image,
    normal_image: bpy.types.Image,
) -> tuple[bpy.types.Material, dict[str, Any]]:
    material = bpy.data.materials.new("IGGY_REF_ONLY_OriginalSteelMaps_v001")
    material.use_nodes = True
    material["comparison_only"] = True
    material["runtime_eligible"] = False
    material["not_original_pbr_intent"] = True
    nodes = material.node_tree.nodes
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    output.name = "Reference_Only_Output"
    bsdf = nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.name = "Reference_Only_Legacy_Map_Carrier"
    uv = nodes.new("ShaderNodeUVMap")
    uv.name = "Preserved_Original_UVMap"
    uv.uv_map = "UVMap"
    diffuse = nodes.new("ShaderNodeTexImage")
    diffuse.name = "Exact_Packed_Steel_Diffuse"
    diffuse.image = diffuse_image
    diffuse.interpolation = "Linear"
    normal = nodes.new("ShaderNodeTexImage")
    normal.name = "Exact_Packed_Steel_Normal"
    normal.image = normal_image
    normal.interpolation = "Linear"
    normal_map = nodes.new("ShaderNodeNormalMap")
    normal_map.name = "Undocumented_Source_Normal_Convention"
    normal_map.space = "TANGENT"
    normal_map.inputs["Strength"].default_value = 1.0
    if bsdf.inputs.get("Weight") is not None:
        set_input(bsdf, "Weight", 1.0)
    set_input(bsdf, "Metallic", 1.0)
    set_input(bsdf, "Roughness", 0.5)
    links = material.node_tree.links
    links.new(uv.outputs["UV"], diffuse.inputs["Vector"])
    links.new(uv.outputs["UV"], normal.inputs["Vector"])
    links.new(diffuse.outputs["Color"], bsdf.inputs["Base Color"])
    links.new(normal.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])
    links.new(bsdf.outputs["BSDF"], output.inputs["Surface"])
    return material, {
        "material_name": material.name,
        "classification": "REFERENCE_ONLY_LEGACY_MAP_RECONSTRUCTION",
        "comparison_only": True,
        "runtime_eligible": False,
        "not_original_pbr_intent": True,
        "metallic_scaffold": 1.0,
        "roughness_scaffold": 0.5,
        "normal_convention": "used_as_packed; source convention undocumented",
    }


def set_material(obj: bpy.types.Object, material: bpy.types.Material) -> None:
    obj.data.materials.clear()
    obj.data.materials.append(material)
    for polygon in obj.data.polygons:
        polygon.material_index = 0


def escape_xml(value: str) -> str:
    return value.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def assemble_board(output_root: Path, panels: dict[str, dict[str, Any]]) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    sips = shutil.which("sips")
    if ffmpeg is None or sips is None:
        raise RuntimeError("ffmpeg and sips are required")
    raw = output_root / "_raw.png"
    overlay_svg = output_root / "_overlay.svg"
    overlay_png = output_root / "_overlay.png"
    board = output_root / "bastard_sword_original_vs_canonical.png"
    ordered_paths = [Path(panels[name]["path"]) for name, _, _, _ in VIEWS]
    layout = "|".join(
        f"{(index % 2) * PANEL_WIDTH}_{(index // 2) * PANEL_HEIGHT}"
        for index in range(len(VIEWS))
    )
    result = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum((["-i", str(path)] for path in ordered_paths), []),
            "-filter_complex",
            f"xstack=inputs={len(VIEWS)}:layout={layout}[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(raw),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode:
        raise RuntimeError(result.stderr)
    width = PANEL_WIDTH * 2
    height = PANEL_HEIGHT * 3
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">']
    for index, (_, label, _, _) in enumerate(VIEWS):
        x = (index % 2) * PANEL_WIDTH
        y = (index // 2) * PANEL_HEIGHT
        svg.extend(
            [
                f'<rect x="{x}" y="{y}" width="{PANEL_WIDTH}" height="38" fill="#090b0f" fill-opacity="0.92"/>',
                f'<text x="{x + 14}" y="{y + 25}" fill="#f2f4f6" font-family="Arial" font-size="15" font-weight="bold">{escape_xml(label)}</text>',
            ]
        )
    svg.append("</svg>")
    overlay_svg.write_text("\n".join(svg) + "\n")
    convert = subprocess.run(
        [sips, "-s", "format", "png", str(overlay_svg), "--out", str(overlay_png)],
        capture_output=True,
        text=True,
        check=False,
    )
    if convert.returncode:
        raise RuntimeError(convert.stderr)
    merge = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            "-i",
            str(raw),
            "-i",
            str(overlay_png),
            "-filter_complex",
            "[0:v][1:v]overlay=0:0:format=auto[board]",
            "-map",
            "[board]",
            "-frames:v",
            "1",
            str(board),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    if merge.returncode:
        raise RuntimeError(merge.stderr)
    for temporary in (raw, overlay_svg, overlay_png):
        temporary.unlink(missing_ok=True)
    return artifact(board) | {"resolution": [width, height]}


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    panels_root = output_root / "panels"
    panels_root.mkdir(parents=True, exist_ok=True)
    for path in panels_root.glob("*.png"):
        path.unlink()
    for name in (
        "bastard_sword_original_vs_canonical.png",
        "source_steel_diffuse.png",
        "source_steel_normal.png",
        "canonical_macro.png",
        "canonical_grind.png",
        "manifest.json",
    ):
        (output_root / name).unlink(missing_ok=True)

    source_before = sha256_file(SOURCE)
    canonical_before = sha256_file(CANONICAL_BUILDER)
    if source_before != SOURCE_SHA256 or canonical_before != CANONICAL_SHA256:
        raise RuntimeError("comparison authority drifted")

    canonical = load_module(CANONICAL_BUILDER, "iggy_compare_canonical")
    study = load_module(STUDY_BUILDER, "iggy_compare_study")
    response = load_module(RESPONSE_BUILDER, "iggy_compare_response")
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
    diffuse_path = output_root / "source_steel_diffuse.png"
    normal_path = output_root / "source_steel_normal.png"
    extract_packed_image("steel_diffuse.png", diffuse_path, DIFFUSE_SHA256)
    extract_packed_image("steel_normal.png", normal_path, NORMAL_SHA256)

    source = bpy.data.objects["BastardSword"]
    component = study.classify_components(study.connected_components(source))["blade"][0]
    blade, uv_record = copy_blade_preserving_uv(source, component)
    region_record = response.author_blade_uv_and_regions(blade, "BastardSword", canonical)
    response.center_and_orient_blade(blade)
    for obj in list(bpy.data.objects):
        if obj is not blade:
            bpy.data.objects.remove(obj, do_unlink=True)

    profile, conductor, tangent = canonical.load_contracts()
    macro_path = output_root / "canonical_macro.png"
    grind_path = output_root / "canonical_grind.png"
    canonical.common.write_png_gray16(macro_path, canonical.generate_macro_polish_field(profile))
    canonical.common.write_png_gray16(grind_path, canonical.generate_grind_field(profile))
    if sha256_file(macro_path) != MACRO_SHA256 or sha256_file(grind_path) != GRIND_SHA256:
        raise RuntimeError("canonical maps changed")

    diffuse_image = bpy.data.images.load(str(diffuse_path), check_existing=False)
    diffuse_image.colorspace_settings.name = "sRGB"
    normal_image = bpy.data.images.load(str(normal_path), check_existing=False)
    normal_image.colorspace_settings.name = "Non-Color"
    macro_image = bpy.data.images.load(str(macro_path), check_existing=False)
    macro_image.colorspace_settings.name = "Non-Color"
    grind_image = bpy.data.images.load(str(grind_path), check_existing=False)
    grind_image.colorspace_settings.name = "Non-Color"
    original_material, original_contract = build_original_map_material(diffuse_image, normal_image)
    canonical_material, canonical_contract = canonical.build_sword_steel_material(
        profile,
        conductor,
        tangent,
        macro_image,
        grind_image,
    )

    scene, camera, lights = response.configure_scene(args.samples)
    scene.render.resolution_x = PANEL_WIDTH
    scene.render.resolution_y = PANEL_HEIGHT
    panels: dict[str, dict[str, Any]] = {}
    for name, _, view, owner in VIEWS:
        set_material(blade, original_material if owner == "original" else canonical_material)
        response.configure_view(scene, camera, lights, blade, view)
        panel_path = panels_root / f"{name}.png"
        panels[name] = response.render_panel(scene, panel_path)
    board = assemble_board(output_root, panels)

    source_after = sha256_file(SOURCE)
    canonical_after = sha256_file(CANONICAL_BUILDER)
    if source_after != source_before or canonical_after != canonical_before:
        raise RuntimeError("comparison mutated a frozen authority")
    manifest = {
        "schema": "iggy3d.bastard-sword-original-vs-canonical.v1",
        "status": "VISUAL_COMPARISON_NOT_ACCEPTANCE",
        "source": {"path": str(SOURCE), "sha256_before": source_before, "sha256_after": source_after},
        "canonical_builder": {
            "path": str(CANONICAL_BUILDER),
            "sha256_before": canonical_before,
            "sha256_after": canonical_after,
        },
        "blade": region_record | uv_record,
        "original_material": original_contract,
        "canonical_material": canonical_contract,
        "maps": {
            "source_diffuse": artifact(diffuse_path),
            "source_normal": artifact(normal_path),
            "canonical_macro": artifact(macro_path),
            "canonical_grind": artifact(grind_path),
        },
        "panels": panels,
        "board": board,
        "matched_conditions": {
            "same_geometry": True,
            "same_camera": True,
            "same_lights": True,
            "same_exposure": True,
            "same_samples": args.samples,
        },
        "limitations": [
            "The Blender 2.7-era source material graph is broken after Blender 5.1 conversion.",
            "The source diffuse and normal are exact; metallic 1 and roughness 0.5 are comparison-only carrier values.",
            "The packed normal-map convention is undocumented and is used without channel inversion.",
        ],
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
    }
    (output_root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"BASTARD_SWORD_COMPARISON={board['path']}")


if __name__ == "__main__":
    main()
