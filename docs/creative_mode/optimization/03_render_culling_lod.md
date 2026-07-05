# 03 — Render Path: Draw Adapter, Culling, LOD Sockets

> ↔ research `03_visibility_render_lighting_audio.md` (its lighting/audio halves
> move to doc 04 — they are gameplay bridges here). Lane: **[C]**.
> Rides **W8b**: `adapters/Draw` (0-byte reserved socket) — "CreativeDocument →
> draw-list items". Foundation plan L5 rules the first visual: **wireframe/box
> primitives per object bounds — proof, not final art.**
>
> Honesty first: iggy3d creative objects have NO mesh/material identity today.
> The research doc's instancing/LOD machinery is therefore SOCKETS here — laws
> and key shapes stated now (cheap), implementation metric-gated later (OP6).
> Do not let a worker build an instancing system for wireframe boxes.

## Entry criteria

- W8a (doc 02) landed: the draw adapter reads the occupancy index + document,
  never scans raw objects per frame.
- W4 (doc 05) landed: rebuilds driven by the Render/Preview dirty channel drain.
- W1w landed: renderer parity law (LAW-14) and coordinate law (LAW-1) repairs —
  the adapter must not attach to the broken seams.
- W7 (entry) for the literal boxes-on-screen moment — the adapter itself is
  unit-gated BEFORE that (foundation plan W8 note: "may run before W6/W7").

## Frozen interfaces

```cpp
// creative/adapters/Draw.hpp (socket comes alive)

struct CreativeDrawItem {
    CreativeObjectId id = 0;
    CreativeDrawItemKind kind;      // WireBox first; WireLine (link profile),
                                    // Handle (point profile). Mesh kinds RESERVED.
    CreativeAabb3 bounds;           // world space
    std::uint32_t flags = 0;        // Selected, Hidden(never emitted—see law),
                                    // Locked, GhostPreview
};

struct CreativeDrawList {
    std::vector<CreativeDrawItem> items;   // sorted by id — stable output
    std::uint64_t documentRevision = 0;    // provenance stamp
};

struct CreativeDrawBuildReceipt {
    std::uint32_t objectsConsidered = 0;
    std::uint32_t itemsEmitted = 0;
    std::uint32_t culledInvisible = 0;     // object.visible == false
    std::uint32_t culledDomain = 0;        // no Render domain
    std::uint32_t culledDistance = 0;      // when distance culling activates
    std::uint64_t documentRevision = 0;
};

CreativeDrawList buildCreativeDrawList(const CreativeDocument&,
                                       const CreativeOccupancyIndex&,
                                       const CreativeDrawRequest&,   // camera pos, maxDistance
                                       CreativeDrawBuildReceipt&);
```

Laws applied:

- `object.visible == false` → no item, counted (`culledInvisible`). Visibility
  is already a real persisted field (§7 R6); this adapter is its first real
  renderer consumer.
- Items carry ids and copies, never pointers into the document (research 03's
  proxy rule — survives here even for wireframes).
- Output sorted by id; rebuilds are diffable in tests.

## Culling stack (scaled to what exists)

Research 03's eight-layer stack, pruned to the layers that have inputs today:

1. **Domain filter** — descriptor `bakeDomains & Render` (doc 01).
2. **Visibility flag** — document truth.
3. **Distance culling** — squared distance from camera to bounds center vs
   `maxDrawDistance²` (no sqrt). Camera position source: the creative fly
   camera (`map_maker/CreativeFly`) — RECON-03a anchors it. First value:
   maxDistance 0 = disabled (draw all); the constant exists so the metric can
   prove when it starts paying.
4. **Frustum culling** — SOCKET. Requires camera basis+fov anchors
   (RECON-03a). When built: p-vertex/n-vertex AABB vs 6 planes, no SIMD until
   measured. NOT in the first slice.
5. **Room/portal visibility** — NOT HERE. Multiroom is a different stream
   (`docs/multiroom_connectivity_design.md`); revisit only when it lands.
6. **LOD / instancing / static batching** — SOCKETS (below), OP6, metric-gated.

Order is cheapest-rejection-first; each layer's rejects are receipted (the
receipt IS the proof the layer earns its place — a layer whose reject count
stays ~0 in real use gets deleted, by law LAW-20's spirit).

## Renderer integration (the parity law, LAW-14)

The existing creative UI overlay attaches only in the Vulkan path
(`FramePresenter.cpp:762-768`); the SDL path draws nothing while input still
routes (confirmed finding F4). The draw adapter output MUST integrate through a
single choke point that BOTH renderers consume, or must gate creative mode off
for renderers that cannot draw it. The W1w order owns the repair; this packet's
wiring slice attaches AFTER it and adds the test: **draw list emitted ⇒ every
active renderer path consumed it** (assert via receipt fields, both configs —
Mac Vulkan-ON, box Vulkan-OFF).

## LOD + instancing sockets (laws now, code later — OP6)

Stated now so future meshes inherit laws instead of inventing:

- LOD selection: one metric (camera distance to bounds center) with
  **hysteresis** (LAW-15): band edges overlap by `kLodHysteresis` (first value:
  10% of band width); an object keeps its current band until it exits the
  overlapped edge. Bands sorted, validated, overlapping-beyond-hysteresis
  invalid, hidden band valid and emits nothing.
- Instance key: `(meshId, materialId, lodBand)` — never object id. Batches are
  contiguous transform arrays rebuilt from the VISIBLE set only.
- Static batching happens at bake, keyed by material, never per frame.
- All three wait for: creative mesh identity (Blockout/palette lineage — does
  not exist), and the `culledDistance`/draw-count metrics proving box drawing
  is actually hot. Until then this section is contract, not backlog.

## Golden cases

1. Document: Room bounds [0,0,0]→[4,3,4] visible, Crate [1,0,1]→[2,1,2]
   hidden, Note at (8,0,8). Build with maxDistance 0 → items: Room WireBox +
   Note Handle. Receipt: considered 3, emitted 2, culledInvisible 1,
   culledDomain 0.
2. Same, camera (0,1,0), maxDistance 5 → Note center (8,*,8) rejected:
   culledDistance 1, emitted 1.
3. Revision provenance: mutate visibility, drain, rebuild → new list's
   `documentRevision` == document revision after mutation; old list unequal.
   (Pins double-buffering discipline, LAW-12.)

## Slices

- **S1 — adapter core.** `buildCreativeDrawList` layers 1–3, receipts, golden
  1–2. Files: `creative/adapters/Draw.{hpp,cpp}` + `creative_draw_adapter_tests`.
  Unit-gated; no renderer wiring. Forbidden: FramePresenter, window/*.
- **S2 — dirty-driven rebuild.** Rebuild on Render/Preview channel drain only;
  golden 3; stats into the receipt pattern. Needs doc 05.
- **S3 — renderer wiring.** Post-W1w, post-W7: single choke point, both
  renderer paths, the parity test, boxes on screen. Router-heavy; smallest
  possible diff (mirrors the W7 discipline).

## Metrics

itemsEmitted, per-layer rejects, rebuildCount vs drainCount (proves dirty
gating), rebuild time (measured only — no estimates), draw items on screen vs
document object count.

## Non-goals

Shaders, Vulkan command recording, meshes/materials, occlusion, room/portal
visibility, lighting (doc 04), final art. The wireframe box IS the deliverable.

## RECON items

- RECON-03a: creative fly camera anchors — exact position/orientation/fov
  state location (`map_maker/CreativeFly.{hpp,cpp}`) and whether a reusable
  ray/frustum type exists there (shared with RECON-02c) — `observed`.
- RECON-03b: the draw-list primitive vocabulary the renderers already consume
  (menu/HUD primitives, `CreativeUiDrawList` text rows) — can WireBox ride an
  existing primitive kind or does the Vulkan path need a line-list primitive
  added? Exact types + file:line — `observed`; if a new primitive is needed,
  that is a named [C] sub-slice with its own overlay test, not an improvisation.
- RECON-03c: current FramePresenter choke points at HEAD post-W1w (F4 repair
  may have moved lines) — `observed`.
