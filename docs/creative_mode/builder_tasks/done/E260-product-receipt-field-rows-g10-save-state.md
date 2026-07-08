# E260: Product Receipt Field Rows G10 - Save State

## Objective

Apply the proven local receipt row-table pattern to
`src/app/iggy3d/receipt/SaveStateFields.cpp`.

This is the tenth receipt-field row slice after E251-E259. It is the first
larger mixed-context receipt file after the smaller direct candidates. Keep it
mechanical: preserve the public appender signature, field order, field keys,
field values, formatting, and receipt golden output exactly.

## Current Context

The local receipt row-table pattern has been applied successfully to nine
sections:

- E251: `GameplaySceneStateFields.cpp`
- E252: `GameplayRuntimeMovementFields.cpp`
- E253: `WorldAuthoringFields.cpp`
- E254: `FeedbackSurfaceAutomationVulkanFields.cpp`
- E255: `CreativePickWireframeFields.cpp`
- E256: `StartupProbeFields.cpp`
- E257: `ActiveRoomFields.cpp`
- E258: `DebugHudFields.cpp`
- E259: `TailFields.cpp`

Each completed slice used a file-local row table, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

E250 classified `SaveStateFields.cpp` as a mixed table candidate because it
needs a wider context than `ProductAppWindowState`, but the file shape is still
mechanically tableable:

- 333 lines
- 146 `appendReceiptField(...)` rows
- one appender function
- no loops
- no conditionals
- values come from:
  - `FrontendState`
  - `ProductAppWindowState`
  - `creative::CreativeActiveIdentity`
- values include direct bool/string/count/hash fields plus:
  - `std::to_string(...)`
  - `floatReceiptValue(...)`
  - `productRoomEditorToolName(...)`
  - `productRoomEditorDirectionName(...)`
  - `frontendSaveBrowserModeName(...)`

The current appender starts with:

- `product_save_status`

and ends with:

- `product_save_load_session_loaded`

## Scope

Edit only:

- `src/app/iggy3d/receipt/SaveStateFields.cpp`
- this task card when moving it to `done/`

Add a file-local context type, row type, and ordered row array inside
`SaveStateFields.cpp`.

Preserve the existing public API:

```cpp
void appendProductSaveStateFields(
    RenderReceipt& receipt,
    const FrontendState& frontend,
    const ProductAppWindowState& window,
    const creative::CreativeActiveIdentity& creativeIdentity);
```

The public appender should build a file-local context and iterate the ordered
rows.

## Required Row Shape

Use a file-local append-callback row. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct SaveStateReceiptContext {
  const FrontendState& frontend;
  const ProductAppWindowState& window;
  const creative::CreativeActiveIdentity& creativeIdentity;
};

struct SaveStateReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const SaveStateReceiptContext& context,
                 std::string_view key);
};

const std::array<SaveStateReceiptFieldRow, 146> kSaveStateReceiptFields{{
    {"product_save_status",
     [](RenderReceipt& receipt,
        const SaveStateReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key,
                          context.window.saveSession.productSaveStatus);
     }},
    // Preserve every existing row in current order.
}};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep `std::to_string(...)`, `floatReceiptValue(...)`, room-editor stringifiers,
frontend save-browser stringifier, direct bool/string/count/hash values, and
overload selection inside row callbacks and pass results directly to
`appendReceiptField(...)`, matching current code.

## Required Behavior Preservation

Preserve every current row from `appendProductSaveStateFields(...)`:

- same key literal
- same order
- same value expression
- same `std::to_string(...)` calls
- same `floatReceiptValue(...)` calls
- same stringifier calls
- same formatter behavior and overload selection

The appender must still start with:

- `product_save_status`

The appender must still end with:

- `product_save_load_session_loaded`

The table must cover all 146 current rows. Do not leave direct procedural rows
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
Do not split save-state sections into multiple public appenders.
Do not perform broad include cleanup beyond what is needed to compile this file.
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "SaveStateReceiptContext|SaveStateReceiptFieldRow|kSaveStateReceiptFields|appendProductSaveStateFields|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/SaveStateFields.cpp
```

Expected:

- the file-local context and row types exist
- the ordered row array exists
- `appendProductSaveStateFields(...)` remains the only public appender
- `appendReceiptField(...)` calls live inside row callbacks, with no shared
  helper introduced

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|SaveStateReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/SaveStateFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `SaveStateReceiptFieldRow` appears only in `SaveStateFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/SaveStateFields.cpp
```

Expected:

- `146`

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

- `src/app/iggy3d/receipt/SaveStateFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row table changes any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- `std::to_string(...)`, `floatReceiptValue(...)`, stringifier output, direct
  bool/string/count/hash formatting, or overload selection changes
- compile fallout expands beyond include repairs in `SaveStateFields.cpp`
- the implementation starts to require `ProductAppReceiptContext`

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
- confirmation that no shared helper, `ProductAppReceiptContext`, CMake, tests,
  golden, staging, commit, push, broad CTest, or window launch was performed

## Completion Brief

Status: Done.

Files changed:

- `src/app/iggy3d/receipt/SaveStateFields.cpp`
- `docs/creative_mode/builder_tasks/done/E260-product-receipt-field-rows-g10-save-state.md`

Refactor shape:

- Added file-local `SaveStateReceiptContext` with references to
  `FrontendState`, `ProductAppWindowState`, and
  `creative::CreativeActiveIdentity`.
- Added file-local `SaveStateReceiptFieldRow` with `std::string_view key` and
  an append callback:
  `void (*append)(RenderReceipt&, const SaveStateReceiptContext&,
  std::string_view)`.
- Added ordered `const std::array<SaveStateReceiptFieldRow, 146>
  kSaveStateReceiptFields`.
- `appendProductSaveStateFields(...)` now builds a `SaveStateReceiptContext`
  and iterates the ordered rows.

Row coverage:

- Row count: 146.
- First key: `product_save_status`.
- Last key: `product_save_load_session_loaded`.
- Rows left procedural: none.
- `std::to_string(...)`, `floatReceiptValue(...)`,
  `productRoomEditorToolName(...)`, `productRoomEditorDirectionName(...)`,
  `frontendSaveBrowserModeName(...)`, and direct bool/string/count/hash values
  remain inside row callbacks.

Required grep classification:

- `SaveStateReceiptContext`, `SaveStateReceiptFieldRow`, and
  `kSaveStateReceiptFields` exist only in `SaveStateFields.cpp`.
- `appendProductSaveStateFields(...)` remains the only public appender.
- `appendReceiptField(...)` calls live inside row callbacks.
- `rg -c "appendReceiptField\\(" SaveStateFields.cpp` returned `146`.
- No `ProductAppReceiptContext` or shared `ReceiptFieldRow` was introduced.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d
  product_receipt_key_order_tests -j10`: passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R
  '^product_receipt_key_order_tests$' --output-on-failure`: passed.
- `(cd /Users/kogaryu/iggy3d &&
  /Users/kogaryu/iggy3d/build/product_receipt_key_order_tests)`: passed with
  `receipt key-order oracle: 1032 fields match golden (order + values)`.
- `git -C /Users/kogaryu/iggy3d diff --
  tests/golden/product_receipt_key_order.golden`: no diff.
- `git -C /Users/kogaryu/iggy3d diff --check`: passed.
- Focused trailing-whitespace scan over `SaveStateFields.cpp` and this task
  card: passed.

Scope confirmation:

- No shared receipt helper, `ProductAppReceiptContext`, CMake, tests, fixtures,
  receipt golden files, staging, commit, push, broad CTest, or window launch
  were performed.
