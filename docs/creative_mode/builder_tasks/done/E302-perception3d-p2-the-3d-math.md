# E302 — Perception 3D · P2: the 3D math (Mode C — FIRST behavior-changing slice)

**Released to `ready/`** (tree clean, lane empty). Parent: `docs/perception-3d-maximum.md` **v1.3** (§4/§5,
v1.1–v1.3 corrections govern). Depends: E301 (landed `d4d807ea`). **Commit:** `claude: planned. codex: …`.

## Goal (one line)

`queryNpcPerception` computes real 3D perception — 3D distance, horizontal + vertical cones, eye-to-eye
geometry, tri-state LOS input — and the FSM gains **exactly one new gate: verticality.** Flat-ground behavior
is byte-identical.

## METRIC PROMISE

- **Planner-authored pin-tests below land verbatim and pass** (semantic content untouched; only mechanical
  adaptation to file-local names permitted). Suite green, test count UP.
- **Golden byte-identical** (all default fixtures are flat; flat behavior must not move).
- Existing-files-edited ≤ **5**: `NpcBehaviorSystem.{hpp,cpp}`, `Session.cpp`,
  `npc_behavior_system_tests.cpp`, `stealth_garden_tests.cpp`. New files 0.

## STOP CONDITIONS

- The consumption sites are NOT reducible to the two known conjunctions (`Session.cpp:715`, `:1157`
  `targetInVisionCone && hasLineOfSight`) — i.e. other combiners of perception booleans exist.
- The `targetHasLineOfSight` → `targetLos` type change ripples beyond ~6 sites.
- Any *flat-geometry* test changes value (that means the math changed more than the vertical axis — wrong).

## The changes

**1. Request (`NpcBehaviorSystem.hpp:69` area):** replace `bool targetHasLineOfSight = true` with
`NpcPerceptionResult::Los targetLos = NpcPerceptionResult::Los::Clear;` — same documented assume-clear
contract, now tri-state. Update the 3 setters/copies (`Session.cpp:1143` maps its existing raycast bool →
`Clear/Blocked`; kernel copy; the one test). **`Unknown` is never produced in P2** — that's P3.

**2. The kernel (`NpcBehaviorSystem.cpp`, replacing the internals of `queryNpcPerception` + `targetWithinVisionCone`):**
per maximum §4 with the v1.1/v1.3 corrections —
- **Eye points:** `guardEye = actorPos + (0, config.guardEyeHeightMeters, 0)`;
  `targetEye = targetPos + (0, config.targetStandEyeHeightMeters, 0)` (stance socket = TODO comment, stand only).
  `delta = targetEye - guardEye`; `dist = length(delta)` (**full 3D**) → `result.distanceMeters`,
  `targetInPerceptionRadius = dist <= perceptionRadiusMeters`.
- **Degenerate `dist < 1e-4`:** perceived true, `los = Clear`, angles 0, all gates true.
- **Horizontal cone:** flatten facing + delta to XZ. **Zero/degenerate facing → BOTH cones pass
  (omnidirectional, the documented contract — v1.1 §3).** Zero XZ-delta (directly above/below) →
  `horizontalAngleDeg = 0`, `inHorizontalCone = true`. Else angle via `acos(clamp(dot,-1,1))`, inclusive `<=`.
  Populate `horizontalAngleDeg` always.
- **Vertical cone:** `verticalAngleDeg = deg(asin(clamp(delta.y / dist, -1, 1)))`;
  `inVerticalCone = |verticalAngleDeg| <= verticalHalfAngleDegrees`, inclusive. Omnidirectional-facing bypass
  applies here too.
- **Populate:** `los = request.targetLos` (in-cone or not — it's an input in P2);
  `hasLineOfSight = (los == Clear)` (kept as a derived alias for the debug mirror `Session.cpp:1226` until P4);
  `perceived = targetInPerceptionRadius && targetInVisionCone && inVerticalCone && los == Clear` (the §2
  invariant, always asserted by tests).
- `horizontalDistanceMeters` **stays** — the home-leash predicates (`:97-146`) keep it; perception stops using it.

**3. FSM wiring (`Session.cpp:715` and `:1157`) — behavior-preserving + one new gate:** replace
`perception.targetInVisionCone && perception.hasLineOfSight` with
`perception.targetInVisionCone && perception.inVerticalCone && perception.hasLineOfSight` at BOTH sites.
**Do NOT substitute `.perceived`** (it adds a radius term the current truth table doesn't have — that's a
pending design ruling, maximum v1.3 note).

**4. Integration pin (`stealth_garden_tests.cpp`):** existing garden cases must pass **unchanged** (flat, walls
span eye height). ADD the money case: thief on a ledge **3m directly above** a guard whose target is 2–4m away
horizontally (vert angle > 30°) → **not visually confirmed**; same thief at ground level, same XZ → confirmed.

## Planner-authored pin-tests (LAND VERBATIM — the implementer does not edit the exam)

Add to `tests/unit/npc_behavior_system_tests.cpp` (uses the file's `expect`/`near`/`entity`/fixture style;
`Kind` constants and combat fixture as in the file's existing perception tests; register each fn in `main`):

```cpp
// P2 fixture: guard at origin facing +Z; target at (x, y, z), both default stand eye height.
iggy3d::NpcPerceptionResult perceive3D(float tx, float ty, float tz,
                                       iggy3d::Vec3 facing = {0.0F, 0.0F, 1.0F}) {
  iggy3d::WorldState world;
  auto guard = entity({1}, "npc", /*file's npc kind*/ ..., 0.0F, 0.0F);
  auto player = entity({2}, "player", /*file's player kind*/ ..., tx, tz);
  player.transform.position.y = ty;
  (void)world.seedEntity(guard);
  (void)world.seedEntity(player);
  iggy3d::CombatState combat;
  iggy3d::NpcBehaviorConfig config;   // radius 6, hCone 60, vCone 30, eyes 1.6/1.6
  iggy3d::NpcPerceptionRequest request{&world, &combat, {1}, {2}, config};
  request.actorFacingDirection = facing;
  return iggy3d::queryNpcPerception(request);
}

bool invariantHolds(const iggy3d::NpcPerceptionResult& r) {
  return r.perceived == (r.targetInPerceptionRadius && r.targetInVisionCone &&
                         r.inVerticalCone &&
                         r.los == iggy3d::NpcPerceptionResult::Los::Clear);
}

bool flatGroundPerceptionUnchanged() {   // the no-regression pin
  const auto r = perceive3D(0.0F, 0.0F, 3.0F);
  return expect(r.perceived, "flat dead-ahead perceived") &&
         expect(r.targetInPerceptionRadius && r.targetInVisionCone && r.inVerticalCone,
                "flat gates all pass") &&
         expect(near(r.verticalAngleDeg, 0.0F), "flat vertical angle ~0") &&
         expect(r.horizontalAngleDeg <= 0.01F, "flat horizontal angle ~0") &&
         expect(invariantHolds(r), "flat invariant");
}

bool ledgeAboveEscapesVerticalCone() {   // THE 2D-cheat pin: dead-ahead but 45 deg up
  const auto r = perceive3D(0.0F, 4.0F, 4.0F);   // atan(4/4)=45 > 30
  return expect(r.targetInVisionCone, "ledge still in horizontal cone") &&
         expect(r.targetInPerceptionRadius, "ledge within 3D radius (5.66m)") &&
         expect(!r.inVerticalCone, "ledge outside vertical cone") &&
         expect(!r.perceived, "ledge NOT perceived") &&
         expect(r.verticalAngleDeg > 44.0F && r.verticalAngleDeg < 46.0F,
                "ledge vertical angle ~45") && expect(invariantHolds(r), "ledge invariant");
}

bool verticalBoundaryIsInclusive() {
  const auto in  = perceive3D(0.0F, 2.28F, 4.0F);   // atan(2.28/4)=29.68 deg
  const auto out = perceive3D(0.0F, 2.35F, 4.0F);   // atan(2.35/4)=30.44 deg
  const auto below = perceive3D(0.0F, -2.28F, 4.0F);
  return expect(in.inVerticalCone && in.perceived, "29.7 deg inside") &&
         expect(!out.inVerticalCone && !out.perceived, "30.4 deg outside") &&
         expect(below.inVerticalCone && below.perceived, "below is symmetric") &&
         expect(invariantHolds(out), "boundary invariant");
}

bool radiusIsThreeDimensional() {   // the 3D-radius pin: horizontally near, far in 3D
  const auto r = perceive3D(0.0F, 5.0F, 3.5F);   // dist = sqrt(25+12.25) = 6.10 > 6
  return expect(!r.targetInPerceptionRadius, "3D distance exceeds radius") &&
         expect(near(r.distanceMeters, 6.1033F) || (r.distanceMeters > 6.09F && r.distanceMeters < 6.12F),
                "distance is 3D") &&
         expect(!r.perceived, "not perceived beyond 3D radius") &&
         expect(invariantHolds(r), "radius invariant");
}

bool directlyOverheadHandlesDegenerateXZ() {
  const auto r = perceive3D(0.0F, 3.0F, 0.0F);   // straight up, vert 90
  return expect(r.targetInVisionCone, "pure-vertical target not horizontally out") &&
         expect(near(r.horizontalAngleDeg, 0.0F), "overhead horizontal angle 0") &&
         expect(!r.inVerticalCone, "overhead outside vertical cone") &&
         expect(r.verticalAngleDeg > 89.0F, "overhead vertical angle ~90") &&
         expect(!r.perceived, "overhead not perceived") && expect(invariantHolds(r), "overhead invariant");
}

bool zeroFacingStaysOmnidirectional() {   // documented contract (v1.1 §3): BOTH cones bypass
  const auto r = perceive3D(0.0F, 2.0F, -3.0F, {0.0F, 0.0F, 0.0F});   // behind AND above
  return expect(r.targetInVisionCone, "zero facing bypasses horizontal cone") &&
         expect(r.inVerticalCone, "zero facing bypasses vertical cone") &&
         expect(r.perceived, "zero facing perceives behind+above target in radius") &&
         expect(invariantHolds(r), "omnidirectional invariant");
}

bool blockedLosSuppressesPerception() {   // tri-state input mapping
  iggy3d::WorldState world;   // same fixture as perceive3D, inline with los override
  auto guard = entity({1}, "npc", ..., 0.0F, 0.0F);
  auto player = entity({2}, "player", ..., 0.0F, 3.0F);
  (void)world.seedEntity(guard); (void)world.seedEntity(player);
  iggy3d::CombatState combat; iggy3d::NpcBehaviorConfig config;
  iggy3d::NpcPerceptionRequest request{&world, &combat, {1}, {2}, config};
  request.actorFacingDirection = {0.0F, 0.0F, 1.0F};
  request.targetLos = iggy3d::NpcPerceptionResult::Los::Blocked;
  const auto r = iggy3d::queryNpcPerception(request);
  return expect(!r.perceived, "blocked los suppresses") &&
         expect(!r.hasLineOfSight, "alias mirrors tri-state") &&
         expect(r.targetInVisionCone && r.inVerticalCone, "gates unaffected by los") &&
         expect(invariantHolds(r), "blocked invariant");
}

bool requestDefaultsAssumeClear() {
  const iggy3d::NpcPerceptionRequest request{};
  return expect(request.targetLos == iggy3d::NpcPerceptionResult::Los::Clear,
                "default los preserves documented assume-clear contract");
}
```
`...` = the file's existing `EntityKind` constants (mechanical adaptation ONLY — values/assertions frozen).
Also update the one existing occluded-bool test (`:224`) to `targetLos = Los::Blocked` with its assertions kept.

## Gates

All-targets build green (incl. `tools/`) · full suite green with the tests above verbatim · golden
byte-identical · flat garden tests unchanged + the ledge case added · maximum §12 P2 flipped on landing.

## NOT in this card

No occlusion computation inside the kernel, no `segmentOcclusion`, no startInside/fail-closed inversions
(P3); no debug drawing (P4); no alert/investigate decay fixes (P5); no `.perceived` substitution at the FSM
sites (pending the radius ruling); no stance wiring (socket).

## Completion Brief

Completed E302 as the first behavior-changing Perception 3D slice.

Files changed:

- `src/runtime/ai/NpcBehaviorSystem.hpp`
- `src/runtime/ai/NpcBehaviorSystem.cpp`
- `src/runtime/session/Session.cpp`
- `tests/unit/npc_behavior_system_tests.cpp`
- `tests/unit/stealth_garden_tests.cpp`
- `docs/perception-3d-maximum.md`
- this task card, moved to `done/`

Implemented:

- Replaced `NpcPerceptionRequest::targetHasLineOfSight` with
  `NpcPerceptionResult::Los targetLos`, defaulting to `Clear`.
- `queryNpcPerception(...)` now computes full 3D eye-to-eye distance using
  stand-eye heights, horizontal angle, vertical angle, horizontal cone,
  vertical cone, tri-state `los`, derived `hasLineOfSight`, and final
  `perceived`.
- Degenerate eye-to-eye distance below `1.0e-4F` returns clear perceived
  sight with zero angles and all gates true.
- Zero/degenerate facing remains omnidirectional and bypasses both cone gates.
- `horizontalDistanceMeters(...)` intentionally remains for home/leash logic,
  but perception no longer uses it for range.
- Session LOS raycast bool now maps to `Los::Clear` or `Los::Blocked`.
- Both FSM visual-confirmation conjunctions now require
  `targetInVisionCone && inVerticalCone && hasLineOfSight`.
- `.perceived` was not substituted at FSM sites, preserving the open radius
  ruling from maximum v1.3.

Tests added/updated:

- Added the planner-authored P2 perception pins in
  `npc_behavior_system_tests.cpp`: flat unchanged, ledge vertical miss,
  vertical boundary, 3D radius, directly overhead, zero-facing
  omnidirectional behavior, blocked LOS suppression, and default assume-clear
  request LOS.
- Updated the existing occluded perception test to use `Los::Blocked`.
- Added the garden ledge integration pin: same clear XZ lane, ground-level
  target visually confirms and raises alert, 3m ledge target does not.

Verification:

- `cmake --build /Users/kogaryu/iggy3d/build --target npc_behavior_system_tests stealth_garden_tests -j10`
  passed.
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(npc_behavior_system_tests|stealth_garden_tests)$' --output-on-failure`
  passed.
- `cmake --build /Users/kogaryu/iggy3d/build --target all -j10` passed,
  including tools.
- `ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure` passed:
  261/261 tests.
- Receipt golden diff remained byte-identical.
- `git diff --check` passed.
- Focused trailing-whitespace scan over touched files and this card passed.

Not performed:

- No occlusion computation, `segmentOcclusion`, start-inside/fail-closed
  inversion, debug drawing, alert/investigate decay fix, `.perceived`
  substitution at FSM sites, stance wiring, fixture/golden edit, broad
  behavior beyond P2, interactive window launch, staging, commit, or push.
