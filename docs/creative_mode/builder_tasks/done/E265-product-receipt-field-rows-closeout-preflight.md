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

## Completion Brief

Status: Done.

Files inspected:

- `docs/refactor_targets.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `src/app/iggy3d/receipt/ReceiptFields.hpp`
- `src/app/iggy3d/receipt/ReceiptFields.cpp`
- `src/app/iggy3d/receipt/*Fields.cpp`
- `tests/unit/product_receipt_key_order_tests.cpp`
- `tests/golden/product_receipt_key_order.golden`

Current inventory:

| File | Lines | `appendReceiptField(...)` calls |
| --- | ---: | ---: |
| `ActiveRoomFields.cpp` | 253 | 34 |
| `CreativePickWireframeFields.cpp` | 372 | 52 |
| `CreativeUiFields.cpp` | 1666 | 159 |
| `DebugHudFields.cpp` | 234 | 27 |
| `FeedbackSurfaceAutomationVulkanFields.cpp` | 387 | 54 |
| `FrontendSettingsWindowFields.cpp` | 914 | 92 |
| `GameplayRuntimeMovementFields.cpp` | 752 | 116 |
| `GameplaySceneStateFields.cpp` | 1117 | 178 |
| `ReceiptFields.cpp` | 18 | 0 |
| `SaveStateFields.cpp` | 1381 | 146 |
| `StartupProbeFields.cpp` | 272 | 38 |
| `StartupWorldBuildoutFields.cpp` | 11 | 0 |
| `TailFields.cpp` | 149 | 12 |
| `WorldAuthoringFields.cpp` | 577 | 91 |

Totals:

- `src/app/iggy3d/receipt/*Fields.cpp`: 8103 lines.
- `src/app/iggy3d/receipt/*Fields.cpp`: 999 `appendReceiptField(...)`
  call sites.
- `src/app/iggy3d/ReceiptBuilder.cpp`: 2 additional top-level
  `appendReceiptField(...)` call sites for `result` and `reason_code`.
- `tests/golden/product_receipt_key_order.golden`: 1032 ordered fields.

Per-file classification:

| File | Classification | Row types and row arrays | Remaining procedural sites | Shared-helper judgement |
| --- | --- | --- | --- | --- |
| `ActiveRoomFields.cpp` | Table Callback Row | `ActiveRoomReceiptContext`, `ActiveRoomReceiptFieldRow`, `kActiveRoomReceiptFields` | None | Candidate shape, but local context keeps room/collision/freshness explicit. |
| `CreativePickWireframeFields.cpp` | Table Callback Row | `CreativePickWireframeReceiptContext`, `CreativePickWireframeReceiptFieldRow`, `kCreativePickWireframeReceiptFields` | None | Candidate shape, but no signature churn saved. |
| `CreativeUiFields.cpp` | Table Callback Row plus Intentional Procedural | `CreativeUiCommandReceiptFieldRow`, `ProductCreativeBakedRoomRefreshReceiptKeySet`, `ProductCreativeBakedRoomRefreshReceiptFieldRow`, `CreativeUiReceiptContext`, `CreativeUiReceiptFieldRow`; arrays `kCreativeUiCommandMutationReceiptFields`, `kCreativeUiCommandCreateReceiptFields`, `kCreativeUiCommandDeleteReceiptFields`, `kCreativeUiCommandUndoReceiptFields`, `kCreativeUiCommandRoomShellReceiptFields`, `kCreativeUiCommandReceiptFields`, `kProductCreativeBakedRoomRefreshPreOptionalReceiptFields`, `kProductCreativeBakedRoomRefreshPostOptionalReceiptFields`, `kCreativeUiPreCommandReceiptFields`, `kCreativeUiPostRefreshReceiptFields` | Optional `clearedActiveRoom` append branch | Keep local because command diagnostics and baked-room key indirection are section-specific. |
| `DebugHudFields.cpp` | Table Callback Row | `DebugHudReceiptContext`, `DebugHudReceiptFieldRow`, `kDebugHudReceiptFields` | None | Candidate shape, but little payoff alone. |
| `FeedbackSurfaceAutomationVulkanFields.cpp` | Table Callback Row | `FeedbackSurfaceAutomationVulkanReceiptContext`, `FeedbackSurfaceAutomationVulkanReceiptFieldRow`, `kFeedbackSurfaceAutomationVulkanReceiptFields` | None | Candidate shape; context has several derived inputs, so keep local for clarity. |
| `FrontendSettingsWindowFields.cpp` | Table Callback Row plus Intentional Procedural | `FrontendSettingsWindowReceiptContext`, `FrontendSettingsWindowReceiptFieldRow`, `kFrontendSettingsWindowPreludeReceiptFields`, `kFrontendSettingsWindowPostTuningReceiptFields` | `kProductGameplayMovementTuningFields` descriptor loop with dynamic keys and `BG-1208` toggle/float branch | Keep local; loop should remain procedural. |
| `GameplayRuntimeMovementFields.cpp` | Table Callback Row | `GameplayRuntimeMovementReceiptContext`, `GameplayRuntimeMovementReceiptFieldRow`, `kGameplayRuntimeMovementReceiptFields` | None | Candidate shape; current context avoids appender signature churn. |
| `GameplaySceneStateFields.cpp` | Table Callback Row | `GameplaySceneStateReceiptFieldRow`, `kGameplaySceneStateReceiptFields` | None | Simplest possible shared-helper pilot if one is ever forced. |
| `ReceiptFields.cpp` | Not A Field Boilerplate Target | None | None | Shared `floatReceiptValue(...)` helper only. |
| `SaveStateFields.cpp` | Table Callback Row | `SaveStateReceiptContext`, `SaveStateReceiptFieldRow`, `kSaveStateReceiptFields` | None | Candidate shape; wider context should stay local. |
| `StartupProbeFields.cpp` | Table Callback Row | `StartupProbeReceiptContext`, `StartupProbeReceiptFieldRow`, `kStartupProbeReceiptFields` | None | Candidate shape, but small file. |
| `StartupWorldBuildoutFields.cpp` | Not A Field Boilerplate Target | None | Aggregation-only wrapper calls startup probe, world authoring, and active room appenders | Keep procedural aggregation. |
| `TailFields.cpp` | Table Callback Row | `TailReceiptContext`, `TailReceiptFieldRow`, `kTailReceiptFields` | None | Candidate shape, but too small to justify helper work. |
| `WorldAuthoringFields.cpp` | Table Callback Row | `WorldAuthoringReceiptFieldRow`, `kWorldAuthoringReceiptFields` | None | Simple direct candidate if a helper is ever piloted. |

Intentional procedural append sites:

- `ReceiptBuilder.cpp`: final top-level `result` and `reason_code` fields.
  These are receipt-wide result policy, not section boilerplate.
- `FrontendSettingsWindowFields.cpp`: the
  `kProductGameplayMovementTuningFields` descriptor loop. It constructs dynamic
  keys, preserves `BG-1208`, and branches between toggle bool formatting and
  `floatReceiptValue(...)`.
- `CreativeUiFields.cpp`: the optional `clearedActiveRoom` branch in
  `appendProductCreativeBakedRoomRefreshDiagnosticFields(...)`. It is keyed by
  whether the wrapper key-set supplies an empty key or a real key.
- `StartupWorldBuildoutFields.cpp`: aggregation-only wrapper. It has no
  `appendReceiptField(...)` calls.

Answers:

1. All E250-targeted receipt field rows are table-driven where safe. The
   remaining procedural sites are dynamic, optional, top-level policy, or
   aggregation-only.
2. The remaining direct append sites should stay procedural for the reasons
   above.
3. The local row-table pattern did create duplication: many files repeat a
   `{key, append}` row type and a trivial iteration loop. However, the line
   growth is dominated by explicit per-row callback bodies, which a shared
   helper would not remove without changing the design and obscuring receipt
   order.
4. A shared helper is not worth an implementation slice now. It would save
   small local struct/loop boilerplate while touching many receipt files and
   increasing abstraction in a golden-sensitive path.
5. `docs/refactor_targets.md` target #1 should be marked complete. The next
   recommended current backlog target is #3, `Extract iggy3d_creative main()`,
   because target #2 `Controller.cpp` split is already complete and #3 is the
   next largest unclosed target in the ranked backlog.
6. `ProductAppReceiptContext` is still unnecessary for target #1. It may still
   be valuable as separate target #6 for long appender signatures and derived
   receipt inputs, but the row-table lane did not require it.
7. Focused verification for any future receipt helper remains the same:
   targeted `iggy3d` plus `product_receipt_key_order_tests` build, focused
   CTest regex for `product_receipt_key_order_tests`, direct oracle from the
   repo root, golden diff, `git diff --check`, and a touched-file whitespace
   scan.

Shared-helper recommendation:

- Recommendation: do not implement a shared receipt row helper now.
- Rationale: the current local tables are explicit and golden-stable. A helper
  would reduce minor repetition but not the large callback bodies. It would also
  create a new shared abstraction that every future receipt edit must reason
  about.
- If an owner still wants a helper later, the safest pilot would be a single
  direct file such as `GameplaySceneStateFields.cpp` or
  `WorldAuthoringFields.cpp`, with a header-local or file-local helper that
  preserves the public appender signature and local row order. Self-blockers
  for that pilot should be: any golden diff, any public appender signature
  change, any need for `ProductAppReceiptContext`, any broad migration across
  multiple receipt files, or any loss of visible receipt order in the table.

Recommended next implementation/preflight:

- Close the receipt-boilerplate target and promote a new preflight for
  `docs/refactor_targets.md` target #3,
  `Extract iggy3d_creative main()`.
- Suggested first card: read-only preflight only. Inspect
  `apps/iggy3d_creative/main.cpp`, identify frame-stage seams, locate the
  headless `--capture` verification path, and propose the smallest extraction
  that does not change editor behavior.
- Non-goals for that next preflight: do not edit `main.cpp`, do not introduce
  `CreativeEditorState`, do not split frame stages yet, do not touch receipt
  fields, do not launch a window, do not run broad CTest, and do not stage,
  commit, or push.
- Focused verification for that future implementation lane should be chosen by
  the preflight, but likely starts with a targeted build of the standalone
  creative executable and its existing headless/capture tests, not the receipt
  oracle unless receipt output is touched.

Required command results:

- Current size inventory command completed.
- Append-row inventory command completed.
- Focused receipt-shape grep completed. Output is large; classification above
  records the relevant symbols and procedural sites.
- Focused no-shared-helper grep completed. It found only local row type
  declarations in receipt field files; no `ProductAppReceiptContext`,
  `appendReceiptRows`, or `std::span` shared helper exists.
- `git -C /Users/kogaryu/iggy3d diff --check`: passed.

Scope confirmation:

- No receipt source/header files, `ReceiptBuilder.cpp`, tests, golden files,
  CMake, fixtures, or production docs outside this task card were edited.
- No golden generation, broad CTest, window launch, staging, commit, or push
  was performed.
