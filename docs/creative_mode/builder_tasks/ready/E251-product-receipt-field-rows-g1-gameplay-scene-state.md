# E251: Product Receipt Field Rows G1 - Gameplay Scene State

## Objective

Pilot the receipt field table-drive pattern in the safest one-file section:
`src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`.

This is a mechanical refactor only. Preserve the public appender signature,
field order, field keys, field values, formatting, and receipt golden output
exactly.

## Current Context

E250 preflight classified `GameplaySceneStateFields.cpp` as the best first
pilot:

- 386 lines
- 178 `appendReceiptField(...)` rows
- one appender function
- no loops
- no conditionals
- no local helper functions
- all values available from `ProductAppWindowState`
- values are direct bool/string/count fields plus:
  - `std::to_string(...)`
  - `static_cast<std::uint64_t>(...)`
  - `floatReceiptValue(...)`
  - one provenance stringifier

The receipt golden places this section from:

- first key: `position_hud_visible`
- last key: `product_vulkan_room_geometry_signature`

## Scope

Edit only:

- `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
- this task card when moving it to `done/`

Add a file-local row type and ordered row array inside
`GameplaySceneStateFields.cpp`.

Preserve the existing public API:

```cpp
void appendProductGameplaySceneStateFields(RenderReceipt& receipt,
                                           const ProductAppWindowState& window);
```

The public appender should iterate the ordered rows and append each field.

## Required Row Shape

Use a file-local append-callback row. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct GameplaySceneStateReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ProductAppWindowState& window,
                 std::string_view key);
};

const std::array<GameplaySceneStateReceiptFieldRow, 178>
    kGameplaySceneStateReceiptFields{{
        {"position_hud_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key,
                              window.debugHud.positionHud.visible);
         }},
        // Preserve every existing row in current order.
    }};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears. The important behavior is ordered static rows, not compile-time
evaluation.

If a row has temporary string ownership through `std::to_string(...)` or
`floatReceiptValue(...)`, keep that call inside the row callback and pass the
result directly to `appendReceiptField(...)`, matching the current code.

## Required Behavior Preservation

Preserve every current row from `appendProductGameplaySceneStateFields(...)`:

- same key literal
- same order
- same value expression
- same formatter call
- same casts
- same provenance stringifier output

The appender must still start with:

- `position_hud_visible`

The appender must still end with:

- `product_vulkan_room_geometry_signature`

The table must cover all 178 current rows. Do not leave direct procedural rows
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
rg -n "GameplaySceneStateReceiptFieldRow|kGameplaySceneStateReceiptFields|appendProductGameplaySceneStateFields|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplaySceneStateFields.cpp
```

Expected:

- the file-local row type exists
- the ordered row array exists
- `appendProductGameplaySceneStateFields(...)` remains the only public appender
- `appendReceiptField(...)` calls live inside row callbacks, with no broad
  shared helper introduced

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|GameplaySceneStateReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplaySceneStateFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `GameplaySceneStateReceiptFieldRow` appears only in
  `GameplaySceneStateFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplaySceneStateFields.cpp
```

Expected:

- `178`

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_receipt_key_order_tests$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row table changes any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- `floatReceiptValue(...)`, `std::to_string(...)`, bool formatting, casts, or
  the provenance stringifier output changes
- compile fallout expands beyond include repairs in
  `GameplaySceneStateFields.cpp`

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
