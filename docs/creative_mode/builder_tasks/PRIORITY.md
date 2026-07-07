# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

**Collision freshness store slice COMPLETE** (G2–G7 done,
`docs/active_room_collision_freshness_preflight_v0_2.md`). The `activeCreative`
mirror delete is also COMPLETE (E136-E139). Current work: decomposition deficit
#3 — move creative-fly anchor ownership into a freshness-aware store. Gate-0/1
is ratified in `done/E141-creativefly-anchor-store-preflight.md`; builder
should pull E142-E147 in numeric order.

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

1. **E142** — CreativeFly G2 store types and `creativeWorldEpoch`.
2. **E143** — CreativeFly G3 store verbs and direct freshness tests.
3. **E144** — CreativeFly G4 origin seed and camera consumer wiring.
4. **E145** — CreativeFly G5 remaining seeder/integrator migration.
5. **E146** — CreativeFly G6 stress and ordering guards.
6. **E147** — CreativeFly G7 receipt update and legacy raw-field delete.

## Tier 1: Correctness And Compatibility

- **E142-E147** — creativeFly anchor freshness store (deficit #3, M,
  policy-sensitive). Preserve the three distinct seeding behaviors, use
  `creativeWorldEpoch` rather than any content/session hash, and delete legacy
  raw fields only in E147.

## Tier 2: Feature-Add Seams

None currently ready.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

None currently ready.

## Parking Lot

Held — do NOT promote to `ready/` on a guess:

- **#2 `activeCreative`→delete is COMPLETE as E136-E139** (turned out L, not
  the audit's "cheapest S" — the mirror fed a hot-path routing predicate;
  preflight v0.2). E135 is retained as the decomposed parent in `done/`.
  NOTE: the audit's S/M/L ratings are directional — each slice needs its own
  preflight to confirm scope (E135 was rated S, proved L; **E140 re-ranked #1
  from 9.0 → hold**).
- **RE-RANK (2026-07-07, `blocked/E140-activeroom-roomstore-preflight.md`):** after #2
  clears, the next real ownership kill is **#3 `creativeFly`**, NOT #1. Recon found #1's
  ownership payoff is already banked by the collision freshness store, and its nested
  `roomEditing.activeRoom` is a load-bearing producer copy (do not delete). #1 is now a
  high-churn (252 refs + 225 tests) zero-ownership mechanical regroup — deferred until
  the decomposition actually gates the kernel.
- **Ownership-deficit queue** (`docs/ownership_deficit_audit.md`) landing onto the
  decomposition map (`docs/god_struct_decomposition_target_map.md`):
  - **#3 `creativeFly`→`CreativeFlyAnchorStore`** — **CURRENT WORK as E142-E147.**
    Genuine freshness deficit. Gate-0/1 preflight `done/E141` v0.2 ratified:
    token is a new window-owned `creativeWorldEpoch` (NOT a content hash — two
    blank worlds hash identically), 3 seeders are NOT redundant (distinct
    yaw/pitch + latch behaviors), app-lane-only (stays out of the session gate).
  - **#1 `activeRoom`→`RoomStore`** — **HELD** (`blocked/E140`). Structural move only
    when it gates the kernel; NOT an ownership kill (see re-rank above). Preserve the
    nested producer copy.
  - **#2 `activeCreative`→delete** (`CreativeIdentityStore`) — cheapest standalone, own Gate-0.
  - **#3 `creativeFly`→`CreativeFlyAnchorStore`** — own preflight.
  - Two delete-cleanups (`inputOwner`/`gameplayInputSuppressed`, `runtimeStateHash`).
- **Traversal-tag emitter/consumer migration** across RoomBake, ASCII room,
  movement, collision, and display/debug strings: after the catalog contract is
  stable. Do not migrate false-positive receipt/render strings blindly.
- Further draw-kind metadata cleanup after E127, if render owners centralize
  secondary/decorative colors.
- Further include-hygiene passes (the remaining ~11 headers) after E125.
- `npc_vision_lab` stale training_room fixture + the "same room checked in 3×"
  duplication smell — needs a single-source-of-truth card.
- Remaining complexity-audit roadmap items (`OpeningMenuView` split,
  `Operations.cpp` seam split after the identity-mirror removal, shared test
  infrastructure) — see `docs/complexity_audit_v0_1.md` §3.
