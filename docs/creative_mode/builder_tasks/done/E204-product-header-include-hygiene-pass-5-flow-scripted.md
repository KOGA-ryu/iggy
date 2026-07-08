# E204: Product Header Include Hygiene - Pass 5 Flow And Scripted Driver

## Status

Done.

## Context

E125, E201, E202, and E203 proved the include-hygiene pattern: headers that
only mention `ProductAppWindowState` by reference/pointer should
forward-declare it, while implementation files that read/write fields include
the full `ProductAppWindowState.hpp` directly.

Current live census still shows `save/Flow.hpp` and
`gameplay/ScriptedDriver.hpp` pulling in the full window-state definition even
though both only expose `ProductAppWindowState` by non-owning reference.

## Objective

Remove the direct `ProductAppWindowState.hpp` include from the two target
headers and make concrete users include their own dependency directly when
needed.

Target headers only:

- `src/app/iggy3d/save/Flow.hpp`
- `src/app/iggy3d/gameplay/ScriptedDriver.hpp`

## Scope

Allowed edits:

- `src/app/iggy3d/save/Flow.hpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/gameplay/ScriptedDriver.hpp`
- `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
- direct compile fallout in production/test `.cpp` or `.hpp` files that
  genuinely use `ProductAppWindowState` as a complete type and previously got
  it transitively through one of the two target headers;
- this task card.

Do not touch other include-hygiene candidate headers in this pass.

Do not change function signatures, function bodies, struct layouts, save-flow
behavior, scripted gameplay behavior, receipt fields, CMake, or receipt golden.

## Required Work

For each target header:

1. Confirm every `ProductAppWindowState` use is incomplete-type safe:
   `ProductAppWindowState&`, `const ProductAppWindowState&`, or
   `ProductAppWindowState*` in declarations only.
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

   Preserve existing namespace style if the header already has one.
3. Ensure the paired implementation file includes the full type directly:
   - `src/app/iggy3d/save/Flow.cpp`
   - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
4. Build. For any fallout, add direct
   `#include "app/iggy3d/ProductAppWindowState.hpp"` to the file that actually
   instantiates, reads, writes, or otherwise needs the complete type.

## Escape Hatch

Stop and move this card to `blocked/` with evidence if:

- either target header needs a complete `ProductAppWindowState` type after all;
- fallout expands beyond a handful of obvious direct include repairs;
- fixing fallout requires declaration/signature/body changes;
- tests reveal behavior or receipt output changes.

Do not solve a cascade by reverting the target header changes unless you are
blocking the card and explaining the cascade.

## Required Checks

Run:

```sh
grep -rl 'app/iggy3d/ProductAppWindowState.hpp' \
  /Users/kogaryu/iggy3d/src \
  /Users/kogaryu/iggy3d/apps \
  /Users/kogaryu/iggy3d/tests | wc -l

cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_gameplay_controls_smoke product_receipt_key_order_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_gameplay_controls_smoke|product_receipt_key_order_tests)$' --output-on-failure

git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden

git -C /Users/kogaryu/iggy3d diff --check
```

Run the include-count command before and after the edit. The raw count may rise
if hidden transitive dependencies become explicit; report that honestly.

Also run a focused trailing-whitespace scan over touched files.

## Completion Brief Requirements

Report:

- target headers forward-declared;
- confirmation that each target header uses `ProductAppWindowState` only
  incompletely;
- `.cpp` partners that now include the full type directly;
- fallout files given direct includes, if any;
- before/after direct includer count for `ProductAppWindowState.hpp`;
- focused build/CTest results;
- receipt golden diff result;
- `git diff --check` and trailing-whitespace results;
- confirmation that no signatures, behavior, CMake, or receipt fields changed.

## Completion Brief

- Card moved to done: `/Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E204-product-header-include-hygiene-pass-5-flow-scripted.md`
- Files changed:
  - `src/app/iggy3d/save/Flow.hpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `src/app/iggy3d/gameplay/ScriptedDriver.hpp`
  - `src/app/iggy3d/gameplay/ScriptedDriver.cpp`
  - `docs/creative_mode/builder_tasks/done/E204-product-header-include-hygiene-pass-5-flow-scripted.md`
- Target headers forward-declared:
  - `Flow.hpp` removed `#include "app/iggy3d/ProductAppWindowState.hpp"` and now forward-declares `struct ProductAppWindowState;`.
  - `ScriptedDriver.hpp` removed `#include "app/iggy3d/ProductAppWindowState.hpp"` and now forward-declares `struct ProductAppWindowState;`.
- Incomplete-type confirmation:
  - `Flow.hpp` uses `ProductAppWindowState` only as non-owning function parameters by reference.
  - `ScriptedDriver.hpp` uses `ProductAppWindowState` only as a non-owning function parameter by reference.
- `.cpp` partners with direct full-type includes:
  - `Flow.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` directly.
  - `ScriptedDriver.cpp` now includes `app/iggy3d/ProductAppWindowState.hpp` directly.
- Fallout files given direct includes:
  - None. The focused build passed without additional compile fallout.
- Direct includer count for `ProductAppWindowState.hpp`:
  - Before: 68
  - After: 68
  - Count stayed flat because the two target header includes moved to their two implementation files.
- Focused build:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_starter_menu_action_tests product_gameplay_controls_smoke product_receipt_key_order_tests -j10` passed.
- Focused CTest:
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_starter_menu_action_tests|product_gameplay_controls_smoke|product_receipt_key_order_tests)$' --output-on-failure` passed: 4/4.
- Receipt golden diff:
  - `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` was empty.
- Checks:
  - `git -C /Users/kogaryu/iggy3d diff --check` passed.
  - Focused trailing-whitespace scan over touched files and this card passed.
- Confirmations:
  - No function signatures changed.
  - No function bodies, struct layouts, save-flow behavior, scripted gameplay behavior, receipt fields, CMake, or receipt golden changed.
- Concerns/deferred:
  - None for this slice.
