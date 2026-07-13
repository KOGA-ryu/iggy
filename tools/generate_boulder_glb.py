#!/usr/bin/env python3
"""Generate the small deterministic GLB used by the Creative import fixture."""

import json
import math
import pathlib
import struct


ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "assets" / "creative" / "boulder_01.glb"


def ring(y, radii, phase=0.0):
    return [
        (
            radii[index][0] * math.cos(phase + index * math.pi / 4.0),
            y + radii[index][2],
            radii[index][1] * math.sin(phase + index * math.pi / 4.0),
        )
        for index in range(8)
    ]


upper = ring(
    0.28,
    [
        (0.74, 0.58, 0.03),
        (0.69, 0.64, -0.02),
        (0.72, 0.61, 0.05),
        (0.66, 0.57, 0.00),
        (0.70, 0.63, -0.04),
        (0.75, 0.56, 0.02),
        (0.67, 0.65, -0.01),
        (0.71, 0.59, 0.04),
    ],
    0.08,
)
lower = ring(
    -0.30,
    [
        (0.62, 0.54, -0.02),
        (0.67, 0.51, 0.03),
        (0.59, 0.58, -0.01),
        (0.65, 0.50, 0.02),
        (0.61, 0.55, -0.03),
        (0.64, 0.52, 0.01),
        (0.60, 0.57, 0.00),
        (0.66, 0.49, -0.02),
    ],
    -0.05,
)
positions = [(0.04, 0.72, -0.03), *upper, *lower, (-0.02, -0.66, 0.05)]
top = 0
upper_start = 1
lower_start = 9
bottom = 17

indices = []
for index in range(8):
    next_index = (index + 1) % 8
    indices.extend((top, upper_start + next_index, upper_start + index))
    indices.extend((upper_start + index, upper_start + next_index,
                    lower_start + index))
    indices.extend((upper_start + next_index, lower_start + next_index,
                    lower_start + index))
    indices.extend((bottom, lower_start + index, lower_start + next_index))

position_bytes = b"".join(struct.pack("<3f", *position) for position in positions)
index_bytes = b"".join(struct.pack("<H", index) for index in indices)
binary = position_bytes + index_bytes
while len(binary) % 4:
    binary += b"\0"

minimum = [min(position[axis] for position in positions) for axis in range(3)]
maximum = [max(position[axis] for position in positions) for axis in range(3)]
document = {
    "asset": {"version": "2.0", "generator": "iggy3d boulder fixture"},
    "scene": 0,
    "scenes": [{"nodes": [0]}],
    "nodes": [{
        "name": "Boulder_01",
        "mesh": 0,
        "extras": {
            "iggy_category": "boulder",
            "iggy_collision": "bounds",
            "iggy_walkable": False,
        },
    }],
    "meshes": [{
        "name": "Boulder_01_Mesh",
        "primitives": [{"attributes": {"POSITION": 0}, "indices": 1,
                        "material": 0}],
    }],
    "materials": [{
        "name": "Boulder Granite",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.42, 0.39, 0.34, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 0.92,
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
