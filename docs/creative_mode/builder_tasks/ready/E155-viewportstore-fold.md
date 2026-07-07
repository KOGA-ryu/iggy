# E155 — ViewportStore fold (god-struct decomposition #6) — PARENT

**STATUS: READY — claim next.**
Recon-grounded + spot-verified (workflow `wkqdxuj2u`, 2026-07-07). **Commit convention:** `claude: planned. codex: …`.

> **Tree is mid-flight** — RoomStore (E148–E152) and SaveSessionStore (E153) just landed; the god-struct is
> being reshaped. **Anchor by field NAME and re-verify line numbers at slice time.**

---

## Goal

Fold the **11 flat `mapMaker*` fields** off the god-struct **into the existing `ProductViewportState`**, so
the god-struct keeps its single `viewport` member and loses 11 top-level fields. Camera + map-maker grid;
**plain owned state, no derived truth, no freshness token.**

## Design decision — fold (B), do NOT wrap (A)

**Recommended: append the 11 `mapMaker*` fields to `ProductViewportState`** (`src/app/iggy3d/view/ViewportState.hpp`),
keep the god-struct's `ProductViewportState viewport;` member as-is. Repoint only `window.mapMakerX →
window.viewport.mapMakerX` (~92 refs).
- **Why not a `viewportStore` wrapper:** that would force repointing all **~388 `window.viewport.X` readers**
  to `window.viewportStore.viewport.X` across ~30 files — enormous churn, zero benefit.
- **Fold is collision-free (verified):** `ProductViewportState` has no `mapMakerStatus`/`mapMakerGrid*`
  members today (it has render-count `productDraw*`/`productRenderBridge*` names, which don't clash).
- The creativeFly anchor (#3) is already a self-contained nested `viewport.creativeFlyAnchor` — it rides
  along untouched.

## The 11 fields to fold

```
mapMakerStatus mapMakerReasonCode mapMakerGridVisible mapMakerGridStatus mapMakerGridReasonCode
mapMakerGridPitchMeters mapMakerGridMajorStepMeters mapMakerGridPlaneY mapMakerGridLayerCount
mapMakerGridDotCount mapMakerGridMajorDotCount
```

## LAW

1. **Receipt golden byte-identical.** `mapMaker*` is emitted by `receipt/FrontendSettingsWindowFields.cpp`
   (~L125-146) via string keys (`map_maker_status`, `map_maker_grid_pitch_meters`, …). Repointing the read
   path to `window.viewport.mapMakerX` changes no key/value/order. **A golden diff = a BUG — STOP.**
2. **Compiler-guided, NEVER `replace_all`** (see HAZARDS). Delete the flat fields, add them to
   `ProductViewportState`, fix each compiler error.

## Method

1. Append the 11 members (verbatim names/types/defaults) to `ProductViewportState` in `ViewportState.hpp`.
2. Delete the 11 flat `mapMaker*` declarations from `ProductAppWindowState.hpp`.
3. `cmake --build build -j8`; repoint each `no member named` error `<obj>.mapMakerX → <obj>.viewport.mapMakerX`.
4. `ctest -j8` → 260/260, then TSV/docs.

## TSV edit (coverage gate is bidirectional, top-level only)

**Delete the 11 `mapMaker*␉ViewportStore` rows** (they leave the top level). **KEEP the `viewport␉ViewportStore`
row** — `viewport` is still a top-level member and already carries its row. **Net: 12 rows → 1; NO new row added**
(differs from the usual "delete flat + add member row" because the destination member already exists).

## HAZARDS (real, verified — all handled by the compiler method)

- **Mis-named test window vars** (a `window.` replace misses them): `creativeWorldWindow`, `inactiveWindow`,
  `blockedWindow` (`product_window_input_frame_tests.cpp`); `legacy`, `document` (`product_frontend_router_tests.cpp`).
  Their `.mapMakerX` reads repoint too. Production is bare `window` (uniform).
- **Foreign `.viewport.` / `.mapMaker*` collisions — do NOT touch:** `frame.viewport`, `request.viewport`,
  and `projection.drawList`/`projection.renderBridge` carry their own `mapMaker*`/`viewport` names
  (`product_vulkan_room_frame_tests.cpp` has ~20 such foreign refs). Under the fold method the compiler only
  errors on the god-struct's `window.mapMakerX`, so these are auto-safe — but never bare-token sed.

## Scope

~92 mapMaker* repoints. Dominant: `receipt/FrontendSettingsWindowFields.cpp` (11), the menu/input flow that
writes map-maker state, and the test files above. `window.viewport.X` (388 refs) is **untouched**.

## GATES

Build green · ctest 260/260 · golden byte-identical · TSV as above · update decomposition map #6 → DONE + PRIORITY.md.

## Why safe

Fold keeps the 388 viewport readers identical; only 92 mapMaker* refs move; compiler finds every one
(incl. the mis-named test vars); golden + coverage gate prove behavior + accounting. Smallest of the
remaining structural moves.

## Suggested slices (for the slicer-Codex)

This is small enough to be **one gate**, but if slicing:
- **G1** — fold the 11 members into `ProductViewportState`, delete the flat fields, repoint all **src** readers; build green.
- **G2** — migrate the **test** readers (incl. the 5 mis-named vars); ctest 260/260.
- **G3** — TSV edit + map/PRIORITY docs; golden confirmed unchanged.
(G1+G2 can reasonably merge — the compiler forces both to land together to compile.)
