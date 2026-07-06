# IGGY3D MASTER DESTINY DOC — the game-wide map slices are cut from

Version: v1.0 (2026-07-06). Master synthesis of the five lane maximums, ordered around the FIRST PLAYABLE VERTICAL SLICE. Reconciled against code at **239/239 tests green**, and against the real movement maximum v1.4 (pulled onto this branch from the box — see §9). Cut all cross-lane slices from this map; each lane's own maximum is the deeper reference behind it.

Lane maximums this binds: `docs/ai-lane-maximum.md` (v1.8), `docs/movement-abilities-maximum.md` (v1.4, now on-branch), `docs/physics-lane-maximum.md` (v1.0, new), `docs/foundational-lane-maximum.md` (v1.0, new), `docs/creative_mode/creative_editor_algorithms_map_v0_1.md`.

---

## 0. THE THROUGH-LINE

**ONE authored room is the pivot the whole game turns on.** Creative bakes it to a single `RoomAsset` whose two payloads are read by two lanes at once: `RoomSpatialSurface[]` (physics footing + LOS truth) and `RoomAnchorAsset.kind` wire-strings (AI reasoning nodes). The thief traverses that room under gravity=data (movement) on collision/-provided footing (physics); one SHIPPED stealth guard reacts (AI) using physics raycasts for LOS; the composed `SessionTick` (foundational) runs both in one tick; the thief logs a `ReconIntel` packet at the objective; that packet transfers to the knight+priestess duo.

Every lane's stack is already SHIPPED. **The first playable slice is not new engines — it is wiring three seams shut**: (1) affordance-string emission on both bake paths, (2) the ReconIntel/notebook projection, (3) the sneak player-input binding. The two absent maximums are now resolved (movement pulled from the box; physics newly consolidated), so their slices can be cut from a real map.

---

## 1. THE VERTICAL-SLICE SPEC — each lane's one link

| Lane | Contribution | Seam it must close for the slice |
|---|---|---|
| **Creative** | Authors the room; bakes the single `RoomAsset` (surfaces + affordance anchors) both other lanes read | EMIT affordance wire-strings on BOTH bake paths (ascii `monster_spawn`→'npc' AsciiRoomToRoomAsset.cpp:344-345; creative-doc CoverPoint/PatrolNode→'navigation' RoomBake.cpp:247) |
| **Physics** | Footing + LOS truth from the RoomAsset via SHIPPED collision/ (`CollisionQuery` + `raycastPhysicsAabbs`) | None new — verify bake roles + one lab receipt. Keep `usePhysicsMovePlanner=FALSE` |
| **Movement** | Thief traversal: run/sprint/jump-under-gravity/Clamber/Vault (all SHIPPED) | WIRE SNEAK's player input — the sneak *stance* is shipped (MA1); the live `PlayerCrouch`→sneak binding is the missing consumer (see §9) |
| **AI** | The reacting stealth guard (LOS cone→alert→patrol→memory, LIVE) + the intel projector | Build `GuardReconObservation`; wire `menu/Notebook.hpp` to include `ReasoningGraph.hpp` + a producer for `ProductNotebookReconPage` |
| **Foundational** | Composes thief+guard in one tick; owns the `ReconIntel` packet + its transfer to the duo | Build the runtime `ReconIntel` truth packet (Notebook.cpp is UI strings only) + the transfer seam |

---

## 2. THE CROSS-LANE DAG (dependency order toward the slice)

```
CREATIVE: Author RoomAsset ────────────────┬─────────────────────────────┐
   │                                        │                             │
   └─ Emit affordance wire-strings          │                             │
        (both bake paths) ───────────┐      │                             │
                                     │      ▼                             ▼
PHYSICS: buildSpatialSurfaceSet ─────┼──► CollisionQuery footing    Projection loaded=true
   │  → CollisionQuery / querySegment │      │                             │
   └─ bakePhysicsAabbColliders ─► raycastPhysicsAabbs (guard LOS)   FOUNDATIONAL: present gate
                                     │      │                             (FramePresenter:782)
MOVEMENT: thief traversal verbs ◄────┘      │
   │  (run/jump/vault, gravity=data)        │
   └─ SNEAK player-input binding (new) ┐     │
                                     ▼      ▼
AI: Guard runs shipped stealth loop over the RoomAsset ◄── (needs: affordance strings + LOS + a moving target)
   │
   └─ GuardReconObservation projector ──► FOUNDATIONAL: ReconIntel packet ──► Intel TRANSFER thief→duo
                    │                              (session-owned, hashed)          │
                    └─► AI: Notebook producer                                       └─► AI: duo consumer acts
                        (Notebook.hpp includes ReasoningGraph.hpp)

FOUNDATIONAL: SessionTick composes thief-GROUND-movement + guard-AI in ONE tick over one SpatialSurfaceSet
              (thief VERTICAL/parkour runs app-side lawless — replay-divergent; see §3 + §9)
```

The deepest chain (the slice's payoff) is **GuardReconObservation → ReconIntel packet → notebook producer / transfer → duo consumer** — three unbuilt pieces split across AI and foundational.

---

## 3. THE SEAM CATALOG (the highest-value output — where lanes actually meet)

Reconciled `seamsProvided` × `seamsNeeded`. **exists** = built & live; **partial** = plumbing present, contract/field/validation missing; **missing** = a needed seam nobody provides yet.

### EXISTS (the slice can lean on these today)
- **Footing surfaces** (Creative→Physics→Movement): `RoomSpatialSurface[]` → `buildSpatialSurfaceSet` → `CollisionQuery.sampleSurfaceHeightAtOrBelow`/`querySegment`. *physics-max §PH1.*
- **Guard LOS raycast** (Creative→Physics→AI): `bakePhysicsAabbCollidersFromSpatialSurfaces` → `raycastPhysicsAabbs` (Session.cpp:1096-1105). *physics-max §PH-LIVE.*
- **Traversal affordance slots** (Creative→Movement): RoomAsset affordances → `buildMovementTraversalSlotRegistry`. *movement-max §M2 / traversal-inside-Move.*
- **Single transform write + collision contract** (Movement↔Physics): `executeKinematicMovement(collisionSurfaces)` → `MovementResult`. *runtime_movement_policy.md:121-127.*
- **Per-tick composition** (Foundational↔ALL): `runSessionTick(SessionTickInput)` + `enqueueNpcBehaviorCommands`. *foundational-max §P1.*
- **Stable per-surface owner id** (Creative→Physics→Notebook): source ids → `runtimeOwnerStableNames`. *physics-max §7.*

### PARTIAL (built one side, contract or field missing)
- **Patrol route waypoints** (Creative→AI): `patrolWaypoints` span of `buildReasoningGraph` is consumed today FROM scenario TOML; authored `PatrolNode`-ordering→waypoint emission + flood-fill loop-validation not built.
- **MovementParams↔physics thresholds** (Movement↔Physics): both sides have radius 0.30/height 1.80/slope 40° but the docs never pinned they must AGREE — silent-divergence hazard.
- **Thief movement state for AI/intel** (Movement→AI/Notebook): `ProductMovementProofPacket` is complete EXCEPT the `sneaking` field (no sneak player-input consumer); nothing consumes the packet as intel yet.
- **Present/freeze gate** (Creative→Foundational): `FramePresenter.cpp:782` gate on `scene.room.loaded` is SHIPPED; unverified that the SLICE room's projection reports loaded=true.

### MISSING (needed-but-unprovided — the slice's real work)
- **Affordance anchor wire-strings** (Creative→AI): AI READER is LIVE (`ReasoningGraph.cpp:35-56`); AUTHORING EMITTER is not — ascii collapses `monster_spawn`→'npc', creative-doc bake maps CoverPoint/PatrolNode→'navigation' (dropped). **THE one live seam break, VERIFIED in code.** Two breaks on one seam (ascii AND creative-doc); the algorithm map's GAPS section only flagged the ascii side.
- **GuardReconObservation** (AI→Foundational/Notebook): all durable inputs exist; the projector struct is unbuilt.
- **Notebook renders live guard state** (AI→menu/Notebook): `Notebook.hpp` does NOT include `ReasoningGraph.hpp`; `ProductNotebookReconPage` has ZERO production producer (VERIFIED).
- **Runtime ReconIntel packet** (AI↔Foundational): no session-owned intel truth; `Notebook.cpp` is UI draw-strings only. **Biggest slice-end gap.**
- **Intel TRANSFER thief→duo** (Foundational→AI): foundational owns the carry; AI's duo-side consumer is RESERVED, not designed.
- **Parkour determinism** (Movement↔Foundational): the thief's vertical/parkour motion (jump/vault/wall-run) runs app-side in the lawless Controller — direct world writes, **invisible to the command log, replay-divergent** (movement's three-motion-worlds constitution, §9). The slice's save/replay gates (steps 10, 12) hold for the *command stream + the ReconIntel packet*, NOT for the exact parkour trajectory. Acceptable for slice 1 (the intel packet is what must survive replay); closed permanently only by movement's motor-unification (MA7, last). **Declared hole — do not over-claim "the whole slice replays deterministically."**

### RESOLVED THIS PASS
- **Movement maximum on-branch** (Movement↔Planner): was absent; **pulled from the box (`~/iggy3d-clean`) onto this branch, now at `docs/movement-abilities-maximum.md` (untracked, pending commit)**. Sneak/MA4 can now be cut from the real map. Reconcile its shipped/planned columns against code (§9).

---

## 4. THE UNIFIED FIRE-ORDER (smallest path to a PLAYABLE slice)

Interleaved by real dependency, smallest-unlock first. `[lane]` in brackets.

1. **[Planner]** Commit the pulled `docs/movement-abilities-maximum.md` v1.4 on-branch and reconcile its columns against code: MA1 "sneak shipped" means the *stance + automation verb* shipped, but the live `PlayerCrouch`→sneak binding is unwired (§9); confirm MA4 nomenclature (code ships Vault/Clamber/WireWalk). *Gate: doc on-branch; shipped/planned columns match code file:line.*
2. **[Creative]** Author the ONE slice room (P/N/$/E + clamber ledge + vault block + floor + cover/patrol_post). *Gate: bakes to RoomAsset with walkable surface + blocker walls + spawn/npc/objective anchors.*
3. **[Creative]** Close the affordance wire-string gap on BOTH bake paths (ascii marker-tags + `monster_spawn`→'monster'; creative-doc descriptor→specific strings). *Gate: baked anchors of kind cover/patrol_post from both paths; `buildReasoningGraph` keeps them.*
4. **[Physics]** Verify bake roles for the slice room + one receipt-producing lab scenario; `usePhysicsMovePlanner=FALSE`. *Gate: floors→Walkable, walls→Blocker, no overlaps, ray blocked by wall.*
5. **[Movement]** Verify shipped traversal fires in the room (run/jump/Clamber/Vault). Decide MA4: DEFER unless the room's verticality forces the guard across a climb edge — if it does, the gate is the "brokered wiring slice" (reasoning graph + slot registry populate together at activation; §9), which also arms NPC tactical reasoning depth. *Gate: thief traverses spawn→objective with accepted MovementProof.*
6. **[AI]** Confirm the shipped guard boots with patrol+facing, reasoning graph from the RoomAsset. *Gate: guard patrols; alert rises in LOS cone.*
7. **[Foundational]** Prove thief-movement + guard-AI run in the SAME tick vs one SpatialSurfaceSet. *Gate: one tick advances both; guard reacts to the moving thief.*
8. **[Movement]** Ship SNEAK's player-input binding: consume the already-bound `PlayerCrouch` axis (mode-filtered — respect `mapMakerConsumesGameplayAction`) → toggle the existing `MovementMode::Sneak`; emit `sneaking` in `ProductMovementProofPacket`. *Gate: crouch→sneaking state at reduced speed; guard alert reads it; packet carries it.*
9. **[AI]** Build `GuardReconObservation` pure projector over durable AiActorState. *Gate: correct position + patrol-timing + watched-node from a seeded guard.*
10. **[Foundational]** Implement the runtime `ReconIntel` packet from GuardReconObservation, emitted at the objective, hashed + serialized. *Gate: objective emits event; the PACKET round-trips through save/replay (not the parkour trajectory — see §3).*
11. **[AI]** Wire the notebook producer (`Notebook.hpp` includes `ReasoningGraph.hpp`; fill `ProductNotebookReconPage`). *Gate: recon page renders LIVE guard state.*
12. **[Foundational]** Wire the intel TRANSFER thief→duo; AI provides the duo consumer; survives save/replay. *Gate: duo receives the same intel and acts on it.*
13. **[Foundational]** Confirm present/freeze gate for the slice room. *Gate: slice room presents a live frame (BG-1030 passes).*

**Target-first note:** steps 4-7 are verification of SHIPPED capability; only steps 3, 8, 9, 10, 11, 12 write genuinely new code. That is the honest size of "first playable."

---

## 5. LANE STATUS TABLE

| Lane | Canonical doc | Shipped (short) | Next for the slice |
|---|---|---|---|
| **Creative** | creative_editor_algorithms_map_v0_1.md (+ world_foundation_plan, affordance_vocabulary) | Two bake paths → playable RoomAsset; CreativeDocument kernel; standalone editor; W1-W8 world layer | Author the room (step 2) + close the affordance emission gap on BOTH paths (step 3) |
| **Movement** | movement-abilities-maximum.md v1.4 — **now on-branch (pulled from box), untracked** | Kinematic solver; run/sprint/jump-gravity=data; Vault/Clamber/WireWalk; dash; wall-run; M-LAB; MA4 (INERT until brokered wiring); sneak stance+automation | Verify traversal (step 5); wire SNEAK player-input (step 8); defer MA4 broker unless the room forces it |
| **AI** | ai-lane-maximum.md v1.8 (strongest map) | Full stealth loop LIVE (L0-L6, L8, persistence); guard already reacts | GuardReconObservation (9); notebook include+producer (11); duo consumer (12) |
| **Physics** | **physics-lane-maximum.md v1.0 (NEW this pass)** | collision/ footing (SHIPPED); physics/ engine (built, gated); bake+LOS live | Verify bake roles + one lab receipt (step 4); keep motor OFF |
| **Foundational** | **foundational-lane-maximum.md v1.0 (NEW this pass)** | Composed SessionTick + AI; SessionState; admission/retry; save/replay+StateHash; FrameInput+present gate; 239/239 | Compose-in-one-tick (7); ReconIntel packet (10); transfer (12); present gate (13) |

---

## 6. CROSS-LANE RISKS

1. **The one live seam break (creative→AI):** affordance strings consumed but emitted by neither bake path; an authored cover point is invisible to the guard until step 3. The creative-doc bake break is a SECOND break the algorithm map's ascii-framed GAPS section doesn't flag. *(Now the #1 risk — the movement-doc-absence risk below is resolved.)*
2. **Notebook seam unbuilt at runtime (biggest slice-end gap):** no ReconIntel truth, Notebook.hpp doesn't include ReasoningGraph.hpp, ProductNotebookReconPage has no producer (all VERIFIED). Three unbuilt pieces on the deepest dependency chain.
3. **Sneak player-input unwired:** the sneak *stance* + `gameplay.crouch` automation are shipped (MA1), but `PlayerCrouch` (bound, mode-filtered — creative-fly eats it as descend) has no gameplay consumer, so a HUMAN thief can't sneak. The fix is a binding, not a build (§9). Memory's "MA1 sneak shipped" is true for the mechanism, misleading for player control.
4. **Parkour is constitutionally lawless (movement↔foundational determinism):** vertical/parkour motion is app-side, invisible to replay/save (§9). The slice's save/replay gates cover the command stream + ReconIntel packet, NOT the parkour trajectory. Don't over-claim full-slice replay determinism; the permanent fix is MA7 (last).
5. **God-struct coupling blocks clean composition:** ProductAppWindowState is 720 fields (grew from 643/421), 2 of ~46 clusters cut. Add ReconIntel as a typed sub-aggregate on SessionState, NOT loose ProductAppWindowState fields.
6. **Duo side undesigned:** companion AI is reserved; step 12's transfer risks ending at a packet nobody reads unless the duo consumer is scoped.
7. **Silent movement↔physics param divergence:** footprint/slope thresholds must agree but were never pinned as a contract.
8. **Overkill traps (target-first honesty):** do NOT flip `usePhysicsMovePlanner`; do NOT build the MA4 ladder-broker unless the room demands ladders; do NOT build navmesh/flood-fill/greedy-meshing for a single authored room (the flood-fill validator is a build-once-reuse-four-ways investment for token-gen, not slice 1).
9. **Stale baselines:** test counts (171/183/185/234 → actual 239), field count (643/421 → 720), `iggy3d_validate_package` required but not built. Correct in the canonical docs so slices aren't cut against wrong numbers.

---

## 7. POINTERS TO THE LANE MAXIMUMS (the deeper reference behind the slice)

- **AI:** `docs/ai-lane-maximum.md` v1.8 — the strongest map; layers L0-L9, streams A1-A10, precedence chain, triple-lock law. (The movement doc cross-references it as v1.10 — reconcile the version on the next AI-map bump.)
- **Movement:** `docs/movement-abilities-maximum.md` v1.4 — **now on-branch (pulled from the box), untracked**; streams MA1-MA7 + M-LAB, the three-motion-worlds constitution, traversal-inside-Move law. Reconcile: MA4 ships as Vault/Clamber/WireWalk; sneak is stance+automation shipped / player-binding pending.
- **Physics:** `docs/physics-lane-maximum.md` **v1.0 (NEW, authored this pass)** — the two-stack truth (collision/ shipped footing; physics/ built-but-gated), 7 laws (L-PHY-1..7), 3 decisions, the precise physics↔movement seam sentence.
- **Foundational:** `docs/foundational-lane-maximum.md` **v1.0 (NEW, authored this pass)** — six pillars (SessionTick, SessionState, validated-mutation/receipt, neutral FrameInput, save/replay, module/shell decouple debt), the one-way dependency DAG, the god-struct decouple long tail.
- **Creative:** `docs/creative_mode/creative_editor_algorithms_map_v0_1.md` — the algorithm spine; world contract `world_foundation_plan_v0_1.md`; wire contract `affordance_vocabulary_v0_1.md`. GAPS 1-5 are the creative↔AI↔movement seams; the flood-fill validator is the build-once-reuse-four-ways investment (defer to token-gen, not slice 1).

---

## 8. THE DOCTRINE, HELD AGAINST THIS PLAN

- **Directed emergence / leverage in the READERS:** the whole slice is ONE RoomAsset read by four readers (footing, LOS, reasoning graph, notebook). `GuardReconObservation` is a generic projector over durable state, never per-guard machinery — leverage lives in the reader.
- **Generic systems, never per-kind:** the affordance seam is a fixed wire-string vocabulary mapped once (`markerToReasoningNode`); the notebook shares that same vocabulary (two-readers law).
- **validate → REPAIR → receipt:** command admission (Accepted/Rejected/Retry) + the physics lab receipt + the flood-fill repairable receipt + MovementProof are the validation/report legs.
- **Minimums carry the full DNA (source→generation→validation→decision→report):** the slice is exactly that end-to-end — authored source → bake → validation (lab + reachability) → guard decision → ReconIntel report/transfer.
- **Target-first:** the fire-order is the SMALLEST path (6 new-code steps); the two maximums and every deferred item (physics motor, MA4 broker, navmesh, god-struct decouple, MA7 motor-unification) sit behind it, flagged as overkill for slice 1.

---

## 9. THE MOVEMENT CONSTITUTION (from the pulled maximum — reconciled post-synthesis)

The master synthesis was built before the real movement maximum (v1.4) was pulled from the box. These are the corrections it forces — all load-bearing for the seams above.

**Three motion worlds (the constitutional finding):**
1. **Horizontal ground movement** — per-input Move commands through admission → `executeMovement`. Deterministic, replayable, receipted. **LAWFUL.**
2. **ALL vertical/parkour motion** (jump arcs, gravity ×18, coyote/buffer/cut, wall-run, wall-jump) — app-side `gameplay/Controller.cpp`, analytic integration per frame, direct world writes via `setProductPlayerPosition`, **bypassing admission, outside the command log, invisible to replay**, state in god-struct fields. Feel is GOOD; constitution is not. **LAWLESS.**
3. **Traversal** (Vault/Clamber/WireWalk) — runtime-side instant teleports from authored tags, validated + data-driven but **command-log-invisible** (its one caller is the Controller). **LAWFUL-ADJACENT.**

**Destiny:** ONE runtime motor (`PlayerMotor` lineage), fixed-tick, fed through admission, serving players AND NPCs, per-dimension profiles as data. The lawless layer **NEVER GROWS** — every new continuous verb lands runtime-first as a PlayerMotor phase. This is MA7 (motor unification), scheduled LAST; the slice does not need it.

**What this corrects in the plan:**
- **Sneak:** MA1 shipped `MovementMode::Sneak` + `kMoveSneakBit` on `userData0` + per-dimension speed/loudness multipliers + `SaveCodec` persistence + the `gameplay.crouch` automation verb. What is NOT wired is the live `PlayerCrouch` input axis → the sneak stance in gameplay (the doc flags this: "PlayerCrouch input exists, NOTE mode-filtered … the gameplay wiring must respect `mapMakerConsumesGameplayAction`"). So slice step 8 is **wire the input to the existing stance**, not build sneak. Confirm against the pulled doc + code before cutting.
- **Parkour determinism (§3 seam / §6 risk 4):** because World 2 is lawless, a replay of the thief's parkour run diverges; only the command stream + the ReconIntel packet are guaranteed to survive save/replay. Also relevant to stealth: airborne motion is currently SILENT to guards and the landing thump is a *declared deferred lie* until landing detection is runtime-side (MA7) — so "thief sneaks past the guard by parkour" trivially works in the slice, but for an unclosed reason, not by design.
- **MA4 brokered wiring:** NPC movement (capability classes + climb edges + traversal-inside-Move) is SHIPPED but **live-session arming is INERT until the ONE brokered product-activation follow-up — the reasoning graph and the slot registry must populate together at activation.** That same wiring arms NPC tactical-reasoning depth (the AI lane's `buildReasoningGraph`-into-live-sessions follow-up). If the slice room forces the guard across a climb edge, that broker is the gate; otherwise defer (guard patrols flat floor).

**Cross-lane law (from the doc):** `creative/**` never touched by this lane; `Controller/gameplay` app files only by explicit order; Codex owns creative authoring of parkour surfaces (`WallRunSurface` already in his descriptor table); never shrink hash coverage.
