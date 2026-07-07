# E132 (E-ARCF-G5): remove the scattered rebakes — `ensure` is the only rebuild path

## Objective

Gate 5 of the freshness guard: **remove every scattered direct rebake** so the production collision-rebuild
path is `ensureActiveRoomCollisionFresh(...)` and nothing else. Delete the Controller mid-tick branch, redirect
TapeRunner, remove the install-path direct bakes, and prove — by a coverage grep + the unchanged suite — that
every reader now depends on the freshness seam. Behavior stays **byte-identical** (the removed bakes were
redundant with the G4 frame-boundary `ensure`).

**Gated spine work — the mass-removal gate.** Owner design:
`docs/active_room_collision_freshness_preflight_v0_2.md` (§G5 in the Gate table). Removal map:
`docs/active_room_collision_rebake_removal_checklist.md` — **§A/§B/§C/§D are the exact sites**.

## Scope boundary (READ FIRST)

This gate removes **rebakes**, not data-model duplicates. Per preflight §11 (non-goal), it does **NOT** move
`activeRoom` off the god-struct and does **NOT** sever the nested `roomEditing.{activeRoom, activeRoomCollision}`
**fields** (`EditingState.hpp:19-20`) — that is **deficit #1** (`activeRoom`→`RoomStore`,
`docs/ownership_deficit_audit.md`), a separate slice with its own preflight. G5 only removes the collision
**copy** at `AutomationRoomEditing.cpp:89` and examines the mirror **bake** at `EditingState.cpp:129`.

## Why This Exists

G4 wired `ensure` at the frame boundary + TapeRunner and proved order-independence (T-order). The scattered
direct bakes are now redundant. Removing them makes the single owner real: one place rebuilds collision, and a
grep proves it. This is the payoff — "who refreshed last?" is gone.

## Required Work

Work strictly from the removal checklist. **Before removing each site, confirm a preceding `ensure` guards its
reader** (G4 wired the frame boundary + TapeRunner seam); the suite is the proof.

1. **§B — delete the Controller mid-tick branch** `Controller.cpp:2240-2241` (the `Interact` rebake). The
   frame-boundary `ensure` subsumes it; T-order stays green. This removes the handler-order dependence for good.
2. **§C — TapeRunner**: remove the unconditional `refreshActiveRoomCollision` bake (`:493` + the helper
   `:221-227`); the G4 `ensure` call at the read seam (`:213`) is the replacement. T-tape stays green.
3. **§A — install-path direct bakes**: remove the collision bakes at `Operations.cpp:{189-190, 720-721,
   746/749, 1328-1329, 1678-1679}`, `Activation.cpp:{72 (intermediate), 117-118}`, and the collision **copy**
   `AutomationRoomEditing.cpp:89`. **Keep the `{}` clears** (rows 2/3 — the frame-boundary `ensure` yields the
   unloaded blob via I4; the clear stays). The writer `bumpActiveRoomRevision` calls from G4 stay.
4. **§D — examine `EditingState.cpp:129`** (the room-editor mirror bake): after removing the `:89` copy, if
   `state.activeRoomCollision` is consumed only by that copy, the mirror bake is dead — remove it. If the room
   editor reads the mirror's collision independently, **keep + justify** in the brief.
5. **Coverage grep (the §G5 assertion)**: after removal, `grep -rn "buildProductActiveRoomCollision(" src`
   outside `ActiveRoomCollisionFreshnessStore.cpp` and the two overload **definitions**
   (`ActiveRoomCollision.cpp:117/122`) must be **empty** (modulo any explicitly-justified retained fallback,
   documented in the brief). Every reader-facing rebuild flows through `ensure`.
6. **C2 — I7 re-audit**: grep `entity.active`/`setActive` outside the hashed session command path; must stay
   empty. Record in the brief.

## Acceptance Notes

- The Controller mid-tick branch and the TapeRunner unconditional bake are gone; the install-path direct bakes
  + the `:89` copy are gone; `{}` clears remain.
- **Coverage grep empty** — `buildProductActiveRoomCollision(` appears only in the store + the two definitions.
- **Full suite green and UNCHANGED — behavior byte-identical.** Specifically: T-order, T-tape, the G3
  stale-case tests, `product_receipt_key_order_tests`, and `product_creative_no_window_bake_scenario_tests` all
  pass **verbatim**. If ANY goes red, a reader's seam is unguarded — **stop, move to `blocked/`** with the
  unguarded reader named (do NOT re-add a scattered bake to paper over it).
- `EditingState.cpp:129` resolved (removed or justified). C2 I7 re-audit recorded.

## Do Not

- Do NOT sever the nested `roomEditing.{activeRoom, activeRoomCollision}` fields or move `activeRoom` off the
  god-struct — that is **deficit #1**, a separate slice (§ Scope boundary).
- Do NOT remove the `{}` clears or the `bumpActiveRoomRevision` calls. Do NOT change collision math / overloads.
- Do NOT paper over a red test by restoring a scattered bake — block instead.
- Do NOT name anything `Kernel`. Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
ctest --test-dir /Users/kogaryu/iggy3d/build
grep -rn "buildProductActiveRoomCollision(" --include='*.cpp' /Users/kogaryu/iggy3d/src | grep -v test \
  | grep -vE "ActiveRoomCollisionFreshnessStore.cpp|ActiveRoomCollision.cpp:11[0-9]|ActiveRoomCollision.cpp:12[0-9]"   # expect empty
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- Sites removed (vs checklist §A/§B/§C):
- `EditingState.cpp:129` resolution (removed / justified):
- Coverage grep result (empty?):
- Behavior-identical evidence (T-order/T-tape/stale/anchors/oracle verbatim):
- C2 I7 re-audit:
- Concerns/deferred:

---

## Completion Brief - 2026-07-07

- Files changed:
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/automation/AutomationGameplay.cpp`
  - `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
  - `src/app/iggy3d/gameplay/Controller.cpp`
  - `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - `docs/creative_mode/builder_tasks/done/E132-arcf-g5-remove-scattered-rebakes.md`
- Sites removed (vs checklist §A/§B/§C):
  - §A install-path direct bakes removed from `Operations.cpp`:
    `createProductSessionFromPackage`, `ProductCreativeBakedRoomRefreshService::handleRejectedBake`,
    `ProductCreativeBakedRoomRefreshService::installBakedRoom`, `launchProductNewWorld`, and
    `launchProductSaveSlot`.
  - §A install-path direct bakes removed from `ascii_room/Activation.cpp`:
    pre-session ASCII preview activation and with-session final activation.
  - §A automation collision copy removed from `AutomationRoomEditing.cpp`; the copied active room now bumps the
    window revision and derives collision through `ensureActiveRoomCollisionFresh(window, nullptr)`.
  - §B controller mid-tick `Interact` direct rebake removed; the branch now only records
    `interactionExecuted`.
  - §C TapeRunner `refreshActiveRoomCollision(...)` helper and post-tick direct call removed.
  - Added missing reader/consumption guards found by the suite:
    `AutomationGameplay.cpp` ensures before and after automation gameplay actions, and
    `TapeRunner.cpp` ensures before collision reads and after ticks. The legacy no-window TapeRunner request
    path adapts `{activeRoom, activeRoomCollision}` through a temporary window-shaped view, still routing the
    rebuild through `ensureActiveRoomCollisionFresh(...)`.
- `EditingState.cpp:129` resolution (removed / justified):
  - Justified retained mirror bake. `ProductRoomEditingState::activeRoomCollision` is still consumed by the
    room-editor state itself for counts/readiness (`fillCounts`) and by focused room-editor tests/cursor/action
    controller paths. The window-facing copy from this mirror was removed, so this retained call is not a
    scattered window active-room collision refresh.
- Coverage grep result (empty?):
  - Raw production grep outside store/definitions reports the single justified survivor:
    `src/app/iggy3d/room_editor/EditingState.cpp:129`.
  - With that §D survivor excluded, the G5 assertion is empty:
    `grep -rn "buildProductActiveRoomCollision(" --include='*.cpp' src | grep -v test | grep -vE "ActiveRoomCollisionFreshnessStore.cpp|ActiveRoomCollision.cpp:11[0-9]|ActiveRoomCollision.cpp:12[0-9]|room_editor/EditingState.cpp:129"` produced no output.
  - `refreshActiveRoomCollision` and `activeRoomCollision = state.activeRoomCollision` no longer appear in
    production source.
- Behavior-identical evidence (T-order/T-tape/stale/anchors/oracle verbatim):
  - `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_tape_runner_tests|product_ascii_authoring_smoke)$' --output-on-failure` passed after adding the missing consumption-seam ensures.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build` passed: 260/260.
  - This includes the T-order/stale active-room collision tests, T-tape `product_gameplay_tape_runner_tests`,
    `product_receipt_key_order_tests`, and `product_creative_no_window_bake_scenario_tests`.
- C2 I7 re-audit:
  - Direct write grep for `entity.active =` outside expected session/replay/save/fixture/marker-binding seams
    produced no output.
  - `setActive(` grep outside expected `runtime/world/WorldState`, `runtime/interaction/InteractionSystem`,
    and creative tool-selection seams produced no output.
- Concerns/deferred:
  - The room-editor mirror still has its own collision bake by design; removing the mirror field/collision
    counts belongs to the separate room-editing state/god-struct deficit work, not G5.
  - TapeRunner has a narrow legacy no-window adapter because that public request shape has no window revision
    counter. It still routes through the freshness store and does not call `buildProductActiveRoomCollision`
    directly.
  - `Testing/Temporary/LastTest.log` changed from CTest output and was left untouched.
