# E262: Product Receipt Field Rows G12 - Creative UI Main Appender

## Objective

Apply the proven local receipt row-table pattern to the fixed rows in
`src/app/iggy3d/receipt/CreativeUiFields.cpp`'s public
`appendProductCreativeUiFields(...)` appender.

This is the twelfth receipt-field row slice after E251-E261. It should remain a
mechanical refactor only: preserve the public appender signature, field order,
field keys, field values, formatting, helper call order, and receipt golden
output exactly.

## Current Context

The local receipt row-table pattern has been applied successfully to eleven
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
- E260: `SaveStateFields.cpp`
- E261: `FrontendSettingsWindowFields.cpp`

Each completed slice used file-local row tables, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

E250 classified `CreativeUiFields.cpp` as a mixed table candidate because it
already has local command sub-appenders and a baked-room diagnostic helper with
one conditional optional key. Do not collapse those helpers in this slice.

Current `CreativeUiFields.cpp` shape:

- 464 lines
- 159 `appendReceiptField(...)` call sites
- command diagnostic helper appenders:
  - `appendProductCreativeUiCommandMutationFields(...)`
  - `appendProductCreativeUiCommandCreateFields(...)`
  - `appendProductCreativeUiCommandDeleteFields(...)`
  - `appendProductCreativeUiCommandUndoFields(...)`
  - `appendProductCreativeUiCommandRoomShellFields(...)`
  - `appendProductCreativeUiCommandFields(...)`
- baked-room refresh diagnostic helper:
  - `ProductCreativeBakedRoomRefreshReceiptKeySet`
  - `appendProductCreativeBakedRoomRefreshDiagnosticFields(...)`
  - its `if (!keys.clearedActiveRoom.empty())` optional-key branch
- public appender fixed rows:
  - 52 fixed rows before command and auto-refresh helper calls
  - 12 fixed rows after command and auto-refresh helper calls

The public appender's fixed row flow is currently:

1. `creative_ui_projection_requested`
2. ...fixed projection/input/last-input/downstream-click rows...
3. `creative_ui_input_downstream_click_reason_code`
4. `appendProductCreativeUiCommandFields(receipt, authoring.creativeUiCommand)`
5. `appendProductCreativeBakedRoomAutoRefreshFields(receipt,
   authoring.creativeBakedRoomAutoRefresh)`
6. `creative_document_revision_observed`
7. ...fixed document/undo/stale rows...
8. `creative_baked_room_stale_reason_code`

## Scope

Edit only:

- `src/app/iggy3d/receipt/CreativeUiFields.cpp`
- this task card when moving it to `done/`

Add a file-local row type and two ordered row arrays inside
`CreativeUiFields.cpp` for the public appender's fixed rows:

- pre-command fixed rows: 52 rows
- post-refresh fixed rows: 12 rows

Preserve the existing public API:

```cpp
void appendProductCreativeUiFields(RenderReceipt& receipt,
                                   const ProductAppWindowState& window);
```

The public appender should:

1. Keep `const auto& authoring = window.creativeAuthoring;`.
2. Iterate the pre-command fixed row table.
3. Call `appendProductCreativeUiCommandFields(...)` unchanged.
4. Call `appendProductCreativeBakedRoomAutoRefreshFields(...)` unchanged.
5. Iterate the post-refresh fixed row table.

## Required Row Shape

Use file-local append-callback rows. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct CreativeUiReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ProductAppWindowState& window,
                 const CreativeAuthoringStore& authoring,
                 std::string_view key);
};

const std::array<CreativeUiReceiptFieldRow, 52>
    kCreativeUiPreCommandReceiptFields{{
        {"creative_ui_projection_requested",
         [](RenderReceipt& receipt,
            const ProductAppWindowState&,
            const CreativeAuthoringStore& authoring,
            std::string_view key) {
           appendReceiptField(
               receipt, key, authoring.creativeUiProjection.requested);
         }},
        // Preserve every pre-command fixed row in current order.
    }};

const std::array<CreativeUiReceiptFieldRow, 12>
    kCreativeUiPostRefreshReceiptFields{{
        {"creative_document_revision_observed",
         [](RenderReceipt& receipt,
            const ProductAppWindowState&,
            const CreativeAuthoringStore& authoring,
            std::string_view key) {
           appendReceiptField(
               receipt, key, authoring.creativeDocumentRevision.observed);
         }},
        // Preserve every post-refresh fixed row in current order.
    }};

}  // namespace
```

If `CreativeAuthoringStore` is not the exact visible type name at this include
point, use the simplest file-local context type instead:

```cpp
struct CreativeUiReceiptContext {
  const ProductAppWindowState& window;
  const decltype(std::declval<const ProductAppWindowState&>().creativeAuthoring)&
      authoring;
};
```

or use a local context with whatever complete type is already available in the
file. Do not introduce a shared context or signature churn.

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep `static_cast<std::uint64_t>(...)`, direct bool/string/count values, and
overload selection inside row callbacks and pass results directly to
`appendReceiptField(...)`, matching current code.

## Required Behavior Preservation

Preserve every current fixed row from `appendProductCreativeUiFields(...)`:

- same key literal
- same order
- same value expression
- same `static_cast<std::uint64_t>(...)` calls
- same formatter behavior and overload selection

The pre-command table must still start with:

- `creative_ui_projection_requested`

The pre-command table must still end with:

- `creative_ui_input_downstream_click_reason_code`

The post-refresh table must still start with:

- `creative_document_revision_observed`

The post-refresh table must still end with:

- `creative_baked_room_stale_reason_code`

The command and baked-room helper calls must remain procedural and in the same
order between the two fixed row tables:

```cpp
appendProductCreativeUiCommandFields(receipt, authoring.creativeUiCommand);
appendProductCreativeBakedRoomAutoRefreshFields(
    receipt, authoring.creativeBakedRoomAutoRefresh);
```

Do not table-drive or otherwise change:

- `appendProductCreativeUiCommandMutationFields(...)`
- `appendProductCreativeUiCommandCreateFields(...)`
- `appendProductCreativeUiCommandDeleteFields(...)`
- `appendProductCreativeUiCommandUndoFields(...)`
- `appendProductCreativeUiCommandRoomShellFields(...)`
- `appendProductCreativeUiCommandFields(...)`
- `ProductCreativeBakedRoomRefreshReceiptKeySet`
- `appendProductCreativeBakedRoomRefreshDiagnosticFields(...)`
- `appendProductCreativeUiCommandBakedRoomRefreshFields(...)`
- `appendProductCreativeBakedRoomAutoRefreshFields(...)`
- the `if (!keys.clearedActiveRoom.empty())` optional-key branch

Do not leave any public-appender fixed direct procedural rows unless a compile
issue forces it; if that happens, stop and report the exact row instead of
widening the design.

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

Do not table-drive command diagnostic helper appenders.
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
rg -n "CreativeUiReceiptFieldRow|kCreativeUiPreCommandReceiptFields|kCreativeUiPostRefreshReceiptFields|appendProductCreativeUiFields|appendProductCreativeUiCommandFields|appendProductCreativeBakedRoomAutoRefreshFields|appendProductCreativeBakedRoomRefreshDiagnosticFields|clearedActiveRoom|appendReceiptField\\(" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- the file-local row type exists
- the two ordered row arrays exist
- `appendProductCreativeUiFields(...)` remains the only public appender
- public-appender fixed `appendReceiptField(...)` calls live inside row
  callbacks
- command helper calls remain procedural between the two row tables
- baked-room refresh helper and `clearedActiveRoom` optional-key branch remain
  procedural

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|CreativeUiReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `CreativeUiReceiptFieldRow` appears only in `CreativeUiFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
```

Expected:

- `159`

The count should remain 159 because this slice only moves public-appender fixed
rows into callbacks; helper appenders and the conditional baked-room diagnostic
path keep their existing append call sites.

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
- command or baked-room helper appenders need to be table-driven
- the `clearedActiveRoom` optional-key branch needs to change
- `static_cast<std::uint64_t>(...)`, direct bool/string/count formatting, or
  overload selection changes
- compile fallout expands beyond include repairs in `CreativeUiFields.cpp`
- the implementation starts to require `ProductAppReceiptContext`

## Completion Brief Checklist

Report:

- files changed
- exact row/context helper shape
- pre-command row count
- post-refresh row count
- whether command/baked-room helpers stayed procedural
- first and last fixed receipt keys
- whether any public-appender fixed rows were left procedural
- required grep classifications
- focused build/CTest/direct oracle results
- receipt golden diff result
- diff/whitespace check results
- confirmation that no shared helper, `ProductAppReceiptContext`, CMake, tests,
  golden, staging, commit, push, broad CTest, or window launch was performed
