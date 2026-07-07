# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

**Collision freshness store slice COMPLETE** (G2–G7 done,
`docs/active_room_collision_freshness_preflight_v0_2.md`). The `activeCreative`
mirror delete is COMPLETE (E136-E139). The `creativeFly` anchor store is
COMPLETE (E142-E147). Current work: structural `activeRoom` regroup into
RoomStore. This is explicitly not an ownership kill; the owner is already
single-source. Builder should pull E148-E152 in numeric order.

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

1. **E148** — RoomStore G1 accessor seam.
2. **E149** — RoomStore G2 production writers.
3. **E150** — RoomStore G3 production readers and receipts.
4. **E151** — RoomStore G4 test fixture migration.
5. **E152** — RoomStore G5 final storage move.

## Tier 1: Correctness And Compatibility

- **E148-E152** — activeRoom RoomStore structural regroup. Preserve
  `roomEditing.activeRoom` and `roomEditing.activeRoomCollision` as producer
  state. Use the accessor seam first; move storage/delete old top-level fields
  only in E152.

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
- **RE-RANK (2026-07-07, `done/E140-activeroom-roomstore-preflight.md`):** after #2
  cleared, the next real ownership kill was **#3 `creativeFly`**, not #1. That work
  is now complete. #1 is being promoted by explicit user direction as a structural
  regroup only; preserve the nested `roomEditing.activeRoom` producer copy.
- **Ownership-deficit queue** (`docs/ownership_deficit_audit.md`) landing onto the
  decomposition map (`docs/god_struct_decomposition_target_map.md`):
  - **#3 `creativeFly`→`CreativeFlyAnchorStore`** — **COMPLETE as E142-E147.**
    Genuine freshness deficit. Gate-0/1 preflight `done/E141` v0.2 ratified:
    token is a new window-owned `creativeWorldEpoch` (NOT a content hash — two
    blank worlds hash identically), 3 seeders are NOT redundant (distinct
    yaw/pitch + latch behaviors), app-lane-only (stays out of the session gate).
  - **#1 `activeRoom`→`RoomStore`** — **CURRENT WORK as E148-E152.** Structural
    regroup only; NOT an ownership kill. Preserve the nested producer copy.
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
