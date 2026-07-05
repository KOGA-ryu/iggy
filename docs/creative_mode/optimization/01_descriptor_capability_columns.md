# 01 — Descriptor Capability Columns

> ↔ research `01_object_capability_matrix.md` + the per-category checklists of
> research `07`. Lane: **[C]**. Rides **W3** (the table slice) under **D6**:
> a new per-kind fact is a descriptor column, never a switch.
>
> The matrix already exists in iggy3d — `src/app/iggy3d/creative/ObjectDescriptor.cpp`
> (~108 rows) with `descriptorFor(kind)`, plus the test target
> `creative_object_descriptor_tests` (landed in W0, `cd6b9f4e`). This packet does
> not create a matrix; it adds the capability columns the optimization program
> reads, and retires the switches that duplicate them.

## Entry criteria

- W1 receipt repairs merged (do not pour columns over false receipts — D5).
- W3 either in flight or this packet lands AS W3's column deliverable —
  coordinate with the W3 order; do not fork a second table.
- RECON-01a returned (see §RECON): exact current descriptor fields at HEAD.

## The two vocabularies — do not merge them

iggy3d already has **change channels**: `CreativeDirtyFlags` produced by
`dirtyFlagsForMutation(objectKind, mutationKind)` (`ObjectDescriptor.cpp` ~:356
— Identity/Preview/Serialization/Transform/Navigation/Gameplay/…). Those answer
"*what just changed*".

This packet adds **capability domains**: what shared systems may consume the
object at all. They answer "*where does this object go when baked/indexed*".

```cpp
// creative/ObjectDescriptor.hpp — new, alongside existing columns
enum class CreativeBakeDomain : std::uint32_t {
    Spatial    = 1u << 0u,  // occupancy index / picking / proximity
    Render     = 1u << 1u,  // draw adapter output (wireframe first, meshes later)
    Collision  = 1u << 2u,  // future RoomAsset SpatialSurfaceSet bake (doc 04)
    Navigation = 1u << 3u,  // future reasoning-graph / nav bake (doc 04)
    Trigger    = 1u << 4u,  // future zone bake (doc 04)
    Lighting   = 1u << 5u,  // future light lists (doc 04)
    Audio      = 1u << 6u,  // future audio bake (doc 04)
    Event      = 1u << 7u,  // future event graph (doc 04)
};
using CreativeBakeDomainMask = std::uint32_t;
```

The W4 dirty map (doc 05) derives change channels FROM capability columns ×
mutation kind. One direction only. A worker who finds themselves writing a
`bakeDomains -> dirtyFlags` copy loop in two places has merged the vocabularies;
stop and re-read this section.

## New columns

| Column | Type | Meaning | Source of truth today |
| --- | --- | --- | --- |
| `projectionProfile` | existing enum (point/box/volume/line/link) | how the object projects to cells | SWITCH in `SpatialProjection.cpp` — W3 migrates it here |
| `occupancyKind` | existing enum | what the occupied cell means | second SWITCH in `SpatialProjection.cpp` — W3 migrates |
| `bakeDomains` | `CreativeBakeDomainMask` | which systems may consume the object | NEW |
| `boundsRule` | enum below | how the spatial proxy derives bounds | NEW (doc 02 consumes) |
| `spatiallyIndexed` | bool | participates in the occupancy index | NEW (derivable: `bakeDomains & Spatial`; keep as validation invariant, not a second bit — see invariants) |

```cpp
enum class CreativeBoundsRule : std::uint8_t {
    Bounds,          // use object.bounds verbatim (box/volume kinds)
    BoundsThin,      // bounds expanded by kThinSurfaceEpsilon on the flat axis
    PointHandle,     // cube of kEditorHandleExtent around transform.position
    LineEndpoints,   // min/max of endpoints + handle radius (link profile)
    InfluenceRadius, // future: lights/audio — radius payload, NOT gizmo size
};
```

`kEditorHandleExtent` first value: 0.25 (world units, cubic). `kThinSurfaceEpsilon`
first value: 0.05. Both are named constants in the L0 world-math header (W2),
receipted if repaired (LAW-16: a degenerate zero-extent bounds is REPAIRED to
the handle extent with receipt code `bounds_repaired_degenerate`, not rejected).

## Default policy per category

Fill all ~108 rows by category defaults first, exceptions second (research 01's
reuse rule). First-pass defaults:

| Category (existing grouping in `Object.hpp:35-162`) | bakeDomains default | boundsRule default |
| --- | --- | --- |
| structural (Room, Wall, Floor, Ramp, Platform…) | Spatial\|Render\|Collision\|Navigation | Bounds |
| terrain volumes | Spatial\|Render\|Collision | Bounds |
| nav/logic markers (SpawnPoint, PatrolNode…) | Spatial\|Render\|Navigation | PointHandle |
| GameplayMarker family (EnemySpawn, NpcSpawn, InterestPoint, ResourceNode, LootPoint, QuestMarker, DialogueMarker — `ObjectDescriptor.cpp:211-221`) | Spatial\|Render\|Navigation\|Event | PointHandle |
| trigger/zone kinds (incl. Water/LavaVolume) | Spatial\|Render\|Trigger | Bounds |
| light kinds | Spatial\|Render\|Lighting | PointHandle (InfluenceRadius once payload exists) |
| sound kinds | Spatial\|Render\|Audio | PointHandle (InfluenceRadius later) |
| camera/link kinds | Spatial\|Render | LineEndpoints |
| authoring/testing/dressing | Spatial\|Render | Bounds or PointHandle by shape |
| editor-only (Note, Measurement…) | Spatial only | PointHandle |

`Render` here means "the draw adapter may emit a wireframe/debug item" (doc 03)
— it does NOT imply runtime visibility. Editor-only kinds keep `Spatial` so they
stay pickable (research 07 policy), and the future runtime bake (doc 04) filters
on the specific gameplay domain, never on `Render`.

## Validation invariants (extend `creative_object_descriptor_tests`)

A descriptor row is invalid if:

1. `bakeDomains == 0` (every kind is at least Spatial or explicitly documented).
2. `projectionProfile` is line/link but `boundsRule != LineEndpoints`.
3. `boundsRule == InfluenceRadius` but the kind has no radius payload column.
4. kind is in the editor-only category but `bakeDomains` contains any of
   Collision|Navigation|Trigger|Lighting|Audio|Event.
5. `bakeDomains & Trigger` but occupancyKind is `Unknown` (fixes the
   Water/LavaVolume hole found in review — minors list).
6. Any two rows for the same kind (uniqueness — already pinned by W0 test;
   keep).
7. A `dirtyFlagsForMutation` result claims a channel whose domain the
   descriptor does not carry (cross-check: change channels ⊆ capabilities ∪
   editor channels {Identity, Preview, Serialization}).

Invariant 7 is the merge-guard between the two vocabularies. It runs over the
full kind × mutation-family product in the test — cheap, exhaustive, and it
turns the matrix into executable policy instead of decorative paperwork.

## Golden rows (test-ready)

| Kind | profile | occupancy | bakeDomains | boundsRule |
| --- | --- | --- | --- | --- |
| Room | box | (existing) | Spatial\|Render\|Collision\|Navigation | Bounds |
| Wall | box | (existing) | Spatial\|Render\|Collision\|Navigation | Bounds |
| Crate | box | (existing) | Spatial\|Render\|Collision | Bounds |
| SpawnPoint | point | (existing) | Spatial\|Render\|Navigation | PointHandle |
| Note | point | (existing) | Spatial | PointHandle |

## Slices

- **S1 — columns + bulk fill.** Add columns to the descriptor struct; fill all
  rows via category defaults + a short exceptions list (each exception gets a
  one-line why). Files: `creative/ObjectDescriptor.{hpp,cpp}`. Forbidden:
  `SpatialProjection.cpp` (S3's job), any runtime file.
- **S2 — invariants.** Extend `creative_object_descriptor_tests` with
  invariants 1–7 + the golden rows. Gate: full ctest green.
- **S3 — switch retirement.** `SpatialProjection.cpp` profile/occupancy switches
  become table reads (this IS a W3 deliverable — land under the W3 order, cite
  it). Behavior-identical: pin with a before/after projection equality test over
  every kind at a fixed fixture. Receipt: `projection_source=descriptor`.

## Metrics

| Metric | Purpose |
| --- | --- |
| rows with non-default masks | exception pressure on the category defaults |
| invariant-7 violations at test time | vocabulary-merge attempts caught |

## Non-goals

No new object kinds. No runtime bake code (doc 04). No dirty-map changes
(doc 05). No UI.

## RECON items (answer before S1, evidence-labeled)

- RECON-01a: exact current `CreativeObjectDescriptor` field list at HEAD
  (W0/W1 may have moved it) — `observed` required.
- RECON-01b: exact `CreativeDirtyFlags` enumerator list + which mutations map to
  which flags today (input to invariant 7).
- RECON-01c: does any kind already carry a radius-like payload (lights/sound)?
  If none, `InfluenceRadius` stays a reserved enumerator — do not invent payload
  storage in this packet.
