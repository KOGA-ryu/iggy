# E119: Receipt Startup/World/Room Cohesion Split

## Dependency

Do not claim this card while E118 is ready or claimed. This is the first
post-E118 receipt-decomposition follow-up from the parallel review packet.

## Objective

Split the fused `StartupWorldBuildoutFields.cpp` appender into smaller
domain-coherent receipt appenders while preserving exact receipt output order and
key/value behavior.

This is a behavior-preserving organization slice. The goal is to keep the good
ReceiptBuilder split as the new baseline while reducing the largest
key-order-adjacency file that still reads like a smaller god-file.

## Current Seam

Current file:

- `src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp`
- current size: about 354 LOC
- current single public appender:
  - `appendProductStartupWorldBuildoutFields(RenderReceipt&, const FrontendState&, const ProductAppWindowState&, const ProductSaveBridgeResult&)`
- only current call site:
  - `src/app/iggy3d/ReceiptBuilder.cpp`
- declaration:
  - `src/app/iggy3d/receipt/ReceiptFields.hpp`

Line map from current checkout:

- `StartupWorldBuildoutFields.cpp:24-97`
  - startup/dev-tools/launch/package/save scan/first-frame/vulkan timing fields
- `StartupWorldBuildoutFields.cpp:98-169`
  - world setup and world creation fields
- `StartupWorldBuildoutFields.cpp:170-243`
  - ascii room preview and activation fields
- `StartupWorldBuildoutFields.cpp:244-291`
  - room editing fields
- `StartupWorldBuildoutFields.cpp:292-351`
  - active room and active room collision fields

The review packet recommended this split first because the current file fuses
several unrelated roots under one name.

## Required Work

1. Extract contiguous field runs into named appenders and files.
   - Suggested files:
     - `src/app/iggy3d/receipt/StartupProbeFields.cpp`
     - `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
     - `src/app/iggy3d/receipt/ActiveRoomFields.cpp`
   - Accept a slightly different file split only if it is more coherent and the
     completion brief explains why.
2. Keep public `appendProductStartupWorldBuildoutFields(...)` as the
   orchestration appender for now, unless removing it causes less churn.
   - It should call the new appenders in the exact same order as today.
   - This preserves `ReceiptBuilder.cpp` as already-stable orchestration.
3. Preserve exact receipt key order.
   - No reordering within field runs.
   - No key renames.
   - No status/reason/value policy changes.
4. Keep appenders pure emitters.
   - Do not add policy computation.
   - Do not read fields back from `RenderReceipt`.
   - Do not mutate `ProductAppWindowState`.
5. Update CMake source lists if new `.cpp` files require it.
6. Update `ReceiptFields.hpp` minimally.
   - It is acceptable for this slice to add new declarations there.
   - Do not do the broader forward-declaration/slim-header pass in this card.

## Verification Gate

Run the focused build/tests that cover product receipts. Use the narrowest green
set available, but include receipt-order protection if present in the tree.

Suggested commands:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_ui_command_receipt_tests product_creative_ui_projection_receipt_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_command_receipt_tests|product_creative_ui_projection_receipt_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

If a deterministic receipt key-order oracle exists in the repo or handoff docs,
run it and report the before/after result. If no oracle exists, state that
explicitly and rely on focused tests plus exact contiguous body cuts.

## Do Not

- Do not change receipt field names, values, or order.
- Do not change `ProductAppWindowState`.
- Do not change gameplay, Creative, render, save/load, mutation, RoomBake,
  input, or Vulkan behavior.
- Do not split `SaveStateFields`, `GameplaySceneStateFields`, or
  `FeedbackSurfaceAutomationVulkanFields` in this card.
- Do not do the `ReceiptFields.hpp` forward-declaration slim pass here.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Completion Brief

Append:

- Files changed:
- Split shape:
- Receipt order proof:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief

- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/receipt/ReceiptFields.hpp`
  - `src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp`
  - `src/app/iggy3d/receipt/StartupProbeFields.cpp`
  - `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
  - `src/app/iggy3d/receipt/ActiveRoomFields.cpp`
- Split shape:
  - `StartupWorldBuildoutFields.cpp` is now only the public orchestration appender.
  - `appendProductStartupProbeFields(...)` owns the startup/dev-tools/launch/package/save-scan/first-frame/Vulkan timing run.
  - `appendProductWorldAuthoringFields(...)` owns world setup, world creation, ASCII room preview/activation, and room-editing receipt fields.
  - `appendProductActiveRoomFields(...)` owns active-room and active-room-collision receipt fields.
- Receipt order proof:
  - No deterministic receipt-order test target was found.
  - Verified mechanically with `diff -u <(git show HEAD:src/app/iggy3d/receipt/StartupWorldBuildoutFields.cpp | perl -ne 'print "$1\n" if /appendReceiptField\\(receipt, "([^"]+)"/') <(perl -ne 'print "$1\n" if /appendReceiptField\\(receipt, "([^"]+)"/' src/app/iggy3d/receipt/StartupProbeFields.cpp src/app/iggy3d/receipt/WorldAuthoringFields.cpp src/app/iggy3d/receipt/ActiveRoomFields.cpp)`, which produced no diff.
  - The public appender calls the new appenders in the same contiguous order as the original monolithic body.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_ui_command_receipt_tests product_creative_ui_projection_receipt_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_ui_command_receipt_tests|product_creative_ui_projection_receipt_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched files.
- Concerns/deferred:
  - `ReceiptFields.hpp` remains broad; the requested forward-declaration/slim-header pass is still intentionally deferred.
  - `WorldAuthoringFields.cpp` still contains multiple related world-authoring subdomains, but this keeps the split to the card's three requested files while separating runtime active-room fields cleanly.
