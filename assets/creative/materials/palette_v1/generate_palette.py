"""ASSET-BLD-1 Phases 1+2, direct-drive.

Phase 1: material calibration proofs (base_color, texture_uv, missing_texture
         pre-surgery export).
Phase 2: palette_v1 - six seamless 1024px albedo tiles generated procedurally
         (numpy value noise, periodic => tileable by construction), tiling
         proofs, swatch sheet.
Bonus:   textured cottage vignette using the palette (look-dev, not engine).

Blender 4.5 headless. Deterministic: fixed seeds, no timestamps.
"""
import bpy
import math
import numpy as np
import os

W = "/home/kogaRyu/iggy3d-bld1"
PAL = f"{W}/assets/creative/materials/palette_v1"
CALDIR = f"{W}/assets/creative/calibration"
PROOF = f"{PAL}/proofs"
OUTDIR = "/tmp/bld1_out"
os.makedirs(PAL, exist_ok=True)
os.makedirs(PROOF, exist_ok=True)
os.makedirs(OUTDIR, exist_ok=True)

RES = 1024

# ---------------- periodic value noise (seamless) ----------------

def value_noise(res, cells, seed, octaves=4, persistence=0.55):
    rng = np.random.default_rng(seed)
    out = np.zeros((res, res))
    amp, total = 1.0, 0.0
    c = cells
    for _ in range(octaves):
        g = rng.random((c, c))
        gx = np.repeat(np.arange(res) * c / res, 1)
        i0 = np.floor(gx).astype(int) % c
        i1 = (i0 + 1) % c
        f = (gx - np.floor(gx))
        f = f * f * (3 - 2 * f)
        row0 = g[np.ix_(i0, i0)]  # placeholder; replaced below
        # bilinear periodic sample
        a = g[np.ix_(i0, i0)]
        b = g[np.ix_(i0, i1)]
        cc = g[np.ix_(i1, i0)]
        d = g[np.ix_(i1, i1)]
        fx = f[np.newaxis, :]
        fy = f[:, np.newaxis]
        layer = (a * (1 - fx) * (1 - fy) + b * fx * (1 - fy) +
                 cc * (1 - fx) * fy + d * fx * fy)
        out += amp * layer
        total += amp
        amp *= persistence
        c *= 2
    return out / total


def save_tile(name, rgb):
    """rgb: (res,res,3) float 0..1 -> PNG in palette dir."""
    res = rgb.shape[0]
    img = bpy.data.images.new(name, res, res, alpha=False)
    px = np.ones((res, res, 4), dtype=np.float32)
    px[:, :, :3] = rgb.astype(np.float32)
    img.pixels.foreach_set(px.ravel())
    img.filepath_raw = f"{PAL}/{name}.png"
    img.file_format = "PNG"
    img.save()
    return f"{PAL}/{name}.png"


def lerp3(a, b, t):
    t3 = t[..., np.newaxis]
    return np.array(a) * (1 - t3) + np.array(b) * t3


tiles = {}

# oak_plank: vertical planks with grain and per-plank tone
n_planks = 6
xcoord = np.tile(np.arange(RES) / RES, (RES, 1))
plank_id = np.floor(xcoord * n_planks) % n_planks
rngp = np.random.default_rng(11)
plank_tone = rngp.random(n_planks) * 0.16
tone = plank_tone[plank_id.astype(int)]
grain = value_noise(RES, 4, 12, octaves=5)
grain = np.abs(np.sin((np.tile(np.arange(RES) / RES, (RES, 1)).T * 26 +
                       grain * 7) * math.pi))
seams = ((xcoord * n_planks) % 1.0 < 0.02) | ((xcoord * n_planks) % 1.0 > 0.98)
base = lerp3((0.34, 0.23, 0.13), (0.46, 0.33, 0.20), grain * 0.65 + tone)
base[seams] *= 0.55
tiles["oak_plank"] = save_tile("oak_plank", base)

# oak_timber: horizontal beam grain, tighter, darker
g1 = value_noise(RES, 3, 21, octaves=5)
ycoord = np.tile(np.arange(RES) / RES, (RES, 1)).T
streak = np.abs(np.sin((ycoord * 18 + g1 * 5) * math.pi))
knots = value_noise(RES, 6, 22, octaves=3)
base = lerp3((0.26, 0.17, 0.10), (0.38, 0.26, 0.15), streak * 0.7)
base[knots > 0.82] *= 0.7
tiles["oak_timber"] = save_tile("oak_timber", base)

# lime_plaster: soft blotch, restrained
n1 = value_noise(RES, 5, 31, octaves=4)
n2 = value_noise(RES, 13, 32, octaves=3)
base = lerp3((0.66, 0.61, 0.50), (0.76, 0.72, 0.61), n1 * 0.7 + n2 * 0.3)
tiles["lime_plaster"] = save_tile("lime_plaster", base)

# stone_rough: cellular-ish blocks via quantized noise + mortar lines
n = value_noise(RES, 7, 41, octaves=4)
q = np.floor(n * 6) / 6
edge = np.abs(n * 6 - np.round(n * 6)) < 0.05
base = lerp3((0.33, 0.32, 0.29), (0.46, 0.44, 0.40), q + value_noise(RES, 17, 42, 3) * 0.2)
base[edge] *= 0.7
tiles["stone_rough"] = save_tile("stone_rough", base)

# shingle_oak: staggered rows, per-shingle jitter
rows = 8
shw = 6
yr = ycoord * rows
row = np.floor(yr).astype(int)
stagger = (row % 2) * 0.5
xs = xcoord * shw + stagger
sh_id = (np.floor(xs).astype(int) % shw) + row * shw
rngs = np.random.default_rng(51)
jit = rngs.random(rows * shw + shw)
tone = jit[sh_id % len(jit)] * 0.2
rowline = (yr % 1.0) < 0.06
shline = (xs % 1.0) < 0.04
base = lerp3((0.24, 0.17, 0.11), (0.34, 0.26, 0.17), tone / 0.2 * 0.8 +
             value_noise(RES, 9, 52, 3) * 0.2)
base[rowline] *= 0.55
base[shline] *= 0.7
tiles["shingle_oak"] = save_tile("shingle_oak", base)

# iron_forged: near-black, hammered speckle
n = value_noise(RES, 9, 61, octaves=5)
base = lerp3((0.055, 0.055, 0.06), (0.11, 0.11, 0.12), n)
tiles["iron_forged"] = save_tile("iron_forged", base)

# uv marker texture for material_texture_uv: checker + orientation corners
uvres = 512
uc = np.tile(np.arange(uvres) / uvres, (uvres, 1))
vc = np.tile(np.arange(uvres) / uvres, (uvres, 1)).T
checker = ((np.floor(uc * 8) + np.floor(vc * 8)) % 2)
uvrgb = lerp3((0.25, 0.25, 0.28), (0.75, 0.72, 0.65), checker)
uvrgb[(uc < 0.12) & (vc < 0.12)] = (0.85, 0.15, 0.15)   # origin corner red
uvrgb[(uc > 0.88) & (vc < 0.12)] = (0.15, 0.65, 0.2)    # +U green
uvrgb[(uc < 0.12) & (vc > 0.88)] = (0.15, 0.3, 0.85)    # +V blue
uv_path = save_tile("calibration_uv_marker", uvrgb)
os.replace(uv_path, f"{CALDIR}/calibration_uv_marker.png")
uv_path = f"{CALDIR}/calibration_uv_marker.png"

print("TILES DONE")

# ---------------- scene helpers ----------------

def reset_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)


def image_mat(name, img_path, rough=0.8, map_scale=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = rough
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(img_path)
    if map_scale:
        coord = nt.nodes.new("ShaderNodeTexCoord")
        mapping = nt.nodes.new("ShaderNodeMapping")
        mapping.inputs["Scale"].default_value = map_scale
        nt.links.new(coord.outputs["UV"], mapping.inputs["Vector"])
        nt.links.new(mapping.outputs["Vector"], tex.inputs["Vector"])
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return m


def export_glb(objs, path):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB",
                              use_selection=True, export_extras=True,
                              export_apply=True, export_yup=True)


# ---------------- phase 1: material proof assets ----------------
reset_scene()
# material_base_color: six 0.4m cubes in a row, one per palette color
colors = [(0.40, 0.29, 0.17), (0.32, 0.22, 0.13), (0.71, 0.66, 0.55),
          (0.42, 0.40, 0.37), (0.29, 0.22, 0.15), (0.08, 0.08, 0.09)]
objs = []
for i, c in enumerate(colors):
    bpy.ops.mesh.primitive_cube_add(size=0.4, location=(i * 0.5 - 1.25, 0, 0.2))
    o = bpy.context.active_object
    o.name = f"swatch_{i}"
    m = bpy.data.materials.new(f"swatch_mat_{i}")
    m.use_nodes = True
    next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED") \
        .inputs["Base Color"].default_value = (*c, 1.0)
    o.data.materials.append(m)
    o["iggy_category"] = "calibration"
    o["iggy_collision"] = "none"
    objs.append(o)
export_glb(objs, f"{CALDIR}/material_base_color.glb")

reset_scene()
# material_texture_uv: 1m cube with the uv marker texture
bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.5))
o = bpy.context.active_object
o.name = "texture_uv_proof"
o.data.materials.append(image_mat("uv_marker", uv_path, 0.9))
o["iggy_category"] = "calibration"
o["iggy_collision"] = "none"
export_glb([o], f"{CALDIR}/material_texture_uv.glb")
# missing_texture starts as a copy; python3 post-pass severs the image
export_glb([o], f"{CALDIR}/material_missing_texture.glb")
print("PHASE1 DONE")

# ---------------- phase 2 proofs: tiling + swatch sheet ----------------
scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.samples = 48
scene.cycles.use_denoising = True
scene.cycles.device = "CPU"
scene.view_settings.view_transform = "Standard"

for name, path in [(k, f"{PAL}/{k}.png") for k in
                   ["oak_plank", "oak_timber", "lime_plaster",
                    "stone_rough", "shingle_oak", "iron_forged"]]:
    reset_scene()
    bpy.ops.mesh.primitive_plane_add(size=3.0)
    p = bpy.context.active_object
    p.data.materials.append(image_mat(f"tile_{name}", path, 1.0,
                                      map_scale=(3, 3, 1)))
    sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
    bpy.context.collection.objects.link(sun)
    sun.data.energy = 2.5
    cam_d = bpy.data.cameras.new("c")
    cam_d.type = "ORTHO"
    cam_d.ortho_scale = 3.05
    cam = bpy.data.objects.new("c", cam_d)
    bpy.context.collection.objects.link(cam)
    cam.location = (0, 0, 5)
    scene.camera = cam
    scene.render.resolution_x = 900
    scene.render.resolution_y = 900
    scene.render.filepath = f"{PROOF}/tiling_{name}.png"
    bpy.ops.render.render(write_still=True)
print("TILING PROOFS DONE")

# swatch sheet: six labeled-by-position quads
reset_scene()
for i, name in enumerate(["oak_plank", "oak_timber", "lime_plaster",
                          "stone_rough", "shingle_oak", "iron_forged"]):
    bpy.ops.mesh.primitive_plane_add(size=1.9,
                                     location=((i % 3) * 2 - 2,
                                               -(i // 3) * 2 + 1, 0))
    p = bpy.context.active_object
    p.data.materials.append(image_mat(f"sw_{name}", f"{PAL}/{name}.png", 1.0))
sun = bpy.data.objects.new("s", bpy.data.lights.new("s", "SUN"))
bpy.context.collection.objects.link(sun)
sun.data.energy = 2.5
cam_d = bpy.data.cameras.new("c")
cam_d.type = "ORTHO"
cam_d.ortho_scale = 6.4
cam = bpy.data.objects.new("c", cam_d)
bpy.context.collection.objects.link(cam)
cam.location = (0, 0, 5)
scene.camera = cam
scene.render.resolution_x = 1200
scene.render.resolution_y = 800
scene.render.filepath = f"{PROOF}/palette_sheet.png"
bpy.ops.render.render(write_still=True)
print("PALETTE SHEET DONE")
print("BLD1 PHASE 1+2 COMPLETE")
