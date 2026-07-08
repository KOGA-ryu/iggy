# E189: Product Test Infrastructure Audit

## Status

Done.

## Context

The god-struct/store decomposition is complete through E179, but the review
tail showed the next real complexity lever is test infrastructure. Store moves
were expensive because many product tests keep local copies of the same support
shapes:

- `expect(...)`, `near(...)`, and `nearlyEqual(...)`;
- `receiptFor(...)`, `expectReceiptField(...)`, and `ReceiptFieldExpectation`;
- temp-root and `ProductAppOptions` builders;
- live active-surface helpers around
  `syncProductWindowInputOwnerFromActiveSurface(...)`;
- repeated `ProductAppWindowState` fixture setup;
- local harness structs such as `StarterHarness`, `MouseDispatchHarness`, and
  `WindowInputHarness`.

Do not implement a shared harness in this card. The goal is to map the real
duplication and identify the first safe helper slice. Keep this grounded in the
current repo, not old decomposition notes.

## Scope

Read-only audit only.

Inspect product unit tests under `tests/unit/product_*.cpp`, with special focus
on the current hot spots:

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/product_gameplay_controller_tests.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_vulkan_room_frame_tests.cpp`
- `tests/unit/product_starter_menu_action_tests.cpp`
- `tests/unit/product_menu_transitions_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_wireframe_frame_tests.cpp`
- `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
- `tests/unit/product_creative_ui_projection_receipt_tests.cpp`

Also inspect `cmake/iggy3d_tests.cmake` enough to report whether the first
implementation should be header-only or should add shared test `.cpp` sources to
test targets.

## Required Inventory Commands

Run these or equivalent refined commands and include the useful results:

```sh
rg -n "\\bProductAppWindowState\\b" tests/unit/product_* --glob '*.cpp' \
  | cut -d: -f1 | sort | uniq -c | sort -nr | head -40

rg -n "^bool expect\\(|^void expect\\(|^bool near\\(|^bool nearlyEqual\\(|^iggy3d::RenderReceipt receiptFor\\(|^bool expectReceipt" \
  tests/unit/product_* --glob '*.cpp'

rg -n "syncProductWindowInputOwnerFromActiveSurface|liveSurface\\(|ProductActiveSurfaceFrame|ProductActiveSurfaceContext|activeSurface" \
  tests/unit/product_* --glob '*.cpp'

rg -n "std::filesystem::temp_directory_path\\(\\)|remove_all\\(|create_directories\\(|ProductAppOptions testOptions\\(" \
  tests/unit/product_* --glob '*.cpp'

rg -n "\\bstruct\\s+[A-Za-z0-9_]+Harness\\b|\\b[A-Za-z0-9_]*Scenario\\b|ReceiptFieldExpectation|CollisionReaderSnapshot|TapeCollisionSnapshot" \
  tests/unit/product_* --glob '*.cpp'
```

Add any narrower scans needed to classify repeated fixture builders and receipt
helpers precisely.

## Deliverable

Append the audit to this card and move it to `done/`.

The audit must include:

1. **Duplication map**
   - grouped by helper family: assertions, numeric comparisons, receipt helpers,
     temp roots/options, active-surface helpers, window fixture builders, harness
     structs, collision snapshots.
   - include file counts and the hottest files.

2. **Recommended helper shape**
   - likely path(s), for example `tests/unit/ProductTestSupport.hpp` or a more
     specific product test support header.
   - header-only vs `.cpp` recommendation with CMake reasoning.
   - what should stay local because it is too behavior-specific.

3. **First implementation cards**
   - draft at least three small follow-up cards.
   - each card must list exact files to migrate first, required test targets,
     and self-blocking criteria.
   - preferred first slices:
     - shared assertion/numeric helper;
     - shared receipt helper;
     - shared active-surface helper;
     - temp-root/options helper;
     - a later audit for `ProductAppWindowState` fixture builders if too broad.

4. **Non-goals**
   - explicitly call out why this is not a repo-wide test rewrite;
   - do not hide behavior assertions behind opaque mega-harness methods;
   - do not migrate non-product tests unless the audit proves they share the
     same product helper.

## Required Checks

Run:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

No build, no CTest, and no receipt golden run are required for this read-only
audit unless source/test/CMake files are edited, which they should not be.

## Non-Scope

- Do not edit source files.
- Do not edit test source files.
- Do not edit CMake.
- Do not create the shared helper yet.
- Do not stage, commit, push, or launch a window.
- Do not run broad CTest.

## Completion Brief Template

- Card moved to done:
- Files inspected:
- Current duplication counts/hotspots:
- Recommended helper shape:
- Draft follow-up cards:
- Self-blockers or risks:
- Checks run:
- Confirmation of no source/test/CMake/receipt golden/window changes:

## Completion Brief

- Card moved to done: yes, after appending this brief.
- Files inspected:
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `tests/unit/product_gameplay_controller_tests.cpp`
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/product_starter_menu_action_tests.cpp`
  - `tests/unit/product_menu_transitions_tests.cpp`
  - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
  - `tests/unit/product_creative_wireframe_frame_tests.cpp`
  - `tests/unit/product_creative_viewport_pick_frame_tests.cpp`
  - `tests/unit/product_creative_ui_projection_receipt_tests.cpp`
  - `tests/unit/product_creative_ui_frame_tests.cpp`
  - `tests/unit/product_creative_ui_window_frame_tests.cpp`
  - `tests/unit/product_creative_move_drag_frame_tests.cpp`
  - `tests/unit/product_active_room_collision_tests.cpp`
  - `tests/unit/product_gameplay_tape_runner_tests.cpp`
  - `tests/unit/product_frontend_router_tests.cpp`
  - `cmake/iggy3d_tests.cmake`

### Current Duplication Counts And Hotspots

Required scan results:
- `ProductAppWindowState` references: hottest files are:
  - `product_window_input_frame_tests.cpp`: 74
  - `product_gameplay_controller_tests.cpp`: 67
  - `product_creative_world_launch_tests.cpp`: 64
  - `product_creative_input_frame_tests.cpp`: 27
  - `product_creative_viewport_pick_frame_tests.cpp`: 25
  - `product_creative_wireframe_frame_tests.cpp`: 23
  - `product_vulkan_room_frame_tests.cpp`: 20
  - `product_creative_ui_input_frame_tests.cpp`: 16
  - `product_window_renderer_lifecycle_tests.cpp`: 14
  - `product_creative_ui_command_receipt_tests.cpp`: 14
- Assertion helpers:
  - 81 product test files define local `expect(...)` or `void expect(...)`.
  - This is the broadest duplication, but it is mechanically simple and low behavior risk.
- Numeric comparison helpers:
  - 15 local `near(...)`, `nearlyEqual(...)`, or `expectNear(...)` definitions across 14 files.
  - Hottest file: `product_vulkan_room_frame_tests.cpp` has 2; the rest are single definitions.
- Receipt helpers:
  - 33 receipt-helper audit hits in the grouped scan; 23 of those are direct
    reusable helper/type definitions.
  - `receiptFor(...)` appears in 8 files:
    - `product_creative_ui_command_receipt_tests.cpp`
    - `product_creative_ui_frame_tests.cpp`
    - `product_creative_ui_input_frame_tests.cpp`
    - `product_creative_ui_projection_receipt_tests.cpp`
    - `product_creative_ui_window_frame_tests.cpp`
    - `product_creative_viewport_pick_frame_tests.cpp`
    - `product_creative_wireframe_frame_tests.cpp`
    - `product_menu_transitions_tests.cpp`
  - `product_creative_ui_command_receipt_tests.cpp` is the receipt hotspot: `ReceiptFieldExpectation`, two `expectReceiptFields(...)` overloads, and multiple arrays.
- Temp roots/options:
  - 36 hits around `temp_directory_path`, `remove_all`, `create_directories`, and `ProductAppOptions testOptions(...)`.
  - Hottest files:
    - `product_window_input_frame_tests.cpp`: 5
    - `product_save_bridge_tests.cpp`: 5
    - `product_creative_world_launch_tests.cpp`: 4
    - `product_creative_no_window_bake_scenario_tests.cpp`: 4
    - `product_world_creation_tests.cpp`: 3
    - `product_window_renderer_lifecycle_tests.cpp`: 3
    - `product_starter_menu_action_tests.cpp`: 3
    - `product_save_delete_executor_tests.cpp`: 3
    - `product_automation_dispatch_tests.cpp`: 3
- Active-surface helpers:
  - 106 broad active-surface term hits from the required scan.
  - 50 helper/API subset hits after excluding generic `activeSurface`
    assertion/member references.
  - Hottest files:
    - `product_window_input_frame_tests.cpp`: 23
    - `product_menu_transitions_tests.cpp`: 13
    - `product_starter_menu_action_tests.cpp`: 6
    - `product_frontend_router_tests.cpp`: 6
    - `product_interaction_mode_state_tests.cpp`: 2
  - Three files define equivalent `liveSurface(frontend, window)` wrappers around `syncProductWindowInputOwnerFromActiveSurface(...)`.
- Window fixture builders:
  - The broad `ProductAppWindowState` reference count is too large for one support slice.
  - Repeated small fixture shapes exist (`creativeWindow()`, `markCreativeDocumentWindow(...)`, `markCreativeAppIdentity(...)`, `makeGameplayWindow(...)`), but their expected state differs enough that this should be a later focused audit.
- Harness structs and scenarios:
  - 63 hits from the harness/scenario/snapshot scan.
  - Key local harnesses:
    - `StarterHarness` in `product_starter_menu_action_tests.cpp`
    - `MouseDispatchHarness` in `product_window_input_frame_tests.cpp`
    - `WindowInputHarness` in `product_creative_move_drag_frame_tests.cpp`
  - Large behavior-specific scenarios in `product_creative_world_launch_tests.cpp`:
    - `ManualRebuildRoomScenario`
    - `GeneratedRoomShellScenario`
    - `AutoMoveScenario`
  - These are not good first shared-helper candidates because they encode test workflows, not neutral setup.
- Collision snapshots:
  - `CollisionReaderSnapshot` in `product_active_room_collision_tests.cpp`.
  - `TapeCollisionSnapshot` in `product_gameplay_tape_runner_tests.cpp`.
  - They are similar enough to revisit later, but the owner surface is active-room collision freshness, not generic product test support.

### Recommended Helper Shape

- Add a product-test support header first, not a `.cpp`:
  - Preferred path: `tests/unit/ProductTestSupport.hpp`.
  - Optional later split if it grows: `tests/unit/ProductReceiptTestSupport.hpp` and `tests/unit/ProductFilesystemTestSupport.hpp`.
- Header-only is the right first move because `cmake/iggy3d_tests.cmake` registers most tests with `iggy3d_add_unit_test(test_name source_file)`, which calls `add_executable("${test_name}" "${source_file}")`. Adding a shared `.cpp` would require either target-by-target `target_sources(...)` churn or changing the helper macro for all tests. A header-only support file lets each migration stay localized to the touched test sources.
- Suggested helper namespaces:
  - `iggy3d::test::expect(...)`
  - `iggy3d::test::near(...)` / `nearlyEqual(...)`
  - `iggy3d::test::receiptForProductWindow(...)`
  - `iggy3d::test::expectReceiptField(...)`
  - `iggy3d::test::ReceiptFieldExpectation`
  - `iggy3d::test::expectReceiptFields(...)`
  - `iggy3d::test::liveSurface(...)`
  - `iggy3d::test::cleanProductTestRoot(...)`
  - `iggy3d::test::productTestOptions(...)`
- Keep local for now:
  - Large workflow harnesses and scenarios (`GeneratedRoomShellScenario`, `AutoMoveScenario`, `ManualRebuildRoomScenario`, `StarterHarness`, `MouseDispatchHarness`) because they carry behavior assertions and route-specific state.
  - Fixture builders that encode a specific authored room, gameplay state, or creative document identity until a smaller follow-up proves a neutral helper shape.
  - Collision snapshot structs until active-room collision tests and tape runner tests agree on one shared freshness snapshot contract.

### Draft Follow-Up Cards

1. **E190: Product Test Support G1 - Assertions And Numeric Helpers**
   - Scope:
     - Add `tests/unit/ProductTestSupport.hpp`.
     - Move only generic `expect(...)`, `near(...)`, and `nearlyEqual(...)` helpers.
     - Migrate first files:
       - `tests/unit/product_camera_controller_tests.cpp`
       - `tests/unit/product_gameplay_controller_tests.cpp`
       - `tests/unit/product_vulkan_room_frame_tests.cpp`
       - `tests/unit/product_creative_fly_tests.cpp`
       - `tests/unit/product_creative_navigate_fly_tests.cpp`
   - Required targets:
     - `product_camera_controller_tests`
     - `product_gameplay_controller_tests`
     - `product_vulkan_room_frame_tests`
     - `product_creative_fly_tests`
     - `product_creative_navigate_fly_tests`
   - Self-blocking criteria:
     - Stop if a migrated file needs custom failure formatting that the helper cannot preserve.
     - Stop if namespace collisions force broad mechanical churn beyond the listed files.

2. **E191: Product Test Support G2 - Receipt Helpers**
   - Scope:
     - Extend `tests/unit/ProductTestSupport.hpp` or add `tests/unit/ProductReceiptTestSupport.hpp`.
     - Move `receiptFor(...)`, `expectReceiptField(...)`, `expectReceiptCount(...)`, `expectReceiptKey(...)`, `ReceiptFieldExpectation`, and `expectReceiptFields(...)` where shapes match.
     - Migrate first files:
       - `tests/unit/product_creative_ui_command_receipt_tests.cpp`
       - `tests/unit/product_creative_ui_projection_receipt_tests.cpp`
       - `tests/unit/product_creative_ui_frame_tests.cpp`
       - `tests/unit/product_creative_ui_window_frame_tests.cpp`
   - Required targets:
     - `product_creative_ui_command_receipt_tests`
     - `product_creative_ui_projection_receipt_tests`
     - `product_creative_ui_frame_tests`
     - `product_creative_ui_window_frame_tests`
     - `product_receipt_key_order_tests`
   - Self-blocking criteria:
     - Stop if receipt defaults differ by file in a way that would hide a setup requirement.
     - Do not change receipt golden order or values.

3. **E192: Product Test Support G3 - Active Surface Helper**
   - Scope:
     - Add shared `liveSurface(frontend, window)` wrapper around `syncProductWindowInputOwnerFromActiveSurface(...)`.
     - Migrate first files:
       - `tests/unit/product_menu_transitions_tests.cpp`
       - `tests/unit/product_starter_menu_action_tests.cpp`
       - `tests/unit/product_window_input_frame_tests.cpp` only for the top-level helper definition and a narrow group of call sites.
   - Required targets:
     - `product_menu_transitions_tests`
     - `product_starter_menu_action_tests`
     - `product_window_input_frame_tests`
     - `product_frontend_router_tests` if shared naming touches router tests.
   - Self-blocking criteria:
     - Stop if the helper starts masking production active-surface mutation side effects.
     - Keep expectations explicit; do not replace owner/status assertions with a mega-harness.

4. **E193: Product Test Support G4 - Temp Root And Options Builder**
   - Scope:
     - Add `cleanProductTestRoot(prefix, name)` and `productTestOptions(prefix, name)` helpers.
     - Migrate first files:
       - `tests/unit/product_creative_world_launch_tests.cpp`
       - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
       - `tests/unit/product_starter_menu_action_tests.cpp`
       - `tests/unit/product_automation_dispatch_tests.cpp`
   - Required targets:
     - `product_creative_world_launch_tests`
     - `product_creative_no_window_bake_scenario_tests`
     - `product_starter_menu_action_tests`
     - `product_automation_dispatch_tests`
   - Self-blocking criteria:
     - Stop if any test needs to preserve dirty temp roots for failure investigation.
     - Do not migrate `product_save_bridge_tests.cpp` first; it has save-path edge cases and parent-directory setup that are more behavior-specific.

5. **E194: Product Test Fixture Builder Audit**
   - Scope:
     - Read-only audit of `ProductAppWindowState` fixture builders after E190-E193 land.
     - Focus files:
       - `tests/unit/product_window_input_frame_tests.cpp`
       - `tests/unit/product_gameplay_controller_tests.cpp`
       - `tests/unit/product_creative_world_launch_tests.cpp`
       - `tests/unit/product_vulkan_room_frame_tests.cpp`
   - Required checks:
     - `git diff --check`.
   - Self-blocking criteria:
     - Do not implement in the audit.
     - Produce helper cards only where setup is neutral and repeated, not workflow-specific.

### Non-Goals

- This is not a repo-wide test rewrite. Product tests have many small local helpers, but only the product-specific duplicated support should move.
- Do not hide assertions behind opaque mega-harness methods. Shared helpers should reduce setup noise while keeping behavior expectations visible at the call site.
- Do not migrate non-product tests unless a later audit proves they use the exact same product helper contract.
- Do not move large scenario structs just to reduce line count; they encode end-to-end product workflows and need separate owner decisions.
- Do not add a shared support `.cpp` until there is a clear CMake plan for how every affected test target receives that source.

### Self-Blockers Or Risks

- The highest-count `ProductAppWindowState` files are also the most behavior-specific, so fixture-builder extraction should wait.
- `expect(...)` is easy to duplicate but can be noisy to migrate across 81 files; first slice should intentionally touch only a few numeric-heavy tests.
- Receipt helpers are safe but must preserve golden receipt behavior and default receipt construction exactly.
- Temp-root helpers must avoid deleting a path a test intentionally leaves for a save-path edge case.
- Active-surface helpers must stay a direct wrapper around the production resolver; no cached owner/suppression state should be reintroduced.

### Checks Run

- `git -C /Users/kogaryu/iggy3d diff --check`

### Confirmation

- No source files were edited.
- No test files were edited.
- No CMake files were edited.
- No shared helper was created.
- Receipt golden was not edited or run.
- No build or CTest was run.
- No window was launched.
