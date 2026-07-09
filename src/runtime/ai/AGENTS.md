# runtime/ai — the guard brain (perception, alert, memory, tactics)

**Verified at:** `0bad85c2` (2026-07-09). Reviewer refreshes this at each plan milestone.
Governing plan: `docs/guard_senses_and_tactics_plan_v1.md` (rulings R1–R8). Contract reference:
`docs/perception-3d-maximum.md`. Wire vocabulary: `docs/affordance_vocabulary_v0_1.md`.

## What this folder owns

Pure simulation kernels for NPC senses and decisions. GPU-free, deterministic, no `app/`/`render/` includes
(lane law — data flows runtime → app, never back).

## The shape (data flow)

`Session.cpp` (runtime/session) is the ONLY production wirer. Per tick it: bakes vision colliders → resolves
LOS occlusion → builds `NpcPerceptionRequest` → `queryNpcPerception` (NpcBehaviorSystem) → stimulus →
`npcStepAlert` (NpcAlertSystem) → behavior stages (patrol / investigate / search via NpcPatrolSystem,
NpcInvestigateSystem, GuardDecision + ReasoningRoute over ReasoningGraph) → NPC commands. Observability flows
`AiActorState.lastTarget*` mirrors → `NpcBehaviorDebugSnapshot` → app debug HUD / gaze blade.

- **NpcBehaviorSystem** — the perception kernel: 3D distance, horizontal + vertical cones, eye heights,
  `Los` tri-state. Pure; occlusion verdict is injected by the caller.
- **NpcBehaviorProfile** — the data layer. `NpcBehaviorConfig` is DERIVED from profiles
  (`configFromNpcBehaviorProfile`) and is **never serialized** — saves persist `behavior_profile_id` only.
- **NpcAlertSystem** — banded alert level (thresholds → bands; rise scaled by stimulus; grace windows; decay).
- **NpcSoundPerception** — loudest-audible-sound kernel; `events`/`blockers` parallel spans are built in
  lockstep by Session.
- **ReasoningGraph** — the tactical node graph, built from `RoomAsset.anchors[].kind` wire strings at
  ACTIVATION time (not per tick). Unknown kind strings are dropped silently BY DESIGN (append-only vocabulary).
- **GuardDecision / ReasoningRoute** — node scoring (`strategicValueByKind`) and routing over the graph.
- **GuardRecon / ReconIntel** — intel projection + packet kernels (serialize + hash). No production caller
  yet (the notebook consumer is planned, plan §P6e).

## Laws / invariants

- `perceived == (inRadius && inHorizontalCone && inVerticalCone && los == Clear)`; `Los::Unknown` NEVER
  grants sight (fail toward non-detection — plan R3/R3.1: empty-successful bake = Clear; failure = Unknown).
- Zero/degenerate facing = **omnidirectional perception** — a documented contract for low-level callers,
  not a bug. Do not "fix" it.
- `lastTarget*` mirrors are debug-only: NOT in StateHash, NOT in SaveCodec, NOT in receipts. Keep it that way
  unless a ruling says otherwise.
- Determinism: no wall-clock, no randomness, no frame-rate dependence in any kernel.

## Hazards (do NOT)

- No per-tick allocations on the `enqueueNpcBehaviorCommands` path (hot: per guard per tick).
- `perceptionHasLiveTarget` (Session) is an EXHAUSTIVE switch over `NpcPerceptionStatus` — new enum values
  force deliberate caller updates.
- Eye heights and occlusion margins come from config/profiles; hardcoded eye/margin constants are being
  eliminated (plan R2) — never add a new one.
- Sound `events`/`blockers` spans must stay lockstep-length (they are built together in Session).
- Sound alone must never reach the Combat band (plan R7 — the cap is intended design).
