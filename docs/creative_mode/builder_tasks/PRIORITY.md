# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

**Collision freshness store slice COMPLETE** (G2–G7 done,
`docs/active_room_collision_freshness_preflight_v0_2.md`). Current work:
decomposition deficit #2 — delete the `activeCreative` mirror. The oversized
parent card E135 has been decomposed into four builder-sized gates E136-E139
(design: `docs/activecreative_mirror_delete_preflight_v0_2.md`, Gate-1
ratified; L slice, compiler-guarded, thread-first-delete-last).

## Claim Policy

1. If a task listed under **Pull Next** is present in `ready/`, claim the first
   such task.
2. If none of the Pull Next tasks are ready, claim the first ready task from the
   highest non-empty tier below.
3. If a task is already in `claimed/`, do not claim another until that task
   moves to `done/` or `blocked/`.
4. If this file falls behind the physical bucket, the physical bucket wins.

## Currently Claimed

E138.

## Pull Next

1. **E138** — ActiveCreative G3 receipt identity threading. Depends on
   completed E136-E137.
2. **E139** — ActiveCreative G4 delete mirror. Depends on E136-E138.

## Tier 1: Correctness And Compatibility

- **E138-E139** — activeCreative mirror delete (deficit #2, L,
  compiler-guarded). Behavior-preserving; coverage + receipt truth-gates fire by
  design. E136 and E137 are done; builder should claim the remaining gates in
  numeric order only.

## Tier 2: Feature-Add Seams

None currently ready.

## Tier 3: Organization, Receipt Shape, And Test Hygiene

None currently ready.

## Parking Lot

Held — do NOT promote to `ready/` on a guess:

- **#2 `activeCreative`→delete is IN FLIGHT as E136-E139** (turned out L, not
  the audit's "cheapest S" — the mirror feeds a hot-path routing predicate;
  preflight v0.2). E135 is retained as the decomposed parent in `done/`. Next
  landings after it: **#1 `activeRoom`→`RoomStore`** (its own preflight; severs the nested
  `roomEditing.{activeRoom, activeRoomCollision}` duplicate) and **#3 `creativeFly`→`CreativeFlyAnchorStore`**.
  NOTE: the audit's S/M/L ratings are directional — each slice needs its own preflight to
  confirm scope (E135 was rated S, proved L; **E140 re-ranked #1 from 9.0 → hold**).
- **RE-RANK (2026-07-07, `blocked/E140-activeroom-roomstore-preflight.md`):** after #2
  clears, the next real ownership kill is **#3 `creativeFly`**, NOT #1. Recon found #1's
  ownership payoff is already banked by the collision freshness store, and its nested
  `roomEditing.activeRoom` is a load-bearing producer copy (do not delete). #1 is now a
  high-churn (252 refs + 225 tests) zero-ownership mechanical regroup — deferred until
  the decomposition actually gates the kernel.
- **Ownership-deficit queue** (`docs/ownership_deficit_audit.md`) landing onto the
  decomposition map (`docs/god_struct_decomposition_target_map.md`):
  - **#3 `creativeFly`→`CreativeFlyAnchorStore`** — **NEXT real kill after #2.** Genuine
    unfixed freshness deficit. **Gate-0 preflight `blocked/E141` v0.2** — decision A
    (re-seed on world-open drift) LOCKED and hardened by adversarial verification
    (workflow `wuu7jt0jx`): token is a new window-owned `creativeWorldEpoch` (NOT a
    content hash — two blank worlds hash identically), 3 seeders are NOT redundant
    (distinct yaw/pitch + latch behaviors), app-lane-only (stays out of the session
    gate). BLOCKED on (a) E139 landing and (b) Gate-1 ratification of the §7 checklist.
    Ready to decompose into G2–G7 once ratified.
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
