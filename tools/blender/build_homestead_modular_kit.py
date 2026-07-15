#!/usr/bin/env python3
"""Build the Iggy3D homestead modular construction kit in Blender."""

from __future__ import annotations

import argparse
import math
import pathlib
import sys
from dataclasses import dataclass
from typing import Callable

import bpy
from mathutils import Matrix, Vector


@dataclass(frozen=True)
class AssetSpec:
    asset_id: str
    category: str
    collision: str
    walkable: bool
    builder: Callable[[dict[str, bpy.types.Material]], list[bpy.types.Object]]
    gallery_position: tuple[float, float, float]


def script_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--preview", type=pathlib.Path)
    parser.add_argument("--assembly-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def clear_objects() -> None:
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for curve in list(bpy.data.curves):
        if curve.users == 0:
            bpy.data.curves.remove(curve)


def material(name: str, color: tuple[float, float, float, float],
             roughness: float, metallic: float = 0.0) -> bpy.types.Material:
    value = bpy.data.materials.new(name)
    value.diffuse_color = color
    value.use_nodes = True
    principled = value.node_tree.nodes.get("Principled BSDF")
    principled.inputs["Base Color"].default_value = color
    principled.inputs["Roughness"].default_value = roughness
    principled.inputs["Metallic"].default_value = metallic
    return value


def build_materials() -> dict[str, bpy.types.Material]:
    return {
        "plaster": material("Homestead Plaster", (0.64, 0.60, 0.52, 1.0), 0.86),
        "timber": material("Homestead Oak", (0.30, 0.14, 0.055, 1.0), 0.68),
        "dark_timber": material("Homestead Dark Oak", (0.12, 0.050, 0.018, 1.0), 0.72),
        "floor": material("Homestead Floor Oak", (0.43, 0.22, 0.085, 1.0), 0.62),
        "stone": material("Homestead Foundation Stone", (0.31, 0.35, 0.36, 1.0), 0.91),
        "roof": material("Homestead Roof Tile", (0.26, 0.075, 0.045, 1.0), 0.82),
        "iron": material("Homestead Iron", (0.075, 0.082, 0.085, 1.0), 0.48, 0.72),
    }


def assign_material(obj: bpy.types.Object, value: bpy.types.Material) -> None:
    obj.data.materials.append(value)


def box(name: str, dimensions: tuple[float, float, float],
        location: tuple[float, float, float], value: bpy.types.Material,
        bevel: float = 0.025,
        rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    if bevel > 0.0:
        modifier = obj.modifiers.new("Edge Softening", "BEVEL")
        modifier.width = min(bevel, min(dimensions) * 0.22)
        modifier.segments = 2
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    assign_material(obj, value)
    return obj


def mesh_object(name: str, vertices: list[tuple[float, float, float]],
                faces: list[tuple[int, ...]], value: bpy.types.Material,
                bevel: float = 0.0) -> bpy.types.Object:
    mesh = bpy.data.meshes.new(f"{name}_mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign_material(obj, value)
    if bevel > 0.0:
        modifier = obj.modifiers.new("Edge Softening", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        obj.select_set(False)
    return obj


def framed_wall(materials: dict[str, bpy.types.Material], width: float,
                height: float, prefix: str) -> list[bpy.types.Object]:
    depth = 0.24
    post_width = 0.16
    rail_height = 0.16
    objects = [
        box(f"{prefix}_plaster", (width, 0.16, height),
            (0.0, 0.0, height * 0.5), materials["plaster"], 0.018),
        box(f"{prefix}_left_post", (post_width, depth, height),
            (-width * 0.5 + post_width * 0.5, 0.0, height * 0.5),
            materials["timber"], 0.022),
        box(f"{prefix}_right_post", (post_width, depth, height),
            (width * 0.5 - post_width * 0.5, 0.0, height * 0.5),
            materials["timber"], 0.022),
        box(f"{prefix}_bottom_rail", (width - 2.0 * post_width, depth,
                                       rail_height),
            (0.0, 0.0, rail_height * 0.5), materials["dark_timber"], 0.022),
        box(f"{prefix}_top_rail", (width - 2.0 * post_width, depth,
                                    rail_height),
            (0.0, 0.0, height - rail_height * 0.5),
            materials["dark_timber"], 0.022),
    ]
    if width >= 1.8:
        objects.append(
            box(f"{prefix}_center_post", (0.12, depth, height - 0.32),
                (0.0, 0.0, height * 0.5), materials["timber"], 0.018))
    return objects


def build_wall_full(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return framed_wall(materials, 4.0, 3.0, "wall_full")


def build_wall_half(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return framed_wall(materials, 2.0, 3.0, "wall_half")


def build_wall_low(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return framed_wall(materials, 4.0, 1.2, "wall_low")


def build_wall_pier(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return framed_wall(materials, 1.0, 3.0, "wall_pier")


def build_lintel(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return [
        box("lintel_body", (2.0, 0.24, 0.6), (0.0, 0.0, 0.3),
            materials["timber"], 0.035),
        box("lintel_face_band", (1.76, 0.04, 0.16), (0.0, -0.14, 0.3),
            materials["dark_timber"], 0.015),
    ]


def build_pillar(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return [
        box("pillar_foot", (0.5, 0.5, 0.24), (0.0, 0.0, 0.12),
            materials["stone"], 0.035),
        box("pillar_shaft", (0.34, 0.34, 2.56), (0.0, 0.0, 1.52),
            materials["timber"], 0.035),
        box("pillar_cap", (0.5, 0.5, 0.20), (0.0, 0.0, 2.90),
            materials["dark_timber"], 0.035),
    ]


def build_beam(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return [
        box("beam_body", (4.0, 0.32, 0.40), (0.0, 0.0, 0.20),
            materials["timber"], 0.045),
        box("beam_face_inlay", (3.62, 0.035, 0.11), (0.0, -0.177, 0.20),
            materials["dark_timber"], 0.012),
    ]


def build_floor(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("floor_backing", (4.0, 4.0, 0.16), (0.0, 0.0, 0.08),
            materials["dark_timber"], 0.018),
    ]
    for index in range(10):
        x = -1.8 + index * 0.4
        objects.append(
            box(f"floor_board_{index + 1:02d}", (0.385, 3.94, 0.08),
                (x, 0.0, 0.20), materials["floor"], 0.012))
    return objects


def build_foundation(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("foundation_core", (4.0, 4.0, 0.40), (0.0, 0.0, 0.20),
            materials["stone"], 0.045),
    ]
    for side, y in (("front", -1.965), ("back", 1.965)):
        for index in range(8):
            x = -1.75 + index * 0.5
            objects.append(
                box(f"foundation_{side}_stone_{index + 1:02d}",
                    (0.46, 0.05, 0.15), (x, y, 0.21),
                    materials["stone"], 0.018))
    return objects


def build_ceiling(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return [
        box("ceiling_panel", (4.0, 4.0, 0.12), (0.0, 0.0, 0.12),
            materials["plaster"], 0.018),
        box("ceiling_beam_left", (0.18, 4.0, 0.18), (-1.5, 0.0, 0.09),
            materials["timber"], 0.022),
        box("ceiling_beam_center", (0.18, 4.0, 0.18), (0.0, 0.0, 0.09),
            materials["timber"], 0.022),
        box("ceiling_beam_right", (0.18, 4.0, 0.18), (1.5, 0.0, 0.09),
            materials["timber"], 0.022),
    ]


def roof_slope(materials: dict[str, bpy.types.Material], width: float,
               depth: float, high: float, name: str) -> list[bpy.types.Object]:
    low = 0.0
    thickness = 0.18
    x0, x1 = -width * 0.5, width * 0.5
    y0, y1 = -depth * 0.5, depth * 0.5
    vertices = [
        (x0, y0, low), (x1, y0, low),
        (x0, y1, high - thickness), (x1, y1, high - thickness),
        (x0, y0, low + thickness), (x1, y0, low + thickness),
        (x0, y1, high), (x1, y1, high),
    ]
    faces = [
        (0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4),
        (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5),
    ]
    return [mesh_object(name, vertices, faces, materials["roof"], 0.018)]


def build_roof_shed(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return roof_slope(materials, 4.0, 4.0, 1.5, "roof_shed")


def build_roof_gable_slope(
        materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    return roof_slope(materials, 4.4, 2.2, 1.5, "roof_gable_slope")


def build_roof_ridge(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    x0, x1 = -2.2, 2.2
    vertices = [
        (x0, -0.28, 0.0), (x0, 0.28, 0.0), (x0, 0.0, 0.30),
        (x1, -0.28, 0.0), (x1, 0.28, 0.0), (x1, 0.0, 0.30),
    ]
    faces = [
        (0, 1, 2), (3, 5, 4), (0, 3, 4, 1),
        (1, 4, 5, 2), (2, 5, 3, 0),
    ]
    return [mesh_object("roof_ridge", vertices, faces, materials["roof"], 0.018)]


def build_gable_cap(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    x0, x1 = -2.0, 2.0
    y0, y1 = -0.12, 0.12
    vertices = [
        (x0, y0, 0.0), (x1, y0, 0.0), (0.0, y0, 1.5),
        (x0, y1, 0.0), (x1, y1, 0.0), (0.0, y1, 1.5),
    ]
    faces = [
        (0, 2, 1), (3, 4, 5), (0, 1, 4, 3),
        (1, 2, 5, 4), (2, 0, 3, 5),
    ]
    return [mesh_object("gable_plaster", vertices, faces,
                        materials["plaster"], 0.012)]


def build_door(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("door_leaf", (1.10, 0.10, 2.20), (0.0, 0.0, 1.10),
            materials["timber"], 0.025),
    ]
    for z in (0.47, 1.10, 1.73):
        objects.append(
            box(f"door_panel_rail_{z:.2f}", (0.86, 0.035, 0.11),
                (0.0, -0.064, z), materials["dark_timber"], 0.010))
    for x in (-0.40, 0.40):
        objects.append(
            box(f"door_panel_stile_{x:.2f}", (0.10, 0.035, 1.82),
                (x, -0.064, 1.10), materials["dark_timber"], 0.010))
    objects.append(
        box("door_handle", (0.11, 0.11, 0.11), (0.36, -0.10, 1.08),
            materials["iron"], 0.04))
    return objects


def build_window_frame(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    width = 1.5
    height = 1.2
    depth = 0.14
    border = 0.12
    return [
        box("window_left", (border, depth, height),
            (-width * 0.5 + border * 0.5, 0.0, height * 0.5),
            materials["timber"], 0.018),
        box("window_right", (border, depth, height),
            (width * 0.5 - border * 0.5, 0.0, height * 0.5),
            materials["timber"], 0.018),
        box("window_bottom", (width - 2.0 * border, depth, border),
            (0.0, 0.0, border * 0.5), materials["timber"], 0.018),
        box("window_top", (width - 2.0 * border, depth, border),
            (0.0, 0.0, height - border * 0.5), materials["timber"], 0.018),
        box("window_mullion", (0.075, depth * 0.8, height - 2.0 * border),
            (0.0, 0.0, height * 0.5), materials["dark_timber"], 0.012),
        box("window_transom", (width - 2.0 * border, depth * 0.8, 0.075),
            (0.0, 0.0, height * 0.5), materials["dark_timber"], 0.012),
    ]


ASSET_SPECS = (
    AssetSpec("wall_full_4x3", "wall", "bounds", False, build_wall_full,
              (-10.0, 5.0, 0.0)),
    AssetSpec("wall_half_2x3", "wall", "bounds", False, build_wall_half,
              (-5.5, 5.0, 0.0)),
    AssetSpec("wall_pier_1x3", "wall", "bounds", False, build_wall_pier,
              (-2.5, 5.0, 0.0)),
    AssetSpec("wall_low_4x1p2", "wall", "bounds", False, build_wall_low,
              (1.5, 5.0, 0.0)),
    AssetSpec("lintel_2x0p6", "wall", "bounds", False, build_lintel,
              (5.5, 5.0, 0.0)),
    AssetSpec("pillar_0p5x3", "structure", "bounds", False, build_pillar,
              (8.5, 5.0, 0.0)),
    AssetSpec("beam_4x0p4", "structure", "bounds", False, build_beam,
              (-8.5, 0.5, 0.0)),
    AssetSpec("floor_4x4", "floor", "bounds", True, build_floor,
              (-3.5, 0.5, 0.0)),
    AssetSpec("foundation_4x4", "foundation", "bounds", True,
              build_foundation, (2.0, 0.5, 0.0)),
    AssetSpec("ceiling_4x4", "ceiling", "bounds", False, build_ceiling,
              (7.5, 0.5, 0.0)),
    AssetSpec("roof_shed_4x4", "roof", "none", False, build_roof_shed,
              (-8.0, -5.0, 0.0)),
    AssetSpec("roof_gable_slope_4p4x2p2", "roof", "none", False,
              build_roof_gable_slope, (-4.0, -5.0, 0.0)),
    AssetSpec("roof_ridge_4p4", "roof", "none", False, build_roof_ridge,
              (0.5, -5.0, 0.0)),
    AssetSpec("gable_cap_4x1p5", "roof", "none", False, build_gable_cap,
              (4.0, -5.0, 0.0)),
    AssetSpec("door_leaf_1p1x2p2", "door", "bounds", False, build_door,
              (7.0, -5.0, 0.0)),
    AssetSpec("window_frame_1p5x1p2", "window", "none", False,
              build_window_frame, (9.5, -5.0, 0.0)),
)


def apply_metadata(objects: list[bpy.types.Object], spec: AssetSpec) -> None:
    for obj in objects:
        obj["iggy_category"] = spec.category
        obj["iggy_collision"] = spec.collision
        obj["iggy_walkable"] = spec.walkable


def export_asset(output_root: pathlib.Path, spec: AssetSpec,
                 materials: dict[str, bpy.types.Material]) -> None:
    clear_objects()
    objects = spec.builder(materials)
    apply_metadata(objects, spec)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    output = output_root / f"{spec.asset_id}.glb"
    output.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=str(output),
        export_format="GLB",
        use_selection=True,
        export_extras=True,
        export_apply=True,
        export_yup=True,
        export_materials="EXPORT",
        export_animations=False,
        export_cameras=False,
        export_lights=False,
        export_texcoords=False,
        export_normals=True,
        check_existing=False,
    )
    print(f"exported {spec.asset_id}: {output.stat().st_size} bytes")


def look_at(obj: bpy.types.Object, point: tuple[float, float, float]) -> None:
    direction = Vector(point) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def add_preview_environment(materials: dict[str, bpy.types.Material]) -> None:
    ground = box("gallery_ground", (30.0, 19.0, 0.08), (0.0, 0.0, -0.08),
                 materials["stone"], 0.0)
    ground.data.materials.clear()
    ground_material = material("Gallery Ground", (0.075, 0.085, 0.09, 1.0), 0.94)
    assign_material(ground, ground_material)

    bpy.ops.object.light_add(type="AREA", location=(1.0, -5.0, 15.0))
    key = bpy.context.object
    key.name = "Gallery Key"
    key.data.energy = 1800.0
    key.data.shape = "DISK"
    key.data.size = 9.0

    bpy.ops.object.light_add(type="AREA", location=(-10.0, -3.0, 8.0))
    fill = bpy.context.object
    fill.name = "Gallery Fill"
    fill.data.energy = 850.0
    fill.data.size = 7.0
    look_at(fill, (0.0, 0.0, 1.5))

    bpy.ops.object.light_add(type="SUN", location=(0.0, 0.0, 12.0))
    sun = bpy.context.object
    sun.name = "Gallery Sun"
    sun.rotation_euler = (math.radians(28.0), math.radians(-18.0),
                          math.radians(25.0))
    sun.data.energy = 1.8
    sun.data.angle = math.radians(18.0)


def build_preview(materials: dict[str, bpy.types.Material], preview: pathlib.Path,
                  blend: pathlib.Path | None) -> None:
    clear_objects()
    for spec in ASSET_SPECS:
        objects = spec.builder(materials)
        offset = Vector(spec.gallery_position)
        for obj in objects:
            obj.location += offset

    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(23.0, -29.0, 23.0))
    camera = bpy.context.object
    camera.name = "Gallery Camera"
    camera.data.lens = 50.0
    look_at(camera, (0.0, 0.5, 1.3))
    bpy.context.scene.camera = camera

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(preview)
    scene.render.film_transparent = False
    if scene.world is None:
        scene.world = bpy.data.worlds.new("Gallery World")
    scene.world.color = (0.018, 0.022, 0.026)
    scene.view_settings.look = "AgX - Medium High Contrast"
    preview.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def place_asset(builder: Callable[[dict[str, bpy.types.Material]],
                                  list[bpy.types.Object]],
                materials: dict[str, bpy.types.Material],
                offset: tuple[float, float, float],
                rotation_z: float = 0.0) -> list[bpy.types.Object]:
    objects = builder(materials)
    rotation = Matrix.Rotation(rotation_z, 4, "Z")
    translation = Vector(offset)
    for obj in objects:
        obj.location = rotation @ obj.location + translation
        obj.rotation_euler.z += rotation_z
    return objects


def build_assembly_preview(materials: dict[str, bpy.types.Material],
                           preview: pathlib.Path) -> None:
    clear_objects()
    place_asset(build_foundation, materials, (0.0, 0.0, 0.0))
    place_asset(build_floor, materials, (0.0, 0.0, 0.40))
    wall_base = 0.64
    place_asset(build_wall_full, materials, (0.0, 2.0, wall_base))
    place_asset(build_wall_full, materials, (-2.0, 0.0, wall_base),
                math.radians(90.0))
    place_asset(build_wall_full, materials, (2.0, 0.0, wall_base),
                math.radians(90.0))
    place_asset(build_wall_pier, materials, (-1.5, -2.0, wall_base))
    place_asset(build_wall_pier, materials, (1.5, -2.0, wall_base))
    place_asset(build_lintel, materials, (0.0, -2.0, wall_base + 2.4))
    place_asset(build_door, materials, (0.0, -2.10, wall_base))
    place_asset(build_window_frame, materials, (2.13, 0.0, wall_base + 0.9),
                math.radians(90.0))
    roof_base = wall_base + 3.0
    place_asset(build_roof_gable_slope, materials, (0.0, -1.1, roof_base))
    place_asset(build_roof_gable_slope, materials, (0.0, 1.1, roof_base),
                math.radians(180.0))
    place_asset(build_roof_ridge, materials, (0.0, 0.0, roof_base + 1.5))
    place_asset(build_gable_cap, materials, (0.0, -2.02, roof_base))

    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(11.5, -14.5, 9.5))
    camera = bpy.context.object
    camera.name = "Assembly Camera"
    camera.data.lens = 56.0
    look_at(camera, (0.0, 0.0, 2.4))
    scene = bpy.context.scene
    scene.camera = camera
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 1200
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(preview)
    scene.render.film_transparent = False
    if scene.world is None:
        scene.world = bpy.data.worlds.new("Assembly World")
    scene.world.color = (0.018, 0.022, 0.026)
    scene.view_settings.look = "AgX - Medium High Contrast"
    preview.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_preview(materials, arguments.preview, arguments.blend)
    if arguments.assembly_preview is not None:
        build_assembly_preview(materials, arguments.assembly_preview)
    print(f"homestead modular kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
