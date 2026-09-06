#!/usr/bin/env python3
"""Render the unchanged clean-steel graph on four licensed donor blades."""

from __future__ import annotations

import argparse
from bisect import bisect_right
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any

import bpy
from mathutils import Matrix, Vector


PACKAGE_ROOT = Path(__file__).resolve().parent
CANONICAL_BUILDER_PATH = PACKAGE_ROOT / "build_sword_steel_v1.py"
STUDY_BUILDER_PATH = PACKAGE_ROOT / "build_historical_swords_set_study_v1.py"
STUDY_MANIFEST_PATH = (
    PACKAGE_ROOT / "output" / "historical_swords_set_study_v1" / "manifest.json"
)
REVIEW_PATH = PACKAGE_ROOT / "HISTORICAL_SWORDS_CLEAN_STEEL_RESPONSE.md"
DEFAULT_OUTPUT = PACKAGE_ROOT / "output" / "historical_swords_clean_steel_response_v1"
PANEL_WIDTH = 720
PANEL_HEIGHT = 240
SWORD_ORDER = ("ArmingSword", "BastardSword", "LongSword", "ClaymoreSword")
VIEW_ORDER = ("neutral", "grazing", "gameplay", "regions")
VIEW_LABELS = {
    "neutral": "NEUTRAL STEEL RESPONSE",
    "grazing": "GRAZING STRIP RESPONSE",
    "gameplay": "GAMEPLAY DISTANCE",
    "regions": "BODY / FULLER / BEVEL / EDGE",
}
EXPECTED_CANONICAL_SHA256 = (
    "76030d14d442ad2c1d4550b6d150c82caa536a8ac63ee8a94208a9e51ead7bbb"
)
EXPECTED_SOURCE_SHA256 = (
    "b090eddf53f63cb8fd8a3bfac0c3dbe36749d934ca079595485629c2a2eb55de"
)
EXPECTED_MACRO_SHA256 = (
    "dcffc56eed667746218c001503e923defcc9b9a8855b3ace371237d2cb084e35"
)
EXPECTED_GRIND_SHA256 = (
    "52c7b2f15c8a176d67d9c0d6e722f49f3b2be86c511eea7a6a438eca902f6c63"
)
BLADE_UV_NAME = "IGGY_BladeUV"
REGION_ATTRIBUTE_NAME = "sinc_sword_region"
REGION_VECTORS = {
    "body": (1.0, 0.0, 0.0, 0.0),
    "fuller": (0.0, 1.0, 0.0, 0.0),
    "bevel": (0.0, 0.0, 1.0, 0.0),
    "edge": (0.0, 0.0, 0.0, 1.0),
}

# Replaced only after inspection of the original 2880x960 board and panels.
VISUAL_REVIEW = {
    "decision": "diagnostic_response_pass_with_geometry_gaps",
    "strongest_geometry_separation": (
        "BastardSword: the recessed green fuller lane produces a distinct "
        "longitudinal highlight/value channel and terminates before the point."
    ),
    "weakest_geometry_separation": (
        "LongSword: its nearly constant-thickness broad face stays comparatively "
        "uniform, confirming the geometry-study negative control."
    ),
    "semantic_adapter_findings": [
        "ArmingSword has no separately modelled edge-land polygons; the edge lane remains honestly absent.",
        "BastardSword cross-section rise produces a 24-polygon fuller band that releases between the audited 50% and 75% stations.",
        "ClaymoreSword retains only four recessed forte polygons before simplifying to its distal section.",
        "Production swords require authored face roles; this inferred adapter is diagnostic only.",
    ],
    "material_response_findings": [
        "One unchanged material produces visibly different neutral and grazing responses on all four sections.",
        "The BastardSword fuller remains legible at the supplied gameplay proof distance.",
        "ArmingSword compound planes read, but no independent polished cutting-edge response can exist without edge geometry.",
        "ClaymoreSword forte reinforcement is locally visible and then releases into a quiet distal blade.",
        "LongSword proves the shader does not rescue a broad constant-thickness construction.",
    ],
    "known_limitations": [
        "Canonical whole-blade fields are normalized to each donor UV domain, so their physical span varies by sword.",
        "The low-poly donor faces and inferred semantics are not production-authored sword topology.",
        "No saved blend, real-game sword acceptance, manual material acceptance, or Unreal parity is established.",
    ],
    "manual_acceptance_established": False,
}


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--samples", type=int, default=16)
    parser.add_argument("--review-only", action="store_true")
    return parser.parse_args(argv)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def artifact_record(path: Path) -> dict[str, Any]:
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def load_module(path: Path, name: str) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"unable to load module: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_canonical_builder() -> Any:
    return load_module(CANONICAL_BUILDER_PATH, "iggy_canonical_sword_steel")


def load_study_builder() -> Any:
    return load_module(STUDY_BUILDER_PATH, "iggy_historical_sword_study")


def clear_generated_artifacts(output_root: Path) -> None:
    panels_root = output_root / "panels"
    panels_root.mkdir(parents=True, exist_ok=True)
    for path in panels_root.glob("*.png"):
        path.unlink()
    for name in (
        "_raw_board.png",
        "_board_overlay.svg",
        "_board_overlay.png",
        "historical_swords_clean_steel_response_board.png",
    ):
        (output_root / name).unlink(missing_ok=True)


def blade_ring_envelopes(
    mesh: bpy.types.Mesh,
) -> tuple[float, float, list[tuple[float, float, float]]]:
    minimum_x = min(vertex.co.x for vertex in mesh.vertices)
    maximum_x = max(vertex.co.x for vertex in mesh.vertices)
    minimum_z = min(vertex.co.z for vertex in mesh.vertices)
    maximum_z = max(vertex.co.z for vertex in mesh.vertices)
    center_x = 0.5 * (minimum_x + maximum_x)
    center_z = 0.5 * (minimum_z + maximum_z)
    rings: dict[float, list[float]] = {}
    for vertex in mesh.vertices:
        key = round(float(vertex.co.y), 7)
        record = rings.setdefault(key, [0.0, 0.0])
        record[0] = max(record[0], abs(float(vertex.co.x) - center_x))
        record[1] = max(record[1], abs(float(vertex.co.z) - center_z))
    ordered = [
        (position, max(values[0], 1.0e-7), max(values[1], 1.0e-7))
        for position, values in sorted(rings.items())
    ]
    if len(ordered) < 2:
        raise RuntimeError("blade adapter requires at least two longitudinal rings")
    return center_x, center_z, ordered


def interpolated_envelope(
    rings: list[tuple[float, float, float]],
    position_y: float,
) -> tuple[float, float]:
    positions = [ring[0] for ring in rings]
    upper = bisect_right(positions, position_y)
    if upper <= 0:
        return rings[0][1], rings[0][2]
    if upper >= len(rings):
        return rings[-1][1], rings[-1][2]
    lower_ring = rings[upper - 1]
    upper_ring = rings[upper]
    span = max(upper_ring[0] - lower_ring[0], 1.0e-9)
    factor = min(max((position_y - lower_ring[0]) / span, 0.0), 1.0)
    half_width = lower_ring[1] + factor * (upper_ring[1] - lower_ring[1])
    half_thickness = lower_ring[2] + factor * (upper_ring[2] - lower_ring[2])
    return max(half_width, 1.0e-7), max(half_thickness, 1.0e-7)


def classify_longitudinal_face(
    sword_name: str,
    normal: Vector,
    x_norm: float,
    z_norm: float,
    cross_section_rise: float,
    inner_x_norm: float,
) -> str:
    if abs(x_norm) > 0.82 and abs(normal.x) > 0.72:
        return "edge"
    has_recess = sword_name in {"BastardSword", "ClaymoreSword"}
    if (
        has_recess
        and inner_x_norm < 0.40
        and cross_section_rise > 0.08
    ):
        return "fuller"
    if abs(x_norm) > 0.52 or abs(normal.x) > 0.55:
        return "bevel"
    return "body"


def polygon_cross_section_rise(
    mesh: bpy.types.Mesh,
    polygon: bpy.types.MeshPolygon,
    center_x: float,
    center_z: float,
    rings: list[tuple[float, float, float]],
) -> tuple[float, float]:
    station_groups: dict[float, list[tuple[float, float]]] = {}
    for vertex_index in polygon.vertices:
        coordinate = mesh.vertices[int(vertex_index)].co
        half_width, half_thickness = interpolated_envelope(rings, float(coordinate.y))
        station_groups.setdefault(round(float(coordinate.y), 7), []).append(
            (
                abs((float(coordinate.x) - center_x) / half_width),
                abs((float(coordinate.z) - center_z) / half_thickness),
            )
        )
    rises: list[float] = []
    inner_positions: list[float] = []
    for points in station_groups.values():
        if len(points) < 2:
            continue
        ordered = sorted(points, key=lambda point: point[0])
        inner, outer = ordered[0], ordered[-1]
        rises.append(outer[1] - inner[1])
        inner_positions.append(inner[0])
    if not rises:
        return -1.0, 1.0
    return max(rises), min(inner_positions)


def author_blade_uv_and_regions(
    obj: bpy.types.Object,
    sword_name: str,
    canonical: Any,
) -> dict[str, Any]:
    mesh = obj.data
    if canonical.BLADE_UV != BLADE_UV_NAME or canonical.REGION_ATTRIBUTE != REGION_ATTRIBUTE_NAME:
        raise RuntimeError("canonical sword semantic names drifted")
    mesh.update()
    minimum_x = min(vertex.co.x for vertex in mesh.vertices)
    maximum_x = max(vertex.co.x for vertex in mesh.vertices)
    minimum_y = min(vertex.co.y for vertex in mesh.vertices)
    maximum_y = max(vertex.co.y for vertex in mesh.vertices)
    width_m = maximum_x - minimum_x
    length_m = maximum_y - minimum_y
    if width_m <= 0.0 or length_m <= 0.0:
        raise RuntimeError(f"{sword_name} has a degenerate blade frame")

    existing_uv = mesh.uv_layers.get(canonical.BLADE_UV)
    if existing_uv is not None:
        mesh.uv_layers.remove(existing_uv)
    uv_layer = mesh.uv_layers.new(name=canonical.BLADE_UV)
    for polygon in mesh.polygons:
        for loop_index in polygon.loop_indices:
            vertex = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            uv_layer.data[loop_index].uv = (
                (float(vertex.y) - minimum_y) / length_m,
                (float(vertex.x) - minimum_x) / width_m,
            )

    existing_region = mesh.color_attributes.get(canonical.REGION_ATTRIBUTE)
    if existing_region is not None:
        mesh.color_attributes.remove(existing_region)
    region = mesh.color_attributes.new(
        name=canonical.REGION_ATTRIBUTE,
        type="FLOAT_COLOR",
        domain="CORNER",
    )
    center_x, center_z, rings = blade_ring_envelopes(mesh)
    counts = {name: 0 for name in REGION_VECTORS}
    polygon_regions: list[str] = []
    for polygon in mesh.polygons:
        if abs(float(polygon.normal.y)) > 0.60:
            semantic = "body"
        else:
            half_width, half_thickness = interpolated_envelope(
                rings,
                float(polygon.center.y),
            )
            x_norm = (float(polygon.center.x) - center_x) / half_width
            z_norm = (float(polygon.center.z) - center_z) / half_thickness
            cross_section_rise, inner_x_norm = polygon_cross_section_rise(
                mesh,
                polygon,
                center_x,
                center_z,
                rings,
            )
            semantic = classify_longitudinal_face(
                sword_name,
                polygon.normal,
                x_norm,
                z_norm,
                cross_section_rise,
                inner_x_norm,
            )
        polygon_regions.append(semantic)
        counts[semantic] += 1
        for loop_index in polygon.loop_indices:
            region.data[loop_index].color = REGION_VECTORS[semantic]

    one_hot_errors = 0
    for loop in region.data:
        color = tuple(float(value) for value in loop.color)
        if abs(sum(color) - 1.0) > 1.0e-6 or sum(value > 0.5 for value in color) != 1:
            one_hot_errors += 1
    if sum(counts.values()) != len(mesh.polygons) or one_hot_errors:
        raise RuntimeError(f"{sword_name} region ownership is not one-hot")
    if counts["body"] == 0 or counts["bevel"] == 0:
        raise RuntimeError(f"{sword_name} lost a required steel region: {counts}")
    if sword_name in {"ArmingSword", "LongSword"} and counts["fuller"]:
        raise RuntimeError(f"{sword_name} gained an invented fuller")
    if sword_name in {"BastardSword", "ClaymoreSword"} and not counts["fuller"]:
        raise RuntimeError(f"{sword_name} lost its audited recessed planes")

    for polygon in mesh.polygons:
        polygon.use_smooth = False
    mesh.update()
    obj["iggy_diagnostic_sword_name"] = sword_name
    obj["iggy_region_classifier"] = "model_section_normals_v1"
    obj["iggy_source_geometry_copy"] = True
    return {
        "source_object": sword_name,
        "proof_object": obj.name,
        "vertex_count": len(mesh.vertices),
        "polygon_count": len(mesh.polygons),
        "dimensions_local_m": [
            round(width_m, 9),
            round(length_m, 9),
            round(
                max(vertex.co.z for vertex in mesh.vertices)
                - min(vertex.co.z for vertex in mesh.vertices),
                9,
            ),
        ],
        "region_polygon_counts": counts,
        "edge_lane_state": (
            "modelled" if counts["edge"] else "absent_zero_width_source_edge"
        ),
        "one_hot_loop_error_count": one_hot_errors,
        "uv_layer": canonical.BLADE_UV,
        "region_attribute": canonical.REGION_ATTRIBUTE,
        "uv_contract": "normalized donor root-to-tip U and edge-to-edge V",
        "region_classifier": "model_section_normals_v1",
        "polygon_regions": polygon_regions,
    }


def center_and_orient_blade(obj: bpy.types.Object) -> None:
    coordinates = [vertex.co.copy() for vertex in obj.data.vertices]
    minimum = Vector(tuple(min(coordinate[axis] for coordinate in coordinates) for axis in range(3)))
    maximum = Vector(tuple(max(coordinate[axis] for coordinate in coordinates) for axis in range(3)))
    center = (minimum + maximum) * 0.5
    for vertex in obj.data.vertices:
        vertex.co -= center
    obj.data.update()
    obj.matrix_world = Matrix.Rotation(-math.pi * 0.5, 4, "Z")


def prepare_donor_blades(
    source_path: Path,
    canonical: Any,
    study: Any,
) -> tuple[dict[str, bpy.types.Object], dict[str, dict[str, Any]]]:
    bpy.ops.wm.open_mainfile(filepath=str(source_path))
    blades: dict[str, bpy.types.Object] = {}
    records: dict[str, dict[str, Any]] = {}
    for sword_name in SWORD_ORDER:
        source = bpy.data.objects.get(sword_name)
        if source is None or source.type != "MESH":
            raise RuntimeError(f"source donor lost {sword_name}")
        components = study.connected_components(source)
        construction = study.classify_components(components)
        blade_component = construction["blade"][0]
        blade = study.create_component_object(
            source,
            blade_component,
            f"IGGY_DIAG_{sword_name}_Blade",
        )
        blade.matrix_world = Matrix.Identity(4)
        records[sword_name] = author_blade_uv_and_regions(blade, sword_name, canonical)
        center_and_orient_blade(blade)
        blade.hide_render = True
        blades[sword_name] = blade

    keep = {blade.name for blade in blades.values()}
    for obj in list(bpy.data.objects):
        if obj.name not in keep:
            bpy.data.objects.remove(obj, do_unlink=True)
    for scene in bpy.data.scenes:
        scene.render.filepath = ""
    return blades, records


def point_at(obj: bpy.types.Object, target: Vector) -> None:
    obj.rotation_euler = (target - obj.location).to_track_quat("-Z", "Y").to_euler()


def create_area_light(
    scene: bpy.types.Scene,
    name: str,
    location: tuple[float, float, float],
    energy: float,
    size: float,
    size_y: float,
) -> bpy.types.Object:
    data = bpy.data.lights.new(name, "AREA")
    data.shape = "RECTANGLE"
    data.energy = energy
    data.size = size
    data.size_y = size_y
    obj = bpy.data.objects.new(name, data)
    obj.location = location
    scene.collection.objects.link(obj)
    point_at(obj, Vector((0.0, 0.0, 0.0)))
    return obj


def configure_scene(samples: int) -> tuple[bpy.types.Scene, bpy.types.Object, dict[str, bpy.types.Object]]:
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.render.resolution_x = PANEL_WIDTH
    scene.render.resolution_y = PANEL_HEIGHT
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.film_transparent = False
    scene.view_settings.look = "Medium High Contrast"
    scene.view_settings.exposure = 0.0
    if scene.world is None:
        scene.world = bpy.data.worlds.new("IGGY_HistoricalSwordResponseWorld")
    scene.world.use_nodes = True
    background = scene.world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.007, 0.009, 0.014, 1.0)
    background.inputs["Strength"].default_value = 0.025

    camera_data = bpy.data.cameras.new("IGGY_HistoricalSwordResponseCamera")
    camera_data.type = "ORTHO"
    camera = bpy.data.objects.new("IGGY_HistoricalSwordResponseCamera", camera_data)
    scene.collection.objects.link(camera)
    scene.camera = camera
    lights = {
        "key": create_area_light(scene, "IGGY_Response_Key", (-0.20, -0.32, 0.54), 10.0, 0.56, 0.07),
        "fill": create_area_light(scene, "IGGY_Response_Fill", (0.36, 0.34, 0.32), 2.5, 0.50, 0.14),
        "grazing": create_area_light(scene, "IGGY_Response_Grazing", (-0.43, -0.11, 0.055), 12.0, 0.72, 0.012),
    }
    return scene, camera, lights


def isolate_blade(blades: dict[str, bpy.types.Object], active: bpy.types.Object) -> None:
    for blade in blades.values():
        blade.hide_render = blade is not active


def configure_view(
    scene: bpy.types.Scene,
    camera: bpy.types.Object,
    lights: dict[str, bpy.types.Object],
    blade: bpy.types.Object,
    view: str,
) -> None:
    dimensions = blade.dimensions
    aspect = scene.render.resolution_x / scene.render.resolution_y
    in_plane = sorted((float(dimensions.x), float(dimensions.y)))
    width_m, length_m = in_plane[0], in_plane[1]
    base_scale = max(length_m * 1.35, width_m * aspect * 2.8)
    camera.data.ortho_scale = base_scale * (1.72 if view == "gameplay" else 1.0)
    camera.location = Vector((0.0, -0.15, 0.68))
    point_at(camera, Vector((0.0, 0.0, 0.0)))
    for light in lights.values():
        light.hide_render = True
    if view in {"neutral", "gameplay"}:
        lights["key"].hide_render = False
        lights["fill"].hide_render = False
    elif view == "grazing":
        lights["grazing"].hide_render = False
        lights["fill"].hide_render = False
        lights["fill"].data.energy = 0.45
    elif view != "regions":
        raise KeyError(view)
    if view != "grazing":
        lights["fill"].data.energy = 2.5


def render_panel(scene: bpy.types.Scene, path: Path) -> dict[str, Any]:
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return artifact_record(path)


def escape_xml(value: str) -> str:
    return (
        value.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def rasterize_svg(svg_path: Path, png_path: Path) -> None:
    sips = shutil.which("sips")
    if sips is None:
        raise RuntimeError("sips is required for response-board labels")
    completed = subprocess.run(
        [sips, "-s", "format", "png", str(svg_path), "--out", str(png_path)],
        capture_output=True,
        text=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError("response-board label rasterization failed: " + completed.stderr)


def write_board_overlay(path: Path) -> None:
    width = PANEL_WIDTH * len(VIEW_ORDER)
    height = PANEL_HEIGHT * len(SWORD_ORDER)
    rows = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">']
    for row_index, sword_name in enumerate(SWORD_ORDER):
        for column_index, view in enumerate(VIEW_ORDER):
            x = column_index * PANEL_WIDTH
            y = row_index * PANEL_HEIGHT
            rows.extend(
                [
                    f'<rect x="{x}" y="{y}" width="{PANEL_WIDTH}" height="34" fill="#090b0f" fill-opacity="0.90"/>',
                    f'<text x="{x + 12}" y="{y + 22}" fill="#f2f4f6" font-family="Arial" font-size="14" font-weight="bold">{escape_xml(sword_name)} / {escape_xml(VIEW_LABELS[view])}</text>',
                ]
            )
    rows.append("</svg>")
    path.write_text("\n".join(rows) + "\n")


def assemble_board(output_root: Path, panels: dict[str, dict[str, Any]]) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for response-board assembly")
    ordered = [
        Path(panels[f"{sword}:{view}"]["path"])
        for sword in SWORD_ORDER
        for view in VIEW_ORDER
    ]
    raw = output_root / "_raw_board.png"
    overlay_svg = output_root / "_board_overlay.svg"
    overlay_png = output_root / "_board_overlay.png"
    board = output_root / "historical_swords_clean_steel_response_board.png"
    layout = [
        f"{column * PANEL_WIDTH}_{row * PANEL_HEIGHT}"
        for row in range(len(SWORD_ORDER))
        for column in range(len(VIEW_ORDER))
    ]
    stack = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum((["-i", str(path)] for path in ordered), []),
            "-filter_complex",
            f"xstack=inputs={len(ordered)}:layout={'|'.join(layout)}[board]",
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
    if stack.returncode != 0:
        raise RuntimeError("response-board stack failed: " + stack.stderr)
    write_board_overlay(overlay_svg)
    rasterize_svg(overlay_svg, overlay_png)
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
    if merge.returncode != 0:
        raise RuntimeError("response-board overlay failed: " + merge.stderr)
    for temporary in (raw, overlay_svg, overlay_png):
        temporary.unlink(missing_ok=True)
    record = artifact_record(board)
    record["resolution"] = [
        PANEL_WIDTH * len(VIEW_ORDER),
        PANEL_HEIGHT * len(SWORD_ORDER),
    ]
    record["layout"] = "four sword rows by neutral, grazing, gameplay, and region columns"
    return record


def review_only(output_root: Path) -> None:
    manifest_path = output_root / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    expected = {manifest["board"]["path"]: manifest["board"]["sha256"]}
    expected.update({record["path"]: record["sha256"] for record in manifest["panels"].values()})
    expected.update({record["path"]: record["sha256"] for record in manifest["maps"].values()})
    for path_string, digest in expected.items():
        path = Path(path_string)
        if not path.is_file() or sha256_file(path) != digest:
            raise RuntimeError(f"response artifact changed before review: {path}")
    if sha256_file(CANONICAL_BUILDER_PATH) != manifest["canonical_builder"]["sha256_after"]:
        raise RuntimeError("canonical sword shader changed before review")
    source_path = Path(manifest["source"]["path"])
    if sha256_file(source_path) != manifest["source"]["sha256_after"]:
        raise RuntimeError("historical sword donor changed before review")
    if not REVIEW_PATH.is_file():
        raise FileNotFoundError(REVIEW_PATH)
    manifest["visual_review"] = VISUAL_REVIEW
    manifest["review_document"] = artifact_record(REVIEW_PATH)
    manifest["script"]["sha256"] = sha256_file(Path(__file__).resolve())
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(f"HISTORICAL_SWORD_RESPONSE_REVIEW={VISUAL_REVIEW['decision']}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if args.review_only:
        review_only(output_root)
        return
    clear_generated_artifacts(output_root)

    canonical_hash_before = sha256_file(CANONICAL_BUILDER_PATH)
    if canonical_hash_before != EXPECTED_CANONICAL_SHA256:
        raise RuntimeError("canonical sword-steel builder drifted before response proof")
    study_manifest = json.loads(STUDY_MANIFEST_PATH.read_text())
    source_path = Path(study_manifest["source"]["path"])
    source_hash_before = sha256_file(source_path)
    if source_hash_before != EXPECTED_SOURCE_SHA256:
        raise RuntimeError("historical sword source drifted before response proof")

    canonical = load_canonical_builder()
    study = load_study_builder()
    profile, conductor, tangent = canonical.load_contracts()
    macro_field = canonical.generate_macro_polish_field(profile)
    grind_field = canonical.generate_grind_field(profile)
    macro_path = output_root / "canonical_sword_steel_macro_polish.png"
    grind_path = output_root / "canonical_sword_steel_grind_field.png"
    canonical.common.write_png_gray16(macro_path, macro_field)
    canonical.common.write_png_gray16(grind_path, grind_field)
    if sha256_file(macro_path) != EXPECTED_MACRO_SHA256:
        raise RuntimeError("response proof changed the canonical macro map")
    if sha256_file(grind_path) != EXPECTED_GRIND_SHA256:
        raise RuntimeError("response proof changed the canonical grind map")

    blades, sword_records = prepare_donor_blades(source_path, canonical, study)
    scene, camera, lights = configure_scene(args.samples)
    macro_image = bpy.data.images.load(str(macro_path), check_existing=False)
    macro_image.name = "IGGY_IMG_Response_CanonicalMacro"
    macro_image.colorspace_settings.name = "Non-Color"
    grind_image = bpy.data.images.load(str(grind_path), check_existing=False)
    grind_image.name = "IGGY_IMG_Response_CanonicalGrind"
    grind_image.colorspace_settings.name = "Non-Color"
    physical, material_contract = canonical.build_sword_steel_material(
        profile,
        conductor,
        tangent,
        macro_image,
        grind_image,
    )
    region_material = canonical.build_region_material()
    for blade in blades.values():
        canonical.set_material(blade, physical)

    panels_root = output_root / "panels"
    panels: dict[str, dict[str, Any]] = {}
    for sword_name in SWORD_ORDER:
        blade = blades[sword_name]
        isolate_blade(blades, blade)
        for view in VIEW_ORDER:
            canonical.set_material(
                blade,
                region_material if view == "regions" else physical,
            )
            configure_view(scene, camera, lights, blade, view)
            path = panels_root / f"{sword_name}_{view}.png"
            panels[f"{sword_name}:{view}"] = render_panel(scene, path)
        canonical.set_material(blade, physical)

    board = assemble_board(output_root, panels)
    canonical_hash_after = sha256_file(CANONICAL_BUILDER_PATH)
    source_hash_after = sha256_file(source_path)
    if canonical_hash_after != canonical_hash_before:
        raise RuntimeError("response proof modified the canonical sword shader")
    if source_hash_after != source_hash_before:
        raise RuntimeError("response proof modified the historical sword donor")
    material_contract["shared_consumer_count"] = len(blades)
    material_contract["graph_owner"] = str(CANONICAL_BUILDER_PATH)
    manifest = {
        "schema": "iggy3d.historical-swords-clean-steel-response.v1",
        "status": "DIAGNOSTIC_MULTI_GEOMETRY_RESPONSE_NOT_ACCEPTANCE",
        "canonical_builder": {
            "path": str(CANONICAL_BUILDER_PATH),
            "sha256_before": canonical_hash_before,
            "sha256_after": canonical_hash_after,
        },
        "source": {
            "path": str(source_path),
            "creator": study_manifest["source"]["creator"],
            "selected_license": study_manifest["source"]["selected_license"],
            "sha256_before": source_hash_before,
            "sha256_after": source_hash_after,
        },
        "adapter": {
            "version": "model_section_normals_v1",
            "source_frame": {"length": "+Y", "width": "X", "thickness": "Z"},
            "proof_frame": {"length": "+X", "width": "-Y", "thickness": "Z"},
            "uv": "normalized donor root-to-tip U and edge-to-edge V",
            "map_span_caveat": "canonical whole-blade fields scale to each donor normalized UV domain",
            "fuller_eligible_models": ["BastardSword", "ClaymoreSword"],
            "thresholds": {
                "cap_normal_y": 0.60,
                "edge_x_norm": 0.82,
                "edge_normal_x": 0.72,
                "fuller_x_norm": 0.40,
                "fuller_cross_section_rise": 0.08,
                "bevel_x_norm": 0.52,
                "bevel_normal_x": 0.55,
            },
        },
        "material": material_contract,
        "maps": {
            "macro": artifact_record(macro_path),
            "grind": artifact_record(grind_path),
        },
        "swords": sword_records,
        "proof_contract": {
            "panel_resolution": [PANEL_WIDTH, PANEL_HEIGHT],
            "board_resolution": [PANEL_WIDTH * 4, PANEL_HEIGHT * 4],
            "sword_order": list(SWORD_ORDER),
            "view_order": list(VIEW_ORDER),
            "cycles_samples": args.samples,
            "view_transform": scene.view_settings.view_transform,
            "look": scene.view_settings.look,
            "exposure": scene.view_settings.exposure,
            "shared_lights": {
                name: {
                    "energy": float(light.data.energy),
                    "size": float(light.data.size),
                    "size_y": float(light.data.size_y),
                }
                for name, light in lights.items()
            },
        },
        "panels": panels,
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "review_document": None,
        "source_materials_consumed": False,
        "source_geometry_modified": False,
        "saved_blend_created": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "uses_ai_generated_imagery": False,
        "uses_damage": False,
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "limitations": [
            "The donors are low-poly historical-ish models, not measured historical specimens.",
            "The normalized UV adapter changes the physical span of the whole-blade fields per donor.",
            "The geometry-derived semantic adapter is diagnostic and not production-authored face metadata.",
            "Blender response does not establish Unreal parity or material acceptance.",
        ],
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "HISTORICAL_SWORD_RESPONSE="
        f"swords:{len(blades)},panels:{len(panels)},"
        "shared_material:1,source_unchanged:true,canonical_unchanged:true"
    )
    print(f"HISTORICAL_SWORD_RESPONSE_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
