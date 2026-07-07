# E130 (E-ARCF-G4): wire the freshness seam — bump all writers + frame-boundary `ensure` (first consumer)

## Objective

Gate 4 of the activeRoom→collision freshness guard: **wire the one real consumer.** Bump
`activeRoomRevision` at every `activeRoom` writer, and call `ensureActiveRoomCollisionFresh` once at the
**frame-boundary seam** (and the TapeRunner read seam) so production readers now depend on the freshness
store. **Do NOT remove any direct rebake yet** — the direct writer-bakes stay (redundant but harmless) until
G5. Prove order-independence with T-order.

**Gated spine work — the riskiest gate (first production behavior change).** Owner design:
`docs/active_room_collision_freshness_preflight_v0_2.md` — read **§6 (the seam + call contract)**, §5
(invariants, esp. I1/I2/I7), **§10 (T-order + T-tape)**, and the removal checklist
`docs/active_room_collision_rebake_removal_checklist.md` **§A (the bump sites)**.

## Why This Exists

G2/G3 built the store + the dirty-check with no consumer. G4 makes production *use* it: one owner ensures
freshness before readers, replacing "every writer remembers to rebake in the right order." T-order proves the
handler-order dependence is gone. (G5 then deletes the now-redundant scattered rebakes.)

## Required Work

1. **Bump the revision at every writer** (removal checklist §A, the "G4 bump" column). Add
   `bumpActiveRoomRevision(window)` after the **FINAL** `window.activeRoom` write in each of the ~11 sites:
   `Operations.cpp` {187, 284/clear, 453/terminal-clear, 721, 746, 1327, 1663}, `Activation.cpp` {71, 118},
   and — critically — **after the whole-struct copy** `AutomationRoomEditing.cpp:88` (window-owned counter is
   copy-safe, I2). Follow the §G4 bump discipline: bump once after the final write; terminal clears bump;
   intermediate clear-then-set do not; `Activation.cpp:72`'s intermediate blob may ship unstamped.
2. **Call `ensure` at the frame-boundary seam** (preflight §6): after the session tick settles
   (`markDirtyAndHash` has run, `stateHashDirty` cleared) and **before** the reader fan-out — the window
   input/loop path where `collisionSurfaces` is captured (`window/InputFrame.cpp` ~:525, before
   `applyProductGameplayActions`). Call `ensureActiveRoomCollisionFresh(window, activeSession)` there.
3. **Call `ensure` at the TapeRunner read seam** — before each step's collision read (`TapeRunner.cpp` ~:209,
   the `currentCollisionSurfaces` derivation). **Leave the existing `refreshActiveRoomCollision:493` in place**
   (its removal is G5); the ensure call is additive here.
4. **Do NOT remove any direct bake** and **do NOT delete** the Controller mid-tick branch
   (`Controller.cpp:2236-2241`) — those are G5. The direct writer-bakes remain; `ensure` runs first at the
   boundary and finds them fresh (or rebakes their unstamped `(0,…)` blob once — §7).
5. **Tests** (preflight §10):
   - **T-order (headline):** one frame, two ticked commands each flipping a distinct door — an `Attack`-class
     (`Controller.cpp:2244`, no rebake today) + an `Interact`. Run `[Attack, Interact]` and `[Interact, Attack]`
     from identical baselines; assert reader-visible `window.activeRoomCollision` (`querySurfaceCount`,
     `activeDoorBlockerSurfaceCount`, surfaces size) is **byte-identical across orderings** after the boundary
     `ensure`. This proves the order-dependence is removed.
   - **T-tape:** a TapeRunner run with a `Wait`/no-op-tick step (exercises the `skipped_fresh` path) and a
     door-toggle step (rebake path); assert reader-visible surfaces identical to the pre-slice
     unconditional-rebake baseline.
6. **C2 — I7 re-audit** (grep `entity.active`/`setActive` outside the hashed session path; must stay empty) and
   **C3 — checklist reconciliation**; record both in the brief.

## Acceptance Notes

- Every `activeRoom` writer bumps the revision; `ensure` is called at the frame boundary + TapeRunner seam.
- **T-order green** (byte-identical across command orderings) — the order-dependence is gone.
- **T-tape green** (skip + rebake paths both match the pre-slice baseline).
- Full suite green, **incl. `product_receipt_key_order_tests` + `product_creative_no_window_bake_scenario_tests`
  verbatim** and the G3 stale-case tests. `git diff --check` clean.
- **No direct rebake removed, no mid-tick branch deleted** (a grep for `buildProductActiveRoomCollision(` still
  shows the §A/§B/§C sites — those are G5).
- C2 I7 re-audit + C3 reconciliation recorded.

## Do Not

- Do NOT remove/modify any direct rebake callsite or delete the Controller mid-tick branch (G5).
- Do NOT change collision math or overload semantics. Do NOT add fields to save/hash. Do NOT name anything `Kernel`.
- Do NOT widen scope beyond wiring the seam + the two tests.
- Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build -j10
ctest --test-dir /Users/kogaryu/iggy3d/build
grep -rn "buildProductActiveRoomCollision(" --include='*.cpp' /Users/kogaryu/iggy3d/src | grep -v test   # G5 sites still present
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- Writer bump sites wired (list vs checklist §A):
- Frame-boundary + TapeRunner seam locations:
- T-order result (byte-identical across orderings?):
- T-tape result (skip + rebake paths):
- Suite + anchors verbatim:
- C2 I7 re-audit / C3 reconciliation:
- Concerns/deferred:
