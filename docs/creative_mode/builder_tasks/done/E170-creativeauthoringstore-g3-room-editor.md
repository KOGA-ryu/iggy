# E170 — CreativeAuthoringStore G3: room-editor fields

**STATUS: DONE.** Parent: `blocked/E161-creativeauthoringstore-bulk-move.md`.
Depends on E168 and E169. **Commit convention:** `claude: planned. codex: ...`.

## Goal

Continue the CreativeAuthoringStore decomposition by moving only the
ProductAppWindowState room-editing and room-editor telemetry/state fields into
the existing `ProductAppWindowState::creativeAuthoring` store.

This is a behavior-preserving structural move. Receipt key names, receipt order,
defaults, room-editor behavior, room-editing active-room producer behavior,
input routing, automation, save/load, UI semantics, and render behavior must not
change.

## Required implementation

1. Add only these 18 fields to `CreativeAuthoringStore`, preserving exact types,
   defaults, and field order:
   - `roomEditing`
   - `roomEditingLastOperation`
   - `roomEditingLastOperationStatus`
   - `roomEditingLastOperationReasonCode`
   - `roomEditingLastInputSource`
   - `roomEditingLastOperationAccepted`
   - `roomEditingLastPrimitiveId`
   - `roomEditorCursorReady`
   - `roomEditorCursor`
   - `roomEditorStatus`
   - `roomEditorReasonCode`
   - `roomEditorLastOperation`
   - `roomEditorLastOperationAccepted`
   - `roomEditorLastPrimitiveId`
   - `roomEditorOverlay`
   - `roomEditorPreview`
   - `roomEditorPlacementPreview`
   - `roomEditorHud`
2. Remove those 18 flat fields from `ProductAppWindowState`.
3. Repoint only ProductAppWindowState room-editor storage users to
   `window.creativeAuthoring.<field>` or the equivalent named
   `ProductAppWindowState` variable.
4. Update `docs/god_struct_member_ownership.tsv`:
   - keep the existing `creativeAuthoring	CreativeAuthoringStore` row.
   - delete only the 18 moved top-level room-editor rows.
5. Do not mark the parent CreativeAuthoringStore target complete. This is G3 only.

## Known relevant files

Expect compiler-guided repoints across these areas:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- `src/app/iggy3d/receipt/SaveStateFields.cpp`
- `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `src/app/iggy3d/menu/Transitions.cpp`
- focused room-editor, input-frame, transition, frontend-router, and receipt tests

Use the compiler to find additional ProductAppWindowState users after removing
the flat fields. Do not use broad token replacement.

## Do not move in this slice

Do not move viewport-pick or wireframe again. Do not move ASCII room fields,
world setup/creation fields, `creativeDocumentRevision`,
`creativeDocumentChangedThisFrame`, `creativeUndo`, `creativeBakedRoomStale*`,
`creativeNavigateActive`, `creativeWorldEpoch`, `creativeUiProjection`,
`creativeUiInput`, `creativeUiLast`, `creativeUiCommand`, or
`creativeBakedRoomAutoRefresh`.

Also do not move `ProductRoomStore room`; that is already RoomStore ownership
and is not part of CreativeAuthoringStore.

## Foreign collisions to leave alone

Do not repoint fields that are not ProductAppWindowState storage:

- request/context booleans such as `roomEditingReady` and `roomEditorReady`
- `ProductGameplayProjectionFrame::roomEditorHud` and related projection-frame
  payloads
- `PrimitiveDrawList` / `RenderBridge` counters such as
  `roomEditorPlacementPreviewVisible` and `roomEditorPlacementPreviewCount`
- room-editor domain request structs with their own `request.roomEditing`
  reference fields
- local helper names and test fixture row fields that are not
  `ProductAppWindowState`

## Required greps

Report these results in the completion brief:

```sh
rg -n "window\\.(roomEditing|roomEditingLastOperation|roomEditingLastOperationStatus|roomEditingLastOperationReasonCode|roomEditingLastInputSource|roomEditingLastOperationAccepted|roomEditingLastPrimitiveId|roomEditorCursorReady|roomEditorCursor|roomEditorStatus|roomEditorReasonCode|roomEditorLastOperation|roomEditorLastOperationAccepted|roomEditorLastPrimitiveId|roomEditorOverlay|roomEditorPreview|roomEditorPlacementPreview|roomEditorHud)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "\\b(roomEditing|roomEditingLastOperation|roomEditingLastOperationStatus|roomEditingLastOperationReasonCode|roomEditingLastInputSource|roomEditingLastOperationAccepted|roomEditingLastPrimitiveId|roomEditorCursorReady|roomEditorCursor|roomEditorStatus|roomEditorReasonCode|roomEditorLastOperation|roomEditorLastOperationAccepted|roomEditorLastPrimitiveId|roomEditorOverlay|roomEditorPreview|roomEditorPlacementPreview|roomEditorHud)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^(roomEditing|roomEditor)" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Expected result for all three: no output.

Also run and classify the broader old-flat-access scan:

```sh
rg --pcre2 -n "(?<!creativeAuthoring)\\.(roomEditing|roomEditingLastOperation|roomEditingLastOperationStatus|roomEditingLastOperationReasonCode|roomEditingLastInputSource|roomEditingLastOperationAccepted|roomEditingLastPrimitiveId|roomEditorCursorReady|roomEditorCursor|roomEditorStatus|roomEditorReasonCode|roomEditorLastOperation|roomEditorLastOperationAccepted|roomEditorLastPrimitiveId|roomEditorOverlay|roomEditorPreview|roomEditorPlacementPreview|roomEditorHud)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

Any remaining matches must be classified as foreign request/context/projection
payloads or fixed before completion.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

`tests/golden/product_receipt_key_order.golden` must be byte-identical. If it
diffs, stop and report instead of regenerating.

## Completion brief must include

- exact files changed
- the 18 fields moved
- receipt golden result
- ownership coverage result
- required grep results
- classification of remaining broad `roomEditing` / `roomEditor` foreign hits
- full suite result
- deferred slices still untouched
- any remaining dirty `Testing/Temporary/LastTest.log` note

No stage, commit, push, or window launch.

## Completion Brief

Files changed:
- `docs/god_struct_member_ownership.tsv`
- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/automation/AutomationDispatch.cpp`
- `src/app/iggy3d/automation/AutomationRoomEditing.cpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- `src/app/iggy3d/menu/FrontendRouter.cpp`
- `src/app/iggy3d/menu/Transitions.cpp`
- `src/app/iggy3d/receipt/SaveStateFields.cpp`
- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `tests/unit/product_frontend_router_tests.cpp`
- `tests/unit/product_interaction_mode_state_tests.cpp`
- `tests/unit/product_menu_transitions_tests.cpp`
- `tests/unit/product_room_editor_action_controller_tests.cpp`
- `tests/unit/product_vulkan_room_frame_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- this builder task card

Moved fields:
- `roomEditing`
- `roomEditingLastOperation`
- `roomEditingLastOperationStatus`
- `roomEditingLastOperationReasonCode`
- `roomEditingLastInputSource`
- `roomEditingLastOperationAccepted`
- `roomEditingLastPrimitiveId`
- `roomEditorCursorReady`
- `roomEditorCursor`
- `roomEditorStatus`
- `roomEditorReasonCode`
- `roomEditorLastOperation`
- `roomEditorLastOperationAccepted`
- `roomEditorLastPrimitiveId`
- `roomEditorOverlay`
- `roomEditorPreview`
- `roomEditorPlacementPreview`
- `roomEditorHud`

Receipt golden result:
- `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed.
- Output: `receipt key-order oracle: 1032 fields match golden (order + values)`.
- `git diff -- tests/golden/product_receipt_key_order.golden` produced no output.

Ownership coverage result:
- `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests` passed.
- Output: `god-struct ownership coverage: assigned=49 CreativeAuthoringStore=20 DebugHudStore=1 FrontendWindowShell=16 GameplayStore=1 InputDeviceStore=1 PresentPathStore=1 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=4 delete=1`.

Required grep results:
- `rg -n "window\\.(roomEditing|roomEditingLastOperation|roomEditingLastOperationStatus|roomEditingLastOperationReasonCode|roomEditingLastInputSource|roomEditingLastOperationAccepted|roomEditingLastPrimitiveId|roomEditorCursorReady|roomEditorCursor|roomEditorStatus|roomEditorReasonCode|roomEditorLastOperation|roomEditorLastOperationAccepted|roomEditorLastPrimitiveId|roomEditorOverlay|roomEditorPreview|roomEditorPlacementPreview|roomEditorHud)\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'` produced no output.
- `rg -n "\\b(roomEditing|roomEditingLastOperation|roomEditingLastOperationStatus|roomEditingLastOperationReasonCode|roomEditingLastInputSource|roomEditingLastOperationAccepted|roomEditingLastPrimitiveId|roomEditorCursorReady|roomEditorCursor|roomEditorStatus|roomEditorReasonCode|roomEditorLastOperation|roomEditorLastOperationAccepted|roomEditorLastPrimitiveId|roomEditorOverlay|roomEditorPreview|roomEditorPlacementPreview|roomEditorHud)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp` produced no output.
- `rg -n "^(roomEditing|roomEditor)" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv` produced no output.

Broad old-flat-access scan classification:
- `tests/unit/product_vulkan_room_frame_tests.cpp:1268`, `1309`, `1311`: foreign projection payload fields on `projection.roomEditorHud` / `projection.roomEditorOverlay`.
- `src/app/iggy3d/room_editor/Presentation.cpp:245`: foreign room-editor request payload `request.roomEditing.ready`.
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp:727`, `729`, `732`, `740`, `799`, `802`, `807`, `815`, `819`: foreign projection-frame payloads on `frame.roomEditorOverlay` / `frame.roomEditorHud`.
- `src/app/iggy3d/window/FramePresenter.cpp:929`, `1034`: foreign projection-frame/render payloads on `request.projectionFrame.roomEditorHud` / `projectionFrame.roomEditorHud`.
- No remaining old-flat scan hits are ProductAppWindowState storage; they are request/context/projection/render payloads intentionally left outside `CreativeAuthoringStore`.

Full suite result:
- `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed: `100% tests passed, 0 tests failed out of 260`.

Other checks:
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files produced no matches.

Deferred slices still untouched:
- Did not move ASCII/world setup fields.
- Did not move UI command, stale, undo, revision, navigate, or auto-refresh fields.
- Did not move `ProductRoomStore room`.
- Did not move already-moved viewport pick or wireframe fields again.
- Did not move render/projection `creativeWireframeDebug*` payloads.
- Did not stage, commit, push, or launch a window.

Dirty `Testing/Temporary/LastTest.log` note:
- `Testing/Temporary/LastTest.log` is not dirty in `git status --short`.
