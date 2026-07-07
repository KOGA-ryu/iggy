# activeRoom → collision direct-rebake removal checklist

**Purpose:** the authoritative list of every current direct `activeRoomCollision` rebake/write callsite —
the removal checklist for **G4** (add `bumpActiveRoomRevision(window)` at the final `activeRoom` write) and
**G5** (remove the direct bake; prove the reader now depends on the freshness seam). Produced for the
E-ARCF-G2 additional requirement (Gate-1 ratification, 2026-07-07). Grepped from `iggy3d-main`; **G2 must
re-run the greps and reconcile any drift before G4 consumes this.**

Reproduce:
```sh
grep -rn "buildProductActiveRoomCollision(" --include='*.cpp' src | grep -v test
grep -rn "activeRoomCollision =" --include='*.cpp' src/app/iggy3d | grep -v test | grep -vE '==|!='
grep -rn "refreshActiveRoomCollision" --include='*.cpp' src/app/iggy3d | grep -v test
```

## A. Window install/clear writers — G4 bump, G5 subsume into `ensure`

| # | Site | Function | Kind | G4 (bump after final activeRoom write) | G5 (remove direct bake) |
|---|---|---|---|---|---|
| 1 | `Operations.cpp:184` + `:189-190` | `createProductSessionFromPackage` | clear `{}` then bake install | bump once after `:187` set | remove `:189-190`; reader via seam |
| 2 | `Operations.cpp:284` | `createCreativeBlankSession` | clear `{}` | bump after final `activeRoom` write | (clear only; keep clear, drop redundant collision reset) |
| 3 | `Operations.cpp:453` | `clearProductGameplayLaunchState` | terminal clear `{}` | **bump on the clear** | keep clear; `ensure` yields unloaded blob (I4) |
| 4 | `Operations.cpp:720-721` | `ProductCreativeBakedActiveRoomRefreshExecutor::handleRejectedBake` | bake install | bump after room set | remove; seam rebakes |
| 5 | `Operations.cpp:746` + `:749` | `...::installBakedRoom` | bake install | bump after room set | remove `:746/:749` |
| 6 | `Operations.cpp:1328-1329` | `launchProductNewWorld` (ascii) | bake install | bump after `:1327` | remove `:1328-1329` |
| 7 | `Operations.cpp:1678-1679` | `launchProductSaveSlot`/`launchProductContinueSave` | bake install | bump after `:1663` | remove `:1678-1679` |
| 8 | `Activation.cpp:72` | `activateProductAsciiRoomPreview` (no-session) | nullptr bake | bump after `:71` room set | intermediate — superseded at `:118` before any reader; may drop early (§G4 note) |
| 9 | `Activation.cpp:117-118` | `activateProductAsciiRoomPreview` (with-session) | bake install | (final write — bump here) | remove `:117-118` |
| 10 | `AutomationRoomEditing.cpp:89` | `copyRoomEditingStateToWindow` | **whole-struct copy** `window.activeRoom = state.activeRoom` (`:88`) then collision copy (`:89`) | **bump `window` AFTER the `:88` copy** (window-owned counter is copy-safe, I2) | remove `:89`; seam rebakes for the window |

## B. Order-dependence — G5 DELETE (the staleness fix, not just cleanup)

| # | Site | Function | Action |
|---|---|---|---|
| 11 | `Controller.cpp:2240-2241` | `submitProductGameplayCommand` mid-tick Interact rebake | **DELETE.** The frame-boundary `ensure` subsumes it; deleting removes the handler-order dependence (T-order proves it). Valid-memory stale read today (§1), so no UB caveat. |

## C. TapeRunner — G5 redirect

| # | Site | Function | Action |
|---|---|---|---|
| 12 | `TapeRunner.cpp:221-227` (helper), `:226` (write), `:493` (callsite) | `refreshActiveRoomCollision` (unconditional) | **Redirect `:493` to `ensure`.** Equivalence lemma T-tape: TapeRunner re-derives the pointer per step (`:209`), so no captured pointer is invalidated; a no-op-tick step exercises the skip path, a door-toggle step the rebake path. |

## D. Out of scope / definitions — do NOT touch

- `room_editor/EditingState.cpp:129` — writes `ProductRoomEditingState::activeRoomCollision` (a **non-window
  mirror**), not `window.activeRoomCollision`. Its output reaches the window only via `AutomationRoomEditing.cpp:89`
  (row 10), which IS the in-scope window write. The room-editor's own mirror is outside the freshness pair.
- `gameplay/ActiveRoomCollision.cpp:117` / `:122` — the two `buildProductActiveRoomCollision` overload
  **definitions**. The Store calls these; they are not callsites to remove.

## Coverage assertion for G5

After G5, a grep for `buildProductActiveRoomCollision(` outside `ActiveRoomCollisionFreshnessStore.cpp`, the
two overload definitions, and any explicitly-retained install fallback must be **empty** — every reader-facing
rebake flows through `ensureActiveRoomCollisionFresh`. Re-audit **I7** (no `entity.active` door mutation bypasses
the hashed session path — grep `.active =` / `setActive` outside the session command path) at this gate and G7,
per the Gate-1 condition.
