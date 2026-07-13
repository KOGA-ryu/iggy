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
importer, imports bounded authoring metadata, and reports rejected paths with a
reason code. RoomBake receives the resulting catalog directly; neither RoomBake
nor the renderer reopens Blender assets in the frame loop.

Asset discovery can be repeated explicitly without restarting Creative. Open
the `ASSETS` catalog page, select `RELOAD ASSETS`, and confirm with `X` on a
controller or `Enter` on a keyboard. Reload is a user-requested transaction at
a safe frame boundary, not a background watcher or per-frame filesystem poll.
It waits for the renderer to become idle, stages a complete replacement mesh
atlas, texture set, descriptor layout, and material pipeline, and publishes
them only when all required GPU resources are ready.

Stable `assetId` values preserve equipped hotbar entries and catalog selection
across reload. Existing objects whose bounds still match their prior imported
source bounds adopt new source bounds and pivot geometry in one undoable
document mutation. Objects that the creator resized explicitly keep those
custom bounds. Updated collision and walkability metadata take effect through
the refreshed catalog on the next scene bake. Deleted or newly broken files
remain visible as explicit catalog errors, while already placed references use
the existing magenta missing-asset proxy instead of disappearing.

## Creative workflow

1. Open the Creative catalog.
2. Choose the `ASSETS` tab.
3. Search or select an imported mesh. The detail panel reports its asset ID,
   dimensions, pivot offset, and physics mode.
4. Equip remains the default action. To replace existing imported objects,
   select them first, use left/right or controller D-pad left/right to focus
   `REPLACE SELECTION`, then press `Enter` or controller X. Pointer users can
   click `EQUIP` or `REPLACE SELECTION` directly.
5. Replacement opens a full-scene preview using the actual imported mesh.
   `Enter` or controller X applies it; `Escape` or controller Circle cancels.
   Multi-selection is one atomic document revision and one undo record. Object
   IDs, transforms, hierarchy, layer, tags, and visibility remain unchanged.
6. Aim at a placement surface. The held viewmodel and placement target use the
   imported mesh, while green/red role colors retain placement validity.
7. Place normally. Pick-block on an imported object restores its `assetId` and
   authored dimensions to the selected hotbar slot.
8. After exporting a changed `.glb`, use `RELOAD ASSETS` on this page. Restarting
   Creative is not required.

Replacement is intentionally conservative. It accepts only imported Prop,
Rock, and Bridge objects whose stored bounds still match the source asset's
natural bounds at the object's pivot. Locked objects, missing source assets,
unsupported kinds, invalid targets, and custom resized bounds fail closed. No
eligible object is changed unless the complete selected batch validates.

The hotbar carries a bounded asset ID and the exact source-space minimum and
maximum bounds. The placement plan aligns the rotated bottom-center of those
bounds to the target while retaining the asset coordinate origin as the object
transform pivot. The same bounds and pivot feed the preview, create request,
duplicate check, undo transaction, and saved object. Pick-block reconstructs
the source bounds relative to the stored pivot. Generic materials and imported
assets with the same object kind remain distinct placement items.

Preview geometry is flattened into one startup GPU atlas with held-yellow,
valid-green, and invalid-red ranges per asset. Aim movement only changes the
preview matrix; it does not rebuild room geometry or upload meshes per frame.

## Blender export

1. Model at real scale. One Blender unit should mean one meter.
2. Set the object origin where Creative rotation and scaling should pivot, then
   place that origin at the exported scene origin.
3. Apply rotation and scale before export.
4. Keep triangle count and material count appropriate for repeated map props.
5. Export with `File > Export > glTF 2.0`.
6. Choose `Format: glTF Binary (.glb)` and export selected objects when the
   file contains unrelated authoring helpers.
7. Place the file under `assets/creative/` and use its relative path without
   `.glb` as the `assetId`.

glTF is right-handed, Y-up, and meter-based. Blender's exporter performs the
coordinate conversion; do not rotate the root object to compensate again.
Creative has one pivot per imported asset: the glTF asset coordinate origin.
Multi-node assets therefore share that origin rather than retaining a separate
editable pivot for every node.

## Supported now

- Binary glTF 2.0 files.
- Static triangle primitives with indexed or non-indexed geometry.
- Multiple nodes, meshes, primitives, and materials.
- Node transforms flattened into asset-local geometry.
- Material base-color, metallic, roughness, and base-color texture references
  retained in the imported material contract.
- `TEXCOORD_0`, base-color UV transforms, and glTF wrap/filter settings.
- Embedded buffer-view and base64 PNG/JPEG images plus safe relative external
  PNG/JPEG files.
- Bounded RGBA8 decoding: maximum 64 MiB encoded, 8192 pixels per axis, and
  16 million decoded pixels per image.
- Startup GPU texture/sampler/descriptor creation, deduplicated by image
  content and sampler state.
- Object bounds used as the collision and placement envelope.
- `iggy_collision="bounds"` emits actor and projectile bounds blockers.
- `iggy_collision="none"` keeps the object renderable without collision.
- Authored `iggy_walkable=true` adds one walkable top surface to bounds
  collision for upright or yaw-rotated instances. Pitched or rolled instances
  retain bounds blockers but do not fabricate a horizontal walkable top.
  Walkability is never inferred from shape or filename.
- An omitted `iggy_collision` property uses explicit legacy defaults: bounds
  collision, not walkable unless `iggy_walkable=true` is authored. The Assets
  catalog marks default collision as `SOLID DEFAULT`.
- Unsupported `convex` and `mesh` collision modes remain visible in the scene
  and catalog but fail closed to no physics until their cookers exist.
- Dedicated Assets catalog page with search and hotbar assignment.
- Explicit `RELOAD ASSETS` command with stable-ID selection/hotbar retention,
  transactional GPU replacement, and catalog-visible import failures.
- Exact imported held and placement previews with the existing bounds outline.
- Non-destructive exact-mesh replacement preview with atomic multi-selection,
  stable authored metadata, and one-step undo.
- Source-origin rotation and scale pivots, including off-center mesh bounds.
- Per-object Creative translation, Euler rotation, and non-uniform scale.
- Deterministic content hash and one-load process cache.

Textured primitives use their Blender base-color texture multiplied by the
material base-color factor and deterministic face shading. Untextured objects,
missing textures, unsupported texture coordinates, decode failures, and GPU
texture failures retain the vertex-color fallback instead of disappearing or
preventing Creative from starting. Texture descriptors are selected per mesh
primitive; no decoding, image allocation, descriptor creation, or texture
upload occurs in the frame loop.

## Explicitly deferred

- Mipmap generation, anisotropic filtering, and cross-run cooked texture
  caches.
- Normal, occlusion, metallic-roughness, and emissive texture evaluation.
- Alpha blend/mask materials.
- Convex-hull and triangle-mesh collision cooking.
- Sockets and attachment points from glTF extras.
- Skinning, armatures, morph targets, animation clips, and character graphs.
- Offline cooked mesh packages and cross-run derived-data caching.

Static import rejects skins, morph targets, non-triangle primitives, malformed
accessors, unsafe asset IDs, non-finite geometry, and degenerate three-axis
bounds. Do not silently convert those cases to boxes.

## Blender custom properties

Blender custom properties exported into glTF `extras` are scanned at the glTF
document, asset, node, and mesh levels. Identical repeated values are accepted;
conflicting values, wrong types, oversized extras, and impossible combinations
are marked invalid. Invalid metadata does not hide geometry, but RoomBake emits
no collision for that asset.

| Property | Values | Runtime behavior |
|---|---|---|
| `iggy_category` | bounded ASCII identifier such as `boulder`, `walkway`, `prop` | Catalog object-kind classification, with filename fallback |
| `iggy_collision` | `bounds`, `none`, `convex`, `mesh` | Bounds and none are live; convex and mesh are visibly unsupported |
| `iggy_walkable` | boolean | `true` adds a top walkable surface only when collision is bounds |
| `iggy_socket_*` | local transform | Reserved for a future attachment/socket cooker |

Authoring metadata belongs to the imported asset catalog, not the Creative
document schema. Saved objects continue to store only the stable `assetId`, so
this feature does not add persistence fields or migrate existing maps. The
existing authored bounds plus transform position already encode the source
bounds and pivot relationship.

## Fixture and regeneration

The repository carries two end-to-end fixtures:

- `assets/creative/boulder_01.glb`: low-poly 18-vertex, 32-triangle boulder.
- `assets/creative/walkway_stone_01.glb`: four-slab, 32-vertex modular walkway
  with `TEXCOORD_0` and an embedded 4x4 PNG checker material.

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
- stb_image PNG/JPEG decoder, vendored at a pinned commit under public
  domain/MIT terms:
  https://github.com/nothings/stb
