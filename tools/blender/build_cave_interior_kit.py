#!/usr/bin/env python3
"""Build the Iggy3D modular cave interior kit in Blender."""

from __future__ import annotations

import argparse
import math
import pathlib
import sys

import bpy

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_homestead_interior_kit import configure_render  # noqa: E402
from build_homestead_modular_kit import (  # noqa: E402
    AssetSpec,
    add_preview_environment,
    box,
    clear_objects,
    export_asset,
    look_at,
    mark_collision_part,
    material,
    place_asset,
)
from build_rock_cliff_kit import (  # noqa: E402
    RockMassSpec,
    build_rock_materials,
    rock_group,
)


def script_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--preview", type=pathlib.Path)
    parser.add_argument("--cave-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def build_cave_materials() -> dict[str, bpy.types.Material]:
    values = build_rock_materials()
    values.update(
        {
            "cave_damp": material(
                "Cave Damp Stone", (0.16, 0.22, 0.20, 1.0), 0.99
            ),
            "cave_earth": material(
                "Cave Packed Earth", (0.16, 0.095, 0.040, 1.0), 1.0
            ),
            "cave_mineral": material(
                "Cave Pale Mineral", (0.46, 0.50, 0.47, 1.0), 0.93
            ),
        }
    )
    return values


FloorPartSpec = tuple[
    tuple[float, float, float],
    tuple[float, float, float],
    float,
    str,
]
WallSegment = tuple[float, float, float, float]


def floor_parts(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    parts: tuple[FloorPartSpec, ...],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for index, (location, dimensions, yaw_degrees, material_key) in enumerate(
        parts, start=1
    ):
        slab = box(
            f"{prefix}_{index:02d}",
            dimensions,
            location,
            materials[material_key],
            0.075,
            rotation=(0.0, 0.0, math.radians(yaw_degrees)),
        )
        mark_collision_part(slab, walkable=True)
        objects.append(slab)
    return objects


def floor_grid(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    centers: tuple[tuple[float, float], ...],
    cell_size: tuple[float, float] = (2.04, 2.04),
) -> list[bpy.types.Object]:
    keys = ("rock_gray", "cave_damp", "rock_light", "rock_dark")
    parts = tuple(
        (
            (x, y, 0.11 + (index % 3) * 0.012),
            (cell_size[0], cell_size[1], 0.22),
            -1.2 + (index % 4) * 0.8,
            keys[index % len(keys)],
        )
        for index, (x, y) in enumerate(centers)
    )
    return floor_parts(materials, prefix, parts)


def wall_rows(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    segments: tuple[WallSegment, ...],
    row_centers: tuple[float, ...] = (0.75, 2.25),
    row_height: float = 1.55,
) -> list[bpy.types.Object]:
    keys = ("rock_dark", "rock_gray", "cave_damp", "rock_moss")
    masses: list[RockMassSpec] = []
    for row, z in enumerate(row_centers):
        for index, (x, y, width, depth) in enumerate(segments):
            material_key = keys[(row * len(segments) + index) % len(keys)]
            masses.append(
                (
                    (x, y, z),
                    (width * 1.06, depth * 1.06, row_height),
                    -7.0 + ((index + row) % 5) * 3.5,
                    -8.0 + ((index * 3 + row) % 7) * 2.7,
                    material_key,
                )
            )
    return rock_group(materials, prefix, tuple(masses), collision=True)


def ceiling_cells(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    centers: tuple[tuple[float, float], ...],
    height: float = 3.28,
) -> list[bpy.types.Object]:
    keys = ("rock_dark", "cave_damp", "rock_gray", "rock_moss")
    masses: tuple[RockMassSpec, ...] = tuple(
        (
            (x, y, height),
            (2.18, 2.18, 0.72),
            -4.0 + (index % 4) * 2.8,
            -9.0 + (index % 5) * 4.0,
            keys[index % len(keys)],
        )
        for index, (x, y) in enumerate(centers)
    )
    return rock_group(materials, prefix, masses, collision=True)


def cone(
    name: str,
    location: tuple[float, float, float],
    radius_bottom: float,
    radius_top: float,
    depth: float,
    value: bpy.types.Material,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cone_add(
        vertices=7,
        radius1=radius_bottom,
        radius2=radius_top,
        depth=depth,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(value)
    bevel = obj.modifiers.new("Weathered edges", "BEVEL")
    bevel.width = min(0.045, depth * 0.04)
    bevel.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj


FLOOR_4X4_CENTERS = (
    (-1.0, -1.33),
    (1.0, -1.33),
    (-1.0, 0.0),
    (1.0, 0.0),
    (-1.0, 1.33),
    (1.0, 1.33),
)


def build_cave_floor_4x4(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return floor_grid(
        materials,
        "cave_floor",
        FLOOR_4X4_CENTERS,
        cell_size=(2.04, 1.38),
    )


def build_cave_ramp(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    parts: list[FloorPartSpec] = []
    keys = ("rock_dark", "rock_gray", "cave_damp", "rock_light")
    for row in range(4):
        height = 0.375 * (row + 1)
        for column, x in enumerate((-1.0, 1.0)):
            index = row * 2 + column
            parts.append(
                (
                    (x, -1.5 + row, height * 0.5),
                    (2.06, 1.04, height),
                    -0.8 + index * 0.23,
                    keys[index % len(keys)],
                )
            )
    return floor_parts(materials, "cave_ramp_tread", tuple(parts))


def build_cave_wall_straight(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return wall_rows(
        materials,
        "cave_wall_straight",
        tuple((-1.5 + index, 0.0, 1.08, 1.02) for index in range(4)),
    )


def build_cave_wall_corner_inner(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    segments = (
        (-1.0, 1.75, 1.42, 0.92),
        (0.35, 1.75, 1.42, 0.92),
        (1.55, 1.75, 1.15, 0.92),
        (1.75, -1.0, 0.92, 1.42),
        (1.75, 0.35, 0.92, 1.42),
        (1.75, 1.55, 0.92, 1.15),
    )
    return wall_rows(materials, "cave_wall_inner", segments)


def build_cave_wall_corner_outer(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    segments = (
        (0.50, 0.0, 1.10, 0.92),
        (1.50, 0.0, 1.10, 0.92),
        (2.50, 0.0, 1.10, 0.92),
        (0.0, 0.50, 0.92, 1.10),
        (0.0, 1.50, 0.92, 1.10),
        (0.0, 2.50, 0.92, 1.10),
    )
    return wall_rows(materials, "cave_wall_outer", segments)


def build_cave_ceiling_4x4(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return ceiling_cells(materials, "cave_ceiling", FLOOR_4X4_CENTERS)


def straight_side_segments() -> tuple[WallSegment, ...]:
    return (
        (-2.0, -1.0, 1.02, 2.10),
        (-2.0, 1.0, 1.02, 2.10),
        (2.0, -1.0, 1.02, 2.10),
        (2.0, 1.0, 1.02, 2.10),
    )


def build_cave_tunnel_straight(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = build_cave_floor_4x4(materials)
    objects.extend(
        wall_rows(materials, "tunnel_straight_wall", straight_side_segments())
    )
    objects.extend(
        ceiling_cells(materials, "tunnel_straight_ceiling", FLOOR_4X4_CENTERS)
    )
    return objects


TURN_CENTERS = (
    (0.0, -2.0),
    (0.0, 0.0),
    (0.0, 2.0),
    (2.0, 2.0),
    (4.0, 2.0),
)


def build_cave_tunnel_turn(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    segments = (
        (-1.5, -2.0, 1.0, 2.1),
        (-1.5, 0.0, 1.0, 2.1),
        (-1.5, 2.0, 1.0, 2.1),
        (1.5, -2.0, 1.0, 2.1),
        (1.5, 0.0, 1.0, 2.1),
        (0.0, 3.5, 2.1, 1.0),
        (2.0, 3.5, 2.1, 1.0),
        (4.0, 3.5, 2.1, 1.0),
        (2.0, 0.5, 2.1, 1.0),
        (4.0, 0.5, 2.1, 1.0),
    )
    objects = floor_grid(materials, "tunnel_turn_floor", TURN_CENTERS)
    objects.extend(wall_rows(materials, "tunnel_turn_wall", segments))
    objects.extend(
        ceiling_cells(materials, "tunnel_turn_ceiling", TURN_CENTERS)
    )
    return objects


JUNCTION_CENTERS = (
    (0.0, -2.0),
    (0.0, 0.0),
    (0.0, 2.0),
    (-2.0, 2.0),
    (2.0, 2.0),
)


def build_cave_tunnel_t_junction(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    segments = (
        (-1.5, -2.0, 1.0, 2.1),
        (-1.5, 0.0, 1.0, 2.1),
        (1.5, -2.0, 1.0, 2.1),
        (1.5, 0.0, 1.0, 2.1),
        (-2.0, 3.5, 2.1, 1.0),
        (0.0, 3.5, 2.1, 1.0),
        (2.0, 3.5, 2.1, 1.0),
        (-2.0, 0.5, 2.1, 1.0),
        (2.0, 0.5, 2.1, 1.0),
    )
    objects = floor_grid(materials, "tunnel_junction_floor", JUNCTION_CENTERS)
    objects.extend(wall_rows(materials, "tunnel_junction_wall", segments))
    objects.extend(
        ceiling_cells(materials, "tunnel_junction_ceiling", JUNCTION_CENTERS)
    )
    return objects


def build_cave_tunnel_dead_end(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = build_cave_tunnel_straight(materials)
    end_segments = tuple((-1.33 + index * 1.33, 2.0, 1.42, 1.02)
                         for index in range(3))
    objects.extend(wall_rows(materials, "tunnel_dead_end", end_segments))
    return objects


CHAMBER_CENTERS = tuple(
    (x, y)
    for y in (-2.5, 0.0, 2.5)
    for x in (-2.5, 0.0, 2.5)
)


def build_cave_chamber(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    wall_segments = (
        (-4.0, -2.5, 1.0, 2.55),
        (-4.0, 0.0, 1.0, 2.55),
        (-4.0, 2.5, 1.0, 2.55),
        (4.0, -2.5, 1.0, 2.55),
        (4.0, 2.5, 1.0, 2.55),
        (-3.0, 4.0, 2.05, 1.0),
        (-1.0, 4.0, 2.05, 1.0),
        (1.0, 4.0, 2.05, 1.0),
        (3.0, 4.0, 2.05, 1.0),
        (-3.0, -4.0, 2.05, 1.0),
        (3.0, -4.0, 2.05, 1.0),
    )
    objects = floor_grid(
        materials,
        "chamber_floor",
        CHAMBER_CENTERS,
        cell_size=(2.58, 2.58),
    )
    objects.extend(
        wall_rows(
            materials,
            "chamber_wall",
            wall_segments,
            row_centers=(0.95, 2.95),
            row_height=2.05,
        )
    )
    objects.extend(
        ceiling_cells(
            materials, "chamber_ceiling", CHAMBER_CENTERS, height=4.05
        )
    )
    return objects


def build_cave_rock_column(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    masses: tuple[RockMassSpec, ...] = (
        ((0.0, 0.0, 0.55), (1.45, 1.32, 1.10), -4.0, 8.0, "rock_dark"),
        ((0.05, -0.03, 1.60), (1.20, 1.10, 1.35), 7.0, -12.0,
         "rock_gray"),
        ((-0.04, 0.04, 2.75), (1.38, 1.26, 1.10), -6.0, 17.0,
         "cave_damp"),
    )
    return rock_group(materials, "cave_column", masses, collision=True)


def build_stalactite_cluster(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    specs = (
        (-0.72, -0.22, 2.45, 0.08, 0.42, 1.45, -7.0, 4.0),
        (-0.18, 0.18, 2.58, 0.06, 0.34, 1.18, 5.0, -6.0),
        (0.38, -0.10, 2.38, 0.07, 0.40, 1.62, -4.0, -8.0),
        (0.82, 0.26, 2.70, 0.05, 0.28, 1.02, 7.0, 5.0),
        (0.08, -0.62, 2.72, 0.05, 0.25, 0.92, -6.0, 7.0),
    )
    objects: list[bpy.types.Object] = []
    for index, (x, y, z, bottom, top, depth, rx, ry) in enumerate(specs):
        objects.append(
            cone(
                f"stalactite_{index + 1:02d}",
                (x, y, z),
                bottom,
                top,
                depth,
                materials["cave_mineral" if index % 3 == 0 else "rock_gray"],
                rotation=(math.radians(rx), math.radians(ry), 0.0),
            )
        )
    return objects


def build_stalagmite_cluster(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    specs = (
        (-0.72, -0.18, 0.72, 0.44, 0.07, 1.44, 4.0, -5.0),
        (-0.18, 0.25, 0.50, 0.32, 0.05, 1.00, -6.0, 4.0),
        (0.40, -0.12, 0.82, 0.48, 0.07, 1.64, 7.0, 6.0),
        (0.82, 0.32, 0.44, 0.27, 0.04, 0.88, -5.0, -7.0),
        (0.10, -0.62, 0.38, 0.24, 0.04, 0.76, 3.0, 8.0),
    )
    objects: list[bpy.types.Object] = []
    for index, (x, y, z, bottom, top, depth, rx, ry) in enumerate(specs):
        objects.append(
            cone(
                f"stalagmite_{index + 1:02d}",
                (x, y, z),
                bottom,
                top,
                depth,
                materials["cave_mineral" if index % 3 == 0 else "rock_gray"],
                rotation=(math.radians(rx), math.radians(ry), 0.0),
            )
        )
    return objects


def build_collapsed_passage(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    masses: tuple[RockMassSpec, ...] = (
        ((-1.45, 0.0, 0.55), (1.40, 1.45, 1.10), 8.0, -12.0,
         "rock_dark"),
        ((-0.35, -0.12, 0.62), (1.65, 1.55, 1.24), -11.0, 16.0,
         "rock_gray"),
        ((0.88, 0.08, 0.52), (1.52, 1.48, 1.04), 13.0, -20.0,
         "cave_damp"),
        ((1.55, -0.08, 0.42), (1.16, 1.30, 0.84), -7.0, 25.0,
         "rock_light"),
        ((-1.02, 0.08, 1.55), (1.55, 1.40, 1.42), -9.0, 13.0,
         "rock_gray"),
        ((0.15, -0.04, 1.62), (1.70, 1.46, 1.50), 10.0, -17.0,
         "rock_dark"),
        ((1.18, 0.02, 1.44), (1.42, 1.36, 1.26), -12.0, 21.0,
         "rock_moss"),
        ((-0.55, 0.0, 2.52), (1.48, 1.32, 1.18), 7.0, -8.0,
         "cave_damp"),
        ((0.68, 0.04, 2.45), (1.52, 1.35, 1.14), -6.0, 11.0,
         "rock_gray"),
    )
    return rock_group(materials, "collapsed_passage", masses, collision=True)


def build_cave_ledge(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    base: tuple[RockMassSpec, ...] = (
        ((-1.28, 0.35, 0.62), (1.58, 1.72, 1.24), 7.0, -12.0,
         "rock_dark"),
        ((0.0, 0.32, 0.70), (1.72, 1.78, 1.40), -9.0, 16.0,
         "rock_gray"),
        ((1.28, 0.38, 0.60), (1.56, 1.68, 1.20), 11.0, -21.0,
         "cave_damp"),
    )
    objects = rock_group(materials, "cave_ledge_base", base, collision=True)
    for index, x in enumerate((-1.0, 1.0), start=1):
        cap = box(
            f"cave_ledge_walkable_{index:02d}",
            (2.12, 2.35, 0.18),
            (x, -0.18, 1.32),
            materials["rock_gray" if index == 1 else "cave_damp"],
            0.10,
            rotation=(0.0, 0.0, math.radians(-1.0 + index * 1.2)),
        )
        mark_collision_part(cap, walkable=True)
        objects.append(cap)
    return objects


ASSET_SPECS = (
    AssetSpec("floor_4x4", "terrain", "compound_bounds", False,
              build_cave_floor_4x4, (-15.0, 12.0, 0.0)),
    AssetSpec("ramp_4x4x1p5", "stairs", "compound_bounds", False,
              build_cave_ramp, (-5.0, 12.0, 0.0)),
    AssetSpec("wall_straight_4x3", "terrain", "compound_bounds", False,
              build_cave_wall_straight, (5.0, 12.0, 0.0)),
    AssetSpec("wall_corner_inner_4x4x3", "terrain", "compound_bounds", False,
              build_cave_wall_corner_inner, (15.0, 12.0, 0.0)),
    AssetSpec("wall_corner_outer_4x4x3", "terrain", "compound_bounds", False,
              build_cave_wall_corner_outer, (-15.0, 2.0, 0.0)),
    AssetSpec("ceiling_4x4", "terrain", "compound_bounds", False,
              build_cave_ceiling_4x4, (-5.0, 2.0, 0.0)),
    AssetSpec("tunnel_straight_4x4x3p5", "structure", "compound_bounds",
              False, build_cave_tunnel_straight, (5.0, 2.0, 0.0)),
    AssetSpec("tunnel_turn_6x6x3p5", "structure", "compound_bounds", False,
              build_cave_tunnel_turn, (13.0, 1.0, 0.0)),
    AssetSpec("tunnel_t_junction_6x6x3p5", "structure", "compound_bounds",
              False, build_cave_tunnel_t_junction, (-15.0, -8.0, 0.0)),
    AssetSpec("tunnel_dead_end_4x4x3p5", "structure", "compound_bounds",
              False, build_cave_tunnel_dead_end, (-5.0, -8.0, 0.0)),
    AssetSpec("chamber_8x8x4", "structure", "compound_bounds", False,
              build_cave_chamber, (6.0, -8.0, 0.0)),
    AssetSpec("rock_column_1p4x3p2", "structure", "compound_bounds", False,
              build_cave_rock_column, (16.0, -8.0, 0.0)),
    AssetSpec("stalactite_cluster_2x2x3", "decor", "none", False,
              build_stalactite_cluster, (-15.0, -18.0, 0.0)),
    AssetSpec("stalagmite_cluster_2x2x2", "decor", "none", False,
              build_stalagmite_cluster, (-5.0, -18.0, 0.0)),
    AssetSpec("collapsed_passage_4x3", "structure", "compound_bounds", False,
              build_collapsed_passage, (5.0, -18.0, 0.0)),
    AssetSpec("ledge_walkable_4x2", "terrain", "compound_bounds", False,
              build_cave_ledge, (15.0, -18.0, 0.0)),
)


def enlarge_gallery_ground(width: float, depth: float) -> None:
    ground = bpy.data.objects.get("gallery_ground")
    if ground is None:
        return
    ground.dimensions = (width, depth, 0.08)
    bpy.context.view_layer.objects.active = ground
    ground.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    ground.select_set(False)


def build_gallery_preview(
    materials: dict[str, bpy.types.Material],
    preview: pathlib.Path,
    blend: pathlib.Path | None,
) -> None:
    clear_objects()
    for spec in ASSET_SPECS:
        objects = place_asset(spec.builder, materials, spec.gallery_position)
        if spec.asset_id.startswith("tunnel_") or spec.asset_id.startswith(
            "chamber_"
        ):
            for obj in objects:
                if "ceiling" in obj.name:
                    obj.hide_render = True
    add_preview_environment(materials)
    enlarge_gallery_ground(50.0, 48.0)
    bpy.ops.object.camera_add(location=(43.0, -66.0, 46.0))
    camera = bpy.context.object
    camera.name = "Cave Kit Gallery Camera"
    camera.data.lens = 57.0
    look_at(camera, (0.0, -3.0, 1.35))
    bpy.context.scene.camera = camera
    configure_render(preview, 1920, 1200)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def place_cutaway(
    builder,
    materials: dict[str, bpy.types.Material],
    offset: tuple[float, float, float],
    rotation_z: float = 0.0,
) -> list[bpy.types.Object]:
    objects = place_asset(builder, materials, offset, rotation_z)
    for obj in objects:
        if "ceiling" in obj.name:
            obj.hide_render = True
    return objects


def build_cave_preview(
    materials: dict[str, bpy.types.Material], preview: pathlib.Path
) -> None:
    clear_objects()
    place_cutaway(build_cave_tunnel_t_junction, materials, (0.0, -6.0, 0.0))
    place_cutaway(
        build_cave_tunnel_turn,
        materials,
        (-6.0, -4.0, 0.0),
        math.radians(90.0),
    )
    place_cutaway(
        build_cave_tunnel_turn,
        materials,
        (6.0, -4.0, 0.0),
        math.radians(-90.0),
    )
    place_cutaway(build_cave_tunnel_straight, materials, (-6.0, 3.0, 0.0))
    place_cutaway(build_cave_chamber, materials, (-6.0, 9.0, 0.0))
    place_cutaway(
        build_cave_tunnel_dead_end,
        materials,
        (8.0, -11.0, 0.0),
        math.radians(180.0),
    )
    place_asset(build_cave_ramp, materials, (0.0, -11.0, 0.0))
    place_asset(
        build_collapsed_passage,
        materials,
        (-1.0, 9.0, 0.0),
        math.radians(90.0),
    )
    place_asset(build_cave_ledge, materials, (-6.0, 10.0, 0.20))
    place_asset(build_cave_rock_column, materials, (-8.0, 8.0, 0.20))
    place_asset(build_stalagmite_cluster, materials, (-4.2, 8.0, 0.20))
    add_preview_environment(materials)
    enlarge_gallery_ground(46.0, 42.0)
    bpy.ops.object.camera_add(location=(23.0, -34.0, 28.0))
    camera = bpy.context.object
    camera.name = "Cave Assembly Camera"
    camera.data.lens = 60.0
    look_at(camera, (0.0, 0.0, 1.35))
    bpy.context.scene.camera = camera
    configure_render(preview, 1720, 1120)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_cave_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_gallery_preview(materials, arguments.preview, arguments.blend)
    if arguments.cave_preview is not None:
        build_cave_preview(materials, arguments.cave_preview)
    print(f"cave interior kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
