# Stealth AI Hardening + Depth Plan

Structure to build from (review slice 1, workflow `wzcfuhxtf`). The AI brain is architecturally clean
(reasoning/tactics scored 4/5) but **correctness is 2/5** — a cluster of *silent, untested, stealth-inverting*
failures. Verdict was **harden-first before the affordance seam / notebook lean on it.** Three parts below:
**A** = fix the cheats (card-ready, no design call), **B** = the one design ruling you must make, **C** = the
tactical depth (the juicy build). All findings verified at the cited lines.

---

## Part A — The cheats (fix now; each is card-ready, ~S each)

Each is a silent behavior bug in the *dangerous* path, none pinned by a test. For each: fix + the test that
pins it (the test is the point — these hid because nothing asserts the stealth invariant).

**A1. LOS sees through walls it starts inside.** `Session.cpp:1032` does `if (hit.startInside) continue;` — a
guard flush against / embedded in a wall AABB gets "clear sight" through it.
- **Fix:** a `startInside` collider counts as a *blocker* for LOS/occlusion (you're inside geometry), OR clamp
  the guard eye-point out of solid before casting. Same policy in `hasBlockerBetween` (`:1074`).
- **Test:** guard eye inside a wall AABB → `actorHasLineOfSightToTarget` returns *blocked*.

**A2. Failed LOS query fails OPEN.** `Session.cpp:1027` `if (!result.ok) return true;` + degenerate delta
(`:1016`) → sight granted. A broken/invalid/unwired occlusion query = "guard sees through walls," silently.
- **Fix:** fail **closed** for stealth (a failed occlusion query must NOT grant sight), OR carry a tri-state
  `{clear, blocked, unknown}` so the FSM + notebook can tell a *confirmed* sight from an *unverified* one.
  `NpcPerceptionRequest.targetHasLineOfSight` (`NpcBehaviorSystem.hpp:69`) currently defaults `true` — invert or
  make it non-defaulting.
- **Test:** failed/empty-collider query → no sight.

**A3. Sustained noise freezes alert forever.** `NpcAlertSystem.cpp:163` — the heard/sound branch bypasses decay,
so a persistent audible sound ratchets alert and it can never come back down.
- **Fix:** the heard branch must still apply decay / a cap that decays; alert should relax when the *stimulus*
  (not the ambient noise) stops.
- **Test:** hold a sound N ticks → alert rises, then plateaus/decays, does not climb forever.

**A4. Sustained noise traps a guard in perpetual investigation.** `NpcInvestigateSystem.cpp:30` — a fresh
sighting/hearing *unconditionally* resets the dwell counter, so "give up → return to patrol" never fires while
noise continues.
- **Fix:** separate "new distinct stimulus" (resets dwell) from "same ongoing noise" (does not); or cap total
  investigation time regardless of refresh.
- **Test:** sustained noise → guard investigates, then gives up and resumes patrol within the bound.

**A5 (structural, ~M). One occlusion owner.** Eye-height segment occlusion is duplicated 3×: LOS
(`Session.cpp:1002-1040`), sound (`hasBlockerBetween :1047-1083`), reasoning edges (`reasoningSegmentBlocked`,
`ReasoningGraph.hpp:69`). To change eye height / margin / startInside policy you must edit 3 sites in sync.
- **Fix:** consolidate into one shared segment-occlusion function (the ReasoningGraph one already aspires to be
  "the ONE segment discipline"); LOS + sound + reasoning-edge all call it. Fixing A1 once then fixes all three.
- Also: `NpcSoundPerception.cpp:45` reads occlusion from a parallel `blockers[i]` span with no length check — a
  short span silently treats tail events as unobstructed. Add the length guard.

---

## Part B — THE RULING you must make: does a guard perceive verticality?

**The bug:** perception is entirely 2D. Cone, radius, and distance are computed on the XZ plane only — facing
and to-target are both flattened to `y=0` (`NpcBehaviorSystem.cpp:25,30`), radius gates on horizontal distance
(`:277,283`), and occlusion uses a single fixed `kEyeHeightMeters=1.0` for both endpoints (`Session.cpp:1008`).
A thief on a ledge directly above is "in cone, in radius, seen"; short cover under ~1m never occludes.

**This is a design fork, not a fix. Pick one tomorrow, then it becomes the plan:**

- **Option 1 — 3D perception (harder game).** Verticality is just another axis the guard watches. Add
  `verticalHalfAngleDegrees` to `NpcBehaviorConfig` (`NpcBehaviorSystem.hpp:23`), gate the cone on pitch, use
  y-aware distance in `queryNpcPerception`, and derive eye height per-entity (stance/capsule) instead of the
  shared constant. **Build:** ~M, all in `NpcBehaviorSystem` + the `Session` eye-height wiring; test cone
  boundary in pitch + above/below cases.
- **Option 2 — 2D on purpose (height IS the escape verb, à la Thief/Splinter Cell).** Then it's a *mechanic*,
  not a bug — but it must be a **documented ruling**, AND occlusion must actually respect floors/ledges so
  "break LOS by going up" works (today the flat 1m ray half-ignores height). **Build:** ~S — write the ruling,
  make eye-height data-driven so cover heights matter, keep 2D cone. The stealth verb "go vertical to vanish"
  becomes real and intentional.

### RULING (2026-07-08): **3D perception** — the "realistic guard." Plan below.

**The game function — `NpcPerceptionResult queryNpcPerception3D(guard, target, config)`.** Replace the `bool`
return with a rich, *observable* result struct. This one change also fixes the review's "occlusion is a
bare-bool defaulting `true`" (A2) and "no single occlusion owner" (A5): the struct IS the typed contract that
the FSM, the debug layer, the tests, and (later) the notebook all read from.

```cpp
struct NpcPerceptionResult {
  bool  perceived;              // the only thing the FSM reads
  float distanceMeters;         // full 3D
  float horizontalAngleDeg;     // yaw between facing(XZ) and delta(XZ)
  float verticalAngleDeg;       // asin(delta.y / dist)
  bool  inRadius, inHorizontalCone, inVerticalCone;
  enum class Los { Clear, Blocked, Unknown } los;   // tri-state — no more fail-open
};
```

- **Distance:** full 3D `length(target - guard)`, gated on `perceptionRadiusMeters` — far above/below now
  drops out of radius (today it doesn't).
- **Cone = TWO angles, not one solid angle** (a guard scans *wide* horizontally, *narrow* vertically — matches
  a real head): keep the existing horizontal yaw half-angle; **add `verticalHalfAngleDegrees` to
  `NpcBehaviorConfig`** (`NpcBehaviorSystem.hpp:23`) and gate on `verticalAngleDeg`. Verticality becomes the
  escape verb: exceed the vertical half-angle — even dead-ahead horizontally — and you're out of cone.
- **Per-entity eye height** (guard + target stance/capsule), not the shared `kEyeHeightMeters` constant, so
  crouch and cover-height matter. Feeds the LOS ray (with A1 startInside + A2 fail-closed folded in).
- FSM reads only `.perceived`; every other field exists to be *observed and tested*.

**The debug layer — renders FROM the result, so it cannot lie about what the sim decided.** Extend
`NpcBehaviorDebugHud` + the debug draw-list, behind a dev-tools toggle:
- **Cone:** a wireframe frustum from the guard eye along facing, sized by the h/v half-angles + radius — you
  literally *see* the vertical slice, so vertical blind-spots are visible, not guessed.
- **LOS ray** per (guard, target): a line eye→eye, colored by the result — **green** `perceived`, **yellow**
  in-cone but `los == Blocked`, **grey** out-of-cone/radius. See-through-walls (A1) and vertical-miss bugs
  become literally visible.
- **Readout:** small text — `dist / horiz° vs H / vert° vs V / los` — so you see *why* (e.g. `vert 42° > 30°
  → out of cone`), not just the verdict.
- The **same struct** powers the pin-tests (above / below / vertical-boundary — closes the risk-coverage gap)
  and, later, the notebook packet.

**Build slices:** (1) the `NpcPerceptionResult` struct + 3D check + `verticalHalfAngleDegrees` config + FSM
reads `.perceived` + pin-tests; (2) the debug overlay driven off the struct; (3) fold in Part A's LOS fixes
(startInside blocker, tri-state fail-closed) — they share the eye-height path, so land them here, not separately.

---

## Part C — Tactical depth: wire the 3 dead node-kinds (the chess layer)

The reasoning-graph reader is **live** for `chokepoint`→chokepoint, `high_ground`→highGround,
`hiding_spot`→hidingSpot (`ReasoningGraph.cpp:39-47`), but **nothing emits them and no tactic consumes them** —
dead vocabulary. Wiring them is what turns a guard from "patrol + chase a blip" into one that *holds a
chokepoint, takes high ground, checks hiding spots* — the "chess, not flavour" tactical AI. Two sides:

- **Emitter side (same shape as the affordance seam, `blocked/E165`):** creative object kinds + ascii glyphs
  that map to `chokepoint`/`high_ground`/`hiding_spot`, emitted onto `RoomAsset.anchors[].kind`. This is
  *net-new authoring surface* (no glyphs/object-kinds exist yet — `AsciiRoomGrid` has P/N/M only). Do E165
  (cover/patrol) first as the proven pipe, then extend.
- **Consumer side (the actual AI):** `GuardDecision` currently doesn't act on these node kinds. Give the guard
  tactics that read them — e.g. on alert, path to the nearest `hidingSpot`/`highGround` node to check, or hold a
  `chokepoint` between itself and last-known. **Build:** L — this is the real tactical-AI work and where the
  "deck = flavour, chess = tactics, Go = space" design gets its teeth. Plan it as its own slice after the
  perception ruling (B) lands, since good tactics need correct perception under them.

---

## Suggested order for tomorrow

1. **Part A** (the cheats) — card A1–A4 now, they're pure correctness + tests, no design needed. A5 bundles in.
2. **Part B** — make the verticality ruling; I turn it into the perception/LOS card. (A1/A5 land inside this.)
3. **Part C** — the tactical-depth slice, after B, on top of a corrected perception. The juicy one.

*Do not build the affordance seam's guard payoff, the notebook, or the recon-intel packet on top of this brain
until Part A lands — they'll inherit the see-through-walls / never-decay cheats.*
