# E125: Product Header Include Hygiene — Pass 1

## Objective

**Mechanical, behavior-preserving.** In headers that use `ProductAppWindowState`
only by reference/pointer in declarations, replace the heavy
`#include "app/iggy3d/ProductAppWindowState.hpp"` with a forward declaration and
move the full include into the paired `.cpp`. **Pass 1 is scoped to two headers
only:** `ReceiptBuilder.hpp` (≈23 includers) and `window/RendererLifecycle.hpp`
(≈18 includers).

## Why This Exists

From `docs/complexity_audit_v0_1.md` (finding #11 / bucket 9). `ProductAppWindowState.hpp`
is a 62-include god-header. 13 headers hard-include it when their declarations
only take it by `&`/`*`, forcing the full struct to re-parse in ~40 translation
units. A forward declaration suffices for by-reference parameters. This is the
same safe re-pointing pattern already proven in the receipt struct extraction.

## Required Work

For each of the two target headers (`ReceiptBuilder.hpp`, then
`window/RendererLifecycle.hpp`):

1. **Confirm the header only uses `ProductAppWindowState` incompletely** — i.e.
   every use is a `const ProductAppWindowState&` / `ProductAppWindowState*`
   parameter or return-by-ref in a declaration, with **no member access, no
   by-value use, no `sizeof`, no inheritance** inside the header. If any use
   needs the complete type, that header is out of scope — leave it and note why.
2. **Forward-declare instead of include.** Replace
   `#include "app/iggy3d/ProductAppWindowState.hpp"` with:
   ```cpp
   namespace iggy3d { struct ProductAppWindowState; }
   ```
3. **Guarantee the paired `.cpp` still gets the full definition.** Ensure the
   matching `.cpp` (`ReceiptBuilder.cpp` / `RendererLifecycle.cpp`) includes
   `ProductAppWindowState.hpp` directly (through `receipt/ReceiptFields.hpp` is
   fine for `ReceiptBuilder.cpp` — verify).
4. **Fix fallout by adding direct includes, not by reverting.** Rebuild the whole
   library + tests. Some TUs relied on getting the struct *transitively* through
   the re-pointed header; each such TU should now `#include
   "app/iggy3d/ProductAppWindowState.hpp"` directly (this is correct — it makes
   the dependency explicit, exactly as the receipt re-point pass did). Add those
   direct includes.
5. **Escape hatch:** if fixing fallout cascades beyond a handful of obvious direct
   includes (i.e. it is turning into a repo-wide churn, not a Pass-1 tidy), stop,
   revert your changes, and move this card to `blocked/` with the TU list and the
   cascade evidence. Pass 1 must stay small and clean.

## Acceptance Notes

- The two target headers forward-declare `ProductAppWindowState`; their `.cpp`
  partners still compile with the full type.
- Any TU that lost the transitive include now includes
  `ProductAppWindowState.hpp` directly.
- **Build clean, full suite green (currently 257/257), including
  `product_receipt_key_order_tests`.** Zero behavior change — this is includes
  only.
- Report the before/after count of files that include `ProductAppWindowState.hpp`
  (`grep -rl 'ProductAppWindowState.hpp' src apps tests | wc -l`).

## Do Not

- Do not touch any header beyond the two named (later passes handle the rest).
- Do not change any declaration signature, function body, or struct.
- Do not reorder or remove unrelated includes.
- Do not stage, commit, or push.

## Suggested Verification

```sh
grep -rl 'app/iggy3d/ProductAppWindowState.hpp' /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests | wc -l   # before
cmake --build /Users/kogaryu/iggy3d/build -j10
ctest --test-dir /Users/kogaryu/iggy3d/build
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Headers forward-declared:
- .cpp partners confirmed to hold the full type:
- TUs given a direct include (fallout fixed):
- ProductAppWindowState.hpp includer count (before → after):
- Tests/checks run:
- Concerns/deferred (or blocker if cascaded):

## Completion Brief - E125

### Headers Forward-Declared

- `src/app/iggy3d/ReceiptBuilder.hpp`
  - Removed `#include "app/iggy3d/ProductAppWindowState.hpp"`.
  - Added `struct ProductAppWindowState;`.
  - Also forward-declared declaration-only receipt fixture types previously
    arriving through the transitive window-state include.
  - Confirmed `ProductAppWindowState` use is by reference only in declarations.
- `src/app/iggy3d/window/RendererLifecycle.hpp`
  - Removed `#include "app/iggy3d/ProductAppWindowState.hpp"`.
  - Added `struct ProductAppWindowState;`.
  - Confirmed `ProductAppWindowState` use is by pointer/reference only in
    declarations.

### .cpp Partners With Full Type

- `src/app/iggy3d/ReceiptBuilder.cpp` still receives the complete
  `ProductAppWindowState` definition through
  `src/app/iggy3d/receipt/ReceiptFields.hpp`, which still includes
  `ProductAppWindowState.hpp`.
- `src/app/iggy3d/window/RendererLifecycle.cpp` now directly includes
  `app/iggy3d/ProductAppWindowState.hpp` because it reads/writes fields.

### Direct Include Fallout Fixed

The following concrete users relied on the old transitive include and now
include their own dependency directly:

- `src/app/iggy3d/AppKernel.hpp`
  - `ProductAppWindowState` by value.
  - `FrontendSettings` by value; added the direct settings include while there.
- `src/app/iggy3d/window/Loop.hpp`
  - `ProductAppWindowState` by value.
- `src/app/iggy3d/gameplay/Controller.cpp`
  - Mutates `ProductAppWindowState` fields.
- `src/app/iggy3d/gameplay/TapeRunner.cpp`
  - Mutates `ProductAppWindowState` fields.
- `tests/unit/product_mouse_capture_policy_tests.cpp`
- `tests/unit/product_receipt_key_order_tests.cpp`
- `tests/unit/product_top_down_map_overlay_tests.cpp`
- `tests/unit/product_creative_ui_projection_receipt_tests.cpp`
- `tests/unit/product_creative_ui_command_receipt_tests.cpp`
- `tests/unit/product_creative_ui_frame_tests.cpp`
- `tests/unit/product_movement_debug_hud_tests.cpp`
- `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`

These are direct include repairs only; no declarations, function bodies, or
struct layouts changed.

### Include Count

- Command:
  `grep -rl 'app/iggy3d/ProductAppWindowState.hpp' /Users/kogaryu/iggy3d/src /Users/kogaryu/iggy3d/apps /Users/kogaryu/iggy3d/tests | wc -l`
- Before: `39`
- After: `50`

The raw direct include count increased because hidden transitive dependencies in
receipt-focused tests and owner headers were made explicit. The hygiene win is
that the two high-fanout headers no longer propagate
`ProductAppWindowState.hpp` to all includers.

### Tests / Checks Run

- `cmake --build /Users/kogaryu/iggy3d/build -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed:
  `258/258`.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over touched files passed.

### Concerns / Deferred

- This pass fixed the small, visible transitive fallout without reverting the
  header forward declarations. Later include-hygiene passes should expect the
  same pattern: direct include counts may rise while high-fanout propagation
  drops.
