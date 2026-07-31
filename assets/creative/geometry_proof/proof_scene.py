"""SINC_GeometryProof_Profile scene builder - headless bpy, Blender 5.1.

Run:
    Blender --background --python proof_scene.py -- \
        --spec profiles/paley_pl1_fig6_v1.json --out output/paley_pl1_fig6_v1 \
        [--flip SEGMENT_NAME] [--skip-render]

Everything derives from profile_compiler.compile_profile - the single source.
The .blend embeds the spec JSON, this module, the compiler, the schema, and a
rebuild driver as text datablocks, so the saved file reopens self-contained
and editable with zero add-ons: edit the JSON text, run rebuild_proof.py in
the text editor, and the whole proof regenerates.

Renders (EEVEE, emission materials, flat diagram look):
    front.png          ortho along +Y, reference overlay behind the profile
    opposite.png       ortho along -Y (reference hidden - it would occlude)
    three_quarter.png  perspective, shows the proof extrusion
    wire.png           three-quarter framing, wireframe proxy + annotations
"""

import hashlib
import json
import math
import sys
from pathlib import Path

sys.dont_write_bytecode = True  # keep __pycache__ out of the asset package

import bpy
from mathutils import Vector

PACKAGE_DIR = Path(__file__).resolve().parent if "__file__" in globals() else None
if PACKAGE_DIR is not None:
    sys.path.insert(0, str(PACKAGE_DIR))

from profile_compiler import (  # noqa: E402
    compile_profile, point_in_polygon, polygon_centroid, to_world)
from profile_spec import EVIDENCE_COLOURS  # noqa: E402

BLACK = (0.0, 0.0, 0.0, 1.0)
STONE = (0.62, 0.60, 0.55, 1.0)
GUIDE = (0.35, 0.40, 0.55, 1.0)
ARROW = (0.95, 0.80, 0.05, 1.0)
RES_X, RES_Y = 1600, 1200

REBUILD_DRIVER = '''"""Rebuild the proof from the embedded spec. Edit the
profile_spec.json text datablock, then run this script (Alt+P here)."""
import bpy, sys, types, json

def _load(name):
    mod = types.ModuleType(name)
    mod.__dict__["__file__"] = name + ".py"
    exec(bpy.data.texts[name + ".py"].as_string(), mod.__dict__)
    sys.modules[name] = mod
    return mod

_load("profile_spec")
_load("profile_compiler")
scene_mod = _load("proof_scene")
spec = json.loads(bpy.data.texts["profile_spec.json"].as_string())
scene_mod.rebuild_in_current_file(spec)
print("proof rebuilt from embedded spec")
'''


# ---------------------------------------------------------------- primitives

def _mesh_object(name, verts, faces, collection, material, location=(0, 0, 0),
                 rotation=(0, 0, 0)):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    if material is not None:
        mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    obj.location = location
    obj.rotation_euler = rotation
    collection.objects.link(obj)
    return obj


def _octahedron(r):
    verts = [(r, 0, 0), (-r, 0, 0), (0, r, 0), (0, -r, 0), (0, 0, r), (0, 0, -r)]
    faces = [(0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4),
             (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)]
    return verts, faces


def _disc(r, thickness, n=16):
    half = thickness / 2.0
    verts = []
    for y in (-half, half):
        for i in range(n):
            a = 2 * math.pi * i / n
            verts.append((r * math.cos(a), y, r * math.sin(a)))
    faces = [(i, (i + 1) % n, n + (i + 1) % n, n + i) for i in range(n)]
    faces += [tuple(range(n - 1, -1, -1)), tuple(range(n, 2 * n))]
    return verts, faces


def _cone_x(length, r, n=12):
    """Cone pointing along +X, base at x=0."""
    verts = [(length, 0, 0)] + [(0, r * math.cos(2 * math.pi * i / n),
                                 r * math.sin(2 * math.pi * i / n)) for i in range(n)]
    faces = [(0, 1 + i, 1 + (i + 1) % n) for i in range(n)]
    faces.append(tuple(range(n, 0, -1)))
    return verts, faces


def _bar(a, b, r, n=8):
    """Prism from 3D point a to b, square-ish cross-section radius r."""
    a, b = Vector(a), Vector(b)
    axis = b - a
    length = axis.length
    if length == 0:
        return [], []
    axis.normalize()
    ref = Vector((0, 1, 0)) if abs(axis.dot(Vector((0, 1, 0)))) < 0.9 else Vector((0, 0, 1))
    u = axis.cross(ref).normalized()
    v = axis.cross(u).normalized()
    verts = []
    for base in (a, b):
        for i in range(n):
            ang = 2 * math.pi * i / n
            p = base + u * (r * math.cos(ang)) + v * (r * math.sin(ang))
            verts.append(tuple(p))
    faces = [(i, (i + 1) % n, n + (i + 1) % n, n + i) for i in range(n)]
    faces += [tuple(range(n - 1, -1, -1)), tuple(range(n, 2 * n))]
    return verts, faces


# ---------------------------------------------------------------- materials

def _emission_material(name, rgba):
    mat = bpy.data.materials.get(name)
    if mat is not None:
        return mat
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = rgba
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    em = nodes.new("ShaderNodeEmission")
    em.inputs["Color"].default_value = rgba
    out = nodes.new("ShaderNodeOutputMaterial")
    mat.node_tree.links.new(em.outputs["Emission"], out.inputs["Surface"])
    return mat


def _image_material(name, image):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    nodes.clear()
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = image
    em = nodes.new("ShaderNodeEmission")
    out = nodes.new("ShaderNodeOutputMaterial")
    mat.node_tree.links.new(tex.outputs["Color"], em.inputs["Color"])
    mat.node_tree.links.new(em.outputs["Emission"], out.inputs["Surface"])
    return mat


def _text_object(name, body, location, size, collection, material,
                 align="CENTER"):
    cu = bpy.data.curves.new(name, type="FONT")
    cu.body = body
    cu.size = size
    cu.align_x = align
    cu.align_y = "CENTER"
    if material is not None:
        cu.materials.append(material)
    obj = bpy.data.objects.new(name, cu)
    obj.location = location
    obj.rotation_euler = (math.radians(90), 0, 0)  # face the front camera
    collection.objects.link(obj)
    return obj


# ---------------------------------------------------------------- the scene

def build_scene_content(spec, repo_root=None):
    """Create all SINC_ collections/objects for `spec`. Returns build info."""
    compiled = compile_profile(spec)
    s = spec["construction_plane"]["scale_m_per_unit"]
    datum = compiled.datum_value
    # annotation sizing follows the drawing extent, not the datum semantics -
    # a profile with a small datum (stepped: order_width) must not get
    # unreadably small markers
    ds = max(compiled.extents["width"], compiled.extents["depth"]) * s
    extrude_m = spec["proof"]["extrusion_depth"] * s / 2.0  # curve extrude is +/-

    scene = bpy.context.scene
    cols = {}
    for key in ("Reference", "Master", "Annotations", "Cameras", "WireProxy"):
        col = bpy.data.collections.new(f"SINC_{key}")
        scene.collection.children.link(col)
        cols[key] = col

    # --- master: ONE curve object = editable master + solid fill + extrusion
    cu = bpy.data.curves.new(f"SINC_Master_{spec['id']}", "CURVE")
    cu.dimensions = "2D"
    cu.fill_mode = "BOTH"
    cu.extrude = extrude_m
    spline = cu.splines.new("POLY")
    spline.points.add(len(compiled.outline) - 1)
    for pt, (x, y) in zip(spline.points, compiled.outline):
        pt.co = (x * s, y * s, 0.0, 1.0)
    spline.use_cyclic_u = True
    cu.materials.append(_emission_material("SINC_Mat_Stone", STONE))
    master = bpy.data.objects.new(f"SINC_Master_{spec['id']}", cu)
    master.rotation_euler = (math.radians(90), 0, 0)  # local XY -> world XZ
    master["sinc_profile_id"] = spec["id"]
    master["sinc_generated_from"] = "profile_compiler.compile_profile"
    cols["Master"].objects.link(master)

    # --- reference image plane
    ref = spec["reference"]
    ref_placed = False
    if ref.get("image") and ref.get("crop_px_at_origin") and ref.get("crop_px_per_unit"):
        image = None
        img_name = Path(ref["image"]).name
        if repo_root is not None and (Path(repo_root) / ref["image"]).exists():
            image = bpy.data.images.load(str(Path(repo_root) / ref["image"]),
                                         check_existing=True)
        elif img_name in bpy.data.images:  # rebuild inside a saved .blend: packed
            image = bpy.data.images[img_name]
        if image is not None:
            w_px, h_px = image.size
            ox, oy = ref["crop_px_at_origin"]
            c = ref["crop_px_per_unit"]
            bl = ((-ox / c) * s, ((oy - h_px) / c) * s)
            tr = (((w_px - ox) / c) * s, (oy / c) * s)
            y_ref = extrude_m + 0.02 * ds  # just behind the solid (front view)
            verts = [(bl[0], y_ref, bl[1]), (tr[0], y_ref, bl[1]),
                     (tr[0], y_ref, tr[1]), (bl[0], y_ref, tr[1])]
            mesh = bpy.data.meshes.new("SINC_ReferencePlane")
            mesh.from_pydata(verts, [], [(0, 1, 2, 3)])
            uv = mesh.uv_layers.new()
            for loop_idx, uv_co in zip(range(4), [(0, 0), (1, 0), (1, 1), (0, 1)]):
                uv.data[loop_idx].uv = uv_co
            mesh.materials.append(_image_material("SINC_Mat_Reference", image))
            plane = bpy.data.objects.new("SINC_ReferencePlane", mesh)
            cols["Reference"].objects.link(plane)
            image.pack()
            ref_placed = True

    # --- annotations
    acol = cols["Annotations"]
    guide_mat = _emission_material("SINC_Mat_Guide", GUIDE)
    arrow_mat = _emission_material("SINC_Mat_Arrow", ARROW)
    black_mat = _emission_material("SINC_Mat_Black", BLACK)
    ev_mats = {ev: _emission_material(f"SINC_Mat_Ev_{ev}", rgba)
               for ev, rgba in EVIDENCE_COLOURS.items()}

    z_front = -(extrude_m + 0.015 * ds)  # annotations float in front of the solid
    label_pool = []  # (text_object, estimated_width) for collision resolution

    for idx, (name, st) in enumerate(compiled.stations.items()):
        wx, _, wz = to_world(st["at"], spec)
        loc = (wx, z_front, wz)
        mat = ev_mats[st["evidence"]]
        if st["join"] == "HARD":
            verts, faces = _octahedron(0.014 * ds)
        else:
            verts, faces = _disc(0.012 * ds, 0.006 * ds)
        _mesh_object(f"SINC_Station_{name}", verts, faces, acol, mat, location=loc)
        # stagger labels; a collision-resolution pass below handles the rest
        dz = 0.02 * ds if idx % 2 == 0 else -0.05 * ds
        label = _text_object(f"SINC_Label_{name}", name,
                             (wx + 0.02 * ds, z_front, wz + dz),
                             0.022 * ds, acol, black_mat, align="LEFT")
        label_pool.append((label, 0.014 * ds * len(name)))
        if st["join"] == "SMOOTH":
            tx, ty = st["tangent_out"]
            theta = math.atan2(ty, tx)
            verts, faces = _cone_x(0.05 * ds, 0.008 * ds)
            _mesh_object(f"SINC_Tangent_{name}", verts, faces, acol, arrow_mat,
                         location=loc, rotation=(0, -theta, 0))

    for name, mapped in compiled.source_px_mapped.items():
        wx, _, wz = to_world(mapped, spec)
        r = 0.008 * ds
        verts = [(-r, -r, -r), (r, -r, -r), (r, r, -r), (-r, r, -r),
                 (-r, -r, r), (r, -r, r), (r, r, r), (-r, r, r)]
        faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
                 (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
        _mesh_object(f"SINC_SourcePx_{name}", verts, faces, acol, black_mat,
                     location=(wx, z_front, wz))

    for seg in compiled.segments:
        if seg["kind"] != "ARC":
            continue
        cx, _, cz = to_world(seg["centre"], spec)
        arm = 0.03 * ds
        t = 0.003 * ds
        for i, (dx, dz) in enumerate(((arm, 0), (0, arm))):
            verts = [(-dx - t, -t, -dz - t), (dx + t, -t, -dz - t),
                     (dx + t, -t, dz + t), (-dx - t, -t, dz + t),
                     (-dx - t, t, -dz - t), (dx + t, t, -dz - t),
                     (dx + t, t, dz + t), (-dx - t, t, dz + t)]
            faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1),
                     (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
            _mesh_object(f"SINC_ArcCentre_{seg['name']}_{i}", verts, faces, acol,
                         ev_mats[seg["evidence"]], location=(cx, z_front, cz))
        for end_name in (seg["from"], seg["to"]):
            ex, _, ez = to_world(compiled.stations[end_name]["at"], spec)
            verts, faces = _bar((cx, z_front, cz), (ex, z_front, ez), 0.0025 * ds)
            if verts:
                _mesh_object(f"SINC_Radius_{seg['name']}_{end_name}", verts, faces,
                             acol, guide_mat)
        mx, _, mz = to_world(seg["mid_point"], spec)
        tx, ty = seg["mid_tangent"]
        theta = math.atan2(ty, tx)
        verts, faces = _cone_x(0.055 * ds, 0.010 * ds)
        _mesh_object(f"SINC_ArcDir_{seg['name']}", verts, faces, acol, arrow_mat,
                     location=(mx, z_front, mz), rotation=(0, -theta, 0))
        short = "CCW" if seg["direction"] == "COUNTERCLOCKWISE" else "CW"
        info_body = f"{short} {seg['sweep_deg']:.0f}deg R{seg['radius']:.2f}"
        info = _text_object(f"SINC_ArcInfo_{seg['name']}", info_body,
                            (cx + 0.02 * ds, z_front, cz - 0.045 * ds),
                            0.022 * ds, acol, black_mat, align="LEFT")
        label_pool.append((info, 0.014 * ds * len(info_body)))

    # solid / void labels
    interior = polygon_centroid(compiled.outline)
    if not point_in_polygon(interior, compiled.outline):
        interior = None
    if interior is not None:
        ix, _, iz = to_world(interior, spec)
        _text_object("SINC_LabelSolid", "SOLID", (ix, z_front, iz),
                     0.05 * ds, acol, black_mat)
    moulded = [g for g in compiled.segments if g["role"] == "MOULDED"]
    if moulded:
        # longest by real length, not sample count - a straight LINE has only
        # two points however long it is (the stepped profile exposed this)
        def seg_length(g):
            return sum(math.dist(a, b) for a, b in zip(g["points"], g["points"][1:]))
        longest = max(moulded, key=seg_length)
        pts = longest["points"]
        if len(pts) >= 3:
            mid = pts[len(pts) // 2]
            nxt = pts[len(pts) // 2 + 1]
        else:
            mid = ((pts[0][0] + pts[1][0]) / 2.0, (pts[0][1] + pts[1][1]) / 2.0)
            nxt = pts[1]
        tx, ty = nxt[0] - mid[0], nxt[1] - mid[1]
        L = math.hypot(tx, ty) or 1.0
        # exterior = the non-solid side of the travel direction
        if spec["solid_side"] == "LEFT":
            nx, ny = ty / L, -tx / L
        else:
            nx, ny = -ty / L, tx / L
        off = 0.10 * ds / s  # extent-relative, in profile units
        void_pt = (mid[0] + nx * off, mid[1] + ny * off)
        if point_in_polygon(void_pt, compiled.outline):
            void_pt = (mid[0] - nx * off, mid[1] - ny * off)
        vx, _, vz = to_world(void_pt, spec)
        _text_object("SINC_LabelVoid", "VOID", (vx, z_front, vz),
                     0.05 * ds, acol, black_mat)

    # --- label collision resolution: nudge overlapping labels apart (the
    # parity stagger alone let wall_start/stock_bottom_left collide on fig 6)
    min_gap = 0.032 * ds
    for _ in range(12):
        moved = False
        for i, (la, wa) in enumerate(label_pool):
            for lb, wb in label_pool[i + 1:]:
                ax, _, az = la.location
                bx, _, bz = lb.location
                if abs(az - bz) < min_gap and (ax < bx + wb and bx < ax + wa):
                    lb.location[2] = bz - (min_gap - abs(az - bz)) - 0.01 * ds
                    moved = True
        if not moved:
            break

    # --- wire proxy, derived from the evaluated master (not a second author)
    bpy.context.view_layer.update()
    deps = bpy.context.evaluated_depsgraph_get()
    wire_mesh = bpy.data.meshes.new_from_object(master.evaluated_get(deps))
    # dissolve the fill-fan triangulation so the wire view shows profile edges
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(wire_mesh)
    bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(1.0),
                             verts=list(bm.verts), edges=list(bm.edges))
    bm.to_mesh(wire_mesh)
    bm.free()
    wire_mesh.materials.clear()
    wire_mesh.materials.append(black_mat)
    wire = bpy.data.objects.new("SINC_WireProxy", wire_mesh)
    wire.rotation_euler = master.rotation_euler[:]
    mod = wire.modifiers.new("SINC_Wireframe", "WIREFRAME")
    mod.thickness = 0.004 * ds
    mod.use_replace = True
    cols["WireProxy"].objects.link(wire)
    wire.hide_render = True

    # --- cameras
    xs = [p[0] for p in compiled.outline]
    ys = [p[1] for p in compiled.outline]
    cx = (min(xs) + max(xs)) / 2.0 * s
    cz = (min(ys) + max(ys)) / 2.0 * s
    w_world = compiled.extents["width"] * s
    d_world = compiled.extents["depth"] * s
    max_dim = max(w_world, d_world)
    dist = 4.0 * max_dim
    # ortho_scale covers the horizontal axis at this aspect; the vertical
    # cover is ortho_scale * RES_Y/RES_X, so tall profiles need the depth
    # term scaled up or their labels crop (found on the stepped profile)
    ortho_span = max(1.35 * w_world, 1.35 * d_world * RES_X / RES_Y)

    def add_camera(name, location, rotation=None, ortho=False):
        cam = bpy.data.cameras.new(name)
        if ortho:
            cam.type = "ORTHO"
            cam.ortho_scale = ortho_span
        # scaled to the scene: a 20 mm profile must not vanish inside the
        # default 0.1 m clip_start, and clip_end must clear the far reference
        cam.clip_start = dist * 0.001
        cam.clip_end = dist * 20.0
        obj = bpy.data.objects.new(name, cam)
        obj.location = location
        if rotation is not None:
            obj.rotation_euler = rotation
        cols["Cameras"].objects.link(obj)
        return obj

    cam_front = add_camera("SINC_Cam_Front", (cx, -dist, cz),
                           (math.radians(90), 0, 0), ortho=True)
    add_camera("SINC_Cam_Opposite", (cx, dist, cz),
               (math.radians(90), 0, math.radians(180)), ortho=True)
    # perspective framing must cover the extrusion too: for a deep extrusion
    # on a small profile (CFB: depth 1.2 vs extents 1.6) framing only the
    # profile extents crops the sweep out of the three-quarter view
    persp_dist = 2.4 * max(max_dim, 1.6 * extrude_m * 2.0)
    for name in ("SINC_Cam_ThreeQuarter", "SINC_Cam_Wire"):
        loc = Vector((cx, 0, cz)) + Vector((-0.55, -1.0, 0.45)).normalized() * persp_dist
        cam = add_camera(name, tuple(loc))
        direction = Vector((cx, 0, cz)) - loc
        cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

    scene.camera = cam_front
    return {"compiled": compiled, "collections": cols, "master": master,
            "wire": wire, "reference_placed": ref_placed}


def rebuild_in_current_file(spec):
    """Delete generated SINC_ content and rebuild from `spec` (in-Blender use)."""
    for col in [c for c in bpy.data.collections if c.name.startswith("SINC_")]:
        for obj in list(col.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(col)
    return build_scene_content(spec, repo_root=None)


# ---------------------------------------------------------------- headless run

def _embed_texts(spec):
    sources = ["profile_spec.py", "profile_compiler.py", "proof_scene.py"]
    for fname in sources:
        text = bpy.data.texts.get(fname) or bpy.data.texts.new(fname)
        text.clear()
        text.write((PACKAGE_DIR / fname).read_text())
    spec_text = bpy.data.texts.get("profile_spec.json") or bpy.data.texts.new("profile_spec.json")
    spec_text.clear()
    spec_text.write(json.dumps(spec, indent=1))
    driver = bpy.data.texts.get("rebuild_proof.py") or bpy.data.texts.new("rebuild_proof.py")
    driver.clear()
    driver.write(REBUILD_DRIVER)


def _render(out_dir, info):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 1600
    scene.render.resolution_y = 1200
    scene.render.image_settings.file_format = "PNG"
    world = bpy.data.worlds.new("SINC_World")
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg is not None:
        bg.inputs["Color"].default_value = (1.0, 1.0, 1.0, 1.0)
        bg.inputs["Strength"].default_value = 1.0
    scene.world = world

    ref_objs = list(info["collections"]["Reference"].objects)
    master = info["master"]
    wire = info["wire"]
    annotations = list(info["collections"]["Annotations"].objects)

    views = {
        "front": ("SINC_Cam_Front", dict(ref=True, master=True, wire=False, ann=True)),
        "opposite": ("SINC_Cam_Opposite", dict(ref=False, master=True, wire=False, ann=True)),
        "three_quarter": ("SINC_Cam_ThreeQuarter", dict(ref=True, master=True, wire=False, ann=True)),
        "wire": ("SINC_Cam_Wire", dict(ref=False, master=False, wire=True, ann=True)),
    }
    renders = {}
    render_dir = Path(out_dir) / "renders"
    render_dir.mkdir(parents=True, exist_ok=True)
    for view, (cam_name, vis) in views.items():
        for obj in ref_objs:
            obj.hide_render = not vis["ref"]
        master.hide_render = not vis["master"]
        wire.hide_render = not vis["wire"]
        for obj in annotations:
            obj.hide_render = not vis["ann"]
        scene.camera = bpy.data.objects[cam_name]
        path = render_dir / f"{view}.png"
        scene.render.filepath = str(path)
        bpy.ops.render.render(write_still=True)
        renders[view] = str(path)
    # restore front-state defaults before saving
    for obj in ref_objs:
        obj.hide_render = False
    master.hide_render = False
    wire.hide_render = True
    for obj in annotations:
        obj.hide_render = False
    scene.camera = bpy.data.objects["SINC_Cam_Front"]
    return renders


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    args = {"--spec": None, "--out": None, "--flip": None}
    skip_render = "--skip-render" in argv
    for key in args:
        if key in argv:
            args[key] = argv[argv.index(key) + 1]
    if not args["--spec"] or not args["--out"]:
        raise SystemExit("usage: ... -- --spec SPEC.json --out DIR [--flip SEG] [--skip-render]")

    spec_path = Path(args["--spec"]).resolve()
    repo_root = spec_path.parents[4]  # <root>/assets/creative/geometry_proof/profiles/x.json
    spec = json.loads(spec_path.read_text())
    suffix = ""
    if args["--flip"]:
        out_resolved = Path(args["--out"]).resolve()
        canonical = (PACKAGE_DIR / "output").resolve()
        if out_resolved == canonical or canonical in out_resolved.parents:
            raise SystemExit(
                "--flip builds are A/B experiments and must not write into the "
                "canonical output/ directory - use a scratch directory")
        seg = next(g for g in spec["segments"] if g["name"] == args["--flip"])
        seg["direction"] = ("CLOCKWISE" if seg["direction"] == "COUNTERCLOCKWISE"
                            else "COUNTERCLOCKWISE")
        suffix = f"_flipped_{args['--flip']}"

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.preferences.filepaths.save_version = 0  # no .blend1 debris
    info = build_scene_content(spec, repo_root=repo_root)
    _embed_texts(spec)

    out_dir = Path(args["--out"])
    out_dir.mkdir(parents=True, exist_ok=True)
    renders = {} if skip_render else _render(out_dir, info)

    blend_path = out_dir / f"{spec['id']}{suffix}_proof.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path), compress=True)

    compiled = info["compiled"]
    manifest = {
        "status": "SINC_GEOMETRY_PROOF_BUILT",
        "profile_id": spec["id"],
        "flip": args["--flip"],
        "spec_sha256": hashlib.sha256(json.dumps(spec, sort_keys=True).encode()).hexdigest(),
        "blender_version": bpy.app.version_string,
        "samples_per_quarter": compiled.samples_per_quarter,
        "outline_vertices": len(compiled.outline),
        "winding": compiled.winding,
        "extents": compiled.extents,
        "normalized_stations": {k: list(v) for k, v in sorted(compiled.normalized_stations.items())},
        "warnings": compiled.warnings,
        "reference_placed": info["reference_placed"],
        "reference_note": (
            "placed" if info["reference_placed"]
            else ("image declared but NOT placed - missing crop_px_at_origin/"
                  "crop_px_per_unit or the image file"
                  if spec["reference"].get("image") else "no reference image declared")),
        "objects": sorted(o.name for o in bpy.data.objects),
        "renders": renders,
        "single_source": "profile_compiler.compile_profile",
    }
    manifest_path = out_dir / f"{spec['id']}{suffix}_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=1, sort_keys=True))
    print(f"SINC_PROOF_OK blend={blend_path} manifest={manifest_path}")


if __name__ == "__main__":
    main()
