# E194: Product Test Fixture Builder Audit

## Status

Done.

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

## Completion Brief

- Card moved to done: yes
- Files inspected:
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `tests/unit/product_gameplay_controller_tests.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/ProductTestSupport.hpp`
  - `tests/unit/ProductReceiptTestSupport.hpp`
  - `tests/unit/ProductActiveSurfaceTestSupport.hpp`
  - `tests/unit/ProductFilesystemTestSupport.hpp`
- Current counts/hotspots:
  - `product_window_input_frame_tests.cpp`: 73 `ProductAppWindowState` occurrences on 73 matching lines; 30 `Session` occurrences. Main fixtures are `editingWindow()` at line 129, `gameplayWindow(...)` at line 256, `setInputFrameClamberActiveRoom(...)` at line 238, and `MouseDispatchHarness` at line 347. `editingWindow()` seeds room-editor state from a small authored ASCII room; `gameplayWindow(...)` activates a gameplay ASCII room and sets Player interaction mode; both are behavior-specific to input-frame routing.
  - `product_gameplay_controller_tests.cpp`: 67 `ProductAppWindowState` occurrences on 67 matching lines; 66 `Session` occurrences. `makeGameplayWindow(...)` at line 56 is used 58 times and activates a controller-specific ASCII room. Active-room seeders at lines 293, 310, and 328 install authored clamber, wall-jump, and layered-floor room/collision policy. `runManualMove(...)` at line 423 and `seedAirborneWallRunSetup(...)` at line 1319 encode movement behavior, not neutral fixture setup.
  - `product_creative_world_launch_tests.cpp`: 64 `ProductAppWindowState` occurrences on 64 matching lines; 61 `Session` occurrences. Hotspots are launch/open helpers at lines 85 and 100, sentinel active-room helpers at lines 245 and 250, creative input-frame click helpers at lines 312-420, and scenario structs `ManualRebuildRoomScenario` at line 2164, `GeneratedRoomShellScenario` at line 2469, and `AutoMoveScenario` at line 3663. This file also has 94 `activeRoom(...)` and 34 `activeRoomCollision(...)` hits, all tied to bake/undo/delete/refresh assertions.
  - `product_vulkan_room_frame_tests.cpp`: 20 `ProductAppWindowState` occurrences on 20 matching lines; 20 `Session` occurrences. `makeGameplayWindow(...)` at line 103 is used 17 times and sets a Vulkan-room ASCII fixture plus camera yaw/pitch. `seedMovementDebugFacts(...)` at line 151 and physics seeders at lines 141 and 165 are render/projection diagnostic setup.
  - Existing support headers after E190-E193 are intentionally small: assertions/numerics, receipt field checks, active-surface wrapper, and clean temp-root/options helpers. None currently hides window/session behavior.
- Classification buckets:
  - **Shared Now Candidate**: none. No remaining fixture builder is small and neutral enough to promote immediately without hiding behavior under test.
  - **Local By Design**:
    - `MouseDispatchHarness` in `product_window_input_frame_tests.cpp` owns opening-menu input dispatch context and should remain local.
    - `ManualRebuildRoomScenario`, `GeneratedRoomShellScenario`, and `AutoMoveScenario` in `product_creative_world_launch_tests.cpp` encode creative launch, UI command routing, stale/auto-refresh, undo, and active-room assertions. Sharing them would create a behavior harness.
    - `editingWindow()` / `gameplayWindow(...)` in `product_window_input_frame_tests.cpp` are tied to input-frame surface policy, room-editor state, and Player interaction mode.
    - `setClamberActiveRoom(...)`, `setWallJumpActiveRoom(...)`, `setLayeredFloorActiveRoom(...)`, `makeProductClamberRoom()`, `makeProductWallJumpRoom(...)`, and `makeProductLayeredFloorRoom()` in `product_gameplay_controller_tests.cpp` are movement-mechanic fixtures.
    - `seedMovementDebugFacts(...)`, `seedPhysicsMovementStats(...)`, and `seedPhysicsMovementStatsAndGeometry(...)` in `product_vulkan_room_frame_tests.cpp` are render/debug diagnostic fixtures.
  - **Needs Smaller Preflight**:
    - The three ASCII-activated gameplay window helpers (`product_gameplay_controller_tests.cpp::makeGameplayWindow`, `product_vulkan_room_frame_tests.cpp::makeGameplayWindow`, and `product_window_input_frame_tests.cpp::gameplayWindow`) are similar but not identical. They differ by authored room text/id/source, camera yaw/pitch, and interaction-mode setup. A future preflight could compare whether a tiny `activateAsciiRoomWindowForTest(...)` helper is neutral, but this card does not recommend implementing it yet.
    - Active-room state installation repeats `loaded/status/reason/source/count/collision` boilerplate in multiple files, but the room payloads and expected counts are mechanism-specific. This needs a focused active-room fixture audit before any shared helper.
  - **Do Not Share**:
    - A generic `ProductAppWindowState` mega-builder spanning these files.
    - A combined `Session + ProductAppWindowState + FrontendState + CreativeAppState` harness.
    - Any helper that combines launch/open, creative UI click routing, stale flags, undo stack, active-room bake, and collision assertions.
    - Any helper that hides explicit `activeRoom(...)`, `activeRoomCollision(...)`, gameplay movement, or render-projection assertions.
- Follow-up card recommendation:
  - No implementation card drafted. The audit did not find a small neutral helper with clear ownership.
  - If the next planner wants to continue, the safest next card is a read-only preflight titled `Product Test ASCII Room Window Fixture Preflight`, scoped only to the three ASCII activation helpers listed above, with a decision on whether a tiny header-only helper can accept explicit room text/id/source/camera/mode inputs without hiding assertions. That is not an implementation recommendation yet.
- Commands run:
  - `rg -n "ProductAppWindowState|make.*Window|.*Window\\(|struct .*Harness|struct .*Scenario|struct .*Snapshot|activeRoom|collision|Session|Facade|CreativeAppState" /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_gameplay_controller_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_creative_world_launch_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_vulkan_room_frame_tests.cpp`
  - `rg -n "ProductAppWindowState" /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_gameplay_controller_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_creative_world_launch_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_vulkan_room_frame_tests.cpp`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Additional read-only grouping commands for occurrence counts, struct/helper inventories, repeated window-construction call counts, and support-header context.
- Concerns/deferred:
  - No source, test, CMake, receipt golden, or production docs were edited.
  - No helper was added because the remaining duplication is behavior-shaped.
  - No build or CTest was run; E194 is read-only and required only `diff --check`.
  - No staging, commit, push, or window launch was performed.
