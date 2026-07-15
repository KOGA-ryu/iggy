#!/usr/bin/env python3
"""Build the Iggy3D road and retaining infrastructure kit in Blender."""

from __future__ import annotations

import argparse
import math
import pathlib
import sys
from typing import Callable

import bpy
from mathutils import Matrix, Vector

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
from build_homestead_yard_kit import build_yard_materials  # noqa: E402


def script_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--preview", type=pathlib.Path)
    parser.add_argument("--road-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def build_road_materials() -> dict[str, bpy.types.Material]:
    values = build_yard_materials()
    values.update(
        {
            "path_stone": material(
                "Road Path Stone", (0.34, 0.37, 0.36, 1.0), 0.96
            ),
            "path_light": material(
                "Road Path Stone Light", (0.44, 0.46, 0.43, 1.0), 0.94
            ),
            "path_dark": material(
                "Road Path Stone Dark", (0.23, 0.26, 0.25, 1.0), 0.98
            ),
            "wall_core": material(
                "Retaining Wall Core", (0.21, 0.23, 0.22, 1.0), 0.99
            ),
            "wall_face": material(
                "Retaining Wall Face", (0.38, 0.40, 0.37, 1.0), 0.96
            ),
            "wall_moss": material(
                "Retaining Wall Moss", (0.28, 0.34, 0.23, 1.0), 0.98
            ),
            "marker_paint": material(
                "Roadside Marker Paint", (0.73, 0.67, 0.44, 1.0), 0.89
            ),
        }
    )
    return values


PathCell = tuple[float, float]
SlabPattern = tuple[float, float, float, float, float, float, str]

SLAB_PATTERNS: tuple[SlabPattern, ...] = (
    (0.94, 0.92, 0.080, 0.00, 0.00, -1.8, "path_stone"),
    (0.91, 0.95, 0.095, 0.02, -0.01, 1.2, "path_light"),
    (0.96, 0.90, 0.075, -0.01, 0.02, -0.7, "path_dark"),
    (0.92, 0.93, 0.088, 0.01, 0.01, 2.1, "path_stone"),
    (0.95, 0.91, 0.082, -0.02, -0.01, -1.3, "path_light"),
    (0.90, 0.96, 0.092, 0.01, -0.02, 0.8, "path_dark"),
)


def path_cells(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    cells: tuple[PathCell, ...],
    pattern_offset: int = 0,
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for index, (x, y) in enumerate(cells):
        width, depth, height, offset_x, offset_y, angle, material_key = (
            SLAB_PATTERNS[(index + pattern_offset) % len(SLAB_PATTERNS)]
        )
        slab = box(
            f"{prefix}_slab_{index + 1:02d}",
            (width, depth, height),
            (x + offset_x, y + offset_y, height * 0.5),
            materials[material_key],
            0.035,
            rotation=(0.0, 0.0, math.radians(angle)),
        )
        mark_collision_part(slab, walkable=True)
        objects.append(slab)
    return objects


def build_path_straight_2x2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return path_cells(
        materials,
        "path_2x2",
        ((-0.5, -0.5), (0.5, -0.5), (-0.5, 0.5), (0.5, 0.5)),
    )


def build_path_straight_4x2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    cells = tuple(
        (x, y)
        for y in (-0.5, 0.5)
        for x in (-1.5, -0.5, 0.5, 1.5)
    )
    return path_cells(materials, "path_4x2", cells, 1)


def build_path_turn_4x4(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    cells = (
        (-1.5, -0.5),
        (-0.5, -0.5),
        (-1.5, 0.5),
        (-0.5, 0.5),
        (0.5, 0.5),
        (-0.5, 1.5),
        (0.5, 1.5),
    )
    return path_cells(materials, "path_turn", cells, 2)


def build_path_t_4x4(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    cells = tuple(
        [(x, y) for y in (-0.5, 0.5)
         for x in (-1.5, -0.5, 0.5, 1.5)]
        + [(x, 1.5) for x in (-0.5, 0.5)]
    )
    return path_cells(materials, "path_t", cells, 3)


def build_path_cross_4x4(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    cells = tuple(
        [(x, y) for y in (-0.5, 0.5)
         for x in (-1.5, -0.5, 0.5, 1.5)]
        + [(x, y) for y in (-1.5, 1.5) for x in (-0.5, 0.5)]
    )
    return path_cells(materials, "path_cross", cells, 4)


def build_path_end_2x2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    cells = ((-0.5, -0.55), (0.5, -0.55), (-0.5, 0.45), (0.5, 0.45))
    objects = path_cells(materials, "path_end", cells, 5)
    for obj in objects[:2]:
        obj.scale.x *= 0.88
        obj.scale.y *= 0.84
    return objects


def build_doorway_threshold_2x1(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for index, x in enumerate((-0.5, 0.5), start=1):
        height = 0.12 if index == 1 else 0.10
        slab = box(
            f"doorway_threshold_{index:02d}",
            (0.96, 0.90, height),
            (x, 0.0, height * 0.5),
            materials["path_light" if index == 1 else "path_stone"],
            0.030,
            rotation=(0.0, 0.0, math.radians(-1.0 + index * 1.5)),
        )
        mark_collision_part(slab, walkable=True)
        objects.append(slab)
    return objects


def build_stepping_stones_4x1p5(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    stone_data = (
        (-1.65, -0.12, 0.56, 0.72, 0.10, -12.0),
        (-1.10, 0.17, 0.64, 0.60, 0.12, 8.0),
        (-0.52, -0.08, 0.58, 0.74, 0.09, -4.0),
        (0.03, 0.14, 0.68, 0.62, 0.13, 11.0),
        (0.61, -0.13, 0.60, 0.70, 0.10, -9.0),
        (1.15, 0.15, 0.62, 0.64, 0.11, 5.0),
        (1.68, -0.05, 0.54, 0.72, 0.09, -6.0),
    )
    objects: list[bpy.types.Object] = []
    for index, (x, y, width, depth, height, angle) in enumerate(
        stone_data, start=1
    ):
        stone = box(
            f"stepping_stone_{index:02d}",
            (width, depth, height),
            (x, y, height * 0.5),
            materials[("path_stone", "path_light", "path_dark")[index % 3]],
            0.10,
            rotation=(0.0, 0.0, math.radians(angle)),
        )
        mark_collision_part(stone, walkable=True)
        objects.append(stone)
    return objects


def transform_objects(
    objects: list[bpy.types.Object],
    offset: tuple[float, float, float],
    rotation_z: float,
) -> None:
    rotation = Matrix.Rotation(rotation_z, 4, "Z")
    translation = Vector(offset)
    for obj in objects:
        obj.location = rotation @ obj.location + translation
        obj.rotation_euler.z += rotation_z


def masonry_wall(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    width: float,
) -> list[bpy.types.Object]:
    core = box(
        f"{prefix}_core",
        (width, 0.40, 1.20),
        (0.0, 0.0, 0.60),
        materials["wall_core"],
        0.025,
    )
    mark_collision_part(core)
    objects = [core]
    columns = max(2, int(round(width / 0.50)))
    stone_width = width / columns
    for row, z in enumerate((0.20, 0.56, 0.92), start=1):
        row_offset = stone_width * 0.22 if row % 2 == 0 else 0.0
        for column in range(columns):
            x = -width * 0.5 + stone_width * (column + 0.5) + row_offset
            if x + stone_width * 0.45 > width * 0.5:
                x -= width
            objects.append(
                box(
                    f"{prefix}_face_{row:02d}_{column + 1:02d}",
                    (stone_width * 0.90, 0.12, 0.30),
                    (x, -0.245, z),
                    materials["wall_moss" if (row + column) % 5 == 0
                              else "wall_face"],
                    0.035,
                )
            )
    for column in range(columns):
        x = -width * 0.5 + stone_width * (column + 0.5)
        objects.append(
            box(
                f"{prefix}_cap_{column + 1:02d}",
                (stone_width * 0.94, 0.52, 0.13),
                (x, 0.0, 1.235),
                materials["path_light" if column % 2 == 0 else "path_stone"],
                0.035,
            )
        )
    return objects


def build_retaining_wall_2x1p2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return masonry_wall(materials, "retaining_2m", 2.0)


def build_retaining_wall_4x1p2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return masonry_wall(materials, "retaining_4m", 4.0)


def build_retaining_wall_corner_2x2x1p2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    x_arm = masonry_wall(materials, "retaining_corner_x", 2.0)
    transform_objects(x_arm, (1.0, 0.0, 0.0), 0.0)
    y_arm = masonry_wall(materials, "retaining_corner_y", 2.0)
    transform_objects(y_arm, (0.0, 1.0, 0.0), math.radians(90.0))
    return x_arm + y_arm


def build_retaining_wall_end_1x1p2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = masonry_wall(materials, "retaining_end", 1.0)
    objects.append(
        box(
            "retaining_end_face",
            (0.12, 0.48, 1.10),
            (0.53, 0.0, 0.57),
            materials["wall_face"],
            0.030,
        )
    )
    return objects


def build_terrain_steps_2x3x1p2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for index in range(6):
        top_height = 0.20 * (index + 1)
        tread = box(
            f"terrain_step_{index + 1:02d}",
            (2.0, 0.50, top_height),
            (0.0, -1.25 + index * 0.50, top_height * 0.5),
            materials["path_light" if index % 2 == 0 else "path_stone"],
            0.025,
        )
        mark_collision_part(tread, walkable=True)
        objects.append(tread)
    return objects


def build_drainage_culvert_2x2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for side, y in (("left", -0.82), ("right", 0.82)):
        wall = box(
            f"culvert_{side}_wall",
            (2.0, 0.32, 0.68),
            (0.0, y, 0.34),
            materials["wall_core"],
            0.035,
        )
        mark_collision_part(wall)
        objects.append(wall)
    for index, (x, y) in enumerate(
        ((-0.5, -0.5), (0.5, -0.5), (-0.5, 0.5), (0.5, 0.5)),
        start=1,
    ):
        deck = box(
            f"culvert_deck_{index:02d}",
            (0.96, 0.96, 0.12),
            (x, y, 0.74),
            materials["path_light" if index % 2 else "path_stone"],
            0.035,
            rotation=(0.0, 0.0, math.radians((index - 2) * 0.7)),
        )
        mark_collision_part(deck, walkable=True)
        objects.append(deck)
    return objects


def build_roadside_marker_0p4x1p2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return [
        box(
            "roadside_marker_base",
            (0.48, 0.48, 0.22),
            (0.0, 0.0, 0.11),
            materials["wall_core"],
            0.045,
        ),
        box(
            "roadside_marker_post",
            (0.28, 0.28, 0.92),
            (0.0, 0.0, 0.66),
            materials["wall_face"],
            0.040,
        ),
        box(
            "roadside_marker_band",
            (0.31, 0.31, 0.20),
            (0.0, 0.0, 0.94),
            materials["marker_paint"],
            0.025,
        ),
        box(
            "roadside_marker_cap",
            (0.36, 0.36, 0.12),
            (0.0, 0.0, 1.18),
            materials["path_light"],
            0.035,
        ),
    ]


ASSET_SPECS = (
    AssetSpec("stone_path_straight_2x2", "walkway", "compound_bounds", False,
              build_path_straight_2x2, (-10.0, 4.0, 0.0)),
    AssetSpec("stone_path_straight_4x2", "walkway", "compound_bounds", False,
              build_path_straight_4x2, (-5.0, 4.0, 0.0)),
    AssetSpec("stone_path_turn_4x4", "walkway", "compound_bounds", False,
              build_path_turn_4x4, (0.0, 4.0, 0.0)),
    AssetSpec("stone_path_t_4x4", "walkway", "compound_bounds", False,
              build_path_t_4x4, (5.0, 4.0, 0.0)),
    AssetSpec("stone_path_cross_4x4", "walkway", "compound_bounds", False,
              build_path_cross_4x4, (10.0, 4.0, 0.0)),
    AssetSpec("stone_path_end_2x2", "walkway", "compound_bounds", False,
              build_path_end_2x2, (-10.0, 0.0, 0.0)),
    AssetSpec("doorway_threshold_2x1", "walkway", "compound_bounds", False,
              build_doorway_threshold_2x1, (-5.0, 0.0, 0.0)),
    AssetSpec("stepping_stones_4x1p5", "walkway", "compound_bounds", False,
              build_stepping_stones_4x1p5, (0.0, 0.0, 0.0)),
    AssetSpec("retaining_wall_2x1p2", "structure", "compound_bounds", False,
              build_retaining_wall_2x1p2, (5.0, 0.0, 0.0)),
    AssetSpec("retaining_wall_4x1p2", "structure", "compound_bounds", False,
              build_retaining_wall_4x1p2, (10.0, 0.0, 0.0)),
    AssetSpec("retaining_wall_corner_2x2x1p2", "structure",
              "compound_bounds", False, build_retaining_wall_corner_2x2x1p2,
              (-10.0, -5.0, 0.0)),
    AssetSpec("retaining_wall_end_1x1p2", "structure", "compound_bounds",
              False, build_retaining_wall_end_1x1p2, (-5.0, -5.0, 0.0)),
    AssetSpec("terrain_steps_2x3x1p2", "stairs", "compound_bounds", False,
              build_terrain_steps_2x3x1p2, (0.0, -5.0, 0.0)),
    AssetSpec("drainage_culvert_2x2", "bridge", "compound_bounds", False,
              build_drainage_culvert_2x2, (5.0, -5.0, 0.0)),
    AssetSpec("roadside_marker_0p4x1p2", "marker", "bounds", False,
              build_roadside_marker_0p4x1p2, (10.0, -5.0, 0.0)),
)


def build_gallery_preview(
    materials: dict[str, bpy.types.Material],
    preview: pathlib.Path,
    blend: pathlib.Path | None,
) -> None:
    clear_objects()
    for spec in ASSET_SPECS:
        place_asset(spec.builder, materials, spec.gallery_position)
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(17.5, -27.0, 20.5))
    camera = bpy.context.object
    camera.name = "Road Kit Gallery Camera"
    camera.data.lens = 54.0
    look_at(camera, (0.0, 0.0, 0.35))
    bpy.context.scene.camera = camera
    configure_render(preview, 1920, 1080)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def build_road_preview(
    materials: dict[str, bpy.types.Material], preview: pathlib.Path
) -> None:
    clear_objects()
    placements: tuple[
        tuple[
            Callable[[dict[str, bpy.types.Material]], list[bpy.types.Object]],
            tuple[float, float, float],
            float,
        ],
        ...,
    ] = (
        (build_path_cross_4x4, (0.0, 0.0, 0.0), 0.0),
        (build_path_straight_4x2, (-4.0, 0.0, 0.0), 0.0),
        (build_path_straight_4x2, (4.0, 0.0, 0.0), 0.0),
        (build_path_straight_4x2, (0.0, 4.0, 0.0), math.radians(90.0)),
        (build_path_end_2x2, (0.0, 7.0, 0.0), math.radians(90.0)),
        (build_doorway_threshold_2x1, (0.0, 8.4, 0.0), 0.0),
        (build_drainage_culvert_2x2, (0.0, -3.0, 0.0), 0.0),
        (build_path_turn_4x4, (-8.0, 0.0, 0.0), math.radians(-90.0)),
        (build_path_t_4x4, (8.0, 0.0, 0.0), math.radians(90.0)),
        (build_stepping_stones_4x1p5, (-5.5, -4.2, 0.0),
         math.radians(-12.0)),
        (build_retaining_wall_4x1p2, (3.0, 5.1, 0.0), 0.0),
        (build_retaining_wall_2x1p2, (6.0, 5.1, 0.0), 0.0),
        (build_retaining_wall_corner_2x2x1p2, (7.0, 5.1, 0.0), 0.0),
        (build_retaining_wall_end_1x1p2, (7.0, 7.6, 0.0),
         math.radians(90.0)),
        (build_terrain_steps_2x3x1p2, (6.3, -4.5, 0.0), 0.0),
        (build_roadside_marker_0p4x1p2, (-3.0, -2.4, 0.0), 0.0),
    )
    for builder, location, rotation in placements:
        place_asset(builder, materials, location, rotation)
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(18.5, -25.5, 19.0))
    camera = bpy.context.object
    camera.name = "Road Assembly Camera"
    camera.data.lens = 56.0
    look_at(camera, (0.0, 1.2, 0.40))
    bpy.context.scene.camera = camera
    configure_render(preview, 1600, 1100)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_road_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_gallery_preview(materials, arguments.preview, arguments.blend)
    if arguments.road_preview is not None:
        build_road_preview(materials, arguments.road_preview)
    print(f"road and retaining kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
