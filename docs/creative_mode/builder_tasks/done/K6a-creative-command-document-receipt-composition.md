# K6a - Creative Command Document Receipt Composition

## Status

READY. K6 batch step 1 of 2. Claim only after K5 is accepted; accepted baseline
is `3bda2139`. Authority:
`docs/creative_mode/builder_tasks/blocked/K6-creative-ui-command-receipt-composition-plan.md`.

On green completion, commit this card, move it to `done/`, and continue to K6b
without waiting for Reviewer. A STOP pauses the whole K6 batch.

## Goal

Compose mutation, create, remove, and undo native receipts into the frame
receipt, install the single diagnostics conversion owner, and preserve every
external product receipt value.

## Exact Files

Create and register:

- `src/app/iggy3d/creative/CreativeUiCommandDiagnostics.cpp`

Update only:

- `CMakeLists.txt`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.hpp`
- `src/app/iggy3d/creative/bridge/UiCommandFrame.cpp`
- `src/app/iggy3d/creative/CreativeUiCommandDiagnostics.hpp`
- `src/app/iggy3d/receipt/CreativeReceiptRecording.cpp`
- `tests/unit/product_creative_ui_command_frame_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_pick_flow_tests.cpp`
- `tests/unit/product_starter_menu_action_tests.cpp`

Do not edit room-shell kernels, diagnostic field structs, ProductAppWindowState,
receipt emitters/key order, goldens, policy, or any other test/source.

## Required Intermediate Shape

Add exactly the parent plan's `ProductCreativeUiCommandCreateOutcome` and
`ProductCreativeUiCommandRemoveOutcome`. Replace the 55 flat
mutation/create/delete/undo members with:

- `creative::CreativeFacadeMutationReceipt mutation`;
- `ProductCreativeUiCommandCreateOutcome create`;
- `ProductCreativeUiCommandRemoveOutcome remove`;
- `creative::CreativeDocumentUndoApplyReceipt undo`.

Keep all 13 flat shell fields unchanged for K6b. The intermediate frame receipt
has exactly 32 direct members.

Handlers assign native receipts directly. Remove `copyMutationReceipt`,
`copyCreateReceipt`, `copyDeleteReceipt`, and `copyUndoReceipt`.

For no selection, set `remove.noSelection`, native `requested`, unchanged
revisions, message, and reason as specified by the parent. Do not call Facade.

For create, store native document receipt plus placement metadata. Move fixed
two-decimal formatting out of UiCommandFrame and into the diagnostics converter.

## Diagnostics Owner

Add the exact public `toProductCreativeUiCommandDiagnostics(...)` declaration
from the parent. The new cpp owns all command-to-diagnostic mapping, including a
temporary private mapping of the still-flat shell fields. Its private converters
must preserve:

- unrequested create/delete `none` defaults;
- no-selection delete status `NoSelection`;
- existing enum/string mappings;
- second-default-room placement message
  `object_created placement_offset_x=1.00` and reason
  `object_created_placement_offset`.

Replace command diagnostic copy helpers in CreativeReceiptRecording with one
whole-value assignment from the converter. Preserve refresh reset and last
command cache timing; retarget create/mutation reads to composed fields. Delete
the now-single-use local creative tool string mapper.

## Test Migration And Pins

Mechanically retarget existing assertions:

- mutation -> `receipt.mutation.*` using native field names;
- create -> `receipt.create.document.*` plus placement metadata;
- delete -> `receipt.remove.document.*` plus `remove.noSelection`;
- undo -> `receipt.undo.*`.

Do not weaken or delete assertions. Add:

1. a frame pin that the second default room create records offset-applied and
   `placementOffsetX == 1.0` while its native message remains `object_created`;
2. a receipt pin for the exact enriched placement message/reason;
3. a receipt pin that delete-without-selection remains requested, rejected,
   status `NoSelection`, and message/reason `no_selection`.

## Mechanical Checkpoint

- `UiCommandFrame.hpp` has 32 direct frame members.
- The four old copy helpers and all old non-shell flat fields are absent.
- `CreativeUiCommandDiagnostics.cpp` is the sole frame-to-diagnostics owner and
  is no more than 350 lines.
- Production source count is 703.
- Dependency graph remains 317 direct edges, 16/16 pairs, app fan-out 188, zero
  SCCs, zero violations.
- No receipt key/order/default/applied expectation changes.

## Focused Verification

    cmake -S . -B build
    cmake --build build --target iggy3d_app product_creative_ui_command_frame_tests product_creative_ui_command_receipt_tests product_creative_pick_flow_tests product_starter_menu_action_tests product_creative_ui_window_frame_tests product_window_input_frame_tests product_receipt_key_order_tests
    ctest --test-dir build -R '^(product_creative_ui_command_frame_tests|product_creative_ui_command_receipt_tests|product_creative_pick_flow_tests|product_starter_menu_action_tests|product_creative_ui_window_frame_tests|product_window_input_frame_tests|product_receipt_key_order_tests|dependency_direction_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    awk '/struct ProductCreativeUiCommandFrameReceipt/{inside=1; next} inside && /^};/{print count; exit} inside && /;[[:space:]]*$/{count++}' src/app/iggy3d/creative/bridge/UiCommandFrame.hpp
    wc -l src/app/iggy3d/creative/CreativeUiCommandDiagnostics.cpp
    rg -n 'copy(Mutation|Create|Delete|Undo)Receipt' src/app/iggy3d/creative/bridge/UiCommandFrame.cpp
    rg -n 'mutationRequested|createRequested|deleteRequested|undoRequested' src/app/iggy3d/creative/bridge/UiCommandFrame.hpp
    git diff -- tests/golden
    git diff --check

Both greps and the golden diff must return no output. Report exact source/graph
metrics and the 32-member count.

## Stop Conditions

- Native Facade/Document/Undo receipt definitions need edits.
- Placement or no-selection external strings cannot remain exact without a
  compatibility copy field.
- A non-listed production consumer reads a removed field.
- Shell fields/handlers must change in this slice.
- Receipt expectations need repinning, the member count differs from 32, graph
  differs from 703/317/188/16/16, or a focused test regresses.

## Completion Brief

Append final intermediate type shape, deleted copy helpers, converter ownership,
placement/no-selection pins, receipt preservation evidence, exact graph, and
focused tests. Commit and continue to K6b without per-slice review.
