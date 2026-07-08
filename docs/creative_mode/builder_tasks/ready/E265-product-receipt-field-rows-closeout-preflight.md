# E265: Product Receipt Field Rows Closeout Preflight

## Objective

Perform a read-only closeout/preflight for the receipt field row-table lane
from E250-E264.

The goal is to determine whether `docs/refactor_targets.md` target #1
(receipt field boilerplate) is now complete, and whether a shared receipt row
helper is worth a follow-up implementation slice. Do not edit receipt source in
this card.

## Current Context

E250 preflight chose a local file-only row-table pilot instead of introducing
`ProductAppReceiptContext` or a shared helper up front.

E251-E264 then table-drove all eligible direct/mixed receipt field sections:

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
- E261: fixed rows in `FrontendSettingsWindowFields.cpp`
- E262: fixed rows in `CreativeUiFields.cpp`'s public appender
- E263: fixed creative UI command diagnostic helper rows
- E264: fixed creative UI baked-room refresh diagnostic rows

All implementation slices preserved the receipt golden:

- `receipt key-order oracle: 1032 fields match golden (order + values)`
- no `tests/golden/product_receipt_key_order.golden` diff

Known remaining intentionally procedural append sites include at least:

- `ReceiptBuilder.cpp` top-level `result` / `reason_code` fields
- `FrontendSettingsWindowFields.cpp` gameplay movement tuning descriptor loop
- `CreativeUiFields.cpp` optional `clearedActiveRoom` branch

The local row-table pattern increased per-file line counts substantially. This
closeout should quantify that tradeoff and decide whether to stop, share a
small helper, or move to the next refactor target.

## Scope

Read and classify:

- `docs/refactor_targets.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- `src/app/iggy3d/receipt/ReceiptFields.cpp`
- all `src/app/iggy3d/receipt/*Fields.cpp`
- `tests/unit/product_receipt_key_order_tests.cpp`
- `tests/golden/product_receipt_key_order.golden` only if needed for order
  context; do not edit it

Classify current receipt append sites into:

- **Table Callback Row**: fixed rows now owned by file-local row arrays.
- **Intentional Procedural**: loops, optional branches, top-level receipt
  result fields, aggregation-only wrappers, or dynamic keys.
- **Shared Helper Candidate**: repeated local row/iteration shape that could be
  centralized without changing field order or appender signatures.
- **Keep Local**: row shapes that are clearer or safer when file-local because
  they use special key indirection, typed contexts, optional branches, or
  section-specific policy.

For each `*Fields.cpp` file, record:

- current line count
- `appendReceiptField(...)` count
- row type names and row array names
- remaining procedural append sites, if any
- whether a shared helper would reduce meaningful duplication or only hide
  explicit receipt order
- whether a shared helper would require public appender signature changes

## Questions To Answer

1. Are all fields targeted by E250 now table-driven where safe?
2. Which remaining `appendReceiptField(...)` calls are intentionally
   procedural, and why?
3. Did the local row-table pattern create enough duplication to justify a
   shared helper?
4. If yes, what is the safest next implementation slice, and which one file or
   helper shape should pilot it?
5. If no, should the receipt-boilerplate target be marked complete and the
   queue move to the next `docs/refactor_targets.md` target?
6. Is `ProductAppReceiptContext` still unnecessary, or is it now justified by
   concrete appender-signature churn avoidance?
7. What focused verification is enough for the next implementation slice, if
   any?

## Helper Shapes To Evaluate

Evaluate, but do not implement, a minimal shared iterator/helper such as:

```cpp
template <typename Row, typename Context>
void appendReceiptRows(RenderReceipt& receipt,
                       const std::span<const Row> rows,
                       const Context& context);
```

or a local/common row alias such as:

```cpp
template <typename Context>
struct ProductReceiptFieldRow {
  std::string_view key;
  void (*append)(RenderReceipt& receipt,
                 const Context& context,
                 std::string_view key);
};
```

Be conservative. A shared helper is only worth recommending if it reduces real
duplication without obscuring receipt order, forcing broad signature churn, or
touching every receipt file in one implementation card.

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

Run current size and append-row inventory:

```sh
find /Users/kogaryu/iggy3d/src/app/iggy3d/receipt -maxdepth 1 -name '*Fields.cpp' -print | sort | xargs wc -l
for f in /Users/kogaryu/iggy3d/src/app/iggy3d/receipt/*Fields.cpp; do printf '%4d %s\n' "$(rg -c 'appendReceiptField\(' "$f" || true)" "${f#/Users/kogaryu/iggy3d/}"; done | sort -nr
```

Run focused receipt-shape greps:

```sh
rg -n "appendReceiptField|ReceiptFieldRow|ReceiptContext|k.*ReceiptFields|ProductAppReceiptContext|appendProduct.*Fields|clearedActiveRoom|kProductGameplayMovementTuningFields" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt \
  /Users/kogaryu/iggy3d/tests/unit/product_receipt_key_order_tests.cpp \
  --glob '*.cpp' --glob '*.hpp'
```

Run focused no-shared-helper grep:

```sh
rg -n "ProductAppReceiptContext|struct .*ReceiptFieldRow|template <typename .*ReceiptFieldRow|appendReceiptRows|std::span" \
  /Users/kogaryu/iggy3d/src/app/iggy3d/ReceiptBuilder.cpp \
  /Users/kogaryu/iggy3d/src/app/iggy3d/receipt \
  --glob '*.cpp' --glob '*.hpp'
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
- per-file classification table
- list of intentionally procedural append sites
- answer whether E250 target #1 is complete
- shared-helper recommendation: implement now, defer, or do not do
- if implementing a helper, the exact first slice and self-blockers
- if closing the target, the next recommended `docs/refactor_targets.md`
  target and why
- exact non-goals for any next implementation
- focused verification commands for any next implementation

## Candidate Follow-Up Shapes

If a shared helper is justified, draft one of these as the next card:

- `E266: Product Receipt Rows Shared Helper G1 - <chosen pilot>`
- Scope must touch only `ReceiptFields.hpp/.cpp` if needed, one chosen
  `*Fields.cpp` pilot, and the task card.
- It must preserve appender signatures and receipt golden output.

If no helper is justified, recommend closing the receipt-boilerplate target and
promoting the next largest current `docs/refactor_targets.md` target.
