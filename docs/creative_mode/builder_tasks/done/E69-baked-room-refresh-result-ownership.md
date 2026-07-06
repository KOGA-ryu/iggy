# E69: Baked Room Refresh Result Ownership

## Objective

Move the Creative baked-room refresh request/result DTOs to an ownership seam
that does not make `ReceiptBuilder.cpp` depend on `Operations.hpp`.

## Problem

The baked-room refresh result is produced by product operations, recorded by
input-frame diagnostics, mirrored into launch/open results, and copied into
receipt/window fields. Today its type lives in `Operations.hpp`.

That forces `ReceiptBuilder.cpp` to include `Operations.hpp` just to inspect the
refresh result fields, while `Operations.hpp` already includes
`ReceiptBuilder.hpp` for `ProductAppWindowState` and related types. This is a
layering knot: receipt construction should not need the full operations API, and
operations should not become the home for DTOs consumed by receipt builders and
input orchestration.

## Evidence

- `src/app/iggy3d/Operations.hpp:56` defines
  `ProductCreativeBakedActiveRoomRefreshRequest`.
- `src/app/iggy3d/Operations.hpp:64` defines
  `ProductCreativeBakedActiveRoomRefreshResult`.
- `src/app/iggy3d/Operations.hpp:15` includes `ReceiptBuilder.hpp`.
- `src/app/iggy3d/ReceiptBuilder.hpp:35` forward-declares the refresh result.
- `src/app/iggy3d/ReceiptBuilder.cpp:18` includes `Operations.hpp` so it can
  read the result fields.
- `src/app/iggy3d/ReceiptBuilder.cpp:118` copies individual refresh result
  fields into manual and auto refresh diagnostics.
- `src/app/iggy3d/window/InputFrame.cpp:1060` and `:1256` construct refresh
  requests and pass refresh results to receipt mirror helpers.

## Required Reads

- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `docs/creative_mode/builder_tasks/ready/E44-product-creative-bake-refresh-service.md`
- `docs/creative_mode/builder_tasks/ready/E47-creative-command-diagnostic-subreceipts.md`

## Scope

- Move `ProductCreativeBakedActiveRoomRefreshRequest` and
  `ProductCreativeBakedActiveRoomRefreshResult` to a small shared header with a
  name that reflects the product/creative baked-room seam.
- Update `Operations.hpp`, `ReceiptBuilder.hpp/.cpp`, and `InputFrame.cpp` to
  include that header instead of forcing ReceiptBuilder through Operations.
- Keep public behavior, field names, defaults, and receipt keys unchanged.
- This may be done before or alongside E44, but it should not require the full
  refresh service extraction to be correct.

## Acceptance

- `ReceiptBuilder.cpp` no longer includes `app/iggy3d/Operations.hpp`.
- `Operations.hpp` no longer needs to be the owner of the baked-room refresh DTO
  definitions.
- Launch/open/manual/auto refresh tests still observe identical result fields
  and receipt diagnostics.
- No RoomBake behavior, no-renderable policy, active-room install behavior, or
  command routing changes.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_world_launch_tests product_creative_ui_command_receipt_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_world_launch_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change refresh semantics.
- Do not combine this with a bake cache.
- Do not move all Operations result types in one broad refactor.
- Do not remove receipt diagnostics or rename external receipt keys.

## Completion Brief

- Status: done.
- Files modified/created:
  - `src/app/iggy3d/ProductCreativeBakedRoomRefresh.hpp`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/ReceiptBuilder.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
- Implementation:
  - Moved `ProductCreativeBakedActiveRoomRefreshRequest` and
    `ProductCreativeBakedActiveRoomRefreshResult` from `Operations.hpp` into the
    new shared `ProductCreativeBakedRoomRefresh.hpp` seam.
  - Updated `Operations.hpp` to include the shared DTO header while preserving
    public result fields, defaults, and launch/open refresh mirrors.
  - Updated `ReceiptBuilder.hpp/.cpp` so receipt diagnostics can inspect refresh
    result fields without including `Operations.hpp`.
  - Added an explicit DTO include in `InputFrame.cpp` for manual/auto refresh
    request/result construction.
  - No RoomBake behavior, no-renderable policy, active-room installation, command
    routing, or receipt key semantics changed.
- Verification:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_world_launch_tests product_creative_ui_command_receipt_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_world_launch_tests|product_creative_ui_command_receipt_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing whitespace scan over touched files
- Result: all passed.
