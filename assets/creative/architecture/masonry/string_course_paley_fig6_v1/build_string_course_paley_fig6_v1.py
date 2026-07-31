"""String course swept from the accepted Paley fig 6 profile donor.

THE FIRST PRODUCTION CONSUMER of SINC_GeometryProof_Profile (Phase 6 of the
side-quest roadmap): this script consumes the compiled donor geometry and
recreates NOTHING - no station, no arc, no outline point is authored here.
The moulded edge, the closed section, and the extrusion all come from
profile_compiler.compile_profile() on the reference-verified spec, and the
manifest records the donor's sha256 so the provenance chain is checkable.

Run:
    Blender --background --python build_string_course_paley_fig6_v1.py -- \
        [--length-m 2.0] [--out OUTDIR] [--skip-render]
"""

import hashlib
import json
import math
import sys
from pathlib import Path

sys.dont_write_bytecode = True

import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[4]  # <repo>/assets/creative/architecture/masonry/<pkg>
GEOMETRY_PROOF = REPO / "assets" / "creative" / "geometry_proof"
sys.path.insert(0, str(GEOMETRY_PROOF))

from profile_compiler import compile_profile  # noqa: E402

DONOR = GEOMETRY_PROOF / "profiles" / "paley_pl1_fig6_v1.json"
ASSET_NAME = "SINC_StringCourse_PaleyFig6"


def build(length_m, out_dir, skip_render):
    spec = json.loads(DONOR.read_text())
    compiled = compile_profile(spec)  # the single source - validated on every build
    s = spec["construction_plane"]["scale_m_per_unit"]

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0
    scene = bpy.context.scene

    # section curve in local XY (profile units * scale = metres), swept along
    # local Z = world Y after the same +90 deg X rotation the proof scene uses
    cu = bpy.data.curves.new(f"{ASSET_NAME}_Section", "CURVE")
    cu.dimensions = "2D"
    cu.fill_mode = "BOTH"
    cu.extrude = length_m / 2.0
    spline = cu.splines.new("POLY")
    spline.points.add(len(compiled.outline) - 1)
    for pt, (x, y) in zip(spline.points, compiled.outline):
        pt.co = (x * s, y * s, 0.0, 1.0)
    spline.use_cyclic_u = True
    curve_obj = bpy.data.objects.new(f"{ASSET_NAME}_SectionMaster", cu)
    curve_obj.rotation_euler = (math.radians(90), 0, 0)
    scene.collection.objects.link(curve_obj)

    # bake the swept solid to a mesh named as the production asset
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(curve_obj.evaluated_get(deps))
    # the conversion emits separate cap rings on top of the wall rings -
    # weld them so the production mesh is exactly two outline rings
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
    bm.to_mesh(mesh)
    bm.free()
    asset = bpy.data.objects.new(ASSET_NAME, mesh)
    asset.rotation_euler = curve_obj.rotation_euler[:]
    scene.collection.objects.link(asset)
    # the curve master stays in the file (hidden) so the section remains
    # inspectable; the donor spec is the editable input, not this file
    curve_obj.hide_render = True
    curve_obj.hide_viewport = True

    spec_sha = hashlib.sha256(
        json.dumps(spec, sort_keys=True).encode()).hexdigest()
    asset["sinc_donor_profile"] = spec["id"]
    asset["sinc_donor_sha256"] = spec_sha
    asset["sinc_generated_from"] = "profile_compiler.compile_profile"

    mat = bpy.data.materials.new("SINC_Mat_StringCourseStone")
    mat.diffuse_color = (0.62, 0.60, 0.55, 1.0)
    mesh.materials.append(mat)

    renders = {}
    if not skip_render:
        scene.render.engine = "BLENDER_EEVEE"
        scene.render.resolution_x = 1600
        scene.render.resolution_y = 1200
        world = bpy.data.worlds.new("W")
        world.use_nodes = True
        bg = world.node_tree.nodes.get("Background")
        if bg:
            bg.inputs["Color"].default_value = (1, 1, 1, 1)
        scene.world = world
        sun = bpy.data.lights.new("Sun", "SUN")
        sun.energy = 6.0
        sun_obj = bpy.data.objects.new("Sun", sun)
        sun_obj.rotation_euler = (math.radians(55), math.radians(15), math.radians(140))
        scene.collection.objects.link(sun_obj)

        w = compiled.extents["width"] * s
        d = compiled.extents["depth"] * s
        centre = Vector((w / 2.0, 0.0, d / 2.0))
        cam = bpy.data.cameras.new("Cam")
        cam_obj = bpy.data.objects.new("SINC_Cam_ThreeQuarter", cam)
        dist = 2.0 * max(w, d, length_m)
        # view from the soffit side, where the moulded run (chamfer, bowtell,
        # hollow) actually lives
        cam_obj.location = centre + Vector((0.85, -1.0, -0.25)).normalized() * dist
        cam.clip_start = dist * 0.001
        cam.clip_end = dist * 20
        direction = centre - cam_obj.location
        cam_obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(cam_obj)
        scene.camera = cam_obj
        render_dir = out_dir / "renders"
        render_dir.mkdir(parents=True, exist_ok=True)
        path = render_dir / "three_quarter.png"
        scene.render.filepath = str(path)
        bpy.ops.render.render(write_still=True)
        renders["three_quarter"] = str(path)

    blend_path = out_dir / "string_course_paley_fig6_v1.blend"
    out_dir.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path), compress=True)

    manifest = {
        "status": "SINC_STRING_COURSE_BUILT",
        "asset": ASSET_NAME,
        "consumed_donor": {
            "profile_id": spec["id"],
            "spec_path": str(DONOR.relative_to(REPO)),
            "spec_sha256": spec_sha,
            "badge": "reference-verified pilot (see geometry_proof README)",
        },
        "single_source": "profile_compiler.compile_profile - no geometry authored here",
        "length_m": length_m,
        "section_vertices": len(compiled.outline),
        "section_extents_units": compiled.extents,
        "scale_m_per_unit": s,
        "blender_version": bpy.app.version_string,
        "renders": renders,
    }
    (out_dir / "string_course_paley_fig6_v1_manifest.json").write_text(
        json.dumps(manifest, indent=1, sort_keys=True))
    print(f"SINC_STRING_COURSE_OK blend={blend_path}")


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    length_m = float(argv[argv.index("--length-m") + 1]) if "--length-m" in argv else 2.0
    out_dir = Path(argv[argv.index("--out") + 1]) if "--out" in argv else HERE / "output"
    build(length_m, out_dir, "--skip-render" in argv)


if __name__ == "__main__":
    main()
