# E154 — GameplayStore bulk-move (god-struct decomposition #8)

**STATUS: DECOMPOSED PARENT — do not claim directly.**
Sliced into E157-E160 on 2026-07-07 so builder can send smaller briefs after
each document and reviewer can release the next slice one at a time.
Recon-grounded (workflow `wxhudv684`, 2026-07-07) against HEAD `20371ac1`.
**Separate disjoint track from RoomStore (E148–E152) and SaveSessionStore (E153).**

**Commit convention:** `claude: planned. codex: <what you did>`.

---

## SCOPE — large but mechanically uniform (~1670 repoints, LOW risk)

Biggest move by count, but **production code is 100% uniform**: every window var in `src`/`apps` is named
`window` (verified — **0 mis-named**), so the dominant lane (`gameplay/Controller.cpp` ≈ 474 bare
`window.gameplay*` sites) is pure mechanical repointing. The only non-uniform surface is a **42-var,
test-only fixture tail** (named gameplay scenarios like `attackWindow`/`neutralWindow`/`openWindow`), all
caught by the compiler. **No controller-brand or keyboard/gamepad duplication exists.** Two ways to land it:
- **(A) Single compiler-guided move** (this card) — one large but low-risk diff. **Recommended.**
- **(B) Accessor-seam slice** (RoomStore E148–E152 pattern) — only if a ~1670-line diff is a review burden;
  it is NOT needed for safety.
Field list / gates / hazards below apply either way.

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
2. **COMPILER-GUIDED, NEVER `replace_all`.** A text replace of `window.<field>` silently misses the 42
   test-fixture window vars (`openWindow`/`attackWindow`/…) and, via bare-token replace, corrupts foreign
   types with the same member names (HAZARDS). Delete the flat fields, add the member, fix each compiler
   error. (Production is uniform bare `window`, so risk is low — this rule is about the test tail + the
   foreign-name collisions, not a minefield.)

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

## HAZARDS (all auto-handled by the nested-member + compiler method)

These only bite if you deviate into a bare text replace. The method makes them near-automatic.

- **Mis-named window vars are a TEST-ONLY, compiler-caught tail — 0 in production.** Verified: `src`/`apps`
  have **no** window var named anything but `window` (Controller.cpp's ~474 sites are all bare `window`).
  The 42 mis-named vars are all in tests, each a named gameplay-scenario fixture: `openWindow`(×7),
  `attackWindow`, `neutralWindow`, `noTargetWindow`, `rejectedWindow`, `blockedWindow`, `staleWindow`,
  `pauseWindow`, `creativeWorldWindow`, `inactiveWindow`, … A `window.`-anchored replace would miss them —
  which is exactly why you **fix compiler errors** instead. (`product_gameplay_feedback_tests.cpp` /
  `product_gameplay_tape_runner_tests.cpp` are ~100% mis-named.) These are **scenario fixtures, NOT
  controller/input-device variants** — there is no keyboard/gamepad duplication.
- **Foreign structs share field names — but the compiler NEVER touches them under this method.**
  `playerVisible`/`roomVisible`/`objectiveVisible` are also members of `PrimitiveDrawList`/`RenderBridge`;
  `gameplayActive` lives on Presentation/MouseCapturePolicy/FrontendRouter-contexts/DrawList/
  InteractionModeHud. Because you delete only `ProductAppWindowState`'s flat fields, the compiler errors
  only on `window.<field>` (repoint those) and leaves `drawList.playerVisible` etc. untouched. This is ONLY
  a hazard for a bare-token replace (`s/playerVisible/…/`) — banned. Corollary:
  `product_render_bridge_tests.cpp` / `product_primitive_draw_list_tests.cpp` read the FOREIGN
  `playerVisible` — do NOT "fix" them.
- **Wrapper substrings (SAFE — repoint to `<wrapper>.window.gameplay.<field>`):** `context.window.*`
  (InputFrame), `request.window.*` (FramePresenter), `harness.window.*` (starter tests) — all hold a live
  `ProductAppWindowState& window`.

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

Compiler = exhaustive reader-finder (it sees every `openWindow`/`rejectedWindow` test fixture a grep/replace can't);
golden = byte-level behavior oracle; coverage gate = bidirectional member accounting. Green build +
260/260 + unchanged golden + updated TSV = provably complete and behavior-preserving. No freshness debt.

## Decomposition

- **E157** — GameplayStore G1: create the `GameplayStore` member and move the
  lifecycle/input flags (`runtimeSessionCreated`, `gameplayActive`,
  `gameplayInputUsed`, `gameplayInputSource`, `gameplayTickAdvanced`,
  `playerPositionChanged`).
- **E158** — GameplayStore G2: move movement/planner command state
  (`gameplayCommand`, `gameplayMovement`, `gameplayWallRun`, `gameplayJump`,
  `gameplayReset`, `gameplayTraversal`, `gameplayDash`, `gameplayCollision`,
  `gameplayTickReasonCode`, `physicsMovementPlanner`).
- **E159** — GameplayStore G3: move action/outcome/tape/transition state
  (`targetDiscovered`, `gameplayTarget`, `gameplayOutcome`, `sessionOutcome`,
  `gameplayTape`, `interactionExecuted`, `attackExecuted`,
  `productTransition`, `gameplayReachGate`, `gameplayLastRejection`).
- **E160** — GameplayStore G4: move visibility/render diagnostics
  (`playerVisible`, `roomVisible`, `objectiveVisible`,
  `rendererMutatedRuntime`, `scriptedGameplaySmoke`, `sceneItemCount`,
  `debugItemCount`) and mark the target map complete.
