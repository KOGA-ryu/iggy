#!/usr/bin/env python3
"""Generate the deterministic modular stone walkway Creative fixture."""

import json
import pathlib
import struct
import zlib


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


def png_chunk(kind, payload):
    return (struct.pack(">I", len(payload)) + kind + payload +
            struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))


def checker_png():
    width = 4
    height = 4
    dark = (78, 86, 92, 255)
    light = (146, 154, 160, 255)
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            rows.extend(light if (x + y) % 2 == 0 else dark)
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", header) +
            png_chunk(b"IDAT", zlib.compress(bytes(rows), 9)) +
            png_chunk(b"IEND", b""))


position_bytes = b"".join(struct.pack("<3f", *position) for position in positions)
minimum = [min(position[axis] for position in positions) for axis in range(3)]
maximum = [max(position[axis] for position in positions) for axis in range(3)]
uvs = [
    ((position[0] - minimum[0]) / (maximum[0] - minimum[0]),
     (position[2] - minimum[2]) / (maximum[2] - minimum[2]))
    for position in positions
]
uv_bytes = b"".join(struct.pack("<2f", *uv) for uv in uvs)
index_bytes = b"".join(struct.pack("<H", index) for index in indices)
texture_bytes = checker_png()
binary = position_bytes + uv_bytes + index_bytes + texture_bytes
while len(binary) % 4:
    binary += b"\0"

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
        "primitives": [{"attributes": {"POSITION": 0, "TEXCOORD_0": 1},
                        "indices": 2,
                        "material": 0}],
    }],
    "materials": [{
        "name": "Walkway Stone",
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.46, 0.49, 0.51, 1.0],
            "baseColorTexture": {"index": 0},
            "metallicFactor": 0.0,
            "roughnessFactor": 0.88,
        },
    }],
    "images": [{"name": "Walkway Checker", "bufferView": 3,
                "mimeType": "image/png"}],
    "textures": [{"name": "Walkway Checker", "source": 0}],
    "buffers": [{"byteLength": len(binary)}],
    "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": len(position_bytes),
         "target": 34962},
        {"buffer": 0, "byteOffset": len(position_bytes),
         "byteLength": len(uv_bytes), "target": 34962},
        {"buffer": 0, "byteOffset": len(position_bytes) + len(uv_bytes),
         "byteLength": len(index_bytes), "target": 34963},
        {"buffer": 0,
         "byteOffset": len(position_bytes) + len(uv_bytes) + len(index_bytes),
         "byteLength": len(texture_bytes)},
    ],
    "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": len(positions),
         "type": "VEC3", "min": minimum, "max": maximum},
        {"bufferView": 1, "componentType": 5126, "count": len(uvs),
         "type": "VEC2", "min": [0.0, 0.0], "max": [1.0, 1.0]},
        {"bufferView": 2, "componentType": 5123, "count": len(indices),
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
