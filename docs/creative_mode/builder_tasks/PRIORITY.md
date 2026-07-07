# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

**Collision freshness store slice COMPLETE** (G2–G7 done,
`docs/active_room_collision_freshness_preflight_v0_2.md`). The `activeCreative`
mirror delete is COMPLETE (E136-E139). The `creativeFly` anchor store is
COMPLETE (E142-E147). The structural `activeRoom` regroup into RoomStore is
COMPLETE (E148-E152). The SaveSessionStore bulk move is COMPLETE (E153).

**Also complete:** the 2 dead write-only fields `window.inputOwner` /
`window.gameplayInputSuppressed` were deleted (`36ceeac3`) — god-struct now 153
members. **GameplayStore is COMPLETE**: E154 was decomposed into E157-E160 and
all four child slices are complete. Disjoint from completed
RoomStore/SaveSessionStore work; `runtimeSessionCreated` now belongs to
GameplayStore.

**ViewportStore fold is COMPLETE as E155** (#6 — folded 11 mapMaker* fields into
ProductViewportState). `E156` InputDeviceStore (#7) has been decomposed into
E165-E167 and is **COMPLETE**. E165 moved the device/action fields, E166 moved
controller/capture fields, and E167 moved `interactionMode` plus
`interactionModeHud` into `ProductAppWindowState::inputDevice`.
`gamepadMenuSelectUsed` remains in FrontendWindowShell.
`E162` DebugHudStore (#9) is **COMPLETE**: the five HUD/debug mirrors now live
under `ProductAppWindowState::debugHud`.
`E163` PresentPathStore (#11) is **COMPLETE**: the nine productVulkan present
path/status fields now live under `ProductAppWindowState::presentPath`, while
`productVulkanMenu` remains for FrontendWindowShell.

**Decomposition card set now COMPLETE through PresentPathStore.** CreativeAuthoringStore
is now being sliced from parent `E161`; `E168` wireframe and `E169`
viewport-pick are complete, and `E170` room-editor is ready. Remaining
parents staged in `blocked/` (recon-grounded, `wdnplylk0`): `E161` CreativeAuthoringStore (#4 — the
giant, 86 fields, ~1679 repoints, MUST slice by sub-domain), `E164` FrontendWindowShell (#10 — RULING: split store vs app-global
remainder; **flags a reclaim of `gamepadMenuSelectUsed` from InputDeviceStore/E156 —
needs planner confirm**). NOTE: E157-E160 are the completed GameplayStore slices
(E154); new cards start at E161 to avoid collision. **EXECUTION SERIALIZES on the
god-struct — release/run one store at a time, re-anchoring each.**

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

1. `E170-creativeauthoringstore-g3-room-editor.md` — third
   CreativeAuthoringStore child. Move only the 18 ProductAppWindowState
   room-editing and room-editor fields.

## Tier 1: Correctness And Compatibility

None currently ready.

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
  - **#1 `activeRoom`→`RoomStore`** — **COMPLETE as E148-E152.** Structural
    regroup only; NOT an ownership kill. Preserved the nested producer copy.
  - **#5 `SaveSessionStore`** — **COMPLETE as E153.** Structural regroup only;
    `runtimeSessionCreated` was corrected to GameplayStore ownership.
  - **#6 `ViewportStore`** — **COMPLETE as E155.**
  - **#7 `InputDeviceStore`** — **COMPLETE as E165-E167.**
    E165 moved the lower-risk device/last-input fields into `inputDevice`;
    E166 moved controller/capture fields; E167 moved the high-collision
    interaction-mode fields last. `gamepadMenuSelectUsed` remains in
    FrontendWindowShell.
  - **#8 `GameplayStore`** — **COMPLETE as E157-E160.**
  - **#9 `DebugHudStore`** — **COMPLETE as E162.**
  - **#11 `PresentPathStore`** — **COMPLETE as E163.**
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
