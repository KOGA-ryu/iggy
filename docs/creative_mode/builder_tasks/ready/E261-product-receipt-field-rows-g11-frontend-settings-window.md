# E261: Product Receipt Field Rows G11 - Frontend Settings Window

## Objective

Apply the proven local receipt row-table pattern to the fixed rows in
`src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`, while keeping the
existing gameplay movement tuning descriptor loop procedural.

This is the eleventh receipt-field row slice after E251-E260. It is a mixed
candidate because one dynamic key loop must remain local. Preserve the public
appender signature, field order, field keys, field values, formatting, loop
behavior, and receipt golden output exactly.

## Current Context

The local receipt row-table pattern has been applied successfully to ten
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

Each completed slice used file-local row tables, did not add a shared helper or
`ProductAppReceiptContext`, and kept the receipt golden byte-identical.

E250 classified `FrontendSettingsWindowFields.cpp` as a mixed table candidate:

- 213 lines
- 92 `appendReceiptField(...)` call sites
- one appender function
- one existing descriptor loop over `kProductGameplayMovementTuningFields`
- values come from:
  - `ProductAppOptions`
  - `FrontendState`
  - `FrontendSettings`
  - `ProductAppWindowState`
  - `ProductCreativeSurfaceKind`
  - `mapMakerLive`
- fixed values include direct bool/string/count fields plus:
  - `floatReceiptValue(...)`
  - frontend/settings/window/input stringifiers
  - ternary strings for `window_shell` and `window_title`

Current fixed-row shape:

- 33 fixed rows before the tuning descriptor loop:
  - first key: `app`
  - last key before loop: `gameplay_movement_tuning_selected_field`
- 2 procedural `appendReceiptField(...)` call sites inside the tuning descriptor
  loop, which emit dynamic keys named
  `gameplay_movement_tuning_<descriptor.name>`
- 57 fixed rows after the tuning descriptor loop:
  - first key after loop: `window_requested`
  - last key: `controller_action_input_action`

## Scope

Edit only:

- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
- this task card when moving it to `done/`

Add a file-local context type, row type, and two ordered row arrays inside
`FrontendSettingsWindowFields.cpp`:

- pre-tuning fixed rows: 33 rows
- post-tuning fixed rows: 57 rows

Preserve the existing public API:

```cpp
void appendProductFrontendSettingsWindowFields(
    RenderReceipt& receipt,
    const ProductAppOptions& options,
    const FrontendState& frontend,
    const FrontendSettings& settings,
    const ProductAppWindowState& window,
    ProductCreativeSurfaceKind creativeSurface,
    bool mapMakerLive);
```

The public appender should:

1. Build a file-local context.
2. Iterate the pre-tuning fixed row table.
3. Run the existing gameplay movement tuning descriptor loop unchanged.
4. Iterate the post-tuning fixed row table.

## Required Row Shape

Use file-local append-callback rows. Do not add a shared helper yet.

Suggested shape:

```cpp
namespace {

struct FrontendSettingsWindowReceiptContext {
  const ProductAppOptions& options;
  const FrontendState& frontend;
  const FrontendSettings& settings;
  const ProductAppWindowState& window;
  ProductCreativeSurfaceKind creativeSurface;
  bool mapMakerLive;
};

struct FrontendSettingsWindowReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const FrontendSettingsWindowReceiptContext& context,
                 std::string_view key);
};

const std::array<FrontendSettingsWindowReceiptFieldRow, 33>
    kFrontendSettingsWindowPreludeReceiptFields{{ /* app ... selected_field */ }};

const std::array<FrontendSettingsWindowReceiptFieldRow, 57>
    kFrontendSettingsWindowPostTuningReceiptFields{{
        /* window_requested ... controller_action_input_action */
    }};

}  // namespace
```

Use `const std::array` rather than forcing `constexpr` if any compiler issue
appears.

Keep `floatReceiptValue(...)`, stringifier calls, ternary expressions, direct
bool/string/count values, and overload selection inside row callbacks and pass
results directly to `appendReceiptField(...)`, matching current code.

## Required Behavior Preservation

Preserve every current fixed row from
`appendProductFrontendSettingsWindowFields(...)`:

- same key literal
- same order
- same value expression
- same `floatReceiptValue(...)` calls
- same stringifier calls
- same ternary expressions
- same formatter behavior and overload selection

The pre-tuning table must still start with:

- `app`

The pre-tuning table must still end with:

- `gameplay_movement_tuning_selected_field`

The post-tuning table must still start with:

- `window_requested`

The post-tuning table must still end with:

- `controller_action_input_action`

The existing descriptor loop must stay procedural and remain between the two
fixed row tables. Preserve:

- `for (const ProductGameplayMovementTuningFieldDescriptor& descriptor :
  kProductGameplayMovementTuningFields)`
- dynamic key construction:
  `"gameplay_movement_tuning_" + std::string{descriptor.name}`
- `// branch-gate: BG-1208`
- toggle branch threshold `>= 0.5F`
- float branch `floatReceiptValue(...)`
- field value source
  `productGameplayMovementTuningFieldValue(window.gameplay.gameplayMovement.tuning,
  descriptor.field)`

Do not leave any fixed direct procedural rows unless a compile issue forces it;
if that happens, stop and report the exact row instead of widening the design.

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

Do not table-drive the descriptor loop.
Do not add a shared receipt row helper.
Do not change receipt keys, order, values, status strings, stringifier outputs,
or formatting.
Do not split frontend/settings/window sections into multiple public appenders.
Do not perform broad include cleanup beyond what is needed to compile this file.
Do not regenerate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "FrontendSettingsWindowReceiptContext|FrontendSettingsWindowReceiptFieldRow|kFrontendSettingsWindowPreludeReceiptFields|kFrontendSettingsWindowPostTuningReceiptFields|appendProductFrontendSettingsWindowFields|appendReceiptField\\(|kProductGameplayMovementTuningFields|BG-1208" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp
```

Expected:

- the file-local context and row types exist
- the two ordered row arrays exist
- `appendProductFrontendSettingsWindowFields(...)` remains the only public
  appender
- fixed `appendReceiptField(...)` calls live inside row callbacks
- the tuning descriptor loop and `BG-1208` branch-gate remain procedural

Run:

```sh
rg -n "ProductAppReceiptContext|ReceiptFieldRow|FrontendSettingsWindowReceiptFieldRow" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.hpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/ReceiptFields.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp
```

Expected:

- no `ProductAppReceiptContext`
- no shared `ReceiptFieldRow`
- `FrontendSettingsWindowReceiptFieldRow` appears only in
  `FrontendSettingsWindowFields.cpp`

Run:

```sh
rg -c "appendReceiptField\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp
```

Expected:

- `92`

The count should remain 92 because the fixed rows move into 90 callbacks and
the descriptor loop keeps its two procedural append call sites.

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

- `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
- this task card after moving it to `done/`

## Self-Blockers

Stop and report instead of widening scope if:

- the local row tables change any receipt key, order, value, or golden output
- preserving a row requires a shared helper or appender signature change
- the tuning descriptor loop needs to be table-driven or rewritten
- `floatReceiptValue(...)`, stringifier output, ternary output, direct
  bool/string/count formatting, or overload selection changes
- compile fallout expands beyond include repairs in
  `FrontendSettingsWindowFields.cpp`
- the implementation starts to require `ProductAppReceiptContext`

## Completion Brief Checklist

Report:

- files changed
- exact context/row helper shape
- pre-tuning row count
- post-tuning row count
- whether the tuning descriptor loop stayed procedural
- first and last fixed receipt keys
- whether any fixed rows were left procedural
- required grep classifications
- focused build/CTest/direct oracle results
- receipt golden diff result
- diff/whitespace check results
- confirmation that no shared helper, `ProductAppReceiptContext`, CMake, tests,
  golden, staging, commit, push, broad CTest, or window launch was performed
