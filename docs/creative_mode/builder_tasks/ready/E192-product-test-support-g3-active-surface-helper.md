# E192: Product Test Support G3 - Active Surface Helper

## Status

Ready.

## Context

E189 found repeated active-surface test helpers. After E190/E191, keep reducing
test boilerplate in small mechanical slices.

This card only centralizes the duplicated test wrapper around
`syncProductWindowInputOwnerFromActiveSurface(...)`. It must not change active
surface routing behavior, assertions, or production code.

## Scope

Add:

- `tests/unit/ProductActiveSurfaceTestSupport.hpp`

Migrate only these files in this slice:

- `tests/unit/product_menu_transitions_tests.cpp`
- `tests/unit/product_starter_menu_action_tests.cpp`
- `tests/unit/product_window_input_frame_tests.cpp`

Do not migrate `product_frontend_router_tests.cpp` unless the build forces it.
That file owns route matrix helpers and broader active-surface contract tests;
it should stay explicit for now.

## Required Helper Shape

Use a small header-only namespace:

```cpp
namespace iggy3d::test {

inline ProductActiveSurfaceFrame liveSurface(const FrontendState& frontend,
                                             ProductAppWindowState& window);

} // namespace iggy3d::test
```

Implementation should be the same wrapper currently duplicated in the target
files:

```cpp
return syncProductWindowInputOwnerFromActiveSurface(frontend, window);
```

Keep the non-const `ProductAppWindowState&` signature even though the current
production implementation only resolves and returns the frame. The helper is
documenting the current test seam, not changing the production API contract.

## Migration Policy

- Remove only the local `liveSurface(...)` helpers that exactly wrap
  `syncProductWindowInputOwnerFromActiveSurface(...)`.
- Replace them with a narrow `using iggy3d::test::liveSurface;` declaration near
  the anonymous namespace.
- Do not rename existing `liveSurface(...)` call sites unless necessary.
- Do not move or hide explicit assertions against:
  - `inputOwner`;
  - `gameplayInputSuppressed`;
  - `activeSurface`;
  - receipt `active_surface` / `input_surface` fields.
- Do not migrate unrelated helpers such as hit-test helpers, temp-root helpers,
  action harnesses, route matrix builders, or product fixtures.
- Do not edit production source, CMake, or receipt golden.

## Required Tests

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build \
  --target product_menu_transitions_tests \
           product_starter_menu_action_tests \
           product_window_input_frame_tests \
           product_frontend_router_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build \
  -R '^(product_menu_transitions_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_frontend_router_tests)$' \
  --output-on-failure

git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

No full CTest is required for builder on this slice unless the helper shape
widens beyond the three listed migration files.

## Self-Blocking Criteria

Move this card to `blocked/` with evidence if:

- the target files use meaningfully different active-surface helper semantics;
- removing the local helper would hide explicit owner/suppression assertions;
- migration requires production or CMake edits;
- unrelated tests must be edited to make the listed targets compile;
- `product_frontend_router_tests.cpp` requires nontrivial migration rather than
  only compiling against the untouched production API.

## Non-Scope

- Do not migrate receipt helpers.
- Do not migrate temp-root/options helpers.
- Do not migrate large harness/scenario structs.
- Do not rewrite active-surface routing tests.
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
