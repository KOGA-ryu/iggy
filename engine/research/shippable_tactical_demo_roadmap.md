# Shippable Tactical Demo Roadmap

Updated: 2026-06-20

This roadmap is the first tactical-demo execution slice inside the broader 3D
product platform direction documented in
`engine/research/3d_product_platform_roadmap.md`. It remains useful as a
single-player-first proof path, but it is not the complete product target.
Authoring tools are internal. The runtime, save format, content package
contract, and simulation boundaries must still leave a clean path to future
multiplayer.

For the builder-facing packet plan with semantics, data ownership, compute
budgets, and verification gates, use
`engine/research/shippable_tactical_demo_execution_spec.md`.

## Locked Direction

- Shipping target: playable demo first, not a public editor.
- Tools target: internal authoring, checking, debugging, and content packaging.
- Runtime target: bespoke native product path. Qt is not a shipping shell.
- Gameplay target: real-time tactical combat that can be slowed, paused, or
  stepped to allow turn-like command decisions.
- Persistence target: durable user-facing saves, not temporary debug snapshots.
- Multiplayer target: not in the first demo, but architecture must support an
  authoritative simulation and serializable player commands later.

## Product Definition

The demo is done when a player can launch a native app, load a packaged
scenario, play a real tactical encounter, save during the encounter, quit,
resume, retry/reset, and reach a win or loss state without developer-only UI.

The demo should prove:

- one explicit content package as the product input;
- package checking before play;
- native bespoke launch and play loop;
- deterministic-enough simulation stepping for replay, save, and later network
  authority;
- target discovery and reach-gated action execution;
- tactical time controls: normal, slow, paused, and stepped;
- durable save/load across process restarts;
- one playable combat encounter with visible feedback;
- acceptance automation that proves the shipped loop without manual UI.

## Non-Goals For The First Demo

- Public editor UX.
- Multiplayer sessions.
- General-purpose modding.
- Full renderer feature parity.
- Broad combat content.
- Complex campaign progression.
- Qt as a product dependency.

## Phase 0: Baseline And Product Contract

Goal: turn the current product-loop proof into the first shippable-demo contract.

Deliverables:

- Canonical demo package under `engine/content/demos`.
- Demo acceptance command documented next to the package.
- Versioned product package identity: package id, scenario id, content version,
  asset references, and compatibility metadata.
- Product-loop save envelope design: package identity, simulation clock,
  gameplay state, command queues, random seed/state if used, and migration
  version.
- A short first-demo feature budget: map size, unit count, enemy count, ability
  count, asset budget, and performance target.

Exit criteria:

- A fresh checkout can build the runtime targets and run one acceptance command
  that loads the demo package, performs scripted play, saves, resets, loads, and
  finishes with an expected state.

## Phase 1: Runtime / Gameplay Loop

Goal: make the existing product loop playable and recoverable.

Deliverables:

- Explicit target discovery policy: nearest target, priority class, tie-breaks,
  disabled targets, and diagnostics.
- Reach-gated interaction execution for discovered or explicit targets.
- Pause, resume, retry, reset, save, and load exposed through the native product
  session.
- Runtime diagnostics for no target, target too far, missing required item,
  blocked interaction, invalid command, and paused input.
- Acceptance demo that proves target discovery, reach gating, interaction
  mutation, pause/retry/reset, durable save/load, and scenario completion.

Exit criteria:

- The product loop is testable as one shipping path rather than isolated runtime
  helpers.

## Phase 2: Tactical Simulation Clock

Goal: define the time model before combat grows around accidental frame
behavior.

Deliverables:

- Simulation clock with normal, slow, paused, and step modes.
- Fixed or otherwise explicit simulation tick policy.
- Time-scale rules for movement, cooldowns, windups, recovery, status effects,
  and AI updates.
- Player command windows that allow turn-like decisions while the world is
  slowed or paused.
- Tests for frame-step equivalence across normal, slow, paused, and stepped
  execution.

Exit criteria:

- A command issued while paused or slowed resolves through the same simulation
  pipeline as a command issued in real time.

## Phase 3: Command Model And Multiplayer-Ready Boundary

Goal: make single-player local play behave like a local authoritative session.

Deliverables:

- Serializable player command objects for movement, targeting, attack, ability,
  wait, cancel, pause/time-scale, save, and retry/reset policy hooks.
- Stable entity ids for player units, enemies, items, doors, hazards, and
  scenario objectives.
- Command log that can be saved, replayed, and inspected.
- Authoritative simulation boundary: input proposes commands, simulation applies
  accepted commands, presentation observes results.
- Deterministic combat resolution where practical, including random seed/state
  capture if randomness is used.
- Rejection diagnostics for invalid commands without mutating simulation state.

Exit criteria:

- The single-player demo can be described as a local host session, leaving
  future client/server work as transport and authority distribution rather than
  a rewrite of gameplay.

## Phase 4: Durable Save / Load / Resume

Goal: saves are product data that survive process restarts and version checks.

Deliverables:

- Versioned save envelope.
- Package compatibility check on load.
- Scenario id and content version in every save.
- Player and enemy unit state: position, health, alive/dead, faction, current
  order, cooldowns, statuses, and equipment/inventory needed by the demo.
- World mutations: collected items, opened doors, triggered objectives, defeated
  enemies, and encounter state.
- Simulation clock state, command queue, command log cursor, and random
  seed/state.
- Corrupt, missing, incompatible, and stale-save diagnostics.

Exit criteria:

- Save mid-encounter, quit, relaunch, load, and complete the encounter with the
  same tactical state.

## Phase 5: Combat Vertical Slice

Goal: turn the product loop into the first real-time tactical game moment.

Recommended first slice:

- One controllable unit.
- One or two enemy units.
- Movement order.
- Target acquisition.
- One attack or ability.
- Enemy reaction.
- Slow-time or pause command issuing.
- Health, defeat, and win/loss state.

Deliverables:

- Unit combat state.
- Faction/team ownership.
- Targetable enemy actors.
- Attack or ability definition.
- Cooldown/windup/recovery semantics.
- Damage or effect resolution.
- Enemy behavior sufficient to force a tactical decision.
- Scenario objective and completion state.

Exit criteria:

- The demo has a beginning, a tactical choice, a consequence, and an ending.

## Phase 6: Bespoke Native Product Shell

Goal: remove shipping dependence on developer/test shells.

Deliverables:

- Native app launch into the demo package or a minimal demo menu.
- Input mapping for movement, target selection, attack/ability, pause, slow
  time, retry/reset, save, load, and quit.
- In-game HUD for unit state, target state, time mode, objective state, and save
  feedback.
- Hidden developer overlay for package, target, command, clock, and save
  diagnostics.
- Error surfaces for package load failure, incompatible save, renderer failure,
  and missing assets.

Exit criteria:

- A player can operate the demo without knowing the test harness or CLI command
  syntax.

## Phase 7: Asset And Renderer Demo Quality

Goal: make the demo visually legible and package-driven.

Deliverables:

- Asset catalog entries for the demo map, unit, enemy, floor, wall, props, and
  interaction objects.
- Static mesh loading through the existing native asset path.
- Material and texture subset sufficient for the demo.
- Missing asset fallback policy and diagnostics.
- Renderer validation path: screenshot, headless frame, or deterministic render
  summary.
- Performance baseline for the demo scene.

Exit criteria:

- The shipped demo uses package-referenced assets instead of hardcoded renderer
  fixtures for its primary visuals.

## Phase 8: Internal Authoring And Packaging Pipeline

Goal: keep tools internal but make demo content repeatable and inspectable.

Deliverables:

- Scenario/package checker for demo content.
- Internal map and encounter authoring path.
- Unit, enemy, item, ability, objective, and spawn definitions.
- Asset-reference validation.
- Authoring diagnostics for broken ids, invalid rules, missing assets, and
  unsupported content.
- Internal debug preview for package state, simulation clock, commands, targets,
  and save state.

Exit criteria:

- A developer can edit demo content, check it, run it, and inspect failures
  without changing runtime code.

## Phase 9: Demo Hardening

Goal: prepare the demo for someone outside the codebase.

Deliverables:

- Shippable build/package script.
- Clean first-run path.
- Durable save directory policy.
- Settings policy for window, input, and audio if audio exists.
- Crash/error reporting basics.
- Regression suite covering acceptance demo, save/load, package compatibility,
  target/action behavior, tactical clock, and combat outcome.
- Known issues and release checklist.

Exit criteria:

- A packaged build can be handed off with instructions and reproduced from a
  clean checkout.

## Immediate Packets

1. Promote the current product-loop demo into the canonical shippable-demo
   fixture and document its acceptance command.
2. Add a versioned product-loop save envelope around the existing save/load
   state.
3. Define and test target priority for multiple reachable targets.
4. Add tactical simulation clock types and pause/slow/step tests.
5. Convert player actions into serializable command objects at the product
   boundary.
6. Add unit combat state: health, faction, alive/dead, cooldowns, and current
   order.
7. Add one enemy actor and one attack/ability.
8. Extend durable saves to combat, command, and clock state.
9. Add native controls/HUD feedback for time mode, target, attack, save, load,
   retry, and objective state.
10. Package the first shippable demo build.

## Open Questions

These do not block Phase 0 or Phase 1, but they should be answered before the
combat slice hardens:

- Is the first playable character a single hero, a squad leader, or a whole
  squad?
- Does slow/turn-like play allow issuing commands while fully paused, or only
  while time is slowed?
- Should combat randomness exist in the first demo, or should all results be
  deterministic?
- Is the future multiplayer model lockstep, authoritative server with command
  replication, or host-authoritative co-op?
- What is the first demo camera: fixed tactical camera, follow camera, or free
  pan/zoom?
- What platform is the first shippable binary expected to target first: macOS
  only, or macOS plus Linux?
