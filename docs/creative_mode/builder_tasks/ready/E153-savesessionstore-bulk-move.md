# E153 — SaveSessionStore bulk-move (god-struct decomposition #5)

**STATUS: READY — claim after E148-E152 unless planner changes priority.**
Recon-grounded (workflow `wal28mbzs`, 2026-07-07) against HEAD `36ceeac3`.
**Separate track from the RoomStore regroup (E148–E152, `activeRoom`) already in `ready/`** — disjoint
field sets, so either order is fine; do this after RoomStore unless directed otherwise.

**Commit convention:** `claude: planned. codex: <what you did>` — this card is the plan; you build.

---

## Goal

Move the **31** SaveSessionStore fields off the `ProductAppWindowState` god-struct into a
`SaveSessionStore` struct in its own header, so the god-struct holds one `SaveSessionStore
saveSession;` member instead of 31 flat fields. **Structural regroup only — behavior-preserving.**

## LAW (do not violate)

1. **The receipt golden must stay BYTE-IDENTICAL.** `tests/golden/product_receipt_key_order.golden`
   keys on emitted receipt *strings* (`product_save_status`, `active_product_save_id`, …), not C++
   field names. Repointing `window.X → window.saveSession.X` changes only the lvalue the compiler
   resolves — same key, same value, same emit order. **If the golden diffs, STOP: you introduced a
   bug. Do NOT regenerate it to absorb the diff.**
2. **No freshness token.** None of these 31 fields are derived truth — they are plain owned
   state (verified). This is NOT the collision/creativeFly pattern. Do not add revision/provenance.
3. **Compiler-guided, NOT `replace_all`.** Grep undercounts and cannot see the mis-named window
   vars (below). Let the compiler enumerate readers.

## Method (nested-member, compiler-driven)

1. Create `src/app/iggy3d/save/SaveSessionStore.hpp` — `struct SaveSessionStore { <the 31 fields, in
   their current order, with their current defaults> };`. Include the headers those field *types*
   need (`ProductSaveLoadResult`, `ProductSavedRoomMarkerBindingResult`,
   `ProductSelectedProductSaveState`, `ProductSaveFlowState`, `ProductSaveDeleteState`,
   `ProductSaveRecoverState`).
2. In `ProductAppWindowState.hpp`: **delete the 31 flat fields**, add `SaveSessionStore saveSession;`
   in their place (keep it near the old location for a readable diff). Include the new header.
3. `cmake --build build -j8`. For **every** `no member named '<field>' in ProductAppWindowState`
   error, repoint that reader: `<obj>.<field> → <obj>.saveSession.<field>`. Repeat until 0 errors.
4. `cd build && ctest -j8` → **260/260**. Then the TSV + docs edits below (same commit).

## The 31 fields to MOVE (KEEP)

```
productSaveStatus productSaveReasonCode productSaveDurableReason productSaveSource
productSaveSaveId productSaveSessionSaved activeProductSaveId
productSaveLoadResult productSaveLoadSource productSaveLoadSelectedId productSaveLoadSelectedEnabled
savedMarkerBind selectedProductSave
saveSlotBrowserMode saveSlotRingCount saveSlotRingSelectedIndex saveSlotRingSelectedId
saveSlotRingSelectedStatus saveSlotActionCommand saveSlotActionEnabled
saveSlotActionConfirmationRequired saveSlotActionStatus
saveFlow saveDelete saveRecover
deletedSaveBrowserOpen deletedSaveCount deletedCompatibleSaveCount
deletedSelectedSaveId deletedSelectedSaveEnabled deletedSelectedSaveStatus
```

## EXCLUDE — `runtimeSessionCreated` (do NOT move it)

It's a **gameplay-launch flag misfiled into #5**: written in lockstep with `window.gameplayActive`
at 6 sites (`Operations.cpp:153,248,412`, `Activation.cpp:127`, `Transitions.cpp:227,236`); its only
reader is `GameplayRuntimeMovementFields.cpp:25` (the gameplay receipt, not `SaveStateFields.cpp`).
**Leave it on the god-struct.** In the TSV, **retarget its row from `SaveSessionStore` → `GameplayStore`**
(a value fix, not a delete — the field still exists). It joins GameplayStore in a later move.

## Scope — honest expectations (~310 repoints)

Dominated by two files (compiler will list every site):
- `Operations.cpp` — ~146 sites (the save-mutation lane).
- `receipt/SaveStateFields.cpp` — ~82 sites (the sole save receipt appender). **⚠ This file ALSO reads
  `roomEditorCursorReady`, `roomEditorCursor.*`, `roomEditorStatus/ReasonCode/LastOperation`
  (~L129–146) — those belong to CreativeAuthoringStore #4, NOT this move. Repoint ONLY the 31 save
  fields; do not sweep the file.**
- Smaller: `AutomationSaveBrowser.cpp` (6, `context.window.*`), `InputFrame.cpp` (4),
  `ActionHandlers.cpp` (4), `FramePresenter.cpp` (3, `request.window.*`), `Transitions.cpp` (2),
  `DrawList.hpp` (2), `InputRouter.cpp` (1).

## HAZARD WARNINGS (the exact traps the warm-up hit)

- **Wrapped-but-SAFE** — `context.window.saveDelete`, `request.window.selectedProductSave`,
  `harness.window.*`, `scenario.window.*`, `deleteSave.window.*`, `loadSave.window.*`, `slot.window.*`
  all hold a live `ProductAppWindowState& window` — the compiler fixes these normally to
  `<wrapper>.window.saveSession.<field>`. Fine.
- **Mis-named window vars — the compiler catches, replace_all would MISS:** hand-repoint
  `openWindow.activeProductSaveId` (×3) and `productWindow.activeProductSaveId` (×2) in
  `product_creative_world_launch_tests.cpp`, and `staleStarterWindow.runtimeSessionCreated`… — wait,
  `runtimeSessionCreated` is EXCLUDED, so `staleStarterWindow.runtimeSessionCreated`
  (`product_menu_transitions_tests.cpp:129`) **stays unchanged**. But `openWindow`/`productWindow`
  save-field reads DO move → `openWindow.saveSession.activeProductSaveId`, etc.
- **NEVER bare-token replace** `saveFlow`/`saveDelete`/`activeProductSaveId` etc. — those are also
  nested-member roots (`saveDelete.confirmationOpen`, `saveFlow.<sub>`). Anchor to `window.<field>`, or
  better, just fix compiler errors.

## Test-migration tail (per-file structure)

| test file | ~sites | structure |
|---|--:|---|
| `product_save_delete_executor_tests.cpp` | 20 | bare `window` — safe |
| `product_creative_world_launch_tests.cpp` | 15 | MIXED + **mis-named vars `openWindow`/`productWindow`** (hand-repoint) |
| `product_starter_menu_action_tests.cpp` | 9 | fully wrapped `harness.window.*` — compiler-safe |
| `product_window_input_frame_tests.cpp` | 8 | mixed bare + wrapped — safe |
| `product_menu_transitions_tests.cpp` | 2 | bare (its `staleStarterWindow.runtimeSessionCreated` is EXCLUDED, untouched) |
| `product_ascii_room_activation_tests.cpp` | 2 | bare `runtimeSessionCreated` (EXCLUDED — untouched) |
| `product_interaction_mode_state_tests.cpp`, `product_automation_dispatch_tests.cpp` | 1 ea | bare — safe |

Unlike the warm-up delete, these tests read `saveSession.<field>` for **value**, not as a
deleted-mirror observable — so it's a pure path repoint, no re-pointing to a different source.

## GATES / acceptance (all in ONE commit)

1. `cmake --build build -j8` → 0 errors.
2. `ctest -j8` → **260/260**.
3. **`product_receipt_key_order.golden` UNCHANGED** (`git diff` on it is empty). A diff = bug → STOP.
4. **`docs/god_struct_member_ownership.tsv`** (the coverage gate parses top-level members textually,
   bidirectionally): **delete the 31 moved rows**, **add one** `saveSession␉SaveSessionStore`, and
   **retarget** the `runtimeSessionCreated` row to `GameplayStore`. (`SaveSessionStore` is already a
   valid owner.) Net rows 212 → 182.
5. Update `docs/god_struct_decomposition_target_map.md` (#5 → DONE; note the `runtimeSessionCreated`
   exclusion→GameplayStore correction) and `PRIORITY.md`.

## Why this is safe

The compiler is the exhaustive reader-finder (grep undercounts and can't see `openWindow`/
`productWindow`), the receipt golden is a byte-level behavior oracle, and the god-struct coverage gate
enforces the member accounting bidirectionally. **Green build + 260/260 + unchanged golden + updated
TSV = provably complete and behavior-preserving.** No freshness debt, no derived truth — the cheapest
kind of decomposition, exactly what the bulk-move method is for.
