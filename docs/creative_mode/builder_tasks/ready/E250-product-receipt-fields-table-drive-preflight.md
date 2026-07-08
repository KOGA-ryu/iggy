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
