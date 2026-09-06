#!/usr/bin/env python3
"""Render the authorized neutral-clay proof of the frozen GH-018 hinge.

This script is a presentation owner, not a second geometry owner. It opens the
validated geometry-only blend, hashes every runtime mesh and object transform,
adds one neutral material, one camera, three broad area lights, and a quiet
backdrop, then renders the five views declared by the reviewed profile.

It does not create, edit, remesh, smooth, subdivide, displace, or apply a
modifier to any runtime mesh. The forged-iron material and every surface-detail
route remain blocked.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
from typing import Any, Sequence


PACKAGE_ROOT = Path(__file__).resolve().parent
PROFILE_PATH = (
    PACKAGE_ROOT
    / "profiles"
    / "openwork_strap_hinge_55_61_58_decomposition_v1.json"
)
DEFAULT_GEOMETRY_PATH = (
    PACKAGE_ROOT / "output" / "openwork_strap_hinge_geometry_v1.blend"
)
DEFAULT_PROOF_BLEND_PATH = (
    PACKAGE_ROOT / "output" / "openwork_strap_hinge_clay_proof_v1.blend"
)
DEFAULT_OUTPUT_DIR = PACKAGE_ROOT / "output"

ASSET_ID = "GH-018-Openwork-Strap-Hinge-v1"
RUNTIME_COLLECTION = "GH018_OPENWORK_RUNTIME"
PRESENTATION_COLLECTION = "GH018_CLAY_PROOF_PRESENTATION"
CLAY_MATERIAL = "M_GH018_NeutralClay"
BACKDROP_MATERIAL = "M_GH018_ProofBackdrop"
CAMERA_NAME = "CAM_GH018_ClayProof"
BACKDROP_NAME = "BG_GH018_ClayProof"
LIGHT_NAMES = (
    "LGT_GH018_ClayProof_Key",
    "LGT_GH018_ClayProof_Fill",
    "LGT_GH018_ClayProof_Rim",
)
EXPECTED_RUNTIME_OBJECTS = (
    "FixedKnuckle_02",
    "FixedKnuckle_04",
    "MovingKnuckle_01",
    "MovingKnuckle_03",
    "MovingKnuckle_05",
    "SM_GH018_FixedLeaf",
    "SM_GH018_MovingLeaf_Openwork",
    "SM_GH018_Pintle",
)
ALLOWED_MATERIAL_NODE_TYPES = {
    "ShaderNodeBsdfPrincipled",
    "ShaderNodeOutputMaterial",
}

# All values are authored presentation measurements in giant-house metres.
# They do not claim historical camera or lighting measurements.
VIEW_SPECS = (
    {
        "name": "reference_front",
        "purpose": (
            "Complete front silhouette, rail continuity, five-cell rhythm, "
            "and negative-space comparison."
        ),
        "camera_type": "ORTHO",
        "location_m": (1.13, 7.0, 0.0),
        "target_m": (1.13, 0.0, 0.0),
        "ortho_scale_m": 4.55,
        "roll_degrees": 0.0,
        "resolution_px": (1800, 520),
        "side": "front",
    },
    {
        "name": "oblique_depth",
        "purpose": (
            "Stock thickness, crown, bevel shoulder, aperture lip, and "
            "terminal depth."
        ),
        "camera_type": "PERSP",
        "location_m": (1.10, 7.2, 2.25),
        "target_m": (1.13, 0.0, 0.0),
        "lens_mm": 54.0,
        "roll_degrees": 0.0,
        "resolution_px": (1600, 760),
        "side": "front",
    },
    {
        "name": "cell_macro",
        "purpose": (
            "Middle cell openings, minimum webs, and the two continuous "
            "front crease loops."
        ),
        "camera_type": "ORTHO",
        "location_m": (1.54, 3.2, 0.0),
        "target_m": (1.54, 0.0, 0.0),
        "ortho_scale_m": 0.54,
        "roll_degrees": 0.0,
        "resolution_px": (1200, 820),
        "side": "front",
    },
    {
        "name": "pivot_macro",
        "purpose": (
            "Five alternating knuckles, open seams, bearing gap, and leaf "
            "root overlap."
        ),
        "camera_type": "PERSP",
        "location_m": (-0.03, 1.42, 0.46),
        "target_m": (-0.03, 0.0, 0.0),
        "lens_mm": 68.0,
        "roll_degrees": 0.0,
        "resolution_px": (1200, 900),
        "side": "front",
    },
    {
        "name": "rear_construction",
        "purpose": (
            "Planar rear stock, genuine through-holes, aperture walls, and "
            "absence of front-relief fakery."
        ),
        "camera_type": "ORTHO",
        "location_m": (1.13, -7.0, 0.0),
        "target_m": (1.13, 0.0, 0.0),
        "ortho_scale_m": 4.55,
        "roll_degrees": 0.0,
        "resolution_px": (1800, 520),
        "side": "rear",
    },
)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def load_profile() -> dict[str, Any]:
    return json.loads(PROFILE_PATH.read_text())


def blender_arguments(arguments: Sequence[str]) -> Sequence[str]:
    if "--" not in arguments:
        return ()
    return arguments[arguments.index("--") + 1 :]


def parse_args(arguments: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--render",
        action="store_true",
        help="Render only when the reviewed neutral-clay proof gate is open.",
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_GEOMETRY_PATH,
        help="Validated geometry-only blend to present.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_PROOF_BLEND_PATH,
        help="Presentation blend destination.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help="Directory for five proof PNGs and the proof manifest.",
    )
    return parser.parse_args(arguments)


def require_authorization(
    profile: dict[str, Any],
    *,
    render_requested: bool,
) -> dict[str, Any]:
    if not render_requested:
        raise PermissionError("An explicit --render argument is required.")
    implementation = profile["implementation_contract"]
    if not implementation["proof_render_authorized"]:
        raise PermissionError("The reviewed profile still blocks proof rendering.")
    contract = implementation["proof_render_contract"]
    if contract["geometry_mutation_authorized"]:
        raise ValueError("The clay proof contract must prohibit geometry mutation.")
    if contract["forged_iron_material_authorized"]:
        raise ValueError("The clay proof contract must block the iron material.")
    if contract["surface_detail_authorized"]:
        raise ValueError("The clay proof contract must block surface detail.")
    expected_views = [view["name"] for view in VIEW_SPECS]
    if contract["views"] != expected_views:
        raise ValueError("The proof script and reviewed view inventory diverged.")
    if contract["material"] != "neutral_clay_only":
        raise ValueError("The proof contract no longer specifies neutral clay.")
    return contract


def runtime_objects(bpy_module: Any) -> list[Any]:
    collection = bpy_module.data.collections.get(RUNTIME_COLLECTION)
    if collection is None:
        raise ValueError("The geometry-only runtime collection is missing.")
    objects = sorted(
        (obj for obj in collection.objects if obj.type == "MESH"),
        key=lambda obj: obj.name,
    )
    names = tuple(obj.name for obj in objects)
    if names != EXPECTED_RUNTIME_OBJECTS:
        raise ValueError(f"Runtime object inventory changed: {names!r}")
    for obj in objects:
        if obj.get("sinc_asset_id") != ASSET_ID:
            raise ValueError(f"{obj.name}: asset identity changed.")
        if obj.modifiers:
            raise ValueError(f"{obj.name}: a live modifier entered the proof.")
    return objects


def runtime_geometry_hash(objects: Sequence[Any]) -> str:
    digest = hashlib.sha256()
    for obj in sorted(objects, key=lambda item: item.name):
        digest.update(obj.name.encode("utf-8"))
        for row in obj.matrix_world:
            for value in row:
                digest.update(f"{float(value):.12f},".encode("ascii"))
        mesh = obj.data
        digest.update(f"v={len(mesh.vertices)};p={len(mesh.polygons)};".encode())
        for vertex in mesh.vertices:
            digest.update(
                (
                    f"{vertex.co.x:.12f},{vertex.co.y:.12f},"
                    f"{vertex.co.z:.12f};"
                ).encode("ascii")
            )
        for polygon in mesh.polygons:
            digest.update(
                (
                    ",".join(str(index) for index in polygon.vertices) + ";"
                ).encode("ascii")
            )
    return digest.hexdigest()


def clear_existing_presentation(bpy_module: Any) -> None:
    collection = bpy_module.data.collections.get(PRESENTATION_COLLECTION)
    if collection is not None:
        for obj in tuple(collection.objects):
            bpy_module.data.objects.remove(obj, do_unlink=True)
        bpy_module.data.collections.remove(collection)
    for obj in tuple(bpy_module.data.objects):
        if obj.type in {"CAMERA", "LIGHT"}:
            bpy_module.data.objects.remove(obj, do_unlink=True)


def new_collection(bpy_module: Any, name: str) -> Any:
    collection = bpy_module.data.collections.new(name)
    bpy_module.context.scene.collection.children.link(collection)
    return collection


def make_plain_material(
    bpy_module: Any,
    name: str,
    *,
    base_color: tuple[float, float, float, float],
    roughness: float,
) -> Any:
    material = bpy_module.data.materials.get(name)
    if material is None:
        material = bpy_module.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    principled = nodes.new("ShaderNodeBsdfPrincipled")
    principled.inputs["Base Color"].default_value = base_color
    principled.inputs["Roughness"].default_value = roughness
    principled.inputs["Metallic"].default_value = 0.0
    material.node_tree.links.new(
        principled.outputs["BSDF"],
        output.inputs["Surface"],
    )
    if {node.bl_idname for node in nodes} != ALLOWED_MATERIAL_NODE_TYPES:
        raise ValueError(f"{name}: non-clay material nodes entered the proof.")
    return material


def assign_clay(objects: Sequence[Any], material: Any) -> None:
    for obj in objects:
        obj.data.materials.clear()
        obj.data.materials.append(material)


def create_backdrop(
    bpy_module: Any,
    collection: Any,
    material: Any,
) -> Any:
    mesh = bpy_module.data.meshes.new(BACKDROP_NAME + "_Mesh")
    centre_x = 1.13
    half_x = 3.25
    half_z = 2.60
    mesh.from_pydata(
        [
            (centre_x - half_x, 0.0, -half_z),
            (centre_x - half_x, 0.0, half_z),
            (centre_x + half_x, 0.0, half_z),
            (centre_x + half_x, 0.0, -half_z),
        ],
        [],
        [(0, 1, 2, 3)],
    )
    mesh.materials.append(material)
    obj = bpy_module.data.objects.new(BACKDROP_NAME, mesh)
    obj.location.y = -0.20
    obj["sinc_proof_role"] = "quiet_shadow_backdrop"
    collection.objects.link(obj)
    return obj


def point_object_at(obj: Any, target: tuple[float, float, float]) -> None:
    from mathutils import Vector

    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def create_camera(bpy_module: Any, collection: Any) -> Any:
    data = bpy_module.data.cameras.new(CAMERA_NAME + "_Data")
    data.lens = 54.0
    data.clip_start = 0.01
    data.clip_end = 100.0
    camera = bpy_module.data.objects.new(CAMERA_NAME, data)
    camera["sinc_proof_role"] = "diagnostic_camera"
    collection.objects.link(camera)
    bpy_module.context.scene.camera = camera
    return camera


def create_area_light(
    bpy_module: Any,
    collection: Any,
    *,
    name: str,
    energy_w: float,
    size_m: float,
) -> Any:
    data = bpy_module.data.lights.new(name + "_Data", type="AREA")
    data.energy = energy_w
    data.shape = "DISK"
    data.size = size_m
    data.color = (1.0, 1.0, 1.0)
    light = bpy_module.data.objects.new(name, data)
    light["sinc_proof_role"] = "broad_neutral_light"
    collection.objects.link(light)
    return light


def create_lights(bpy_module: Any, collection: Any) -> list[Any]:
    settings = (
        (LIGHT_NAMES[0], 220.0, 3.4),
        (LIGHT_NAMES[1], 60.0, 4.0),
        (LIGHT_NAMES[2], 130.0, 2.6),
    )
    return [
        create_area_light(
            bpy_module,
            collection,
            name=name,
            energy_w=energy,
            size_m=size,
        )
        for name, energy, size in settings
    ]


def configure_world(bpy_module: Any) -> None:
    scene = bpy_module.context.scene
    world = scene.world
    if world is None:
        world = bpy_module.data.worlds.new("W_GH018_ClayProof")
        scene.world = world
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.028, 0.032, 0.040, 1.0)
    background.inputs["Strength"].default_value = 0.08


def configure_render(bpy_module: Any) -> None:
    scene = bpy_module.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.image_settings.compression = 15
    scene.render.film_transparent = False
    scene.render.use_file_extension = True
    scene.view_settings.view_transform = "AgX"
    try:
        scene.view_settings.look = "AgX - Medium High Contrast"
    except TypeError:
        pass


def set_camera_for_view(camera: Any, view: dict[str, Any]) -> None:
    from mathutils import Quaternion, Vector

    camera.location = view["location_m"]
    direction = Vector(view["target_m"]) - camera.location
    orientation = direction.to_track_quat("-Z", "Y")
    roll = Quaternion((0.0, 0.0, 1.0), math.radians(view["roll_degrees"]))
    camera.rotation_mode = "QUATERNION"
    camera.rotation_quaternion = orientation @ roll
    camera.data.type = view["camera_type"]
    if view["camera_type"] == "ORTHO":
        camera.data.ortho_scale = view["ortho_scale_m"]
    else:
        camera.data.lens = view["lens_mm"]


def set_lights_for_side(
    lights: Sequence[Any],
    *,
    side: str,
    target: tuple[float, float, float],
) -> None:
    direction = 1.0 if side == "front" else -1.0
    positions = (
        (-0.65, 4.2 * direction, 2.85),
        (3.15, 3.0 * direction, -1.75),
        (1.20, 1.8 * direction, 2.10),
    )
    for light, position in zip(lights, positions):
        light.location = position
        point_object_at(light, target)


def view_manifest_record(camera: Any, view: dict[str, Any]) -> dict[str, Any]:
    return {
        "purpose": view["purpose"],
        "camera_type": camera.data.type,
        "location_m": [round(float(value), 6) for value in camera.location],
        "target_m": list(view["target_m"]),
        "rotation_quaternion": [
            round(float(value), 9) for value in camera.rotation_quaternion
        ],
        "lens_mm": (
            round(float(camera.data.lens), 6)
            if camera.data.type == "PERSP"
            else None
        ),
        "ortho_scale_m": (
            round(float(camera.data.ortho_scale), 6)
            if camera.data.type == "ORTHO"
            else None
        ),
        "resolution_px": list(view["resolution_px"]),
        "side": view["side"],
    }


def render_views(
    bpy_module: Any,
    *,
    camera: Any,
    lights: Sequence[Any],
    backdrop: Any,
    output_dir: Path,
) -> dict[str, dict[str, Any]]:
    scene = bpy_module.context.scene
    records: dict[str, dict[str, Any]] = {}
    output_dir.mkdir(parents=True, exist_ok=True)
    for view in VIEW_SPECS:
        set_camera_for_view(camera, view)
        set_lights_for_side(
            lights,
            side=view["side"],
            target=view["target_m"],
        )
        backdrop.location.y = -0.20 if view["side"] == "front" else 0.20
        width, height = view["resolution_px"]
        scene.render.resolution_x = width
        scene.render.resolution_y = height
        output_path = (
            output_dir
            / f"openwork_strap_hinge_clay_{view['name']}.png"
        )
        scene.render.filepath = str(output_path)
        bpy_module.ops.render.render(write_still=True)
        if not output_path.is_file() or output_path.stat().st_size < 40_000:
            raise ValueError(f"{view['name']}: proof PNG is missing or trivial.")
        record = view_manifest_record(camera, view)
        record["path"] = str(output_path)
        record["sha256"] = sha256_file(output_path)
        record["bytes"] = output_path.stat().st_size
        records[view["name"]] = record
    return records


def validate_saved_proof(
    bpy_module: Any,
    *,
    geometry_hash_before: str,
    expected_views: Sequence[str],
) -> dict[str, Any]:
    objects = runtime_objects(bpy_module)
    geometry_hash_after = runtime_geometry_hash(objects)
    material_names = sorted(
        {
            slot.material.name
            for obj in objects
            for slot in obj.material_slots
            if slot.material is not None
        }
    )
    cameras = [obj for obj in bpy_module.data.objects if obj.type == "CAMERA"]
    lights = [obj for obj in bpy_module.data.objects if obj.type == "LIGHT"]
    scene = bpy_module.context.scene
    stored_views = json.loads(scene["sinc_proof_views_json"])
    return {
        "runtime_mesh_count": len(objects),
        "runtime_objects": [obj.name for obj in objects],
        "runtime_geometry_hash_before": geometry_hash_before,
        "runtime_geometry_hash_after": geometry_hash_after,
        "geometry_mutated": geometry_hash_before != geometry_hash_after,
        "material_names": material_names,
        "camera_count": len(cameras),
        "camera_names": sorted(obj.name for obj in cameras),
        "light_count": len(lights),
        "light_names": sorted(obj.name for obj in lights),
        "proof_views": stored_views,
        "surface_detail_authored": False,
        "forged_iron_material_authored": False,
        "render_engine": scene.render.engine,
        "blender_version": bpy_module.app.version_string,
        "expected_views_match": stored_views == list(expected_views),
    }


def build_and_render_proof(
    bpy_module: Any,
    profile: dict[str, Any],
    contract: dict[str, Any],
    *,
    input_path: Path,
    proof_blend_path: Path,
    output_dir: Path,
) -> dict[str, Any]:
    if not input_path.is_file():
        raise FileNotFoundError(input_path)
    input_geometry_sha256 = sha256_file(input_path)
    bpy_module.ops.wm.open_mainfile(filepath=str(input_path))
    scene = bpy_module.context.scene
    if scene.get("sinc_asset_id") != ASSET_ID:
        raise ValueError("The input blend is not the reviewed hinge asset.")
    if scene.get("sinc_profile_sha256") != sha256_file(PROFILE_PATH):
        raise ValueError(
            "The geometry blend predates the current reviewed profile; "
            "rebuild geometry before rendering."
        )

    objects = runtime_objects(bpy_module)
    geometry_hash_before = runtime_geometry_hash(objects)
    clear_existing_presentation(bpy_module)
    presentation = new_collection(bpy_module, PRESENTATION_COLLECTION)
    clay = make_plain_material(
        bpy_module,
        CLAY_MATERIAL,
        base_color=(0.070, 0.085, 0.110, 1.0),
        roughness=0.70,
    )
    backdrop_material = make_plain_material(
        bpy_module,
        BACKDROP_MATERIAL,
        base_color=(0.30, 0.33, 0.38, 1.0),
        roughness=0.88,
    )
    assign_clay(objects, clay)
    backdrop = create_backdrop(
        bpy_module,
        presentation,
        backdrop_material,
    )
    camera = create_camera(bpy_module, presentation)
    lights = create_lights(bpy_module, presentation)
    configure_world(bpy_module)
    configure_render(bpy_module)

    outputs = render_views(
        bpy_module,
        camera=camera,
        lights=lights,
        backdrop=backdrop,
        output_dir=output_dir,
    )
    geometry_hash_after_render = runtime_geometry_hash(objects)
    if geometry_hash_before != geometry_hash_after_render:
        raise ValueError("Runtime geometry changed during proof rendering.")

    scene["sinc_render_gate"] = "NEUTRAL_CLAY_PROOF_AUTHORIZED"
    scene["sinc_proof_profile_sha256"] = sha256_file(PROFILE_PATH)
    scene["sinc_proof_source_blend_sha256"] = input_geometry_sha256
    scene["sinc_proof_views_json"] = json.dumps(contract["views"])
    scene["sinc_surface_detail_authored"] = False
    scene["sinc_forged_iron_material_authored"] = False
    proof_blend_path.parent.mkdir(parents=True, exist_ok=True)
    bpy_module.ops.wm.save_as_mainfile(filepath=str(proof_blend_path))
    bpy_module.ops.wm.open_mainfile(filepath=str(proof_blend_path))

    saved_validation = validate_saved_proof(
        bpy_module,
        geometry_hash_before=geometry_hash_before,
        expected_views=contract["views"],
    )
    if saved_validation["geometry_mutated"]:
        raise ValueError("Saved proof blend changed runtime geometry.")
    if not saved_validation["expected_views_match"]:
        raise ValueError("Saved proof blend lost the reviewed view inventory.")
    if saved_validation["material_names"] != [CLAY_MATERIAL]:
        raise ValueError("Saved proof blend is not neutral-clay-only.")
    if saved_validation["camera_count"] != 1:
        raise ValueError("Saved proof blend must contain exactly one camera.")
    if saved_validation["light_count"] != 3:
        raise ValueError("Saved proof blend must contain exactly three lights.")

    manifest_path = (
        output_dir / "openwork_strap_hinge_clay_proof_v1_manifest.json"
    )
    manifest = {
        "schema": "iggy-openwork-strap-hinge-clay-proof-manifest/1.0",
        "asset_id": ASSET_ID,
        "reference_object": "Met 55.61.58",
        "authorization": contract["authorization"],
        "source_hashes": {
            "profile_sha256": sha256_file(PROFILE_PATH),
            "geometry_sha256": input_geometry_sha256,
            "proof_builder_sha256": sha256_file(Path(__file__)),
            "proof_blend_sha256": sha256_file(proof_blend_path),
        },
        "outputs": outputs,
        "saved_proof_validation": saved_validation,
        "scope": {
            "neutral_clay_only": True,
            "geometry_mutation_authorized": False,
            "surface_detail_authorized": False,
            "forged_iron_material_authorized": False,
            "ai_generated_imagery": False,
        },
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )
    return {
        "proof_blend": str(proof_blend_path),
        "manifest": str(manifest_path),
        "outputs": outputs,
        "saved_proof_validation": saved_validation,
    }


def main(arguments: Sequence[str] | None = None) -> int:
    arguments = list(sys.argv[1:] if arguments is None else arguments)
    if "bpy" not in sys.modules:
        raise RuntimeError("The clay proof requires Blender's Python runtime.")
    parsed = parse_args(blender_arguments([sys.argv[0], *arguments]))
    profile = load_profile()
    contract = require_authorization(
        profile,
        render_requested=parsed.render,
    )
    import bpy

    result = build_and_render_proof(
        bpy,
        profile,
        contract,
        input_path=parsed.input.resolve(),
        proof_blend_path=parsed.output.resolve(),
        output_dir=parsed.output_dir.resolve(),
    )
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
