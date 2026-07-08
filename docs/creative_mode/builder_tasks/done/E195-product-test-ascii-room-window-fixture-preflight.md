# E195: Product Test ASCII Room Window Fixture Preflight

## Status

Done.

## Context

E194 found no safe immediate fixture-builder implementation. The only plausible
smaller follow-up was a preflight for three similar ASCII-activated gameplay
window helpers:

- `tests/unit/product_gameplay_controller_tests.cpp::makeGameplayWindow(...)`;
- `tests/unit/product_vulkan_room_frame_tests.cpp::makeGameplayWindow(...)`;
- `tests/unit/product_window_input_frame_tests.cpp::gameplayWindow(...)`.

They are similar because they all seed `window.creativeAuthoring.asciiRoomDraft`
and call `activateProductAsciiRoomPreview(...)`. They are not identical:
room text/id/source, camera yaw/pitch, and interaction-mode setup differ.

This is read-only. Do not create the helper in this card.

## Scope

Inspect only:

- `tests/unit/product_gameplay_controller_tests.cpp`
- `tests/unit/product_vulkan_room_frame_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`
- `src/app/iggy3d/ascii_room/Activation.hpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- existing product test support headers under `tests/unit/Product*TestSupport.hpp`

## Required Inventory

For each of the three helper functions, document:

- function name and exact line range;
- signature and return type;
- room text;
- room id;
- source name;
- session/null-session assumptions;
- activation function called;
- post-activation writes, especially camera and interaction-mode writes;
- assertion wording for activation success;
- all call sites in the same file and what behavior those call sites test.

Then compare the helpers field-by-field and decide whether a shared helper could
be neutral.

## Decision Buckets

Classify the possible next step:

1. **Implement Tiny Helper**: only if the helper can accept explicit room text,
   room id, source name, optional camera values, optional interaction mode, and
   success message without hiding behavior.
2. **Preflight More**: if active-room/collision stamping or session ownership
   needs its own guard before sharing.
3. **Keep Local**: if the helper would mostly rename behavior-specific setup.

## Required Commands

Run and summarize:

```sh
rg -n "makeGameplayWindow|gameplayWindow\\(|activateProductAsciiRoomPreview|asciiRoomDraft|cameraYawDegrees|cameraPitchDegrees|interactionMode" \
  /Users/kogaryu/iggy3d/tests/unit/product_gameplay_controller_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_vulkan_room_frame_tests.cpp \
  /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room/Activation.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room/Activation.cpp

git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required because this card is read-only.

## Output Requirements

Append the audit to this card and move it to `done/`.

If an implementation card is justified, draft it inside the completion brief
only. Do not create the next card yourself.

Any drafted implementation card must include:

- exact helper name and header path;
- exact API shape;
- exact files to migrate;
- proof that all room/camera/mode values remain explicit at call sites;
- self-blockers for hidden behavior or widened scope.

If no implementation is justified, say so directly.

## Non-Scope

- Do not edit source or test files.
- Do not add or modify support headers.
- Do not edit CMake.
- Do not run broad CTest.
- Do not edit receipt golden.
- Do not stage, commit, push, or launch a window.

## Completion Brief Template

- Card moved to done:
- Files inspected:
- Helper comparison:
- Decision:
- Draft follow-up card, if any:
- Commands run:
- Concerns/deferred:

## Completion Brief

- Card moved to done: yes
- Files inspected:
  - `tests/unit/product_gameplay_controller_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `src/app/iggy3d/ascii_room/Activation.hpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `tests/unit/ProductActiveSurfaceTestSupport.hpp`
  - `tests/unit/ProductFilesystemTestSupport.hpp`
  - `tests/unit/ProductReceiptTestSupport.hpp`
  - `tests/unit/ProductTestSupport.hpp`
- Helper comparison:
  - `product_gameplay_controller_tests.cpp::makeGameplayWindow(...)` at lines 56-72:
    - Signature/return: `iggy3d::ProductAppWindowState makeGameplayWindow(std::optional<iggy3d::Session>& session)`.
    - Room text:
      ```text
      #######
      #.....#
      #..P..#
      #.....#
      #..$.E#
      #######
      ```
    - Room id: `gameplay_controller_step_room`.
    - Source name: `unit/gameplay_controller_step_room.iggyroom.txt`.
    - Session assumption: caller passes a mutable `std::optional<Session>&`, typically empty; `activateProductAsciiRoomPreview(...)` replaces it with a created session on success. Callers usually assert `session.has_value()` after construction.
    - Activation call: `iggy3d::activateProductAsciiRoomPreview(session, window)`.
    - Post-activation writes: none in the helper. Individual tests later set `window.viewport.cameraYawDegrees` or install custom active-room/collision fixtures where needed.
    - Activation assertion wording: `expect(activation.ok, "ascii room activation ok")`.
    - Call sites and behavior covered:
      - `runManualMove(...)` line 423: shared manual movement harness for tuned step/camera-relative movement.
      - Movement idle/manual/tuning/accel/decel: lines 497, 559, 583, 615, 643, 671, 700, 702, 731, 761, 763.
      - Jump/air/coyote/buffer/cut/fall tuning: lines 801, 836, 880, 910, 941, 977, 1008, 1010, 1050, 1052.
      - Collision and wall-run candidate/active behavior: lines 1096, 1121, 1154, 1175, 1201, 1225, 1227, 1268, 1271, 1342, 1364, 1366, 1407, 1430, 1455, 1476, 1497, 1499.
      - Traversal/jump/floor/reset/dash/physics-planner behavior: lines 1539, 1597, 1652, 1685, 1714, 1761, 1793, 1826, 1859, 1890, 1924, 1964, 1985, 2014, 2043, 2079, 2116.
  - `product_vulkan_room_frame_tests.cpp::makeGameplayWindow(...)` at lines 103-119:
    - Signature/return: `iggy3d::ProductAppWindowState makeGameplayWindow(std::optional<iggy3d::Session>& session)`.
    - Room text:
      ```text
      #######
      #P..$.#
      #..E..#
      #######
      ```
    - Room id: `vulkan_product_room_frame`.
    - Source name: `unit/vulkan_product_room_frame.iggyroom.txt`.
    - Session assumption: caller passes a mutable `std::optional<Session>&`, typically empty; activation creates/replaces it. Callers usually assert `session.has_value()` after construction.
    - Activation call: `iggy3d::activateProductAsciiRoomPreview(session, window)`.
    - Post-activation writes: camera yaw/pitch are written before activation in the helper: `cameraYawDegrees = 18.0F`, `cameraPitchDegrees = -3.0F`. No interaction mode is set in the helper; tests set Creative mode later when testing creative document/room-editor/map-maker surfaces.
    - Activation assertion wording: `expect(activation.ok, "ascii room activation ok")`.
    - Call sites and behavior covered:
      - Gameplay room/projection/render frame proof: line 189.
      - Default overlay/debug HUD and physics HUD states: lines 321, 436, 521, 588, 619, 735.
      - Receipt and Vulkan UI overlay parity: lines 763, 870, 919, 978, 1054.
      - Menu/creative surface visibility policies: lines 1141, 1181, 1264, 1326.
  - `product_window_input_frame_tests.cpp::gameplayWindow(...)` at lines 256-274:
    - Signature/return: `iggy3d::ProductAppWindowState gameplayWindow(std::optional<iggy3d::Session>& session)`.
    - Room text:
      ```text
      #######
      #.....#
      #..P..#
      #.....#
      #..$.E#
      #######
      ```
    - Room id: `input_frame_gameplay_room`.
    - Source name: `unit/input_frame_gameplay_room.iggyroom.txt`.
    - Session assumption: caller passes a mutable `std::optional<Session>&`, usually empty; activation creates/replaces it. Several call sites assert `session.has_value()`.
    - Activation call: `iggy3d::activateProductAsciiRoomPreview(session, window)`.
    - Post-activation writes: sets `window.inputDevice.interactionMode = iggy3d::ProductInteractionMode::Player` after activation.
    - Activation assertion wording: `expect(activated.ok, "input frame gameplay activation ok")`.
    - Call sites and behavior covered:
      - Gameplay/controller input ownership: `controllerSouthJumpsInGameplayPlayerMode()` line 711 and `controllerSouthDoesNotJumpWhenFrontendBlocksGameplay()` line 753.
      - Map-maker/gameplay-owned movement and creative suppression: `mapMakerMovementStaysGameplayOwnedAndDoesNotPause()` line 784 and `creativeDocumentSuppressesProductControllerMovement()` line 852.
      - Controller chord creative consumption: `controllerChordToggleRecordsCreativeConsumption()` line 916.
      - Clambered wall-top jump path: `controllerSouthJumpsFromClamberedWallTop()` line 952.
  - Activation seam:
    - `Activation.hpp` exposes `ProductAsciiRoomActivationResult` lines 13-32 and `activateProductAsciiRoomPreview(std::optional<Session>&, ProductAppWindowState&)` lines 34-36.
    - `Activation.cpp` lines 60-135 reads `window.creativeAuthoring.asciiRoomDraft`, builds/records preview, assigns `activeRoom(window)`, bumps active-room revision, refreshes collision first without a session, builds an ASCII package/session seed, creates/replaces `activeSession`, refreshes collision again with the session, sets gameplay/session/frontend-shell state, and records activation diagnostics.
    - This means any helper around it must be named as an activation/window fixture, not a passive draft setter.
- Decision:
  - **Implement Tiny Helper** is justified, but only if the helper keeps all behavior-specific values explicit in local wrappers and does not absorb assertions or scenario policy.
  - The helper can be neutral if it accepts room text, room id, source name, activation success message, optional camera yaw/pitch, and optional post-activation interaction mode. It should not know about movement tests, Vulkan tests, controller input, active-room collision expectations, or creative surface policy.
  - Active-room/collision stamping is already owned by `activateProductAsciiRoomPreview(...)`; the test helper should not add a second active-room/collision policy.
- Draft follow-up card, if any:
  - Title: `Product Test Support G5 - ASCII Room Window Activation Helper`
  - Scope:
    - Add `tests/unit/ProductAsciiRoomWindowTestSupport.hpp`.
    - Migrate only:
      - `tests/unit/product_gameplay_controller_tests.cpp::makeGameplayWindow(...)`
      - `tests/unit/product_vulkan_room_frame_tests.cpp::makeGameplayWindow(...)`
      - `tests/unit/product_window_input_frame_tests.cpp::gameplayWindow(...)`
    - Do not migrate active-room/clamber/wall-jump/layered-floor helpers, render/debug seeders, controller harnesses, or creative scenarios.
  - Proposed helper API:
    ```cpp
    namespace iggy3d::test {

    struct ProductAsciiRoomWindowFixtureRequest {
      std::string_view roomText;
      std::string_view roomId;
      std::string_view sourceName;
      std::string_view activationSuccessMessage = "ascii room activation ok";
      std::optional<float> cameraYawDegrees;
      std::optional<float> cameraPitchDegrees;
      std::optional<ProductInteractionMode> interactionMode;
    };

    inline ProductAppWindowState activateAsciiRoomWindowForTest(
        std::optional<Session>& session,
        const ProductAsciiRoomWindowFixtureRequest& request);

    }  // namespace iggy3d::test
    ```
  - Required behavior:
    - Construct a fresh `ProductAppWindowState`.
    - Assign `window.creativeAuthoring.asciiRoomDraft.text`, `roomId`, and `sourceName` from request.
    - If camera yaw/pitch are set, write them before `activateProductAsciiRoomPreview(...)` to preserve the Vulkan helper ordering.
    - Call `activateProductAsciiRoomPreview(session, window)` and assert `activation.ok` using `iggy3d::test::expect(..., request.activationSuccessMessage)`.
    - If `interactionMode` is set, write `window.inputDevice.interactionMode` after activation to preserve `product_window_input_frame_tests.cpp::gameplayWindow(...)` ordering.
    - Return the window.
  - Migration proof requirements:
    - Keep local wrapper names in the three files; each wrapper should call the shared helper with explicit room text/id/source/camera/mode/message values visible at the local wrapper.
    - Existing call sites should remain unchanged.
    - Run `product_gameplay_controller_tests`, `product_vulkan_room_frame_tests`, and `product_window_input_frame_tests`.
  - Self-blockers:
    - Block if migration requires CMake or production edits.
    - Block if any local wrapper needs to hide room text/id/source/camera/mode in shared defaults.
    - Block if the helper starts accepting or asserting active-room/collision/movement/render policy.
    - Block if any call site needs activation failure details rather than the current `expect(ok, message)` behavior.
- Commands run:
  - `rg -n "makeGameplayWindow|gameplayWindow\\(|activateProductAsciiRoomPreview|asciiRoomDraft|cameraYawDegrees|cameraPitchDegrees|interactionMode" /Users/kogaryu/iggy3d/tests/unit/product_gameplay_controller_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_vulkan_room_frame_tests.cpp /Users/kogaryu/iggy3d/tests/unit/product_window_input_frame_tests.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room/Activation.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/ascii_room/Activation.cpp`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Additional read-only `nl`, `awk`, `rg`, and `sed` commands for exact line ranges, per-file call-site mapping, activation behavior, and existing support-header context.
- Concerns/deferred:
  - No source, test, CMake, receipt golden, or support header files were edited.
  - No build or CTest was run; E195 is read-only and required only `diff --check`.
  - No staging, commit, push, or window launch was performed.
