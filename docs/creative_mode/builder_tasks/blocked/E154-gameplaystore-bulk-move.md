# E154 — GameplayStore bulk-move (god-struct decomposition #8)

**STATUS: STAGED in `blocked/` — pending planner release.** Move to `ready/` (or on a "go") to release.
Recon-grounded (workflow `wxhudv684`, 2026-07-07) against HEAD `20371ac1`.
**Separate disjoint track from RoomStore (E148–E152) and SaveSessionStore (E153).**

**Commit convention:** `claude: planned. codex: <what you did>`.

---

## ⚠ SCOPE FIRST — this is the biggest move yet (~1670 repoints)

Five times SaveSessionStore. Dominated by `gameplay/Controller.cpp` (~494 sites). It is **mechanically
simple but large**, and it is riddled with the two traps that make `replace_all` **catastrophic** here
(see HAZARDS). **Two ways to land it — planner/reviewer choose:**
- **(A) Single compiler-guided move** (this card's method) — one big diff, mechanically safe if you obey
  the compiler. Fastest to write, hardest to review.
- **(B) Accessor-seam slice** (RoomStore E148–E152 pattern) — add `GameplayStore gameplay;` *alongside*
  the flat fields, migrate readers in batches behind accessors, delete the flat fields last, over several
  commits. Slower, far more reviewable. **Recommended if the 1670-line diff is a review problem.**
This card specifies (A); if you pick (B), the field list / gates / hazards below still apply per slice.

## Goal

Move **33** GameplayStore fields (32 map #8 members **+ `runtimeSessionCreated`**) off `ProductAppWindowState`
into a `GameplayStore` struct in its own header; god-struct holds one `GameplayStore gameplay;` member.
**Structural regroup — behavior-preserving, no freshness token** (these are per-frame projection results /
flat telemetry, not derived caches).

## LAW

1. **Receipt golden byte-identical.** `product_receipt_key_order.golden` keys on receipt *strings*
   (`gameplay_active`, `runtime_session_created`, `physics_movement_planner_*`, …), not C++ field names.
   A path repoint `window.X → window.gameplay.X` changes only the lvalue. **Any golden diff = a BUG
   (dropped/reordered `appendReceiptField`) — STOP, do NOT regenerate.**
2. **COMPILER-GUIDED, NEVER `replace_all`.** This is not optional here — a text replace of `window.<field>`
   silently misses ~60+ mis-named `ProductAppWindowState` vars and corrupts foreign types with the same
   member names (HAZARDS). Delete the flat fields, add the member, and fix each compiler error.

## Method (single move, A)

1. Create `src/app/iggy3d/gameplay/GameplayStore.hpp` — `struct GameplayStore { <the 33 fields, current
   order + defaults> };`, including the headers the field *types* need (ProductGameplayCommandState,
   ProductGameplayMovementInfo, ProductWallRunState, …ProductTransitionState, etc.).
2. In `ProductAppWindowState.hpp`: delete the 33 flat fields, add `GameplayStore gameplay;`. Include the header.
3. `cmake --build build -j8`; repoint every `no member named` error `<obj>.<field> → <obj>.gameplay.<field>`;
   repeat to 0. 4. `ctest -j8` → 260/260. Then the TSV + docs edits (same commit).

## The 33 fields to MOVE

```
runtimeSessionCreated gameplayActive gameplayInputUsed gameplayInputSource gameplayTickAdvanced
playerPositionChanged gameplayCommand gameplayMovement gameplayWallRun gameplayJump gameplayReset
gameplayTraversal gameplayDash gameplayCollision gameplayTickReasonCode physicsMovementPlanner
targetDiscovered gameplayTarget gameplayOutcome sessionOutcome gameplayTape interactionExecuted
attackExecuted productTransition gameplayReachGate gameplayLastRejection
playerVisible roomVisible objectiveVisible rendererMutatedRuntime scriptedGameplaySmoke
sceneItemCount debugItemCount
```

- **Diagnostics boundary RULING:** the 7 diagnostics (`playerVisible`, `roomVisible`, `objectiveVisible`,
  `rendererMutatedRuntime`, `scriptedGameplaySmoke`, `sceneItemCount`, `debugItemCount`) belong in **#8
  GameplayStore, NOT #9 DebugHudStore** — 6 are written in one block in `applyGameplayProjectionMetrics`,
  and the TSV already tags all 7 GameplayStore. DebugHudStore's set (topDownMap/devCollisionOverlay/
  npcBehaviorDebugHud/physicsDebugHud/positionHud) is disjoint and untouched here.
- **`physicsMovementPlanner` — one emitter + one writer + compiler-found stragglers:** the 5
  `physics_movement_planner_*` keys are emitted only in `GameplaySceneStateFields.cpp:62-71` (repoint 5
  rvalues); `PhysicsReceiptRecording.cpp` `setPhysicsMovementPlannerProof` *writes* the member (no keys —
  repoint 4 lvalues). It is ALSO referenced in `Controller.cpp` and others — **let the compiler find those**.

## HAZARDS (why replace_all is banned)

- **Mis-named `ProductAppWindowState` vars (invisible to a `window.` replace — only the compiler sees them):**
  in `Controller.cpp`: `fastWindow`, `slowWindow`, `normalWindow`, `candidateWindow`, `rejectedWindow`,
  `wallWindow`, `cutWindow`; in tests: `attackWindow`, `noTargetWindow`, `neutralWindow`, `rejectedWindow`
  (feedback), `pauseWindow`, `tuningWindow`, `creativeWorldWindow`, `staleWindow`, `staleStarterWindow`,
  `openWindow`, `blocked`, `inactive` (movement-debug-hud copies). Several test files are **100% mis-named,
  zero bare `window`** (`product_gameplay_feedback_tests.cpp`, `product_gameplay_tape_runner_tests.cpp`).
- **Foreign-type bare-token collisions (never bare-token replace `playerVisible`/`gameplayActive`/etc.):**
  `PrimitiveDrawList.hpp` and `RenderBridge` declare their OWN `playerVisible`/`roomVisible`/`objectiveVisible`;
  `FrontendRouter.hpp`/`DrawList.hpp` context structs declare their own `gameplayActive`. **Do NOT sweep**
  `product_render_bridge_tests.cpp` / `product_primitive_draw_list_tests.cpp` — their `playerVisible` reads
  are on those foreign types (false positives). Anchor to `window.<field>` / fix compiler errors only.
- **Wrapper substrings (SAFE — repoint to `<wrapper>.window.gameplay.<field>`):** `context.window.*`
  (InputFrame), `request.window.*` (FramePresenter), `harness.window.*` (starter tests) — all hold a live
  `ProductAppWindowState& window`.
- **Prefix-collision check CLEARED:** no gameplay field is a proper prefix of another; but this only helps
  if you were anchoring `window.<field>\b` — which you should NOT rely on. Compiler-drive it.

## Scope table (compiler finds the exact set)

**Src (~dominant):** `gameplay/Controller.cpp` 494 · `gameplay/TapeRunner.cpp` 72 ·
`receipt/GameplaySceneStateFields.cpp` 68 · `receipt/GameplayRuntimeMovementFields.cpp` 64 ·
`window/InputFrame.cpp` 59 (`context.window.*`) · `gameplay/MovementProof.cpp` 48 · `menu/Transitions.cpp`
45 · `gameplay/GameplayFeedback.cpp` 22 · `gameplay/ProjectionRefresh.cpp` 18 · `menu/ActionHandlers.cpp`
15 · `automation/AutomationGameplay.cpp` 9 · `window/FramePresenter.cpp` 8 (`request.window.*`).

**Tests (the migration tail):**

| file | ~sites | structure |
|---|--:|---|
| `product_gameplay_controller_tests.cpp` | 354 | bare 294 + **60 mis-named** (fastWindow×22, slowWindow×15, …) |
| `product_window_input_frame_tests.cpp` | 124 | bare 110 + 14 mis-named (pauseWindow, tuningWindow, …) |
| `product_movement_debug_hud_tests.cpp` | 87 | bare 67 + 20 mis-named (`blocked`×19, `inactive`) |
| `product_menu_transitions_tests.cpp` | 41 | bare 39 + `staleStarterWindow`×2 (runtimeSessionCreated lockstep) |
| `product_vulkan_room_frame_tests.cpp` | 28 | all bare — replace-safe within file |
| `product_starter_menu_action_tests.cpp` | 25 | all `harness.window.*` (wrapper-safe) |
| `product_gameplay_feedback_tests.cpp` | 21 | **100% mis-named** (attackWindow×9, rejectedWindow×6, …) |
| `product_frontend_router_tests.cpp` | 12 | bare 3 + **9 on FOREIGN types** declaring `gameplayActive` (do NOT touch) |
| `product_gameplay_tape_runner_tests.cpp` | 5 | 100% mis-named + foreign `ProductGameplayTapeRunResult` vars |
| `product_creative_world_launch_tests.cpp` | 9 | bare 7 + `openWindow`×2 |
| + `interaction_mode`(7), `ascii_room_activation`(4, runtimeSessionCreated), `creative_ui_input_frame`(5), misc | | mostly bare |
| `product_render_bridge_tests.cpp`, `product_primitive_draw_list_tests.cpp` | 0 | **FALSE POSITIVES — do not sweep** |

## GATES (one commit for method A; per-slice for B)

1. `cmake --build build -j8` → 0 errors. 2. `ctest -j8` → **260/260**.
3. **golden UNCHANGED** (`git diff` empty). 4. **`docs/god_struct_member_ownership.tsv`:** delete the 33
   rows (the 32 `*→GameplayStore` rows **plus** the `runtimeSessionCreated` row — it becomes a sub-field,
   invisible to the top-level gate), add one `gameplay␉GameplayStore` row. (`GameplayStore` already a valid
   owner.) 5. Update `docs/god_struct_decomposition_target_map.md` (#8 → DONE) and `PRIORITY.md`.

## Coordination with E153

`runtimeSessionCreated` lands in `gameplay` here. E153 (SaveSessionStore) **excludes** it and retargets its
TSV row to GameplayStore. Whichever lands second must **not** re-touch it:
- If **E153 first**: its row reads `runtimeSessionCreated␉GameplayStore`; E154 still **deletes** that row.
- If **E154 first**: E154 folds it into `gameplay` and deletes the row; E153 then **skips** its
  runtimeSessionCreated retarget step (field already gone from the top level).

## Why safe

Compiler = exhaustive reader-finder (it sees every `fastWindow`/`rejectedWindow` a grep/replace can't);
golden = byte-level behavior oracle; coverage gate = bidirectional member accounting. Green build +
260/260 + unchanged golden + updated TSV = provably complete and behavior-preserving. No freshness debt.
