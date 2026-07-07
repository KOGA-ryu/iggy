# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

Current ready cards seed the complexity-reduction roadmap in
`docs/complexity_audit_v0_1.md`. E124 is a read-only audit that PRODUCES the
next implementation card — the bucket is kept small and fed one slice at a time
on purpose. Do not pre-load implementation cards whose premise a prior slice
could invalidate.

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

1. **E125** — Product Header Include Hygiene Pass 1 (mechanical, free build win).
2. **E126** — TraversalTag Catalog Contract And Validator Parity (fixes the
   `clamber_candidate` validator drift without broad literal migration).
3. **E123** — Creative Object Kind Switch Cleanup (small impl; blocks itself if
   the descriptor lacks the fact).
4. **E124** — ProductPrimitiveDrawKind Metadata Audit (read-only).

## Tier 1: Correctness And Compatibility

- **E124** — `ProductPrimitiveDrawKind` per-kind color drifted across 3 render
  files (`ElevatedFloorTile` confirmed). Read-only audit first.
- **E126** — traversal-tag validator parity from E122; retain
  `clamber_candidate` as valid content vocabulary but keep movement slot policy
  unchanged.

## Tier 2: Feature-Add Seams

- **E123** — route creative `toString()` / `allowedMutations()` through the
  descriptor facts they duplicate; drops object-kind add from 4 sites to 2.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

- **E125** — forward-declare `ProductAppWindowState` in `ReceiptBuilder.hpp` +
  `RendererLifecycle.hpp` so the 62-include god-header stops re-parsing in ~40
  TUs.

## Parking Lot

Held until their audit card returns — do NOT promote to `ready/` on a guess:

- **Traversal-tag emitter/consumer migration** across RoomBake, ASCII room,
  movement, collision, and display/debug strings: do this after E126 lands and
  the catalog contract is stable. Do not migrate false-positive receipt/render
  strings blindly.
- **Draw-kind metadata table implementation** (replace the 3 render switches with
  one `constexpr` table + resolved drift values): held until **E124** returns.
- Further include-hygiene passes (the remaining ~11 headers) after E125.
- `activeRoom → activeRoomCollision` revision/dirty-guard service (audit finding
  #4, product-app / claude lane) — needs its own scoped card; see the audit
  roadmap. Prior context in `done/E91-active-room-collision-writer-graph.md`.
- Remaining complexity-audit roadmap items (`OpeningMenuView` split,
  `Operations.cpp` seam split after the identity-mirror removal, shared test
  infrastructure) — see `docs/complexity_audit_v0_1.md` §3.
