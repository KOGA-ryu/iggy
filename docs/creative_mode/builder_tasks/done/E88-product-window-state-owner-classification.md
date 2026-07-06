# E88: Product Window State Owner Classification

## Objective

Create a current owner classification for `ProductAppWindowState` fields before
any state ownership surgery.

## Dependency

Do this after E87 unless planner explicitly redirects. This is read-only.

## Problem

The normalized architecture tally ranks `ProductAppWindowState` mirror sprawl as
the highest-pressure surface:

- large state struct,
- many production writers,
- Creative identity mirrors,
- active-room/collision mirrors,
- command/input receipt mirrors,
- tests seeding flat fields by hand.

The next safe move is not code surgery. It is a field ownership map that says
which fields are authoritative, derived, transient, receipt-only, or stale
compatibility mirrors.

## Required Reads

- `docs/creative_mode/post_claude_architecture_review_tally.md`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/creative/CreativeAppState.hpp`
- `src/app/iggy3d/gameplay/ActiveRoomState.hpp`
- `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `src/app/iggy3d/save/Flow.cpp`
- Focused tests that seed `ProductAppWindowState` directly.

## Scope

Read-only audit only.

Produce a completion brief with a table grouped by field family:

- active Creative identity,
- active room,
- active room collision,
- Creative UI input/command receipts,
- baked-room refresh/stale state,
- undo state,
- menu/frontend state,
- save/session state,
- anything else found in the struct.

For each family, classify:

- authoritative owner,
- writers,
- readers,
- whether the window field is authoritative, derived, transient, receipt-only,
  or compatibility mirror,
- first safe migration direction,
- tests that would catch stale mirror bugs.

## Do Not

- Do not edit production code.
- Do not change field layout.
- Do not change routing, save, launch, active-room, input, or receipt behavior.
- Do not create a broad refactor card inside this card.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Acceptance

- Completion brief gives a concrete field-family owner matrix.
- It identifies the first migration candidate and explains why it is safe.
- It identifies at least one stale-mirror failure mode and which test should
  catch it.

## Suggested Checks

```sh
rg -n "activeCreative|activeRoom|activeRoomCollision|creativeBakedRoom|creativeUndo|recordProductCreative|ProductAppWindowState" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests/unit
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files inspected:
- Field-family owner matrix:
- Production writers/readers:
- Stale-mirror risks:
- First safe migration candidate:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief - 2026-07-06

- Files inspected:
  - `docs/creative_mode/post_claude_architecture_review_tally.md`
  - `docs/lane_function_contracts_v0_1.md`
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/creative/CreativeAppState.hpp`
  - `src/app/iggy3d/gameplay/ActiveRoomState.hpp`
  - `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp`
  - `src/app/iggy3d/menu/FrontendRouter.cpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - focused tests that seed/read `ProductAppWindowState` directly, especially
    `product_creative_world_launch_tests.cpp`,
    `product_creative_ui_input_frame_tests.cpp`,
    `product_creative_ui_command_receipt_tests.cpp`,
    `product_frontend_router_tests.cpp`, and `product_save_bridge_tests.cpp`.
- Field-family owner matrix:

  | Field family | Current window fields | Authoritative owner | Classification | First migration direction |
  |---|---|---|---|---|
  | Active Creative identity | `activeCreativeSaveId`, `activeCreativeSavePath`, `activeCreativeWorldId`, `activeCreativeDocumentId`, object/next-id/save dirty mirrors | `creative::CreativeAppState::identity` / `CreativeActiveIdentity` (`CreativeAppState.hpp`) | Compatibility mirror plus receipt projection | Move production reads to `CreativeActiveIdentity`; keep window fields receipt-only until tests no longer seed them. |
  | Active room | `activeRoom` | `ProductActiveRoomState` built from package/ascii/RoomBake helpers | Derived runtime snapshot that is currently authoritative inside product window state | Keep typed state, but centralize install/clear with collision so room/collision cannot drift. |
  | Active room collision | `activeRoomCollision` | `buildProductActiveRoomCollision(activeRoom, session.state())` | Derived runtime snapshot paired to active room | Migrate writers to one active-room install service returning room+collision together. |
  | Creative UI input/downstream | `creativeUiInput*`, `creativeUiInputDownstreamClick*`, sticky last-click fields | `ProductCreativeUiInputFrameReceipt` / downstream click receipt | Receipt-only plus sticky diagnostics | Keep window as render-receipt projection; avoid new command behavior reading these fields except same-frame orchestration. |
  | Creative UI command | `creativeUiCommand` nested diagnostics | `ProductCreativeUiCommandFrameReceipt` | Receipt-only mirror | Schema-descriptor follow-up from E87 should serialize from command receipt/mirror groups without adding flat fields. |
  | Baked-room refresh and stale | `creativeBakedRoomAutoRefresh`, `creativeBakedRoomStale*`, revision frame fields | `refreshProductCreativeBakedActiveRoom(...)` result plus document id/revision compare | Derived state and receipt diagnostics | Keep stale keyed to `(documentId, revision)`; later cache should use that as the bake key, not flat booleans. |
  | Undo | `creativeUndoAvailable`, `creativeUndoDepth` | `CreativeAppState::undoStack` | Derived UI/receipt mirror | UI projection already reads undo stack directly; keep window fields receipt-only and clear them only through lifecycle helpers. |
  | Menu/frontend | opening/menu/gamepad/settings/frontend action flags | `FrontendState`, menu model, input/action handlers | Transient presentation/input state | Long-term, derive from frontend/menu state at receipt time; do not mix with Creative identity. |
  | Save/session | `runtimeSessionCreated`, `gameplayActive`, launch/status/package/startup timers, product save/load fields, save-slot browser fields | `Session`, `FrontendState`, save flow result structs, `ProductSaveBridgeResult` | Mixed authoritative operational state and receipt mirrors | Split typed results from selection/transient UI fields; product save identity should not depend on Creative identity mirrors. |
  | Room editor / legacy map-maker | room editor state, overlay/preview, map-maker/top-down flags | room editor controller/state, map-maker surface classifier | Compatibility/transient editor surface state | Keep as legacy/editor-owned until old creative/map-maker paths are retired. |
  | Gameplay/render/automation diagnostics | movement, jump, traversal, target, tape, Vulkan, automation fields | gameplay controller/session/renderer/automation systems | Mostly receipt diagnostics with some transient controller state | Leave out of Creative ownership surgery; they need separate field-family audits. |

- Production writers/readers:
  - Active Creative identity writer: `mirrorProductActiveCreativeIdentity(...)`
    in `Operations.cpp`, with clear path also resetting undo mirrors.
  - Active Creative identity readers still using window mirrors:
    `productCreativeWorldActiveForWindowMirror(...)` in `FrontendRouter.cpp`
    and `recordPauseCreativeFacadeMissing(...)` in `save/Flow.cpp`.
    `FrontendRouter.cpp` already has `productCreativeWorldActiveForIdentity(...)`,
    so the typed owner is available.
  - Active room/collision writers:
    session/package launch installs both in `Operations.cpp`; RoomBake refresh
    clears or installs both; ascii activation and room-editor copy paths also
    write both.
  - Active room/collision readers:
    `ReceiptBuilder.cpp` appends both; `InputFrame.cpp` uses collision surfaces
    for gameplay/creative input; `PrimitiveDrawList` and automation/gameplay
    helpers read them for projection, collision, and authored-room editing.
  - Creative UI input/command writers:
    `InputFrame.cpp` routes input/command receipts, then calls
    `recordProductCreativeUiInputFrame(...)` and
    `recordProductCreativeUiCommandFrame(...)`.
  - Baked-room refresh/stale writers:
    launch/open and manual refresh call `refreshProductCreativeBakedActiveRoom`;
    mutation-time input uses the document revision/id comparison in
    `InputFrame.cpp`; `ReceiptBuilder.cpp` records stale/fresh fields.
  - Undo writers:
    `InputFrame.cpp` pushes snapshots on real document revision changes and
    mirrors availability/depth after the creative input section. Launch/open and
    return-to-title clear the stack/mirrors through operations/menu lifecycle.
- Stale-mirror risks:
  - Active Creative identity drift is the highest risk: frontend routing and
    save-failure diagnostics can read `window.activeCreative*` even if
    `CreativeAppState::identity` has already changed or cleared. A stale mirror
    can make Creative look active after identity clear, or hide a live Creative
    document from save/routing.
  - Active room/collision drift: any future path that assigns `window.activeRoom`
    without rebuilding `window.activeRoomCollision` can leave rendering loaded
    but collision stale, or collision ready for a different room.
  - Undo mirror drift: `creativeUndoAvailable/depth` can show an enabled/disabled
    row inconsistent with `CreativeAppState::undoStack` if a lifecycle path
    clears only one side.
  - Receipt weak-green risk: tests often seed flat `ProductAppWindowState`
    fields directly, so they can pass even if production receipts stop recording
    the same values from source receipts.
- First safe migration candidate:
  - Start with active Creative identity reads, not state layout. The typed owner
    already exists (`CreativeActiveIdentity`), `Operations.cpp` already mirrors
    it, and `FrontendRouter.cpp` already has an identity-based predicate. A
    focused slice can:
    1. add/pin a stale-window-mirror test where `CreativeAppState::identity`
       says inactive but `window.activeCreativeDocumentId` is nonzero,
    2. change the frontend/save call sites that can see `CreativeAppState` to
       read `identity` instead of window mirrors,
    3. keep `window.activeCreative*` as receipt compatibility fields only.
  - Do not start with active room/collision; that is higher blast radius because
    gameplay, projection, room editor, ascii activation, and automation all read
    it.
- Tests/checks run:
  - `rg -n "activeCreative|activeRoom|activeRoomCollision|creativeBakedRoom|creativeUndo|creativeUiCommand|creativeUiInput|ProductAppWindowState" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests/unit`
  - targeted `rg` over required source files for active Creative, active room,
    collision, UI command/input, undo, stale, and revision fields.
  - `rg -l "ProductAppWindowState" /Users/kogaryu/iggy3d/tests/unit`
  - `git -C /Users/kogaryu/iggy3d diff --check`
- Concerns/deferred:
  - `ProductAppWindowState` still mixes true runtime state, UI transients,
    compatibility mirrors, and receipt-only diagnostics. The safest path is a
    series of family-specific migrations with stale-mirror tests, not a broad
    struct rewrite.
  - The next implementation card should be explicit about one family only. For
    active Creative identity, do not delete window fields yet; first move
    production readers to `CreativeActiveIdentity` and leave public receipts
    unchanged.
