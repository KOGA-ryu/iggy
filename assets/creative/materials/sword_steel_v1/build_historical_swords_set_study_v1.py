#!/usr/bin/env python3
"""Build a non-destructive geometry study from a licensed four-sword donor."""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any
import urllib.request

import bpy
from mathutils import Vector


PACKAGE_ROOT = Path(__file__).resolve().parent
PROVENANCE_PATH = PACKAGE_ROOT / "references" / "historical_swords_set_v1.json"
DEFAULT_OUTPUT = PACKAGE_ROOT / "output" / "historical_swords_set_study_v1"
REVIEW_PATH = PACKAGE_ROOT / "HISTORICAL_SWORDS_SET_STUDY.md"
PANEL_WIDTH = 480
PANEL_HEIGHT = 560
OBJECT_ORDER = ("ArmingSword", "BastardSword", "LongSword", "ClaymoreSword")
PANEL_ORDER = ("whole_front", "hilt_oblique", "blade_oblique", "blade_topology")
PANEL_LABELS = {
    "whole_front": "WHOLE SILHOUETTE",
    "hilt_oblique": "HILT COMPONENTS / 3-4",
    "blade_oblique": "BLADE PLANES / 3-4",
    "blade_topology": "BLADE TOPOLOGY",
    "sections": "FIVE MODEL SECTIONS",
}

# Replaced only after original-resolution inspection of the hash-locked board.
VISUAL_REVIEW = {
    "decision": "usable_reference_study_with_source_limits",
    "best_fuller_study": (
        "BastardSword: its recessed 12-point section persists through the "
        "50% station and simplifies to eight points by 75%, making the model's "
        "fuller termination explicit."
    ),
    "best_compound_plane_study": (
        "ArmingSword: one eight-point compound section changes both width and "
        "thickness progressively along the blade."
    ),
    "best_transition_study": (
        "ClaymoreSword: the 20-point forte section simplifies to an eight-point "
        "section by 25%, exposing a deliberate plane transition."
    ),
    "negative_control": (
        "LongSword: thickness remains 6.293 mm at the 12%, 25%, 50%, and 75% "
        "stations, so the broad faces read over-uniform and provide a useful "
        "constant-thickness failure control."
    ),
    "transferable_lessons": [
        "Author blades as a series of longitudinal cross-section stations, not one extruded outline.",
        "Control profile taper and distal taper independently.",
        "Terminate or simplify a fuller before the point with an explicit topology transition.",
        "Give body, fuller, bevel, and edge real planes before asking the steel shader to reveal them.",
        "Keep blade, guard, grip, and pommel as separately inspectable construction components.",
    ],
    "source_limitations": [
        "The creator calls the set historical-ish; it is not a measured-sword survey.",
        "The low-poly game meshes simplify edge geometry, distal taper, and hilt construction.",
        "Original materials and textures were not used as finish evidence.",
        "Model dimensions describe only the downloaded meshes and are not historical facts.",
    ],
    "manual_acceptance_established": False,
}


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    argv = argv[argv.index("--") + 1 :] if "--" in argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=Path, default=DEFAULT_OUTPUT)
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


def clear_generated_artifacts(*roots: Path) -> None:
    """Remove only this builder's prior PNG/SVG proofs, never its source donor."""
    for root in roots:
        if not root.is_dir():
            continue
        for pattern in ("*.png", "*.svg"):
            for path in root.glob(pattern):
                path.unlink()


def fetch_hash_locked_source(
    provenance: dict[str, Any],
    source_path: Path,
) -> None:
    expected = provenance["source_archive"]
    source_path.parent.mkdir(parents=True, exist_ok=True)
    if not source_path.is_file():
        temporary = source_path.with_suffix(".download")
        urllib.request.urlretrieve(provenance["download_url"], temporary)
        temporary.replace(source_path)
    if source_path.stat().st_size != expected["bytes"]:
        raise RuntimeError("historical sword donor byte count drifted")
    if sha256_file(source_path) != expected["sha256"]:
        raise RuntimeError("historical sword donor hash drifted")


def mesh_bounds(
    mesh: bpy.types.Mesh,
    vertex_ids: set[int],
) -> tuple[Vector, Vector]:
    coordinates = [mesh.vertices[index].co for index in vertex_ids]
    minimum = Vector(tuple(min(coordinate[axis] for coordinate in coordinates) for axis in range(3)))
    maximum = Vector(tuple(max(coordinate[axis] for coordinate in coordinates) for axis in range(3)))
    return minimum, maximum


def connected_components(obj: bpy.types.Object) -> list[dict[str, Any]]:
    mesh = obj.data
    adjacency: list[list[int]] = [[] for _ in mesh.vertices]
    for edge in mesh.edges:
        a, b = (int(value) for value in edge.vertices)
        adjacency[a].append(b)
        adjacency[b].append(a)
    seen: set[int] = set()
    components: list[dict[str, Any]] = []
    for root in range(len(mesh.vertices)):
        if root in seen:
            continue
        stack = [root]
        seen.add(root)
        vertex_ids: set[int] = set()
        while stack:
            current = stack.pop()
            vertex_ids.add(current)
            for neighbor in adjacency[current]:
                if neighbor not in seen:
                    seen.add(neighbor)
                    stack.append(neighbor)
        polygons = [
            polygon
            for polygon in mesh.polygons
            if all(int(vertex) in vertex_ids for vertex in polygon.vertices)
        ]
        edges = [
            edge
            for edge in mesh.edges
            if all(int(vertex) in vertex_ids for vertex in edge.vertices)
        ]
        minimum, maximum = mesh_bounds(mesh, vertex_ids)
        dimensions = maximum - minimum
        material_counts = Counter(
            mesh.materials[polygon.material_index].name
            if polygon.material_index < len(mesh.materials)
            and mesh.materials[polygon.material_index] is not None
            else f"slot_{polygon.material_index}"
            for polygon in polygons
        )
        components.append(
            {
                "_vertex_ids": vertex_ids,
                "vertex_count": len(vertex_ids),
                "edge_count": len(edges),
                "polygon_count": len(polygons),
                "minimum_m": [float(value) for value in minimum],
                "maximum_m": [float(value) for value in maximum],
                "dimensions_m": [float(value) for value in dimensions],
                "center_m": [float(value) for value in (minimum + maximum) * 0.5],
                "material_polygon_counts": dict(material_counts),
            }
        )
    components.sort(
        key=lambda component: max(component["dimensions_m"]),
        reverse=True,
    )
    return components


def classify_components(
    components: list[dict[str, Any]],
) -> dict[str, list[dict[str, Any]]]:
    blade = max(components, key=lambda component: component["dimensions_m"][1])
    remaining = [component for component in components if component is not blade]
    leather = [
        component
        for component in remaining
        if component["material_polygon_counts"].get("LeatherDark", 0) > 0
    ]
    if len(leather) != 1:
        raise RuntimeError(f"expected one leather grip component, found {len(leather)}")
    grip = leather[0]
    steel = [component for component in remaining if component is not grip]
    pommel = min(steel, key=lambda component: component["center_m"][1])
    guard = [component for component in steel if component is not pommel]
    if not guard:
        raise RuntimeError("sword donor lost its guard components")
    return {
        "blade": [blade],
        "guard": guard,
        "grip": [grip],
        "pommel": [pommel],
    }


def public_component(component: dict[str, Any]) -> dict[str, Any]:
    return {
        key: value
        for key, value in component.items()
        if not key.startswith("_")
    }


def deduplicate_and_sort_section(
    points: list[tuple[float, float]],
) -> list[tuple[float, float]]:
    unique: list[tuple[float, float]] = []
    for point in points:
        if not any(
            abs(point[0] - other[0]) < 1.0e-6
            and abs(point[1] - other[1]) < 1.0e-6
            for other in unique
        ):
            unique.append(point)
    if len(unique) < 6:
        raise RuntimeError(f"section produced only {len(unique)} unique intersections")
    center_x = sum(point[0] for point in unique) / len(unique)
    center_z = sum(point[1] for point in unique) / len(unique)
    unique.sort(key=lambda point: math.atan2(point[1] - center_z, point[0] - center_x))
    return unique


def section_intersections(
    mesh: bpy.types.Mesh,
    component_vertex_ids: set[int],
    station_y_m: float,
) -> list[tuple[float, float]]:
    points: list[tuple[float, float]] = []
    for edge in mesh.edges:
        a, b = (int(value) for value in edge.vertices)
        if a not in component_vertex_ids or b not in component_vertex_ids:
            continue
        ca, cb = mesh.vertices[a].co, mesh.vertices[b].co
        da = float(ca.y - station_y_m)
        db = float(cb.y - station_y_m)
        if abs(da) < 1.0e-9:
            points.append((float(ca.x), float(ca.z)))
        if da * db < 0.0:
            t = -da / (db - da)
            points.append(
                (
                    float(ca.x + t * (cb.x - ca.x)),
                    float(ca.z + t * (cb.z - ca.z)),
                )
            )
    return deduplicate_and_sort_section(points)


def normalized_section(
    points: list[tuple[float, float]],
) -> tuple[list[list[float]], float, float]:
    minimum_x = min(point[0] for point in points)
    maximum_x = max(point[0] for point in points)
    minimum_z = min(point[1] for point in points)
    maximum_z = max(point[1] for point in points)
    center_x = 0.5 * (minimum_x + maximum_x)
    center_z = 0.5 * (minimum_z + maximum_z)
    half_width = 0.5 * (maximum_x - minimum_x)
    half_thickness = 0.5 * (maximum_z - minimum_z)
    if half_width <= 0.0 or half_thickness <= 0.0:
        raise RuntimeError("section has zero width or thickness")
    normalized = [
        [
            round((point[0] - center_x) / half_width, 6),
            round((point[1] - center_z) / half_thickness, 6),
        ]
        for point in points
    ]
    return normalized, 2.0 * half_width, 2.0 * half_thickness


def audit_sections(
    obj: bpy.types.Object,
    blade_component: dict[str, Any],
    station_fractions: list[float],
) -> list[dict[str, Any]]:
    minimum_y = blade_component["minimum_m"][1]
    maximum_y = blade_component["maximum_m"][1]
    length_m = maximum_y - minimum_y
    result: list[dict[str, Any]] = []
    for fraction in station_fractions:
        station_y_m = minimum_y + fraction * length_m
        points = section_intersections(
            obj.data,
            blade_component["_vertex_ids"],
            station_y_m,
        )
        normalized, width_m, thickness_m = normalized_section(points)
        result.append(
            {
                "station_fraction": fraction,
                "station_y_m": round(station_y_m, 9),
                "points_m": [[round(x, 9), round(z, 9)] for x, z in points],
                "normalized_points": normalized,
                "width_m": round(width_m, 9),
                "thickness_m": round(thickness_m, 9),
                "intersection_count": len(points),
            }
        )
    return result


def create_component_object(
    source: bpy.types.Object,
    component: dict[str, Any],
    name: str,
) -> bpy.types.Object:
    vertex_ids = component["_vertex_ids"]
    ordered = sorted(vertex_ids)
    remap = {source_index: target_index for target_index, source_index in enumerate(ordered)}
    vertices = [tuple(source.data.vertices[index].co) for index in ordered]
    faces = [
        tuple(remap[int(vertex)] for vertex in polygon.vertices)
        for polygon in source.data.polygons
        if all(int(vertex) in vertex_ids for vertex in polygon.vertices)
    ]
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.matrix_world = source.matrix_world.copy()
    obj.color = (0.34, 0.37, 0.41, 1.0)
    obj["iggy_reference_source_object"] = source.name
    obj["iggy_reference_role"] = "blade_geometry_study"
    obj.hide_render = True
    return obj


def combine_components(components: list[dict[str, Any]]) -> dict[str, Any]:
    """Create one render-only component record without altering donor geometry."""
    vertex_ids: set[int] = set()
    for component in components:
        vertex_ids.update(component["_vertex_ids"])
    if not vertex_ids:
        raise RuntimeError("cannot combine an empty component collection")
    return {"_vertex_ids": vertex_ids}


def object_world_bounds(obj: bpy.types.Object) -> tuple[Vector, Vector, Vector]:
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    minimum = Vector(tuple(min(corner[axis] for corner in corners) for axis in range(3)))
    maximum = Vector(tuple(max(corner[axis] for corner in corners) for axis in range(3)))
    return minimum, maximum, (minimum + maximum) * 0.5


def point_camera(camera: bpy.types.Object, center: Vector) -> None:
    camera.rotation_euler = (center - camera.location).to_track_quat("-Z", "Y").to_euler()


def isolate_render(targets: list[bpy.types.Object]) -> None:
    target_ids = {id(target) for target in targets}
    for obj in bpy.data.objects:
        if obj.type == "MESH":
            obj.hide_render = id(obj) not in target_ids


def frame_camera(
    camera: bpy.types.Object,
    target: bpy.types.Object,
    view: str,
) -> None:
    minimum, maximum, center = object_world_bounds(target)
    span = maximum - minimum
    distance = max(span.y, 1.0) * 1.9
    if view in {"whole_front", "blade_topology"}:
        camera.location = center + Vector((0.0, 0.0, distance))
    elif view in {"hilt_oblique", "blade_oblique"}:
        # Keep the blade-length axis vertical in frame. A Y component here
        # rolls this very long target and produces a misleading diagonal view.
        camera.location = center + Vector((0.52, 0.0, 1.0)).normalized() * distance
    else:
        raise KeyError(view)
    point_camera(camera, center)
    aspect = PANEL_WIDTH / PANEL_HEIGHT
    if view == "blade_topology":
        # Roll the long blade across the wide panel so its topology is large
        # enough to inspect instead of collapsing into two vertical pixels.
        camera.rotation_euler.rotate_axis("Z", math.radians(90.0))
        camera.data.ortho_scale = max(span.x * 1.35, span.y / aspect * 1.10)
    else:
        camera.data.ortho_scale = max(span.y * 1.10, span.x / aspect * 1.18)


def configure_scene() -> tuple[bpy.types.Scene, bpy.types.Object]:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = PANEL_WIDTH
    scene.render.resolution_y = PANEL_HEIGHT
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.film_transparent = False
    scene.display.shading.light = "STUDIO"
    scene.display.shading.studio_light = "rim.sl"
    scene.display.shading.color_type = "OBJECT"
    scene.display.shading.show_shadows = True
    scene.display.shading.show_cavity = True
    scene.display.shading.cavity_type = "WORLD"
    scene.display.shading.curvature_ridge_factor = 1.6
    scene.display.shading.curvature_valley_factor = 1.6
    scene.display.shading.show_specular_highlight = True
    scene.world.color = (0.018, 0.018, 0.022)
    camera_data = bpy.data.cameras.new("IGGY_SwordStudyCamera")
    camera_data.type = "ORTHO"
    camera = bpy.data.objects.new("IGGY_SwordStudyCamera", camera_data)
    bpy.context.collection.objects.link(camera)
    scene.camera = camera
    return scene, camera


def render_geometry_panel(
    scene: bpy.types.Scene,
    camera: bpy.types.Object,
    targets: list[bpy.types.Object],
    framing_target: bpy.types.Object,
    view: str,
    path: Path,
) -> dict[str, Any]:
    isolate_render(targets)
    frame_camera(camera, framing_target, view)
    if hasattr(scene.display.shading, "studiolight_rotate_z"):
        scene.display.shading.studiolight_rotate_z = (
            0.75 if view in {"hilt_oblique", "blade_oblique"} else 0.0
        )
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return artifact_record(path)


def make_topology_overlay(blade: bpy.types.Object, name: str) -> bpy.types.Object:
    overlay = blade.copy()
    overlay.data = blade.data.copy()
    overlay.name = name
    bpy.context.collection.objects.link(overlay)
    overlay.color = (0.82, 0.54, 0.16, 1.0)
    modifier = overlay.modifiers.new("IGGY_TopologyWire", "WIREFRAME")
    modifier.thickness = 0.00018
    modifier.use_even_offset = True
    modifier.use_replace = True
    overlay.hide_render = True
    return overlay


def escape_xml(value: str) -> str:
    return (
        value.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
    )


def write_section_chart_svg(
    sword_name: str,
    sections: list[dict[str, Any]],
    path: Path,
) -> None:
    rows = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{PANEL_WIDTH}" height="{PANEL_HEIGHT}">',
        '<rect width="100%" height="100%" fill="#17191d"/>',
        '<text x="24" y="58" fill="#aeb5bd" font-family="Arial" font-size="12">model intersections; thickness drawn x2; metre values retained</text>',
    ]
    start_y = 105
    row_height = 88
    for index, section in enumerate(sections):
        center_x = 324.0
        center_y = start_y + index * row_height
        points = section["points_m"]
        maximum_half_width = max(abs(point[0]) for point in points)
        scale = 116.0 / max(maximum_half_width, 1.0e-9)
        path_points = [
            (
                center_x + point[0] * scale,
                center_y - point[1] * scale * 2.0,
            )
            for point in points
        ]
        d = "M " + " L ".join(f"{x:.2f} {y:.2f}" for x, y in path_points) + " Z"
        station = int(round(section["station_fraction"] * 100.0))
        width_mm = section["width_m"] * 1000.0
        thickness_mm = section["thickness_m"] * 1000.0
        rows.extend(
            [
                f'<line x1="24" y1="{center_y + 35:.1f}" x2="456" y2="{center_y + 35:.1f}" stroke="#2b2f35" stroke-width="1"/>',
                f'<text x="24" y="{center_y - 5:.1f}" fill="#d5a55b" font-family="Arial" font-size="15" font-weight="bold">{station}%</text>',
                f'<text x="24" y="{center_y + 15:.1f}" fill="#aeb5bd" font-family="Arial" font-size="11">W {width_mm:.1f} mm / T {thickness_mm:.1f} mm / {section["intersection_count"]} pts</text>',
                f'<path d="{d}" fill="#303740" stroke="#f0f2f4" stroke-width="2" stroke-linejoin="round"/>',
                f'<line x1="{center_x - 120:.1f}" y1="{center_y:.1f}" x2="{center_x + 120:.1f}" y2="{center_y:.1f}" stroke="#626b75" stroke-width="0.7"/>',
            ]
        )
    rows.append("</svg>")
    path.write_text("\n".join(rows) + "\n")


def rasterize_svg(svg_path: Path, png_path: Path) -> None:
    sips = shutil.which("sips")
    if sips is None:
        raise RuntimeError("sips is required to rasterize the study charts")
    result = subprocess.run(
        [sips, "-s", "format", "png", str(svg_path), "--out", str(png_path)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError("section chart rasterization failed: " + result.stderr.strip())


def write_board_overlay_svg(path: Path) -> None:
    width = PANEL_WIDTH * 5
    height = PANEL_HEIGHT * len(OBJECT_ORDER)
    rows = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}">'
    ]
    labels = tuple(PANEL_LABELS[panel] for panel in PANEL_ORDER) + (PANEL_LABELS["sections"],)
    for row_index, sword_name in enumerate(OBJECT_ORDER):
        for column_index, label in enumerate(labels):
            x = column_index * PANEL_WIDTH
            y = row_index * PANEL_HEIGHT
            rows.extend(
                [
                    f'<rect x="{x}" y="{y}" width="{PANEL_WIDTH}" height="36" fill="#0b0d10" fill-opacity="0.86"/>',
                    f'<text x="{x + 12}" y="{y + 23}" fill="#f3f4f5" font-family="Arial" font-size="14" font-weight="bold">{escape_xml(sword_name)} / {escape_xml(label)}</text>',
                ]
            )
    rows.append("</svg>")
    path.write_text("\n".join(rows) + "\n")


def assemble_board(
    output_root: Path,
    panel_paths: dict[str, Path],
    section_paths: dict[str, Path],
) -> dict[str, Any]:
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg is None:
        raise RuntimeError("ffmpeg is required for study-board assembly")
    ordered = [
        panel_paths[f"{sword_name}:{panel}"]
        for sword_name in OBJECT_ORDER
        for panel in PANEL_ORDER
    ]
    ordered_with_sections: list[Path] = []
    for row_index, sword_name in enumerate(OBJECT_ORDER):
        start = row_index * len(PANEL_ORDER)
        ordered_with_sections.extend(ordered[start : start + len(PANEL_ORDER)])
        ordered_with_sections.append(section_paths[sword_name])
    raw = output_root / "_historical_swords_raw.png"
    overlay_svg = output_root / "_historical_swords_overlay.svg"
    overlay_png = output_root / "_historical_swords_overlay.png"
    board = output_root / "historical_swords_set_geometry_study_board.png"
    layout = [
        f"{column * PANEL_WIDTH}_{row * PANEL_HEIGHT}"
        for row in range(len(OBJECT_ORDER))
        for column in range(5)
    ]
    stack = subprocess.run(
        [
            ffmpeg,
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *sum((["-i", str(path)] for path in ordered_with_sections), []),
            "-filter_complex",
            f"xstack=inputs={len(ordered_with_sections)}:layout={'|'.join(layout)}[board]",
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
        raise RuntimeError("study-board stack failed: " + stack.stderr.strip())
    write_board_overlay_svg(overlay_svg)
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
        raise RuntimeError("study-board overlay failed: " + merge.stderr.strip())
    for temporary in (raw, overlay_svg, overlay_png):
        temporary.unlink(missing_ok=True)
    record = artifact_record(board)
    record["resolution"] = [PANEL_WIDTH * 5, PANEL_HEIGHT * len(OBJECT_ORDER)]
    record["layout"] = "four sword rows by four geometry panels plus one section chart"
    return record


def review_only(output_root: Path) -> None:
    manifest_path = output_root / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    expected = {manifest["board"]["path"]: manifest["board"]["sha256"]}
    for group in ("geometry_panels", "section_charts", "section_svgs"):
        expected.update(
            {
                artifact["path"]: artifact["sha256"]
                for artifact in manifest["artifacts"][group].values()
            }
        )
    for path_string, digest in expected.items():
        path = Path(path_string)
        if not path.is_file() or sha256_file(path) != digest:
            raise RuntimeError(f"study artifact changed before review: {path}")
    source_path = Path(manifest["source"]["path"])
    if sha256_file(source_path) != manifest["source"]["sha256"]:
        raise RuntimeError("study source changed before review")
    if not REVIEW_PATH.is_file():
        raise FileNotFoundError(REVIEW_PATH)
    manifest["visual_review"] = VISUAL_REVIEW
    manifest["review_document"] = {
        "path": str(REVIEW_PATH),
        "sha256": sha256_file(REVIEW_PATH),
    }
    manifest["script"]["sha256"] = sha256_file(Path(__file__).resolve())
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=False) + "\n")
    print(f"HISTORICAL_SWORD_STUDY_REVIEW={VISUAL_REVIEW['decision']}")


def main() -> None:
    args = parse_args()
    output_root = args.output_root.expanduser().resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    if args.review_only:
        review_only(output_root)
        return
    provenance = json.loads(PROVENANCE_PATH.read_text())
    source_path = output_root / "source" / provenance["source_archive"]["filename"]
    fetch_hash_locked_source(provenance, source_path)
    source_hash_before = sha256_file(source_path)
    bpy.ops.wm.open_mainfile(filepath=str(source_path))
    missing = [name for name in OBJECT_ORDER if bpy.data.objects.get(name) is None]
    if missing:
        raise RuntimeError(f"historical sword donor lost objects: {missing}")
    unexpected = sorted(
        obj.name
        for obj in bpy.data.objects
        if obj.type == "MESH" and obj.name not in OBJECT_ORDER
    )
    if unexpected:
        raise RuntimeError(f"historical sword donor added unexpected meshes: {unexpected}")
    scene, camera = configure_scene()
    panels_root = output_root / "panels"
    sections_root = output_root / "sections"
    panels_root.mkdir(parents=True, exist_ok=True)
    sections_root.mkdir(parents=True, exist_ok=True)
    clear_generated_artifacts(panels_root, sections_root)
    station_fractions = provenance["study_contract"]["section_stations_from_blade_root"]
    swords: dict[str, Any] = {}
    panel_records: dict[str, Any] = {}
    section_chart_records: dict[str, Any] = {}
    section_svg_records: dict[str, Any] = {}
    panel_paths: dict[str, Path] = {}
    section_paths: dict[str, Path] = {}

    for sword_name in OBJECT_ORDER:
        source = bpy.data.objects[sword_name]
        source.color = (0.34, 0.37, 0.41, 1.0)
        components = connected_components(source)
        construction = classify_components(components)
        blade_component = construction["blade"][0]
        sections = audit_sections(source, blade_component, station_fractions)
        blade = create_component_object(
            source,
            blade_component,
            f"IGGY_STUDY_{sword_name}_Blade",
        )
        topology = make_topology_overlay(
            blade,
            f"IGGY_STUDY_{sword_name}_Topology",
        )
        hilt = create_component_object(
            source,
            combine_components(
                construction["guard"] + construction["grip"] + construction["pommel"]
            ),
            f"IGGY_STUDY_{sword_name}_Hilt",
        )
        source.hide_render = True
        blade.hide_render = True
        topology.hide_render = True
        hilt.hide_render = True

        construction_public = {
            role: {
                "component_count": len(records),
                "members": [public_component(record) for record in records],
            }
            for role, records in construction.items()
        }
        sword_record = {
            "source_object": sword_name,
            "source_mesh": source.data.name,
            "source_object_dimensions_m": [round(float(value), 9) for value in source.dimensions],
            "source_vertex_count": len(source.data.vertices),
            "source_edge_count": len(source.data.edges),
            "source_polygon_count": len(source.data.polygons),
            "source_uv_layers": [layer.name for layer in source.data.uv_layers],
            "source_material_slots": [material.name for material in source.data.materials if material],
            "construction_components": construction_public,
            "blade": {
                **public_component(blade_component),
                "length_m": round(blade_component["dimensions_m"][1], 9),
                "maximum_width_m": round(blade_component["dimensions_m"][0], 9),
                "maximum_thickness_m": round(blade_component["dimensions_m"][2], 9),
                "width_taper_ratio_88_to_12": round(
                    sections[-1]["width_m"] / sections[0]["width_m"],
                    6,
                ),
                "thickness_taper_ratio_88_to_12": round(
                    sections[-1]["thickness_m"] / sections[0]["thickness_m"],
                    6,
                ),
            },
            "sections": sections,
        }
        swords[sword_name] = sword_record

        for panel in PANEL_ORDER:
            panel_path = panels_root / f"{sword_name}_{panel}.png"
            if panel == "whole_front":
                targets = [source]
                framing_target = source
            elif panel == "hilt_oblique":
                targets = [hilt]
                framing_target = hilt
            elif panel == "blade_topology":
                targets = [blade, topology]
                framing_target = blade
            else:
                targets = [blade]
                framing_target = blade
            record = render_geometry_panel(
                scene,
                camera,
                targets,
                framing_target,
                panel,
                panel_path,
            )
            key = f"{sword_name}:{panel}"
            panel_records[key] = record
            panel_paths[key] = panel_path

        svg_path = sections_root / f"{sword_name}_sections.svg"
        png_path = sections_root / f"{sword_name}_sections.png"
        write_section_chart_svg(sword_name, sections, svg_path)
        rasterize_svg(svg_path, png_path)
        section_svg_records[sword_name] = artifact_record(svg_path)
        section_chart_records[sword_name] = artifact_record(png_path)
        section_paths[sword_name] = png_path

    board = assemble_board(output_root, panel_paths, section_paths)
    source_hash_after = sha256_file(source_path)
    if source_hash_before != source_hash_after:
        raise RuntimeError("historical sword study changed the downloaded source")
    manifest = {
        "schema": "iggy3d.historical-swords-geometry-study.v1",
        "status": "REFERENCE_DONOR_STUDY_NOT_PRODUCTION_ASSET",
        "source": {
            "path": str(source_path),
            "source_page_url": provenance["source_page_url"],
            "download_url": provenance["download_url"],
            "creator": provenance["creator"],
            "selected_license": provenance["selected_license"]["id"],
            "attribution_text": provenance["selected_license"]["attribution_text"],
            "bytes": source_path.stat().st_size,
            "sha256": source_hash_after,
        },
        "source_unchanged": source_hash_before == source_hash_after,
        "coordinate_contract": {
            "blade_axis": "+Y",
            "width_axis": "X",
            "thickness_axis": "Z",
            "section_station_fractions": station_fractions,
        },
        "swords": swords,
        "artifacts": {
            "geometry_panels": panel_records,
            "section_charts": section_chart_records,
            "section_svgs": section_svg_records,
        },
        "board": board,
        "visual_review": VISUAL_REVIEW,
        "review_document": None,
        "runtime_material_created": False,
        "source_materials_consumed_for_appearance": False,
        "source_geometry_modified": False,
        "saved_blend_created": False,
        "uses_ai_generated_imagery": False,
        "manual_acceptance_established": False,
        "unreal_parity_verified": False,
        "script": {
            "path": str(Path(__file__).resolve()),
            "sha256": sha256_file(Path(__file__).resolve()),
        },
        "limitations": provenance["prohibited_claims"],
    }
    (output_root / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=False) + "\n"
    )
    print(
        "HISTORICAL_SWORD_STUDY="
        f"swords:{len(swords)},geometry_panels:{len(panel_records)},"
        f"sections:{sum(len(sword['sections']) for sword in swords.values())},"
        "source_unchanged:true,production_asset:false"
    )
    print(f"HISTORICAL_SWORD_STUDY_OUTPUT={output_root}")


if __name__ == "__main__":
    main()
