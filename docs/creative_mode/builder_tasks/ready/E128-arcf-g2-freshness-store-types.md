# E128 (E-ARCF-G2): ActiveRoomCollisionFreshnessStore — types + window revision counter (NO consumer)

## Objective

Gate 2 of the activeRoom→collision freshness guard (the kernel on-ramp, complexity-audit finding #4).
Introduce the freshness **types only**: a window-owned revision counter, collision provenance fields, a bump
helper, and an `ensure` **stub**. After this card the types exist and default to 0, the two functions exist,
but **NO writer bumps and NO reader ensures** — existing behavior byte-identical.

**Gated spine work.** Owner design: `docs/active_room_collision_freshness_preflight_v0_2.md` (Gate-1 ratified
2026-07-07). Read §3, §5-I2/I3, and the Gate-1 conditions C1–C3 before starting.

## Why This Exists

`window.activeRoomCollision` is rebuilt at ~15 sites from a ~12-writer input with no revision/dirty guard, so
correctness rests on writers never forgetting to rebake (preflight §1). This slice lays the minimal token that
lets one frame-boundary `ensureActiveRoomCollisionFresh` detect staleness. Per the core-spine review-gate
ladder (`docs/core_spine_work_rules.md:80-90`) it lands as small gated slices; G2 is "compiles, no consumer."

## Required Work

1. `src/app/iggy3d/ProductAppWindowState.hpp` — add near the activeRoom/activeRoomCollision members:
   `std::uint64_t activeRoomRevision = 0;  // monotonic room generation; window-owned (preflight §3/§5-I2)`
   (window-owned so whole-struct `activeRoom` copies cannot stomp it — C1/I2.)
2. `src/app/iggy3d/gameplay/ActiveRoomState.hpp` / `.cpp` — declare + define:
   `void bumpActiveRoomRevision(ProductAppWindowState& window);` → `{ ++window.activeRoomRevision; }`
   (the ONLY sanctioned mutator, §5-I3; forward-declare `ProductAppWindowState`.)
3. `src/app/iggy3d/gameplay/ActiveRoomCollision.hpp` — add to `ProductActiveRoomCollisionState`
   (after `activeDoorBlockerSurfaceCount`, before `surfaces`):
   `std::uint64_t bakedFromRoomRevision = 0;` and `std::uint64_t bakedFromSessionHash = 0;` (provenance, §3).
4. NEW `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp` (**name per C1 — `...Store`, not `Kernel`**):
   - `struct ProductActiveRoomCollisionFreshnessResult { bool rebaked = false; std::uint64_t observedRoomRevision = 0; std::uint64_t observedSessionHash = 0; std::string reasonCode = "skipped_fresh"; };`
   - Declare `ProductActiveRoomCollisionFreshnessResult ensureActiveRoomCollisionFresh(ProductAppWindowState& window, const Session* session);`
     (forward-declare `ProductAppWindowState` + `Session`; include `<cstdint>`, `<string>`.)
5. NEW `src/app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.cpp` — **STUB body** (no dirty-check — that
   is G3). Observe only; do NOT rebake or stamp:
   ```cpp
   result.observedRoomRevision = window.activeRoomRevision;
   result.observedSessionHash  = session ? session->state().currentStateHash : 0;
   result.reasonCode = "g2_stub_no_rebake";
   return result;  // window.activeRoomCollision untouched
   ```
   Add the `.cpp` to the app target in `CMakeLists.txt` (mirror `ActiveRoomCollision.cpp`'s entry).
6. **Condition C3 — removal-checklist reconciliation (this card, no removal):** re-run the three greps in
   `docs/active_room_collision_rebake_removal_checklist.md` against current `src`, and if any callsite drifted
   from the checklist, update that doc. Do **not** remove or modify any rebake callsite (that is G4/G5).
7. **Condition C2 — I7 re-audit:** grep for `entity.active`/`setActive` writes outside the hashed session
   command path; confirm the set is empty (as at ratification). Record the grep + result in the completion
   brief. (Trivial at G2 — this card touches no door state — but the audit runs every gate until I7 is
   structurally enforced.)

## Acceptance Notes

- Full build compiles (app + tests).
- **ENTIRE existing suite green and UNCHANGED** — `product_receipt_key_order_tests` and
  `product_creative_no_window_bake_scenario_tests` pass verbatim.
- New type-level tests green (below).
- `grep` confirms **zero** call sites of `bumpActiveRoomRevision` and **zero** non-test call sites of
  `ensureActiveRoomCollisionFresh` — proves "no consumer" (the whole point of Gate 2).
- Removal checklist reconciled (C3); I7 re-audit recorded (C2).

## Tests (G2 — type-level only)

- `tests/unit/product_active_room_state_tests.cpp`: default `window.activeRoomRevision == 0`;
  `bumpActiveRoomRevision` advances by exactly 1 per call; three calls → 3.
- `tests/unit/product_active_room_collision_tests.cpp`: default `bakedFromRoomRevision == 0 && bakedFromSessionHash == 0`.
- ensure-stub test: default window, `session == nullptr` → `rebaked == false`, `observedSessionHash == 0`,
  `reasonCode == "g2_stub_no_rebake"`, `querySurfaceCount` unchanged.

## Do Not (scope firewall — later gates)

- Do NOT call `bumpActiveRoomRevision` from any writer (G4).
- Do NOT call `ensureActiveRoomCollisionFresh` from any reader / frame boundary (G4).
- Do NOT touch `Controller.cpp:2236-2241` or `TapeRunner.cpp:493` (G5).
- Do NOT implement the dirty-check / rebake / stamp logic in `ensure` (G3).
- Do NOT remove or modify any direct rebake callsite (G4/G5 — checklist is read-only here).
- Do NOT add fields to `SaveCodec`/`SaveEnvelope`/`StateHash` (runtime-only — preflight §7).
- Do NOT rename or change any `buildProductActiveRoomCollision` overload.
- Do NOT name anything `Kernel` (C1).
- Do NOT stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_active_room_state_tests product_active_room_collision_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build
grep -rn "bumpActiveRoomRevision\|ensureActiveRoomCollisionFresh" /Users/kogaryu/iggy3d/src | grep -v test   # expect: only the definitions
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Files changed:
- Types added (fields + functions):
- Stub behavior confirmed (no rebake, no stamp):
- "No consumer" grep result:
- C3 removal-checklist reconciliation (drift found?):
- C2 I7 re-audit (grep + result):
- Tests/checks run:
- Concerns/deferred:
