# E203: Product Header Include Hygiene - Pass 4 ASCII Room

## Status

Ready.

## Context

E125, E201, and E202 proved the include-hygiene pattern: headers that only
mention `ProductAppWindowState` by reference/pointer should forward-declare it,
while implementation files that read/write fields include the full
`ProductAppWindowState.hpp` directly.

Current live census still shows two ASCII room headers pulling in the full
window-state definition through declarations. This pass is intentionally small
and mechanical.

## Objective

Remove the direct `ProductAppWindowState.hpp` include from the two ASCII room
headers and make concrete users include their own dependency directly when
needed.

Target headers only:

- `src/app/iggy3d/ascii_room/Activation.hpp`
- `src/app/iggy3d/ascii_room/Preview.hpp`

## Scope

Allowed edits:

- `src/app/iggy3d/ascii_room/Activation.hpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- `src/app/iggy3d/ascii_room/Preview.hpp`
- `src/app/iggy3d/ascii_room/Preview.cpp`
- direct compile fallout in production/test `.cpp` or `.hpp` files that
  genuinely use `ProductAppWindowState` as a complete type and previously got
  it transitively through one of the two target headers;
- this task card.

Do not touch other include-hygiene candidate headers in this pass.

Do not change function signatures, function bodies, struct layouts, ASCII room
behavior, package/session behavior, receipt fields, CMake, or receipt golden.

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
   - `src/app/iggy3d/ascii_room/Activation.cpp`
   - `src/app/iggy3d/ascii_room/Preview.cpp`
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

cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_ascii_room_activation_tests product_ascii_authoring_smoke product_ascii_package_smoke product_creative_no_window_bake_scenario_tests product_automation_dispatch_tests product_receipt_key_order_tests -j10

ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_ascii_room_activation_tests|product_ascii_authoring_smoke|product_ascii_package_smoke|product_creative_no_window_bake_scenario_tests|product_automation_dispatch_tests|product_receipt_key_order_tests)$' --output-on-failure

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
