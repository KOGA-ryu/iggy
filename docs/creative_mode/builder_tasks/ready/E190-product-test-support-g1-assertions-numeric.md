# E190: Product Test Support G1 - Assertions And Numeric Helpers

## Status

Ready.

## Context

E189 found that product tests duplicate simple assertion and numeric comparison
helpers heavily:

- 81 product test files define local `expect(...)` or `void expect(...)`.
- 15 local `near(...)`, `nearlyEqual(...)`, or `expectNear(...)` definitions
  appear across 14 files.

Start with a small header-only support helper. Do not introduce a broad harness,
do not change CMake, and do not migrate all tests at once.

## Scope

Add:

- `tests/unit/ProductTestSupport.hpp`

Migrate only these files in this slice:

- `tests/unit/product_camera_controller_tests.cpp`
- `tests/unit/product_gameplay_controller_tests.cpp`
- `tests/unit/product_vulkan_room_frame_tests.cpp`
- `tests/unit/product_creative_fly_tests.cpp`
- `tests/unit/product_creative_navigate_fly_tests.cpp`

## Required Helper Shape

Use a small header-only namespace:

```cpp
namespace iggy3d::test {

inline bool expect(bool condition, std::string_view message);
inline bool expect(bool condition, const char* message);
inline bool near(float lhs, float rhs, float epsilon = 0.0001F);
inline bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001F);
inline bool nearlyEqual(const Mat4& lhs, const Mat4& rhs, float epsilon = 0.0001F);

} // namespace iggy3d::test
```

Keep behavior byte-equivalent:

- failed expectations print `FAIL: ` plus the message and newline to
  `std::cerr`;
- numeric comparisons preserve the existing epsilon defaults in migrated files;
- the `Mat4` helper compares all matrix slots using the float helper.

If one of the listed files uses a different epsilon default that cannot be
preserved cleanly through a default argument or explicit argument, keep that
helper local and report why.

## Migration Policy

- Remove only the local helpers that are replaced by `iggy3d::test::*`.
- Keep behavior-specific helpers local, for example:
  - `horizontalDistance(...)` in `product_gameplay_controller_tests.cpp`;
  - product-specific fixture builders;
  - render/debug/collision helpers that are not simple numeric comparisons.
- Prefer explicit namespace qualification (`iggy3d::test::expect(...)`) or local
  `using` declarations near the anonymous namespace. Avoid broad `using
  namespace`.
- Do not change test names, assertions, fixture data, expected values, or
  production source.

## Required Tests

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build \
  --target product_camera_controller_tests product_gameplay_controller_tests \
           product_vulkan_room_frame_tests product_creative_fly_tests \
           product_creative_navigate_fly_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build \
  -R '^(product_camera_controller_tests|product_gameplay_controller_tests|product_vulkan_room_frame_tests|product_creative_fly_tests|product_creative_navigate_fly_tests)$' \
  --output-on-failure

git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

No full CTest is required for builder on this slice unless the helper shape
widens beyond the five listed tests.

## Self-Blocking Criteria

Move this card to `blocked/` with evidence if:

- migrating the five listed files requires CMake changes;
- custom failure formatting or return behavior differs from the proposed helper;
- the helper starts attracting behavior-specific product fixture code;
- unrelated tests must be edited to make the five targets compile.

## Non-Scope

- Do not migrate receipt helpers.
- Do not migrate active-surface helpers.
- Do not migrate temp-root/options helpers.
- Do not migrate large harness/scenario structs.
- Do not edit production source.
- Do not edit receipt golden.
- Do not stage, commit, push, or launch a window.

## Completion Brief Template

- Card moved to done:
- Files changed:
- Helper API added:
- Files migrated:
- Helpers intentionally left local:
- Tests/checks run:
- Concerns/deferred:
