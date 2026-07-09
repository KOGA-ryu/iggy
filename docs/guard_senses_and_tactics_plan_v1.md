# Guard Senses & Tactics — Master Build Plan v1.1

**The governing detail for everything after perception P2.** Handoff target: the slicing loop — this doc is
written so cards can be cut from it without asking the planner intent questions. Every claim verified at HEAD
`0bad85c2` (recon `wp2ph26c0`, 4 agents, hostile-verification). Supersedes `perception-3d-maximum.md` §§P3–P5
detail (the maximum stays the contract reference; this is the build order). Companion:
`docs/operating_loop_v1.md` — its rules are restated here as slicing law.

---

## 0. STATE AT HEAD — what is already true (do not re-plan these)

- **P1 + P2 are LANDED.** 3D perception math is live: eye-height cone math, `verticalAngleDeg`,
  `inVerticalCone`, `perceived`, `Los` tri-state (`NpcBehaviorSystem.cpp:312-368`, `hpp:97`). The 3m-ledge
  garden test exists (`stealth_garden_tests.cpp:690-735`). New statuses `TargetOutOfCone` (covers both cones)
  and `TargetOccluded` exist (`cpp:361-368`).
- **E180 already closed half the affordance seam:** creative bake emits `cover`/`patrol_post`
  (CoverPoint/PatrolNode descriptors carry semantics, `ObjectDescriptor.cpp:716,:731`; end-to-end pin at
  `creative_document_room_bake_tests.cpp:1169-1200`), and ascii `monster_spawn`→`"monster"` is split. STALE
  claims to ignore: "wire-strings emitted by neither path", "ReconIntel doesn't exist".
- **ReconIntel + GuardRecon kernels EXIST** (`ReconIntel.hpp:24-50`, `GuardRecon.hpp:36-39`, incl.
  `watchedNodeKind`) — capture/hash/serialize all built, **zero production callers**. The missing piece is a
  consumer, not the packet.
- **Tactical node kinds are consumed by exactly ONE thing:** `chooseSearchNode` scores nodes
  `suspicion·w + strategicValueByKind[kind]·w − travel·w` (`GuardDecision.cpp:49-125`, table
  `GuardDecision.hpp:41-56`). No kind-specific tactic exists anywhere. The chess layer is unbuilt.
- **Two divergent eye models coexist:** kernel cone math uses config eyes (1.6m); the actual LOS ray
  (`Session.cpp:1009`), sound ray (`:1053`), reasoning-edge ray (`ReasoningGraph.cpp:18`), and the gaze-blade
  render (`SceneProjection.cpp:196`) all hardcode **1.0m**. The senses disagree about where the eyes are.
- **A debug vision overlay already ships:** the per-guard "gaze blade" (`SceneProjection.cpp:186-231` →
  `appendGazeBlade`, `BufferImageResources.cpp:606-647`), gated by `debugOverlayEnabled` via
  `includeNpcVisionDebug` (`ProjectionRefresh.cpp:754`), tinted by the three legacy mirrors. P4 extends a
  proven path, not greenfield.
- **Known perf debt on the hot path:** colliders are rebaked from surfaces EVERY tick
  (`Session.cpp:1099-1108`, no cache); `reasoningSegmentBlocked` copies the whole collider set per call
  (`ReasoningGraph.cpp:81`) and runs per-tick (`Session.cpp:890`) and per-node
  (`GuardDecision.cpp:36`, `ReasoningRoute.cpp:23`); `raycastPhysicsAabbs` collects+sorts ALL hits with no
  early-out (`PhysicsCollisionQueries.cpp:442-469`) — occlusion callers only need "any hit?".

## 1. DESIGN RULINGS (made here so no slice re-litigates them; veto before cutting P3)

**R1 — Radius joins visual confirmation.** `stimulus.visualConfirmed` (`Session.cpp:1160-1162`) and
`maybeApplyInvestigate`'s recompute (`:714-716`) omit the radius gate: an in-cone, clear-LOS target **beyond
perception radius still plants last-known memory every tick and bypasses the alert grace window** — a
detection cheat. Both sites become `perception.perceived`. Evidence this is safe: rise is already scaled by
`proximity01` (`NpcAlertSystem.cpp:160-163`); `targetPerceived` already uses `status==Ready` which is
equivalent to `perceived` for live targets (`Session.cpp:1154`); all garden fixtures operate inside radius.
Behavior change is confined to beyond-radius geometries — which is the point.

**R2 — Config owns the eye. One model everywhere.** Truth = `guardEyeHeightMeters` (1.6) /
`targetStandEyeHeightMeters`, from the resolved profile. The hardcoded 1.0m in the LOS ray, sound ray,
reasoning ray, and gaze blade all die. `occlusionMarginMeters` (0.05 config) replaces the hardcoded 0.01
margins — wire it or the config field is decorative drift. **This is a real behavior change near wall tops**
(3m garden walls still occlude; short/mid cover shifts) — it lands with its own pins, never silently inside a
"mechanical" migration.

**R3 — Fail toward NON-detection, uniformly.** A failed/degenerate occlusion query must never fabricate
*detection*. Vision: `Unknown` → no sight (kills `Session.cpp:1028-1030` fail-open). Sound: a failed blocker
query **applies** the wall loss (muffled), replacing "hears at full volume through unknown walls"
(`:1071-1073`). Reasoning edges: a failed query means **no edge** (conservative graph). All three = the same
law with per-sense mapping.
**R3.1 amendment (builder-raised at P3b, ruled 2026-07-09): absence of blockers is knowledge; absence of
knowledge is Unknown.** An EMPTY collider set from a *successful* bake means open space → `Clear` — an empty
room grants sight; that is reality, not fabricated detection. The existing reasoning/route tests that treat
`noColliders` as traversable / edge-restoring (`reasoning_graph_tests.cpp:125` et al.) encode the correct
semantics and need **no re-pins**. `Unknown` is reserved for: degenerate segments, failed physics queries,
and **failed/absent bakes**. The real hazard R3 kills: at HEAD a failed bake erases itself into an empty
vector (`Session.cpp:1099-1108`) — indistinguishable from open space downstream. P3b therefore threads a
**bake-validity signal** alongside the collider set so failure ≠ emptiness at every call site.

**R4 — startInside = Blocked** for all occlusion (`Session.cpp:1033-1035`, `:1076-1078`, and the copy in
`ReasoningGraph`): inside geometry means you cannot see/hear-clearly/path through it.

**R5 — The observable layer adds ZERO golden-pinned receipt fields.** (Deviation from maximum §9 — deliberate
subtraction-ratchet call.) Per-guard perception observability = AiActorState mirrors → debug snapshot rows →
gaze-blade/HUD, all already outside StateHash/SaveCodec/golden (verified). Indexed per-guard keys would never
appear in the zero-guard golden fixture anyway. If a contract-grade receipt is ever needed, it must arrive
with a named consumer.

**R6 — P4 extends the gaze-blade path,** not the creative wireframe frame slot (that slot is single-owner,
fed by the creative lane, `Loop.cpp:264-282`). Scene-projection mesh emit is proven, cheap, and already
debug-gated. The wireframe slot generalization is a reserved socket, not this arc.

**R7 — Sound alone still cannot reach Combat band.** The existing cap (`NpcAlertSystem.cpp:135-140,:164-172`)
is intended design and survives P5 untouched.

**R8 — Dual patrolPost sources stay** (PatrolNode point anchors AND PatrolRoute waypoints both → patrolPost
nodes): route = the beat, node = the post. Documented, not deduplicated.

## 2. THE PHASES — detail for slicing

Dependency DAG: `P3a → P3b → P3c` · `P3d` independent after P3b · `P4` after P3 (draws the unified truth) ·
`P5` independent of P3/P4 (alert internals) · `P6a/6b` independent · `P6c` before `P6d` matters in non-creative
worlds · `P6d` after P3 (tactics must not build on cheating senses) · `P6e` last (cross-lane payoff).
**Everything serializes per-file as usual; P5 and P6a/6b can interleave with P3/P4 (disjoint files).**

---

### P3 — ONE OCCLUSION TRUTH (kills the wall cheats) — Mode C

**P3a — physics: a segment-blocked query built for occlusion.** New
`bool segmentHitsAnyPhysicsAabb(std::span<const PhysicsAabbCollider>, Vec3 from, Vec3 to, float margin,
bool* startInside)`-shaped query in `PhysicsCollisionQueries` (span-native, **early-out on first hit**, no
sort, no hit-vector allocation — the current `raycastPhysicsAabbs` full-scan+sort stays for callers that need
ordered hits). Include a micro-bench vs the old path (the benchmark harness exists in `runtime/physics`).
*Metric promise:* occlusion query allocations per call → 0; bench shows ≥ the old cost, expected ~O(first-hit).
*Stop:* if `PhysicsRaycastQueryRequest` has hidden consumers that preclude a sibling query, report.

**P3b — the owner: `SegmentOcclusion` (runtime/ai, new TU).**
`enum class SegmentOcclusionVerdict { Clear, Blocked, Unknown }` +
`SegmentOcclusionVerdict segmentOcclusion(std::span<const PhysicsAabbCollider>, Vec3 fromEye, Vec3 toEye,
float marginMeters)` — implemented on P3a; `startInside → Blocked` (R4); **empty span → `Clear`;**
degenerate segment / failed query → `Unknown` (R3.1).
**Bake-validity threading (R3.1):** the per-tick vision-collider bake (`Session.cpp:1099-1108`) and
`buildReasoningGraph`'s own bake must report success distinctly from emptiness — carry `{colliders, ok}` (or
equivalent) to the call sites; on `!ok`, the caller passes `Unknown` down (vision → no sight, sound → wall
loss, reasoning → no edge); on `ok`+empty, normal `Clear` behavior. *Stop:* if the bake API cannot report
failure distinctly, report before inventing a wrapper — the fix may belong in the bake fn's signature.
Migrate all three duplicates onto it and DELETE their bodies: `actorHasLineOfSightToTarget`
(`Session.cpp:1003-1041`), `hasBlockerBetween` (`:1048-1084`), `reasoningSegmentBlocked`
(`ReasoningGraph.cpp:64-101` — its per-call vector copy dies with it). Eye lifting moves to the CALLERS
(they own entity + config): guard eye from resolved config (R2); sneak-eye stays a TODO socket. Per-sense
mapping at the call sites: vision `Unknown→no sight`; sound `Unknown→apply wall loss`; reasoning
`Unknown→no edge`.
*Metric promise:* 3 occlusion implementations → 1; hardcoded `1.0F` eye + `0.01F` margin count in
runtime → 0 (grep-gated); net LOC down.
*Pin-tests (planner-authored at card-cut — the P2 discipline):* wall-spans-eye blocks; guard-inside-wall →
Blocked; successfully-baked EMPTY collider set → `Clear` → sight preserved (open room, R3.1); FAILED/absent bake →
`Unknown` → no sight / wall-loss applied / no edge per sense; short wall under eye height does NOT block standing target;
margin boundary (touching wall ≠ blocked); reasoning-edge failure → no edge; sound failure → wall loss
applied. Garden suite re-pinned **deliberately** where the 1.0→1.6 eye shift moves an outcome (expected: 3m
walls still occlude; document any test whose value changes and why).

**P3c — collider reuse.** Cache the per-tick bake (`Session.cpp:1099-1108`): bake once per tick into
`SessionTransientState` (the existing transient-buffer pattern, `SessionState.hpp:64-79`) or key on the
collision-surface revision; `buildReasoningGraph`'s separate bake (`ReasoningGraph.cpp:~183-186`) reuses the
same product where an activation has one. *Metric promise:* collider bakes per tick → ≤1; behavior
byte-identical (Mode P — suite + garden green unchanged). *Stop:* if surface revision tokens don't exist on
this path, report rather than invent a freshness scheme (the collision store pattern is nearby — reuse it).

**P3d — the R1 wire.** `visualConfirmed := perception.perceived` at BOTH sites (`Session.cpp:1160-1162`,
`:714-716`); `stimulus.targetPerceived := perception.perceived` (drop-in, `:1154`). Delete the now-unread
`hasLineOfSight` conjunctions. *Pin:* beyond-radius + in-cone + clear-LOS target plants NO last-known memory
and opens NO grace bypass (new test); all in-radius behavior unchanged (garden green).
*Stop:* if any OTHER reader of `stimulus.visualConfirmed` exists beyond the alert/investigate/record trio,
report before rewiring.

---

### P4 — THE OBSERVABLE GUARD (the debug layer earns the game-feel harness) — Mode C, app+runtime

1. **Mirrors:** extend `AiActorState` observability mirrors (`AiState.hpp:126-131`) with
   `lastHorizontalAngleDeg`, `lastVerticalAngleDeg`, `lastInVerticalCone`, `lastPerceived`, `lastLos`,
   `lastGuardEyeHeightMeters`; written beside the existing three (`Session.cpp:1229-1232`). Excluded from
   StateHash/SaveCodec exactly like the existing mirrors (verified they are outside both).
2. **Snapshot:** extend `NpcBehaviorDebugActorRow` (`NpcBehaviorDebugSnapshot.hpp:31-56`) + `applyAiState`
   (`cpp:48-59`) with the new fields; HUD line format gains `v=<vert°> los=<C|B|U>` (append at the END of the
   line — HUD text is not golden-pinned, verified).
3. **Gaze blade → truth-driven frustum:** upgrade `appendGazeBlade` (`SceneProjection.cpp:186-231`): apex at
   the guard's **mirrored eye height** (kills its hardcoded 1.0m — R2 finishes here), blade extent from
   `lastSightRangeMeters`, add the **vertical extent** (a second blade pitched to ±`verticalHalfAngleDegrees`,
   or a 4-edge pyramid — implementer's choice, geometry only), tint: green `lastPerceived`, yellow in-cone
   but `lastLos==Blocked`, grey otherwise (replaces the 3-bool tint at `:217-218`).
4. **Toggles:** ride the existing `debugOverlayEnabled → includeNpcVisionDebug` gate (`ProjectionRefresh.cpp:754`)
   — no new flags in P4 v1. (The `productDraw*Visible` fields are OUTPUT mirrors, not toggles — do not touch.)
5. **No golden change** (R5). *Metric promise:* golden byte-identical; snapshot/projection tests extended
   (tint truth-table test at `projection_tests.cpp:953-983` pattern); hardcoded eye heights in render path → 0.
*Stop:* if the blade geometry needs a primitive the scene-projection emit can't express, report (do NOT reach
into the creative wireframe slot — R6).

---

### P5 — HONEST ALERT DYNAMICS (kills the sustained-noise freezes) — Mode C, `runtime/ai` only

**The two confirmed defects, with root causes:**
- **Alert never decays under sound:** the else-if chain (`NpcAlertSystem.cpp:158-177`) — ANY `heard` tick
  takes the rise branch and `npcDecayAlert` is never called, even when the rise is grace-swallowed or rounds
  to 0; every accepted rise re-anchors `lastRiseTick` (`:145`) keeping decay dead 6 more ticks.
- **Investigation never gives up:** any fresh sighting/hearing unconditionally zeroes
  `investigateDwellTicks` (`NpcInvestigateSystem.cpp:30`), so `dwellLimitTicks` (`:64`) never elapses under a
  sustained source.

**The fix semantics (spec, not code):**
- A tick whose stimulus produces **no accepted rise** (grace-swallowed, zero increment, or band-capped) falls
  through to decay. Sustained constant noise ⇒ alert rises to its sound cap, **plateaus**, and once the noise
  stops, decays after `deadTimeTicks` — never ratchets forever. `lastRiseTick` re-anchors only on **accepted**
  rises (already true — the bug is only the skipped decay call).
- Investigation dwell resets only on a **relocated** stimulus (new position > epsilon, e.g. 0.75m, config)
  or a strictly higher band; a sustained same-origin source lets dwell elapse → guard gives up → resumes
  patrol (R7 keeps sound below Combat regardless).
*Pin-tests (planner-authored at card-cut):* N-tick constant sound → plateau (level stops rising); noise stops
→ decay begins after dead time; sustained same-origin sound → investigate → give-up within
`dwellLimitTicks + deadTime`; relocated sound → dwell resets once; visual behavior byte-unchanged.
*Metric promise:* the two new dynamics tests green; all existing alert-band tests green unchanged (rise
rates, caps, grace, band thresholds untouched). *Stop:* if `alertLevel` participates in session StateHash
(verify — recon shows only ReconIntel hashes it, no production caller), flag before changing decay timing.

---

### P6 — THE TACTICAL ARC (the payoff: guards that use the map) — Mode C

**P6a — ascii affordance glyphs.** Ascii can author NO affordance strings today
(`AsciiRoomToRoomAsset.cpp:351-384` maps everything else → `"marker"`). Add glyph rows (`AsciiRoomGrid` table)
+ cell kinds + marker tags for at minimum `cover` and `patrol_post` (parity with creative), with the tag →
anchor-kind pass-through. Glyph choice is the implementer's (table capacity + collision check) but the TAGS
are the vocabulary strings verbatim. *Pin:* ascii room with the new glyphs → bake → `buildReasoningGraph`
yields coverCluster/patrolPost (mirror of the creative pin at `creative_document_room_bake_tests.cpp:1169`).

**P6b — the missing three: `chokepoint` / `high_ground` / `hiding_spot` authoring.** E180's declared
follow-up. Creative: 3 descriptor rows (pattern: CoverPoint at `ObjectDescriptor.cpp:703-717`) + 3
`CreativeRuntimeAnchorSemantic` values + `toString` cases + sentinel/coverage updates. Ascii: 3 more glyph
rows (with P6a's machinery). Reader needs NOTHING (already live). *Pin:* both paths × three kinds → correct
node kinds. *Note:* `InterestPoint` (`ObjectDescriptor.cpp:1584-1597`) stays reserved — new rows, no reuse.

**P6c — reasoning graphs outside creative rooms.** Production builds a reasoning graph ONLY via the creative
RebuildRoom activation (`CreativeReasoningActivation.cpp:13-14`); ascii/package-launched worlds run guards on
an EMPTY graph. Add the world-launch activation hook: after a launched world's room bake, build the graph
(same inputs: RoomAsset + patrol waypoints — production waypoints come from creative PatrolRoute paths via
`patrolRouteWaypointsFromDocument`; for scenario-TOML worlds, feed the TOML waypoints that today only seed
movement, `PackageSessionSeed.cpp:252,:348`). **Perf gate:** graph build is O(n²)+ray — it runs at LAUNCH
(once), never per tick; P3a/P3c make its rays cheap. *Pin:* launch an ascii world with anchors → guards
search real nodes (a `session_tick` or garden-style test on a launched, non-creative session).

**P6d — kind-specific tactics v1 (the chess layer).** Today kinds are one scalar table. Build the first REAL
tactic, smallest-payoff-first, all inside `GuardDecision` + `Session` behavior stages, deterministic, each
with its own sim test:
1. **Hiding-spot sweep:** in Searching band with last-known memory, `chooseSearchNode` already scores — add
   the behavior: visit the nearest K `hidingSpot` nodes within R of last-known before giving up (a bounded
   search plan, not a score tweak).
2. **Chokepoint hold:** when a target was perceived and lost near an `exit`, prefer holding the `chokepoint`
   node between last-known and the nearest exit for M ticks over direct chase (uses the existing route
   machinery, `maybeFollowRoute`).
3. **High-ground overwatch:** in Agitated band without a target, prefer the nearest `highGround` node as the
   watch post (`watchedNodeKind` already flows into GuardRecon).
Each tactic = its own slice with a sim pin ("guard with hiding spots checks them; without them, old behavior
byte-identical"). The scalar table stays as the fallback scorer. *Stop per tactic:* if it needs state the
`AiActorState` doesn't carry, add the field in ITS slice with save/hash implications stated (these ARE
durable-adjacent — unlike P4 mirrors, tactic state likely needs SaveCodec/StateHash treatment; verify the a2
lock pattern per slice).

**P6e — the recon payoff (cross-lane, LAST).** `projectGuardRecon`/`ReconIntel` exist and are testable with
zero consumers. Wire the vertical-slice moment: at objective-reach (the session outcome path), capture
`ReconIntel`, carry it through the save/session seam, render a first Notebook recon page from it
(`menu/Notebook` currently includes neither header). This is the thief→duo interface made real. Scope: ONE
page, real data, no polish. *Pin:* scripted run reaches objective → intel packet captured → notebook page
renders guard count/alert/watched-node from the packet → survives save/load (the packet already
hashes/serializes — pin the round-trip in production wiring).

---

## 3. SLICING LAW (how the loop cuts this — restated from operating_loop_v1)

- **Every slice declares Mode** (P3c is Mode P; everything else here is Mode C) **+ a metric promise + a stop
  condition.** The promises above are per-phase minimums — keep them mechanical (`grep -c`, golden diff,
  bench numbers, test counts).
- **Behavior slices get planner-authored pin-tests at card-cut** (P3b/P3d/P5 especially — the exam is not
  written by the implementer). Request them from the planner with the card draft; do not fill them in.
- **Deliberate re-pins are commits of their own:** R1/R2 change real outcomes near walls and beyond radius.
  Any existing test whose value changes gets its re-pin in the SAME slice with a one-line justification in
  the commit — a re-pin without a named ruling (R#) is a regression, stop.
- **The golden moves ZERO times in this entire plan** (R5). If a slice diffs it, the slice is wrong.
- **Ratchet obligations:** P3b deletes 2 duplicate occlusion bodies + all hardcoded eye/margin constants
  (grep-gated to 0); P3a/P3c reduce per-tick allocations; P4 kills the last hardcoded eye height. Weight goes
  DOWN through this plan — any slice that adds a parallel path instead of replacing one is off-plan.
- **Re-anchor at card-cut.** Line numbers here are HEAD `0bad85c2`; the tree moves fast. Anchor by symbol.
- **Serial on shared files** (`Session.cpp` is the choke point: P3b/P3c/P3d/P4-mirrors all touch it — those
  slices are strictly sequential; P5 and P6a/6b interleave freely).
- **Reserved sockets — leave the seams, do NOT build:** sneak-stance eye height (movement lane owns stance;
  TODO stays at `NpcBehaviorSystem.cpp:314`); peripheral/second cone; 3D hearing; wireframe-slot
  generalization; multi-guard shared alert; notebook polish beyond the P6e page.

## 4. STALE-DOC CORRECTIONS (fold when next touched)

- `game_master_plan_v0_1.md`: the "one live seam break" is half-closed (E180); "ReconIntel doesn't exist" is
  false — the gap is production consumers (P6c/P6e).
- `stealth_ai_hardening_plan.md` Part A: A1/A2/A5 land as P3b; A3/A4 land as P5; the sound length-guard
  claim was overstated (the read is bounds-guarded, lengths lockstep-constructed — an assert is optional
  hygiene, not a fix).
- `perception-3d-maximum.md`: bump pointer to this plan for P3+; §9's receipt keys are superseded by R5.

---
**DONE for the arc** = a guard that cannot see through walls or floors, whose senses share one eye model and
one occlusion truth, whose alertness rises AND falls honestly, who is watchable in the overlay (cone, rays,
why-readout), who uses cover/chokepoints/high-ground/hiding-spots authored in EITHER editor path in ANY
launched world — and whose knowledge, when the thief escapes with it, lands on a notebook page.
