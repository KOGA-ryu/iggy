# The Movement & Abilities Maximum — destiny document v1.1

> Planner-authored 2026-07-03. The lane's SECOND WING: parkour movement, the ability engine,
> and NPCs USING BOTH. Sibling to `docs/ai-lane-maximum.md` (v1.10) — same laws: slices cut
> FROM this map, the cutter corrects the map, receipts never lie. Fused from a 3-reader recon
> at trunk ~c932be07 + the user's world rulings (gravity per dimension; the wild-magic thief;
> "i don't really see npcs using movement tech in games"); hardened by a 2-critic pass
> (v1.0 → v1.1: three blockers — runtime-first verbs, the MA4 trigger ruling, crouch-sneak).
> Lane law: Claude + the box fleet (runtime movement/ability/AI; `gameplay/Controller.cpp`
> ONLY by explicit order); Codex owns creative authoring of parkour surfaces
> (WallRunSurface is already in his descriptor table).

---

## 1. Identity → destiny (what the recon revealed, and what it demands)

**THE CONSTITUTIONAL FINDING — three motion worlds:**
1. Horizontal ground movement: per-input-step Move COMMANDS through admission →
   `executeMovement` (teleport-validate). Deterministic, replayable, receipted — LAWFUL.
2. ALL vertical/parkour motion (jump arcs, gravity ×18, coyote/buffer/cut, wall-run,
   wall-jump): app-side `gameplay/Controller.cpp`, analytic integration per input frame,
   DIRECT world writes via `setProductPlayerPosition` — **bypassing admission, outside the
   command log, invisible to replay**, state in god-struct window fields. Feel is GOOD;
   constitution is not. LAWLESS.
3. Traversal (Vault/Clamber/WireWalk): runtime-side instant teleports
   (`MovementTraversal` + `MovementTraversalSlots` from authored tags) — **validated and
   data-driven but COMMAND-LOG-INVISIBLE** (its one caller is the Controller, direct world
   mutation + manual re-hash). LAWFUL-ADJACENT, not lawful.
Plus TWO ORPHANS that are the destiny already written: `executeKinematicMovement`
(velocity·dt runtime mover — tests + the collision-probe tool only) and **`PlayerMotor`**
(a real phase-machine motor — Grounded/Airborne/WireWalk, verticalVelocity, terminal
velocity, gravity, jump — debug-only, zero product callers, and with a DIFFERENT jump
impulse than the live tuning: 5.8 vs 15.8).

**DESTINY:** ONE runtime motor (PlayerMotor lineage), fixed-tick, fed by intents through
admission, serving players AND NPCs, with per-dimension profiles as data. The Controller
becomes input-shaping (buffers/coyote timing are FEEL and may stay app-side) — not a physics
owner. Converged INCREMENTALLY (the 2,206-line controller test + receipt surface forbid a
big-bang) — and **the lawless layer NEVER GROWS: every NEW continuous verb lands
runtime-first on the PlayerMotor lineage as a new phase** (ledge-hang, climb, slide are
phases — exactly its shape), each a down-payment on MA7. Only triggers/input-shaping may
touch the Controller.

**Stealth couplings (identity, non-negotiable):**
- **SNEAK IS A STANCE, and it lands FIRST (rides MA1):** today the only way to be quiet is
  standing still (footstep loudness = base + perMeter × displacement, nothing else).
  Crouch-walk = a stance with a speed multiplier + a footstep-LOUDNESS multiplier — the
  exact seam armour later multiplies into. **The mechanism already exists as a stub:**
  `MovementMode` (Walk/Tactical/Reposition — carried on every MovementRequest, branched on
  by NOTHING) becomes the stance carrier (a Sneak mode; the stub finds its purpose).
  PlayerCrouch input exists (NOTE: mode-filtered — creative-fly consumes it as descend; the
  gameplay wiring must respect `mapMakerConsumesGameplayAction`).
- **Airborne motion is SILENT to guards** (the tree's ONE sound emitter is the LocalPlayer
  Move branch in SessionTick). The landing THUMP is REQUIRED but has a determinism hole:
  landings are detected in the lawless frame-rate layer — a thump pushed from app frame code
  is unlogged and replay-divergent. RULING: the thump is DEFERRED (named, not dropped) until
  landing detection is runtime-side (MA7-adjacent motor work); airborne silence is a
  DECLARED known lie until then. Casting noise is NOT deferred (casts are commands — lawful
  emission point exists; rides MA3a).
- Every new verb/ability declares its noise or its silence explicitly.
- Determinism constitution + the triple-lock + append-only enums (inherited).
- **Churn multipliers (corrected, pre-declared):** a new `CommandKind` touches **~8 sites**
  — queuesForTickExecution, CommandReplay queued/immediate kind lists, requiresActor/
  requiresXTarget, CommandLog payload validation, SaveCodec enumText+parseEnum,
  CommandAdmission per-kind validation, and **SessionTick's execution dispatch (an if-chain
  whose fall-through returns InvalidState — bricks the tick, compiler-silent: the worst
  trap)**; StateHash only if the kind adds payload. A new `AiIntentKind` = 4 sites. Ability
  actor-state is hash/save-pinned FIELD-BY-FIELD (generalizing = ONE versioned schema change,
  golden re-baselined once). `movementDistanceMeters` is hashed+saved.
- **LIVE BUG (MA1 fixes it, declared):** dash distance 18.5×0.18 = 3.33 m EXCEEDS the 3.0 m
  admission limit — dash bookkeeping reports accepted before the Move can be rejected.

## 2. The layer stack

```
M0  Motion substrate   PART   3 worlds today; destiny = ONE motor; lawless layer FROZEN
M1  Dimension profiles ABSENT gravity/tuning as PER-WORLD DATA + the SNEAK stance
M2  Parkour verbs      PART   built: jump/coyote/buffer/cut/wall-run/wall-jump/dash/
                              vault/clamber/wire-walk · absent: sneak-walk (MA1!), slide,
                              ledge-hang, continuous climb, ladder · new verbs RUNTIME-FIRST
M3  NPC movement       ABSENT capability classes → graph climb edges → traversal INSIDE
                              Move-command execution (no new CommandKind in v1)
B0  Ability engine     PART   pipe BUILT end-to-end, n=1, one global slot — generalize to N
B1  Producers          SPLIT  MA3a player cast (cuttable NOW) · MA3b NPC caster (needs MA2)
B2  Damage types +     ABSENT bare int32 today; the interaction matrix's spine
    status skeleton
—   Sound coupling     LAW    every verb declares noise; sneak rides MA1; thump deferred-named
—   Authoring          ⚠CROSS ascii tags live ('J'→wall_jump); creative WallRunSurface (Codex)
```

### M1 — Dimension profiles + the sneak stance (the FIRST stream)
- **`MovementDimensionProfile`** (reserved): the per-world movement envelope — the FULL
  `ProductGameplayMovementTuning` STRUCT (~26 numeric fields + 3 profile-name ids; do NOT
  size from the 21-entry HUD descriptor table — it omits the wall-jump quad and
  inputStepSeconds) PLUS the runtime fields (`movementDistanceMeters`, slope-band span,
  `MovementParams`) — selected PER SCENARIO/WORLD by a named profile id (`earth_standard`,
  `giant_lowgrav` first two rows), following the fixedTickRateHz TOML→RuntimeConfig pattern;
  the window's already-mutable tuning copy is the app-side injection point.
- The GIANT row: lighter gravity, higher jump, longer wall-runs — the user's "all movement
  systems tuned up" as ONE data row.
- **Hash/save ruling (annotated — the sibling law binds):** profile-ID persists; the derived
  numbers follow the **a9s1 carried-across-load precedent** (derived state CARRIED across
  the swap, never rebuilt-on-load — the load path has no scenario access). NEVER shrink hash
  coverage (`movementDistanceMeters` stays hashed — the A2 golden-dodge rule); old replay
  logs must rebuild identical admission limits. The exact mechanism is the MA1 order's
  decide-now ruling.
- **SNEAK (in this stream):** `MovementMode` gains the stance semantics — Sneak mode =
  speed multiplier + footstep-loudness multiplier (named profile fields); PlayerCrouch
  (gameplay-mode, mode-filter respected) toggles/holds it; automation verb. The thief gets
  quiet feet the same week the giant dimension gets low gravity.
- Folds in: the dash/admission bug fix (dashSpeed×dashDuration validated ≤ the profile's
  movement limit — coupled fields validated together).
- Falling damage: RESERVED here by name (computed at the landing event from fall speed ×
  profile lethality fields) — BODY arrives with the thump/motor work, not MA1. Swimming:
  REFUSED until water exists in the tree.

### M2 — Parkour verbs (runtime-first law)
Built verbs stay where they are (frozen, param-fed by M1). NEW verbs — slide, ledge-hang,
continuous climb, ladder — land as **PlayerMotor-lineage PHASES** (runtime, deterministic,
testable headless), with Controller touching only their input triggers. Each verb: authored
affordance (tags/slots — Codex authors surfaces) + runtime phase + M1 profile fields +
SOUND declaration + receipts + a MovementTestLab lane. Every verb is a down-payment on MA7.

### M3 — NPC movement capability (the novel one; MA4)
- **Capability CLASSES, not per-guard configs**: a small enum (grounded / climber / leaper /
  flier-reserved), each a `TravelCostConfig` row (+∞ multiplier = edge unusable); profile
  carries its class. The config threads through the **FOUR default-{} call sites** that must
  agree on reachability: GuardDecision (search routing), Session (route-following),
  InfluenceMap (Dijkstra weights), EncounterPlacement (validator reachability).
- Graph edges GROW: `climb` edges emitted where traversal slots/tags connect nodes (the
  buildReasoningGraph edge loop; authored tags already reach `CollisionSurfaceView`).
- **THE TRIGGER RULING (constitutional — v1.0's 'a CALL' was a violation):** NPC traversal
  executes **INSIDE Move-command execution** — when a route leg crosses a climb edge and the
  ground move is blocked, the runtime move executor invokes the traversal mechanic
  deterministically as part of executing that logged Move command (same command → same
  execution → replay-sound; receipted via MovementResult). NO bare calls from AI code; NO
  new CommandKind in v1 (`CommandKind::Traverse` is RESERVED for the MA7-era unification,
  where the player's traversal also becomes lawful).
- The payoff: a guard that vaults the crate you thought was cover; monkey-folk taking climb
  routes humans can't (race capability classes — the content maximum's senses pattern
  applied to motion).

### B0 — Ability engine: n=1 → N (MA2)
The pipe (admission → SessionTick → AbilitySystem → ProjectileSystem → CombatSystem) is
BUILT and test-pinned. Generalize the data:
- `CommandAbilityKind`/`AbilityId` grow (append-only); `AbilityDefinition` is already the
  parameter-row shape — the registry becomes a TABLE (count static_assert, data not switch).
- **Actor state goes slot-based** — ONE versioned schema change (a2 four-way discipline,
  golden re-baselined once, declared) — **and the schema's one version INCLUDES the
  wild-magic sockets**: a HASHED surge-seed field on ability actor state + surge-table rows
  reserved on `AbilityDefinition` (the constitution's seeded-randomness law; content maximum
  fills the rows; no second schema version later).
- **Per-actor projectile slots** (today: ONE projectile in flight per SESSION — untenable
  the moment player + archer coexist).
- The cast-time surface wart (CORRECTED understanding): in-flight projectiles ALREADY
  collide honestly; the wart is that admission and execution feed `castAbility` a fabricated
  EMPTY surface set just to pass its non-null gate — cast-time validation can never do a
  real muzzle/world check. Thread the real surfaces to the cast.
- Engines stay FEW: projectile (built), melee-strike (Attack), self/status (arrives with
  MA6). The fire bolt IS ArcaneBolt with fire flavor the day damage types exist.

### B1 — Producers (split; the false MA2 edge corrected)
- **MA3a — player cast (INDEPENDENT, cuttable now):** CastAbility is fully piped; only
  producers are missing. Wire `PlayerCast` (dead input action) → Controller → CastAbility
  command (targeting = look direction); automation verb **`gameplay.cast` (alias
  `game.cast`)** per the registry convention; the cast EMITS SOUND (data per ability — the
  lawful command emission point). The wild-magic thief's first spark lands EARLY.
- **MA3b — NPC caster (needs MA2's per-actor slots):** new `AiIntentKind` (4-site churn) +
  a decision rung — v1 rule-based (ranged profile + LOS + range window + cooldown ⇒ cast;
  the ARCHER card becomes a real ranged guard). **SEQUENCING LAW: MA3b's rung insertion and
  the AI map's a9s3/a9s4 NEVER run concurrently** (both churn the overlay chain and
  enqueueNpcBehaviorCommands; precedence — where CastAbility sits relative to
  combat/investigate/search — is ruled by an AI-map bump BEFORE the slice cuts).

### B2 — Damage types + status skeleton (MA6)
- `DamageKind` (append-only from birth) replaces bare int32; a status framework SKELETON
  {id, duration ticks, tick effect, on-apply/on-expire} — enough for burning/wet/oiled rows;
  the CONTENT maximum fills the matrix. Statuses touch senses (blinded/deafened remove a
  gate) — the stealth column from day one.
- `factionId` GRADUATES: it already blocks same-faction friendly fire (CombatSystem) — the
  relations table (world lore: ally/neutral/hostile/trades) extends it; coordinate with
  a9s3/a9s4, don't duplicate.

## 3. Streams and order

```
MA1   Dimension profiles + SNEAK stance + dash fix            ← FIRST (identity + user ruling)
MA2   Ability engine n→N (+ wild-magic sockets, ONE schema)
MA3a  Player cast + gameplay.cast + cast noise                ← independent, early payoff
MA3b  NPC caster rung (needs MA2; sequenced vs a9s3/a9s4)
MA4   NPC movement (capability classes + climb edges +
      traversal-inside-Move; owns its determinism ruling)     ← independent of MA1-3
MA5   New parkour verbs, RUNTIME-FIRST (slide, ledge-hang,
      climb, ladder — PlayerMotor phases + gym lanes)
MA6   Damage types + status skeleton                           ← unlocks the content matrix
MA7   (pre-named, unscheduled) Motor unification — landing
      thump + falling damage + CommandKind::Traverse +
      impulse reconciliation land HERE
DAG: MA1 ∥ MA2 ∥ MA3a ∥ MA4 all independent · MA3b ← MA2 (+ AI-map sequencing) ·
MA5 ← M2 law (motor-phase pattern, benefits from MA7 direction but doesn't wait) ·
MA6 independent · MA7 last.
```

## 4. Refusals
- The lawless layer NEVER GROWS (new continuous verbs are runtime phases — no exceptions).
- No big-bang motor migration (MA7 is pre-named, opt-in-gated, LAST).
- No per-guard movement configs (capability CLASSES only).
- No bare AI calls into world mutation — NPC verbs execute inside logged commands.
- No silent verbs — declare noise or declare silence (the thump's deferral is DECLARED).
- No status/damage CONTENT here (rows are the content maximum's; this wing builds spines).
- Swimming: refused until water exists. Falling damage: reserved, not refused (MA7-adjacent).
- Cross-lane: creative/** never; Controller/gameplay app files ONLY by explicit order
  (named in MA1/MA3a/MA5 trigger scopes); never shrink hash coverage.
- The feedback rule + version log (inherited verbatim from the AI map).

## 5. Version log
- v1.1 (2026-07-03): 2-critic pass. Blockers fixed: new verbs RUNTIME-FIRST (PlayerMotor
  phases — the lawless layer frozen); MA4 trigger ruled (traversal inside Move-command
  execution; world 3 relabeled lawful-adjacent; CommandKind::Traverse reserved for MA7);
  CROUCH-SNEAK added (MovementMode stub becomes the stance carrier; rides MA1). MA3 split
  a/b (player cast was falsely gated behind MA2); landing thump deferred-with-name
  (determinism hole); wild-magic surge sockets reserved into MA2's one schema version;
  churn counts corrected (~8-site CommandKind incl. the SessionTick dispatch trap; FOUR
  TravelCostConfig sites; 26+3-field tuning struct — never size from the HUD table);
  cast-time wart re-described (flight collision was always honest); falling damage
  reserved / swimming refused; factionId already does friendly fire — it graduates.
- v1.0 (2026-07-03): initial fusion from 3-reader recon. Constitutional finding recorded
  (three motion worlds; orphaned PlayerMotor; divergent jump impulses 15.8/5.8; airborne
  silence; dash>admission bug). Streams MA1–MA7 derived.
