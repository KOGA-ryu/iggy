# E162 — DebugHudStore bulk-move (god-struct decomposition #9) — PARENT

**STATUS: DONE.** Recon-grounded + spot-verified (workflow `wdnplylk0`).
**Commit convention:** `claude: planned. codex: …`.

> **EXECUTION SERIALIZES** on `ProductAppWindowState.hpp`; re-anchor at slice time.

---

## Ruling — STAND UP a store (not fold, not delete)

The map flagged #9 as "maybe delete/inline/fold-into-remainder." **Recon rules: it is a real store.** All 5
fields are written + read every session (none dead); the TSV already assigns all 5 to `DebugHudStore`; all 5
field-type structs already cohabit `src/app/iggy3d/debug/`. **Single atomic slice, ~79 repoints.**

## The 5 fields → `window.debugHud`

```
topDownMap  devCollisionOverlay  npcBehaviorDebugHud  physicsDebugHud  positionHud
```

## Method

New header `src/app/iggy3d/debug/DebugHudStore.hpp` — `struct DebugHudStore { ProductTopDownMapState topDownMap;
ProductDevCollisionOverlayState devCollisionOverlay; ProductNpcBehaviorDebugHudState npcBehaviorDebugHud;
ProductPhysicsDebugHud physicsDebugHud; ProductPositionHud positionHud; };`. Replace the 5 flat god-struct
fields with `DebugHudStore debugHud;`. Build; repoint each `no member named` error `window.<field> →
window.debugHud.<field>`.

## LAW & gates

- **Golden byte-identical** — these emit via string keys across `DebugHudFields.cpp` + `FrontendSettingsWindowFields.cpp`
  (topDownMap, devCollisionOverlay) + `GameplaySceneStateFields.cpp` (positionHud); a path repoint changes no
  key/value. A diff = BUG. *(Note the map §3 receipt-split — do NOT fork the store along it; one member serves all.)*
- **Compiler-guided, not sed.** Low-risk: production is uniform bare `window`; watch only the split appenders.
- **TSV:** delete the 5 `*␉DebugHudStore` rows, add one `debugHud␉DebugHudStore` row.
- Build green · ctest 260/260 · golden unchanged · update map #9 → DONE + PRIORITY.md.

## Suggested slices

**One gate.** Small and self-contained; no reason to split.

## Completion Brief

- Files changed:
  - `src/app/iggy3d/debug/DebugHudStore.hpp`
  - `src/app/iggy3d/ProductAppWindowState.hpp`
  - `src/app/iggy3d/ReceiptBuilder.cpp`
  - `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/menu/Transitions.cpp`
  - `src/app/iggy3d/receipt/DebugHudFields.cpp`
  - `src/app/iggy3d/receipt/FrontendSettingsWindowFields.cpp`
  - `src/app/iggy3d/receipt/GameplaySceneStateFields.cpp`
  - `tests/unit/product_menu_transitions_tests.cpp`
  - `tests/unit/product_top_down_map_overlay_tests.cpp`
  - `tests/unit/product_vulkan_room_frame_tests.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - `tools/iggy3d_product_frame_metrics/main.cpp`
  - `docs/god_struct_member_ownership.tsv`
  - `docs/god_struct_decomposition_target_map.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `docs/creative_mode/builder_tasks/claimed/E162-debughudstore-bulk-move.md`
- Store/API shape:
  - Added `iggy3d::DebugHudStore` with exactly `topDownMap`, `devCollisionOverlay`,
    `npcBehaviorDebugHud`, `physicsDebugHud`, and `positionHud`.
  - Replaced the five flat `ProductAppWindowState` members with
    `ProductAppWindowState::debugHud`.
- Repoint policy:
  - Used compiler-guided repoints for window state users only.
  - Left foreign payloads/receipt/frame fields such as projection overlays,
    debug line arrays, and input key state unchanged.
  - Final scans found no remaining direct `window.<moved debug hud field>`
    references and no direct moved-field declarations in `ProductAppWindowState.hpp`.
- Receipt/ownership result:
  - Receipt golden output is byte-identical; `git diff -- tests/golden/product_receipt_key_order.golden`
    produced no output.
  - Ownership TSV now has one `debugHud	DebugHudStore` row and no individual
    moved-field rows.
  - `product_god_struct_ownership_coverage_tests` reports `DebugHudStore=1`.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build -j10`
  - `/Users/kogaryu/iggy3d/build/product_receipt_key_order_tests`
  - `/Users/kogaryu/iggy3d/build/product_god_struct_ownership_coverage_tests`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure`
    - `100% tests passed, 0 tests failed out of 260`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
  - direct-field grep:
    `rg -n "\b(window|collisionWindow|mapMakerWindow)\.(topDownMap|devCollisionOverlay|npcBehaviorDebugHud|physicsDebugHud|positionHud)\b" src tests tools`
    produced no output.
- Concerns/deferred:
  - None for E162. PresentPathStore/FrontendWindowShell/CreativeAuthoringStore remain separate future cards.
