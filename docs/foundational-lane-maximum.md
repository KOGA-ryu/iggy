# FOUNDATIONAL LANE MAXIMUM — the substrate destiny doc

Version: v1.0 (2026-07-06). Reconciled against code at 239/239 tests green (`ctest -N`).
Cut ALL future foundational slices from this map. Cite coordinates; version-bump on corrections.

Foundational is the tick/session/save/render substrate that lets the whole vertical slice compose:
authored world (creative) → traversal (movement) → footing/collision (physics) → detection/reaction (AI)
→ intel capture + asymmetric transfer (the notebook seam) → duo response. This lane had no spine; this is it.

---

## THE SIX PILLARS

### P1 — SESSION TICK (the composition root)
`src/runtime/session/SessionTick.cpp` `runSessionTick(SessionTickInput)` runs, per tick, over admitted
commands: Ability → Combat → Interaction → Movement → Objective, emitting RuntimeEvents.
`SessionRunner.cpp` `runSession()` / `stepPausedOnce()` drives it (maxTicks, stopWhenIdle/Complete,
collisionSurfaces, usePhysicsMovePlanner). **Every other lane's system is composed HERE.** AI is already
spliced in via `Session.cpp:1085 enqueueNpcBehaviorCommands`. NOTE the composition caveat: the thief's
GROUND movement runs through this lawful tick, but vertical/parkour motion runs app-side in the lawless
Controller (invisible to the command log/replay) — see the master plan §3 parkour-determinism seam.

### P2 — SessionState (the truth aggregate)
`src/runtime/session/SessionState.hpp:87-101`: world, players, clock, camera, abilities, commandLog,
inventory, combat, ai, objectives, transient. Baseline snapshot (52-65) for Reset. The single owned struct
the tick mutates, save serializes, StateHash hashes. **New cross-lane truth (e.g. the ReconIntel packet)
belongs HERE as a typed sub-aggregate — never as loose `ProductAppWindowState` fields.**

### P3 — VALIDATED MUTATION / RECEIPT path
`src/runtime/command/CommandAdmission.cpp` admits (Accepted/Rejected) BEFORE execution; SessionTick
executes only accepted records; Retry re-resolves a Rejected command by id (`resolveEffectiveIntent`).
Receipt surface = `src/app/iggy3d/ReceiptBuilder.cpp` (2687 lines) from `ProductAppWindowState` (720
fields). Receipt TEXT is the observable contract. **validate → REPAIR(retry) → receipt is live** — the
full DNA the doctrine demands.

### P4 — NEUTRAL FrameInput RENDER seam
`src/render/FrameInput.hpp` = backend-neutral derived presentation, NOT durable truth
(docs/vulkan/frame_input_contract.md forbids owning gameplay/combat/save/command state or mutating
runtime). Projection → FrameInput → {NullRenderer, VulkanBackend}. PRESENT path = `FramePresenter.cpp:782`
(branch-gate BG-1030) gates on `scene.room.loaded` — **creative's freeze root** (a room-less stage never
sets loaded=true, so nothing submits; the standalone creative app sidesteps this with its own present path).

### P5 — SAVE / REPLAY
`src/runtime/save/*` (SaveCodec/Envelope/Load/FileStore/Compatibility) + `src/runtime/replay/*`
(CommandLog/CommandReplay/StateHash). Deterministic hash + command-log replay = the determinism crown
jewel that lets AI author + self-verify. CAVEAT: replay determinism holds for what flows through the
command log; the lawless parkour layer (movement) writes the world directly and is replay-divergent until
the motor-unification work (movement MA7) lands it in the lawful path.

### P6 — MODULE / SHELL REFACTOR (the decouple debt)
Governed by docs/plan_bucket/data_oriented_product_spine_refactor_contract_v0_1.md + audits
(appshell_shell_refactor, product_module_consolidation, product_shell_consolidation, module_taxonomy).
The god-struct decouple (720-field ProductAppWindowState) is the central coupling; branch-gate governance
(tools/check_branch_gate.py + branch_gate_policy.md + approvals.tsv) holds the line. Typed-member
migration mostly PLANNED (2 of ~46 clusters cut: savedMarkerBind 19→1, productSaveLoad 12→1).

---

## THE DAG
core (Result/ids/math/hash) → content (PackageLoader/Validator/FixtureScenarioLoader) → SessionState →
command admission → SessionTick systems → SessionRunner → save/replay → projection → FrameInput → present.
One-way dependency (architecture.md law): runtime owns truth; apps only collect input + call APIs;
projection is read-only; FrameInput never reaches back.

---

## SHIPPED (reconciled vs code)
- Session tick + runner composing Ability/Combat/Interaction/Movement/Objective (session_tick_tests, session_runner_tests, complete_runtime_demo_tests).
- SessionState truth aggregate + Reset baseline.
- AI wired into the live tick (enqueueNpcBehaviorCommands: Alert+Behavior+Patrol+Investigate) — stealth loop end-to-end.
- Command admission + Retry/repair; single receipt text surface.
- Save/replay + deterministic StateHash; pause→Load wired (sd1-sd6).
- FrameInput neutral seam + present gate (BG-1030); NullRenderer + Vulkan consumers.
- Branch-gate governance; data-oriented cleanup baseline (registries, mapping tables, alias canonicalization).

## PLANNED (documented, not built)
- God-struct decouple long tail (720 fields, ~44 clusters left) — next_work.md Stream 1.
- **Notebook runtime intel packet** — DOES NOT EXIST; Notebook.cpp is UI draw-list only. Highest-value slice gap.
- AppShell corridor shrink → command registry + state handlers; ProductAppOperations → executors; ReceiptBuilder → descriptor table; frontend router → route table.
- Consolidation audits (review-only, no slices cut); dead-code cleanup.
- Multi-room load/transition (only package.rooms.front() loads today).

## STALE (contradicted by code)
- Test counts (171/183/185/234) → actual 239/239.
- ProductAppWindowState field count (643 memory / 421 next_work) → actual 720; coupling worse than recorded.
- architecture.md requires iggy3d_validate_package — NOT built; iggy3d_creative/collision_probe/physics_kernel_bench/product_frame_metrics exist instead.
- frame_input_contract.md 'expected' tests are shipped+green.
- next_work.md frames stealth 5-6 as live focus — shipped.

---

## VERTICAL-SLICE CRITICAL PATH (foundational's part of thief→room→guard→notebook→duo)
1. **[S]** Verify authored-room → Session::create → SessionRunner boot; pin with a slice fixture. (needs: creative room package)
2. **[M]** Prove thief-movement AND guard-AI run in the SAME tick vs one SpatialSurfaceSet; add slice smoke. (needs: movement verbs + physics collision bake)
3. **[L]** Define + implement the runtime **ReconIntel** packet: session-owned struct capturing guard position / patrol timing / cover+objective points at the moment the thief logs intel; emit as RuntimeEvent, stash in SessionState (hashed, serializable). (needs: intel schema decision)
4. **[M]** Wire intel TRANSFER: expose ReconIntel to the knight+priestess duo; prove it survives save/replay. (needs: step 3 + AI duo consumer)
5. **[S]** Confirm present/freeze gate: authored-room projection reports scene.room.loaded so FramePresenter.cpp:782 presents (not a no-op); add slice-room smoke. (needs: projection scene.room.loaded)

---

## SEAMS — foundational ↔ all (named concretely)

### PROVIDES
| To | What | Via |
|---|---|---|
| ALL | Per-tick composition point over one SessionState | `runSessionTick()` (SessionTick.cpp) + `runSession()` (SessionRunner.cpp) |
| Movement+Physics | Collision surfaces + physics-planner flag into the tick | `SessionTickInput.collisionSurfaces` (const SpatialSurfaceSet*) + `usePhysicsMovePlanner` |
| AI | Live guard-command enqueue + authoritative AiState | `Session.cpp:1085 enqueueNpcBehaviorCommands` + `SessionState.ai` |
| ALL | Validated admission + retry/repair | `CommandAdmission.cpp` + `resolveEffectiveIntent` |
| Creative+Render | Neutral frame + present gate | `render/FrameInput.hpp` + `FramePresenter.cpp:782` (BG-1030) |
| AI(notebook)+Creative | Deterministic persistence/replay of new SessionState | SaveCodec/SaveEnvelope + CommandLog/CommandReplay + StateHash |
| ALL | Single test-pinned receipt text | `ReceiptBuilder.cpp` from `ProductAppWindowState` |

### NEEDS
| From | What | Blocks |
|---|---|---|
| Creative | Loadable authored-room package whose projection reports scene.room.loaded | Session boot (step1) + present gate (step5) |
| Physics | SpatialSurfaceSet collision bake for the room | Threading collisionSurfaces into the tick (step2) |
| Movement | Movement verbs executing in the Movement system | Thief traversal during the composed tick (step2) |
| AI | ReconIntel schema + duo-side consumer of the transferred packet | Steps 3-4 (foundational owns packet+persistence; AI owns content+response) |

---

## LAWS (architecture.md + ownership.md, reconciled — still true)
- Runtime owns gameplay truth; apps only collect input/call APIs/print; content makes validated seed; projection read-only; save persists truth; replay proves determinism.
- FrameInput is derived, never authoritative, never mutates state, never alters the state hash.
- No new source branch without a branch-gate id.
- ASCII is map-making only — owns no behavior.
- Headless-first: the deterministic loop proves out before renderer work.
