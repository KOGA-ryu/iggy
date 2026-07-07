# E169 — CreativeAuthoringStore G2: viewport-pick fields

**STATUS: DONE.** Parent: `blocked/E161-creativeauthoringstore-bulk-move.md`.
Depends on E168. **Commit convention:** `claude: planned. codex: ...`.

## Goal

Continue the CreativeAuthoringStore decomposition by moving only the
ProductAppWindowState `creativeViewportPick*` telemetry fields into the existing
`ProductAppWindowState::creativeAuthoring` store.

This is a behavior-preserving structural move. Receipt key names, receipt order,
defaults, viewport-pick behavior, input routing, projection, save/load, UI
semantics, and render behavior must not change.

## Required implementation

1. Add only these 23 fields to `CreativeAuthoringStore`, preserving exact types,
   defaults, and field order:
   - `creativeViewportPickRequested`
   - `creativeViewportPickActive`
   - `creativeViewportPickClickPresent`
   - `creativeViewportPickClickSuppressed`
   - `creativeViewportPickFacadeAvailable`
   - `creativeViewportPickSourceAvailable`
   - `creativeViewportPickProjected`
   - `creativeViewportPickPicked`
   - `creativeViewportPickObjectCount`
   - `creativeViewportPickProjectionCellCount`
   - `creativeViewportPickStatus`
   - `creativeViewportPickReasonCode`
   - `creativeViewportPickPickStatus`
   - `creativeViewportPickMessage`
   - `creativeViewportPickCoordX`
   - `creativeViewportPickCoordY`
   - `creativeViewportPickCoordZ`
   - `creativeViewportPickGridIndex`
   - `creativeViewportPickObjectId`
   - `creativeViewportPickObjectKind`
   - `creativeViewportPickOccupancyKind`
   - `creativeViewportPickTarget`
   - `creativeViewportPickCellIndex`
2. Remove those 23 flat fields from `ProductAppWindowState`.
3. Repoint only ProductAppWindowState telemetry reads/writes to
   `window.creativeAuthoring.<field>` or the equivalent named
   `ProductAppWindowState` variable.
4. Update `docs/god_struct_member_ownership.tsv`:
   - keep the existing `creativeAuthoring	CreativeAuthoringStore` row from E168.
   - delete only the 23 moved top-level `creativeViewportPick*` rows.
5. Do not mark the parent CreativeAuthoringStore target complete. This is G2 only.

## Known relevant files

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `src/app/iggy3d/receipt/CreativePickWireframeFields.cpp`
- tests that seed/read viewport-pick window telemetry:
  - `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
  - `tests/unit/product_creative_wireframe_frame_tests.cpp`

Use the compiler to find additional ProductAppWindowState users after removing
the flat fields. Do not use broad token replacement.

## Do not move in this slice

Do not move `creativeWireframe*` again, room-editor fields, ASCII room fields,
world setup/creation fields, `creativeDocumentRevision`,
`creativeDocumentChangedThisFrame`, `creativeUndo`, `creativeBakedRoomStale*`,
`creativeNavigateActive`, `creativeWorldEpoch`, `creativeUiProjection`,
`creativeUiInput`, `creativeUiLast`, `creativeUiCommand`, or
`creativeBakedRoomAutoRefresh`.

## Foreign collisions to leave alone

Do not repoint context/request fields that are not ProductAppWindowState storage:

- `ProductWindowInputFrameContext::creativeViewportPickViewport`
- `ProductWindowInputFrameContext::creativeViewportPickProjectionRequest`
- `ProductWindowInputFrameContext::creativeViewportPickZ`
- `ProductWindowInputFrameContext::creativeViewportPickDepthMode`
- local helper functions such as `creativeViewportPickProjectionRequest()`
- designated initializers in tests that build request/context objects, not
  `ProductAppWindowState`

## Required greps

Report these results in the completion brief:

```sh
rg -n "window\\.(creativeViewportPick(Requested|Active|ClickPresent|ClickSuppressed|FacadeAvailable|SourceAvailable|Projected|Picked|ObjectCount|ProjectionCellCount|Status|ReasonCode|PickStatus|Message|CoordX|CoordY|CoordZ|GridIndex|ObjectId|ObjectKind|OccupancyKind|Target|CellIndex))\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "\\bcreativeViewportPick(Requested|Active|ClickPresent|ClickSuppressed|FacadeAvailable|SourceAvailable|Projected|Picked|ObjectCount|ProjectionCellCount|Status|ReasonCode|PickStatus|Message|CoordX|CoordY|CoordZ|GridIndex|ObjectId|ObjectKind|OccupancyKind|Target|CellIndex)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^creativeViewportPick" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Expected result for all three: no output.

Also report remaining `creativeViewportPick*` foreign request/context hits and
confirm they were intentionally not moved.

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
- the 23 fields moved
- receipt golden result
- ownership coverage result
- required grep results
- full suite result
- deferred slices still untouched
- any remaining dirty `Testing/Temporary/LastTest.log` note

No stage, commit, push, or window launch.

## Completion Brief

Files changed:
- `docs/creative_mode/builder_tasks/done/E169-creativeauthoringstore-g2-viewport-pick.md`
- `docs/god_struct_member_ownership.tsv`
- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `src/app/iggy3d/receipt/CreativePickWireframeFields.cpp`
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_wireframe_frame_tests.cpp`

Fields moved:
- Added the 23 requested `creativeViewportPick*` telemetry fields to
  `CreativeAuthoringStore`, preserving exact type/default/order.
- Removed those 23 flat fields from `ProductAppWindowState`.
- Repointed viewport-pick recorder writes and pick/wireframe receipt reads to
  `window.creativeAuthoring.<field>`.
- Repointed focused tests that seed/assert those ProductAppWindowState
  telemetry values.
- Removed the 23 top-level `creativeViewportPick*` rows from
  `docs/god_struct_member_ownership.tsv`; kept the existing
  `creativeAuthoring	CreativeAuthoringStore` row.

Receipt golden result:
- `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests` passed:
  `receipt key-order oracle: 1032 fields match golden (order + values)`.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
  produced no output.

Ownership coverage result:
- `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`
  passed:
  `god-struct ownership coverage: assigned=67 CreativeAuthoringStore=38 DebugHudStore=1 FrontendWindowShell=16 GameplayStore=1 InputDeviceStore=1 PresentPathStore=1 RoomStore=1 SaveSessionStore=1 ViewportStore=2 app-global-remainder=4 delete=1`.

Required grep results:
- Direct `window.creativeViewportPick*` telemetry grep: no output.
- `ProductAppWindowState.hpp` flat `creativeViewportPick*` telemetry grep:
  no output.
- TSV `^creativeViewportPick` grep: no output.
- Remaining `creativeViewportPick*` hits are the moved
  `window.creativeAuthoring.*` telemetry uses, the receipt bridge, the store
  definition, and foreign request/context fields:
  `ProductWindowInputFrameContext::creativeViewportPickViewport`,
  `ProductWindowInputFrameContext::creativeViewportPickProjectionRequest`,
  `ProductWindowInputFrameContext::creativeViewportPickZ`,
  `ProductWindowInputFrameContext::creativeViewportPickDepthMode`, plus the
  local `creativeViewportPickProjectionRequest()` helper. Those foreign
  request/context fields were intentionally not moved.

Full suite result:
- `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
  passed: `100% tests passed, 0 tests failed out of 260`.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files produced no output.

Deferred slices still untouched:
- Did not move wireframe again, room editor, ASCII/world setup, UI command,
  stale, undo, revision, navigate, or auto-refresh fields.
- Did not move viewport-pick request/context fields.
- Did not mark the parent `CreativeAuthoringStore` target complete.

Dirty notes:
- `Testing/Temporary/LastTest.log` is not reported dirty by
  `git status --short`.
- No stage, commit, push, or window launch performed.
