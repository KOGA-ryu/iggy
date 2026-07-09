# Perception 3D — Build Maximum (v1.3)

> **v1.3 corrections (P2 card-cut recon, E302):**
> 4. **`horizontalDistanceMeters` is NOT deletable** (§16 said it dies) — the home-leash predicates
>    (`NpcBehaviorSystem.cpp:97-146`) use it against `homePosition`. Perception stops using it; the fn stays.
> 5. **TWO FSM consumption sites, not one:** `Session.cpp:715` and `:1157`, both `targetInVisionCone &&
>    hasLineOfSight` — note **radius is NOT in the current confirmation truth table.** P2 therefore wires
>    `old-conjunction ∧ inVerticalCone` (flat behavior byte-identical) and does NOT substitute `.perceived`.
>    **OPEN RULING for Law 6:** should visual confirmation also require `targetInPerceptionRadius`? Decide
>    before P3 wires `.perceived` as the sole FSM read.
> 6. The request bool becomes `Los targetLos = Los::Clear` in P2 (documented assume-clear preserved, type
>    upgraded); `Unknown` is produced only from P3's occlusion owner. `result.hasLineOfSight` survives as a
>    derived alias (`los == Clear`) for the debug mirror (`Session.cpp:1226`) until P4.

> **v1.2 (from E301's correctly-invoked STOP):** `NpcBehaviorConfig` is **never serialized** — saves persist
> `behavior_profile_id` only; config is derived from the in-code `NpcBehaviorProfile` catalog via
> `configFromNpcBehaviorProfile`. §3's "serialized wherever NpcBehaviorConfig already is" and §15's "config TOML
> plumbing" are WRONG for this repo: the data layer is the **profile struct + derivation fn** (mirror fields
> there; built-ins inherit member defaults). Bonus finding: `visionHalfAngleDegrees` was never profile-carried —
> P1 heals that. Config-as-derived-truth also means zero save-drift risk for the new fields.

> **v1.1 re-anchor corrections (verified at HEAD before P1 card-cut):**
> 1. **`NpcPerceptionResult` ALREADY EXISTS** (`NpcBehaviorSystem.hpp:72-87`) with `status`, entity/combat
>    fields, `distanceMeters`, `targetInPerceptionRadius`, `targetInVisionCone`, `hasLineOfSight`. §2 is an
>    **extension** of that struct, not a new type: add `horizontalAngleDeg`, `verticalAngleDeg`,
>    `inVerticalCone`, `perceived`, and the `los` tri-state; `targetInVisionCone` keeps its meaning as the
>    *horizontal* cone gate. The request's default-true `targetHasLineOfSight` (`hpp:69`) is the fail-open
>    bool to remove per §16.
> 2. The existing horizontal cone config field is **`visionHalfAngleDegrees`** (`hpp:31`, default 60), not
>    "horizontalHalfAngleDegrees" — keep its name; the new field `verticalHalfAngleDegrees` sits beside it.
> 3. **Zero facing = omnidirectional is a DOCUMENTED contract** (`hpp:62-64`: degenerate facing disables the
>    cone gate for low-level callers). §4's edge table said "zero facing → not in cone" — **wrong; preserve
>    omnidirectional-on-zero-facing** for both cones (vertical gate likewise disabled on zero facing).
> Part A sites re-verified: alert heard-branch `NpcAlertSystem.cpp:164`; dwell-reset
> `NpcInvestigateSystem.cpp:30` ("a fresh sighting restarts any look-around"); occlusion sites exact
> (`Session.cpp:1002/1008/1027/1032/1047/1075/1134`).

The complete demand for the guard 3D-perception build. Every slice is cut from this; cite coordinates,
version-bump on correction. Supersedes the 2D perception in `NpcBehaviorSystem`. Companion:
`docs/stealth_ai_hardening_plan.md` (Parts A/B/C). Verified line refs are HEAD-of-review (`wzcfuhxtf`);
re-anchor at slice time (files churn).

---

## 0. THE ONE PILLAR

**Verticality is a first-class stealth axis.** A guard perceives in 3D; the player escapes by *height* only
when genuinely outside the guard's vertical field, not for free. Every rule below serves that one sentence.
Second pillar: **perception is a single observable truth** — one struct that gameplay decides on, the debug
layer draws, tests pin, and the notebook later reads. Debug can never disagree with the sim because it renders
the sim's own output.

## 1. LAWS (non-negotiable — a slice that breaks one is rejected)

1. **Fail closed for stealth.** A failed / degenerate / unwired occlusion query grants **no** sight. Never
   fabricate visibility from a broken query. (Fixes the current `return true` at `Session.cpp:1027`.)
2. **One occlusion owner.** Exactly one segment-occlusion function; LOS, sound, and reasoning-edge all call it.
   No third eye-height copy. (Consolidates `Session.cpp:1002-1040`, `:1047-1083`, `ReasoningGraph.hpp:69`.)
3. **Debug renders FROM the result struct** — never recomputes perception. If debug and gameplay can disagree,
   the design is wrong.
4. **Determinism.** No wall-clock, no `Math.random`, no frame-rate dependence in the perception math. Given the
   same `(guard, target, config, colliders)` the result is bit-stable. Replay-safe (StateHash lane).
5. **Config is data.** Every threshold (angles, radius, eye heights, margins) lives in `NpcBehaviorConfig`,
   tunable, never a hardcoded literal in the algorithm. (Kills `kEyeHeightMeters=1.0` as a constant.)
6. **The FSM reads only `perceived`.** All other struct fields are observability; alert/investigate logic must
   not branch on raw angles/distance — only on the resolved `perceived` bool + the tri-state `los`.

## 2. THE DATA CONTRACT — `NpcPerceptionResult`

New type (home: `NpcBehaviorSystem.hpp`, beside `NpcPerceptionRequest`). Every field, type, unit, invariant:

```cpp
struct NpcPerceptionResult {
  bool  perceived        = false;   // FINAL verdict: inRadius && inHorizontalCone && inVerticalCone && los==Clear
  float distanceMeters   = 0.0F;    // full 3D length(target - guard); >= 0
  float horizontalAngleDeg = 0.0F;  // [0,180] angle between facing(XZ) and delta(XZ); 0 = dead ahead
  float verticalAngleDeg = 0.0F;    // [-90,90] asin(delta.y / dist); + = target above guard eye
  bool  inRadius         = false;   // distanceMeters <= config.perceptionRadiusMeters
  bool  inHorizontalCone = false;   // horizontalAngleDeg <= config.horizontalHalfAngleDegrees
  bool  inVerticalCone   = false;   // abs(verticalAngleDeg) <= config.verticalHalfAngleDegrees
  enum class Los { Clear, Blocked, Unknown } los = Los::Unknown;  // occlusion; Unknown never grants sight
};
```
- **Invariant:** `perceived == (inRadius && inHorizontalCone && inVerticalCone && los == Los::Clear)`. Assert it.
- **Invariant:** `los == Unknown` ⇒ `perceived == false` (Law 1).
- `distanceMeters == 0` (guard == target) ⇒ `perceived = true, los = Clear, angles = 0` (self/degenerate-safe).
- Replaces the bare `bool NpcPerceptionRequest.targetHasLineOfSight = true` (`NpcBehaviorSystem.hpp:69`) — that
  default-true bool is deleted; occlusion enters as the tri-state below.

## 3. CONFIG — `NpcBehaviorConfig` additions (`NpcBehaviorSystem.hpp:23-35`)

| field | type | default | range | meaning |
|---|---|---|---|---|
| `horizontalHalfAngleDegrees` | float | *existing* | 10–90 | yaw FOV half-angle (already present as the cone) |
| **`verticalHalfAngleDegrees`** | float | **30.0** | 5–89 | NEW: pitch FOV half-angle — the verticality gate |
| `perceptionRadiusMeters` | float | *existing* | >0 | now gates **3D** distance (was horizontal) |
| **`guardEyeHeightMeters`** | float | **1.6** | >0 | NEW: guard eye above feet (was `kEyeHeightMeters=1.0`) |
| **`targetStandEyeHeightMeters`** | float | **1.6** | >0 | NEW: target eye when standing |
| **`targetSneakEyeHeightMeters`** | float | **0.9** | >0 | NEW: target eye when sneaking/crouched |
| **`occlusionMarginMeters`** | float | **0.05** | ≥0 | NEW: shrink toward endpoints so touching-wall ≠ blocked |

Defaults preserve today's feel where possible; a narrow default `verticalHalfAngleDegrees=30` makes "get well
above/below" an escape without making guards blind to stairs. All must be serialized wherever
`NpcBehaviorConfig` already is (scenario TOML / defaults), with **back-compat defaults** so existing scenarios
that omit the new fields behave sanely (not zero).

## 4. THE PERCEPTION ALGORITHM — `queryNpcPerception3D(guard, target, config, colliders) -> NpcPerceptionResult`

Replaces `queryNpcPerception` (`NpcBehaviorSystem.cpp:277-305`) + `targetWithinVisionCone` (`:21-42`) +
`horizontalDistanceMeters` (`:12-16`). Pure function; no Session access (colliders passed in). Steps:

1. `delta = target.position - guard.position` (**full 3D**). `dist = length(delta)`.
2. **Degenerate:** if `dist < kEpsilon` (1e-4) → `{perceived:true, los:Clear, dist:0, angles:0, all cones true}`.
3. `result.distanceMeters = dist; result.inRadius = dist <= config.perceptionRadiusMeters`.
4. **Horizontal cone:** `fh = normalize(facing.xz0)`, `dh = normalize(delta.xz0)`. If `length(dh)==0` (target
   directly above/below) → `horizontalAngleDeg = 0`, `inHorizontalCone = true` (purely vertical target is not
   horizontally out). Else `horizontalAngleDeg = deg(acos(clamp(dot(fh,dh),-1,1)))`;
   `inHorizontalCone = horizontalAngleDeg <= horizontalHalfAngleDegrees`.
5. **Vertical cone:** `verticalAngleDeg = deg(asin(clamp(delta.y / dist, -1, 1)))`;
   `inVerticalCone = abs(verticalAngleDeg) <= verticalHalfAngleDegrees`.
6. **Early-out:** if `!(inRadius && inHorizontalCone && inVerticalCone)` → `los = Unknown` **is wrong here**;
   set `los = Blocked`? No — set `los` = **not evaluated** but `perceived=false`. Convention: if outside
   cone/radius, **skip the raycast** (perf) and leave `los = Unknown`, `perceived = false`. Debug shows grey.
7. **LOS (only if in cone+radius):** `guardEye = guard.pos + (0,guardEyeHeight,0)`;
   `targetEye = target.pos + (0, targetEyeHeightFor(target.stance), 0)`; call the **one** occlusion owner (§6):
   `los = segmentOcclusion(guardEye, targetEye, colliders, occlusionMargin)` → `Clear|Blocked` (never Unknown on
   a successful query; `Unknown` only if the query itself fails → Law 1 keeps `perceived=false`).
8. `result.perceived = inRadius && inHorizontalCone && inVerticalCone && los == Clear`. Assert §2 invariant.
9. Return. **No mutation of guard/target/session** — pure.

**Edge-case table (all must be handled + tested):** target exactly on horizontal boundary; on vertical
boundary; directly overhead (`delta.xz==0`); directly below; at `dist==0`; at `dist==radius` (inclusive);
behind guard (`horizontalAngleDeg>90`); target inside a wall; guard eye inside a wall (§6 A1); zero-length
facing (guard undefined heading → treat as not-facing, `inHorizontalCone=false`); NaN guard (clamp all
acos/asin inputs — never emit NaN).

## 5. EYE-HEIGHT MODEL

- `targetEyeHeightFor(stance)` = `sneak ? targetSneakEyeHeightMeters : targetStandEyeHeightMeters`. Stance comes
  from the movement lane's `MovementMode::Sneak` / the proof packet's `sneaking` field (MA1 sneak). If unavailable
  this build, default stand; leave a `// TODO wire stance` seam — do NOT hardcode.
- Guard eye = `guardEyeHeightMeters`. Both endpoints raised before the ray (replaces the shared 1.0 constant at
  `Session.cpp:1008-1012`).
- Consequence made real: short cover (< target eye) occludes a standing target; a sneaking target behind
  head-high-only cover is hidden — the crouch mechanic finally interacts with LOS.

## 6. LOS / OCCLUSION — THE SINGLE OWNER (folds in Part A A1/A2/A5)

`enum Los; Los segmentOcclusion(Vec3 a, Vec3 b, span<PhysicsAabbCollider> colliders, float margin)`:
- Shrink the segment by `margin` at each end (touching-wall ≠ blocked).
- Raycast `a→b`; **startInside counts as Blocked** (Law: you're inside geometry — fixes `Session.cpp:1032`
  `if (hit.startInside) continue;` and the `Aabb3.cpp:67/102/105` startInside clamp). Any hit strictly before
  `b` (beyond margin) → `Blocked`. No hit → `Clear`. Query failure (`!result.ok`, invalid collider set) →
  `Unknown` (caller treats as no-sight, Law 1).
- **All three callers migrate to this one function:** vision LOS (`actorHasLineOfSightToTarget`), sound
  (`hasBlockerBetween`, `Session.cpp:1047-1083`), reasoning edges (`reasoningSegmentBlocked`,
  `ReasoningGraph.hpp:69`). Delete the two duplicates.
- **Sound parallel-array guard:** `NpcSoundPerception.cpp:45` `blockers[i]` — add a length assert / treat
  missing as **Blocked** (fail-closed), not "no wall."

## 7. FSM INTEGRATION

- `queryNpcPerception3D` result's `.perceived` feeds the alert stimulus exactly where the old bool did
  (`Session.cpp` stimulus wiring ~1133-1157). No FSM logic changes shape (Law 6).
- **But fold in Part A while here** (shared eye-height path): A3 (`NpcAlertSystem.cpp:163` heard-branch must
  decay), A4 (`NpcInvestigateSystem.cpp:30` dwell must be able to elapse under sustained noise). These are
  separate bugs but share the "sustained stimulus" test fixtures, so co-locate.
- `los` tri-state is available to the FSM/notebook to distinguish *confirmed* sight from *unverified* — used by
  the notebook packet later; this build just carries it, doesn't branch on it beyond `perceived`.

## 8. THE DEBUG LAYER — every element

Home: extend `NpcBehaviorDebugHudState` + the debug draw-list (PrimitiveDrawList / render bridge overlay path),
behind a dev-tools toggle (`FrontendScreen::DevOverlay` / a `productDrawPerceptionDebug` viewport flag). **Renders
purely from the per-guard `NpcPerceptionResult` list the sim already produced (Law 3).**

- **Vision frustum (per guard):** wireframe cone/pyramid from `guardEye` along `facing`, apex at eye, extent =
  `perceptionRadiusMeters`, opening = `horizontalHalfAngleDegrees` (yaw) × `verticalHalfAngleDegrees` (pitch).
  Draw as a 4-edge pyramid frustum (or an 8-segment cone ring at the far radius). The **vertical slice is the
  point** — you see the up/down blind cones. Color: faint guard-team color.
- **LOS ray (per guard×perceived-candidate target):** line `guardEye → targetEye`, colored by the result —
  **green** `perceived` (in cone + `los==Clear`), **yellow** in cone but `los==Blocked`, **grey**
  out-of-cone/out-of-radius (`los==Unknown`, ray skipped). Instantly makes see-through-walls (green through a
  wall = A1 bug) and vertical-miss visible.
- **Readout (per guard):** small text block near the guard — `d=<dist>m  h=<horiz°>/<H°>  v=<vert°>/<V°>
  los=<Clear|Blocked|Unknown>  =><PERCEIVED|—>`. Explains *why*, not just the verdict (`v 42°>30° → out`).
- **Eye markers:** small cross at `guardEye` and each `targetEye` (shows the height model — a low target eye
  behind cover visibly ducks under the ray).
- Toggle granularity: cones on/off, rays on/off, readout on/off (three flags) so it's usable in a busy scene.
- Receipt: emit a compact per-guard perception summary (`perceived`, `los`, `horiz`, `vert`, `dist`) into the
  receipt so the **key-order golden oracle** pins the observable — debug + receipt + tests read one struct.

## 9. DETERMINISM & RECEIPTS

- All angle math uses `clamp` before `acos/asin`; fixed float formatting via `floatReceiptValue` for any emitted
  value. No branch on wall-clock or frame index.
- Perception summary joins the receipt stream (new append-only keys, e.g. `npc_perception_<slot>_perceived`,
  `..._los`, `..._vertical_angle`) — **append-only, at the section's existing order**, so the golden regen is a
  controlled one-time add, not a reorder. After that, byte-identical forever (a diff = a real behavior change).

## 10. TEST MATRIX (complete — the risk-coverage gap this closes)

Unit (`npc_behavior_system_tests` + a new `npc_perception_3d_tests`), each a pinned assertion on
`NpcPerceptionResult` fields (not just `perceived`):
- **Horizontal cone:** dead-ahead in; at boundary in (inclusive); just past boundary out; behind out.
- **Vertical cone:** level in; at +V boundary in; at −V boundary in; above +V out; below −V out; directly
  overhead → `inHorizontalCone` true, vertical governs.
- **Radius:** inside in; at radius inclusive; beyond out; far-above out (proves 3D radius, was the 2D bug).
- **Occlusion:** clear→Clear→perceived; wall spanning eye→Blocked→not perceived; short wall below target
  eye→Clear (does NOT occlude standing) but Blocked for a sneaking (low-eye) target; guard-inside-wall
  (startInside)→Blocked (A1); failed/empty-collider query→Unknown→not perceived (A2 fail-closed); margin
  boundary (touching wall)→Clear.
- **Degenerate:** dist==0→perceived; zero facing→not in cone; NaN-guarded inputs→no NaN out.
- **Invariant test:** `perceived == (inRadius&&hCone&&vCone&&los==Clear)` across a fuzz of random poses.
- **FSM (Part A, co-located):** sustained noise → alert rises then decays (A3); sustained noise → investigate
  gives up within bound (A4).
- **Integration:** the `stealth_garden` end-to-end fixture still green (or updated once, deliberately, with the
  new 3D behavior noted) — the garden wall is y=3m so it still spans; add a *vertical* garden case (thief on a
  3m ledge above the guard) asserting NOT perceived at `vert>V`.

## 11. MIGRATION (2D → 3D without silent breakage)

- The garden integration test currently passes because its wall spans the flat 1m ray; with per-entity eye
  heights the ray geometry shifts — **expect that fixture to need a deliberate re-pin**, reviewed, not
  auto-regenerated. Any receipt-golden change is the controlled append in §9 only.
- Existing scenarios lacking the new config fields must default (not zero) — a `verticalHalfAngleDegrees=0`
  would blind the guard vertically to everything; the default `30` (§3) is load-bearing.
- Keep `queryNpcPerception3D` a pure drop-in behind the same call site so the diff is contained.

## 12. SLICE / BUILD ORDER (cut cards from here)

1. **P1 — Contract + config. ✅ DONE (E301, d4d807ea)** Add `NpcPerceptionResult`, the 5 new `NpcBehaviorConfig` fields + defaults +
   serialization/back-compat. No behavior change yet (result unused). Gate: builds, config round-trips.
2. **P2 — The 3D function.** Implement `queryNpcPerception3D` (§4) + eye-height model (§5); wire `.perceived`
   into the FSM at the old call site; delete the default-true LOS bool. Gate: full §10 unit matrix (cones,
   radius, degenerate) green; garden re-pinned.
3. **P3 — Single occlusion owner + A1/A2.** `segmentOcclusion` (§6); migrate LOS/sound/reasoning callers;
   startInside=Blocked; fail-closed tri-state; sound length-guard. Gate: occlusion tests + no through-wall.
4. **P4 — Debug layer.** Frustum + colored LOS rays + readout + eye markers + toggles + receipt summary (§8/§9).
   Gate: debug renders from the struct; receipt golden append reviewed; visually confirm a vertical miss.
5. **P5 — FSM decay fixes (Part A A3/A4).** Co-located; the sustained-stimulus tests. Gate: §10 FSM tests.
6. *(Later, Part C)* — emit + consume `chokepoint/high_ground/hiding_spot` on top of correct 3D perception.

## 13. RESERVED SOCKETS (do not build now; leave the seam)

- **Stance wiring** (§5) — reads the movement proof `sneaking`; seam left in P2.
- **Notebook packet** reads the `los` tri-state + `perceived` (confirmed vs unverified intel) — the struct
  already carries it.
- **Peripheral vision / awareness falloff** — a soft inner cone with reduced detection; the two-angle model
  extends to a second (wider, slower) cone without reshaping the contract.
- **Hearing cone/3D** — sound perception now shares `segmentOcclusion`; a 3D audible-range is a config add.
- **Multi-guard / shared alert** — per-guard `NpcPerceptionResult` list already supports N guards; debug scales.

## 14. FILE-TOUCH MAP (re-anchor at slice time)

| file | change |
|---|---|
| `src/runtime/ai/NpcBehaviorSystem.hpp` | `NpcPerceptionResult` struct; 5 config fields; delete default-true LOS bool (`:69`) |
| `src/runtime/ai/NpcBehaviorSystem.cpp` | replace `:12-16,:21-42,:277-305` with `queryNpcPerception3D` + eye-height helpers |
| `src/runtime/session/Session.cpp` | call the new fn; `segmentOcclusion` owner replaces `:1002-1040` + `:1047-1083`; startInside/fail-closed |
| `src/runtime/ai/ReasoningGraph.hpp/.cpp` | `reasoningSegmentBlocked` → call `segmentOcclusion` |
| `src/runtime/ai/NpcSoundPerception.cpp` | `:45` length-guard, fail-closed |
| `src/runtime/ai/NpcAlertSystem.cpp` | `:163` heard-branch decay (A3) |
| `src/runtime/ai/NpcInvestigateSystem.cpp` | `:30` dwell-can-elapse (A4) |
| `src/core/math/Aabb3.cpp` | confirm startInside reporting supports the blocker policy (`:67,102,105`) |
| debug HUD + draw-list + viewport flag | frustum, LOS rays, readout, markers, toggles |
| receipt appender (perception summary) | append-only keys (§9); regen golden once |
| `tests/unit/npc_perception_3d_tests.cpp` (new) + `npc_behavior_system_tests.cpp` + `stealth_garden_tests.cpp` | §10 matrix |

## 15. WIRING & INTEGRATION (how it actually hooks in — the cross-lane path)

**Lane law first (the constraint that shapes all wiring):** the perception math + `NpcPerceptionResult` live in
`runtime/ai` and must NOT depend on app/render (runtime never includes app/render — verified 0 back-includes).
The debug layer is app-side and *reads* the result. **Data flows UP (runtime → app), never down.** The runtime
never learns about the draw-list.

**Data path per tick (sim → debug):**
1. **Produce (runtime).** In the Session NPC-update (the tick site that today feeds the old bool into the alert
   stimulus, ~`Session.cpp:1133-1157`), compute each guard's `NpcPerceptionResult` and (a) feed `.perceived` to
   the stimulus in-place, (b) collect `{guardId, targetId, result}` into a **plain buffer on `SessionState`**
   (e.g. `std::vector<NpcPerceptionDebugSample> perceptionSamples`). Pure sim data — no draw dependency,
   replay-safe, receipt-emittable. Cleared each tick.
2. **Carry (app frame build).** The app-lane frame/HUD builder (`ProjectionRefresh` / the debug-HUD build path)
   reads that buffer off the session (app→runtime include is legal) and copies the summary into
   `DebugHudStore`/`NpcBehaviorDebugHudState` (the decomposed debug store). No perception recompute (Law 3).
3. **Draw (render bridge).** The draw-list emitter (`PrimitiveDrawList` / render bridge) turns the debug state
   into wireframe-frustum + line + text draw commands, gated by the viewport toggle. The **render lane only ever
   sees geometry**, never the `ai` types — the frustum/rays are pre-baked to lines app-side.

**New files + CMake:**
- `runtime/ai/SegmentOcclusion.{hpp,cpp}` — recommended: the one occlusion owner (§6) is called by ai + session
  + reasoning, so it wants its own TU. Add to `target_sources(iggy3d ...)` in `CMakeLists.txt`. (If kept inline
  in an existing ai header instead, no CMake change — but a shared TU is cleaner.)
- `tests/unit/npc_perception_3d_tests.cpp` — new; register in the CMake test list (mirror an existing
  `npc_*_tests` entry: `add_executable` + `add_test` or the project's test-registration macro).
- The perception fn itself stays in `NpcBehaviorSystem.cpp` — no new source file there.

**Config plumbing (the 5 new `NpcBehaviorConfig` fields):** (a) defaults at the struct decl (§3); (b) add
parse+emit wherever `NpcBehaviorConfig` is (de)serialized for scenarios (the scenario TOML codec); (c) confirm
the construction site that builds the per-session config. **Back-compat:** a missing TOML key → the §3 default,
never zero (a `verticalHalfAngleDegrees=0` blinds the guard).

**Receipt wiring:** the perception-summary appender (§9) reads the SessionState buffer — same table-driven
pattern the receipt lane now uses (rows, not hand-written calls); append-only keys at the section's existing
order; regen the golden once (reviewed).

**Toggle wiring:** a `productDrawPerceptionDebug` viewport flag + 3 sub-flags (cones/rays/readout), set from the
dev-tools menu, read by the draw-list emitter — mirrors the existing `productDraw*Visible` viewport flags exactly.

**Include-direction sanity (must hold):** `app/*` may include `runtime/ai/NpcBehaviorSystem.hpp` (for the
result type) — legal. `runtime/*` must include nothing from `app/`/`render/` — so the sample buffer is a plain
struct in `runtime`, and ALL rendering translation happens app-side. If any slice makes `runtime/ai` include a
draw/HUD header, it's wrong — push the translation up.

## 16. EXISTING-CODE CHANGES (not new code — what must be ADAPTED / DELETED / INVERTED)

The new types are additive; landing them is not. Enumerated so no caller / test / assumption is missed.

**Deletions (remove + migrate every caller — grep before deleting):**
- `horizontalDistanceMeters` (`NpcBehaviorSystem.cpp:12-16`) — dead once 3D distance lands; callers use `distanceMeters`.
- `targetWithinVisionCone` (`:21-42`) — the 2D cone; replaced by the two-angle check.
- `kEyeHeightMeters = 1.0F` constant (`Session.cpp:1008`) — replaced by per-entity config eye heights.
- `hasBlockerBetween` (`Session.cpp:1047-1083`) — sound path calls the shared `segmentOcclusion` instead.
- `NpcPerceptionRequest.targetHasLineOfSight = true` (`NpcBehaviorSystem.hpp:69`) — occlusion now enters via the
  fn + tri-state, not a default-true bool.

**Behavior inversions (SAME line, OPPOSITE outcome — the highest-risk edits; each needs its own pin-test):**
- `Session.cpp:1032` `if (hit.startInside) continue;` → startInside now **blocks** (skip→block). Changes the
  occlusion verdict for any guard inside/flush-with geometry — verify no scenario relied on the old cheat.
- `Session.cpp:1027` `if (!result.ok) return true;` → fail **closed**. Inverts the failure default; audit callers
  that currently lean on the "granted on failure" behavior (there shouldn't be any, but confirm).

**Contract / caller migration (a signature change ripples — grep the constructors):**
- `queryNpcPerception` return type + `NpcPerceptionRequest` shape change → **every** call site adapts (the Session
  tick site ~`:1133-1157` and any test building the request). `grep NpcPerceptionRequest{`.
- `actorHasLineOfSightToTarget` (`Session.cpp:1002-1040`) → thin wrapper over `segmentOcclusion`, or deleted;
  callers read the result's `los`.
- `reasoningSegmentBlocked` (`ReasoningGraph`) → body replaced with a `segmentOcclusion` call; delete its own
  eye-height handling.

**Modified existing logic (real behavior change, Part A — not a refactor):**
- `NpcAlertSystem.cpp:163` heard-branch → add decay to the existing escalation.
- `NpcInvestigateSystem.cpp:30` dwell reset → gate so the give-up timer can elapse under sustained noise.
- `NpcSoundPerception.cpp:45` parallel `blockers[i]` read → length guard + fail-closed on missing.

**Config codec:** the scenario (de)serializer for `NpcBehaviorConfig` gains 5 fields on BOTH parse and emit;
existing parse must tolerate missing keys → §3 defaults (don't zero). Grep the TOML round-trip test — it will
need the new fields asserted.

**Existing debug draw — CHECK FIRST:** grep `NpcBehaviorDebugHud` / draw-list for any *current* cone/perception
drawing. If a 2D cone is already drawn, that code is **replaced** (two cones = confusion), not added alongside.
If none, it's a pure add.

**Existing tests to REWRITE (not re-run):**
- `npc_behavior_system_tests.cpp:173-218` (2D cone + raw-bool LOS) → the §10 struct/3D matrix.
- `stealth_garden_tests.cpp` occlusion cases → re-pin under per-entity eye height (deliberate + reviewed; the
  wall is y=3m so it still spans, but the ray endpoints move).
- Any test asserting old perception receipt keys → the appended keys.
- **`product_receipt_key_order.golden`** → one reviewed append (§9), then byte-stable.

**Assumption audit (silent-break watch):** anything downstream that assumed *horizontal-only* perception or the
*1.0m ray* (e.g. level layouts tuned so a 1.2m wall blocks) shifts under per-entity eye heights — flag for
playtest, not just compile.

---
**DONE = build green · full suite green (incl. the §10 matrix + the vertical-garden case) · receipt golden a
single reviewed append then byte-stable · debug shows a thief on a ledge above the guard as OUT of cone, live.**
