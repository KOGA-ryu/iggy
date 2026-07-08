# E252: Product Receipt Field Rows G2 - Gameplay Runtime Movement

## Objective

Apply the proven local receipt row-table pattern to
`src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`.

This is the second receipt-field pilot after E251. It should remain a
mechanical refactor only: preserve the public appender signature, field order,
field keys, field values, formatting, and receipt golden output exactly.

## Current Context

E251 proved the one-file local row-table pattern in
`GameplaySceneStateFields.cpp`:

- file-local row type
- ordered `std::array`
- no shared helper
- no `ProductAppReceiptContext`
- receipt key-order oracle stayed byte-identical

E250 classified `GameplayRuntimeMovementFields.cpp` as the next clean direct
candidate:

- 240 lines
- 116 `appendReceiptField(...)` rows
- one appender function
- no loops
- no conditionals
- values come from:
  - `ProductAppWindowState`
  - `ProductMovementProofPacket`
  - `runtimeStateHash`
- values use direct bool/string/count fields plus:
  - `floatReceiptValue(...)`
  - `std::uint64_t runtimeStateHash`

The current appender starts with:

- `runtime_session_created`

and ends with:

- `gameplay_dash_direction_z`

## Scope

Edit only:

- `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
- this task card when moving it to `done/`

Add a file-local context type, row type, and ordered row array inside
`GameplayRuntimeMovementFields.cpp`.

Preserve the existing public API:

```cpp
void appendProductGameplayRuntimeMovementFields(
    RenderReceipt& receipt,
    const ProductAppWindowState& window,
    const ProductMovementProofPacket& movementProof,
    std::uint64_t runtimeStateHash);
```

The public appender should build a file-local context and iterate the ordered
rows.

## Required Row Shape

Use a file-local append-callback row. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct GameplayRuntimeMovementReceiptContext {
  const ProductAppWindowState& window;
  const ProductMovementProofPacket& movementProof;
  std::uint64_t runtimeStateHash;
};

struct GameplayRuntimeMovementReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const GameplayRuntimeMovementReceiptContext& context,
                 std::string_view key);
};

const std::array<GameplayRuntimeMovementReceiptFieldRow, 116>
    kGameplayRuntimeMovementReceiptFields{{
        {"runtime_session_created",
         [](RenderReceipt& receipt,
            const GameplayRuntimeMovementReceiptContext& context,
            std::string_view key) {
           appendReceiptField(receipt, key,
                              context.window.gameplay.runtimeSessionCreated);
         }},
        // Preserve every existing row in current order.
    }};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep `floatReceiptValue(...)` calls inside row callbacks and pass results
directly to `appendReceiptField(...)`, matching current code.

## Required Behavior Preservation

Preserve every current row from
`appendProductGameplayRuntimeMovementFields(...)`:

- same key literal
- same order
- same value expression
- same formatter call
- same casts and runtime hash value

The appender must still start with:

- `runtime_session_created`

The appender must still end with:

- `gameplay_dash_direction_z`

The table must cover all 116 current rows. Do not leave direct procedural rows
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
rg -n "GameplayRuntimeMovementReceiptContext|GameplayRuntimeMovementReceiptFieldRow|kGameplayRuntimeMovementReceiptFields|appendProductGameplayRuntimeMovementFields|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp
```

Expected:

- the file-local context and row types exist
- the ordered row array exists
- `appendProductGameplayRuntimeMovementFields(...)` remains the only public
  appender
- `appendReceiptField(...)` calls live inside row callbacks, with no shared
  helper introduced

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|GameplayRuntimeMovementReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `GameplayRuntimeMovementReceiptFieldRow` appears only in
  `GameplayRuntimeMovementFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp
```

Expected:

- `116`

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

- `src/app/iggy3d/receipt/GameplayRuntimeMovementFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row table changes any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- `floatReceiptValue(...)`, bool/string formatting, casts, or
  `runtimeStateHash` output changes
- compile fallout expands beyond include repairs in
  `GameplayRuntimeMovementFields.cpp`

## Completion Brief Checklist

Report:

- files changed
- exact context/row helper shape
- row count
- first and last receipt keys
- whether any rows were left procedural
- required grep classifications
- focused build/CTest/direct oracle results
- receipt golden diff result
- diff/whitespace check results
- confirmation that no shared helper, CMake, tests, golden, staging, commit,
  push, broad CTest, or window launch was performed
