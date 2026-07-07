# E156 — InputDeviceStore bulk-move (god-struct decomposition #7) — PARENT

**STATUS: STAGED in `blocked/` — parent card for a slicer-Codex to decompose into builder gates.**
Recon-grounded + spot-verified (workflow `wkqdxuj2u`, 2026-07-07). **Commit convention:** `claude: planned. codex: …`.

> **Tree is mid-flight** (RoomStore/E153 just landed). **Anchor by field NAME; re-verify lines at slice time.**

---

## Goal

Move the **11 input-device/capture fields** off the god-struct into a new `InputDeviceStore` struct in its own
header; god-struct holds one `InputDeviceStore inputDevice;` member. **Plain owned state, no derived truth, no
freshness token.**

## The 11 fields

```
gamepadAvailable gamepadMenuSelectUsed gamepadName gamepadMapping
interactionMode interactionModeHud
mouseCapture controllerModeToggle controllerAction
lastInputAction lastInputAccepted
```

- The map's two **DELETEs** (`inputOwner`, `gameplayInputSuppressed`) are **already done** (`36ceeac3`) — not here.
- `mouseCapture.inputOwner` (receipt-only) **stays inside `mouseCapture`** and moves with it as one unit.

## Member design

Create `src/app/iggy3d/input/InputDeviceStore.hpp` — `struct InputDeviceStore { …11 fields verbatim… };` with
the types' existing includes (InteractionMode, InteractionModeHud, MouseCaptureState, ControllerModeToggleState,
ControllerActionState, InputAction). Replace the 11 flat god-struct fields with `InputDeviceStore inputDevice;`.

## LAW

1. **Receipt golden byte-identical.** These fields emit via string keys in `receipt/FrontendSettingsWindowFields.cpp`
   (gamepad*/interactionMode/controller*) and `receipt/FeedbackSurfaceAutomationVulkanFields.cpp`
   (lastInputAction/Accepted). Path repoint changes no key/value/order. **A golden diff = a BUG — STOP.**
2. **Compiler-guided, NEVER `replace_all`.** Non-negotiable here — see the dominant hazard.

## ⚠ DOMINANT HAZARD — `interactionMode` is a real field on 6+ foreign structs

A global sed of `interactionMode` **corrupts unrelated code.** `ProductInteractionMode interactionMode` is
declared on: `ControllerActionRouting.hpp` (×2), `ControllerActionMap.hpp` (×2), `MouseCapturePolicy.hpp`,
`FrontendRouter.hpp`, `TopDownMapOverlay.hpp` (verified). Additionally:
- **Coexistence files** where the god-struct AND a foreign `.interactionMode` appear together — e.g.
  `Operations.cpp` accesses the window via the alias `window_` (`ProductAppWindowState& window_;`), so a naive
  `window.` anchor both misses `window_.interactionMode` and risks the foreign hits.
- **`interactionModeHud`** collides on `frame.*`/`projectionFrame.*`/`projection.*` (~9 hits) — do NOT touch.
- **`mouseCapture.inputOwner`** shares the bare name `inputOwner` with ~90 unrelated `MenuOwner`/surface refs
  — it moves *inside* `mouseCapture`, so never bare-token replace `inputOwner`.

**The only safe method: delete the flat fields, add `inputDevice`, and repoint every `no member named` error
the compiler reports** (it distinguishes `window.interactionMode` from `routing.interactionMode`). Grep/sed cannot.

## Method

1. Create `InputDeviceStore.hpp`; replace the 11 flat fields in `ProductAppWindowState.hpp` with
   `InputDeviceStore inputDevice;`. 2. Build; repoint each error `<obj>.<field> → <obj>.inputDevice.<field>`
   (incl. the `window_` alias in Operations.cpp). 3. ctest 260/260. 4. TSV/docs.

## Scope (~265 repoints)

`interactionMode` is the big one (read across gameplay/menu/input for Player-vs-Creative gating). The other 10
fields are lighter. **~50 mis-named `ProductAppWindowState` test aliases** exist across input tests (the crux of
"grep undercounts") — the compiler enumerates them all; a `window.` replace misses them. A few harness files
(`harness.window.*`, `settings.window.*`) surface normally via the compiler.

## TSV edit

Delete the 11 `*␉InputDeviceStore` rows (they already carry the target label), add one `inputDevice␉InputDeviceStore`
row. (`InputDeviceStore` is already a valid owner.)

## GATES

Build green · ctest 260/260 · golden byte-identical · TSV as above · update decomposition map #7 → DONE + PRIORITY.md.

## Why safe

The compiler cleanly separates `window.interactionMode` from the 6 foreign `interactionMode` fields — the one
thing a text tool cannot. Golden + coverage gate prove behavior + accounting.

## Suggested slices (for the slicer-Codex)

- **G1** — `InputDeviceStore.hpp` types + the god-struct member swap; repoint **production** readers
  (compiler-driven; heaviest = `interactionMode`, mind the `window_` alias + foreign-struct namesakes); build green.
- **G2** — migrate **test** readers (the ~50 mis-named aliases + harness files); ctest 260/260.
- **G3** — TSV edit + map/PRIORITY docs; golden confirmed unchanged.
(Given the interactionMode collision density, keeping G1 production-only + G2 tests makes review tractable.)
