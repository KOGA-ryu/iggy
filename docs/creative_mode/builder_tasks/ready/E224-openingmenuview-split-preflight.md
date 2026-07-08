# E224 - OpeningMenuView Split Preflight

## Status

Ready.

## Context

The render draw-kind metadata lane is now complete as E222-E223. The remaining
render organization item from `docs/complexity_audit_v0_1.md` is the large
mixed-responsibility `OpeningMenuView.cpp` file.

Current size at release:

- `src/app/iggy3d/view/OpeningMenuView.cpp`: 1654 lines
- `src/app/iggy3d/view/OpeningMenuView.hpp`: 114 lines

The complexity audit recommends splitting `OpeningMenuView.cpp` by domain into
something like `MenuPanelsView`, `DebugHudView`, `RoomEditorOverlayView`,
`ScenePrimitiveView`, and a shared SDL draw helper, while keeping
`drawOpeningMenuView(...)` as the thin entry point.

This card is only the preflight. Do not implement the split in this card.

## Objective

Read-only audit the current `OpeningMenuView` surface and produce a grounded
split plan with safe child slices.

The output should answer:

1. Which line ranges and functions belong to each domain cluster?
2. Which helpers are genuinely shared draw primitives versus domain policy?
3. Which functions are pure rendering and which are hit-test/action routing?
4. Whether `OpeningMenuView.hpp` should stay as the facade or shrink in a later
   slice.
5. Whether a shared `SdlDraw` helper is justified, and if so which exact
   functions belong there.
6. Which implementation slice should go first with the least compile/test
   fallout.

## Audit Scope

Inspect:

- `src/app/iggy3d/view/OpeningMenuView.cpp`
- `src/app/iggy3d/view/OpeningMenuView.hpp`
- focused callers found by grep, especially `drawOpeningMenuView(...)`,
  `openingMenuActionAt(...)`, and `pauseMenuActionAt(...)`
- focused tests found by grep for `OpeningMenuView`, hit tests, menu routing,
  and render bridge / viewport primitives
- `docs/complexity_audit_v0_1.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- relevant completed cards if needed for context, especially E222-E223

## Classification Requirements

Produce a table or grouped list covering at least these expected clusters:

- SDL primitive helpers:
  - `setColor(...)`
  - `fillRect(...)`
  - `drawText(...)`
  - any tiny renderer helper that is not domain policy
- scene primitive / viewport drawing:
  - marker, room tile, door marker, primitive item dispatch
  - first-person primitive viewport
  - top-down map primitive rendering
  - grid drawing
- gameplay/debug HUD drawing:
  - camera heading
  - gameplay feedback
  - interaction mode HUD
  - movement / NPC / physics debug HUDs
  - position HUD
  - movement tuning HUD
- room editor overlay drawing:
  - room editor HUD
  - room editor cursor / placement preview relationships if applicable
- menu panels:
  - starter, new-world, load-save, delete confirmation, dev tools, settings,
    gameplay panel, ASCII preview lines, panel rows, sliders
- hit-test and action routing:
  - `uiRectContains(...)`
  - `pauseMenuActionAt(...)`
  - `openingMenuDetailSurfaceFor(...)`
  - `openingMenuActionAt(...)`
- facade / orchestration:
  - `drawOpeningMenuView(...)`

For each cluster, classify:

- proposed file owner or "keep in facade"
- whether it can move in a pure extraction slice
- expected CMake/source-list impact
- focused tests that should gate the slice
- self-blockers or behavior traps

## Required Greps

Run and report:

```sh
rg -n "drawOpeningMenuView|OpeningMenuView|OpeningMenuHitTestResult|openingMenuActionAt|pauseMenuActionAt|draw[A-Z]|setColor|fillRect|drawText|ProductViewportFramedItem|ProductUiRect|TopDownMapOverlay|Hud|Panel|Primitive" /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/view/OpeningMenuView.hpp /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "OpeningMenuView|OpeningMenuView.cpp|MenuPanelsView|DebugHudView|RoomEditorOverlayView|ScenePrimitiveView|SdlDraw" /Users/kogaryu/iggy3d/docs/complexity_audit_v0_1.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/PRIORITY.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done --glob '*.md'
git -C /Users/kogaryu/iggy3d diff --check
```

Use additional `rg`, `sed`, `nl`, or `awk` reads as needed for exact line
ranges and call-site mapping.

## Deliverables

Append a completion brief to this card with:

- files inspected
- current line counts
- grouped domain/line-range table
- direct caller/test hotspot summary
- shared helper classification
- recommended child-card sequence
- safest first implementation card, or a clear "no implementation card"
  recommendation if the split is not worthwhile
- focused verification commands for the first implementation card
- stale assumptions found in `docs/complexity_audit_v0_1.md` or priority docs
- confirmation that no source, tests, CMake, receipt golden, staging, commit,
  push, or window launch was performed

## Non-Scope

Do not edit source, tests, CMake, fixtures, receipt golden files, or production
docs in this card.

Do not:

- split `OpeningMenuView.cpp`
- create `SdlDraw.hpp`
- move `drawOpeningMenuView(...)`
- change hit-test behavior
- change render output
- change draw ordering
- change receipt fields/order/values
- launch a window
- run broad CTest
- stage, commit, or push

## Self-Blockers

Stop and report if:

- the split requires a behavior decision rather than pure extraction
- line-range ownership cannot be made clear from current code
- tests do not cover the proposed first slice well enough to recommend it
- the only available split would create a mega-helper or hide domain policy
