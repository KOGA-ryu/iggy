# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

Current ready cards seed the complexity-reduction roadmap in
`docs/complexity_audit_v0_1.md`. Two of them (E122, E124) are read-only audits
that PRODUCE the next implementation cards — the bucket is kept small and fed one
slice at a time on purpose. Do not pre-load implementation cards whose premise a
prior slice could invalidate.

## Claim Policy

1. If a task listed under **Pull Next** is present in `ready/`, claim the first
   such task.
2. If none of the Pull Next tasks are ready, claim the first ready task from the
   highest non-empty tier below.
3. If a task is already in `claimed/`, do not claim another until that task
   moves to `done/` or `blocked/`.
4. If this file falls behind the physical bucket, the physical bucket wins.

## Currently Claimed

None.

## Pull Next

1. **E122** — TraversalTag Catalog Audit (read-only; unblocks the one HIGH seam).
2. **E125** — Product Header Include Hygiene Pass 1 (mechanical, free build win).
3. **E123** — Creative Object Kind Switch Cleanup (small impl; blocks itself if
   the descriptor lacks the fact).
4. **E124** — ProductPrimitiveDrawKind Metadata Audit (read-only).

## Tier 1: Correctness And Compatibility

- **E122** — traversal-tag vocabulary has no owner; two validators already
  drifted on `clamber_candidate` (confirmed live bug). Read-only audit first.
- **E124** — `ProductPrimitiveDrawKind` per-kind color drifted across 3 render
  files (`ElevatedFloorTile` confirmed). Read-only audit first.

## Tier 2: Feature-Add Seams

- **E123** — route creative `toString()` / `allowedMutations()` through the
  descriptor facts they duplicate; drops object-kind add from 4 sites to 2.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

- **E125** — forward-declare `ProductAppWindowState` in `ReceiptBuilder.hpp` +
  `RendererLifecycle.hpp` so the 62-include god-header stops re-parsing in ~40
  TUs.

## Parking Lot

Held until their audit card returns — do NOT promote to `ready/` on a guess:

- **Traversal-tag catalog implementation** (the `TraversalTag` single-source
  migration across ~13 files + drift resolution): held until **E122** returns
  the exact map. E122 drafts this as E126 (+ a per-lane split if needed).
- **Draw-kind metadata table implementation** (replace the 3 render switches with
  one `constexpr` table + resolved drift values): held until **E124** returns.
- Further include-hygiene passes (the remaining ~11 headers) after E125.
- `activeRoom → activeRoomCollision` revision/dirty-guard service (audit finding
  #4, product-app / claude lane) — needs its own scoped card; see the audit
  roadmap. Prior context in `done/E91-active-room-collision-writer-graph.md`.
- Remaining complexity-audit roadmap items (`OpeningMenuView` split,
  `Operations.cpp` seam split after the identity-mirror removal, shared test
  infrastructure) — see `docs/complexity_audit_v0_1.md` §3.
