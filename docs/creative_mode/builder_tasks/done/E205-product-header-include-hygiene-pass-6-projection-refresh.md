# E205: Product Header Include Hygiene - Pass 6 Projection Refresh

## Status

Done.

## Context

E125 and E201-E204 proved the include-hygiene pattern: headers that only
mention `ProductAppWindowState` by reference/pointer should forward-declare it,
while implementation files that read/write fields include the full
`ProductAppWindowState.hpp` directly.

Current live census still shows `gameplay/ProjectionRefresh.hpp` pulling in the
full window-state definition even though its window-state uses are non-owning
references in request structs and function declarations.

## Objective

Remove the direct `ProductAppWindowState.hpp` include from
`ProjectionRefresh.hpp` and make concrete users include their own dependency
directly when needed.

Target header only:

- `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`

## Scope

Allowed edits:

- `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- direct compile fallout in production/test `.cpp` or `.hpp` files that
  genuinely use `ProductAppWindowState` as a complete type and previously got
  it transitively through `ProjectionRefresh.hpp`;
- this task card.

Do not touch other include-hygiene candidate headers in this pass.

Do not change function signatures, function bodies, struct layouts, projection
behavior, viewport/render behavior, receipt fields, CMake, or receipt golden.

## Required Work

1. Confirm every `ProductAppWindowState` use in `ProjectionRefresh.hpp` is
   incomplete-type safe: `ProductAppWindowState&`,
   `const ProductAppWindowState&`, or `ProductAppWindowState*` in declarations
   and request structs only.
2. Replace:

   ```cpp
   #include "app/iggy3d/ProductAppWindowState.hpp"
   ```

   with a forward declaration:

   ```cpp
   namespace iggy3d {
   struct ProductAppWindowState;
   }
   ```

   Preserve existing namespace style.
3. Ensure `src/app/iggy3d/gameplay/ProjectionRefresh.cpp` includes the full
   type directly because it reads/writes window fields.
4. Build. For any fallout, add direct
   `#include "app/iggy3d/ProductAppWindowState.hpp"` to the file that actually
   instantiates, reads, writes, or otherwise needs the complete type.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- `ProjectionRefresh.hpp` needs a complete `ProductAppWindowState` type after
  all;
- fallout expands beyond a handful of obvious direct include repairs;
- fixing fallout requires declaration/signature/body changes;
- tests reveal behavior or receipt output changes.

Do not solve a cascade by reverting the target header change unless you are
blocking the card and explaining the cascade.

## Required Checks

Run:

```sh
grep -rl 'app/iggy3d/ProductAppWindowState.hpp' \
  /Users/kogaryu/iggy3d/src \
  /Users/kogaryu/iggy3d/apps \
  /Users/kogaryu/iggy3d/tests | wc -l

cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_vulkan_room_frame_tests product_movement_debug_hud_tests product_top_down_map_overlay_tests product_creative_world_launch_tests product_receipt_key_order_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_vulkan_room_frame_tests|product_movement_debug_hud_tests|product_top_down_map_overlay_tests|product_creative_world_launch_tests|product_receipt_key_order_tests)$' --output-on-failure

git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden

git -C /Users/kogaryu/iggy3d diff --check
```

Run the include-count command before and after the edit. The raw count may rise
if hidden transitive dependencies become explicit; report that honestly.

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief Requirements

Report:

- target header forward-declared;
- confirmation that `ProjectionRefresh.hpp` uses `ProductAppWindowState` only
  incompletely;
- `.cpp` partner that now includes the full type directly;
- fallout files given direct includes, if any;
- before/after direct includer count for `ProductAppWindowState.hpp`;
- focused build/CTest results;
- receipt golden diff result;
- `git diff --check` and trailing-whitespace results;
- confirmation that no signatures, behavior, CMake, or receipt fields changed.

## Completion Brief

- Card moved to done: `/Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E205-product-header-include-hygiene-pass-6-projection-refresh.md`
- Files changed:
  - `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/window/FramePresenter.cpp`
  - `tests/unit/product_creative_world_launch_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E205-product-header-include-hygiene-pass-6-projection-refresh.md`
- Target header forward-declared:
  - `ProjectionRefresh.hpp` removed `#include "app/iggy3d/ProductAppWindowState.hpp"` and now forward-declares `struct ProductAppWindowState;`.
- Incomplete-type confirmation:
  - `ProjectionRefresh.hpp` uses `ProductAppWindowState` only as non-owning request members and function parameters by reference.
- `.cpp` partner with direct full-type include:
  - `ProjectionRefresh.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` directly.
- Fallout files given direct includes:
  - `FramePresenter.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` because it reads/writes `request.window`.
  - `product_creative_world_launch_tests.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` because it instantiates and mutates `ProductAppWindowState`.
- Direct includer count for `ProductAppWindowState.hpp`:
  - Before: 68
  - After: 70
  - Count rose because one header include was removed and three concrete users now include the full type directly.
- Focused build:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_vulkan_room_frame_tests product_movement_debug_hud_tests product_top_down_map_overlay_tests product_creative_world_launch_tests product_receipt_key_order_tests -j10` passed.
- Focused CTest:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_vulkan_room_frame_tests|product_movement_debug_hud_tests|product_top_down_map_overlay_tests|product_creative_world_launch_tests|product_receipt_key_order_tests)$' --output-on-failure` passed: 5/5.
- Receipt golden diff:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Confirmations:
  - No function signatures changed.
  - No function bodies, struct layouts, projection behavior, viewport/render behavior, receipt fields, CMake, or receipt golden changed.
- Concerns/deferred:
  - None for this slice.
