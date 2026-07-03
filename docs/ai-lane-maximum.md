# The AI Lane Maximum — destiny document v1.6

> Planner-authored 2026-07-02. This is the END of the AI lane: every behaviour layer the game's
> identity demands, their contracts, the dependency DAG, the pre-declared churn, and the
> reserved sockets. Slices are CUT FROM this map — no slice is architecturally homeless, no
> build-order debate, no reimagining. The cutter corrects the map (discoveries flow back as
> version bumps; the map stays authoritative between corrections).
>
> Fused from: `docs/stealth-ai-plan.md` (slices 1–6, TDM clean-room reference values),
> `docs/deck_driven_tactical_ai.md` (the deck/chess/Go/suspicion seed), the s5–s8 slice records
> (churn precedents), and a 3-reader recon of the as-built lane at trunk `465a3821`; hardened
> by a 3-critic adversarial pass (v1.0 → v1.1).
> Lane law: this is Claude + the box fleet's territory (`src/runtime/ai/**`, movement, later
> physics; `src/app/iggy3d/world/` seed plumbing is in-lane shared ground BY EXPLICIT ORDER —
> see refusals). Codex owns creative/map-generation. Cross-lane contracts are marked ⚠CROSS-LANE.

---

## 1. Identity → destiny (the constraints that decide everything)

The game's identity makes these non-negotiable; every layer below inherits them:

- **Determinism constitution.** Tick-indexed off `state.clock.tickIndex`; zero wall-clock, zero
  ambient RNG in the lane (verified). Any future randomness (deck draws, personality jitter)
  seeds through the already-hashed `AiActorState.deterministicPolicy` (u32, reserved, unused —
  this is what it was saved for) or an equivalent hashed field. Actor iteration stays
  EntityId-sorted. Known trap: `runSessionTick` returns NoWork WITHOUT advancing the clock —
  every time-driven layer (decay, suspicion, deck timers) inherits this; tests keep the clock
  alive with player Waits.
- **The triple-lock.** A durable `AiActorState` field exists in `SaveAiActorRecord`
  (SaveEnvelope.hpp) + `SaveCodec` writeAi/read + `StateHash.cpp` **in lockstep**, or is
  transient in all three. Today the exclusion sets agree exactly. `AiBehaviorKind` /
  `AiIntentKind` are append-only (serialized numbering). Every new intent kind touches THREE
  dispatchers: `aiIntentKindName` and `updateNpcFacing` are switches (`-Wswitch` enforces), but
  **`buildNpcBehaviorCommand` is an if-chain — the compiler will NOT flag it**; extend it
  manually or a new intent silently falls through to InvalidDecision and the guard stalls.
  (Converting it to a switch is a legitimate small slice — planner-approvable any time.)
- **Receipts never lie; decisions are inspectable.** Every layer emits receipt-grade facts;
  AI decisions get reasoning receipts (deck doc §18). No black boxes.
- **Headless-verifiable.** Every layer proves itself in a unit binary or a no-window smoke;
  the stealth garden is the fixture pattern (fixture + scenario TOML + snapshot-reading
  harness). Fixtures are part of the maximum, built once, reused forever.
- **Two readers, one vocabulary.** The asymmetric co-op identity (thief intel → battle duo)
  means the reasoning substrate serves BOTH guard AI and the recon journal.
  CONFIRMED already designed: `ProductNotebookReconPage{title, floorPlan('#','D','W','V','G'),
  garrison, patrols, hazards}` (menu/Notebook.hpp) is intel-packet-shaped on purpose. The
  reasoning-graph node vocabulary and the notebook vocabulary are THE SAME LIST — and the
  shared header lives RUNTIME-SIDE (`src/runtime/ai/`), with the notebook including it, never
  the reverse (the runtime-never-includes-app law).
- **Inspired AI, not perfect AI.** Readable, imperfect, purposeful. 1–2 ply lookahead until a
  measured need says otherwise.
- **Style law + clean-room boundary.** Enums + config structs + free functions over plain
  data; no class trees (the TDM State/Task/Subsystem tree is on the DO-NOT-REPRODUCE list,
  with the lightgem polynomial, BSP wavefront, and verbatim EAlertType). Reference VALUES may
  be used (they're in stealth-ai-plan.md §clean-room).
- **Companion AI (the priestess/knight duo side): reserved, not designed.** Out of scope for
  v1.x. If solo-play assist AI ever lands, it is a READER of L4/L6 exactly like guards —
  never a parallel stack. (Explicit so nobody improvises it outside the map.)

## 2. The layer stack (bottom → top, with as-built truth)

```
L0  Physical substrate   BUILT   collision/movement/kinematic motor (shared with movement lane)
L1  Perception stimuli   PART    vision BUILT · light-field ABSENT (slice 3) · sound ABSENT (slice 4)
L2  Alert / suspicion    BUILT   NpcAlertSystem FSM (s5) — the graded ladder
L3  Memory               PART    single-slot last-known (s7) — extend, never replace
L4  Reasoning graph      ABSENT  THE substrate: meaningful-position nodes, two readers
L5  Travel / route cost  ABSENT  travelCost over L4 (no navmesh, no per-tile graph)
L6  Tactical scoring     ABSENT  chess: guard decision loop over L4 (+personality seams)
L7  Influence maps       ABSENT  Go: per-channel territory over L4/grid
L8  Encounter deck       ABSENT  cards/budget/validator/placement/report
L9  Team layer           ABSENT  target selection, shared alert, partner sync, personalities
—   Persistence          PART    durable subset only; ALL stealth state transient (debt)
—   Reports              PART    debug snapshot BUILT; decision receipts + battle report ABSENT
—   Tuning               BUILT   s8 readout dashboard; constant-change slices pending
—   Physics horizon      SOCKET  kinematic kernels are the interface; dynamics later
```

### L0 — Physical substrate (BUILT, shared)
AI acts ONLY through the command pipe: `buildNpcBehaviorCommand` emits Wait/Attack/Move
`CommandRecord`s (CommandSource::Ai) through `appendCommandThroughAdmission`; movement executes
them via `movementRequestFromAcceptedCommand` → `executeMovement`. Spatial queries:
`CollisionQuery.hpp` (querySegment, surface samples), `raycastPhysicsAabbs` (LOS eye-ray,
fail-open by design — "never fabricates stealth"), colliders re-baked from `SpatialSurfaceSet`
every tick (a perf hazard L7 must hoist, pre-declared below). Blocked/no-ground point-moves
consume the NPC's tick — cost-blind destinations stall guards deterministically; that is WHY
L5 exists.

### L1 — Perception stimuli (vision BUILT; two sockets open)
- Vision (slices 1–2): `queryNpcPerception` gate order radius → cone → LOS;
  `NpcBehaviorConfig` thresholds; facing on `AiActorState`.
- **Sound (slice 4, a root stream).** Contracts pre-named in stealth-ai-plan.md —
  `SoundEvent{source, originMeters, loudnessDb, alertFactor, alertMax}`,
  `SoundPerceptionConfig{hearingThresholdDb=20, falloffCoeff=9, perWallLossDb=20, airLossDbPerMeter}`,
  `SoundPerceptionResult{heard, audibilityDb, alertUnits, investigatePos}`. Rules: highest-wins
  per tick, additive across ticks; cheap range-sphere reject; v1 investigatePos = true origin.
  **INTEGRATION RULING (supersedes the plan's signature):** the plan sketched a separate
  `SoundPerceptionResult` parameter on a new FSM entry (`npcStepAlertBehavior`) — that symbol
  does NOT exist in the tree and a separate parameter would violate the stimulus-bus law.
  Sound integrates as FIELDS on `NpcAlertStimulus` (heard/audibilityDb/alertUnits/
  investigatePos), computed upstream in `enqueueNpcBehaviorCommands` and fed to the EXISTING
  `npcStepAlert`. `ObjectSoundProfile{impact,scrape,break}` tags exist inert on ObjectTraits —
  the emission side's landing pad.
- Light-field (slice 3, optional/parallel — couples to VISION, needs nothing else):
  `LightField`/`PointLightSample`, `lightLevelAt`, `calibrateVisibility` — brightness SHRINKS
  effective radius (coupled, never decoupled).
- The stimulus bus is `NpcAlertStimulus` — new channels are FIELDS on it, never parallel paths.

### L2 — Alert / suspicion (BUILT — the ladder everything reads)
`AlertProfile` (rise 0.05/tick; bands 0.065/0.26/0.43/0.78/1.0; drains 100/160/500/1300 ticks;
dead-time 6; grace 1.2/40/5) riding on `NpcBehaviorProfile`. Single scalar `alertLevel` is the
source of truth; behavior always derived (`alertBehaviorForLevel`). No-target combat cap;
visual bypasses grace. Deck-doc §11's 0/25/50/75/100 states map onto these bands — the deck
layer TUNES this ladder via personality cards; it never forks it.

### L3 — Memory (single-slot BUILT; extensions live here)
`npcRecordSighting`/`npcStepInvestigate` with last-known position + dwell on the actor.
Future memory (clue trails, searched-node sets, decoy findings) extends THIS state + the L4
graph's node annotations — never a second memory system.

### L4 — Reasoning graph (THE missing substrate; a root stream)
"The physical map moves bodies; the reasoning map moves thoughts."
- Nodes = meaningful positions ONLY (deck doc §12): doorway, stair, chokepoint, hiding spot,
  cover cluster, objective, window, ladder, exit, patrol post, high ground, sound source,
  last-known position — **plus `reference` (14th, a3s1): the provenance-neutral entity-anchor
  kind (spawn/npc anchors); semantic spawn-region kinds arrive with A7, `reference` stays for
  unclassified anchors.** Edges typed: walkable, hidden, climb, locked, noisy, dangerous, guarded.
- **Home:** `ReasoningNodeKind` and friends live in `src/runtime/ai/`; `menu/Notebook.hpp`
  (the second reader) includes it — NEVER the reverse. The notebook-projection wiring itself
  is a small src/app edit: a ⚠CROSS-LANE-adjacent follow-up slice, planner-brokered, cut
  after A3 v1 proves the graph.
- **Two sources:** DERIVED (from room geometry / `SpatialSurfaceSet` / anchors — buildable
  today, needs nothing) and AUTHORED (the affordance vocabulary, stream A7 ⚠CROSS-LANE —
  glyphs and creative GameplayMarker kinds compile to graph nodes; the inert markers T/R/? and
  the N/M collapse get resolved there, closing next_work Stream 4).
- **Stitch, not merge:** the multiroom room-graph (portal edges) is a SEPARATE coarse substrate;
  when next_work Stream 3 lands, A3 gains ONE stitch slice (reasoning-graph `exit` nodes bind
  to portal anchors); no other layer churns. Pre-named, not built.
- Determinism: graph build is a pure function of room content; node ids stable-ordered.

### L5 — Travel / route cost (ABSENT; needs L4)
`travelCost(actor, node)` over graph edges (no A* over tiles, no navmesh). Terms available
today: distance, `sampleSlope`/`SlopeBand` multipliers, edge type penalties (noisy/dangerous/
guarded). Route = node sequence; the motor interface stays per-tick point-moves toward the
current route node (no new command kinds). **Route state is TRANSIENT BY DESIGN**: routes are
a pure deterministic function of (graph, durable actor state) and recompute on load — no
triple-lock growth in this layer, ever. This unblinds patrol/investigate/return from
straight-line stalls.

### L6 — Tactical scoring, the chess layer (ABSENT; needs L4, wants L5)
The guard decision loop (deck doc §13): score candidate nodes/actions =
`suspicion + strategicValue − travelCost − allyCoverage + personalityBonus`; simulate-lite,
1–2 ply. Inserts into the overlay chain as a new precedence rung (see churn). Scoring factor
list = deck doc §8. **Personality weight seams are reserved columns on the profile from day
one** (L9 lands cards into them without churning L6 again). Every decision emits a reasoning
receipt (§18 shape: chosen action, score, signed factor list).

### L7 — Influence maps, the Go layer (ABSENT; needs L4 only)
Per-channel maps (danger, visibility, sound, escape-route, objective-control, guard-influence)
over graph nodes and/or the room grid. Consumers: L6 scoring terms (enrichment, not
prerequisite), commander-style spread, the battle report's warnings ("right flank has low
cover"). Must hoist the per-tick collider rebake before multiplying spatial queries
(pre-declared perf churn).

### L8 — Encounter deck (SPLIT: A8a groundwork + A8b kernel)
- **A8a — groundwork (independent, cuttable early):** widen `assignNpcBehaviorProfiles`
  (PackageSessionSeed.cpp — today it CLEARS seed.aiActors and drops authored patrol/facing
  when a table is passed); grow the objective/outcome vocabulary past today's `PlayerHasItem`
  condition + TWO hardcoded outcome rules in SessionTick.cpp (`exit_` prefix → Victory,
  `collect_gold_key` → DemoComplete). Both are in-lane and need neither A5 nor A7.
- **A8b — the deck kernel:** cards with `requires/provides/cost` + difficulty budget +
  conflict RULES + placement + validation + battle report (deck doc §5–§7, §16–§17).
  Placement writes what spawn already reads: `ProductNpcProfileAssignmentTable` + scenario
  seeds (patrol/facing/guard anchors — via A8a's widening).
  **A7-stall fallback:** deck-doc §19's full-DNA minimum (one battlefield, 3+3+3 cards, budget,
  validator, placement, one scoring loop, battle report) runs on a FIXTURE-AUTHORED graph —
  authored-kind nodes hand-built in the test fixture, entirely in-lane. Only PRODUCTION
  authoring waits on Codex's half of A7. The milestone cannot be frozen by a cross-lane stall.

### L9 — Team layer (ABSENT; needs L6 seams + A1's SoundEvent)
Target selection (today EVERY guard targets hardcoded player slot 0 — the loop already iterates
N guards deterministically, so this is a scoring problem not a plumbing problem), shared alert
propagation (**ally call = a `SoundEvent` — reuse L1, don't invent a bus; this is a REAL
A1→A9 dependency, drawn in the DAG**), ally coverage term, partner-sync cards,
personalities-as-cards landing in the L6 weight columns and AlertProfile knobs (coward = early
call + safe-node bias; veteran = chokepoint bias + slower decay...).

### Cross-cutting: persistence (debt, pre-declared — a root stream)
ALL stealth state is transient today: alertLevel/grace, patrol route+cursor, last-known
memory, facing — a mid-engagement save reloads guards amnesiac. The accepted
`s6c-patrol-save` backlog item is the template: extend `SaveAiActorRecord` + codec + hash in
triple-lock, round-trip pinned. Independent of everything.

### Cross-cutting: reports
Debug snapshot + HUD lines BUILT (`NpcBehaviorDebugSnapshot`, `npcBehaviorDebugHud` — which is
ALSO a pending god-struct string-mirror cut; A5's decision receipts extend the same rows, so
SEQUENCE the two, never run them concurrently). Decision receipts (L6) and the battle report
(L8) extend these surfaces; `RuntimeSummary`/`CommandReplay` are the deterministic proof spine
a playtest/battle harness rides (⚠ generalizing RuntimeSummary churns `expectedSummaryText`
replay baselines — pre-declared).

### Cross-cutting: tuning
The s8 readout is the dashboard (escalation/decay/dwell tables + knob dump, real tick rate).
Constant-change slices re-run it to confirm feel. Quarantine law from s5: exactly ONE
tuning-coupled test per feature; plumbing tests pre-force `alertLevel=1.0`.

### Physics horizon (SOCKET only)
The kinematic kernels (`planPhysicsKinematicAabbMove`, `raycastPhysicsAabbs`) are the
interface. Future dynamics attaches BEHIND them; nothing above L0 may bind to physics
internals. Not designed here — reserved.

## 3. The DAG and the derived build order

**Edges are DEPENDENCIES only.** (v1.0's drawing mixed in scheduling preferences; this one
does not. If drawing and text ever disagree, the text rules.)

```
ROOTS (independent, fire in any order):
  A1  sound (L1)                      A2  persistence          A3  reasoning graph v1
  A1b light-field (optional; needs        (triple-lock debt)       (L4, derived nodes +
      only built vision — anytime)                                  runtime-side vocab header)

DEPENDENT:
  A3 ──► A4  travel cost (L5)
  A3 ──► A6  influence maps v1 (L7)          [A6 enriches A5's terms; NOT a prerequisite]
  A3 ──► A7  affordance vocabulary ⚠CROSS-LANE (authored nodes; Codex owns marker→node
             mapping; in-lane header is A3's)
  A3 (+A4 wanted) ──► A5  chess scoring v1 (L6, decision receipts, weight seams)
  A8a groundwork: independent (seed-widening + objective vocabulary) — anytime
  A5 + A7 + A8a ──► A8b  DECK FULL-DNA MINIMUM  [A7-stall fallback: fixture-authored graph]
  A1 + A5 (+A8b for cards) ──► A9  team layer
  A10 tuning: continuous from A1 onward
```

Topological order: **A1 / A1b / A2 / A3 / A8a in any order or parallel** (all rootless);
then **A4 / A6 / A7 in any order** (all need only A3); **A5 after A3** (take A4 first when
possible — travelCost makes scoring honest); **A8b after A7+A5+A8a** (fallback path if A7
stalls); **A9 after A1+A5** (partner-sync cards also want A8b); **A10 interleaves**.
Scheduling PREFERENCE (not dependency): fire A1 and A2 early — A1 has pre-named contracts and
completes the suspicion table; A2 pays the amnesiac-guard debt before layers pile more
transient state on top.

## 4. Pre-declared churn budget (no surprise rewrites)

| Future layer | Will churn (deliberately) | Precedent/rule |
|---|---|---|
| A1 sound | `NpcAlertStimulus` grows channels; `enqueueNpcBehaviorCommands` builds them; garden tests gain hearing scenarios; escalation TIMINGS may shift → the TWO product tapes pinning `kNpcEscalationWaits=24` re-baseline (`tests/smoke/product_gameplay_tape_smoke.cpp`, `tests/unit/product_gameplay_tape_runner_tests.cpp`) | s5 rule: tests-only re-baseline is planner-authorizable; N is MEASURED never guessed; passive/ghost receipts stay byte-identical; ReceiptBuilder/src/app product surfaces are cross-lane HARD-STOP |
| A2 persistence | `SaveAiActorRecord`+`SaveCodec`+`StateHash` lockstep growth; save/load + hash tests; **the repo's ONE literal hash pin re-baselines**: `fixtures/demos/first_room/expected_summary.txt` `state_hash` line (a demo-smoke golden; guards present ⇒ hash legitimately shifts) — once per hash-coverage commit, bisected + measured. Vulkan smokes verified pin-free (computed receipts only) | triple-lock; append-only enums; never shrink hash coverage to dodge a golden |
| A3 graph | new runtime/ai module + fixture growth; `PackageSessionSeed` emits derived nodes from anchors (in-lane shared ground, by this order); notebook-projection wiring = separate planner-brokered follow-up slice (src/app); when next_work Stream 3 lands, A3 gains ONE portal-stitch slice | greenfield; keep O(n) descriptor-scan patterns OUT of per-tick loops |
| A4 travel | patrol/investigate/return destinations become route-node-fed; straight-line stall tests re-pin; NEW INTENT kinds (if any) touch the three dispatchers incl. the MANUAL if-chain `buildNpcBehaviorCommand`; route state transient — NO triple-lock growth | motor interface unchanged (point-moves) |
| A5 chess | the overlay chain gains a rung — precedence is ORDER-LOAD-BEARING (combat > investigate > patrol, gated on intent==Wait); `NpcBehaviorProfile` gains weight columns; decision receipts extend snapshot rows; garden sub-combat intent pins re-pin (break-contact investigate / dwell / patrol-resume); SEQUENCE against the pending npcBehaviorDebugHud string-mirror cut | s5 precedent: graded overlay was made the SOLE path, no opt-in flag — same rule here |
| A6 influence | hoists the per-tick `SpatialSurfaceSet` collider rebake (Session.cpp) — a timing-visible perf change; garden tick-cap pins re-measured | declare before, measure after |
| A7 affordance | ⚠CROSS-LANE, re-ruled v1.6: the contract travels as ANCHOR-KIND STRINGS on `RoomAnchorAsset.kind` — `docs/affordance_vocabulary_v0_1.md` IS the contract artifact (no shared header needed). AI half (fleet): `markerToReasoningNode` string→kind mapping in runtime/ai + the `monster` entity-seeding touch in `PackageSessionSeed.cpp` (named shared-ground touch). Codex half: emit-strings-only (glyph table, `anchorKindForMarkerTag`, authoring reference §1/§4 + inertness-note amendments). Resolves N/M collapse (ordering-sensitive: AI half first or same window — warden_vault authors M) + T/R Stream-4 resolution (T inert placement affordance; R already has LIVE reset-to-spawn gameplay — only its deck semantics are future). Creative GameplayMarker emission of the wire strings: DEFERRED to a later Codex order | one vocabulary, authored once; planner brokers; the CONTRACT DOC is the contract, wire strings append-only |
| A8a groundwork | `assignNpcBehaviorProfiles` widened to carry patrol/facing (the seed-rebuild trap, PackageSessionSeed.cpp — in-lane shared ground by this order); objective conditions grow past `PlayerHasItem`; the TWO hardcoded outcome rules in SessionTick.cpp subsumed into data | deliberate schema growth, not drift |
| A8b deck kernel | new module (cards/budget/validator/placement/report); `RuntimeSummary` generalization churns replay `expectedSummaryText` baselines | fallback: fixture-authored graph if A7 stalls |
| A9 team | target selection replaces hardcoded player-slot-0 in `enqueueNpcBehaviorCommands`; profile catalog goes data-driven (churns Session.cpp per-tick rebuild, ProjectionRefresh, PackageValidator, FixtureScenarioLoader, 4+ test binaries — the catalog is rebuilt EVERY tick today) | the catalog churn list is known and finite; do it once, in one slice |

## 5. Reserved sockets (names fixed now; bodies later)

Already reserved IN THE TREE: `AiActorState.deterministicPolicy` (seeded randomness),
`behaviorProfileId` open id-space, `NpcAlertStimulus` (the stimulus bus — sound/light land as
fields here), `ObjectSoundProfile` tags (emission landing pad), the overlay-chain insertion
pattern (`reconcileAlertBand → maybeApplyInvestigate → maybeApplyPatrol` — new rungs slot
between, same intent==Wait gate discipline).

Reserved BY THIS DOCUMENT (inhabit these names when the layer lands; do not invent rivals):
`SoundEvent`, `SoundPerceptionConfig`, `SoundPerceptionResult` (pre-named in the stealth plan;
integration per the §2 L1 ruling — fields on the bus, existing `npcStepAlert`, NOT the plan's
`npcStepAlertBehavior` signature, which is hereby retired) · `LightField`, `PointLightSample`,
`lightLevelAt` · `ReasoningGraph`, `ReasoningNode`, `ReasoningEdge`, `ReasoningNodeKind`,
`ReasoningEdgeKind`, `buildReasoningGraph(room…)` (runtime-side header; notebook includes it) ·
`travelCost(edge, config)` (v1 as landed — actor-free edge cost; actor-bearing terms land with
the slope/personality tuning slices) + `planRoute(graph, colliders, from, to, config)` ·
`GuardDecisionReceipt` · `InfluenceMap`, `InfluenceChannel` ·
`EncounterCard{requires, provides, cost}`, `EncounterBudget`, `EncounterValidator`,
`EncounterBattleReport`, `dealEncounterHand` · `markerToReasoningNode` (the A7 mapping —
re-ruled v1.6: runtime/ai, the AI-lane half; Codex emits wire strings only) ·
`NpcPersonalityWeights` (columns on `NpcBehaviorProfile`) · `selectNpcTarget` ·
persistence: `SaveAiActorRecord` grows `alertLevel/grace*/patrol*/lastKnown*/facing` fields
per the s6c-patrol-save template (route state explicitly NOT among them — transient by design).
All names grep-verified collision-free in the tree at v1.1.

## 6. Refusals (what this lane will not do)

- **No omniscient tracking.** AI reads player position only through perception events (a
  confirmed sighting, a heard sound's origin); between events, memory and probability only.
  (Perception mechanics necessarily test against true position — that is not tracking.)
- No perfect AI: no deep search past 2-ply, no solver aesthetics; readable-imperfect wins.
- No navmesh / per-tile pathfinding; the graph is meaningful-positions only.
- No second memory system, second stimulus path, second alert ladder, second command pipe —
  extensions land IN the existing spines (stimulus fields, overlay rungs, profile columns).
- No ambient randomness; no wall-clock; no reordering serialized enums.
- No class-tree AI architecture (TDM DO-NOT-REPRODUCE list stands).
- **Cross-lane discipline, scoped precisely:** `creative/**` is never touched. ReceiptBuilder
  and src/app product/UI surfaces are out-of-lane EXCEPT by explicit planner order (the
  sd-series precedent). `src/app/iggy3d/world/` seed plumbing (PackageSessionSeed,
  NpcProfileAssignment) is in-lane shared ground FOR THE STREAMS THIS MAP NAMES (A3, A7's
  monster-seeding touch, A8a) — a reviewer should not stop-flag those; anything beyond the
  named touches still stops.
- No companion/priestess AI in v1.x (reserved in §1 — a future READER of L4/L6, never a
  parallel stack).
- No physics redesign inside AI streams; the kinematic kernel interface is the wall.

## 7. The feedback rule + operations + version log

**The map is authoritative; the cutter corrects the map.** Any slice discovering the map wrong
STOPS-and-flags (s5's tape discovery and sd3's phantom pause-path are the precedents); the
planner amends, bumps the version here, and re-fires.

**Operations:** slice records live box-side at `~/iggy3d-bus/bucket/slices/<id>.md`; every
slice header cites its coordinates — `A<stream>, slice k of N — L<layer>, map v<X.Y>`. On every
version bump the planner re-publishes this map to the bucket `PLAN.md` so the reviewer specs
against the CURRENT version, never a stale one. This file must be COMMITTED to trunk (human's
call) so the box clone (`~/iggy3d-clean`) carries it; until then the bucket mirror is the
fleet's copy.

- v1.0 (2026-07-02): initial fusion — stealth plan + deck seed + s5–s8 records + 3-reader recon.
- v1.1 (2026-07-02): 3-critic pass. Retired the phantom `npcStepAlertBehavior` socket (sound =
  fields on the stimulus bus; plan signature superseded). DAG redrawn: dependencies only —
  A3 is a root (A1/A2→A3 edges removed as false), A7→A5 and A5→A6 edges removed as false,
  explicit A1→A9 (ally-call = SoundEvent) and A7→A8b edges added; A8 split into A8a
  groundwork (independent) + A8b kernel with an A7-stall fixture fallback. Fixed: two (not
  three) escalation tapes, named; `buildNpcBehaviorCommand` if-chain warning (−Wswitch does
  not cover it); second hardcoded outcome rule (collect_gold_key → DemoComplete); A4 route
  state ruled transient-by-design; reasoning-vocab header home ruled runtime-side; notebook
  wiring named as a brokered follow-up; multiroom stitch given an owner (A3, one slice);
  seed-plumbing lane scoped in refusals; companion-AI identity hole answered (reserved, not
  silent); omniscience refusal tightened to perception-events wording.
  Known stale sibling: docs/next_work.md still says slice-5 integration is "next" (s5–s8 have
  landed); update it when trunk settles.
- v1.6 (2026-07-02): A7 re-ruled for dispatch (critic pass on the cross-lane package):
  the contract = `docs/affordance_vocabulary_v0_1.md` with anchor-kind STRINGS on the wire
  (no shared header); `markerToReasoningNode` moves to the AI half; `PackageSessionSeed`
  monster-seeding named as A7 shared ground; M de-collapse is ordering-sensitive (AI half
  first — warden_vault exposure); Stream-4 resolution corrected (R is NOT inert — live
  reset-to-spawn gameplay exists; T inert; both become deck affordances); creative
  GameplayMarker emission deferred. A8a commit-2 seam pre-ruled: outcome mapping =
  session-level TRANSIENT table rebuilt on load (ObjectiveRecord-field seam FORBIDDEN this
  slice — objectives persist+hash via the SaveObjectiveRecord mirror; a compiling-but-
  unpersisted field is the trap).
- v1.5 (2026-07-02): A5 COMPLETE (`55916b5f` kernel, `49a2d44c` rung; 192/192). L6 landed:
  `chooseSearchNode` (1-ply, signed-factor `GuardDecisionReceipt`, `excludedNodeId`
  suppression), `NpcPersonalityWeights` neutral columns on the profile (the A9 seam),
  `PlannedRoute.totalCostMeters` additive, the SEARCH rung (combat > investigate > SEARCH >
  patrol; trigger `band∈{3,4} && intent==Wait && !hasLastKnownTarget && graph non-empty`;
  COLD-MEMORY ruling — clearMemory's surviving stale sample steers suspicion). CORRECTIONS:
  the A5 churn row's "decision receipts extend snapshot rows" is DEFERRED past the
  npcBehaviorDebugHud string-mirror cut — receipts live in a transient AiActorState slot read
  by `guard_decision_readout`; V1 search shape = alternation between top nodes (searched-node
  SET = pre-named L3 extension). A10 DIAL LOGGED: at reference constants cold-trail suspicion
  (~0.01) is dwarfed by strategic value (20) — falloff/half-life tuning will re-pin exactly
  the ONE quarantined a5s1 garden test.
- v1.4 (2026-07-02): A4 COMPLETE (`bd8230f1` kernel, `e0c7625c` wiring; 190/190). Signature
  correction ratified: `travelCost(edge, config)` actor-free v1 + `planRoute(graph, colliders,
  from, to, config)` taking BAKED colliders (buildReasoningGraph bakes internally; planRoute
  never does). One shared `reasoningSegmentBlocked` ray discipline for senses/edges/routes.
  Route state transient on AiActorState keyed (intent, plannedForDestination); interim nodes
  use the patrol stop/arrive pair; chase stays direct; guards flank island-blocked
  investigate/return destinations (measured 15 ticks vs infinite stall). Zero existing
  re-pins — makeGardenSession stays graphless by design.
- v1.3 (2026-07-02): a3s1 landed (`e247af24`, 188/188) — vocabulary + anchor/waypoint nodes +
  walkable edges, garden pinned. Cutter-carried flag ruled: `ReasoningNodeKind::reference`
  (14th kind) CONFIRMED — provenance-neutral entity-anchor kind; semantic spawn-region kinds
  arrive with A7. A1+A2 roots done earlier this day (hearing kernel, footsteps→investigation,
  durable stealth state); chokepoints/doorways deferred to pre-named a3s1b (doorways first
  from RoomOpeningAsset).
- v1.2 (2026-07-02, the cutter corrected the map — first live firing of the feedback rule):
  a2's builder hard-stopped on a false "zero literal hash pins" premise; truth: the repo has
  exactly ONE — the first_room demo golden (A2 churn row corrected; once-per-commit
  re-baseline authorized; Vulkan smokes verified pin-free on the Mac). SEQUENCING AMENDMENT:
  parallel work on a DISJOINT slice while another is hard-stopped is authorized, provided the
  reviewer proves disjointness by file-set (a1s1 precedent — verified clean during the a2
  stop). a1s1 landed: `NpcSoundPerception.{hpp,cpp}` kernel + inert stimulus fields,
  commit `ae1e4bc8`, 187/187.
