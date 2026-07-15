#!/usr/bin/env python3
"""Build the Iggy3D rock and cliff terrain kit in Blender."""

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
from build_woodland_edge_kit import (  # noqa: E402
    build_woodland_materials,
    ellipsoid,
)


def script_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--preview", type=pathlib.Path)
    parser.add_argument("--cliff-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def build_rock_materials() -> dict[str, bpy.types.Material]:
    values = build_woodland_materials()
    values.update(
        {
            "rock_gray": material(
                "Cliff Granite", (0.31, 0.34, 0.34, 1.0), 0.98
            ),
            "rock_light": material(
                "Cliff Granite Light", (0.43, 0.45, 0.43, 1.0), 0.96
            ),
            "rock_dark": material(
                "Cliff Granite Dark", (0.19, 0.22, 0.22, 1.0), 0.99
            ),
            "rock_moss": material(
                "Cliff Moss", (0.27, 0.34, 0.22, 1.0), 0.99
            ),
            "rock_earth": material(
                "Cliff Earth", (0.20, 0.12, 0.055, 1.0), 1.0
            ),
        }
    )
    return values


RockMassSpec = tuple[
    tuple[float, float, float],
    tuple[float, float, float],
    float,
    float,
    str,
]


def rock_mass(
    materials: dict[str, bpy.types.Material],
    name: str,
    location: tuple[float, float, float],
    dimensions: tuple[float, float, float],
    rotation_y_degrees: float,
    rotation_z_degrees: float,
    material_key: str,
    collision: bool = False,
    walkable: bool = False,
    subdivisions: int = 1,
) -> bpy.types.Object:
    obj = ellipsoid(
        name,
        dimensions,
        location,
        materials[material_key],
        rotation=(0.0, math.radians(rotation_y_degrees),
                  math.radians(rotation_z_degrees)),
        subdivisions=subdivisions,
    )
    if collision:
        mark_collision_part(obj, walkable=walkable)
    return obj


def rock_group(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    masses: tuple[RockMassSpec, ...],
    collision: bool,
    subdivisions: int = 1,
) -> list[bpy.types.Object]:
    return [
        rock_mass(
            materials,
            f"{prefix}_{index:02d}",
            location,
            dimensions,
            rotation_y,
            rotation_z,
            material_key,
            collision=collision,
            subdivisions=subdivisions,
        )
        for index, (
            location,
            dimensions,
            rotation_y,
            rotation_z,
            material_key,
        ) in enumerate(masses, start=1)
    ]


def build_boulder_medium(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return rock_group(
        materials,
        "boulder_medium",
        (
            ((0.0, 0.0, 0.68), (1.75, 1.45, 1.36), 8.0, 17.0,
             "rock_gray"),
            ((-0.46, 0.18, 0.44), (0.86, 0.82, 0.78), -14.0, -21.0,
             "rock_dark"),
            ((0.52, -0.16, 0.38), (0.78, 0.72, 0.66), 11.0, 29.0,
             "rock_moss"),
        ),
        collision=False,
        subdivisions=2,
    )


def build_boulder_large(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return rock_group(
        materials,
        "boulder_large",
        (
            ((0.0, 0.0, 1.18), (3.05, 2.45, 2.36), -9.0, 12.0,
             "rock_gray"),
            ((-0.78, 0.42, 0.72), (1.48, 1.35, 1.38), 18.0, -18.0,
             "rock_dark"),
            ((0.84, -0.30, 0.68), (1.42, 1.22, 1.30), -16.0, 31.0,
             "rock_light"),
            ((0.18, 0.65, 1.72), (1.50, 1.20, 1.28), 7.0, -7.0,
             "rock_moss"),
        ),
        collision=False,
        subdivisions=2,
    )


def build_outcrop_low(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return rock_group(
        materials,
        "outcrop_low",
        (
            ((-0.92, 0.04, 0.48), (1.55, 1.58, 0.96), 7.0, -14.0,
             "rock_dark"),
            ((-0.18, -0.10, 0.68), (1.62, 1.46, 1.36), -10.0, 19.0,
             "rock_gray"),
            ((0.72, 0.14, 0.54), (1.48, 1.52, 1.08), 14.0, -24.0,
             "rock_moss"),
            ((1.18, -0.28, 0.34), (0.92, 0.96, 0.68), -8.0, 32.0,
             "rock_light"),
        ),
        collision=True,
        subdivisions=1,
    )


def build_outcrop_tall(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return rock_group(
        materials,
        "outcrop_tall",
        (
            ((-0.78, 0.12, 0.72), (1.52, 1.56, 1.44), 5.0, -16.0,
             "rock_dark"),
            ((0.28, -0.08, 0.96), (1.82, 1.62, 1.92), -12.0, 21.0,
             "rock_gray"),
            ((0.72, 0.20, 1.72), (1.46, 1.34, 1.74), 16.0, -9.0,
             "rock_light"),
            ((-0.18, 0.32, 2.28), (1.58, 1.28, 1.52), -7.0, 28.0,
             "rock_moss"),
            ((-0.86, -0.18, 1.46), (1.24, 1.18, 1.52), 13.0, -31.0,
             "rock_gray"),
        ),
        collision=True,
        subdivisions=1,
    )


def cliff_face(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    width: float,
    height: float,
    columns: int,
    rows: int,
) -> list[bpy.types.Object]:
    masses: list[RockMassSpec] = []
    cell_width = width / columns
    cell_height = height / rows
    material_keys = ("rock_dark", "rock_gray", "rock_light", "rock_moss")
    for row in range(rows):
        for column in range(columns):
            index = row * columns + column
            x = -width * 0.5 + cell_width * (column + 0.5)
            z = cell_height * (row + 0.5)
            y = 0.04 * ((index % 3) - 1)
            masses.append(
                (
                    (x, y, z),
                    (cell_width * 1.14, 0.92 + (index % 2) * 0.10,
                     cell_height * 1.13),
                    -7.0 + (index % 4) * 4.5,
                    -4.0 + (index % 5) * 2.0,
                    material_keys[index % len(material_keys)],
                )
            )
    return rock_group(materials, prefix, tuple(masses), collision=True)


def build_cliff_face_2x2(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return cliff_face(materials, "cliff_face_2m", 2.0, 2.0, 2, 2)


def build_cliff_face_4x3(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return cliff_face(materials, "cliff_face_4m", 4.0, 3.0, 4, 3)


def corner_masses(inner: bool) -> tuple[RockMassSpec, ...]:
    masses: list[RockMassSpec] = []
    material_keys = ("rock_dark", "rock_gray", "rock_moss", "rock_light")
    for row, z in enumerate((0.50, 1.50)):
        for column, distance in enumerate((0.50, 1.50)):
            index = row * 2 + column
            if inner:
                masses.extend(
                    [
                        ((distance, 1.82, z), (1.08, 0.86, 1.08),
                         index * 3.0, -8.0 + index * 4.0,
                         material_keys[index]),
                        ((1.82, distance, z), (0.86, 1.08, 1.08),
                         -index * 2.5, 9.0 - index * 3.0,
                         material_keys[(index + 1) % 4]),
                    ]
                )
            else:
                masses.extend(
                    [
                        ((distance, 0.0, z), (1.08, 0.90, 1.08),
                         index * 3.0, -8.0 + index * 4.0,
                         material_keys[index]),
                        ((0.0, distance, z), (0.90, 1.08, 1.08),
                         -index * 2.5, 9.0 - index * 3.0,
                         material_keys[(index + 1) % 4]),
                    ]
                )
    return tuple(masses)


def build_cliff_corner_inner(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return rock_group(
        materials,
        "cliff_corner_inner",
        corner_masses(inner=True),
        collision=True,
    )


def build_cliff_corner_outer(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return rock_group(
        materials,
        "cliff_corner_outer",
        corner_masses(inner=False),
        collision=True,
    )


def build_cliff_cap_walkable(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = rock_group(
        materials,
        "cliff_cap_base",
        (
            ((-1.50, 0.0, 0.58), (1.12, 1.58, 1.16), 6.0, -10.0,
             "rock_dark"),
            ((-0.50, 0.02, 0.62), (1.16, 1.62, 1.24), -8.0, 12.0,
             "rock_gray"),
            ((0.50, -0.02, 0.60), (1.14, 1.56, 1.20), 10.0, -15.0,
             "rock_moss"),
            ((1.50, 0.01, 0.57), (1.12, 1.60, 1.14), -5.0, 18.0,
             "rock_light"),
        ),
        collision=True,
    )
    for index, x in enumerate((-1.0, 1.0), start=1):
        cap = box(
            f"cliff_cap_walkable_{index:02d}",
            (2.02, 2.02, 0.16),
            (x, 0.0, 1.22),
            materials["rock_light" if index == 1 else "rock_gray"],
            0.090,
            rotation=(0.0, 0.0, math.radians(-0.8 + index * 1.1)),
        )
        mark_collision_part(cap, walkable=True)
        objects.append(cap)
    return objects


def build_natural_stone_steps(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for index in range(6):
        height = 0.25 * (index + 1)
        width = 1.86 + (index % 3) * 0.08
        depth = 0.48 + (index % 2) * 0.04
        tread = box(
            f"natural_stone_step_{index + 1:02d}",
            (width, depth, height),
            (0.03 * ((index % 2) * 2 - 1), -1.25 + index * 0.50,
             height * 0.5),
            materials[("rock_gray", "rock_light", "rock_moss")[index % 3]],
            0.085,
            rotation=(0.0, 0.0, math.radians(-1.2 + index * 0.45)),
        )
        mark_collision_part(tread, walkable=True)
        objects.append(tread)
    return objects


def build_cave_mouth_arch(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    masses: list[RockMassSpec] = []
    materials_order = ("rock_dark", "rock_gray", "rock_moss")
    for side, x in (("left", -1.55), ("right", 1.55)):
        for row, z in enumerate((0.55, 1.58, 2.55)):
            masses.append(
                (
                    (x, 0.0, z),
                    (1.05, 1.32, 1.16),
                    -7.0 + row * 6.0,
                    (-12.0 + row * 9.0) * (-1.0 if side == "left" else 1.0),
                    materials_order[row],
                )
            )
    for column, x in enumerate((-1.25, 0.0, 1.25)):
        masses.append(
            (
                (x, 0.0, 3.18),
                (1.48, 1.38, 1.10),
                -6.0 + column * 6.0,
                8.0 - column * 7.0,
                materials_order[(column + 1) % 3],
            )
        )
    return rock_group(
        materials, "cave_mouth_arch", tuple(masses), collision=True
    )


def build_scree_pile(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    masses: list[RockMassSpec] = []
    positions = (
        (-1.15, -0.42, 0.28, 0.66),
        (-0.62, -0.28, 0.34, 0.78),
        (-0.08, -0.40, 0.24, 0.58),
        (0.44, -0.22, 0.31, 0.70),
        (1.02, -0.38, 0.22, 0.54),
        (-0.90, 0.18, 0.22, 0.54),
        (-0.38, 0.22, 0.29, 0.66),
        (0.16, 0.16, 0.36, 0.80),
        (0.70, 0.26, 0.25, 0.60),
        (-0.52, 0.58, 0.20, 0.48),
        (0.02, 0.54, 0.25, 0.58),
        (0.52, 0.56, 0.18, 0.44),
    )
    keys = ("rock_dark", "rock_gray", "rock_light", "rock_moss")
    for index, (x, y, z, size) in enumerate(positions):
        masses.append(
            ((x, y, z), (size, size * 0.82, z * 2.0),
             -12.0 + index * 2.0, index * 19.0, keys[index % 4])
        )
    return rock_group(materials, "scree", tuple(masses), collision=False)


def build_rubble_cluster(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    masses = (
        ((-0.70, -0.18, 0.30), (0.78, 0.64, 0.60), 12.0, -18.0,
         "rock_gray"),
        ((-0.18, 0.12, 0.24), (0.60, 0.56, 0.48), -9.0, 22.0,
         "rock_dark"),
        ((0.34, -0.08, 0.34), (0.74, 0.62, 0.68), 14.0, -31.0,
         "rock_moss"),
        ((0.72, 0.18, 0.20), (0.48, 0.42, 0.40), -7.0, 15.0,
         "rock_light"),
        ((-0.48, 0.42, 0.18), (0.44, 0.40, 0.36), 8.0, 37.0,
         "rock_light"),
        ((0.06, 0.46, 0.22), (0.52, 0.46, 0.44), -13.0, -7.0,
         "rock_gray"),
        ((0.52, 0.52, 0.14), (0.34, 0.30, 0.28), 4.0, 26.0,
         "rock_dark"),
        ((-0.90, 0.38, 0.12), (0.30, 0.28, 0.24), -5.0, -22.0,
         "rock_moss"),
    )
    return rock_group(materials, "rubble", masses, collision=False)


def transition_wedge(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    rising_right: bool,
) -> list[bpy.types.Object]:
    heights = (0.45, 0.90, 1.40, 1.95)
    if not rising_right:
        heights = tuple(reversed(heights))
    masses: list[RockMassSpec] = []
    keys = ("rock_dark", "rock_gray", "rock_moss", "rock_light")
    for index, height in enumerate(heights):
        x = -1.5 + index
        masses.append(
            (
                (x, 0.0, height * 0.5),
                (1.14, 2.02, height),
                -8.0 + index * 5.0,
                -9.0 + index * 7.0,
                keys[index],
            )
        )
    return rock_group(materials, prefix, tuple(masses), collision=True)


def build_transition_left(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return transition_wedge(
        materials, "terrain_transition_left", rising_right=True
    )


def build_transition_right(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return transition_wedge(
        materials, "terrain_transition_right", rising_right=False
    )


def build_overlook_ledge(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = rock_group(
        materials,
        "overlook_base",
        (
            ((-1.25, 0.35, 0.62), (1.58, 1.68, 1.24), 7.0, -12.0,
             "rock_dark"),
            ((0.0, 0.30, 0.68), (1.70, 1.72, 1.36), -9.0, 16.0,
             "rock_gray"),
            ((1.25, 0.38, 0.60), (1.56, 1.66, 1.20), 11.0, -21.0,
             "rock_moss"),
        ),
        collision=True,
    )
    for index, x in enumerate((-1.0, 1.0), start=1):
        ledge = box(
            f"overlook_walkable_{index:02d}",
            (2.12, 2.35, 0.18),
            (x, -0.18, 1.32),
            materials["rock_light" if index == 1 else "rock_gray"],
            0.10,
            rotation=(0.0, 0.0, math.radians(-1.0 + index * 1.2)),
        )
        mark_collision_part(ledge, walkable=True)
        objects.append(ledge)
    return objects


ASSET_SPECS = (
    AssetSpec("boulder_medium_1p8m", "rock", "bounds", False,
              build_boulder_medium, (-10.5, 5.0, 0.0)),
    AssetSpec("boulder_large_3m", "rock", "bounds", False,
              build_boulder_large, (-3.5, 5.0, 0.0)),
    AssetSpec("rock_outcrop_low_3x2", "terrain", "compound_bounds", False,
              build_outcrop_low, (3.5, 5.0, 0.0)),
    AssetSpec("rock_outcrop_tall_3x3", "terrain", "compound_bounds", False,
              build_outcrop_tall, (10.5, 5.0, 0.0)),
    AssetSpec("cliff_face_2x2", "terrain", "compound_bounds", False,
              build_cliff_face_2x2, (-10.5, 0.5, 0.0)),
    AssetSpec("cliff_face_4x3", "terrain", "compound_bounds", False,
              build_cliff_face_4x3, (-3.5, 0.5, 0.0)),
    AssetSpec("cliff_corner_inner_2x2x2", "terrain", "compound_bounds",
              False, build_cliff_corner_inner, (3.5, 0.5, 0.0)),
    AssetSpec("cliff_corner_outer_2x2x2", "terrain", "compound_bounds",
              False, build_cliff_corner_outer, (9.5, 0.5, 0.0)),
    AssetSpec("cliff_cap_walkable_4x2", "terrain", "compound_bounds", False,
              build_cliff_cap_walkable, (-10.5, -4.0, 0.0)),
    AssetSpec("natural_stone_steps_2x3x1p5", "stairs", "compound_bounds",
              False, build_natural_stone_steps, (-3.5, -4.0, 0.0)),
    AssetSpec("cave_mouth_arch_4x3p5", "structure", "compound_bounds", False,
              build_cave_mouth_arch, (3.5, -4.0, 0.0)),
    AssetSpec("scree_pile_3x2", "rock", "none", False,
              build_scree_pile, (10.5, -4.0, 0.0)),
    AssetSpec("rubble_cluster_2x1p5", "rock", "none", False,
              build_rubble_cluster, (-10.5, -8.5, 0.0)),
    AssetSpec("terrain_transition_left_4x2x2", "terrain",
              "compound_bounds", False, build_transition_left,
              (-3.5, -8.5, 0.0)),
    AssetSpec("terrain_transition_right_4x2x2", "terrain",
              "compound_bounds", False, build_transition_right,
              (3.5, -8.5, 0.0)),
    AssetSpec("overlook_ledge_4x2", "terrain", "compound_bounds", False,
              build_overlook_ledge, (10.5, -8.5, 0.0)),
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
    bpy.ops.object.camera_add(location=(0.0, -39.0, 22.5))
    camera = bpy.context.object
    camera.name = "Rock Kit Gallery Camera"
    camera.data.lens = 53.0
    look_at(camera, (0.0, -1.5, 1.35))
    bpy.context.scene.camera = camera
    configure_render(preview, 1920, 1080)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def build_cliff_preview(
    materials: dict[str, bpy.types.Material], preview: pathlib.Path
) -> None:
    clear_objects()
    # Assemble a readable terrain seam: transition -> ridge -> cave -> ridge.
    place_asset(build_cave_mouth_arch, materials, (0.0, 5.0, 0.0))
    place_asset(build_cliff_face_4x3, materials, (-4.0, 5.0, 0.0))
    place_asset(build_cliff_face_4x3, materials, (4.0, 5.0, 0.0))
    place_asset(build_transition_left, materials, (-7.5, 5.0, 0.0))
    place_asset(build_transition_right, materials, (7.5, 5.0, 0.0))
    place_asset(build_cliff_cap_walkable, materials, (-4.0, 3.35, 0.0),
                math.radians(-2.0))
    place_asset(build_outcrop_low, materials, (-4.0, 2.40, 0.0),
                math.radians(4.0))
    place_asset(build_overlook_ledge, materials, (3.8, 3.15, 0.0),
                math.radians(2.0))
    place_asset(build_outcrop_tall, materials, (3.8, 2.55, 0.0),
                math.radians(-3.0))
    place_asset(build_natural_stone_steps, materials, (5.55, 0.90, 0.0),
                math.radians(-8.0))
    place_asset(build_boulder_large, materials, (-6.3, -2.6, 0.0),
                math.radians(17.0))
    place_asset(build_boulder_medium, materials, (1.8, -3.4, 0.0),
                math.radians(-11.0))
    place_asset(build_scree_pile, materials, (-2.1, -3.8, 0.0),
                math.radians(8.0))
    place_asset(build_rubble_cluster, materials, (7.2, -3.5, 0.0),
                math.radians(-16.0))
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(13.5, -29.5, 15.5))
    camera = bpy.context.object
    camera.name = "Cliff Assembly Camera"
    camera.data.lens = 58.0
    look_at(camera, (0.0, 1.9, 1.35))
    bpy.context.scene.camera = camera
    configure_render(preview, 1600, 1100)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_rock_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_gallery_preview(materials, arguments.preview, arguments.blend)
    if arguments.cliff_preview is not None:
        build_cliff_preview(materials, arguments.cliff_preview)
    print(f"rock and cliff kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
