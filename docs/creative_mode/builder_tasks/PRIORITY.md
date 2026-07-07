# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

The current pipeline is a **gated spine change** (the activeRoom→collision
freshness guard, `docs/active_room_collision_freshness_preflight_v0_2.md`,
Gate-1 ratified). **Gates G2–G6 done → G7 seeded** (the finish: receipt
instrumentation + audit + architecture receipt). G7 completes the collision store.

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

1. **E134 (E-ARCF-G7)** — the finish: surface freshness `reasonCode` into the
   receipt, run the naming/ownership audit, write the architecture receipt. Adds a
   receipt field (regenerate the golden) + a window field (add to the ownership TSV
   as `RoomStore`) — both truth-gates fire by design. Read preflight §G7/§8 first.

## Tier 1: Correctness And Compatibility

- **E134 (E-ARCF-G7)** — Gate 7 finish (instrumentation + audit + architecture
  receipt). Completes the `ActiveRoomCollisionFreshnessStore` slice.

## Tier 2: Feature-Add Seams

None currently ready.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

None currently ready.

## Parking Lot

Held — do NOT promote to `ready/` on a guess:

- **After G7 = collision store done.** Next standalone landing on the decomposition
  map: **#2 `activeCreative`→delete** (`CreativeIdentityStore`, cheapest, own Gate-0
  preflight). Then #1 `activeRoom`→`RoomStore` (its own preflight) and #3 `creativeFly`.
- **Ownership-deficit queue** (`docs/ownership_deficit_audit.md`) landing onto the
  decomposition map (`docs/god_struct_decomposition_target_map.md`):
  - **#1 `activeRoom`→`RoomStore`** — the move-off-god-struct that severs the nested
    `roomEditing.{activeRoom, activeRoomCollision}` duplicate storage
    (`EditingState.hpp:19-20`). **Its OWN preflight** (NOT folded into collision G5,
    which only removes rebakes per §11).
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
