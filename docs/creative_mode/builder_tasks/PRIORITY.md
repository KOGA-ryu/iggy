# Builder Priority Index

This file ranks the task bucket without changing the bucket mechanics. Task
files still live in `ready/`, `claimed/`, `done/`, or `blocked/`; builder should
use this index only to decide which ready card to claim next.

**Collision freshness store slice COMPLETE** (G2–G7 done,
`docs/active_room_collision_freshness_preflight_v0_2.md`). The `activeCreative`
mirror delete is COMPLETE (E136-E139). The `creativeFly` anchor store is
COMPLETE (E142-E147). The structural `activeRoom` regroup into RoomStore is
COMPLETE (E148-E152). The SaveSessionStore bulk move is COMPLETE (E153).

**Also complete:** the dead write-only fields `window.inputOwner` /
`window.gameplayInputSuppressed` were deleted (`36ceeac3`), and
`runtimeStateHash` was deleted/rederived in E179. After E179, the god-struct
remainder is 14 top-level members: 9 store members and 5 app-global scalars.
**GameplayStore is
COMPLETE**: E154 was decomposed into E157-E160 and all four child slices are
complete. Disjoint from completed RoomStore/SaveSessionStore work;
`runtimeSessionCreated` now belongs to GameplayStore.

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
`productVulkanMenu` now lives under `ProductAppWindowState::frontendShell`.

**Decomposition card set now COMPLETE through CreativeAuthoringStore.** `E161`
CreativeAuthoringStore (#4) is **COMPLETE** as E168-E172: wireframe,
viewport-pick, room-editor, world/ascii, and creative UI state are now under
`ProductAppWindowState::creativeAuthoring`. Remaining
parent `E164` FrontendWindowShell (#10 — RULING: split store vs app-global
remainder; **COMPLETE** as E174-E176: scalar/menu, startup, and
`productVulkanMenu` now live under `ProductAppWindowState::frontendShell`).
NOTE:
E157-E160 are the completed GameplayStore slices
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

None currently ready.

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
  - **#4 `CreativeAuthoringStore`** — **COMPLETE as E168-E172.**
  - **#10 `FrontendWindowShell`** — **COMPLETE as E174-E176.** E173 audited the
    current state; E174 created the shell and moved scalar/menu/status state
    plus retargeted `automationControl` ownership; E175 moved `startup`; E176
    moved the final deferred `productVulkanMenu` field.
  - **#2 `activeCreative`→delete** (`CreativeIdentityStore`) — cheapest standalone, own Gate-0.
  - **#3 `creativeFly`→`CreativeFlyAnchorStore`** — own preflight.
  - Delete-cleanups (`inputOwner`/`gameplayInputSuppressed`, `runtimeStateHash`)
    are complete.
- **Traversal-tag emitter/consumer migration**: E180-E187 routed durable
  affordance/anchor and traversal payload emitters/consumers. E188 is the
  read-only remainder classification pass for parser/display/stable-id/save
  literals before any additional migration card is written.
- Further draw-kind metadata cleanup after E127 is COMPLETE as E222-E223.
  E222 found no broad metadata/bridge-count card, and E223 routed the only
  remaining `OpeningMenuView` primitive primary colors through the metadata
  backed draw-item path.
- **ProductAppWindowState include-hygiene lane is CLOSED after E208.**
  E201-E207 removed all forward-declarable production/test-support header
  includes. The remaining header includes are complete-type required:
  `AppKernel.hpp` and `window/Loop.hpp` own `ProductAppWindowState` by value,
  and `ProductAsciiRoomWindowTestSupport.hpp` constructs/writes
  `ProductAppWindowState` in an inline helper.
- **Product test infrastructure**: E189 audited duplicated product-test helpers.
  E190 added a header-only assertion/numeric helper over five tests. E191 added
  receipt helpers for four creative UI receipt/frame tests. E192 added the
  duplicated active-surface wrapper helper. E193 added clean temp-root/options
  setup. E194 found no immediate neutral fixture-builder extraction. E195
  preflighted the three ASCII-activated gameplay window helpers and justified
  only a tiny explicit activation helper; E196 implemented that helper.
- `npc_vision_lab` stale training_room fixture + the "same room checked in 3×"
  duplication smell — E197 read-only preflight chose generated package-local
  copy plus drift guard. E198 updated the stale `npc_vision_lab` copy from the
  shared generated asset and extended package parity/load coverage.
- Active-surface sync tail: E199 proved
  `syncProductWindowInputOwnerFromActiveSurface(...)` is a misnamed no-op
  resolver wrapper after window input-owner cache deletion. E200 removed ignored
  production calls, repointed tests/support to the direct resolver, and deleted
  the stale helper.
- Remaining complexity-audit roadmap items: `E209` completed the read-only
  preflight for `Operations.cpp` seam split after identity-mirror removal.
  `E210` completed G1 by extracting only the creative baked active-room refresh
  service. `E211` completed G2 by extracting the save-slot
  browser/delete/recover operations. `E212` completed G3 by extracting creative
  world launch/open/save operations. `E213` completed G4 read-only preflight for
  remaining product session/world launch coupling. `E214` completed G4a by
  extracting only product package-path/world-template resolution. `E215`
  completed G4b by extracting product save/load session launch without moving
  product new-world or current-session save/write behavior. `E216` completed
  G4c by extracting product new-world launch without moving current-session
  save/write or creative blank-stage wrappers. `E217` completed G4d by
  extracting current-session save/write without moving the creative blank-stage
  wrappers. `E218` completed G4e by moving the final launch-state helpers and
  deleting the empty `Operations.*` shell.
- Snap/grid duplicate math (#7 in `docs/complexity_audit_v0_1.md`) is
  COMPLETE as E219-E221. E220 added checked core `double` scalar snap and
  routed creative 2D/document scalar snap through it. E221 added the
  `SpatialProjection` 3D grid guard slice without touching `GridFootprint`.
  Note: the audit's object-kind switch cleanup item is stale in the current tree;
  `CreativeObjectKind` `toString(...)` and `allowedMutations(...)` already route
  through descriptor helpers.
- `OpeningMenuView` split is COMPLETE as E224-E230. E224 preflighted the
  split; E225 extracted `SdlDraw`; E226 extracted `ScenePrimitiveView`; E227
  extracted `DebugHudView`; E228 extracted `MenuPanelsView`; E229 extracted
  `OpeningMenuHitTest`; E230 completed the final `OpeningMenuView` facade
  include cleanup.
- `docs/refactor_targets.md` is the new current-HEAD effort-ranked refactor
  backlog. `Controller.cpp` split target #2 is COMPLETE: E231 preflighted the
  current file shape, E232 extracted the first G1 kinematics helper, E233
  extracted the G2 movement proof/debug writer helpers, E234 extracted the G3
  ground-query helpers, E235 extracted a narrowed G4a jump/dash state helper,
  E236 extracted a G5a wall-surface query/direction helper, and E237 is
  extracted as a G5b wall-run evaluation/publishing helper. E238 extracted the
  G6a traversal proof-writer helper. E239 extracted the G7a player/session
  access helper. E240 extracted the G7b reset/fall helper. E241 extracted the
  G8a target/outcome proof helper. E242 extracted the G9a jump-actions helper.
  E243 extracted the G10a command-execution helper. E244 extracted the G11a
  dash-submit helper. E245 extracted the G12a move-submit helper. E246 extracted
  the G13a target-submit helper. E247 extracted the G14a reset-action helper.
  E248 extracted the G15a input-intent helper. E249 is released as a G16a
  action-phase orchestration helper, leaving `Controller.cpp` as a tiny public
  facade.
- `docs/refactor_targets.md` target #1 receipt field boilerplate is COMPLETE as
  E250-E265. E250 completed the read-only preflight and chose
  `GameplaySceneStateFields.cpp` as the safest one-file pilot. E251-E264
  table-drove every safe direct/mixed receipt field section while preserving the
  receipt golden. E265 closed the lane: the remaining append sites are dynamic,
  optional, top-level result policy, or aggregation-only, so a shared row helper
  is deferred and `ProductAppReceiptContext` remains unnecessary for target #1.
- `docs/refactor_targets.md` target #3 `Extract iggy3d_creative main()` is
  active next because target #2 `Controller.cpp` split is complete. E266
  completed the read-only preflight and chose the safest G1 as moving only the
  file-local `appendWireframeBoxEdges(...)` helper out of
  `apps/iggy3d_creative/main.cpp`. E267 completed that helper extraction. E268
  introduced the app-local `CreativeEditorState` shell and moved the current
  mutable editor/frame-loop locals behind it without moving any frame stages.
  E269 completed the first real frame-loop stage extraction: poll events,
  handle drawable/resize, apply fly-camera input, and return the current
  keyboard state/extent without changing downstream tool or render behavior.
  E270 moved the shared standalone selected-object delete helper out of
  `main.cpp`. E271 extracted only the interactive command-key block (tool
  switch, brush cycle, save/load/clear/delete/undo latches) while leaving
  capture scenario and later movement/gizmo policy in `main.cpp`. E272 is
  complete as the aim-ground-cell camera-ray helper used by placement/capture
  previews, leaving placement and capture policy in `main.cpp`. E273 is
  complete as the shared raw camera-ray-to-ground-point helper for the remaining
  interactive Move branch, without moving Move policy. E274 is released to
  extract only the current selection id/object/bounds resolution, leaving gizmo,
  move, overlay, and logging policy in `main.cpp`. E275 is complete as the
  gizmo-frame geometry and path-handle hit-data helper. E276 is complete as the
  path-handle capture proof logging helper inside the existing gizmo-frame
  module. E277 is complete as visible-object pick candidate construction plus
  point/line/path proxy proof logs. E278 is complete as the capture-only
  world-pick proof logging helper inside the existing pick-frame module. E279
  is complete as synthetic/interactive click selection. E280 is complete as
  interactive placement input. E281 is complete as capture-scenario request
  assembly. The remaining Move, overlay, submit, and shutdown seams should get
  an explicit next card instead of being promoted on a guess.
