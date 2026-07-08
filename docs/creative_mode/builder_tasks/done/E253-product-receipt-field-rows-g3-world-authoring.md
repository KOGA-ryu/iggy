# E253: Product Receipt Field Rows G3 - World Authoring

## Objective

Apply the proven local receipt row-table pattern to
`src/app/iggy3d/receipt/WorldAuthoringFields.cpp`.

This is the third receipt-field row slice after E251 and E252. It should remain
a mechanical refactor only: preserve the public appender signature, field order,
field keys, field values, formatting, and receipt golden output exactly.

## Current Context

E251 proved the one-file local row-table pattern in
`GameplaySceneStateFields.cpp`:

- file-local row type
- ordered `std::array`
- no shared helper
- no `ProductAppReceiptContext`
- receipt key-order oracle stayed byte-identical

E252 repeated the same pattern in `GameplayRuntimeMovementFields.cpp` with a
file-local context because that appender takes multiple inputs. The golden again
stayed byte-identical.

E250 classified `WorldAuthoringFields.cpp` as a direct table candidate:

- 204 lines
- 91 `appendReceiptField(...)` rows
- one appender function
- no loops
- no conditionals
- values come from `ProductAppWindowState`
- values are direct window-backed bool/string/count/hash fields

The current appender starts with:

- `world_setup_title`

and ends with:

- `room_editing_last_primitive_id`

## Scope

Edit only:

- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- this task card when moving it to `done/`

Add a file-local row type and ordered row array inside
`WorldAuthoringFields.cpp`.

Preserve the existing public API:

```cpp
void appendProductWorldAuthoringFields(RenderReceipt& receipt,
                                       const ProductAppWindowState& window);
```

The public appender should iterate the ordered rows.

## Required Row Shape

Use a file-local append-callback row. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct WorldAuthoringReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ProductAppWindowState& window,
                 std::string_view key);
};

const std::array<WorldAuthoringReceiptFieldRow, 91>
    kWorldAuthoringReceiptFields{{
        {"world_setup_title",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key,
                              window.creativeAuthoring.worldSetup.title);
         }},
        // Preserve every existing row in current order.
    }};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep every current value expression inside its row callback and pass it directly
to `appendReceiptField(...)`, matching current code.

## Required Behavior Preservation

Preserve every current row from `appendProductWorldAuthoringFields(...)`:

- same key literal
- same order
- same value expression
- same formatter behavior
- same casts and overload selection

The appender must still start with:

- `world_setup_title`

The appender must still end with:

- `room_editing_last_primitive_id`

The table must cover all 91 current rows. Do not leave direct procedural rows
unless a compile issue forces it; if that happens, stop and report the exact row
instead of widening the design.

## Non-Goals

Do not edit:

- `ReceiptFields.hpp`
- `ReceiptFields.cpp`
- `ReceiptBuilder.cpp`
- any other `src/app/iggy3d/receipt/*Fields.cpp`
- `ProductAppReceiptContext`
- CMake
- tests
- fixtures
- `tests/golden/product_receipt_key_order.golden`

Do not add a shared receipt row helper.
Do not change receipt keys, order, values, status strings, or formatting.
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "WorldAuthoringReceiptFieldRow|kWorldAuthoringReceiptFields|appendProductWorldAuthoringFields|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/WorldAuthoringFields.cpp
```

Expected:

- the file-local row type exists
- the ordered row array exists
- `appendProductWorldAuthoringFields(...)` remains the only public appender
- `appendReceiptField(...)` calls live inside row callbacks, with no shared
  helper introduced

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|WorldAuthoringReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/WorldAuthoringFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `WorldAuthoringReceiptFieldRow` appears only in
  `WorldAuthoringFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/WorldAuthoringFields.cpp
```

Expected:

- `91`

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_receipt_key_order_tests$' --output-on-failure
(cd /Users/kogaryu/iggy3d && /Users/kogaryu/iggy3d/build/product_receipt_key_order_tests)
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row table changes any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- direct bool/string/count/hash formatting or overload selection changes
- compile fallout expands beyond include repairs in `WorldAuthoringFields.cpp`

## Completion Brief Checklist

Report:

- files changed
- exact row helper shape
- row count
- first and last receipt keys
- whether any rows were left procedural
- required grep classifications
- focused build/CTest/direct oracle results
- receipt golden diff result
- diff/whitespace check results
- confirmation that no shared helper, CMake, tests, golden, staging, commit,
  push, broad CTest, or window launch was performed

## Completion Brief

Completed.

Files changed:

- `src/app/iggy3d/receipt/WorldAuthoringFields.cpp`
- `docs/creative_mode/builder_tasks/done/E253-product-receipt-field-rows-g3-world-authoring.md`

Implemented a file-local callback row table:

- `WorldAuthoringReceiptFieldRow`
- `const std::array<WorldAuthoringReceiptFieldRow, 91> kWorldAuthoringReceiptFields`
- `appendProductWorldAuthoringFields(...)` now iterates the ordered rows and calls `row.append(receipt, window, row.key)`.

Row coverage:

- Row count: 91
- First key: `world_setup_title`
- Last key: `room_editing_last_primitive_id`
- Procedural rows left behind: none

Required grep classifications:

- `WorldAuthoringReceiptFieldRow` and `kWorldAuthoringReceiptFields` exist only in `WorldAuthoringFields.cpp`.
- `appendProductWorldAuthoringFields(...)` remains the only public appender in the file.
- `appendReceiptField(...)` count is 91, all inside row callbacks.
- No `ProductAppReceiptContext` was introduced.
- No shared `ReceiptFieldRow` helper was introduced.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_receipt_key_order_tests$' --output-on-failure` passed.
- Direct oracle passed: `receipt key-order oracle: 1032 fields match golden (order + values)`.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over `WorldAuthoringFields.cpp` and this card passed.

No shared helper, CMake edit, tests edit, golden edit, staging, commit, push, broad CTest, or window launch was performed.
