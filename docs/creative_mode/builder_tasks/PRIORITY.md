# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

Current ready cards seed the complexity-reduction roadmap in
`docs/complexity_audit_v0_1.md`. Read-only audits produce implementation cards;
the bucket is kept small and fed one slice at a time on purpose. Do not pre-load
implementation cards whose premise a prior slice could invalidate.

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

1. **E126** — TraversalTag Catalog Contract And Validator Parity (fixes the
   `clamber_candidate` validator drift without broad literal migration).
2. **E127** — ProductPrimitiveDrawKind Presentation Metadata Table (base
   color/size centralization; dispatch and decorative render policy stay local).

## Tier 1: Correctness And Compatibility

- **E126** — traversal-tag validator parity from E122; retain
  `clamber_candidate` as valid content vocabulary but keep movement slot policy
  unchanged.
- **E127** — centralize `ProductPrimitiveDrawKind` base color/size metadata from
  E124 while preserving dynamic/decorative render policy.

## Tier 2: Feature-Add Seams

None currently ready.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

None currently ready.

## Parking Lot

Held until their audit card returns — do NOT promote to `ready/` on a guess:

- **Traversal-tag emitter/consumer migration** across RoomBake, ASCII room,
  movement, collision, and display/debug strings: do this after E126 lands and
  the catalog contract is stable. Do not migrate false-positive receipt/render
  strings blindly.
- Further draw-kind metadata cleanup after E127, if render owners decide to
  centralize secondary/decorative colors.
- Further include-hygiene passes (the remaining ~11 headers) after E125.
- `activeRoom → activeRoomCollision` revision/dirty-guard service (audit finding
  #4, product-app / claude lane) — needs its own scoped card; see the audit
  roadmap. Prior context in `done/E91-active-room-collision-writer-graph.md`.
- Remaining complexity-audit roadmap items (`OpeningMenuView` split,
  `Operations.cpp` seam split after the identity-mirror removal, shared test
  infrastructure) — see `docs/complexity_audit_v0_1.md` §3.
