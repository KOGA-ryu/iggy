# E48: Product Creative Input Request Builders

## Objective

Stop product Creative input tests and callers from depending on brittle aggregate
initializer ordering for request structs.

## Problem

`ProductCreativeInputActionsRequest` has grown as pointer lifecycle and target
state were added. Some call sites still construct it positionally, which now
produces missing-field initializer warnings when new fields are appended.

This is a feature-add friction smell: adding a request field should not require
auditing positional initializers across product input tests and the live input
frame.

## Required Reads

- `src/app/iggy3d/creative/bridge/InputFrame.hpp`
- `src/app/iggy3d/creative/bridge/InputFrame.cpp`
- `src/app/iggy3d/window/InputFrame.cpp`
- `tests/unit/product_creative_ui_input_frame_tests.cpp`
- `tests/unit/product_creative_input_frame_tests.cpp`
- `tests/unit/product_creative_world_launch_tests.cpp`

## Scope

- Replace positional aggregate construction of
  `ProductCreativeInputActionsRequest` and related Creative input request structs
  with named helper builders or member assignment.
- Keep request defaults exactly the same.
- Keep current pointer lifecycle behavior and tests unchanged.
- Prefer small test/local helpers where production helpers would be overkill.

## Acceptance

- Focused build no longer emits the existing missing-field initializer warnings
  for Creative input request construction.
- Adding a field to `ProductCreativeInputActionsRequest` should require updating
  its default/helper, not every positional aggregate call.
- Tool-only, downstream-click, pointer lifecycle, move commit, and no-change
  release tests still pass.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_input_frame_tests product_creative_world_launch_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_input_frame_tests|product_creative_world_launch_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change input routing order.
- Do not change pointer lifecycle semantics.
- Do not suppress warnings globally.
- Do not widen into the E36 orchestration extraction.

## Completion Brief

- Files modified:
  - `tests/unit/product_creative_ui_input_frame_tests.cpp`
  - `tests/unit/product_creative_input_frame_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
- Replaced the remaining positional aggregate construction of
  `ProductCreativeInputActionsRequest` in the UI input test with member
  assignment.
- Replaced pointer lifecycle event aggregate returns in
  `product_creative_input_frame_tests.cpp` with named field assignment.
- Added test-local `clickOverrideFor(...)` in
  `product_window_input_frame_tests.cpp` to avoid positional construction of
  `ProductWindowInputClickOverride`.
- No production input routing or pointer lifecycle semantics changed.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_ui_input_frame_tests product_creative_input_frame_tests product_creative_world_launch_tests product_window_input_frame_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_ui_input_frame_tests|product_creative_input_frame_tests|product_creative_world_launch_tests|product_window_input_frame_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

Notes:

- The focused build no longer emits the previous missing-field initializer
  warnings for creative input request/pointer lifecycle/click override
  construction.
