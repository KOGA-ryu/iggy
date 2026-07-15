#!/usr/bin/env python3
"""Build the Iggy3D homestead yard prop kit in Blender."""

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
    build_interior_materials,
    build_lantern,
    configure_render,
    cylinder,
)
from build_homestead_modular_kit import (  # noqa: E402
    AssetSpec,
    add_preview_environment,
    assign_material,
    attachment_socket,
    box,
    clear_objects,
    export_asset,
    look_at,
    mark_collision_part,
    material,
    place_asset,
)


def script_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--preview", type=pathlib.Path)
    parser.add_argument("--yard-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def build_yard_materials() -> dict[str, bpy.types.Material]:
    values = build_interior_materials()
    values.update(
        {
            "weathered": material(
                "Homestead Weathered Oak", (0.25, 0.15, 0.075, 1.0), 0.91
            ),
            "straw": material(
                "Homestead Straw", (0.63, 0.45, 0.14, 1.0), 0.96
            ),
            "rope": material(
                "Homestead Rope", (0.34, 0.22, 0.10, 1.0), 0.98
            ),
            "earth": material(
                "Homestead Earth", (0.12, 0.075, 0.038, 1.0), 1.0
            ),
        }
    )
    return values


def torus(
    name: str,
    major_radius: float,
    minor_radius: float,
    location: tuple[float, float, float],
    value: bpy.types.Material,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=16,
        minor_segments=6,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, value)
    return obj


def fence_section(
    materials: dict[str, bpy.types.Material], width: float, prefix: str
) -> list[bpy.types.Object]:
    objects = [
        box(f"{prefix}_left_post", (0.18, 0.18, 1.35),
            (-width * 0.5, 0.0, 0.675), materials["dark_timber"], 0.028),
        box(f"{prefix}_right_post", (0.18, 0.18, 1.35),
            (width * 0.5, 0.0, 0.675), materials["dark_timber"], 0.028),
    ]
    for index, z in enumerate((0.44, 0.96), start=1):
        objects.append(
            box(f"{prefix}_rail_{index:02d}", (width, 0.12, 0.14),
                (0.0, 0.0, z), materials["weathered"], 0.022)
        )
    return objects


def build_fence_2m(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    return fence_section(materials, 2.0, "fence_2m")


def build_fence_4m(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = fence_section(materials, 4.0, "fence_4m")
    objects.append(
        box("fence_4m_center_post", (0.16, 0.16, 1.28),
            (0.0, 0.0, 0.64), materials["timber"], 0.025)
    )
    return objects


def build_fence_corner(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        box("fence_corner_post", (0.20, 0.20, 1.42),
            (0.0, 0.0, 0.71), materials["dark_timber"], 0.032),
        box("fence_corner_x_post", (0.18, 0.18, 1.35),
            (2.0, 0.0, 0.675), materials["dark_timber"], 0.028),
        box("fence_corner_y_post", (0.18, 0.18, 1.35),
            (0.0, 2.0, 0.675), materials["dark_timber"], 0.028),
    ]
    for index, z in enumerate((0.44, 0.96), start=1):
        objects.extend(
            [
                box(f"fence_corner_x_rail_{index:02d}", (2.0, 0.12, 0.14),
                    (1.0, 0.0, z), materials["weathered"], 0.022),
                box(f"fence_corner_y_rail_{index:02d}", (0.12, 2.0, 0.14),
                    (0.0, 1.0, z), materials["weathered"], 0.022),
            ]
        )
    for obj in objects:
        mark_collision_part(obj)
    return objects


def build_gate_frame(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    opening_width = 1.55
    post_width = 0.24
    outer_width = opening_width + 2.0 * post_width
    objects = [
        box("yard_gate_left_post", (post_width, 0.28, 2.05),
            (-outer_width * 0.5 + post_width * 0.5, 0.0, 1.025),
            materials["dark_timber"], 0.035),
        box("yard_gate_right_post", (post_width, 0.28, 2.05),
            (outer_width * 0.5 - post_width * 0.5, 0.0, 1.025),
            materials["dark_timber"], 0.035),
        box("yard_gate_header", (outer_width, 0.24, 0.22),
            (0.0, 0.0, 1.94), materials["weathered"], 0.032),
    ]
    for obj in objects:
        mark_collision_part(obj)
    objects.append(
        attachment_socket("yard_gate", "yard.gate", "receiver",
                          (0.0, 0.0, 0.68))
    )
    return objects


def build_gate_leaf(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    width = 1.40
    height = 1.28
    objects = [
        box("yard_gate_leaf_left", (0.12, 0.14, height),
            (-width * 0.5 + 0.06, 0.0, height * 0.5),
            materials["dark_timber"], 0.022),
        box("yard_gate_leaf_right", (0.12, 0.14, height),
            (width * 0.5 - 0.06, 0.0, height * 0.5),
            materials["dark_timber"], 0.022),
    ]
    for index, z in enumerate((0.20, 0.66, 1.12), start=1):
        objects.append(
            box(f"yard_gate_leaf_rail_{index:02d}", (width, 0.12, 0.13),
                (0.0, 0.0, z), materials["weathered"], 0.022)
        )
    brace_length = math.sqrt(width * width + 0.76 * 0.76)
    brace_angle = -math.atan2(0.76, width)
    objects.append(
        box("yard_gate_leaf_brace", (brace_length, 0.10, 0.12),
            (0.0, -0.02, 0.66), materials["timber"], 0.018,
            rotation=(0.0, brace_angle, 0.0))
    )
    objects.append(
        box("yard_gate_latch", (0.18, 0.09, 0.12),
            (0.46, -0.10, 0.76), materials["iron"], 0.018)
    )
    objects.append(
        attachment_socket("yard_gate_leaf", "yard.gate", "plug",
                          (0.0, 0.0, 0.68))
    )
    return objects


def build_well(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    stone_count = 12
    for row, z in enumerate((0.16, 0.46), start=1):
        angle_offset = math.pi / stone_count if row == 2 else 0.0
        for index in range(stone_count):
            angle = angle_offset + index * math.tau / stone_count
            radius = 0.72
            objects.append(
                box(
                    f"well_stone_{row}_{index + 1:02d}",
                    (0.40, 0.28, 0.26),
                    (math.cos(angle) * radius, math.sin(angle) * radius, z),
                    materials["stone"],
                    0.035,
                    rotation=(0.0, 0.0, angle + math.pi * 0.5),
                )
            )
    for x in (-0.86, 0.86):
        objects.append(
            box(f"well_roof_post_{x:+.2f}", (0.16, 0.18, 2.15),
                (x, 0.0, 1.075), materials["dark_timber"], 0.030)
        )
    objects.extend(
        [
            box("well_roof_left", (1.10, 2.10, 0.14), (-0.46, 0.0, 2.18),
                materials["roof"], 0.025,
                rotation=(0.0, math.radians(-28.0), 0.0)),
            box("well_roof_right", (1.10, 2.10, 0.14), (0.46, 0.0, 2.18),
                materials["roof"], 0.025,
                rotation=(0.0, math.radians(28.0), 0.0)),
            cylinder("well_axle", 0.075, 1.72, (0.0, 0.0, 1.34),
                     materials["iron"], vertices=12,
                     rotation=(0.0, math.radians(90.0), 0.0)),
            cylinder("well_rope", 0.035, 0.72, (0.0, 0.0, 0.98),
                     materials["rope"], vertices=10),
        ]
    )
    return objects


def add_wheel(
    objects: list[bpy.types.Object],
    materials: dict[str, bpy.types.Material],
    y: float,
) -> None:
    objects.append(
        torus("cart_wheel_rim", 0.39, 0.055, (0.34, y, 0.47),
              materials["iron"], rotation=(math.radians(90.0), 0.0, 0.0))
    )
    objects.append(
        cylinder("cart_wheel_hub", 0.09, 0.16, (0.34, y, 0.47),
                 materials["dark_timber"], vertices=12,
                 rotation=(math.radians(90.0), 0.0, 0.0))
    )
    for index, angle in enumerate((0.0, math.pi * 0.25, math.pi * 0.5,
                                   math.pi * 0.75), start=1):
        objects.append(
            box(f"cart_wheel_spoke_{y:+.2f}_{index:02d}",
                (0.72, 0.055, 0.055), (0.34, y, 0.47),
                materials["weathered"], 0.008,
                rotation=(0.0, angle, 0.0))
        )


def build_handcart(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        box("cart_bed", (1.45, 0.92, 0.15), (0.12, 0.0, 0.72),
            materials["floor"], 0.025),
        box("cart_left_side", (1.35, 0.12, 0.44), (0.16, -0.46, 0.91),
            materials["weathered"], 0.025),
        box("cart_right_side", (1.35, 0.12, 0.44), (0.16, 0.46, 0.91),
            materials["weathered"], 0.025),
        box("cart_back", (0.13, 0.92, 0.44), (0.80, 0.0, 0.91),
            materials["weathered"], 0.025),
        box("cart_left_handle", (1.45, 0.08, 0.09), (-0.93, -0.34, 0.62),
            materials["dark_timber"], 0.018),
        box("cart_right_handle", (1.45, 0.08, 0.09), (-0.93, 0.34, 0.62),
            materials["dark_timber"], 0.018),
    ]
    add_wheel(objects, materials, -0.54)
    add_wheel(objects, materials, 0.54)
    return objects


def build_signpost(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        box("signpost_pole", (0.18, 0.18, 2.25), (0.0, 0.0, 1.125),
            materials["dark_timber"], 0.030),
        box("signpost_board", (1.18, 0.12, 0.42), (0.42, 0.0, 1.72),
            materials["weathered"], 0.035),
        box("signpost_arrow", (0.28, 0.10, 0.28), (1.02, 0.0, 1.72),
            materials["weathered"], 0.020,
            rotation=(0.0, 0.0, math.radians(45.0))),
        box("signpost_hook", (0.48, 0.07, 0.07), (-0.31, 0.0, 1.62),
            materials["iron"], 0.015),
    ]
    objects.append(
        attachment_socket("lantern_hook", "decor.surface", "receiver",
                          (-0.53, 0.0, 1.55))
    )
    return objects


def build_woodpile(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects: list[bpy.types.Object] = []
    rows = ((-0.44, 0.18), (-0.15, 0.18), (0.15, 0.18), (0.44, 0.18),
            (-0.29, 0.44), (0.0, 0.44), (0.29, 0.44),
            (-0.15, 0.70), (0.15, 0.70))
    for index, (y, z) in enumerate(rows, start=1):
        objects.append(
            cylinder(f"woodpile_log_{index:02d}", 0.13, 1.45,
                     (0.0, y, z), materials["weathered"], vertices=10,
                     rotation=(0.0, math.radians(90.0), 0.0))
        )
    objects.extend(
        [
            box("woodpile_left_stop", (0.12, 1.25, 0.90),
                (-0.78, 0.0, 0.45), materials["dark_timber"], 0.018),
            box("woodpile_right_stop", (0.12, 1.25, 0.90),
                (0.78, 0.0, 0.45), materials["dark_timber"], 0.018),
        ]
    )
    return objects


def build_trough(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("trough_base", (1.55, 0.58, 0.12), (0.0, 0.0, 0.43),
            materials["dark_timber"], 0.022),
        box("trough_left", (1.62, 0.12, 0.48), (0.0, -0.31, 0.62),
            materials["weathered"], 0.030,
            rotation=(math.radians(-8.0), 0.0, 0.0)),
        box("trough_right", (1.62, 0.12, 0.48), (0.0, 0.31, 0.62),
            materials["weathered"], 0.030,
            rotation=(math.radians(8.0), 0.0, 0.0)),
        box("trough_end_a", (0.14, 0.70, 0.50), (-0.74, 0.0, 0.62),
            materials["timber"], 0.028),
        box("trough_end_b", (0.14, 0.70, 0.50), (0.74, 0.0, 0.62),
            materials["timber"], 0.028),
    ]
    for x in (-0.56, 0.56):
        for y in (-0.22, 0.22):
            objects.append(
                box(f"trough_leg_{x:+.2f}_{y:+.2f}", (0.11, 0.11, 0.40),
                    (x, y, 0.20), materials["dark_timber"], 0.020)
            )
    return objects


def build_hay_bale(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        box("hay_bale_body", (1.02, 0.62, 0.62), (0.0, 0.0, 0.31),
            materials["straw"], 0.095),
    ]
    for x in (-0.25, 0.25):
        objects.append(
            box(f"hay_bale_rope_{x:+.2f}", (0.045, 0.66, 0.66),
                (x, 0.0, 0.31), materials["rope"], 0.012)
        )
    for index, z in enumerate((0.10, 0.31, 0.52), start=1):
        objects.append(
            box(f"hay_bale_layer_{index:02d}", (1.04, 0.025, 0.035),
                (0.0, -0.325, z), materials["rope"], 0.008)
        )
    return objects


ASSET_SPECS = (
    AssetSpec("fence_2m", "prop", "bounds", False, build_fence_2m,
              (-6.0, 3.0, 0.0)),
    AssetSpec("fence_4m", "prop", "bounds", False, build_fence_4m,
              (-2.5, 3.0, 0.0)),
    AssetSpec("fence_corner_2x2", "prop", "compound_bounds", False,
              build_fence_corner, (2.0, 3.0, 0.0)),
    AssetSpec("gate_frame_1p8", "prop", "compound_bounds", False,
              build_gate_frame, (6.0, 3.0, 0.0)),
    AssetSpec("gate_leaf_1p4", "prop", "bounds", False, build_gate_leaf,
              (-6.0, -1.0, 0.0)),
    AssetSpec("well_roofed_1p8", "prop", "bounds", False, build_well,
              (-3.0, -1.0, 0.0)),
    AssetSpec("handcart_2p4", "prop", "bounds", False, build_handcart,
              (0.5, -1.0, 0.0)),
    AssetSpec("signpost_2p2", "prop", "bounds", False, build_signpost,
              (4.0, -1.0, 0.0)),
    AssetSpec("woodpile_1p5", "prop", "bounds", False, build_woodpile,
              (6.5, -1.0, 0.0)),
    AssetSpec("trough_1p6", "prop", "bounds", False, build_trough,
              (-3.0, -4.5, 0.0)),
    AssetSpec("hay_bale_1p0", "prop", "bounds", False, build_hay_bale,
              (0.5, -4.5, 0.0)),
)


def place_builder(
    builder: Callable[[dict[str, bpy.types.Material]], list[bpy.types.Object]],
    materials: dict[str, bpy.types.Material],
    location: tuple[float, float, float],
    rotation_z: float = 0.0,
) -> list[bpy.types.Object]:
    return place_asset(builder, materials, location, rotation_z)


def build_gallery_preview(
    materials: dict[str, bpy.types.Material],
    preview: pathlib.Path,
    blend: pathlib.Path | None,
) -> None:
    clear_objects()
    for spec in ASSET_SPECS:
        objects = spec.builder(materials)
        offset = Vector(spec.gallery_position)
        for obj in objects:
            obj.location += offset
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(16.5, -25.0, 15.5))
    camera = bpy.context.object
    camera.name = "Yard Gallery Camera"
    camera.data.lens = 58.0
    look_at(camera, (0.0, -0.6, 0.8))
    bpy.context.scene.camera = camera
    configure_render(preview, 1920, 1080)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def build_yard_preview(
    materials: dict[str, bpy.types.Material], preview: pathlib.Path
) -> None:
    clear_objects()
    place_builder(build_fence_4m, materials, (-2.1, 3.5, 0.0))
    place_builder(build_fence_4m, materials, (2.1, 3.5, 0.0))
    place_builder(build_fence_4m, materials, (-4.1, 1.5, 0.0),
                  math.radians(90.0))
    place_builder(build_fence_4m, materials, (4.1, 1.5, 0.0),
                  math.radians(90.0))
    place_builder(build_fence_2m, materials, (-2.2, -2.5, 0.0))
    place_builder(build_fence_2m, materials, (2.2, -2.5, 0.0))
    place_builder(build_gate_frame, materials, (0.0, -2.5, 0.0))
    place_builder(build_gate_leaf, materials, (0.0, -2.5, 0.0))
    place_builder(build_well, materials, (-1.7, 1.15, 0.0))
    place_builder(build_handcart, materials, (1.55, 0.95, 0.0),
                  math.radians(-18.0))
    place_builder(build_signpost, materials, (-4.8, -2.4, 0.0))
    place_builder(build_lantern, materials, (-5.33, -2.4, 1.55))
    place_builder(build_woodpile, materials, (2.65, 2.55, 0.0),
                  math.radians(6.0))
    place_builder(build_trough, materials, (2.35, -0.85, 0.0),
                  math.radians(-10.0))
    place_builder(build_hay_bale, materials, (-3.0, 2.35, 0.0),
                  math.radians(12.0))
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(14.2, -19.0, 12.0))
    camera = bpy.context.object
    camera.name = "Yard Assembly Camera"
    camera.data.lens = 58.0
    look_at(camera, (-0.2, 0.45, 0.75))
    bpy.context.scene.camera = camera
    configure_render(preview, 1600, 1100)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_yard_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_gallery_preview(materials, arguments.preview, arguments.blend)
    if arguments.yard_preview is not None:
        build_yard_preview(materials, arguments.yard_preview)
    print(f"homestead yard kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
