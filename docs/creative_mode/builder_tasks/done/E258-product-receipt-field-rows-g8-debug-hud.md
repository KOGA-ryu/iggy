# E258: Product Receipt Field Rows G8 - Debug HUD

## Objective

Apply the proven local receipt row-table pattern to
`src/app/iggy3d/receipt/DebugHudFields.cpp`.

This is the eighth receipt-field row slice after E251-E257. It should remain a
mechanical refactor only: preserve the public appender signature, field order,
field keys, field values, formatting, and receipt golden output exactly.

## Current Context

The receipt row-table pattern has now been applied successfully to seven direct
receipt sections:

- E251: `GameplaySceneStateFields.cpp`
- E252: `GameplayRuntimeMovementFields.cpp`
- E253: `WorldAuthoringFields.cpp`
- E254: `FeedbackSurfaceAutomationVulkanFields.cpp`
- E255: `CreativePickWireframeFields.cpp`
- E256: `StartupProbeFields.cpp`
- E257: `ActiveRoomFields.cpp`

Each completed slice used a file-local row table, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

E250 classified `DebugHudFields.cpp` as a direct table candidate:

- 87 lines
- 27 `appendReceiptField(...)` rows
- one appender function
- no loops
- no conditionals
- values come from:
  - `ProductAppWindowState`
  - `MovementDebugHud`
  - `NpcBehaviorDebugHud`
  - `PhysicsDebugHud`
- values are direct bool/string/count fields plus:
  - `static_cast<std::uint64_t>(...)`
  - `floatReceiptValue(...)`

The current appender starts with:

- `movement_debug_hud_visible`

and ends with:

- `physics_debug_hud_has_warnings`

## Scope

Edit only:

- `src/app/iggy3d/receipt/DebugHudFields.cpp`
- this task card when moving it to `done/`

Add a file-local context type, row type, and ordered row array inside
`DebugHudFields.cpp`.

Preserve the existing public API:

```cpp
void appendProductDebugHudFields(RenderReceipt& receipt,
                                 const ProductAppWindowState& window,
                                 const MovementDebugHud& movementHud,
                                 const NpcBehaviorDebugHud& npcBehaviorHud,
                                 const PhysicsDebugHud& physicsHud);
```

The public appender should build a file-local context and iterate the ordered
rows.

## Required Row Shape

Use a file-local append-callback row. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct DebugHudReceiptContext {
  const ProductAppWindowState& window;
  const MovementDebugHud& movementHud;
  const NpcBehaviorDebugHud& npcBehaviorHud;
  const PhysicsDebugHud& physicsHud;
};

struct DebugHudReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const DebugHudReceiptContext& context,
                 std::string_view key);
};

const std::array<DebugHudReceiptFieldRow, 27> kDebugHudReceiptFields{{
    {"movement_debug_hud_visible",
     [](RenderReceipt& receipt,
        const DebugHudReceiptContext& context,
        std::string_view key) {
       appendReceiptField(receipt, key, context.movementHud.visible);
     }},
    // Preserve every existing row in current order.
}};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep `static_cast<std::uint64_t>(...)` and `floatReceiptValue(...)` calls inside
row callbacks and pass results directly to `appendReceiptField(...)`, matching
current code.

## Required Behavior Preservation

Preserve every current row from `appendProductDebugHudFields(...)`:

- same key literal
- same order
- same value expression
- same `static_cast<std::uint64_t>(...)`
- same `floatReceiptValue(...)` formatting
- same formatter behavior and overload selection

The appender must still start with:

- `movement_debug_hud_visible`

The appender must still end with:

- `physics_debug_hud_has_warnings`

The table must cover all 27 current rows. Do not leave direct procedural rows
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
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "DebugHudReceiptContext|DebugHudReceiptFieldRow|kDebugHudReceiptFields|appendProductDebugHudFields|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/DebugHudFields.cpp
```

Expected:

- the file-local context and row types exist
- the ordered row array exists
- `appendProductDebugHudFields(...)` remains the only public appender
- `appendReceiptField(...)` calls live inside row callbacks, with no shared
  helper introduced

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|DebugHudReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/DebugHudFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `DebugHudReceiptFieldRow` appears only in `DebugHudFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/DebugHudFields.cpp
```

Expected:

- `27`

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

- `src/app/iggy3d/receipt/DebugHudFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row table changes any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- direct bool/string/count formatting, `static_cast<std::uint64_t>(...)`,
  `floatReceiptValue(...)`, or overload selection changes
- compile fallout expands beyond include repairs in `DebugHudFields.cpp`

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

## Completion Brief

Completed.

Files changed:

- `src/app/iggy3d/receipt/DebugHudFields.cpp`
- `docs/creative_mode/builder_tasks/done/E258-product-receipt-field-rows-g8-debug-hud.md`

Implemented a file-local context and callback row table:

- `DebugHudReceiptContext`
- `DebugHudReceiptFieldRow`
- `const std::array<DebugHudReceiptFieldRow, 27> kDebugHudReceiptFields`
- `appendProductDebugHudFields(...)` now builds the context and iterates the ordered rows with `row.append(receipt, context, row.key)`.

Row coverage:

- Row count: 27
- First key: `movement_debug_hud_visible`
- Last key: `physics_debug_hud_has_warnings`
- Procedural rows left behind: none

Required grep classifications:

- `DebugHudReceiptContext`, `DebugHudReceiptFieldRow`, and `kDebugHudReceiptFields` exist in `DebugHudFields.cpp`.
- `appendProductDebugHudFields(...)` remains the only public appender in the file.
- `appendReceiptField(...)` count is 27, all inside row callbacks.
- No `ProductAppReceiptContext` was introduced.
- No shared `ReceiptFieldRow` helper was introduced.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests -j10` passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_receipt_key_order_tests$' --output-on-failure` passed.
- Direct oracle passed: `receipt key-order oracle: 1032 fields match golden (order + values)`.
- `git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden` produced no diff.
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over `DebugHudFields.cpp` and this card passed.

No shared helper, CMake edit, tests edit, golden edit, staging, commit, push, broad CTest, or window launch was performed.
