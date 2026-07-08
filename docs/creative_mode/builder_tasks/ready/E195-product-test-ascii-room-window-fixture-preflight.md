# E195: Product Test ASCII Room Window Fixture Preflight

## Status

Ready.

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
