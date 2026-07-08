# E193: Product Test Support G4 - Temp Root And Options Builder

## Status

Ready.

## Context

E189 found repeated test temp-root and `ProductAppOptions` setup. E190-E192
handled assertion, receipt, and active-surface helper duplication. This slice
should centralize the boring filesystem setup only.

This is intentionally not a save-system harness. Keep save-path edge-case tests
explicit.

## Scope

Add:

- `tests/unit/ProductFilesystemTestSupport.hpp`

Migrate only these files in this slice:

- `tests/unit/product_creative_world_launch_tests.cpp`
- `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
- `tests/unit/product_starter_menu_action_tests.cpp`
- `tests/unit/product_automation_dispatch_tests.cpp`

Do not migrate `tests/unit/product_save_bridge_tests.cpp`.

## Required Helper Shape

Use a small header-only namespace:

```cpp
namespace iggy3d::test {

inline std::filesystem::path cleanProductTestRoot(std::string_view suite,
                                                  std::string_view name);

inline ProductAppOptions productTestOptions(std::string_view suite,
                                            std::string_view name);

inline ProductAppOptions productTestOptions(std::string_view suite,
                                            std::string_view name,
                                            ProductWindowMode windowMode);

} // namespace iggy3d::test
```

`cleanProductTestRoot(...)` should preserve the current behavior in the target
files:

- root path is `std::filesystem::temp_directory_path() / suite / name`;
- remove the whole root with a local `std::error_code`;
- create the root directory with the same `std::error_code`;
- return the root path.

`productTestOptions(...)` should default-construct `ProductAppOptions`, assign
`saveRoot = cleanProductTestRoot(suite, name)`, and optionally set
`windowMode`.

## Migration Policy

- Replace local `testRoot(...)` helpers only when they match the clean-root
  behavior above.
- Replace local `testOptions(...)` helpers only when they only set `saveRoot`
  and, optionally, `windowMode`.
- Keep local helpers when they encode additional save-path behavior or fixture
  policy.
- Keep suite prefixes byte-identical:
  - `iggy3d_creative_launch`;
  - `iggy3d_creative_no_window_bake`;
  - `iggy3d_starter_action`;
  - `iggy3d_automation_dispatch`.
- It is acceptable to leave short local wrappers if they materially improve
  readability in a very large test file, but those wrappers should call the
  shared helper and keep the old helper names stable for the file.
- Do not edit production source, CMake, receipt golden, or save bridge tests.

## Required Tests

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build \
  --target product_creative_world_launch_tests \
           product_creative_no_window_bake_scenario_tests \
           product_starter_menu_action_tests \
           product_automation_dispatch_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build \
  -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_starter_menu_action_tests|product_automation_dispatch_tests)$' \
  --output-on-failure

git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files.

No full CTest is required for builder on this slice unless the helper shape
widens beyond the four listed migration files.

## Self-Blocking Criteria

Move this card to `blocked/` with evidence if:

- any target file needs dirty temp roots or preexisting files to preserve
  behavior;
- a local options helper sets more than `saveRoot` and `windowMode`;
- migrating requires production or CMake edits;
- `product_save_bridge_tests.cpp` appears necessary to make the helper useful;
- unrelated tests must be edited to make the listed targets compile.

## Non-Scope

- Do not migrate assertion helpers.
- Do not migrate receipt helpers.
- Do not migrate active-surface helpers.
- Do not migrate large harness/scenario structs.
- Do not migrate `product_save_bridge_tests.cpp`.
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
