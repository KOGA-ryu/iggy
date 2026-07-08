# E196: Product Test Support G5 - ASCII Room Window Activation Helper

## Status

Done.

## Context

E195 compared the three ASCII-activated gameplay window helpers in:

- `tests/unit/product_gameplay_controller_tests.cpp::makeGameplayWindow(...)`
- `tests/unit/product_vulkan_room_frame_tests.cpp::makeGameplayWindow(...)`
- `tests/unit/product_window_input_frame_tests.cpp::gameplayWindow(...)`

They all construct a fresh `ProductAppWindowState`, seed
`window.creativeAuthoring.asciiRoomDraft`, call
`activateProductAsciiRoomPreview(...)`, assert activation success, and return
the window. They differ by explicit room text, room id, source name, camera
yaw/pitch, interaction-mode setup, and success-message wording.

This card implements only the tiny neutral helper justified by E195. Keep the
behavior-specific values visible in local wrappers.

## Scope

Add:

- `tests/unit/ProductAsciiRoomWindowTestSupport.hpp`

Migrate only these local wrappers:

- `tests/unit/product_gameplay_controller_tests.cpp::makeGameplayWindow(...)`
- `tests/unit/product_vulkan_room_frame_tests.cpp::makeGameplayWindow(...)`
- `tests/unit/product_window_input_frame_tests.cpp::gameplayWindow(...)`

Do not migrate:

- active-room/clamber/wall-jump/layered-floor helpers;
- render/debug seeders;
- controller harnesses;
- creative scenarios;
- save/bridge tests;
- production source;
- CMake.

## Required Helper API

Add this header-only test support API:

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

Use existing test support assertion plumbing instead of adding a new local
assertion helper.

## Required Behavior

`activateAsciiRoomWindowForTest(...)` must:

- construct a fresh `ProductAppWindowState`;
- assign `window.creativeAuthoring.asciiRoomDraft.text`,
  `window.creativeAuthoring.asciiRoomDraft.roomId`, and
  `window.creativeAuthoring.asciiRoomDraft.sourceName` from the request;
- write optional `window.viewport.cameraYawDegrees` and
  `window.viewport.cameraPitchDegrees` before calling
  `activateProductAsciiRoomPreview(...)`;
- call `activateProductAsciiRoomPreview(session, window)`;
- assert `activation.ok` with `request.activationSuccessMessage`;
- write optional `window.inputDevice.interactionMode` after activation;
- return the window.

Do not add active-room, active-room-collision, movement, render, or creative UI
assertions to the helper. Those policies stay in the caller tests.

## Migration Requirements

Keep the existing local wrapper names and call sites unchanged:

- `makeGameplayWindow(...)` in `product_gameplay_controller_tests.cpp`
- `makeGameplayWindow(...)` in `product_vulkan_room_frame_tests.cpp`
- `gameplayWindow(...)` in `product_window_input_frame_tests.cpp`

Each local wrapper should call the shared helper with explicit room text, room
id, source name, camera values, interaction mode, and success-message values
visible at the wrapper.

Preserve ordering:

- Vulkan camera yaw/pitch are set before activation.
- Input-frame `ProductInteractionMode::Player` is set after activation.

## Self-Blockers

Move this card to `blocked/` with evidence instead of widening scope if:

- migration requires production source or CMake edits;
- any local wrapper needs to hide room text, room id, source name, camera values,
  interaction mode, or success-message policy in shared defaults;
- the helper starts accepting or asserting active-room, active-room-collision,
  movement, render, or creative UI behavior;
- any call site needs activation failure details beyond the current
  `expect(ok, message)` behavior.

## Required Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build \
  --target product_gameplay_controller_tests product_vulkan_room_frame_tests product_window_input_frame_tests \
  -j10

ctest --test-dir /Users/kogaryu/iggy3d/build \
  -R '^(product_gameplay_controller_tests|product_vulkan_room_frame_tests|product_window_input_frame_tests)$' \
  --output-on-failure

git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

No full CTest is required unless the helper migration widens beyond this card.

## Completion Brief Requirements

Report:

- exact files changed;
- helper API added;
- the three local wrappers migrated and the explicit request values used by each;
- proof that existing call sites stayed unchanged;
- whether camera-before-activation and interaction-mode-after-activation ordering
  was preserved;
- tests/checks run;
- any remaining local ASCII/window helpers intentionally left local.

## Completion Brief

- Card moved to done: yes.
- Files changed:
  - `tests/unit/ProductAsciiRoomWindowTestSupport.hpp`
  - `tests/unit/product_gameplay_controller_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
- Helper API added:
  - `iggy3d::test::ProductAsciiRoomWindowFixtureRequest`
  - `iggy3d::test::activateAsciiRoomWindowForTest(std::optional<Session>&, const ProductAsciiRoomWindowFixtureRequest&)`
  - The helper constructs a fresh `ProductAppWindowState`, writes ASCII draft fields, optionally writes camera yaw/pitch before activation, calls `activateProductAsciiRoomPreview(...)`, asserts activation via `iggy3d::test::expect(...)`, optionally writes `window.inputDevice.interactionMode` after activation, and returns the window.
- Files migrated:
  - `product_gameplay_controller_tests.cpp::makeGameplayWindow(...)` now passes:
    - room text: `####### / #.....# / #..P..# / #.....# / #..$.E# / #######`
    - room id: `gameplay_controller_step_room`
    - source name: `unit/gameplay_controller_step_room.iggyroom.txt`
    - success message: `ascii room activation ok`
    - no camera override, no interaction-mode override
  - `product_vulkan_room_frame_tests.cpp::makeGameplayWindow(...)` now passes:
    - room text: `####### / #P..$.# / #..E..# / #######`
    - room id: `vulkan_product_room_frame`
    - source name: `unit/vulkan_product_room_frame.iggyroom.txt`
    - success message: `ascii room activation ok`
    - camera yaw: `18.0F`
    - camera pitch: `-3.0F`
    - no interaction-mode override
  - `product_window_input_frame_tests.cpp::gameplayWindow(...)` now passes:
    - room text: `####### / #.....# / #..P..# / #.....# / #..$.E# / #######`
    - room id: `input_frame_gameplay_room`
    - source name: `unit/input_frame_gameplay_room.iggyroom.txt`
    - success message: `input frame gameplay activation ok`
    - no camera override
    - interaction mode: `ProductInteractionMode::Player`
- Proof that existing call sites stayed unchanged:
  - The wrapper names and signatures remain unchanged:
    - `makeGameplayWindow(std::optional<iggy3d::Session>&)`
    - `makeGameplayWindow(std::optional<iggy3d::Session>&)`
    - `gameplayWindow(std::optional<iggy3d::Session>&)`
  - Call-site grep still routes through the same local wrappers; only wrapper bodies changed.
- Ordering preserved:
  - Camera yaw/pitch are applied before `activateProductAsciiRoomPreview(...)` inside the shared helper.
  - Optional interaction mode is applied after `activateProductAsciiRoomPreview(...)`; the input-frame `Player` mode write stays post-activation.
- Helpers intentionally left local:
  - Active-room, clamber, wall-jump, layered-floor, render/debug seeding, controller harness, and broader scenario helpers remain local to their tests.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_gameplay_controller_tests product_vulkan_room_frame_tests product_window_input_frame_tests -j10` passed.
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_gameplay_controller_tests|product_vulkan_room_frame_tests|product_window_input_frame_tests)$' --output-on-failure` passed: 3/3.
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched source/card files passed.
- Concerns/deferred:
  - None. The helper stays limited to ASCII room window activation setup and does not absorb active-room, movement, render, or creative UI policy.
