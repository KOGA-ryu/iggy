# E210: Operations Split G1 - Creative Baked Active-Room Refresh

## Status

Done.

## Context

E209 audited `Operations.cpp` and recommended the creative baked active-room
refresh service as the safest first Operations split. It is cohesive, already
has focused coverage, and has a small direct caller set.

This is an implementation move only. Preserve behavior byte-for-byte at the
receipt/test level; do not redesign the refresh policy.

## Objective

Move `refreshProductCreativeBakedActiveRoom(...)` and its private service
implementation out of `Operations.cpp` into a creative-owned implementation
file.

## Target Shape

Add:

- `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp`
- `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp`

Keep:

- request/result structs in
  `src/app/iggy3d/ProductCreativeBakedRoomRefresh.hpp`
- API namespace as `iggy3d`
- function name/signature exactly:

  ```cpp
  ProductCreativeBakedActiveRoomRefreshResult refreshProductCreativeBakedActiveRoom(
      const ProductCreativeBakedActiveRoomRefreshRequest& request,
      std::optional<Session>& activeSession,
      ProductAppWindowState& window,
      const creative::CreativeAppState& creativeApp);
  ```

Expected header style:

- include `<optional>`
- include `app/iggy3d/ProductCreativeBakedRoomRefresh.hpp`
- forward-declare `Session`, `ProductAppWindowState`, and
  `creative::CreativeAppState`

## Scope

Allowed edits:

- `CMakeLists.txt`
- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp`
- `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp`
- direct callers that need the new header:
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
  - any other compile fallout that directly calls the refresh function
- this task card

Do not move creative new/open/save world operations in this slice.

Do not move save-slot/delete/recover operations in this slice.

Do not move package/session bootstrap, new-world launch, world-template
helpers, or `ProductCreativeBakedRoomRefresh.hpp` request/result structs.

Do not change receipt keys/order/values, status/reason strings, activation
hook behavior, active-room freshness behavior, `ProductAppWindowState` storage,
renderer/Vulkan/projection code, save/load format, or CMake test definitions.

## Required Move

Move from `Operations.cpp` to the new `.cpp`:

- `kCreativeRoomBakeNoRenderableObjects`
- `kProductCreativeBakedRoomClearedNoRenderableObjects`
- `setCreativeBakedActiveRoomRefreshStatus(...)`
- `fallbackString(...)`
- `clearedCreativeBakedActiveRoom(...)`
- `ProductCreativeBakedRoomRefreshService`
- `refreshProductCreativeBakedActiveRoom(...)`

Timing helper policy:

- Do not move the existing `Operations.cpp::elapsedMicroseconds(...)` helper
  if other Operations paths still use it.
- Add a file-local equivalent in `BakedActiveRoomRefresh.cpp` if needed for
  bake timing.

Declaration/caller policy:

- Remove the refresh function declaration from `Operations.hpp`.
- Declare it in `creative/BakedActiveRoomRefresh.hpp`.
- Include the new header in `Operations.cpp` for the creative launch/open
  internal calls.
- Include the new header in direct external callers/tests that call
  `refreshProductCreativeBakedActiveRoom(...)`.
- Do not leave a forwarding wrapper in `Operations.cpp`.

CMake policy:

- Add `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp` to the `iggy3d`
  library source list near the other creative app sources.

## Behavior Preservation Requirements

Preserve exactly:

- default activation hook fallback to `activateCreativeReasoningGraph`;
- caller-provided activation hook override;
- inactive/session-missing/document-invalid rejection behavior;
- accepted bake install behavior;
- no-renderable `clearOnNoRenderable` clear behavior;
- active room revision bumping;
- `ensureActiveRoomCollisionFresh(...)` usage;
- collision/readiness fields mirrored into the result;
- stale/fresh creative baked-room receipt recording through existing callers;
- accepted/rejected status and reason strings;
- timing diagnostics still measured for bake execution.

## Required Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_creative_ui_input_frame_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_creative_ui_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure
rg -n "refreshProductCreativeBakedActiveRoom\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files and this card.

Expected grep shape:

- no declaration/definition remains in `Operations.hpp`;
- `Operations.cpp` may still have call sites for creative launch/open;
- the declaration lives in `BakedActiveRoomRefresh.hpp`;
- the definition lives in `BakedActiveRoomRefresh.cpp`.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- extracting the service requires changing the refresh function signature;
- circular includes require moving unrelated Operations APIs;
- focused tests reveal receipt/status/collision behavior drift;
- CMake/test fallout expands beyond direct caller include repair.

## Completion Brief Requirements

Report:

- files changed;
- new header/source names;
- exact helpers moved out of `Operations.cpp`;
- callers given the new include;
- CMake source-list update;
- final `refreshProductCreativeBakedActiveRoom(...)` grep classification;
- focused build/CTest results;
- receipt golden diff result;
- `diff --check` and whitespace-scan results;
- confirmation that no creative launch/save operations, save-slot operations,
  package/session launch, request/result structs, receipt keys, status strings,
  CMake test definitions, renderer/Vulkan/projection, save/load format,
  staging, commit, push, or window launch were changed.

## Completion Brief

- Card moved to done: yes.
- Files changed:
  - `CMakeLists.txt`
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp`
  - `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E210-operations-split-g1-creative-baked-room-refresh.md`
- New header/source:
  - `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp`
  - `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp`
- Helpers moved out of `Operations.cpp`:
  - `kCreativeRoomBakeNoRenderableObjects`
  - `kProductCreativeBakedRoomClearedNoRenderableObjects`
  - `setCreativeBakedActiveRoomRefreshStatus(...)`
  - `fallbackString(...)`
  - `clearedCreativeBakedActiveRoom(...)`
  - `ProductCreativeBakedRoomRefreshService`
  - `refreshProductCreativeBakedActiveRoom(...)`
  - A file-local `elapsedMicroseconds(...)` equivalent was added in the new
    source for bake timing; the existing `Operations.cpp` helper stayed in
    place for other Operations paths.
- Callers given the new include:
  - `src/app/iggy3d/Operations.cpp` for creative launch/open internal calls.
  - `src/app/iggy3d/window/InputFrame.cpp` for manual/auto refresh calls.
  - `tests/unit/product_creative_world_launch_tests.cpp` for direct refresh
    calls.
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp` for direct
    refresh calls.
- CMake source-list update:
  - Added `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp` to the `iggy3d`
    library source list near the other creative app sources.
- Final `refreshProductCreativeBakedActiveRoom(...)` grep classification:
  - `Operations.hpp`: no hits.
  - `Operations.cpp`: two remaining call sites for creative launch/open.
  - `creative/BakedActiveRoomRefresh.hpp`: declaration.
  - `creative/BakedActiveRoomRefresh.cpp`: definition.
- Focused build:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_creative_ui_input_frame_tests product_receipt_key_order_tests -j10`
  - Result: passed.
- Focused CTest:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_creative_ui_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure`
  - Result: 4/4 passed.
- Receipt golden diff:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden`
  - Result: empty.
- Diff/whitespace checks:
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Result: passed.
  - Focused trailing-whitespace scan over touched files and this card.
  - Result: clean.
- Confirmed unchanged:
  - No creative launch/save operations were moved.
  - No save-slot/delete/recover operations were moved.
  - No package/session launch or world-template helpers were moved.
  - `ProductCreativeBakedRoomRefresh.hpp` request/result structs were not
    moved.
  - No receipt keys/order/values, status/reason strings, CMake test
    definitions, renderer/Vulkan/projection code, save/load format, staging,
    commit, push, or window launch changes were made.
