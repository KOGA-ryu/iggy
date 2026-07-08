# E250: Product Receipt Fields Table-Drive Preflight

## Objective

Perform a read-only preflight for the XL receipt-field boilerplate target from
`docs/refactor_targets.md`.

The goal is to decide whether the receipt `*Fields.cpp` family is ready for a
small table-driven implementation slice, and if so, define the safest first
slice. Do not edit receipt code in this card.

## Current Context

The controller split target is complete after E249:

- `src/app/iggy3d/gameplay/Controller.cpp` is now a 21-line public facade.
- The gameplay controller implementation is split behind controller-owned
  sibling helpers.

The next largest backlog target is receipt field boilerplate:

- `docs/refactor_targets.md` target #1:
  table-drive the receipt `*Fields.cpp` boilerplate family.
- Current snapshot from the card release:
  - 14 `src/app/iggy3d/receipt/*Fields.cpp` files.
  - 2429 total lines across those files.
  - Current largest files:
    - `CreativeUiFields.cpp`: 464 lines
    - `GameplaySceneStateFields.cpp`: 386 lines
    - `SaveStateFields.cpp`: 333 lines
  - Current hottest `appendReceiptField(...)` counts:
    - `GameplaySceneStateFields.cpp`: 178
    - `CreativeUiFields.cpp`: 159
    - `SaveStateFields.cpp`: 146
    - `GameplayRuntimeMovementFields.cpp`: 116
    - `FrontendSettingsWindowFields.cpp`: 92
    - `WorldAuthoringFields.cpp`: 91

The key safety constraint is receipt order/value stability:

- `tests/unit/product_receipt_key_order_tests.cpp`
- `tests/golden/product_receipt_key_order.golden`

Any later implementation slice must leave the golden byte-identical unless a
separate owner explicitly approves a receipt change.

## Scope

Read and classify:

- `docs/refactor_targets.md`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- `src/app/iggy3d/receipt/ReceiptFields.cpp`
- all `src/app/iggy3d/receipt/*Fields.cpp`
- `tests/unit/product_receipt_key_order_tests.cpp`
- `tests/golden/product_receipt_key_order.golden` only if needed for order
  context; do not edit it.

Classify each receipt field file into buckets:

- **Direct Table Candidate**: mostly literal key plus direct value/accessor.
- **Mixed Table Candidate**: tableable rows plus local derived helper logic.
- **Keep Procedural For Now**: heavy conditionals, loops, dynamic nested policy,
  or fields whose value construction would become opaque in a table.
- **Not A Field Boilerplate Target**: helper files with no append rows, or
  aggregation-only files.

For the top three candidates, record:

- line count
- `appendReceiptField(...)` count
- helper/function inventory
- whether values are direct window paths, derived locals, stringified enums,
  `floatReceiptValue(...)`, counted ranges, or conditional strings
- whether a table would need captured context beyond `ProductAppWindowState`
- whether field order is simple enough for a `std::array` row table

## Helper Shape To Evaluate

Evaluate, but do not implement, a minimal helper pattern like:

```cpp
struct ProductReceiptFieldRow {
  std::string_view key;
  std::string_view (*value)(const ProductReceiptFieldContext& context);
};
```

or, if string ownership makes that wrong:

```cpp
struct ProductReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt, const ProductReceiptFieldContext& context);
};
```

The preflight should decide whether the first implementation needs:

- a local file-only row type,
- a shared receipt row helper,
- or `ProductAppReceiptContext` first from `docs/refactor_targets.md` target #6.

Be conservative. If a shared helper would force broad signature churn across
many receipt appenders, recommend a one-file local table pilot first.

## Questions To Answer

1. Is `GameplaySceneStateFields.cpp` still the best first pilot, or does current
   `CreativeUiFields.cpp` / `SaveStateFields.cpp` offer a safer first table?
2. Should `ProductAppReceiptContext` be implemented before table-driving any
   field file, or can one receipt file be piloted with current function inputs?
3. What exact row shape is safest for preserving field order, value formatting,
   and local helper ownership?
4. Which fields in the chosen pilot must remain procedural?
5. What focused tests are enough for the first implementation slice?

## Non-Goals

Do not edit:

- receipt source/header files
- `ReceiptBuilder.cpp`
- tests
- receipt golden
- CMake
- fixture files
- production docs other than this task card if the builder workflow appends to
  it

Do not generate the golden.
Do not run broad CTest.
Do not launch a window.
Do not stage, commit, or push.

## Required Commands

Run current-size and append-row inventory:

```sh
find /Users/kogaryu/iggy3d/src/app/iggy3d/receipt -maxdepth 1 -name '*Fields.cpp' -print | sort | xargs wc -l
for f in /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/*Fields.cpp; do printf '%4d %s\n' "$(rg -c 'appendReceiptField\(' "$f" || true)" "${f#/Users/kogaryu/iggy3d/}"; done | sort -nr
```

Run focused receipt-shape greps:

```sh
rg -n "appendReceiptField|floatReceiptValue|appendProduct.*Fields|buildProductAppReceipt|ProductAppReceiptContext|product_receipt_key_order" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt \
  /Users/kogaryu/iggy3d/tests/unit/product_receipt_key_order_tests.cpp \
  --glob '*.cpp' --glob '*.hpp'
```

Run focused reads for the likely pilot candidates:

```sh
sed -n '1,430p' /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/GameplaySceneStateFields.cpp
sed -n '1,520p' /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/CreativeUiFields.cpp
sed -n '1,380p' /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/SaveStateFields.cpp
```

Run:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over this done card after moving it.

## Expected Deliverable

Move this card to `done/` and append a completion brief with:

- files inspected
- current line counts and append-row counts
- per-file classification buckets
- chosen first implementation slice, or a clear reason to implement
  `ProductAppReceiptContext` first
- proposed helper row shape
- exact files and fields in scope for the first implementation
- exact non-goals for the first implementation
- focused verification commands for the first implementation
- any self-blockers

## Candidate Follow-Up Shape

If the preflight confirms a one-file pilot is safe, draft the next card as:

`E251: Product Receipt Field Rows G1 - <Chosen Section>`

Expected implementation constraints for that future card:

- table-drive only the chosen section
- preserve public appender signature
- preserve field order exactly
- preserve `floatReceiptValue(...)` formatting and boolean/string formatting
- run `product_receipt_key_order_tests`
- confirm `tests/golden/product_receipt_key_order.golden` has no diff

## Completion Brief

Status: Done.

### Files inspected

- `docs/refactor_targets.md`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- `src/app/iggy3d/receipt/ReceiptFields.cpp`
- all 14 `src/app/iggy3d/receipt/*Fields.cpp` files
- `tests/unit/product_receipt_key_order_tests.cpp`
- `tests/golden/product_receipt_key_order.golden` for the chosen pilot's
  first/last order context only

No receipt source/header, tests, CMake, fixture, or golden files were edited.
No golden was generated. No build, CTest, staging, commit, push, broad CTest, or
window launch was performed.

### Current inventory

Line counts:

| File | Lines | append rows |
| --- | ---: | ---: |
| `ActiveRoomFields.cpp` | 79 | 34 |
| `CreativePickWireframeFields.cpp` | 136 | 52 |
| `CreativeUiFields.cpp` | 464 | 159 |
| `DebugHudFields.cpp` | 87 | 27 |
| `FeedbackSurfaceAutomationVulkanFields.cpp` | 133 | 54 |
| `FrontendSettingsWindowFields.cpp` | 213 | 92 |
| `GameplayRuntimeMovementFields.cpp` | 240 | 116 |
| `GameplaySceneStateFields.cpp` | 386 | 178 |
| `ReceiptFields.cpp` | 18 | 0 |
| `SaveStateFields.cpp` | 333 | 146 |
| `StartupProbeFields.cpp` | 84 | 38 |
| `StartupWorldBuildoutFields.cpp` | 11 | 0 |
| `TailFields.cpp` | 41 | 12 |
| `WorldAuthoringFields.cpp` | 204 | 91 |
| Total | 2429 | 1105 |

Append-row hottest files are unchanged from release context:
`GameplaySceneStateFields.cpp` 178, `CreativeUiFields.cpp` 159,
`SaveStateFields.cpp` 146, `GameplayRuntimeMovementFields.cpp` 116,
`FrontendSettingsWindowFields.cpp` 92, and `WorldAuthoringFields.cpp` 91.

### Classification buckets

| File | Bucket | Rationale |
| --- | --- | --- |
| `GameplaySceneStateFields.cpp` | Direct Table Candidate | One appender, no loops or conditionals, all rows use `ProductAppWindowState`; values are direct bool/string/count fields plus `std::to_string`, `static_cast<std::uint64_t>`, `floatReceiptValue(...)`, and one provenance stringifier. |
| `GameplayRuntimeMovementFields.cpp` | Direct Table Candidate | One direct appender with no loops/conditionals; values are from `window`, `ProductMovementProofPacket`, and `runtimeStateHash`, with heavy `floatReceiptValue(...)` use. Good later candidate after the single-context pilot. |
| `WorldAuthoringFields.cpp` | Direct Table Candidate | One direct appender, no loops/conditionals; 91 window-backed rows. |
| `ActiveRoomFields.cpp` | Direct Table Candidate | Small direct window-backed appender, no loops/conditionals. |
| `StartupProbeFields.cpp` | Direct Table Candidate | Small direct appender using `frontend`, `window`, and `saves`; no loops/conditionals. |
| `DebugHudFields.cpp` | Direct Table Candidate | Direct appender over `window` plus derived HUD packets; no loops/conditionals. |
| `FeedbackSurfaceAutomationVulkanFields.cpp` | Direct Table Candidate | Direct appender over `window`, `feedback`, active surface, and Vulkan readiness; no loops/conditionals. |
| `CreativePickWireframeFields.cpp` | Direct Table Candidate | Direct appender over `window` plus Vulkan readiness; no loops/conditionals. |
| `TailFields.cpp` | Direct Table Candidate | Small direct tail section over `options`, `world`, `window`, and `saves`. |
| `CreativeUiFields.cpp` | Mixed Table Candidate | Many tableable rows, but already has file-local command sub-appenders and a baked-room diagnostic keyset helper with a conditional optional key. Keep that helper procedural in any later slice. |
| `SaveStateFields.cpp` | Mixed Table Candidate | No loops/conditionals and many direct rows, but it requires `frontend`, `window`, and `creativeIdentity`; values include save state, room editor values, frontend save-browser stringification, tool/direction stringifiers, `std::to_string`, and `floatReceiptValue(...)`. Good later candidate after context decisions. |
| `FrontendSettingsWindowFields.cpp` | Mixed Table Candidate | Direct rows plus settings/tuning loop and local derived values. Tableable by section, not ideal as first pilot. |
| `ReceiptFields.cpp` | Not A Field Boilerplate Target | Shared `floatReceiptValue(...)` helper only; no append rows. |
| `StartupWorldBuildoutFields.cpp` | Not A Field Boilerplate Target | Aggregation-only appender for startup probe, world authoring, and active-room sections; no direct append rows. |

No current `*Fields.cpp` file with append rows needs to be classified as
"keep procedural for now" wholesale. Some sections inside mixed candidates
should stay procedural until a later owner decision.

### Top-three candidate notes

`GameplaySceneStateFields.cpp`: 386 lines, 178 append rows, one function,
zero local helper functions, zero loops, zero conditionals. Its values are all
available from `ProductAppWindowState` and use only existing formatting helpers
or direct value overloads. The golden currently places its run from
`position_hud_visible` through `product_vulkan_room_geometry_signature`, which
matches this appender's first and last fields.

`CreativeUiFields.cpp`: 464 lines, 159 append rows, ten helper/appender
functions plus `ProductCreativeBakedRoomRefreshReceiptKeySet`. It is partly
already sectioned. The direct main appender rows are tableable, but the command
sub-appenders and baked-room diagnostic helper should not be collapsed in a
first pilot because one diagnostic path conditionally omits
`clearedActiveRoom`.

`SaveStateFields.cpp`: 333 lines, 146 append rows, one function, zero loops,
zero conditionals. It is mechanically tableable, but it needs a wider context
than `ProductAppWindowState`: `FrontendState`, `ProductAppWindowState`, and
`creative::CreativeActiveIdentity`. This makes it a worse first pilot than
`GameplaySceneStateFields.cpp`.

### Decision answers

1. `GameplaySceneStateFields.cpp` is still the best first pilot. It is the
   hottest row count and the cleanest shape: no loops, no conditionals, no
   local helper ownership, and no context beyond `ProductAppWindowState`.
2. `ProductAppReceiptContext` should not be implemented first. It does not
   exist today, and adding it would churn all appender signatures before the
   table row pattern is proven. Start with a one-file local row table using the
   current `appendProductGameplaySceneStateFields(...)` signature.
3. A `std::string_view value(...)` row is unsafe because many fields construct
   temporary `std::string` values through `std::to_string(...)` and
   `floatReceiptValue(...)`. The safest row shape is a local append-callback
   row that keeps key order explicit and delegates ownership/formatting to
   `appendReceiptField(...)`.
4. In the chosen pilot, no fields need to remain procedural. The whole
   `appendProductGameplaySceneStateFields(...)` run can be expressed as ordered
   row callbacks. If implementation shows a compile issue, keep the provenance
   stringifier row procedural rather than changing receipt value formatting.
5. Focused tests for the first slice: build `iggy3d` and
   `product_receipt_key_order_tests`, run only
   `product_receipt_key_order_tests`, confirm the golden has no diff, then run
   `git diff --check` and a focused whitespace scan.

### Proposed helper row shape

Use a file-local helper inside `GameplaySceneStateFields.cpp`; do not add a
shared receipt row helper yet.

```cpp
namespace {

struct GameplaySceneStateReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const ProductAppWindowState& window,
                 std::string_view key);
};

constexpr std::array<GameplaySceneStateReceiptFieldRow, 178>
    kGameplaySceneStateReceiptFields{{
        {"position_hud_visible",
         [](RenderReceipt& receipt,
            const ProductAppWindowState& window,
            std::string_view key) {
           appendReceiptField(receipt, key,
                              window.debugHud.positionHud.visible);
         }},
        // ...ordered rows...
    }};

}  // namespace
```

The public appender should keep the current signature and iterate the rows in
array order. Each callback should call `appendReceiptField(...)` directly,
preserving existing bool/string/integer overloads, `std::to_string(...)`,
`static_cast<std::uint64_t>`, and `floatReceiptValue(...)` behavior.

### Recommended next card

Title: `E251: Product Receipt Field Rows G1 - Gameplay Scene State Fields`

Implementation scope:

- Edit only `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp` and the task
  card.
- Add a file-local row type and ordered row array for all fields currently
  appended by `appendProductGameplaySceneStateFields(...)`.
- Preserve `appendProductGameplaySceneStateFields(RenderReceipt&, const
  ProductAppWindowState&)` as the only externally visible API.
- Preserve field order from `position_hud_visible` through
  `product_vulkan_room_geometry_signature` exactly.

Non-goals:

- No changes to `ReceiptFields.hpp/.cpp`, `ReceiptBuilder.cpp`, other
  `*Fields.cpp` files, `ProductAppReceiptContext`, CMake, tests, fixtures, or
  `tests/golden/product_receipt_key_order.golden`.
- No shared helper until one-file local table mechanics are proven.
- No receipt key, order, value, formatter, status string, or golden changes.

Focused verification commands:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_receipt_key_order_tests$' --output-on-failure
/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Self-blockers for E251:

- Stop if the local row array changes any receipt key/order/value or golden
  output.
- Stop if the row table requires a shared context/helper or signature churn.
- Stop if `floatReceiptValue(...)`, `std::to_string(...)`, bool formatting, or
  the provenance stringifier output changes.
- Stop if compile fallout expands beyond
  `GameplaySceneStateFields.cpp` include repairs.

### Commands run

- Current-size and append-row inventory commands from this card.
- Focused receipt-shape grep over `ReceiptBuilder.cpp`, `receipt/*`, and
  `product_receipt_key_order_tests.cpp`.
- Focused reads of `GameplaySceneStateFields.cpp`, `CreativeUiFields.cpp`,
  `SaveStateFields.cpp`, `ReceiptBuilder.cpp`, `ReceiptFields.hpp/.cpp`,
  `docs/refactor_targets.md`, and the receipt key-order test.
- Focused golden grep for the chosen pilot's first/last keys.
- `git -C /Users/kogaryu/iggy3d diff --check` was clean after the card was
  moved to `done/` and this brief was appended.
