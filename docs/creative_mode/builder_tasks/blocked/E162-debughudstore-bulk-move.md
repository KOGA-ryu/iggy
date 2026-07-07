# E162 — DebugHudStore bulk-move (god-struct decomposition #9) — PARENT

**STATUS: STAGED in `blocked/`.** Recon-grounded + spot-verified (workflow `wdnplylk0`).
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
