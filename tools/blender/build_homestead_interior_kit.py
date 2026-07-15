#!/usr/bin/env python3
"""Build the Iggy3D homestead interior prop kit in Blender."""

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

from build_homestead_modular_kit import (  # noqa: E402
    AssetSpec,
    add_preview_environment,
    apply_metadata,
    assign_material,
    attachment_socket,
    box,
    build_floor,
    build_materials,
    build_wall_full,
    build_wall_half,
    clear_objects,
    export_asset,
    look_at,
    material,
    place_asset,
)


def script_arguments() -> argparse.Namespace:
    arguments = sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-root", type=pathlib.Path, required=True)
    parser.add_argument("--preview", type=pathlib.Path)
    parser.add_argument("--room-preview", type=pathlib.Path)
    parser.add_argument("--blend", type=pathlib.Path)
    return parser.parse_args(arguments)


def build_interior_materials() -> dict[str, bpy.types.Material]:
    values = build_materials()
    values.update(
        {
            "linen": material(
                "Homestead Linen", (0.72, 0.68, 0.56, 1.0), 0.92
            ),
            "blanket": material(
                "Homestead Moss Blanket", (0.19, 0.31, 0.20, 1.0), 0.90
            ),
            "leather": material(
                "Homestead Leather", (0.27, 0.095, 0.035, 1.0), 0.76
            ),
            "book_red": material(
                "Homestead Book Red", (0.43, 0.075, 0.045, 1.0), 0.79
            ),
            "book_blue": material(
                "Homestead Book Blue", (0.08, 0.16, 0.31, 1.0), 0.79
            ),
            "book_green": material(
                "Homestead Book Green", (0.12, 0.28, 0.16, 1.0), 0.82
            ),
            "soot": material(
                "Homestead Soot", (0.018, 0.014, 0.012, 1.0), 0.98
            ),
            "ember": material(
                "Homestead Ember", (0.90, 0.16, 0.025, 1.0), 0.52
            ),
            "lantern": material(
                "Homestead Lantern Glow", (0.95, 0.52, 0.08, 1.0), 0.38
            ),
        }
    )
    glow = values["lantern"].node_tree.nodes.get("Principled BSDF")
    emission = glow.inputs.get("Emission Color")
    strength = glow.inputs.get("Emission Strength")
    if emission is not None:
        emission.default_value = (1.0, 0.28, 0.025, 1.0)
    if strength is not None:
        strength.default_value = 2.4
    return values


def cylinder(
    name: str,
    radius: float,
    height: float,
    location: tuple[float, float, float],
    value: bpy.types.Material,
    vertices: int = 16,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=height,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, value)
    bevel = obj.modifiers.new("Edge Softening", "BEVEL")
    bevel.width = min(0.018, radius * 0.12, height * 0.12)
    bevel.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj


def cone(
    name: str,
    radius_bottom: float,
    radius_top: float,
    height: float,
    location: tuple[float, float, float],
    value: bpy.types.Material,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cone_add(
        vertices=12,
        radius1=radius_bottom,
        radius2=radius_top,
        depth=height,
        location=location,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, value)
    return obj


def receiver(
    name: str,
    location: tuple[float, float, float],
    compatibility: str = "decor.surface",
) -> bpy.types.Object:
    return attachment_socket(name, compatibility, "receiver", location)


def plug(
    name: str,
    location: tuple[float, float, float],
    compatibility: str = "decor.surface",
) -> bpy.types.Object:
    return attachment_socket(name, compatibility, "plug", location)


def build_bed(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("bed_left_rail", (2.08, 0.10, 0.30), (0.0, -0.50, 0.32),
            materials["timber"], 0.025),
        box("bed_right_rail", (2.08, 0.10, 0.30), (0.0, 0.50, 0.32),
            materials["timber"], 0.025),
        box("bed_foot", (0.12, 1.10, 0.56), (0.98, 0.0, 0.32),
            materials["dark_timber"], 0.028),
        box("bed_head", (0.14, 1.14, 1.18), (-0.99, 0.0, 0.59),
            materials["dark_timber"], 0.032),
        box("bed_mattress", (1.88, 0.94, 0.24), (0.0, 0.0, 0.57),
            materials["linen"], 0.065),
        box("bed_blanket", (0.92, 0.97, 0.065), (0.40, 0.0, 0.72),
            materials["blanket"], 0.022),
        box("bed_pillow", (0.46, 0.70, 0.13), (-0.65, 0.0, 0.78),
            materials["linen"], 0.055),
    ]
    return objects


def build_chair(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("chair_seat", (0.56, 0.55, 0.11), (0.0, 0.0, 0.52),
            materials["floor"], 0.035),
    ]
    for x in (-0.22, 0.22):
        for y in (-0.20, 0.20):
            objects.append(
                box(f"chair_leg_{x:+.2f}_{y:+.2f}", (0.085, 0.085, 0.48),
                    (x, y, 0.24), materials["timber"], 0.018)
            )
    for x in (-0.23, 0.23):
        objects.append(
            box(f"chair_back_post_{x:+.2f}", (0.085, 0.085, 0.84),
                (x, 0.22, 0.91), materials["dark_timber"], 0.018)
        )
    for index, z in enumerate((0.76, 0.96, 1.16), start=1):
        objects.append(
            box(f"chair_back_slat_{index:02d}", (0.40, 0.065, 0.075),
                (0.0, 0.22, z), materials["timber"], 0.018)
        )
    return objects


def build_bench(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("bench_seat", (1.60, 0.48, 0.13), (0.0, 0.0, 0.54),
            materials["floor"], 0.038),
        box("bench_brace", (1.18, 0.10, 0.11), (0.0, 0.0, 0.26),
            materials["dark_timber"], 0.020),
    ]
    for x in (-0.64, 0.64):
        for y in (-0.16, 0.16):
            objects.append(
                box(f"bench_leg_{x:+.2f}_{y:+.2f}", (0.11, 0.11, 0.49),
                    (x, y, 0.245), materials["timber"], 0.022)
            )
    return objects


def add_book_row(
    objects: list[bpy.types.Object],
    materials: dict[str, bpy.types.Material],
    shelf_z: float,
    row: int,
) -> None:
    widths = (0.10, 0.13, 0.085, 0.12, 0.095, 0.14)
    heights = (0.36, 0.42, 0.33, 0.45, 0.38, 0.40)
    colors = ("book_red", "book_blue", "book_green")
    x = -0.40
    for index, (width, height) in enumerate(zip(widths, heights), start=1):
        x += width * 0.5
        objects.append(
            box(
                f"bookshelf_book_{row}_{index:02d}",
                (width, 0.25, height),
                (x, -0.035, shelf_z + 0.055 + height * 0.5),
                materials[colors[(row + index) % len(colors)]],
                0.008,
            )
        )
        x += width * 0.5 + 0.018


def build_bookshelf(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        box("bookshelf_back", (1.10, 0.07, 2.00), (0.0, 0.16, 1.0),
            materials["dark_timber"], 0.018),
        box("bookshelf_left", (0.10, 0.40, 2.05), (-0.50, 0.0, 1.025),
            materials["timber"], 0.025),
        box("bookshelf_right", (0.10, 0.40, 2.05), (0.50, 0.0, 1.025),
            materials["timber"], 0.025),
        box("bookshelf_crown", (1.18, 0.46, 0.12), (0.0, 0.0, 2.04),
            materials["dark_timber"], 0.030),
    ]
    for index, z in enumerate((0.08, 0.57, 1.06, 1.55), start=1):
        objects.append(
            box(f"bookshelf_shelf_{index:02d}", (1.02, 0.39, 0.10),
                (0.0, 0.0, z), materials["floor"], 0.018)
        )
    add_book_row(objects, materials, 0.08, 1)
    add_book_row(objects, materials, 0.57, 2)
    add_book_row(objects, materials, 1.06, 3)
    objects.append(receiver("top_center", (0.0, -0.02, 2.11)))
    return objects


def build_dresser(
    materials: dict[str, bpy.types.Material],
) -> list[bpy.types.Object]:
    objects = [
        box("dresser_body", (1.24, 0.54, 0.96), (0.0, 0.0, 0.50),
            materials["timber"], 0.035),
        box("dresser_top", (1.34, 0.62, 0.10), (0.0, 0.0, 1.03),
            materials["dark_timber"], 0.032),
        box("dresser_plinth", (1.30, 0.58, 0.12), (0.0, 0.0, 0.08),
            materials["dark_timber"], 0.025),
    ]
    for row, z in enumerate((0.30, 0.57, 0.84), start=1):
        objects.append(
            box(f"dresser_drawer_{row:02d}", (1.08, 0.055, 0.22),
                (0.0, -0.298, z), materials["floor"], 0.018)
        )
        for side, x in enumerate((-0.24, 0.24), start=1):
            objects.append(
                cylinder(
                    f"dresser_knob_{row:02d}_{side:02d}",
                    0.035,
                    0.065,
                    (x, -0.345, z),
                    materials["iron"],
                    vertices=12,
                    rotation=(math.radians(90.0), 0.0, 0.0),
                )
            )
    objects.append(receiver("top_center", (0.0, -0.02, 1.11)))
    return objects


def build_chest(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("chest_body", (1.06, 0.62, 0.56), (0.0, 0.0, 0.31),
            materials["timber"], 0.045),
        box("chest_lid", (1.12, 0.68, 0.18), (0.0, 0.0, 0.67),
            materials["floor"], 0.065),
        box("chest_front_band", (0.09, 0.045, 0.70), (0.0, -0.345, 0.38),
            materials["iron"], 0.012),
        box("chest_left_band", (0.08, 0.70, 0.72), (-0.40, 0.0, 0.38),
            materials["iron"], 0.012),
        box("chest_right_band", (0.08, 0.70, 0.72), (0.40, 0.0, 0.38),
            materials["iron"], 0.012),
        box("chest_latch", (0.16, 0.065, 0.22), (0.0, -0.37, 0.53),
            materials["iron"], 0.018),
    ]
    objects.append(receiver("top_center", (0.0, 0.0, 0.80)))
    return objects


def build_hearth(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("hearth_back", (1.52, 0.22, 1.46), (0.0, 0.20, 0.73),
            materials["stone"], 0.030),
        box("hearth_left", (0.34, 0.70, 1.38), (-0.59, 0.0, 0.69),
            materials["stone"], 0.045),
        box("hearth_right", (0.34, 0.70, 1.38), (0.59, 0.0, 0.69),
            materials["stone"], 0.045),
        box("hearth_lintel", (1.52, 0.70, 0.30), (0.0, 0.0, 1.31),
            materials["stone"], 0.050),
        box("hearth_base", (1.70, 0.86, 0.16), (0.0, -0.06, 0.08),
            materials["stone"], 0.040),
        box("hearth_firebox", (0.86, 0.045, 0.72), (0.0, -0.365, 0.56),
            materials["soot"], 0.010),
        box("hearth_mantel", (1.82, 0.82, 0.18), (0.0, -0.02, 1.58),
            materials["dark_timber"], 0.045),
    ]
    for index, y in enumerate((-0.12, 0.05), start=1):
        objects.append(
            cylinder(
                f"hearth_log_{index:02d}",
                0.075,
                0.68,
                (0.0, y - 0.26, 0.25 + index * 0.025),
                materials["timber"],
                vertices=12,
                rotation=(0.0, math.radians(90.0), 0.0),
            )
        )
    objects.append(
        box("hearth_embers", (0.62, 0.24, 0.06), (0.0, -0.28, 0.20),
            materials["ember"], 0.020)
    )
    objects.append(receiver("mantel_center", (0.0, -0.08, 1.70)))
    return objects


def build_barrel(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        cylinder("barrel_body", 0.38, 0.84, (0.0, 0.0, 0.42),
                 materials["timber"], vertices=16),
        cylinder("barrel_top", 0.345, 0.035, (0.0, 0.0, 0.855),
                 materials["floor"], vertices=16),
    ]
    for index, z in enumerate((0.14, 0.42, 0.70), start=1):
        objects.append(
            cylinder(f"barrel_hoop_{index:02d}", 0.392, 0.055,
                     (0.0, 0.0, z), materials["iron"], vertices=16)
        )
    return objects


def build_crate(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        box("crate_core", (0.58, 0.58, 0.58), (0.0, 0.0, 0.31),
            materials["dark_timber"], 0.018),
    ]
    for face_y in (-0.315, 0.315):
        for index, z in enumerate((0.11, 0.31, 0.51), start=1):
            objects.append(
                box(f"crate_y_{face_y:+.3f}_{index:02d}",
                    (0.62, 0.055, 0.15), (0.0, face_y, z),
                    materials["floor"], 0.012)
            )
    for face_x in (-0.315, 0.315):
        for index, z in enumerate((0.11, 0.31, 0.51), start=1):
            objects.append(
                box(f"crate_x_{face_x:+.3f}_{index:02d}",
                    (0.055, 0.62, 0.15), (face_x, 0.0, z),
                    materials["timber"], 0.012)
            )
    objects.extend(
        [
            receiver("stack_top", (0.0, 0.0, 0.66), "storage.stack"),
            plug("stack_bottom", (0.0, 0.0, 0.0), "storage.stack"),
        ]
    )
    return objects


def build_lantern(materials: dict[str, bpy.types.Material]) -> list[bpy.types.Object]:
    objects = [
        cylinder("lantern_base", 0.17, 0.07, (0.0, 0.0, 0.035),
                 materials["iron"], vertices=12),
        cylinder("lantern_glow", 0.105, 0.36, (0.0, 0.0, 0.28),
                 materials["lantern"], vertices=12),
        cylinder("lantern_cap", 0.14, 0.06, (0.0, 0.0, 0.49),
                 materials["iron"], vertices=12),
        cone("lantern_roof", 0.19, 0.065, 0.15, (0.0, 0.0, 0.595),
             materials["iron"]),
    ]
    for x in (-0.13, 0.13):
        for y in (-0.13, 0.13):
            objects.append(
                box(f"lantern_cage_{x:+.2f}_{y:+.2f}", (0.025, 0.025, 0.42),
                    (x, y, 0.29), materials["iron"], 0.006)
            )
    objects.append(plug("base", (0.0, 0.0, 0.0)))
    return objects


ASSET_SPECS = (
    AssetSpec("bed_single_2p1x1p1", "prop", "bounds", False, build_bed,
              (-5.0, 2.3, 0.0)),
    AssetSpec("chair_ladderback_0p56", "prop", "bounds", False, build_chair,
              (-2.5, 2.3, 0.0)),
    AssetSpec("bench_rustic_1p6", "prop", "bounds", False, build_bench,
              (0.0, 2.3, 0.0)),
    AssetSpec("bookshelf_1p1x2p1", "prop", "bounds", False, build_bookshelf,
              (3.0, 2.3, 0.0)),
    AssetSpec("dresser_1p3", "prop", "bounds", False, build_dresser,
              (5.5, 2.3, 0.0)),
    AssetSpec("storage_chest_1p1", "prop", "bounds", False, build_chest,
              (-5.0, -1.7, 0.0)),
    AssetSpec("hearth_stone_1p8", "prop", "bounds", False, build_hearth,
              (-2.0, -1.7, 0.0)),
    AssetSpec("barrel_oak_0p8", "prop", "bounds", False, build_barrel,
              (1.0, -1.7, 0.0)),
    AssetSpec("crate_wood_0p6", "prop", "bounds", False, build_crate,
              (3.3, -1.7, 0.0)),
    AssetSpec("lantern_iron_0p7", "prop", "none", False, build_lantern,
              (5.4, -1.7, 0.0)),
)


def configure_render(preview: pathlib.Path, width: int, height: int) -> None:
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE_NEXT"
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(preview)
    scene.render.film_transparent = False
    if scene.world is None:
        scene.world = bpy.data.worlds.new("Interior Preview World")
    scene.world.color = (0.018, 0.022, 0.026)
    scene.view_settings.look = "AgX - Medium High Contrast"
    preview.parent.mkdir(parents=True, exist_ok=True)


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
    bpy.ops.object.camera_add(location=(13.8, -22.5, 13.2))
    camera = bpy.context.object
    camera.name = "Interior Gallery Camera"
    camera.data.lens = 58.0
    look_at(camera, (0.0, 0.0, 0.72))
    bpy.context.scene.camera = camera
    configure_render(preview, 1920, 1080)
    bpy.ops.render.render(write_still=True)
    if blend is not None:
        blend.parent.mkdir(parents=True, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))


def build_room_preview(
    materials: dict[str, bpy.types.Material], preview: pathlib.Path
) -> None:
    clear_objects()
    place_builder(build_floor, materials, (0.0, 0.0, 0.0))
    place_builder(build_floor, materials, (4.0, 0.0, 0.0))
    place_builder(build_wall_full, materials, (0.0, 2.0, 0.24))
    place_builder(build_wall_full, materials, (4.0, 2.0, 0.24))
    place_builder(build_wall_half, materials, (-2.0, 1.0, 0.24),
                  math.radians(90.0))
    place_builder(build_wall_half, materials, (-2.0, -1.0, 0.24),
                  math.radians(90.0))
    place_builder(build_bed, materials, (-0.6, 1.05, 0.24))
    place_builder(build_bookshelf, materials, (2.0, 1.72, 0.24))
    place_builder(build_dresser, materials, (3.8, 1.65, 0.24))
    place_builder(build_lantern, materials, (3.8, 1.63, 1.35))
    place_builder(build_hearth, materials, (5.8, 1.55, 0.24))
    place_builder(build_bench, materials, (1.5, -0.30, 0.24),
                  math.radians(-8.0))
    place_builder(build_chair, materials, (3.0, -0.45, 0.24),
                  math.radians(28.0))
    place_builder(build_chest, materials, (5.4, -0.75, 0.24),
                  math.radians(-12.0))
    place_builder(build_barrel, materials, (-1.25, -1.3, 0.24))
    place_builder(build_crate, materials, (-0.55, -1.35, 0.24),
                  math.radians(10.0))
    add_preview_environment(materials)
    bpy.ops.object.camera_add(location=(11.6, -14.8, 8.6))
    camera = bpy.context.object
    camera.name = "Interior Room Camera"
    camera.data.lens = 55.0
    look_at(camera, (2.0, 0.2, 1.15))
    bpy.context.scene.camera = camera
    configure_render(preview, 1600, 1100)
    bpy.ops.render.render(write_still=True)


def main() -> None:
    arguments = script_arguments()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    materials = build_interior_materials()
    for spec in ASSET_SPECS:
        export_asset(arguments.output_root, spec, materials)
    if arguments.preview is not None:
        build_gallery_preview(materials, arguments.preview, arguments.blend)
    if arguments.room_preview is not None:
        build_room_preview(materials, arguments.room_preview)
    print(f"homestead interior kit complete: {len(ASSET_SPECS)} assets")


if __name__ == "__main__":
    main()
