# K6b - Creative Command Shell Receipt Composition

## Status

READY. K6 batch step 2 of 2. Claim only after K6a is committed and in `done/`,
with its report at 32 frame members and graph 703/317/16/16, app fan-out 188.
Authority:
`docs/creative_mode/builder_tasks/blocked/K6-creative-ui-command-receipt-composition-plan.md`.

This is the K6 closeout. Reviewer receives one aggregate two-commit brief only
after this card is committed, unless K6a or K6b STOPs.

## Goal

Replace the remaining flat room-shell payload with native build/remove
composition and close K6 at the exact 20-member frame receipt.

## Exact Files

Update only:

- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/creative/CreativeUiCommandDiagnostics.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`

Do not edit CMake, the diagnostics header/structs, CreativeReceiptRecording,
native RoomShell types/kernels, K6a document outcomes, policy, receipt emitters,
goldens, or other tests/source.

## Required Final Shape

Add exactly the parent plan's `ProductCreativeUiCommandRoomShellOutcome` and
replace all 13 `shell*` flat fields with one `shell` member. The final frame
receipt has exactly the 20 direct members listed in the parent.

Generate handler:

- assign `shell.build` once from the native build result;
- keep `shell.remove` default/unrequested;
- preserve generated/floor/wall counts;
- store only changed/revision apply facts in the wrapper;
- on create/install failure, mutate the build receipt's accepted/status/reason/
  message to the existing final result while retaining its request/count facts.

Remove handler mirrors this using `shell.remove` and leaves `shell.build`
default. On staged-remove/install failure, mutate only the selected remove
receipt's final status fields. Exactly one native shell receipt is requested.

Delete `copyRoomShellReceipt`, `copyRoomShellRemoveReceipt`, and the old flat
apply-status helper. Do not add an operation enum or compatibility accessor.

## Diagnostics And Tests

Finish the private shell `toDiagnostics(...)` conversion:

- neither requested -> current not-requested defaults;
- build requested -> generated counts and build status strings;
- remove requested -> removed counts and remove status strings;
- accepted comes from the selected native receipt;
- changed/revisions come from the wrapper.

Retarget shell assertions to `receipt.shell.build.*`,
`receipt.shell.remove.*`, and wrapper changed/revisions. Preserve all existing
assertions and add explicit opposite-receipt-unrequested assertions for one
successful generate and one successful remove.

## Final Checkpoint

- exactly 20 direct frame members;
- zero old prefixed mutation/create/delete/undo/shell payload fields;
- zero native-to-flat receipt copy helpers in UiCommandFrame.cpp;
- `CreativeUiCommandDiagnostics.cpp` remains no more than 350 lines;
- source count 703; graph 317 direct, 16/16 pairs, app 188, zero SCCs and
  violations;
- receipt key order, default fields, generated-shell values, removed-shell
  values, and all non-shell values remain unchanged.

## Focused Verification

    cmake --build build --target iggy3d_app product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests product_creative_pick_flow_tests product_starter_menu_action_tests product_creative_ui_window_frame_tests product_window_input_frame_tests product_receipt_key_order_tests creative_room_shell_tests
    ctest --test-dir build -R '^(product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests|product_creative_pick_flow_tests|product_starter_menu_action_tests|product_creative_ui_window_frame_tests|product_window_input_frame_tests|product_receipt_key_order_tests|creative_room_shell_tests|dependency_direction_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    awk '/struct ProductCreativeUiCommandFrameReceipt/{inside=1; next} inside && /^};/{print count; exit} inside && /;[[:space:]]*$/{count++}' src/app/iggy3d/creative/bridge/UiCommandFrame.hpp
    wc -l src/app/iggy3d/creative/CreativeUiCommandDiagnostics.cpp
    rg -n 'copy(Mutation|Create|Delete|Undo|RoomShell)Receipt' src/app/iggy3d/creative/bridge/UiCommandFrame.cpp
    rg -n 'mutationRequested|createRequested|deleteRequested|undoRequested|shellRequested|shellAccepted|shellChanged|shellRoomObjectId|shellGeneratedObjectCount|shellRemovedObjectCount|shellFloorCount|shellWallCount|shellRevisionBefore|shellRevisionAfter|shellStatus|shellReasonCode|shellMessage' src/app/iggy3d/creative/bridge/UiCommandFrame.hpp
    git diff -- tests/golden
    git diff --check

Both greps and the golden diff must return no output. Report the exact 20-member
list and live graph, not only test pass.

## Stop Conditions

- Both native shell receipts can become requested in one command.
- Preserving downstream failure status requires changing a native RoomShell
  type or adding duplicate accepted/status/count fields.
- CreativeReceiptRecording, diagnostics structs/header, CMake, or a non-listed
  source/test needs edits.
- Any receipt value/key/order changes, final member count differs from 20,
  graph differs from 703/317/188/16/16, or a focused test regresses.

## Completion Brief

Append final type/member list, selected-native-receipt evidence for generate and
remove, deleted helper grep, exact receipt/graph/test results, and protected-file
evidence. Commit, then send Reviewer one aggregate K6 brief listing both commits
and request milestone code review.
