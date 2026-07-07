# E124: ProductPrimitiveDrawKind Metadata Audit

## Objective

**Read-only, plus one optional guard-test.** Confirm the per-kind
color/size/layer drift across the render draw-kind switches, produce the exact
per-kind diff, and specify the `constexpr` metadata table that will replace them.
Then draft the implementation card. No behavior change in this card (the guard
test, if added, only pins current values).

## Why This Exists

From `docs/complexity_audit_v0_1.md` (finding #5 / bucket 4). `ProductPrimitiveDrawKind`
(~22 values) has its per-kind presentation data (color / marker size / layer)
exhaustively switched in **three** render files, and the copies have **drifted**:

- `view/PrimitiveDrawList.cpp`, `view/OpeningMenuView.cpp`, `view/RenderBridge.cpp`.
- Confirmed drift: `ElevatedFloorTile` color is `{92,126,102}` in one site and
  `{137,168,143}` in another.

Adding a draw kind today means editing 3 files and risks another silent drift.

## Required Work

1. **Enumerate every switch over `ProductPrimitiveDrawKind`** across the three
   files (and any others — verify with a grep). For each site, record what
   per-kind data it encodes: base color, marker/point size, layer/z-order,
   counting, or genuine render dispatch.
2. **Produce the drift table.** For all ~22 kinds, list the value each site uses
   for each attribute and mark every disagreement (start from the confirmed
   `ElevatedFloorTile` color). This is the core deliverable.
3. **Classify each switch:** data-carrying (→ becomes a table lookup) vs genuine
   render-dispatch/control-flow (→ stays a switch). Do not table-ify real
   dispatch.
4. **Specify the table.** A `constexpr` metadata array/table keyed by
   `ProductPrimitiveDrawKind` (base color / markerSize / layer), where it lives,
   and how each data-carrying site becomes a lookup. Model on the credited
   `ProductCreativeUiCommandKind` metadata pattern.
5. **(Optional, recommended) add one guard-test** that pins the current per-kind
   color/size/layer for all kinds at their *primary* site, so the implementation
   card can prove behavior-preservation and the drift resolution is explicit.
   This test must only pin existing values — it changes no render behavior.
6. **Draft the implementation card** (`E12x`) with the exact edits AND the
   resolved value for each drifted kind. **Do not pick the "correct" drifted
   value yourself** — flag each drift for the render owner to resolve; the
   implementation card records their decision.

## Acceptance Notes

Deliverable: the per-kind drift table + switch classification + the table shape
spec + the drafted implementation card. If the guard-test is added, it is green
and pins current values only. No render behavior changes.

## Do Not

- Do not change any render output or resolve a drift value yourself (flag it).
- Do not table-ify genuine render-dispatch switches.
- Do not create the metadata table in this card — only specify it.
- Do not stage, commit, or push.

## Suggested Verification

```sh
grep -rn 'ProductPrimitiveDrawKind' --include='*.cpp' /Users/kogaryu/iggy3d/src/app/iggy3d/view
# if a guard-test is added:
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_primitive_draw_list_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Switch sites (file → attributes encoded → dispatch-or-data):
- Per-kind drift table (all disagreements):
- Proposed metadata table shape + home:
- Guard-test added? (target + what it pins):
- Drafted implementation card + flagged drift decisions:
- Concerns/deferred:
