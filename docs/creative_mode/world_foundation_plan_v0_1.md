# Creative World Foundation — plan v0.2 (the "down" layer)

> Planner-authored 2026-07-02. v0.1 was grounded in a 4-reader recon of the as-built creative
> lane at merge `465a3821` (+ Codex's in-flight WIP diff); v0.2 folds in a 3-lens adversarial
> critique (grounding / doctrine / executability) that killed four blockers. This is the
> contract for everything UNDER object creation: world lifecycle, world math, snapping,
> projection, persistence. The nouns / verbs / descriptors / receipts (the "top") already
> exist — this plan gives them a floor.
>
> Method: put the cathedral into the stone. This file is the shape; the work orders at the end
> are the CNC paths. TWO machines cut here — Codex owns `src/app/iggy3d/creative/**`; the
> Claude box fleet owns the product/runtime seams (save, catalog, menu, router). Every work
> order declares its lanes so the human's per-lane commit discipline survives. The human owns
> git; nothing here asks any machine to remember what this file can say.

---

## 0. What is true today (recon summary — do not rediscover)

- **Nouns/descriptors BUILT:** `CreativeObjectKind` — 107 kinds + Unknown (WIP grows the
  descriptor table 96 → 108 rows). `CreativeObject` record; `kDescriptors` table (category,
  profile, creationDirtyFlags, defaults, capabilities); `describeObject()` linear scan.
  All counts in this doc are indicative — GATES MUST BE STRUCTURAL ("every non-Unknown kind
  has exactly one row"), never numeric literals.
- **Verbs/receipts BUILT (untested):** ~70 `CreativeMutationKind` verbs, single apply machinery,
  opt-in = `allowedMutations()` family switch AND descriptor gates. Receipts:
  `CreativeMutationApplyReceipt` / `CreativeDocumentMutationReceipt` with `dirtyFlags` payload.
  NOTE: the grammar has NO create-object or remove-object verbs; `applyDocumentMutation` only
  mutates an EXISTING object. Apply-layer exact-double no-op semantics are currently pinned by
  NO test (creative_room_tests pins only rename/revision no-ops).
- **Document PARTIAL:** name+revision+object storage only. **World-less and id-less.** Sleeper
  comments reserve: `CreativeDocumentId`, `CreativeUnits`, `CreativeGridSettings`,
  `CreativeSnapSettings`, `worldBounds`, layers, `setObjectTransform/Bounds`.
- **Spatial WIP (Codex, in flight):** `SpatialProjection.{hpp,cpp}` — profiles
  `{NoProjection, Point, Box, Volume, Line, Link}`, occupancy kinds, corner-anchored
  `floor(world/cellSize)` grid math with **per-call** cellSize (no owned grid). `Snap.*`,
  `Ghost/Select/Measure/Tools/Ui/Inspect`, all three `adapters/*` = 0-byte reserved slots.
- **Entry PARTIAL:** creative mode today = fly-toggle inside live gameplay
  (`ProductInteractionMode::Creative`, MapMakerToggle). No creative screen, no world.
- **Persistence ABSENT (creative) / BUILT (product):** zero creative serialization. Product side
  has the full parity target: `world/Creation.hpp` (request → template → initial durable save →
  receipt), `SaveBridge` (scan/write-durable/soft-delete/recover, identity carry-forward,
  `nextProductWorldId` mint discipline), consolidated catalog (sd1–sd6 series, 2026-07-02).
  CAVEAT: the durable-write spine is session-mandatory today (`writeSessionSaveFile` fails
  `save_state_missing` on null state) — see D2b.
- **Render ABSENT:** no creative object is ever drawn. Real path =
  `buildProductPrimitiveDrawList(...)` fed by ProjectionRefresh; adapters are the reserved seam.

### Known diseases the foundation must not build on
1. **Two edit paths:** legacy `Facade::createRoom/renameObject/removeObject` (bool, no receipt,
   ignores locked) vs the receipt pipeline (`applyDocumentMutation`). Facade uses the legacy one.
2. **False receipts:** ~46 sleeper verbs — the text/string-id, non-dimension-scalar, link,
   socket, color, audio, and reference-source families — return `Applied, changed=true` + dirty
   flags while **writing nothing** (`CreativeObject` has no fields for them). Plus one
   wrong-write: `SetLength` and `SetDepth` BOTH write the z axis.
3. **Two id spaces:** `Core.hpp Id` (uint32, selection) vs `CreativeObjectId` (uint64, document).
4. **Descriptor-vs-record divergence:** e.g. Room descriptor `hasTransform=false` yet the record
   stores one. Authority undefined.
5. **Parallel per-kind tables:** adding one kind touches 4+ hand-maintained switches
   (`toString`/category, `kDescriptors`, `allowedMutations`, `projectionProfileForObject` +
   `occupancyKindForObject`). The descriptor header calls itself "the CNC file" — make that true,
   never add a 5th switch.
6. **Four coordinate conventions** in the repo: creative corner-anchored `floor(p/cell)` /
   ascii CENTERED `(col-(w-1)/2)*tile` / room_editor center-on-integer `lround` /
   map_maker float pitch lattice. Half-cell ghosts live in the gaps between them.
7. **Dirty flags emitted into the void:** receipts carry them; nothing accumulates or drains
   them. AND the as-built `dirtyFlagsForMutation` emits Geometry|Collision only for
   Structural/TerrainVolume kinds — a moved Light/Trigger/Marker wakes nothing (see L4).

---

## 1. Decisions (planner-taken; overridable; each is load-bearing)

**D1 — CreativeDocument is the authored truth of creative mode.**
`docs/creative_mode/product_spec.md` §8 ("creative is a facade over room_editor, no second
command system") is **superseded for the world layer** by this plan. The built noun/verb/
descriptor/receipt stack IS the creative command system. room_editor remains the shipped
product editor, untouched and unbroken; it is a sibling, not the kernel under creative worlds.
Consequence: spec §8 gets a v2 amendment note (work order W0) so the repo carries one truth.
*Alternative rejected:* wrapping room_editor's `EditableRoomDocument` — it would discard the
descriptor/mutation grammar the whole method is built on.

**D2 — One registry, kind-tagged. Creative worlds ride the product save service.**
Creative documents persist in the SAME `saveRoot` catalog through the SAME durable temp-write /
validate / commit / soft-delete / recover discipline (just consolidated in sd1–sd6).
`SaveEnvelope` gains a `SaveCreativeDocumentSection` sibling of `SaveAuthoredRoomSection`;
schema version bumps 1 → 2 with an explicit compat rule (v1 decodes fine, section absent).
Catalog `contentKind` (`session` | `creative`) is **derived from section presence at scan
time** — no second stored flag that could diverge from the payload. Product Continue/Load
filter creative out (`canLoadProductSave`); the creative browser filters to it. Identity = the
existing `nextProductWorldId` mint + carry-forward rules.
*Alternative rejected:* a separate creative store directory — a second registry is the exact
disease sd1–sd6 just cured. Note: spec §10's old "no save schema change" constraint applied to
its first UI slices; it expires here, deliberately, with a versioned schema.

**D2b — A state-less creative write entry point.** The durable spine requires a `SessionState`
today (`save_state_missing`). Creative saves get `writeCreativeDocumentSaveDurably(...)` that
shares the SAME temp-write/fsync/validate/commit machinery but builds an envelope whose session
sections are default/absent — no synthetic session lies. Catalog loadable/compatible derivation
for `contentKind=creative` entries must not depend on session decode.

**D2c — Identity correspondence.** `CreativeDocumentId` (uint64) = the numeric core of the
minted `worldId` string (`world_0007` → documentId 7). One mint discipline
(`nextProductWorldId` scans active+deleted; formatWorldId guarantees per-saveRoot uniqueness),
carried forward on re-save exactly like worldId. Invariant pinned by test: worldId and
documentId always correspond.

**D3 — THE grid: corner-anchored, document-owned, cubic cells.**
One coordinate truth for creative space, matching Codex's WIP math:
cell `(x,y,z)` spans `[origin + i*cellSize, origin + (i+1)*cellSize)` per axis; centers are
derived `(i+0.5)`; vertical is cells too (y-index = `floor((worldY-originY)/cellSize)`).
`cellSize` and `origin` live ON the document (`CreativeGridSettings`), never as per-call
parameters — `SpatialProjection` requests are built FROM the document's grid. The grid
primitive types (`CreativeGridSize3`, `CreativeGridCoord3`, `CreativeGridBounds3`, and the pure
coord math) **migrate DOWN into the L0 world-math header**; SpatialProjection.hpp includes it,
never the reverse. The other three conventions stay where they are; converting between
universes happens only in named bridge files with explicit converter functions, never inline.

**D4 — Snap at the tool boundary, never inside apply.**
`applyMutation` stays exact: it applies precisely what the request says. Snapping is
intent-shaping that happens BEFORE the request is built — pure functions in `Snap.{hpp,cpp}`
(the reserved slots) reading `CreativeSnapSettings` from the document. This keeps receipts
truthful, keeps apply-layer exact-double no-op semantics stable (currently UNPINNED — W1 pins
them), and means headless/scripted mutation (the AI-authoring path) can bypass or apply snap
explicitly.

**D5 — Repairs before construction.**
No new floor is poured over false receipts and forked verb paths. W1 fixes the receipt lies and
the id/authority diseases BEFORE the world types land; receipted create/remove machinery lands
with W3 (where the creation grammar lives), and Facade's legacy path is retired there.

**D6 — Descriptor table is the single per-kind truth; new facts become columns.**
The projection profile and occupancy kind (currently switches in SpatialProjection.cpp) migrate
into `CreativeObjectDescriptor` as columns **in W3** (the table slice); `allowedMutations`
families become derivable from descriptor profile/capabilities over time. Rule for all future
work: **a new per-kind fact is a descriptor column, never a new switch.** Table invariants get
their own test (every non-Unknown kind has exactly one row; profile ↔ capability coherence;
creation flags coherent with category).

---

## 2. The layer map (bottom → top)

```
L0  World math        CreativeUnits, CreativeGridSettings, CreativeSnapSettings, worldBounds,
                      grid primitive types + Snap.{hpp,cpp}    (inhabits the sleeper names)
L1  Document identity CreativeDocumentId, createdAtUtc/modifiedAtUtc, dirty accumulator
L2  Serialization     runtime-side SaveCreativeDocumentSection + app-side converter (envelope v2)
L3  World lifecycle   CreativeWorldService: create clean / open existing / save / list
                      (riding SaveBridge + catalog, parity with world/Creation.hpp)
L4  Projection        document-grid-fed SpatialProjection + occupancy index + dirty-driven rebuild
L5  Entry + render    menu asks for a world and receives a result; adapters/Draw feeds the
                      draw list; grid overlay binds to THE grid
```

Everything above L5 (toolbelt, ghost placement, selection UI, inspector) is the "top" — already
planned in product_spec §5–§7 and out of scope here except where it consumes these contracts.

### L0 — World math contract
New `creative/WorldMath.hpp` (or Codex's preferred home INSIDE creative/ — the NAMES are fixed
by the Document sleepers):
- `CreativeUnits { Meters }` (explicit, so nothing ever guesses).
- `CreativeGridSettings { double cellSizeMeters = 1.0; CreativeVec3 origin{}; CreativeGridSize3 size; }`
- `CreativeSnapSettings { bool enabled = true; double positionIncrement = 1.0; double rotationIncrementDegrees = 90.0; double sizeIncrement = 1.0; }`
- `worldBounds : CreativeBounds` (authoring envelope; projection clamps to it).
- Grid primitives migrated down from SpatialProjection.hpp (D3).
Snap functions (pure, tested): `snapPosition(v, settings)`, `snapSize`, `snapRotationDegrees`,
`snapTransform`, `snapBounds` — floor/round rules stated in the header, corner-anchor
consistent with D3. Document gains the sleeper fields + accessors; `reset()` performs the
already-commented reset duties.

### L1 — Document identity + dirty accumulator
- `CreativeDocumentId` (uint64) activated; `0` invalid. The document does NOT mint — the world
  service assigns per D2c; the document stores it.
- `createdAtUtc` / `modifiedAtUtc` strings (same clock discipline as `productSaveTimestampNowUtc`).
- **Dirty accumulator (selective awakening):** `CreativeDirtyState` on the document —
  `accumulate(dirtyFlags)` called by the mutation pipeline (receipts stop vanishing into the
  void), `peek(channel)` / `drain(channel)` for consumers (serialization drains Serialization,
  projection drains its spatial set per L4, preview drains Preview). Object-id reuse disease:
  `nextObjectId_` no longer resets to 1 on `reset()` when the document carries an id, and it
  SERIALIZES (see L2) so persistence keying on `documentId + objectId` holds across lifecycles.

### L2 — Serialization contract
- **Layering rule (hard):** the section lives in `src/runtime/save` as primitive record structs
  (kind as stable STRING, plain doubles) mirroring the `SaveAuthoredRoomSection` precedent;
  the `CreativeDocument ↔ section` converter lives APP-side (creative/ or app/iggy3d/save/).
  `runtime/save` never includes app headers.
- `SaveCreativeDocumentSection { present, version, documentId, name, units, gridSettings,
  snapSettings, worldBounds, nextObjectId, objects[] }` — object record: id, kind-string, name,
  transform, bounds, layerId, visible, locked, tags, parentId. `nextObjectId` serializes (the
  `SaveWorldSection.nextEntityId` precedent) — never recomputed as max+1. Document `revision`
  is NOT serialized: a decoded document starts at revision 0 with a clean dirty state; the
  round-trip equality contract explicitly excludes it.
- **Lossless doubles:** the creative section encodes doubles losslessly
  (`std::to_chars`/max_digits10 class encoding), NOT the existing `formatFloat`
  fixed-3-decimals path — unsnapped/AI-authored coordinates must round-trip exactly. Pinned
  with a non-3-decimal-representable value.
- `kSaveSchemaVersion` 1 → 2. Compat: v1 envelopes decode with `section.present=false`; v2
  decoders never require the section. `SaveCompatibility` gains the one rule.
- Round-trip receipt: encode → decode → structural equality (revision excluded) + object count
  + content hash; pinned by unit tests.

### L3 — World lifecycle contract (the seam the menu talks to)
Parity with `world/Creation.hpp`, creative-flavored. The CONTRACT is fixed; placement splits by
lane: the service orchestration + document construction is creative-lane; the SaveBridge/catalog
touches are product-lane (see work-order lane map):
- `CreativeWorldCreateRequest { title, templateId /* "empty" first */, gridSettings?, requestedAtUtc, saveRoot }`
- `createCreativeWorld(request) -> CreativeWorldCreationResult { accepted, status, reasonCode,
  documentId, worldId, initialSaveWritten, saveId, receipt fields... }`
  — mints `worldId`/`documentId` per D2c, builds the document from the template (EMPTY =
  **zero authored objects** + default grid + default worldBounds), writes the initial durable
  save immediately via D2b (no orphan ghosts — a world exists iff the registry knows it),
  returns a receipt.
- `openCreativeWorld(saveRoot, saveId) -> CreativeWorldOpenResult { document, receipt }`
  — decode, validate section present, identity carried forward.
- `saveCreativeWorld(document, ...) -> receipt` — identity carry-forward, durable write, drains
  the Serialization dirty channel.
- The menu NEVER constructs worlds. It asks; it receives a result; it routes on `accepted`.
Catalog: `contentKind` derived at scan (D2); `canLoadProductSave` excludes creative entries; a
`canOpenCreativeWorld` predicate gates the creative browser and is wired into the live
count/browse path THE SAME SLICE it lands — no decorative predicates (lesson of sd4/sd6).

### L4 — Projection contract (engine-facing derivation)
- `CreativeSpatialProjectionRequest` is BUILT FROM the document (`makeProjectionRequest(doc)`)
  — cellSize/origin/size come from `CreativeGridSettings`; the per-call-cellSize hole closes.
- Profile + occupancy are descriptor columns (D6, landed in W3); the SpatialProjection.cpp
  switches become table reads.
- New second stage: `CreativeOccupancyIndex` — per-channel (occupancy kind) dedup'd cell → object
  index over the flat cell lists, the thing selection/collision/preview actually query.
- **Dirty→channel map (contract, closes disease 7):** any kind whose descriptor profile
  projects (`profile != NoProjection`) emits the spatial dirty flag matching ITS occupancy
  channel on transform/shape mutations — `dirtyFlagsForMutation` derives this from the
  descriptor columns instead of the Structural/TerrainVolume special-case. The rebuild drains
  exactly the dirtied channels. Gate tests BOTH directions: rename a Note → no spatial channel
  wakes; move a Light → Light channel rebuilds, Structural channel untouched.
- Link/Line profiles remain sleepers until `CreativeObject` grows endpoint data — declared out
  of scope here; the profile enum keeps the slots.

### L5 — Entry + render seam (last, thinnest)
- Starter → New World flow gains a creative template choice (or dev-tools entry first — user's
  taste; the WorldSetup seam already routes `CreateAndEnter`). Entering a creative world =
  `createCreativeWorld` (or `openCreativeWorld`) → session boots the clean-world STAGE — the
  ground plane is **session-stage furniture from the product template lineage
  (world/DefaultWorldTemplate), NOT an authored CreativeObject**; the creative document stays
  at zero objects until the user creates one → `interactionMode = Creative` → map_maker grid
  overlay binds its pitch/anchor to THE grid (one truth drawn, not a fourth grid).
- `adapters/Draw.{hpp,cpp}` (reserved slot): `CreativeDocument` → draw-list items. First slice:
  wireframe/box primitives per object bounds — proof, not final art. `adapters/RoomEd` stays
  reserved for a later bake bridge (creative → RoomAsset / `SpatialSurfaceSet` via the
  occupancy index) — contract named, not built.
- Surface-resolution note: creative-fly vs Editor surface exclusivity
  (`FrontendRouter.cpp` BG-1058) and the ProductAppWindowState receipt fields this touches make
  the entry slice the ROUTER-heaviest one; it is deliberately last, deliberately small, and
  product-lane (the fleet that owns those files executes it).

---

## 3. Receipts (what "it happened" looks like, per layer)

Every layer emits receipt-grade facts (window/receipt names indicative):
- Creation: `creative_world_created`, `creative_world_id`, `creative_initial_save_written`.
- Open: `creative_world_opened`, `creative_document_id`, `creative_object_count`.
- Save: `creative_save_written`, `creative_save_id`, `creative_dirty_drained=serialization`.
- Snap: (tool-layer) snapped-from/to in the mutation request trail, not in apply receipts.
- Projection: `creative_projection_cells`, `creative_projection_rebuilt_channels`.
Receipts never lie: a sleeper verb reports NotSupported; a no-op reports changed=false; a verb
writes exactly the field its kind names; a rebuild that skipped clean channels says so.

## 4. Boundaries (what this plan refuses)

- No undo/redo design (receipts lack before-values; separate future contract).
- No link/route endpoint data-model growth (blocks Link projection; noted, deferred).
- No merging of the four grid universes — creative gets ONE truth; ascii/room_editor/map_maker
  keep theirs; bridges are explicit, named, and deferred except the grid-overlay binding in L5.
- No multi-room/world-graph anything (see multiroom_connectivity_design.md — different stream).
- No render polish; the first draw adapter emits boxes.
- room_editor and its tests remain untouched and green throughout.

## 5. Work orders (CNC paths — one per slice, in order)

> Each order = LANES (who cuts what) + scope files + forbidden files + gate + receipt.
> **Gate = full `ctest --test-dir build` green on the Mac — structural, never a hard-coded
> total (the count grows per order).** Box-config (GCC/Vulkan-OFF) parity is a fleet/human
> post-order check — GCC's stricter includes are a known hazard. The human commits per lane;
> no order may mix lanes in one commit.
> Lanes: **[C] = Codex** (`src/app/iggy3d/creative/**` + its tests/cmake rows).
> **[P] = Claude box fleet** (product/runtime lanes: `src/runtime/save`, `src/app/iggy3d/save`,
> `src/app/iggy3d/menu`, `src/app/iggy3d/world`, window/router/receipt files).

- **W0 [C] — Land the in-flight WIP + amend the spec.** Scope, honestly: (a) the compile wiring
  + include canonicalization + revision bridge + vocabulary growth (SetCheckpointId, descriptor
  96→108) already in the diff; (b) NET-NEW deliverables: first test targets
  `creative_object_descriptor_tests` (table invariants), `creative_document_mutation_tests`
  (receipt pipeline incl. exact-double no-op pins), `creative_spatial_projection_tests` (grid
  math); (c) the D1 supersession note in product_spec §8. HUMAN NOTE: the untracked
  `ascii_room/AsciiRoomCanvas.*` feature is interleaved in the SAME CMake hunks — land it as
  its own commit or split hunks; it is not part of this plan.
- **W1 [C] — Truthful receipts + repairs.** Sleeper verb families (text/string-id,
  non-dimension scalars, link, socket, color, audio, reference-source) → NotSupported-class
  status; fix the SetLength/SetDepth shared-axis wrong-write (per-axis pins); RENAME (and
  visible/lock/tag verbs) route through `applyDocumentMutation` in Facade; selection `TargetRef`
  adopts `CreativeObjectId`; Room descriptor/record authority fixed. Gate: no verb returns
  changed=true without writing the field its kind names.
- **W2 [C] — World math.** L0 types + grid-primitive migration down from SpatialProjection.hpp
  + document fields + `Snap.{hpp,cpp}` + `creative_snap_tests` (the 0-byte slots come alive).
  One coordinate truth documented in the header.
- **W3 [C] — Generic creation + the table becomes the CNC file.** Receipted
  `createDocumentObject(kind)` / `removeDocumentObject(id)` document-level entry points
  (creation receipt + creationDirtyFlags consumed; descriptor defaults instantiate every
  non-Unknown kind); Facade's legacy create/remove path retired (creative_room_tests updated
  deliberately); projection profile + occupancy kind become descriptor COLUMNS (D6) and the
  SpatialProjection switches become table reads; table-invariant test lands.
- **W4 [C] — Identity + dirty accumulator.** L1 complete; mutation pipeline accumulates;
  drain API; `dirtyFlagsForMutation` derives spatial flags from descriptor columns (L4 map);
  selective-awakening tests in BOTH directions (Note rename wakes nothing spatial; Light move
  wakes exactly the Light channel).
- **W5 [P runtime + C converter] — Serialization.** [P]: `SaveCreativeDocumentSection` record
  structs + codec (lossless doubles) + schema v2 + compat rule in `src/runtime/save`.
  [C]: the app-side `CreativeDocument ↔ section` converter + round-trip pins (revision
  excluded; nextObjectId round-trips; non-3-decimal double pin). **Needs W1+W2+W4.**
- **W6 [P service/catalog + C document construction] — World service.** [P]: D2b state-less
  durable write entry, scan-time `contentKind` derivation, `canLoadProductSave` exclusion +
  `canOpenCreativeWorld` wired live, `CreativeWorldService` create/open/save orchestration
  riding SaveBridge with D2c identity. [C]: template → document construction (EMPTY = zero
  objects). No-orphan discipline pinned. **Needs W5.** SEQUENCING: coordinate with the
  save-lane owner — this lane was just consolidated (sd1–sd6); pins live in the sd slice docs.
- **W7 [P] — Entry.** Starter New-World creative row (or dev-tools door), routing, clean-world
  stage furniture (product template lineage), `interactionMode=Creative` on entry, grid-overlay
  binding to THE grid, receipt fields for the new surface. Router-heaviest; smallest possible
  diff. **Needs W6.** SEQUENCING: the starter new_world emitter is queued for the byte-identical
  widget migration (ui stream) — land this after/with it, not against it.
- **W8 [C] — Projection index + draw adapter (unit-gated).** L4 `CreativeOccupancyIndex` +
  dirty-driven selective rebuild + `adapters/Draw` document → draw-list items. Gate is
  UNIT-LEVEL (index correctness, selective rebuild both-direction tests, draw-item golden) —
  **needs W2+W4 only, may run before W6/W7.** The literal boxes-on-screen moment is a one-line
  follow-up once W7's entry exists.

## 6. Open decisions for the user (small, non-blocking)

1. L5 entry point taste: starter New-World template row vs dev-tools door first (plan assumes
   starter row; either fits the seam).
2. `contentKind` naming (`creative` vs `blueprint` vs `workshop`) — pure vocabulary.
3. Whether W0's spec amendment retires product_spec §6–§7 file names (Input/Placement/Snap/…)
   in favor of the reserved creative/* slots, or keeps both vocabularies until the toolbelt
   stream starts.
