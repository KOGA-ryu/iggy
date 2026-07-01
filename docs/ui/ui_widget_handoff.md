# UI Widget Layer — Session Handoff

> Cold-start handoff for the next session (targeting **Sonnet 5**). You do NOT need
> the prior conversation. Read this + [`ui_architecture.md`](ui_architecture.md) and
> you have everything. State as of 2026-07-01, commit `6bb6025` on `iggy3d-main`
> (pushed to origin). **Suite is 172/172 green — keep it that way; the full suite is
> the gate on every change.**

## The one-paragraph context

iggy3d's in-game UI is being rebuilt from **reusable widget primitives** instead of
bespoke per-screen emitters (the full design + the "why" is in
[`ui_architecture.md`](ui_architecture.md)). The bottom layer (L0: `ProductUiPrimitive`
draw list) already existed. We added the **L1 widget spine** and are migrating screens
onto it one at a time, each migration **output-preserving** (byte-identical draw-list
receipt) — the same discipline as an output-preserving refactor. Two screens are done.

## What exists now (files)

- **`src/app/iggy3d/ui/Widget.hpp` / `.cpp`** — the L1 spine:
  - `UiText`, `UiPanel` — the two built bricks (pure-value widgets).
  - `WidgetOutput { primitives, hitRegions }` — what a widget emits.
  - `emit(const UiText&, WidgetOutput&)` / `emit(const UiPanel&, WidgetOutput&)` — pure emit functions. An interactive widget (action != None) also emits a `UiHitRegion` from the *same rect* as its draw primitive.
  - `appendWidgetOutput(list, out)` — splats a `WidgetOutput` into a `ProductUiDrawList`, preserving `textCount`/`rectCount` bookkeeping AND threading `hitRegions` onto `list.hitRegions`.
- **`src/app/iggy3d/menu/DrawList.hpp`** — L0. Now also defines `UiHitKind` / `UiHitRegion`, and `ProductUiDrawList` carries a `hitRegions` lane + `hitRegionCount`.
- **`src/app/iggy3d/menu/DrawList.cpp`** — the screen emitters. `emitDeleteConfirmContent` and `emitLoadSaveContent` (+ `emitLoadSaveAction`) are **rebuilt through the widgets** (use them as the reference pattern). The other emitters (`emitNewWorldContent`, `emitSettingsContent`, `emitDevToolsContent`, `emitStarterFrame`, `emitStarterRows`) are **still bespoke** — they call the old `emitText`/`emitRect` helpers directly. These are the next murder targets.
- **Tests**: `tests/unit/product_ui_widget_tests.cpp` (widget unit tests) and `tests/unit/product_ui_draw_list_tests.cpp` (the draw-list receipts — this is the gate that proves a screen rebuild is byte-identical).

## The migration pattern (proven twice — follow it exactly)

To "murder" a bespoke screen emitter:
1. Rewrite its `emitText(list, tone, rect, id, text, action, selected, enabled)` calls as `emit(UiText{...}, out)` where `out` is a local `WidgetOutput`, then `appendWidgetOutput(list, out)` at the end. Rewrite `emitRect(...)` similarly as `emit(UiPanel{...}, out)`.
2. **Use designated initializers** — `UiText{.rect=..., .tone=..., .semanticId=..., .text=..., .action=..., .selected=..., .enabled=...}` (in that field order; omit trailing defaulted fields). This is mandatory: it stops silent `semanticId`/`text` and `selected`/`enabled` transpositions.
3. **Watch the arg-order flip**: old `emitText` is `(list, TONE, RECT, id, text, ...)`; `UiText` is `{RECT, TONE, ...}` — tone and rect are swapped. (Type-guarded, so a slip won't compile, but be careful.)
4. **Emission-preserving**: same primitives, same ORDER, same tones/rects/ids/text/flags, same counts. Do NOT change coordinates or introduce auto-layout — that is a separate, deliberately re-baselined step.
5. Gate: `make -C build -j8 iggy3d product_ui_draw_list_tests && ctest --test-dir build -R product_ui_draw_list_tests`. Then the full suite. The receipt for that screen must stay green with zero edits to the test.
6. `string_view -> string` needs an explicit `std::string(...)` wrap (e.g. `action.label`).

## Build & test commands

```bash
make -C build -j8 iggy3d                       # just the lib (fast)
make -C build -j8                               # everything (before the full gate)
ctest --test-dir build -R product_ui_draw_list_tests   # the UI receipt gate
ctest --test-dir build                          # FULL SUITE — the real gate (must be 172/172)
```
Note: adding a source file or test requires a CMake reconfigure — `make` triggers it, but the first `make` after editing `CMakeLists.txt`/`cmake/iggy3d_tests.cmake` regenerates and may not see a brand-new *test target* in the same invocation; just run `make` again.

## Next work (in priority order)

1. **[Highest leverage] Wire the input router to consume `hitRegions`.** Today the
   widget layer *emits* hit regions and the draw list *carries* them (receipt-guarded),
   but nothing *reads* them: `OpeningMenuView.cpp` (~line 1350) still hand-derives its
   own hit rects (`slotY=318; slotY+=38`) in parallel with the draw loop — the exact
   duplication the widget layer exists to kill. Route the hit-tester to read
   `list.hitRegions`, then delete the hand-derived rects. This *completes* the
   architecture's core promise. (Touches input routing — verify carefully, full suite.)
2. **Murder the remaining starter screens** from widgets, one per commit, byte-identical:
   `emitSettingsContent`, `emitDevToolsContent`, `emitNewWorldContent`. Each is guarded
   by a test in `product_ui_draw_list_tests` (`settingsAndDevToolsChildScreensAreModeled`,
   `newWorldChildScreenBuildsReadySelectorSurface`, etc.). A parameterized workflow for
   this exists — see below.
3. **Next bricks** (`ui_architecture.md` §implementation-order): `Button`, `ProgressBar`
   (both build on today's rect+text L0). Then `Grid`/`List`, then the two engine gaps:
   clipping (for `ScrollView`) and the textured-quad pipeline (for `Image`/`Icon`/`Viewport`).

## Discipline / gotchas

- Full suite green on every commit. Output-preserving rebuilds proven by byte-identical
  receipts. Small commits, `claude:` prefix, end messages with the Co-Authored-By line.
- **Do NOT commit `docs/plan_bucket/physics_*.md`** — those are not ours; stage files by
  explicit path.
- After a substantial change, consider re-running the adversarial review workflow
  (`.claude/workflows/ui-widget-layer-review.js`, if kept) to catch what the receipts
  don't (design smells, dropped side-outputs, doc drift).

## The Sonnet 5 workflow

A reusable, Sonnet-5-model workflow is saved at
**`.claude/workflows/ui-murder-screen.js`**. It rebuilds one bespoke starter screen
from the widget layer, gates it byte-identical, and adversarially verifies — leaving
the diff for a human to review and commit. Invoke it with the target screen:

```
Workflow({ name: "ui-murder-screen", args: "settings" })      # or "dev_tools" or "new_world"
```

It does NOT commit; review `git diff` and commit yourself when green.
