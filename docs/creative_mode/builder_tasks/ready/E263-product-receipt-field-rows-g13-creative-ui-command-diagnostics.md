# E263: Product Receipt Field Rows G13 - Creative UI Command Diagnostics

## Objective

Apply the proven local receipt row-table pattern to the fixed creative UI
command diagnostic helper rows in
`src/app/iggy3d/receipt/CreativeUiFields.cpp`.

This is the thirteenth receipt-field row slice after E251-E262. Keep it
mechanical: preserve helper signatures, field order, field keys, field values,
formatting, helper call order, and receipt golden output exactly.

## Current Context

The local receipt row-table pattern has been applied successfully through:

- E251: `GameplaySceneStateFields.cpp`
- E252: `GameplayRuntimeMovementFields.cpp`
- E253: `WorldAuthoringFields.cpp`
- E254: `FeedbackSurfaceAutomationVulkanFields.cpp`
- E255: `CreativePickWireframeFields.cpp`
- E256: `StartupProbeFields.cpp`
- E257: `ActiveRoomFields.cpp`
- E258: `DebugHudFields.cpp`
- E259: `TailFields.cpp`
- E260: `SaveStateFields.cpp`
- E261: `FrontendSettingsWindowFields.cpp`
- E262: fixed rows in `CreativeUiFields.cpp`'s public
  `appendProductCreativeUiFields(...)` appender

Each completed slice used file-local row tables, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

After E262, `CreativeUiFields.cpp` still owns procedural creative command
diagnostic fixed rows:

- `appendProductCreativeUiCommandMutationFields(...)`: 16 rows
- `appendProductCreativeUiCommandCreateFields(...)`: 12 rows
- `appendProductCreativeUiCommandDeleteFields(...)`: 13 rows
- `appendProductCreativeUiCommandUndoFields(...)`: 14 rows
- `appendProductCreativeUiCommandRoomShellFields(...)`: 13 rows
- fixed top rows in `appendProductCreativeUiCommandFields(...)`: 14 rows

The baked-room refresh diagnostic helper is intentionally different because it
uses a reusable key-set and an optional `clearedActiveRoom` key branch. Leave
that path procedural in this slice.

## Scope

Edit only:

- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- this task card when moving it to `done/`

Add file-local append-callback row tables for the command diagnostic helpers
listed above.

Preserve these helper signatures:

```cpp
void appendProductCreativeUiCommandMutationFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandMutationDiagnostics& fields);

void appendProductCreativeUiCommandCreateFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandCreateDiagnostics& fields);

void appendProductCreativeUiCommandDeleteFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandDeleteDiagnostics& fields);

void appendProductCreativeUiCommandUndoFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandUndoDiagnostics& fields);

void appendProductCreativeUiCommandRoomShellFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandRoomShellDiagnostics& fields);

void appendProductCreativeUiCommandFields(
    RenderReceipt& receipt,
    const ProductCreativeUiCommandDiagnostics& fields);
```

Each helper should iterate its ordered row table. The command facade helper
must still call the sub-helper appenders in the current order after its own
top-level fixed rows:

1. mutation
2. create
3. delete
4. undo
5. room shell
6. baked-room refresh

## Required Row Shape

Use file-local append-callback rows. Do not add a shared helper yet.

A file-local typed row alias/template is acceptable if it stays inside
`CreativeUiFields.cpp` and keeps the callback payload type explicit:

```cpp
template <typename Fields>
struct CreativeUiCommandReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const Fields& fields,
                 std::string_view key);
};
```

Alternatively, use one file-local row type per diagnostics payload. The
important constraints are:

- rows are ordered `const std::array` values;
- callbacks are non-capturing;
- each callback calls `appendReceiptField(...)` directly;
- current value expressions stay inside callbacks;
- no shared receipt helper or `ProductAppReceiptContext` is introduced.

Example:

```cpp
const std::array<
    CreativeUiCommandReceiptFieldRow<
        ProductCreativeUiCommandMutationDiagnostics>,
    16>
    kCreativeUiCommandMutationReceiptFields{{
        {"creative_ui_command_mutation_requested",
         [](RenderReceipt& receipt,
            const ProductCreativeUiCommandMutationDiagnostics& fields,
            std::string_view key) {
           appendReceiptField(receipt, key, fields.requested);
         }},
        // Preserve every current row in current order.
    }};
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

## Required Behavior Preservation

Preserve every current fixed row in the command diagnostic helpers:

- same key literal
- same order
- same value expression
- same formatter behavior and overload selection

The row tables must cover:

- mutation rows from `creative_ui_command_mutation_requested` through
  `creative_ui_command_mutation_message`
- create rows from `creative_ui_command_create_requested` through
  `creative_ui_command_create_reason_code`
- delete rows from `creative_ui_command_delete_requested` through
  `creative_ui_command_delete_reason_code`
- undo rows from `creative_ui_command_undo_requested` through
  `creative_ui_command_undo_reason_code`
- room-shell rows from `creative_ui_command_shell_requested` through
  `creative_ui_command_shell_message`
- command facade rows from `creative_ui_command_requested` through
  `creative_ui_command_reason_code`

Preserve command facade sub-helper call order exactly:

```cpp
appendProductCreativeUiCommandMutationFields(receipt, fields.mutation);
appendProductCreativeUiCommandCreateFields(receipt, fields.create);
appendProductCreativeUiCommandDeleteFields(receipt, fields.deleteObject);
appendProductCreativeUiCommandUndoFields(receipt, fields.undo);
appendProductCreativeUiCommandRoomShellFields(receipt, fields.shell);
appendProductCreativeUiCommandBakedRoomRefreshFields(
    receipt, fields.bakedRoomRefresh);
```

Do not table-drive or otherwise change:

- `ProductCreativeBakedRoomRefreshReceiptKeySet`
- `appendProductCreativeBakedRoomRefreshDiagnosticFields(...)`
- `appendProductCreativeUiCommandBakedRoomRefreshFields(...)`
- `appendProductCreativeBakedRoomAutoRefreshFields(...)`
- the `if (!keys.clearedActiveRoom.empty())` optional-key branch
- the public `appendProductCreativeUiFields(...)` row tables added by E262

Do not leave any in-scope command fixed rows procedural unless a compile issue
forces it; if that happens, stop and report the exact row instead of widening
the design.

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

Do not table-drive baked-room refresh diagnostic helper appenders.
Do not add a shared receipt row helper.
Do not change receipt keys, order, values, status strings, stringifier outputs,
or formatting.
Do not split creative UI sections into multiple public appenders.
Do not perform broad include cleanup beyond what is needed to compile this file.
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "CreativeUiCommand.*ReceiptFieldRow|kCreativeUiCommand.*ReceiptFields|appendProductCreativeUiCommandMutationFields|appendProductCreativeUiCommandCreateFields|appendProductCreativeUiCommandDeleteFields|appendProductCreativeUiCommandUndoFields|appendProductCreativeUiCommandRoomShellFields|appendProductCreativeUiCommandFields|appendProductCreativeBakedRoomRefreshDiagnosticFields|appendProductCreativeUiCommandBakedRoomRefreshFields|appendProductCreativeBakedRoomAutoRefreshFields|clearedActiveRoom|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- file-local command row type(s) exist;
- ordered command row arrays exist;
- in-scope command helper `appendReceiptField(...)` calls live inside row
  callbacks;
- command facade helper still calls sub-helper appenders in order;
- baked-room refresh helper, auto-refresh helper, and `clearedActiveRoom`
  optional-key branch remain procedural.

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|CreativeUiCommand.*ReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- no `ProductAppReceiptContext`;
- no shared `ReceiptFieldRow` in `ReceiptBuilder.cpp` or
  `ReceiptFields.hpp/.cpp`;
- command row type(s) appear only in `CreativeUiFields.cpp`.

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- `159`

The count should remain 159 because this slice moves fixed command rows into
callbacks while the baked-room refresh diagnostic path keeps its current
procedural append call sites.

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

- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row tables change any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- baked-room refresh diagnostic helper appenders need to be table-driven
- the `clearedActiveRoom` optional-key branch needs to change
- direct bool/string/count formatting or overload selection changes
- compile fallout expands beyond include repairs in `CreativeUiFields.cpp`
- the implementation starts to require `ProductAppReceiptContext`

## Completion Brief Checklist

Report:

- files changed
- exact row helper shape
- row count for each command table
- whether command facade sub-helper call order stayed unchanged
- whether baked-room refresh diagnostics stayed procedural
- first and last receipt keys for each in-scope table
- whether any in-scope fixed rows were left procedural
- required grep classifications
- focused build/CTest/direct oracle results
- receipt golden diff result
- diff/whitespace check results
- confirmation that no shared helper, `ProductAppReceiptContext`, CMake, tests,
  golden, staging, commit, push, broad CTest, or window launch was performed
