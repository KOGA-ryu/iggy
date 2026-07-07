# E172 — CreativeAuthoringStore G5: Creative UI State Final Move

**STATUS: READY.** Parent: `blocked/E161-creativeauthoringstore-bulk-move.md`.

## Goal

Move exactly these remaining `CreativeAuthoringStore` ownership fields out of
flat `ProductAppWindowState` storage and into the existing
`ProductAppWindowState::creativeAuthoring` store:

- `creativeDocumentRevision`
- `creativeDocumentChangedThisFrame`
- `creativeUndo`
- `creativeBakedRoomStale`
- `creativeBakedRoomStaleDocumentId`
- `creativeBakedRoomStaleRevision`
- `creativeBakedRoomStaleStatus`
- `creativeBakedRoomStaleReasonCode`
- `creativeNavigateActive`
- `creativeUiProjection`
- `creativeUiInput`
- `creativeUiLast`
- `creativeUiCommand`
- `creativeBakedRoomAutoRefresh`

This is the final child slice for `blocked/E161`. It is still a structural
ownership move only: receipt keys/order/values, creative input behavior, undo,
auto-refresh, stale diagnostics, mouse capture policy, and navigation behavior
must stay unchanged.

## Scope

Expected production files include, but are not limited to:

- `src/app/iggy3d/ProductAppWindowState.hpp`
- `src/app/iggy3d/creative/CreativeAuthoringStore.hpp`
- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `src/app/iggy3d/window/Loop.cpp`
- `src/app/iggy3d/window/MouseCapturePolicy.cpp`
- focused creative UI/input/command/launch/pick/wireframe tests
- `docs/god_struct_member_ownership.tsv`
- `docs/god_struct_decomposition_target_map.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`

Use compiler-guided repoints. Do not use broad token replacement. Because this
slice is dense, local aliases such as
`CreativeAuthoringStore& authoring = window.creativeAuthoring;` are encouraged
inside long contiguous blocks when they reduce boilerplate and make the diff
easier to review.

## Required Move

1. Add the 14 fields to `CreativeAuthoringStore` with exact existing types,
   defaults, and relative order from `ProductAppWindowState`.
2. Add whatever direct includes `CreativeAuthoringStore.hpp` needs for the moved
   types.
3. Remove the 14 flat fields from `ProductAppWindowState`.
4. Repoint only `ProductAppWindowState` storage reads/writes to
   `window.creativeAuthoring.<field>` or a clearly local `CreativeAuthoringStore`
   alias.
5. Delete the 14 old top-level ownership rows from
   `docs/god_struct_member_ownership.tsv`. Keep the existing
   `creativeAuthoring	CreativeAuthoringStore` row.
6. Mark CreativeAuthoringStore complete in
   `docs/god_struct_decomposition_target_map.md` and update
   `docs/creative_mode/builder_tasks/PRIORITY.md` to show E168-E172 complete.

## Do Not Move

Do not move `ProductRoomStore room`.

Do not move already-moved wireframe, viewport-pick, room-editor, world setup,
or ASCII room fields again.

Do not move foreign request/model/projection payload fields that happen to use
the same names. In particular, request structs and policy structs such as
`ProductMouseCapturePolicyRequest::creativeNavigateActive` remain outside the
store unless the compiler proves they are actual `ProductAppWindowState`
storage.

## Required Greps

After implementation, these must produce no output:

```sh
rg -n "window\.(creativeDocumentRevision|creativeDocumentChangedThisFrame|creativeUndo|creativeBakedRoomStale|creativeBakedRoomStaleDocumentId|creativeBakedRoomStaleRevision|creativeBakedRoomStaleStatus|creativeBakedRoomStaleReasonCode|creativeNavigateActive|creativeUiProjection|creativeUiInput|creativeUiLast|creativeUiCommand|creativeBakedRoomAutoRefresh)\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'

rg -n "\b(creativeDocumentRevision|creativeDocumentChangedThisFrame|creativeUndo|creativeBakedRoomStale|creativeBakedRoomStaleDocumentId|creativeBakedRoomStaleRevision|creativeBakedRoomStaleStatus|creativeBakedRoomStaleReasonCode|creativeNavigateActive|creativeUiProjection|creativeUiInput|creativeUiLast|creativeUiCommand|creativeBakedRoomAutoRefresh)\b" /Users/kogaryu/iggy3d/src/app/iggy3d/ProductAppWindowState.hpp

rg -n "^(creativeDocumentRevision|creativeDocumentChangedThisFrame|creativeUndo|creativeBakedRoomStale|creativeBakedRoomStaleDocumentId|creativeBakedRoomStaleRevision|creativeBakedRoomStaleStatus|creativeBakedRoomStaleReasonCode|creativeNavigateActive|creativeUiProjection|creativeUiInput|creativeUiLast|creativeUiCommand|creativeBakedRoomAutoRefresh)\b" /Users/kogaryu/iggy3d/docs/god_struct_member_ownership.tsv
```

Also run and classify remaining hits from:

```sh
rg --pcre2 -n "(?<!creativeAuthoring)\.(creativeDocumentRevision|creativeDocumentChangedThisFrame|creativeUndo|creativeBakedRoomStale|creativeBakedRoomStaleDocumentId|creativeBakedRoomStaleRevision|creativeBakedRoomStaleStatus|creativeBakedRoomStaleReasonCode|creativeNavigateActive|creativeUiProjection|creativeUiInput|creativeUiLast|creativeUiCommand|creativeBakedRoomAutoRefresh)\b" /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/tests --glob '*.cpp' --glob '*.hpp'
```

Remaining hits must be foreign payload/policy structs or intentional local
`CreativeAuthoringStore` aliases, not flat `ProductAppWindowState` storage.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

`git diff -- tests/golden/product_receipt_key_order.golden` must be empty.

## Completion Brief

Report:

- exact files changed;
- the 14 moved fields;
- receipt golden result;
- ownership coverage result;
- required grep results;
- broad old-flat-access scan classification;
- target-map and priority updates;
- full suite result;
- confirmation that `ProductRoomStore room` and already-moved fields were not
  moved again.

Do not stage, commit, push, or launch a window.
