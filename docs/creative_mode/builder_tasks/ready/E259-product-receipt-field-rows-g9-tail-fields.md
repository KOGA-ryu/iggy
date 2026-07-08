# E259: Product Receipt Field Rows G9 - Tail Fields

## Objective

Apply the proven local receipt row-table pattern to
`src/app/iggy3d/receipt/TailFields.cpp`.

This is the ninth receipt-field row slice after E251-E258. It should remain a
mechanical refactor only: preserve the public appender signature, field order,
field keys, field values, formatting, and receipt golden output exactly.

## Current Context

The receipt row-table pattern has now been applied successfully to eight direct
receipt sections:

- E251: `GameplaySceneStateFields.cpp`
- E252: `GameplayRuntimeMovementFields.cpp`
- E253: `WorldAuthoringFields.cpp`
- E254: `FeedbackSurfaceAutomationVulkanFields.cpp`
- E255: `CreativePickWireframeFields.cpp`
- E256: `StartupProbeFields.cpp`
- E257: `ActiveRoomFields.cpp`
- E258: `DebugHudFields.cpp`

Each completed slice used a file-local row table, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

E250 classified `TailFields.cpp` as a direct table candidate:

- 41 lines
- 12 `appendReceiptField(...)` rows
- one appender function
- no loops
- no conditionals
- values come from:
  - `ProductAppOptions`
  - `ProductWorldTemplate`
  - `ProductAppWindowState`
  - `ProductSaveBridgeResult`
- values are direct bool/string/count fields plus:
  - `static_cast<std::uint64_t>(...)`
  - `saves.saveRoot.generic_string()`
  - the existing literal `false` for `normal_package_cli`

The current appender starts with:

- `event_poll_count`

and ends with:

- `normal_package_cli`

## Scope

Edit only:

- `src/app/iggy3d/receipt/TailFields.cpp`
- this task card when moving it to `done/`

Add a file-local context type, row type, and ordered row array inside
`TailFields.cpp`.

Preserve the existing public API:

```cpp
void appendProductTailFields(RenderReceipt& receipt,
                             const ProductAppOptions& options,
                             const ProductWorldTemplate& world,
                             const ProductAppWindowState& window,
                             const ProductSaveBridgeResult& saves);
```

The public appender should build a file-local context and iterate the ordered
rows.

## Required Row Shape

Use a file-local append-callback row. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct TailReceiptContext {
  const ProductAppOptions& options;
  const ProductWorldTemplate& world;
  const ProductAppWindowState& window;
  const ProductSaveBridgeResult& saves;
};

struct TailReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const TailReceiptContext& context,
                 std::string_view key);
};

const std::array<TailReceiptFieldRow, 12> kTailReceiptFields{{
    {"event_poll_count",
     [](RenderReceipt& receipt,
        const TailReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key,
                          context.window.frontendShell.eventPollCount);
     }},
    // Preserve every existing row in current order.
}};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep `static_cast<std::uint64_t>(...)`, `generic_string()`, direct bool/string
formatting, and the `false` literal inside row callbacks and pass results
directly to `appendReceiptField(...)`, matching current code.

## Required Behavior Preservation

Preserve every current row from `appendProductTailFields(...)`:

- same key literal
- same order
- same value expression
- same `static_cast<std::uint64_t>(...)`
- same `saves.saveRoot.generic_string()` output
- same literal `false` output for `normal_package_cli`
- same formatter behavior and overload selection

The appender must still start with:

- `event_poll_count`

The appender must still end with:

- `normal_package_cli`

The table must cover all 12 current rows. Do not leave direct procedural rows
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
Do not change receipt keys, order, values, status strings, stringifier outputs,
or formatting.
Do not perform broad include cleanup beyond what is needed to compile this file.
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "TailReceiptContext|TailReceiptFieldRow|kTailReceiptFields|appendProductTailFields|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/TailFields.cpp
```

Expected:

- the file-local context and row types exist
- the ordered row array exists
- `appendProductTailFields(...)` remains the only public appender
- `appendReceiptField(...)` calls live inside row callbacks, with no shared
  helper introduced

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|TailReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/TailFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `TailReceiptFieldRow` appears only in `TailFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/TailFields.cpp
```

Expected:

- `12`

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

- `src/app/iggy3d/receipt/TailFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row table changes any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- direct bool/string/count formatting, `static_cast<std::uint64_t>(...)`,
  `generic_string()`, the `false` literal, or overload selection changes
- compile fallout expands beyond include repairs in `TailFields.cpp`

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
