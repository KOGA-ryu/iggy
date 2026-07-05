# 06 — Ranked Backlog And Worker Packets

> ↔ research `06_ranked_backlog_and_worker_packets.md`. Ordered by leverage and
> dependency, not glamour. Every OP cites its doc, its foundation-plan ride,
> its lane, entry criteria, and definition of done. The planner cuts slices
> from here; workers execute slices, never whole OPs.

## Current state gate (read before cutting anything)

As of 2026-07-05, trunk `cd6b9f4e`:

- **Gate is RED** (plan §8 F16 — `product_creative_ui_projection_tests`,
  run-confirmed). Nothing in this backlog starts before green.
- W0 landed (`cd6b9f4e`). W1 partially in flight (uncommitted MutationApply
  repairs observed for F12/P1). W1w, W2, W3+ not started.
- Full findings evidence: `docs/creative_mode/creative_series_review_2026_07_03.md`.

## The backlog

### OP0 — Foundation entry (not this program's work, but its gate)

Fix red gate → W1 → W1w → W2. Owned by the foundation plan. This program's
only involvement: the golden cases in doc 02 §S1 land WITH W2's L0 math header
if the timing lines up (one lane, one order — coordinate, don't duplicate).

### OP1 — Descriptor capability columns (doc 01) — [C], rides W3

- Entry: W1 merged; W3 order active.
- Done: columns filled for all rows via category defaults + receipted
  exceptions; invariants 1–7 in `creative_object_descriptor_tests`; golden
  rows pinned; `SpatialProjection` switches retired to table reads with
  before/after equality pin.

### OP2 — Occupancy index (doc 02) — [C], IS W8a

- Entry: W2 (grid settings + L0 math), OP1 S1 (boundsRule), W1 (D8 semantics).
- Done: S1–S6 gates green; pick path consumes DDA rayPick (z-plane deleted);
  per-click full projection retired; stats surface live; golden cases 1–10
  pinned; interim revision-poll present but marked for OP3 deletion.

### OP3 — Dirty channels + drain (doc 05) — [C], IS W4 detail

- Entry: W1 (repaired flags), OP2 S6 (a real consumer exists).
- Done: single-writer test; golden routing table (kind × mutation product);
  drain cycle in the window loop; index + draw adapter on channels;
  revision-poll deleted; overlapping-drain guard at 0 in all tests.

### OP4 — Draw adapter (doc 03 S1–S2) — [C], IS W8b

- Entry: OP2 (index), OP3 for S2 (channel drain).
- Done: `adapters/Draw` alive; layers 1–3 culling with receipts; golden 1–3;
  unit-gated (no renderer wiring yet).

### OP5 — Boxes on screen (doc 03 S3) — [C], needs W1w + W7

- Entry: W1w (parity + coordinate laws), W7 (creative entry exists), OP4.
- Done: single choke point both renderers; parity test at Vulkan-ON and
  Vulkan-OFF; live-entry pick+draw test at 1x and 2x; wireframe boxes visible
  in a real creative world.

### OP6 — Collision bake bridge (doc 04) — [C] bake + [P] activation

- Entry: W3 (creation), W5/W6 (persistence + world service), OP2.
- Done: `bakeCreativeRoomAsset` deterministic (hash-twice pin); golden fixture
  surface counts matching ascii-pipeline conventions (RECON-04a/b resolved);
  activation through the existing room path; a creative-built room is
  walkable/collidable in a real session.

### OP7 — Marker wire-string emission (doc 04) — [C], planner-ordered

- Entry: the A7 emission order (planner-brokered contract bump), OP6 transport
  proven. Done: GameplayMarker objects emit append-only anchor strings; spawns
  and reasoning-graph nodes appear from a creative-authored room; contract v0.2
  amendment merged.

### OP8 — Trigger + audio design packets (doc 04 charters) — planner first

- Entry: RECON-04c/d returned; a consumer ruling exists.
- Done (design packet, not code): one-page vocabulary amendment each; build
  slices cut only after.

### OP9 — Render scaling: LOD/instancing (doc 03 sockets) — metric-gated

- Entry: creative mesh identity exists; OP5 metrics show draw item counts
  actually hurting (measured, not vibes).

### OP10 — BVH / structure upgrades — metric-gated research packet

- Entry: doc 02 stats show grid failure (candidatesPerQuery or exactRejects
  pathological at real object counts; largeObjects_ dominating). Done: a
  measurement report + a decision, not code.

## Worker recon packets

Research packets A–E, iggy3d edition — most questions ALREADY have answers
from the 2026-07-02/03 recon+review passes (labels below). Workers verify at
cutting time (anchors drift) and fill the gaps.

### Worker A — object truth: **answered** (`observed`)

Document/facade/descriptor/mutation shapes: see doc 00 §2 anchors table.
Remaining: RECON-01a/b/c, RECON-05a (post-W1 drift check).

### Worker B — selection/spatial: **answered** (`observed`)

Pick chain, per-click projection scan, z-plane and DPI diseases: plan §8
(F1–F3) + doc 02. Remaining: RECON-02b/c after W1w moves the lines.

### Worker C — render submission: **open**

RECON-03a/b/c: fly-camera anchors, existing draw-primitive vocabulary both
renderers consume, FramePresenter choke points post-W1w. Return exact types +
file:line, labeled. This is the packet to run BEFORE OP4 S3/OP5 are cut.

### Worker D — collision/nav/trigger runtime: **half-answered**

`SpatialSurfaceSet` shape + `entityFromAnchor` + reasoning-graph consumption:
`observed` (doc 00 §2). Open: RECON-04a/b/c/d/e (surface record fields, ramp
bake, R-marker path, sound-event struct, movement broad-phase reality —
the last one decides whether baked surface counts are a budget).

### Worker E — dirty/bake/save: **half-answered**

`dirtyFlagsForMutation` exists + flags dropped after receipt: `observed`.
Save spine (D2/D2b, SaveBridge identity carry-forward, temp-write/validate/
commit): `observed` in the foundation plan. Open: RECON-05b/c (drain cycle
anchor, UI refresh trigger).

Worker returns use the evidence labels (measured/observed/inferred/unknown)
and go into the RECON section of the owning doc — a doc whose RECON items are
all `observed` is cuttable; anything less is not.

## Definition-of-done discipline (applies to every OP)

- Full ctest green on the Mac per slice; box GCC/Vulkan-OFF parity as the
  fleet post-check (F11-class compiler divergence is a live hazard).
- Receipts for every operation the slice adds (LAW-16); metrics wired
  (LAW-20); golden cases as tests, verbatim from the owning doc.
- One lane per commit. Cross-lane OPs (OP6+) are two sequenced commits,
  planner-brokered.
- A slice that discovers its doc's anchor drifted: receipt the drift in the
  PR/commit message and update the doc line — never silently adapt.

## Research 07 disposition

The per-category checklists of research `07_category_optimization_checklists.md`
are folded into doc 01's category-default table (capabilities) and the OP
backlog above (the optimizations themselves). The room/portal category rows
wait on the multiroom stream; foliage/patch rows wait on mesh identity (OP9
territory). Nothing in 07 is lost; it is just not independently cuttable.
