# ProductAppWindowState Decomposition Status

> Live execution ledger for the god-struct decomposition. Canonical field map:
> `docs/god_struct_decomposition_target_map.md`. **Keep this current** — update a store's
> Status the moment its card lands. A stale ledger is worse than none.

## Operating Rule

All execution serializes on the god-struct. Release and land one store at a time. Every card must pass:

- build
- full test suite
- golden byte-identical check
- TSV ownership update
- re-anchor after merge

Do not batch stores together.

## Current Status

### Done

#### #2 CreativeIdentityStore
Status: done
Episodes: E136–E139
Landing: `creativeApp.identity`
Moved/deleted: deleted `activeCreative`
Remember: rated cheapest S, proved L. The mirror fed hot-path routing. Audit ratings are directional, not truth.

#### #3 CreativeFlyAnchorStore
Status: done
Episodes: E142–E147
Landing: `viewport.creativeFlyAnchor`
Fields: `{positionMeters, provenance, seededFromWorldEpoch}`
Remember: freshness token is window-owned `creativeWorldEpoch`, not a content hash. Blank creative worlds can hash identically.

#### #1 RoomStore
Status: done
Episodes: E148–E152
Landing: `activeRoom(window)` accessor seam
Remember: structural regroup only. Do not delete nested `roomEditing.activeRoom` producer copy.

#### #5 SaveSessionStore
Status: done
Episode: E153
Landing: `window.saveSession`
Fields: 31 save/load/delete fields
Remember: `runtimeSessionCreated` is excluded and belongs to GameplayStore.

#### Dead-field deletes
Status: done
Commit: `36ceeac3`
Deleted: `window.inputOwner`, `window.gameplayInputSuppressed`
Remember: write-only mirrors. Compiler exposed 28-site test migration that grep missed.

## In Flight

### #8 GameplayStore
Status: in flight
Parent episode: E154
Slices: E157–E160
Landing: `window.gameplay`
Fields: 33, including `runtimeSessionCreated`
Scale: ~1670 repoints

Slices:
- G1 lifecycle/input: done
- G2 movement/planner: done
- G3 actions/outcomes/tape: ready
- G4 visibility/diagnostics: blocked

Remember: production is 100% uniform bare `window`. No verified `fastWindow`/`slowWindow` style aliases. Recon hallucinated those. Large but low-risk.

## Staged / Blocked

### #6 ViewportStore
Status: staged
Episode: E155
Landing: fold 11 `mapMaker*` fields into existing `ProductViewportState`
Scale: ~92 repoints
Remember: fold into existing `viewport`, not a wrapper. Keep `viewport` TSV row. Delete 11 moved rows.

### #7 InputDeviceStore
Status: staged
Episode: E156
Landing: `window.inputDevice`
Fields: 10
Scale: ~265 repoints
Remember: real hazard. `interactionMode` exists on six foreign structs plus `Operations.cpp` alias `window_`. Compiler-guided only. No sed rampage.

### #4 CreativeAuthoringStore
Status: staged
Episode: E161
Landing: `window.creativeAuthoring`
Fields: 86
Scale: ~1679 repoints across ~41 files
Remember: must slice by sub-domain:
- wireframe
- pick
- room-editor
- ascii/world-setup
- revision/undo

Precondition: relocate 8 inline diagnostic structs from god-header to shared header first.

Receipt gotcha: map said 4 appenders, but real shape is 3 appenders plus 1 recorder. `CreativeReceiptRecording` writes and emits no keys. Do not split the store four ways.

### #9 DebugHudStore
Status: staged
Episode: E162
Landing: `window.debugHud`
Fields: 5
Scale: ~79 repoints
Remember: real store. All five HUD fields are live.

### #11 PresentPathStore
Status: staged
Episode: E163
Landing: `window.presentPath`
Fields: 9 `productVulkan*` status members
Scale: ~92 repoints
Remember: map undercounted at 6. Recon reclaimed all 9 as one present-loop unit. `productVulkanMenu` is separate and goes to FrontendWindowShell.

### #10 FrontendWindowShell
Status: staged last
Episode: E164
Landing: `FrontendWindowShell`
Remember: ruling pass, not blind move. Coherent menu/boot flags move. Roughly 7 SDL/window-lifecycle bits stay app-global. Execute last so the true remainder is visible.

## Cross-Cutting Decisions

### `runtimeSessionCreated`
Owner: GameplayStore
Reason: launch flag written in lockstep with `gameplayActive` at 6 sites. Only reader is gameplay receipt. Not save state.

### `gamepadMenuSelectUsed`
Owner: FrontendWindowShell
Reason: menu-selection flag written beside `mouseMenuSelectUsed` in `InputFrame.cpp`. Not a device property.

### `automationControl`
Owner: app-global
Reason: 127 refs, distinct `automation_control_*` receipt fields, test/scripting-driver domain. Belongs beside `activeSession` and `creativeApp`.

## Target End-State

`ProductAppWindowState` should reduce to:

- roughly 7 app-global lifecycle bits
- `automationControl`
- 11 store members

Held by `AppKernel`.

## Execution Order

Recommended order after GameplayStore:

1. finish GameplayStore E159
2. unblock and finish GameplayStore E160
3. ViewportStore
4. InputDeviceStore
5. DebugHudStore
6. PresentPathStore
7. CreativeAuthoringStore, sliced
8. FrontendWindowShell last
