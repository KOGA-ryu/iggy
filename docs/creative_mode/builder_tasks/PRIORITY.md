# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

The current pipeline is a **gated spine change** (the activeRoom→collision
freshness guard, `docs/active_room_collision_freshness_preflight_v0_2.md`,
Gate-1 ratified). It is fed one gate at a time. A parallel non-spine card (the
god-struct coverage gate) rides alongside. Do not pre-load later gates (G5–G7).

## Claim Policy

1. If a task listed under **Pull Next** is present in `ready/`, claim the first
   such task.
2. If none of the Pull Next tasks are ready, claim the first ready task from the
   highest non-empty tier below.
3. If a task is already in `claimed/`, do not claim another until that task
   moves to `done/` or `blocked/`.
4. If this file falls behind the physical bucket, the physical bucket wins.

## Currently Claimed

- **E131** — god-struct ownership coverage gate (member→owner map + test; no
  production change). The decomposition truth-gate.

## Pull Next

None currently ready.

## Tier 1: Correctness And Compatibility

None currently ready.

## Tier 2: Feature-Add Seams

None currently ready.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

None currently ready.

## Parking Lot

Held — do NOT promote to `ready/` on a guess:

- **Freshness-guard gates G5–G7** — held; seeded one at a time after each prior
  gate is reviewed/committed. **G5 note (from the decomposition map):** widen the
  removal to sever BOTH nested duplicates — `roomEditing.activeRoom` **and**
  `roomEditing.activeRoomCollision` (`EditingState.hpp:19-20`)
  (`docs/active_room_collision_freshness_preflight_v0_2.md` Gate 1–7 table;
  removal checklist `docs/active_room_collision_rebake_removal_checklist.md`).
- **Ownership-deficit queue** (`docs/ownership_deficit_audit.md`) landing onto the
  decomposition map (`docs/god_struct_decomposition_target_map.md`): #2
  `activeCreative`→delete (`CreativeIdentityStore`, cheapest, own Gate-0), #1
  `activeRoom`→`RoomStore` (rides collision G4/G5), #3 `creativeFly`→
  `CreativeFlyAnchorStore` (own preflight), + two delete-cleanups
  (`inputOwner`/`gameplayInputSuppressed`, `runtimeStateHash`).
- **Traversal-tag emitter/consumer migration** across RoomBake, ASCII room,
  movement, collision, and display/debug strings: after the catalog contract is
  stable. Do not migrate false-positive receipt/render strings blindly.
- Further draw-kind metadata cleanup after E127, if render owners decide to
  centralize secondary/decorative colors.
- Further include-hygiene passes (the remaining ~11 headers) after E125.
- `npc_vision_lab` stale training_room fixture + the "same room checked in 3×"
  duplication smell — needs a single-source-of-truth card.
- Remaining complexity-audit roadmap items (`OpeningMenuView` split,
  `Operations.cpp` seam split after the identity-mirror removal, shared test
  infrastructure) — see `docs/complexity_audit_v0_1.md` §3.
