#!/usr/bin/env python3
"""Build the Iggy3D solid-foliage woodland edge kit in Blender."""

from __future__ import annotations

import argparse
import math
import pathlib
import sys
from typing import Callable

import bpy
from mathutils import Vector

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from build_homestead_interior_kit import (  # noqa: E402
    cone,
    configure_render,
    cylinder,
)
from build_homestead_modular_kit import (  # noqa: E402
    AssetSpec,
    add_preview_environment,
    assign_material,
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
    parser.add_argument("--woodland-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def build_woodland_materials() -> dict[str, bpy.types.Material]:
    values = build_yard_materials()
    values.update(
        {
            "bark": material(
                "Woodland Bark", (0.16, 0.075, 0.028, 1.0), 0.96
            ),
            "bark_light": material(
                "Woodland Young Bark", (0.29, 0.16, 0.065, 1.0), 0.93
            ),
            "dead_wood": material(
                "Woodland Dead Wood", (0.24, 0.22, 0.17, 1.0), 0.99
            ),
            "leaf_young": material(
                "Woodland Young Leaf", (0.20, 0.47, 0.12, 1.0), 0.91
            ),
            "leaf_mature": material(
                "Woodland Mature Leaf", (0.075, 0.29, 0.075, 1.0), 0.94
            ),
            "leaf_old": material(
                "Woodland Old Leaf", (0.22, 0.31, 0.075, 1.0), 0.96
            ),
            "pine_dark": material(
                "Woodland Pine Dark", (0.035, 0.18, 0.10, 1.0), 0.96
            ),
            "pine_light": material(
                "Woodland Pine Light", (0.075, 0.28, 0.15, 1.0), 0.94
            ),
            "fern": material(
                "Woodland Fern", (0.14, 0.38, 0.075, 1.0), 0.95
            ),
            "reed": material(
                "Woodland Reed", (0.31, 0.43, 0.11, 1.0), 0.96
            ),
            "seed": material(
                "Woodland Seed Head", (0.34, 0.20, 0.075, 1.0), 0.98
            ),
            "moss_stone": material(
                "Woodland Moss Stone", (0.25, 0.31, 0.21, 1.0), 0.98
            ),
        }
    )
    return values


def ellipsoid(
    name: str,
    dimensions: tuple[float, float, float],
    location: tuple[float, float, float],
    value: bpy.types.Material,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
    subdivisions: int = 2,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_ico_sphere_add(
        subdivisions=subdivisions,
        radius=1.0,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(axis * 0.5 for axis in dimensions)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    assign_material(obj, value)
    return obj


def branch_between(
    name: str,
    start: tuple[float, float, float],
    end: tuple[float, float, float],
    radius: float,
    value: bpy.types.Material,
    vertices: int = 9,
) -> bpy.types.Object:
    start_value = Vector(start)
    end_value = Vector(end)
    direction = end_value - start_value
    obj = cylinder(
        name,
        radius,
        direction.length,
        tuple((start_value + end_value) * 0.5),
        value,
        vertices=vertices,
    )
    obj.rotation_mode = "XYZ"
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler("XYZ")
    return obj


BranchSpec = tuple[
    tuple[float, float, float],
    tuple[float, float, float],
    float,
]
CanopySpec = tuple[
    tuple[float, float, float],
    tuple[float, float, float],
    str,
]


def broadleaf_tree(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    trunk_height: float,
    trunk_radius: float,
    bark_key: str,
    branches: tuple[BranchSpec, ...],
    canopy: tuple[CanopySpec, ...],
) -> list[bpy.types.Object]:
    trunk = cylinder(
        f"{prefix}_trunk",
        trunk_radius,
        trunk_height,
        (0.0, 0.0, trunk_height * 0.5),
        materials[bark_key],
        vertices=11,
    )
    mark_collision_part(trunk)
    objects = [trunk]
    for index, (start, end, radius) in enumerate(branches, start=1):
        objects.append(
            branch_between(
                f"{prefix}_branch_{index:02d}",
                start,
                end,
                radius,
                materials[bark_key],
            )
        )
    for index, (location, dimensions, material_key) in enumerate(
        canopy, start=1
    ):
        objects.append(
            ellipsoid(
                f"{prefix}_canopy_{index:02d}",
                dimensions,
                location,
                materials[material_key],
                rotation=(0.0, 0.0, math.radians(index * 17.0)),
            )
        )
    return objects


def build_broadleaf_young(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return broadleaf_tree(
        materials,
        "broadleaf_young",
        2.85,
        0.20,
        "bark_light",
        (
            ((0.0, 0.0, 2.10), (-0.72, 0.15, 3.05), 0.10),
            ((0.0, 0.0, 2.28), (0.62, -0.22, 3.22), 0.095),
            ((0.0, 0.0, 2.45), (0.18, 0.68, 3.35), 0.085),
        ),
        (
            ((-0.62, 0.18, 3.42), (1.75, 1.55, 1.65), "leaf_young"),
            ((0.58, -0.18, 3.55), (1.80, 1.60, 1.75), "leaf_mature"),
            ((0.12, 0.66, 3.58), (1.65, 1.45, 1.60), "leaf_young"),
            ((0.0, 0.02, 4.02), (1.70, 1.55, 1.45), "leaf_young"),
        ),
    )


def build_broadleaf_mature(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return broadleaf_tree(
        materials,
        "broadleaf_mature",
        4.30,
        0.36,
        "bark",
        (
            ((0.0, 0.0, 2.85), (-1.35, 0.15, 4.65), 0.18),
            ((0.0, 0.0, 3.05), (1.28, -0.45, 4.80), 0.17),
            ((0.0, 0.0, 3.32), (0.40, 1.38, 5.00), 0.16),
            ((0.0, 0.0, 3.55), (-0.35, -1.20, 5.30), 0.14),
            ((0.0, 0.0, 3.80), (0.78, 0.75, 5.60), 0.13),
        ),
        (
            ((-1.20, 0.22, 5.05), (2.70, 2.35, 2.15), "leaf_mature"),
            ((1.16, -0.42, 5.16), (2.70, 2.40, 2.20), "leaf_mature"),
            ((0.35, 1.25, 5.33), (2.50, 2.30, 2.10), "leaf_young"),
            ((-0.25, -1.02, 5.52), (2.45, 2.15, 2.15), "leaf_mature"),
            ((0.65, 0.55, 6.02), (2.55, 2.25, 2.10), "leaf_young"),
            ((-0.58, 0.35, 6.18), (2.45, 2.25, 2.15), "leaf_mature"),
        ),
    )


def build_broadleaf_ancient(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return broadleaf_tree(
        materials,
        "broadleaf_ancient",
        5.25,
        0.62,
        "bark",
        (
            ((0.0, 0.0, 3.15), (-1.95, 0.40, 5.90), 0.28),
            ((0.0, 0.0, 3.35), (1.85, -0.62, 6.05), 0.27),
            ((0.0, 0.0, 3.60), (0.65, 1.92, 6.28), 0.24),
            ((0.0, 0.0, 3.82), (-0.62, -1.75, 6.45), 0.22),
            ((0.0, 0.0, 4.05), (1.18, 1.15, 6.82), 0.20),
            ((0.0, 0.0, 4.25), (-1.10, 1.22, 7.00), 0.19),
        ),
        (
            ((-1.82, 0.42, 6.32), (3.45, 2.85, 2.55), "leaf_old"),
            ((1.72, -0.55, 6.48), (3.45, 2.95, 2.65), "leaf_mature"),
            ((0.58, 1.75, 6.70), (3.25, 2.80, 2.65), "leaf_old"),
            ((-0.48, -1.55, 6.82), (3.15, 2.70, 2.55), "leaf_mature"),
            ((1.02, 1.02, 7.38), (3.20, 2.85, 2.55), "leaf_old"),
            ((-0.92, 1.08, 7.52), (3.10, 2.70, 2.50), "leaf_mature"),
            ((0.05, -0.15, 8.15), (3.35, 3.00, 2.60), "leaf_old"),
        ),
    )


PineTier = tuple[float, float, float, str]


def pine_tree(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    trunk_height: float,
    trunk_radius: float,
    tiers: tuple[PineTier, ...],
) -> list[bpy.types.Object]:
    trunk = cylinder(
        f"{prefix}_trunk",
        trunk_radius,
        trunk_height,
        (0.0, 0.0, trunk_height * 0.5),
        materials["bark"],
        vertices=10,
    )
    mark_collision_part(trunk)
    objects = [trunk]
    for index, (center_z, height, radius, material_key) in enumerate(
        tiers, start=1
    ):
        objects.append(
            cone(
                f"{prefix}_tier_{index:02d}",
                radius,
                radius * 0.08,
                height,
                (0.0, 0.0, center_z),
                materials[material_key],
            )
        )
    return objects


def build_pine_medium(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return pine_tree(
        materials,
        "pine_medium",
        5.80,
        0.24,
        (
            (2.75, 2.40, 1.75, "pine_dark"),
            (3.75, 2.35, 1.48, "pine_light"),
            (4.65, 2.15, 1.18, "pine_dark"),
            (5.42, 1.75, 0.82, "pine_light"),
        ),
    )


def build_pine_tall(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return pine_tree(
        materials,
        "pine_tall",
        8.80,
        0.34,
        (
            (3.45, 2.90, 2.20, "pine_dark"),
            (4.72, 2.85, 1.92, "pine_light"),
            (5.92, 2.70, 1.62, "pine_dark"),
            (7.00, 2.45, 1.30, "pine_light"),
            (7.92, 2.05, 0.94, "pine_dark"),
            (8.60, 1.35, 0.58, "pine_light"),
        ),
    )


def build_dead_tree_snag(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    trunk = branch_between(
        "dead_snag_trunk",
        (0.0, 0.0, 0.0),
        (0.12, -0.05, 5.85),
        0.36,
        materials["dead_wood"],
        vertices=10,
    )
    mark_collision_part(trunk)
    objects = [trunk]
    branch_data = (
        ((0.05, 0.0, 2.15), (-1.05, 0.18, 3.10), 0.16),
        ((0.07, -0.02, 2.82), (0.95, -0.42, 3.72), 0.14),
        ((0.08, -0.02, 3.52), (-0.68, -0.62, 4.35), 0.12),
        ((0.10, -0.03, 4.16), (0.70, 0.52, 4.88), 0.105),
        ((0.11, -0.04, 4.72), (-0.36, 0.35, 5.35), 0.085),
    )
    for index, (start, end, radius) in enumerate(branch_data, start=1):
        objects.append(
            branch_between(
                f"dead_snag_branch_{index:02d}",
                start,
                end,
                radius,
                materials["dead_wood"],
                vertices=8,
            )
        )
    return objects


def build_tree_stump(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        cylinder(
            "tree_stump_body",
            0.47,
            0.68,
            (0.0, 0.0, 0.34),
            materials["bark"],
            vertices=11,
        ),
        cylinder(
            "tree_stump_cut",
            0.42,
            0.035,
            (0.0, 0.0, 0.695),
            materials["weathered"],
            vertices=11,
        ),
    ]
    for index, angle in enumerate((0.0, 1.45, 2.82, 4.35, 5.45), start=1):
        objects.append(
            box(
                f"tree_stump_root_{index:02d}",
                (0.72, 0.18, 0.16),
                (math.cos(angle) * 0.31, math.sin(angle) * 0.31, 0.08),
                materials["bark"],
                0.025,
                rotation=(0.0, 0.0, angle),
            )
        )
    return objects


def build_fallen_branch_pile(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    branch_data = (
        ((-0.95, -0.38, 0.12), (0.90, 0.34, 0.26), 0.10),
        ((-0.82, 0.40, 0.18), (0.88, -0.28, 0.36), 0.095),
        ((-0.65, -0.05, 0.30), (0.72, 0.44, 0.48), 0.075),
        ((-0.55, 0.32, 0.38), (0.55, -0.35, 0.56), 0.065),
        ((-0.35, -0.42, 0.20), (0.38, 0.46, 0.62), 0.055),
        ((-0.22, 0.05, 0.52), (0.66, 0.18, 0.72), 0.045),
    )
    return [
        branch_between(
            f"fallen_branch_{index:02d}",
            start,
            end,
            radius,
            materials["dead_wood"],
            vertices=8,
        )
        for index, (start, end, radius) in enumerate(branch_data, start=1)
    ]


ShrubMass = tuple[
    tuple[float, float, float],
    tuple[float, float, float],
    str,
]


def shrub(
    materials: dict[str, bpy.types.Material],
    prefix: str,
    masses: tuple[ShrubMass, ...],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    for index, (location, dimensions, material_key) in enumerate(
        masses, start=1
    ):
        objects.append(
            ellipsoid(
                f"{prefix}_mass_{index:02d}",
                dimensions,
                location,
                materials[material_key],
                rotation=(0.0, 0.0, math.radians(index * 31.0)),
                subdivisions=1,
            )
        )
    return objects


def build_shrub_low(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return shrub(
        materials,
        "shrub_low",
        (
            ((-0.34, 0.02, 0.34), (0.85, 0.72, 0.62), "leaf_mature"),
            ((0.34, -0.08, 0.36), (0.88, 0.75, 0.66), "leaf_young"),
            ((0.0, 0.28, 0.42), (0.95, 0.78, 0.72), "leaf_mature"),
            ((0.06, -0.30, 0.30), (0.82, 0.66, 0.54), "leaf_old"),
        ),
    )


def build_shrub_dense(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return shrub(
        materials,
        "shrub_dense",
        (
            ((-0.52, 0.02, 0.53), (1.12, 1.00, 1.02), "leaf_mature"),
            ((0.50, -0.10, 0.56), (1.18, 1.02, 1.05), "leaf_young"),
            ((0.0, 0.46, 0.65), (1.20, 1.05, 1.18), "leaf_old"),
            ((0.02, -0.48, 0.48), (1.08, 0.94, 0.90), "leaf_mature"),
            ((-0.28, -0.20, 0.92), (1.05, 0.94, 1.04), "leaf_young"),
            ((0.38, 0.20, 0.96), (1.02, 0.92, 1.08), "leaf_mature"),
        ),
    )


def build_fern_cluster(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    lengths = (0.92, 0.76, 1.04, 0.82, 0.96, 0.72, 1.00, 0.84, 0.90, 0.78)
    for index, length in enumerate(lengths):
        angle = index * math.tau / len(lengths) + (index % 2) * 0.13
        radius = length * 0.34
        objects.append(
            ellipsoid(
                f"fern_frond_{index + 1:02d}",
                (length, 0.18, 0.075),
                (math.cos(angle) * radius, math.sin(angle) * radius,
                 0.16 + (index % 3) * 0.035),
                materials["fern"],
                rotation=(0.0, 0.0, angle),
                subdivisions=1,
            )
        )
    objects.append(
        ellipsoid(
            "fern_crown",
            (0.42, 0.42, 0.34),
            (0.0, 0.0, 0.17),
            materials["leaf_mature"],
            subdivisions=1,
        )
    )
    return objects


def build_reed_grass_clump(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    stem_data = (
        (-0.28, -0.18, 1.15, 0.04, 0.01),
        (-0.12, 0.08, 1.42, -0.02, 0.04),
        (0.02, -0.12, 1.28, 0.03, -0.02),
        (0.18, 0.14, 1.36, -0.01, 0.03),
        (0.34, -0.02, 1.08, 0.02, -0.04),
        (-0.36, 0.16, 1.24, -0.03, 0.01),
        (0.10, 0.32, 1.18, 0.04, 0.02),
        (-0.05, -0.34, 1.32, -0.02, -0.03),
    )
    objects: list[bpy.types.Object] = []
    for index, (x, y, height, lean_x, lean_y) in enumerate(stem_data, start=1):
        end = (x + lean_x, y + lean_y, height)
        objects.append(
            branch_between(
                f"reed_stem_{index:02d}",
                (x, y, 0.0),
                end,
                0.018,
                materials["reed"],
                vertices=7,
            )
        )
        objects.append(
            ellipsoid(
                f"reed_seed_{index:02d}",
                (0.10, 0.10, 0.26),
                (end[0], end[1], end[2] + 0.10),
                materials["seed"],
                subdivisions=1,
            )
        )
    for index, angle in enumerate((0.2, 1.1, 2.0, 2.8, 3.8, 4.6, 5.4), start=1):
        objects.append(
            ellipsoid(
                f"grass_blade_{index:02d}",
                (0.72, 0.075, 0.045),
                (math.cos(angle) * 0.22, math.sin(angle) * 0.22, 0.10),
                materials["fern"],
                rotation=(0.0, 0.0, angle),
                subdivisions=1,
            )
        )
    return objects


def build_rock_cluster_small(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    rock_data = (
        ((-0.42, 0.02, 0.28), (0.82, 0.70, 0.56), "stone", 8.0),
        ((0.30, -0.18, 0.23), (0.68, 0.56, 0.46), "moss_stone", -14.0),
        ((0.18, 0.34, 0.17), (0.52, 0.46, 0.34), "stone", 22.0),
        ((-0.35, 0.42, 0.14), (0.44, 0.36, 0.28), "moss_stone", -5.0),
        ((0.58, 0.28, 0.11), (0.34, 0.30, 0.22), "stone", 31.0),
    )
    return [
        ellipsoid(
            f"rock_cluster_stone_{index:02d}",
            dimensions,
            location,
            materials[material_key],
            rotation=(0.0, math.radians(rotation_y),
                      math.radians(index * 27.0)),
            subdivisions=1,
        )
        for index, (location, dimensions, material_key, rotation_y) in enumerate(
            rock_data, start=1
        )
    ]


ASSET_SPECS = (
    AssetSpec("broadleaf_young_4p7m", "tree", "compound_bounds", False,
              build_broadleaf_young, (-11.5, 3.2, 0.0)),
    AssetSpec("broadleaf_mature_7p3m", "tree", "compound_bounds", False,
              build_broadleaf_mature, (-7.0, 3.2, 0.0)),
    AssetSpec("broadleaf_ancient_9p5m", "tree", "compound_bounds", False,
              build_broadleaf_ancient, (-1.4, 3.2, 0.0)),
    AssetSpec("pine_medium_6p3m", "tree", "compound_bounds", False,
              build_pine_medium, (4.3, 3.2, 0.0)),
    AssetSpec("pine_tall_9p3m", "tree", "compound_bounds", False,
              build_pine_tall, (8.7, 3.2, 0.0)),
    AssetSpec("dead_tree_snag_6m", "tree", "compound_bounds", False,
              build_dead_tree_snag, (12.7, 3.2, 0.0)),
    AssetSpec("tree_stump_0p8m", "log", "bounds", False,
              build_tree_stump, (-10.5, -4.6, 0.0)),
    AssetSpec("fallen_branch_pile_2m", "log", "none", False,
              build_fallen_branch_pile, (-7.1, -4.6, 0.0)),
    AssetSpec("shrub_low_1p2m", "shrub", "none", False,
              build_shrub_low, (-3.8, -4.6, 0.0)),
    AssetSpec("shrub_dense_1p8m", "shrub", "none", False,
              build_shrub_dense, (-0.7, -4.6, 0.0)),
    AssetSpec("fern_cluster_1p2m", "foliage", "none", False,
              build_fern_cluster, (2.5, -4.6, 0.0)),
    AssetSpec("reed_grass_clump_1p4m", "foliage", "none", False,
              build_reed_grass_clump, (5.3, -4.6, 0.0)),
    AssetSpec("rock_cluster_small_1p5m", "rock", "none", False,
              build_rock_cluster_small, (8.5, -4.6, 0.0)),
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
    bpy.ops.object.camera_add(location=(0.0, -38.0, 16.5))
    camera = bpy.context.object
    camera.name = "Woodland Gallery Camera"
    camera.data.lens = 54.0
    look_at(camera, (0.2, 0.0, 3.20))
    bpy.context.scene.camera = camera
    configure_render(preview, 1920, 1080)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def build_woodland_preview(
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
        (build_broadleaf_young, (-8.2, 1.2, 0.0), math.radians(-12.0)),
        (build_broadleaf_ancient, (-4.2, 3.6, 0.0), math.radians(8.0)),
        (build_broadleaf_mature, (0.5, 4.1, 0.0), math.radians(-7.0)),
        (build_dead_tree_snag, (2.8, 1.7, 0.0), math.radians(18.0)),
        (build_pine_tall, (6.2, 4.0, 0.0), math.radians(-6.0)),
        (build_pine_medium, (9.0, 1.9, 0.0), math.radians(11.0)),
        (build_tree_stump, (-6.5, -1.7, 0.0), math.radians(14.0)),
        (build_fallen_branch_pile, (-3.5, -1.8, 0.0), math.radians(-20.0)),
        (build_shrub_low, (-7.2, 0.1, 0.0), math.radians(25.0)),
        (build_shrub_dense, (0.8, 0.5, 0.0), math.radians(-10.0)),
        (build_fern_cluster, (-1.2, -2.1, 0.0), math.radians(17.0)),
        (build_reed_grass_clump, (5.2, -1.8, 0.0), math.radians(-8.0)),
        (build_rock_cluster_small, (7.6, -1.4, 0.0), math.radians(9.0)),
    )
    for builder, location, rotation in placements:
        place_asset(builder, materials, location, rotation)
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(22.0, -27.0, 15.5))
    camera = bpy.context.object
    camera.name = "Woodland Edge Camera"
    camera.data.lens = 58.0
    look_at(camera, (0.2, 1.15, 3.25))
    bpy.context.scene.camera = camera
    configure_render(preview, 1600, 1100)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_woodland_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_gallery_preview(materials, arguments.preview, arguments.blend)
    if arguments.woodland_preview is not None:
        build_woodland_preview(materials, arguments.woodland_preview)
    print(f"woodland edge kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
