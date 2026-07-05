# 05 — Dirty Channels And The Bake DAG

> ↔ research `05_dirty_bake_pipeline.md`. Lane: **[C]**. This IS the detailed
> spec for **W4** ("identity + dirty accumulator; drain API; selective
> awakening") extended with the bake-ordering laws the later packets need.
> The foundation plan owns the order slot; this doc owns the exact shapes.
>
> What already exists: per-mutation flags via `dirtyFlagsForMutation(objectKind,
> mutationKind)` (`ObjectDescriptor.cpp` ~:356) returned inside mutation
> receipts. What does NOT exist: anything that ACCUMULATES them between frames,
> or any consumer that drains them. Flags are currently computed and dropped.
> (Also: the review found aliased verbs computing flags for the WRONG kind —
> P1, fix in flight under W1. This packet builds on repaired flags only.)

## The single-writer law (LAW-10)

`applyDocumentMutation` is the ONLY place that marks dirty. Not the Facade, not
tools, not UI, not the index. Systems DRAIN channels; they never poll document
revision (the interim revision-poll in doc 02 S6 is deleted by this packet's
S3). A grep for writes to the accumulator outside the mutation pipeline is a
test (yes, literally — a source-scan test, same discipline as the repo's
include-hygiene pins).

## Frozen interfaces

```cpp
// creative/DirtyState.hpp (new)

// Channels — reuse the EXISTING CreativeDirtyFlags enumerators (Identity,
// Preview, Serialization, Transform, Geometry, Collision, Navigation,
// Gameplay, ...). RECON-05a pins the exact list at HEAD. No parallel enum —
// research's CreativeDirtyDomain names map onto these (LAW: one flag family;
// doc 01 invariant 7 is the merge-guard).

struct CreativeDirtyObjectRecord {
    CreativeObjectId id;
    CreativeDirtyFlags flags;        // union of this object's un-drained changes
};

class CreativeDirtyState {
public:
    // writer surface — mutation pipeline only (LAW-10)
    void mark(CreativeObjectId, CreativeDirtyFlags);
    void markAll(CreativeDirtyFlags);              // global (load, template init)

    // reader surface — one consumer per channel set
    CreativeDirtyDrainReceipt drain(CreativeDirtyFlags channels,
                                    std::vector<CreativeDirtyObjectRecord>& out);
    [[nodiscard]] bool anyDirty(CreativeDirtyFlags) const;

    [[nodiscard]] const CreativeDirtyStats& stats() const;
};

struct CreativeDirtyDrainReceipt {
    CreativeDirtyFlags requested;
    CreativeDirtyFlags drained;          // subset actually present
    std::uint32_t objectRecords = 0;
    bool globalWasSet = false;           // consumer must full-rebuild
    std::uint64_t documentRevision = 0;  // provenance
};
```

Semantics:

- `drain(channels)` removes EXACTLY those flag bits from every record (and the
  global mask), returning records that had any of them. A record whose flags
  become 0 is dropped. Draining is per-channel-set idempotent: a second
  identical drain returns 0 records.
- Two consumers draining overlapping channel sets is a DESIGN ERROR — the
  channel→consumer ownership table below is law; a debug assert + receipt
  counter (`overlappingDrainSuspected`) guards it (detectable: drain of a
  channel that was drained since last mark).
- Storage: `std::vector<CreativeDirtyObjectRecord>` + `unordered_map<id,index>`
  for O(1) mark; drain compacts. Object count is editor-scale (thousands);
  no cleverness before metrics (LAW-6 spirit).

## Channel → consumer ownership

| Channel(s) | Sole consumer | Consumer doc |
| --- | --- | --- |
| Transform, Geometry (spatial-shaped) | occupancy index update | doc 02 S6 |
| Preview, Render-shaped | draw adapter rebuild | doc 03 S2 |
| Serialization | save/autosave marking (W5/W6 lane) | foundation plan |
| Collision, Navigation, Gameplay | bake bridges (post-W5/W6) | doc 04 |
| Identity | UI projection refresh (existing creative UI model) | existing |

The map from mutation → channels stays DERIVED: `dirtyFlagsForMutation(kind,
mutation)` reads descriptor columns (doc 01) — a new kind gets correct dirty
behavior by filling its descriptor row, zero new switch cases (D6).

## Golden routing table (test-ready; extends the W4 both-direction tests)

| Mutation | Object | Must set | Must NOT set |
| --- | --- | --- | --- |
| Move | Wall | Transform+Geometry+Collision+Navigation+Serialization+Preview | Identity |
| Move | Crate | Transform+Geometry+Collision+Serialization+Preview | Navigation |
| Move | Note | Transform+Serialization+Preview | Geometry, Collision, Navigation, Gameplay |
| SetVisible | Crate | Preview+Serialization (+Render-shaped) | Collision (LAW: visibility is a render fact — doc 04 collision policy) |
| Rename | Room | Identity+Serialization | every spatial/gameplay channel |
| SetSpawnFacing | SpawnPoint | Navigation+Gameplay+Serialization+Preview | Transform-only (the P1 aliased-verb regression pin) |
| AttachTo (no-change) | any | NOTHING (NoChange receipts mark nothing — F12 pin) | all |

Both directions per W4: "Note rename wakes nothing spatial; Light move wakes
exactly the Light channel" — extend with one row per capability domain as
doc 01 fills columns.

## The bake DAG (LAW-11) and rebuild policy (LAW-12/13)

Consumers run in dependency order when multiple channels are dirty in one
drain cycle (the window loop owns the cycle; one drain pass per frame at most):

```
1. occupancy index   (Transform/Geometry)         — everything queries it
2. draw adapter      (Preview/Render)             — reads index + document
3. [future] room assignment                        — reads index
4. [future] collision/nav bakes (doc 04)           — read index (+rooms)
5. [future] light/audio lists                      — read rooms
```

A consumer reads only outputs of earlier stages. Stage list is code (an ordered
table), not convention — adding a consumer means adding a row, and the row
order IS the review surface.

Per consumer, rebuild policy is explicit and receipted:

- records < `kFullRebuildThresholdPct` (first value: 30%) of its population →
  incremental (per-record update);
- above, or `globalWasSet` → full rebuild;
- either way: **double-buffered** (LAW-12) — build aside, swap, receipt carries
  `rebuild_kind=incremental|full` + counts. No consumer ever exposes a
  half-rebuilt structure (the draw list's revision-stamp golden case in doc 03
  pins this from the consumer side).

## Slices

- **S1 — accumulator.** `CreativeDirtyState` + mark from `applyDocumentMutation`
  (single writer; NoChange marks nothing) + the source-scan single-writer test.
  Files: `creative/DirtyState.{hpp,cpp}`, mutation pipeline touch-point,
  `creative_dirty_state_tests`. Forbidden: window/*, adapters.
- **S2 — routing truth.** Golden routing table as tests over the REPAIRED
  `dirtyFlagsForMutation` (post-W1); both-direction selective-awakening pins.
  This slice IS W4's test deliverable — land under the W4 order, cite it.
- **S3 — drain wiring.** Window-loop drain cycle: index update (kills doc 02's
  interim revision-poll — delete it here), draw adapter rebuild, ownership
  table + overlapping-drain guard, DAG order table. Receipts into the stats
  pattern. Needs doc 02 S6 + doc 03 S2 in place to have two real consumers.

Gate per slice: full ctest green; the S2 table runs the full kind × mutation
product for the pinned rows.

## Metrics

marksPerMutation (avg), recordsHighWater, drainsPerFrame, recordsPerDrain,
incremental vs full rebuilds per consumer, skippedCleanDrains (proof the
gating works — research 05's "skipped clean domains"), overlappingDrainSuspected
(must stay 0), serializationDirtyWithoutRuntimeDirty (metadata-only edit proof).

## Non-goals

Undo/redo (mutation history is a separate future lane), persistence of dirty
state (NEVER serialized — derived), cross-document dirt, background/threaded
bakes (single-threaded drain cycle until a measured budget says otherwise).

## RECON items

- RECON-05a: exact `CreativeDirtyFlags` enumerators + `dirtyFlagsForMutation`
  signature at HEAD post-W1 (P1 fix moved receipts; verify flags too) —
  `observed`.
- RECON-05b: where the window loop should host the drain cycle (the frame
  sequencing point after input/mutations, before pick/draw frames — exact
  anchor in `window/Loop.cpp` post-W1w) — `observed`.
- RECON-05c: current UI projection refresh trigger (Identity channel's
  existing consumer — how `buildUiModel` is invoked today) — `observed`.
