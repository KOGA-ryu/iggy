# E189: Product Test Infrastructure Audit

## Status

Ready.

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
