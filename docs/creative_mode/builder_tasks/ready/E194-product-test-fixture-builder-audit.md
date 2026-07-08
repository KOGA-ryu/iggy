# E194: Product Test Fixture Builder Audit

## Status

Ready.

## Context

E190-E193 landed the low-risk shared product test helpers:

- `ProductTestSupport.hpp` for assertions/numeric comparisons;
- `ProductReceiptTestSupport.hpp` for receipt helpers;
- `ProductActiveSurfaceTestSupport.hpp` for active-surface wrapper tests;
- `ProductFilesystemTestSupport.hpp` for clean temp roots/options.

Do not guess the next fixture abstraction. The remaining duplication is
larger and more behavior-shaped, especially around `ProductAppWindowState`,
sessions, active rooms, controller fixtures, window input fixtures, and
creative launch scenarios.

This card is read-only cartography. It should produce the next implementation
card only if the audit finds a small, neutral helper with clear ownership.

## Scope

Audit only these high-duplication product test files:

- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/product_gameplay_controller_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_vulkan_room_frame_tests.cpp`

You may inspect already-added support headers for context:

- `tests/unit/ProductTestSupport.hpp`
- `tests/unit/ProductReceiptTestSupport.hpp`
- `tests/unit/ProductActiveSurfaceTestSupport.hpp`
- `tests/unit/ProductFilesystemTestSupport.hpp`

## Required Inventory

For each scoped test file, report:

- count of `ProductAppWindowState` declarations/usages;
- local helpers that construct a window or session, for example
  `makeGameplayWindow(...)`, `editingWindow(...)`, `gameplayWindow(...)`;
- local scenario/harness/snapshot structs, for example
  `MouseDispatchHarness`, `ManualRebuildRoomScenario`,
  `GeneratedRoomShellScenario`, `AutoMoveScenario`;
- helpers that seed active rooms, collision, movement/debug facts, creative
  documents, or UI state;
- whether each helper is neutral fixture setup or behavior-coupled test policy.

## Classification Buckets

Classify candidates into these buckets:

1. **Shared Now Candidate**: small, neutral setup repeated across files, no
   behavior policy hidden, likely header-only, no CMake change.
2. **Local By Design**: scenario/harness/helper encodes the behavior under test
   and should remain local.
3. **Needs Smaller Preflight**: promising but requires a focused audit or guard
   before implementation.
4. **Do Not Share**: would create a mega-harness or obscure important
   assertions.

## Required Commands

Run and include summarized results:

```sh
rg -n "ProductAppWindowState|make.*Window|.*Window\\(|struct .*Harness|struct .*Scenario|struct .*Snapshot|activeRoom|collision|Session|Facade|CreativeAppState" \
  /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_gameplay_controller_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_creative_world_launch_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_vulkan_room_frame_tests.cpp

rg -n "ProductAppWindowState" \
  /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_gameplay_controller_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_creative_world_launch_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_vulkan_room_frame_tests.cpp

git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required because this card is read-only.

## Output Requirements

Append the audit to this card and move it to `done/`.

The completion brief must include:

- current counts/hotspots after E190-E193;
- candidate helper list by classification bucket;
- explicit "do not share" list for large behavior harnesses;
- whether a follow-up implementation card was drafted;
- if drafted, exact proposed scope, files, helper API, and self-blockers.

If no safe implementation candidate exists, say that directly and do not create
a follow-up card.

## Non-Scope

- Do not edit source or test files.
- Do not add a fixture helper.
- Do not edit CMake.
- Do not run broad CTest.
- Do not edit receipt golden.
- Do not stage, commit, push, or launch a window.

## Completion Brief Template

- Card moved to done:
- Files inspected:
- Current counts/hotspots:
- Classification buckets:
- Follow-up card recommendation:
- Commands run:
- Concerns/deferred:
