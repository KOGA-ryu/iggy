# E133 (E-ARCF-G6): freshness store stress + shutdown behavior

## Objective

Gate 6 of the freshness guard: prove the store holds under **stress** (random command-order permutations) and
**shutdown/transition** (session reset, empty-room churn) — no divergence, no dangling-session read, no
per-frame rebake thrash. **Test-only gate — no production change.**

**Gated spine work.** Owner design: `docs/active_room_collision_freshness_preflight_v0_2.md` — read §6
(thread/lifetime), **§9 (shutdown behavior)**, §5-I5 (idempotence), and the G6 row of the Gate table.

## Why This Exists

G4 proved the *two*-command order case (T-order) and G5 made `ensure` the sole rebuild path. G6 widens that to
N-permutation stress + the lifecycle edges (session teardown, empty rooms) that a single scenario doesn't
exercise, so the store is trusted before the finish gate (G7).

## Required Work (tests only; no production edits)

1. **Order-permutation stress (widen T-order):** a room with ≥2 runtime-owned doors. For several random
   permutations of a fixed multiset of ticked door-toggle commands in one frame, run to the frame-boundary
   `ensure` and assert the reader-visible `window.activeRoomCollision` snapshot (`querySurfaceCount`,
   `activeDoorBlockerSurfaceCount`, `runtimeFilteredSurfaceCount`, surface-set size, surface ids, room-revision
   stamp) is **identical across all permutations**. (Determinism: pick the permutations from a fixed seed passed
   in / hard-coded — do NOT call `Math.random`/`rand`; enumerate or use a fixed list.)
2. **Session-reset-then-`ensure` (§9):** load a room + session, `ensure` (baked with door filtering,
   `bakedFromSessionHash != 0`). Then `activeSession.reset()` (or drop to `nullptr`) and `ensure(window, nullptr)`;
   assert it takes the window-less path — `bakedFromSessionHash == 0`, no crash, no read of destroyed session
   state — and the blob is re-stamped for the sessionless world (I6).
3. **Empty-room thrash (I5):** set a loaded-but-degenerate/empty room (no surfaces). Call `ensure` repeatedly
   across many frames with NO room/session change; assert **exactly one** bake occurs (first call `rebaked==true`,
   every subsequent `rebaked==false`, `reasonCode=="skipped_fresh"`), counts byte-identical — the empty room does
   NOT thrash-rebake every frame.
4. **C2 — I7 re-audit:** grep `entity.active`/`setActive` outside the hashed session command path; must stay
   empty. Record in the brief. (The G5 reviewer already confirmed the `setReasoningGraph` activation hook writes
   off-hash and does not stale the stamp — re-confirm and note.)

Add these to `tests/unit/product_active_room_collision_tests.cpp` (or a focused sibling) using the no-window
fixtures. No production file changes.

## Acceptance Notes

- Stress: reader snapshot **identical across all command-order permutations** — no divergence.
- Reset: `ensure(window, nullptr)` after `activeSession.reset()` is safe, `bakedFromSessionHash==0`, no dangling read.
- Thrash: empty room rebakes **exactly once**, then `skipped_fresh` forever (I5) — no per-frame churn.
- Full suite green + **UNCHANGED** (test-only gate; `product_receipt_key_order_tests` +
  `product_creative_no_window_bake_scenario_tests` verbatim). `git diff --check` clean.
- C2 I7 re-audit recorded.

## Do Not

- Do NOT change any production file (this gate adds tests only).
- Do NOT use `rand`/`Math.random`/non-deterministic ordering — use a fixed permutation set (determinism).
- Do NOT touch collision math / overloads / save-hash. Do NOT name anything `Kernel`.
- Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_active_room_collision_tests$' --output-on-failure
ctest --test-dir /Users/kogaryu/iggy3d/build
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed (tests only):
- Permutation stress (how many orderings; identical?):
- Session-reset-then-ensure result:
- Empty-room thrash result (exactly-one-bake?):
- C2 I7 re-audit:
- Suite verbatim:
- Concerns/deferred:
