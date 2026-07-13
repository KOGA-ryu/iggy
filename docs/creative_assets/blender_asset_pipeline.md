# Blender Static Asset Pipeline

## Current contract

Creative accepts static Blender assets as binary glTF 2.0 (`.glb`). Keep the
`.blend` file as editable source; the engine loads the exported `.glb`.

An authored `CreativeObject` stores a stable `assetId`, not an absolute path.
The renderer resolves:

```text
assetId: boulder_01
file:    assets/creative/boulder_01.glb
meshId:  asset:boulder_01
```

That key survives clipboard duplication, RoomBake, scene projection, save, and
load. Imported files are cached by `assetId`; an unchanged scene reuses both the
parsed asset and the existing room GPU buffer. A missing asset renders as a
magenta bounds proxy so broken content is visible without blanking the room.

At startup, Creative recursively discovers valid `.glb` files beneath
`assets/creative/`. The relative path without `.glb` is the catalog and save
identity. Discovery is sorted, validates every file through the production
importer, and reports rejected paths with a reason code.

## Creative workflow

1. Open the Creative catalog.
2. Choose the `ASSETS` tab.
3. Search or select an imported mesh and equip it into the active hotbar slot.
4. Aim at a placement surface. The held viewmodel and placement target use the
   imported mesh, while green/red role colors retain placement validity.
5. Place normally. Pick-block on an imported object restores its `assetId` and
   authored dimensions to the selected hotbar slot.

The hotbar carries a bounded asset ID and natural imported dimensions. The
placement plan uses those dimensions for the authored bounds, preview, create
request, duplicate check, undo transaction, and saved object. Generic materials
and imported assets with the same object kind remain distinct placement items.

Preview geometry is flattened into one startup GPU atlas with held-yellow,
valid-green, and invalid-red ranges per asset. Aim movement only changes the
preview matrix; it does not rebuild room geometry or upload meshes per frame.

## Blender export

1. Model at real scale. One Blender unit should mean one meter.
2. Set the object origin where Creative rotation and scaling should pivot.
3. Apply rotation and scale before export.
4. Keep triangle count and material count appropriate for repeated map props.
5. Export with `File > Export > glTF 2.0`.
6. Choose `Format: glTF Binary (.glb)` and export selected objects when the
   file contains unrelated authoring helpers.
7. Place the file under `assets/creative/` and use its relative path without
   `.glb` as the `assetId`.

glTF is right-handed, Y-up, and meter-based. Blender's exporter performs the
coordinate conversion; do not rotate the root object to compensate again.

## Supported now

- Binary glTF 2.0 files.
- Static triangle primitives with indexed or non-indexed geometry.
- Multiple nodes, meshes, primitives, and materials.
- Node transforms flattened into asset-local geometry.
- Material base-color, metallic, roughness, and base-color texture references
  retained in the imported material contract.
- Object bounds used as the collision and placement envelope.
- Dedicated Assets catalog page with search and hotbar assignment.
- Exact imported held and placement previews with the existing bounds outline.
- Per-object Creative translation, Euler rotation, and non-uniform scale.
- Deterministic content hash and one-load process cache.

The current room shader is position plus vertex color. Imported material base
color is rendered with deterministic face shading so irregular geometry is
readable. Image textures are recorded but not sampled by Vulkan yet.

## Explicitly deferred

- PNG/JPEG decoding, GPU texture images, samplers, mipmaps, and material
  descriptor sets.
- Normal, occlusion, metallic-roughness, and emissive texture evaluation.
- Alpha blend/mask materials.
- Collision hulls or triangle collision from Blender custom properties.
- Walkable-surface extraction, sockets, and attachment points from glTF extras.
- Skinning, armatures, morph targets, animation clips, and character graphs.
- Offline cooked mesh packages and cross-run derived-data caching.

Static import rejects skins, morph targets, non-triangle primitives, malformed
accessors, unsafe asset IDs, non-finite geometry, and degenerate three-axis
bounds. Do not silently convert those cases to boxes.

## Reserved Blender custom properties

The fixtures use these names so future cooker work has a stable source
vocabulary. They are metadata-only today. Until glTF extras are promoted into
the importer contract, filenames containing `boulder`/`rock` map to `Rock`,
filenames containing `walkway`/`bridge` map to `Bridge`, and other assets map to
`Prop`:

| Property | Intended values | Future owner |
|---|---|---|
| `iggy_category` | `boulder`, `building`, `walkway`, `prop` | Creative catalog |
| `iggy_collision` | `bounds`, `convex`, `mesh`, `none` | Physics cooker |
| `iggy_walkable` | boolean | Surface bake |
| `iggy_socket_*` | local transform | Attachment/socket cooker |

## Fixture and regeneration

The repository carries two end-to-end fixtures:

- `assets/creative/boulder_01.glb`: low-poly 18-vertex, 32-triangle boulder.
- `assets/creative/walkway_stone_01.glb`: four-slab, 32-vertex modular walkway.

Regenerate them with:

```sh
python3 tools/generate_boulder_glb.py
python3 tools/generate_walkway_glb.py
```

The generator is only for the deterministic test fixture. Production art
should be exported from Blender.

## References

- Blender glTF 2.0 exporter:
  https://docs.blender.org/manual/en/latest/addons/import_export/scene_gltf2.html
- Khronos glTF 2.0 specification:
  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- cgltf loader, vendored at v1.15 under MIT:
  https://github.com/jkuhlmann/cgltf
