# E168 — CreativeAuthoringStore G1: wireframe fields + shared diagnostics header

**STATUS: READY.** Parent: `blocked/E161-creativeauthoringstore-bulk-move.md`.
**Commit convention:** `claude: planned. codex: ...`.

## Goal

Start the CreativeAuthoringStore decomposition without taking the full 86-field move.
This slice creates the store seam and moves only the ProductAppWindowState
`creativeWireframe*` telemetry fields into it.

This is a behavior-preserving structural move. Receipt key names, receipt order,
defaults, render behavior, wireframe behavior, RoomBake, save/load, input, UI
semantics, and Vulkan/render payload structs must not change.

## Required implementation

1. Add `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`.
2. Add `ProductAppWindowState::creativeAuthoring`.
3. Move only these 26 fields from `ProductAppWindowState` into
   `CreativeAuthoringStore`, preserving exact types, defaults, and ordering:
   - `creativeWireframeRequested`
   - `creativeWireframeActive`
   - `creativeWireframeFacadeAvailable`
   - `creativeWireframeDocumentAvailable`
   - `creativeWireframeSourceAvailable`
   - `creativeWireframeObjectCount`
   - `creativeWireframeVisibleObjectCount`
   - `creativeWireframeItemCount`
   - `creativeWireframeSegmentCount`
   - `creativeWireframeBoxItemCount`
   - `creativeWireframeLineItemCount`
   - `creativeWireframePointItemCount`
   - `creativeWireframeSkippedDegenerateCount`
   - `creativeWireframeStatus`
   - `creativeWireframeReasonCode`
   - `creativeWireframeWireframeStatus`
   - `creativeWireframeWireframeReasonCode`
   - `creativeWireframeSegmentStatus`
   - `creativeWireframeSegmentReasonCode`
   - `creativeWireframeDebugLineRequested`
   - `creativeWireframeDebugLineSourceAvailable`
   - `creativeWireframeDebugLineInputSegmentCount`
   - `creativeWireframeDebugLineCount`
   - `creativeWireframeDebugLineSkippedDegenerateCount`
   - `creativeWireframeDebugLineStatus`
   - `creativeWireframeDebugLineReasonCode`
4. Move the inline creative diagnostics structs out of
   `ProductAppWindowState.hpp` into a shared header included by both
   `ProductAppWindowState.hpp` and the new store header. Use a focused name such
   as `src/app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp`.
   The moved structs are:
   - `ProductCreativeUiCommandMutationDiagnostics`
   - `ProductCreativeUiCommandCreateDiagnostics`
   - `ProductCreativeUiCommandDeleteDiagnostics`
   - `ProductCreativeUiCommandUndoDiagnostics`
   - `ProductCreativeUiCommandRoomShellDiagnostics`
   - `ProductCreativeBakedRoomRefreshDiagnostics`
   - `ProductCreativeUiCommandDiagnostics`
5. Repoint only `ProductAppWindowState` wireframe telemetry users to
   `window.creativeAuthoring.<field>` or the equivalent named
   `ProductAppWindowState` variable.
6. Update `docs/god_struct_member_ownership.tsv`:
   - add one top-level row `creativeAuthoring	CreativeAuthoringStore` if absent.
   - delete only the 26 moved top-level `creativeWireframe*` rows.
7. Do not mark the parent CreativeAuthoringStore target complete. This is G1 only.

## Known relevant files

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- shared diagnostics header added by this slice
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `src/app/iggy3d/receipt/CreativePickWireframeFields.cpp`
- `src/app/iggy3d/creative/bridge/WireframeFrame.cpp`
- focused wireframe/receipt/god-struct tests under `tests/unit/`

Use the compiler to find additional ProductAppWindowState users after removing
the flat fields. Do not use broad token replacement.

## Do not move in this slice

Do not move `creativeViewportPick*`, room-editor fields, ASCII room fields,
world setup/creation fields, `creativeDocumentRevision`,
`creativeDocumentChangedThisFrame`, `creativeUndo`, `creativeBakedRoomStale*`,
`creativeNavigateActive`, `creativeWorldEpoch`, `creativeUiProjection`,
`creativeUiInput`, `creativeUiLast`, `creativeUiCommand`, or
`creativeBakedRoomAutoRefresh`.

## Foreign collisions to leave alone

Do not repoint render/projection payload fields that are not ProductAppWindowState
storage:

- `RenderFrame::creativeWireframeDebug`
- `ProductWindowFramePresenterRequest::creativeWireframeDebugLineList`
- `ProductWindowFramePresenterFrame::creativeWireframeDebugLines`
- render geometry/test fields such as
  `creativeWireframeDebugLineInputCount`,
  `creativeWireframeDebugGeometryDrawCount`, and
  `creativeWireframeDebugGeometrySkippedCount`
- startup timing fields:
  `window.startup.creativeWireframeFirstFrameMeasured`,
  `window.startup.creativeWireframeFirstFrameMicroseconds`,
  `window.startup.creativeWireframeFirstFrameStatus`

## Required greps

Report these results in the completion brief:

```sh
rg -n "window\\.(creativeWireframe(Requested|Active|FacadeAvailable|DocumentAvailable|SourceAvailable|ObjectCount|VisibleObjectCount|ItemCount|SegmentCount|BoxItemCount|LineItemCount|PointItemCount|SkippedDegenerateCount|Status|ReasonCode|WireframeStatus|WireframeReasonCode|SegmentStatus|SegmentReasonCode|DebugLineRequested|DebugLineSourceAvailable|DebugLineInputSegmentCount|DebugLineCount|DebugLineSkippedDegenerateCount|DebugLineStatus|DebugLineReasonCode))\\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
rg -n "\\bcreativeWireframe(Requested|Active|FacadeAvailable|DocumentAvailable|SourceAvailable|ObjectCount|VisibleObjectCount|ItemCount|SegmentCount|BoxItemCount|LineItemCount|PointItemCount|SkippedDegenerateCount|Status|ReasonCode|WireframeStatus|WireframeReasonCode|SegmentStatus|SegmentReasonCode|DebugLineRequested|DebugLineSourceAvailable|DebugLineInputSegmentCount|DebugLineCount|DebugLineSkippedDegenerateCount|DebugLineStatus|DebugLineReasonCode)\\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp
rg -n "^creativeWireframe" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Expected result for all three: no output.

Also report whether foreign `creativeWireframeDebug*` render/projection payload
hits remain and confirm they were intentionally not moved.

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
- confirmation that the diagnostics structs moved out of `ProductAppWindowState.hpp`
- the 26 fields moved
- receipt golden result
- ownership coverage result
- required grep results
- full suite result
- deferred slices still untouched
- any remaining dirty `Testing/Temporary/LastTest.log` note

No stage, commit, push, or window launch.
