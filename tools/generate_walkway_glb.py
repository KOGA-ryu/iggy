#!/usr/bin/env python3
"""Generate the deterministic modular stone walkway Creative fixture."""

import json
import pathlib
import struct


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets" / "creative" / "walkway_stone_01.glb"


def append_box(positions, indices, center, size):
    base = len(positions)
    hx, hy, hz = (axis * 0.5 for axis in size)
    cx, cy, cz = center
    positions.extend([
        (cx - hx, cy - hy, cz - hz),
        (cx + hx, cy - hy, cz - hz),
        (cx + hx, cy + hy, cz - hz),
        (cx - hx, cy + hy, cz - hz),
        (cx - hx, cy - hy, cz + hz),
        (cx + hx, cy - hy, cz + hz),
        (cx + hx, cy + hy, cz + hz),
        (cx - hx, cy + hy, cz + hz),
    ])
    triangles = (
        (0, 1, 2), (0, 2, 3), (4, 6, 5), (4, 7, 6),
        (0, 3, 7), (0, 7, 4), (1, 5, 6), (1, 6, 2),
        (3, 2, 6), (3, 6, 7), (0, 4, 5), (0, 5, 1),
    )
    for triangle in triangles:
        indices.extend(base + index for index in triangle)


positions = []
indices = []
slabs = (
    ((-1.125, 0.00, 0.00), (0.70, 0.22, 1.18)),
    ((-0.375, 0.025, 0.02), (0.70, 0.27, 1.14)),
    ((0.375, -0.005, -0.02), (0.70, 0.21, 1.20)),
    ((1.125, 0.015, 0.01), (0.70, 0.25, 1.16)),
)
for center, size in slabs:
    append_box(positions, indices, center, size)

position_bytes = b"".join(struct.pack("<3f", *position) for position in positions)
index_bytes = b"".join(struct.pack("<H", index) for index in indices)
binary = position_bytes + index_bytes
while len(binary) % 4:
    binary += b"\0"

minimum = [min(position[axis] for position in positions) for axis in range(3)]
maximum = [max(position[axis] for position in positions) for axis in range(3)]
document = {
    "asset": {"version": "2.0", "generator": "iggy3d walkway fixture"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{
        "name": "Walkway_Stone_01",
        "mesh": 0,
        "extras": {
            "iggy_category": "walkway",
            "iggy_collision": "bounds",
            "iggy_walkable": True,
        },
    }],
    "meshes": [{
        "name": "Walkway_Stone_01_Mesh",
        "primitives": [{"attributes": {"POSITION": 0}, "indices": 1,
                        "material": 0}],
    }],
    "materials": [{
        "name": "Walkway Stone",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.46, 0.49, 0.51, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.88,
        },
    }],
    "buffers": [{"byteLength": len(binary)}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": len(position_bytes),
         "target": 34962},
        {"buffer": 0, "byteOffset": len(position_bytes),
         "byteLength": len(index_bytes), "target": 34963},
    ],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": len(positions),
         "type": "VEC3", "min": minimum, "max": maximum},
        {"bufferView": 1, "componentType": 5123, "count": len(indices),
         "type": "SCALAR", "min": [min(indices)], "max": [max(indices)]},
    ],
}

json_bytes = json.dumps(document, separators=(",", ":")).encode("utf-8")
while len(json_bytes) % 4:
    json_bytes += b" "

total_length = 12 + 8 + len(json_bytes) + 8 + len(binary)
glb = struct.pack("<III", 0x46546C67, 2, total_length)
glb += struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
glb += struct.pack("<II", len(binary), 0x004E4942) + binary

OUTPUT.parent.mkdir(parents=True, exist_ok=True)
OUTPUT.write_bytes(glb)
print(f"wrote {OUTPUT} ({len(glb)} bytes, {len(positions)} vertices, "
      f"{len(indices) // 3} triangles)")
