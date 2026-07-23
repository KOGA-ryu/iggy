"""Sever material_missing_texture.glb's image reference (ASSET-BLD-1).

The missing-texture calibration proof must reference a texture the engine
cannot resolve, to exercise the visible fallback path. This committed,
re-runnable post-pass converts the freshly exported embedded PNG into a
dangling external URI. Run after generate_palette.py's phase-1 exports:

    python3 assets/creative/calibration/tools/sever_missing_texture.py

(generate_palette.py invokes it automatically via subprocess.)
"""
import os

from pygltflib import GLTF2

HERE = os.path.dirname(os.path.abspath(__file__))
PATH = os.path.abspath(
    os.path.join(HERE, os.pardir, "material_missing_texture.glb"))

g = GLTF2().load(PATH)
if not g.images:
    raise SystemExit("no image to sever in " + PATH)
img = g.images[0]
img.bufferView = None
img.mimeType = None
img.uri = "missing/does_not_exist.png"
g.save(PATH)
print("SEVERED", PATH)
