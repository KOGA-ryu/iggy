# E129 (E-ARCF-G3): ActiveRoomCollisionFreshnessStore — real dirty-check + stale-case tests (still NO consumer)

## Objective

Gate 3 of the activeRoom→collision freshness guard. Replace the G2 stub with the **real dirty-check +
provenance stamp** in `ensureActiveRoomCollisionFresh`, and add the three stale-case regression tests
(T-a/T-b/T-c) + idempotence — all **driven directly by tests** that build stale state by hand.
**No production writer bumps and no production reader ensures** — that is G4. Production behavior stays
byte-identical (the verb exists and works, but nothing in production calls it yet).

**Gated spine work.** Owner design: `docs/active_room_collision_freshness_preflight_v0_2.md`. Read §5
(invariants I1–I7), **§8 (the decision order)**, §10 (the tests), and Gate-1 conditions C1–C3 before starting.

## Why This Exists

G2 laid the types + a no-op stub. G3 makes `ensure` actually *decide* freshness and rebake+stamp, **proven
by the three stale cases**, before any consumer is wired (G4). One gate per slice, per the core-spine ladder
(`docs/core_spine_work_rules.md:80-90`).

## Required Work

1. **Implement `ensureActiveRoomCollisionFresh(window, session)`** in
   `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp` per preflight §8 decision order:
   - `effectiveSessionHash = session ? session->state().currentStateHash : 0`.
   - **(1) fresh:** if `collision.bakedFromRoomRevision == window.activeRoomRevision && collision.bakedFromSessionHash == effectiveSessionHash` → **no-op**, `rebaked=false`, `reasonCode="skipped_fresh"`.
   - **(2) stale → rebake + stamp:** call `buildProductActiveRoomCollision(window.activeRoom, session->state())`
     (two-arg overload iff `session != nullptr`, else the one-arg overload). Then stamp
     `bakedFromRoomRevision = window.activeRoomRevision`, `bakedFromSessionHash = effectiveSessionHash`.
   - **Classify `reasonCode`** (after a rebake): `!activeRoom.loaded` → `rebaked_unloaded`; rebaked blob not
     ready / no surfaces → `rebaked_empty`; else room-half mismatch only → `rebaked_room`, session-half only →
     `rebaked_session`, both → `rebaked_both`. (The build path already yields the unloaded/empty blob shape at
     `ActiveRoomCollision.cpp:90-106` — do not hand-roll it.)
   - Set `result.observedRoomRevision`, `observedSessionHash`, `rebaked`, `reasonCode`.
   - **Delete** the `"g2_stub_no_rebake"` string. Include `ActiveRoomCollision.hpp` + the Session/SessionState
     header as needed.
2. **Tests — driven directly (build stale state by hand; NO production wiring).** In
   `tests/unit/product_active_room_collision_tests.cpp` (or a focused sibling), using the no-window fixtures:
   - **T-a (room replaced):** floor+crate baseline (`querySurfaceCount==3`, provenance equal). Overwrite
     `window.activeRoom` with floor-only + `bumpActiveRoomRevision(window)`, **without** touching collision.
     Assert before-mismatch; call `ensure`; assert `querySurfaceCount==1`, `roomId` updated, provenance equal,
     `productActiveRoomCollisionSurfaces(collision)->size()==1`.
   - **T-b (baked without runtime session → door not filtered):** surface mapping a `Door{active=true}`.
     Baseline two-arg. Open the door via a **ticked non-Interact path** (bumps `currentStateHash`); `ensure` →
     `runtimeFilteredSurfaceCount==1`, `activeDoorBlockerSurfaceCount==0`, `querySurfaceCount` drops by 1,
     `bakedFromSessionHash==currentStateHash`. Assert the **two-arg** overload was chosen (`runtimeOwnedSurfaceCount>0`).
   - **T-c (clear leaves stale-ready):** loaded baseline. Set `window.activeRoom={}` + `bumpActiveRoomRevision(window)`
     **without** the paired collision clear. `ensure` → `ready==false`, `status=="active_room_collision_unavailable"`,
     `reasonCode=="rebaked_unloaded"`, `querySurfaceCount==0`, `productActiveRoomCollisionSurfaces()==nullptr`.
   - **Idempotence (I5):** a second `ensure` immediately after → `rebaked==false`, all counts byte-identical.
3. **C2 — I7 re-audit:** grep for `entity.active` / `setActive` writes outside the hashed session command path;
   confirm empty (as at ratification). Record the grep + result in the completion brief.
4. **C3 — checklist reconciliation:** confirm `docs/active_room_collision_rebake_removal_checklist.md` still
   matches current `src` (re-run its greps). Remove nothing.

## Acceptance Notes

- `ensure` implements the §8 decision order; the `g2_stub_no_rebake` string is gone.
- **T-a / T-b / T-c + idempotence green.**
- **STILL zero production consumers:** `grep` confirms zero non-test call sites of `bumpActiveRoomRevision` and
  `ensureActiveRoomCollisionFresh`. G3 wires nothing in production.
- Full suite green and **UNCHANGED** — `product_receipt_key_order_tests` and
  `product_creative_no_window_bake_scenario_tests` pass **verbatim** (production behavior byte-identical: the
  verb works but nothing calls it).
- I7 re-audit recorded (C2); checklist reconciled (C3).

## Do Not (scope firewall)

- Do NOT call `bumpActiveRoomRevision` / `ensureActiveRoomCollisionFresh` from any production writer/reader (G4).
- Do NOT remove or modify any direct rebake callsite; the checklist is read-only here (G4/G5).
- Do NOT touch `Controller.cpp:2236-2241` or `TapeRunner.cpp:493` (G5).
- Do NOT add fields to `SaveCodec`/`SaveEnvelope`/`StateHash` (§7). Do NOT rename any
  `buildProductActiveRoomCollision` overload. Do NOT name anything `Kernel`.
- Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests product_active_room_state_tests product_receipt_key_order_tests product_creative_no_window_bake_scenario_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build
grep -rn "bumpActiveRoomRevision\|ensureActiveRoomCollisionFresh" /Users/kogaryu/iggy3d/src | grep -v test   # expect: only definitions
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- `ensure` decision order implemented (reasonCode paths):
- T-a / T-b / T-c + idempotence results:
- "Still no consumer" grep result:
- C2 I7 re-audit (grep + result):
- C3 checklist reconciliation (drift?):
- Tests/checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp`
  - `tests/unit/product_active_room_collision_tests.cpp`
  - `docs/active_room_collision_rebake_removal_checklist.md`
- `ensure` decision order implemented:
  - Computes `effectiveSessionHash` from `session->state().currentStateHash`, or `0` for `nullptr`.
  - Fresh provenance match returns no-op with `rebaked=false` and `reasonCode="skipped_fresh"`.
  - Stale provenance rebuilds through `buildProductActiveRoomCollision(window.activeRoom, session->state())` when a session is present, otherwise the one-arg overload.
  - Rebuilt collision is stamped with `bakedFromRoomRevision=window.activeRoomRevision` and `bakedFromSessionHash=effectiveSessionHash`.
  - Rebuild result codes are `rebaked_unloaded`, `rebaked_empty`, `rebaked_room`, `rebaked_session`, or `rebaked_both`.
  - Removed the transitional `g2_stub_no_rebake` string from source/tests.
- T-a / T-b / T-c + idempotence results:
  - T-a: stale floor+crate collision over replaced floor-only room rebakes to `querySurfaceCount==1`, refreshed `roomId`, matching provenance, and a one-surface reader pointer.
  - T-b: session hash mismatch after a non-interact tick with the door inactive rebakes through the two-arg overload, filters the runtime-owned door blocker, drops query count from `2` to `1`, and stamps the new session hash.
  - T-c: stale ready collision over cleared `activeRoom` rebakes to the unloaded collision blob, `ready=false`, status `active_room_collision_unavailable`, `querySurfaceCount==0`, and no reader surface pointer.
  - Immediate second `ensure` after each stale rebake returns `rebaked=false`, `reasonCode="skipped_fresh"`, and leaves counts/provenance unchanged.
- "Still no consumer" grep result:
  - `rg -n "bumpActiveRoomRevision\(|ensureActiveRoomCollisionFresh\(" /Users/kogaryu/iggy3d/src --glob '*.cpp' --glob '*.hpp' | rg -v 'ActiveRoomState\.(cpp|hpp)|ActiveRoomCollisionFreshnessStore\.(cpp|hpp)'`
  - Result: no output. G3 adds no production writer bumps and no production reader ensures.
  - Broader source/test grep finds only definitions/declarations plus the focused tests.
- C2 I7 re-audit:
  - `rg -n "setActive\(" /Users/kogaryu/iggy3d/src/app /Users/kogaryu/iggy3d/src/runtime | rg -v "creative|ActiveTool"`
  - Result: `InteractionSystem.cpp` interaction-command calls plus `WorldState` declaration/definition.
  - `rg -n "entity\.active\s*=" /Users/kogaryu/iggy3d/src/app /Users/kogaryu/iggy3d/src/runtime`
  - Result: seed/load/binding construction assignments in `Session.cpp`, `SaveLoad.cpp`, and `RoomMarkerBinding.cpp`.
  - No new production bypass of hashed session command/tick path was introduced.
- C3 checklist reconciliation:
  - Re-ran the three checklist greps for `buildProductActiveRoomCollision(...)`, `activeRoomCollision =`, and `refreshActiveRoomCollision`.
  - The only intentional drift is the new Store-owned `buildProductActiveRoomCollision(...)` callsite; the checklist now records it under definitions/final seam to keep/exclude from G5 removal.
  - No direct rebake callsites were removed or modified.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target product_active_room_collision_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_active_room_collision_tests$' --output-on-failure`
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_collision_tests product_active_room_state_tests product_receipt_key_order_tests product_creative_no_window_bake_scenario_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_active_room_collision_tests|product_active_room_state_tests|product_receipt_key_order_tests|product_creative_no_window_bake_scenario_tests)$' --output-on-failure`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` (`100% tests passed, 0 tests failed out of 259`)
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched source/test/docs/card files.
- Concerns/deferred:
  - G3 still wires no production consumer. Writers do not bump and readers do not call `ensure` until later gates.
  - Direct writer-bakes remain for staged install correctness until G4/G5.
  - The T-b test uses a hand-built runtime room/session fixture and a non-interact session tick to exercise the session-hash stale branch without adding production routing.
