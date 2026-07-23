"""ASSET-BLD-1 facade_slice assembly proof: renders the ENGINE FIXTURE.

Parses fixtures/worlds/facade_slice.iggy3d.save and imports every Prop's GLB
at its saved transform, so the render proves the fixture composition itself
(no hand-arranged scene). Engine space is glTF Y-up; Blender placement is
(x, -z, y). All fixture rotations are 0 (authoring contract for this slice).

Run headless:
    blender --background --python render_facade.py
Writes /tmp/bld1_facade/facade_front.png, facade_three_quarter.png,
facade_door_detail.png.
"""
import math
import os
import re

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[os.pardir] * 4))
SAVE = os.path.join(ROOT, "fixtures", "worlds", "facade_slice.iggy3d.save")
CREATIVE = os.path.join(ROOT, "assets", "creative")
OUT = "/tmp/bld1_facade"
os.makedirs(OUT, exist_ok=True)

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)

objects = {}
with open(SAVE) as fh:
    for line in fh:
        m = re.match(r"creativeDocument\.object\.(\d+)\.(\w+(?:\.\w+)*)=(.*)",
                     line.strip())
        if m:
            objects.setdefault(int(m.group(1)), {})[m.group(2)] = m.group(3)

count = 0
for idx in sorted(objects):
    ob = objects[idx]
    asset = ob.get("assetId")
    if not asset:
        continue
    px, py, pz = (float(v) for v in ob["transform.position"].split(","))
    rot = ob.get("transform.rotation", "0,0,0")
    assert rot == "0,0,0", "facade fixture must be rotation-free: " + str(ob)
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=os.path.join(CREATIVE, asset + ".glb"))
    new = [o for o in bpy.data.objects if o not in before]
    roots = [o for o in new if o.parent is None or o.parent not in new]
    for r in roots:
        r.location.x += px
        r.location.y += -pz
        r.location.z += py
    count += 1
print("PLACED", count, "props from fixture")

# ground plane stand-in for the Floor object (generated surface)
bpy.ops.mesh.primitive_plane_add(size=24.0, location=(0, 0, 0))
ground = bpy.context.active_object
gm = bpy.data.materials.new("ground")
gm.use_nodes = True
gm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = \
    (0.18, 0.21, 0.14, 1.0)
gm.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.95
ground.data.materials.append(gm)

scene = bpy.context.scene
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
bpy.context.collection.objects.link(sun)
sun.data.energy = 2.0
sun.data.angle = math.radians(2.5)
sun.data.color = (1.0, 0.93, 0.82)
sun.rotation_euler = (math.radians(52), 0, math.radians(-38))
world = bpy.data.worlds.new("world")
scene.world = world
world.use_nodes = True
sky = world.node_tree.nodes.new("ShaderNodeTexSky")
sky.sun_elevation = math.radians(38)
sky.sun_rotation = math.radians(142)
bg = world.node_tree.nodes["Background"]
bg.inputs["Strength"].default_value = 0.06
world.node_tree.links.new(sky.outputs["Color"], bg.inputs["Color"])

scene.render.engine = "CYCLES"
scene.cycles.samples = 96
scene.cycles.use_denoising = True
scene.cycles.device = "CPU"
scene.view_settings.view_transform = "Filmic"
scene.view_settings.exposure = -0.4
scene.view_settings.look = "Medium High Contrast"
scene.render.resolution_x = 1280
scene.render.resolution_y = 860

target = bpy.data.objects.new("target", None)
bpy.context.collection.objects.link(target)
cam_d = bpy.data.cameras.new("cam")
cam_d.lens = 38
cam = bpy.data.objects.new("cam", cam_d)
bpy.context.collection.objects.link(cam)
track = cam.constraints.new("TRACK_TO")
track.target = target
track.track_axis = "TRACK_NEGATIVE_Z"
track.up_axis = "UP_Y"
scene.camera = cam

# fixture props stand along engine z=0 facing +Z (blender -Y): camera south
SHOTS = [
    ("facade_front.png", (0.0, -11.5, 2.1), (-0.6, 0.0, 1.7)),
    ("facade_three_quarter.png", (8.6, -8.8, 3.4), (-0.8, 0.0, 1.6)),
    ("facade_door_detail.png", (2.6, -4.2, 1.7), (-0.4, 0.0, 1.3)),
]
for name, loc, tgt in SHOTS:
    cam.location = loc
    target.location = tgt
    scene.render.filepath = os.path.join(OUT, name)
    bpy.ops.render.render(write_still=True)
    print("SHOT", name)
print("FACADE PROOF COMPLETE")
