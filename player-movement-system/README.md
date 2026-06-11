# Player Movement System

This is a small movement-focused sibling architecture inspired by DevilutionX.
It keeps the important lesson and removes everything else:

```text
raw device input
  -> input focus
  -> player intent
  -> action gate
  -> movement command
  -> command dispatcher
  -> player movement simulation
  -> world/path/collision constraints
```

The key idea is that gameplay should not care whether intent came from mouse,
keyboard, controller, touch, replay, or network. Once input becomes a movement
command, the simulation has one path to maintain.

## Docs

- [Movement flow](docs/movement-flow.md) walks through the runtime gameplay
  pipeline from input to simulation, save slots, and session commands.
- [Save system](docs/save-system.md) explains the durable-state and byte-format
  layers behind snapshot save/load.
- [Lifecycle replay](docs/lifecycle-replay.md) explains session command replay,
  lifecycle scripts, and their byte-format validation layers.

## Folder Map

```text
src/app
  Owns startup, runtime input routing, optional lifecycle boot scripts, and
  bounded game-loop updates.

src/combat
  Resolves deterministic attack damage and target defeat.

src/input
  Converts raw input into player-facing intent and exposes raw input sources for
  the app edge.

src/focus
  Decides which surface owns input right now.

src/commands
  Defines semantic movement commands, command sources, queues, and dispatch.

src/targeting
  Registers and resolves what is under a tile: empty space, item, enemy, NPC,
  player, or object.

src/interaction
  Converts targets into move, attack, pickup, talk, and interact intent.

src/items
  Owns durable item state for pickup targets.

src/inventory
  Owns player item ownership, pickup transfer from world state, and equipment
  slot rules.

src/actions
  Executes queued destination actions once movement reaches the acting point.

src/events
  Publishes semantic movement/action events for tests, debugging, UI, audio, VFX,
  replay, and telemetry.

src/effects
  Converts movement and combat facts into presentation requests such as sounds,
  VFX, damage numbers, feedback, and hit-stop.

src/replay
  Records and replays semantic movement commands.

src/save
  Captures and restores durable simulation state without saving frame events or
  presentation effects.

src/session
  Owns game lifecycle: new game, load game, save game, session mode, world,
  clock, frame updates, and save slots.

src/network
  Converts semantic movement commands to stable packets and back.

src/enemies
  Adds constrained enemy pursuit and attack timing pressure.

src/simulation
  Owns the per-tick orchestration order across queued commands, player movement,
  enemy movement, destination actions, and combat consequences.

src/player
  Owns player state, action gating, movement state transitions, and movement
  execution.

src/world
  Owns grid coordinates, pathfinding, collision, diagonal corner rules, and
  walkability rules.

src/config
  Holds movement and path tuning values.

src/debug
  Holds tracing/instrumentation helpers.

tests
  Focused tests for movement behavior.
```

## DevilutionX Comparison

DevilutionX has a path like:

```text
SDL mouse event
  -> HandleMouseButtonDown
  -> LeftMouseDown
  -> LeftMouseCmd
  -> NetSendCmdLoc(CMD_WALKXY)
  -> ParseCmd
  -> OnWalk
  -> MakePlrPath
  -> ProcessPlayers
```

This scaffold reduces that to:

```text
RawInputEvent
  -> InputFocus
  -> InputMapper
  -> PlayerIntent::MoveTo
  -> PlayerActionGate
  -> IntentCommandBuilder
  -> MovementCommand::WalkTo
  -> CommandDispatcher::dispatch
  -> PlayerController::walkTo
  -> PlayerPathPlanner::walkTo
  -> PathFinder::findPath
  -> WalkPath
  -> PlayerMovement::update
  -> PlayerAnimationLockGate::advance
  -> PlayerPathStepper::step
```

For click-to-interact movement:

```text
RawInputEvent
  -> InputFocus
  -> TargetResolver
  -> InteractionIntentBuilder
  -> InteractionCommandBuilder
  -> MovementCommand::WalkTo / MoveThenAct / StandAndAct
  -> CommandDispatcher
  -> PlayerController
  -> PlayerPathPlanner
  -> PlayerMovement
  -> PlayerPathStepper
  -> PlayerActionRunner
  -> ActionExecutor
```

## Design Rules

- Raw input should not directly move the player.
- Inventory, menus, dialogue, cutscenes, and stun states should block or reroute
  intent before it becomes a command.
- Commands should describe game intent, not hardware input.
- Movement simulation should be deterministic enough to test.
- Enemy/combat tuning should build on movement, not compensate for bad controls.

## Movement Pieces

```text
ActorPosition
  tile, future, previous, precise

ActorStepCommitter
  shared step-commit boundary that updates ActorPosition after player pathing
  or enemy pursuit has already chosen a legal next tile

EnemyPursuitStepPlanner
  small chase steering rule that chooses the next one-tile enemy step toward a
  target before budget, blocking, or attack-range constraints are applied

WalkPath
  fixed-size queue of committed tile steps

PlayerPathStepper
  consumes one queued path step, checks late collision, delegates position
  commitment to ActorStepCommitter, and reports when arrival makes a
  destination action ready

PathCostTuning
  pathfinding taste, such as diagonal vs axis-aligned preference

DiagonalCornerPolicy
  prevents diagonal movement through blocked corners

MovementModifiers
  stand-ground and similar player-controlled constraints

AnimationLock
  movement cancel timing and commitment windows

PlayerAnimationLockGate
  advances animation commitment timers and blocks movement until the cancel
  window opens

DestinationAction
  action to perform after movement reaches range

DestinationActionBuilder
  interaction helper that maps attack, pickup, talk, and interact intents into
  destination action payloads and range requirements

PlayerPathPlanner
  command-side helper that finds paths, stores pathing state, and emits
  path-started or action-ready controller events

PlayerActionRunner
  turns an arrived player into acting state and runs the queued destination
  action through ActionExecutor

TargetRegistry
  world-backed clickable target table used by runtime input routing, save/load,
  and interaction command building

TargetSynchronizer
  keeps enemy click targets aligned with enemy movement and combat defeat state

WorldEntityService
  spawns and despawns world entities while keeping enemy, item, combat, and
  target registry state consistent

InventoryService
  transfers successfully picked world items into player inventory when capacity
  allows, and removes their world target state only after transfer

EquipmentService
  moves equippable inventory items into equipment slots and unequips them back
  into inventory when capacity allows

EquipmentStatsService
  derives effective player combat stats from currently equipped items without
  mutating the player's base stats

InventoryCommandDispatcher
  applies semantic inventory commands such as equip-item and unequip-slot through
  the same equipment service used by tests and future UI

InventoryCommandSource
  runtime-facing source boundary that lets menus, controller shortcuts, replay,
  or debug tools feed semantic inventory commands into the active player

InventoryEvent
  observable inventory facts such as Equipped, Unequipped, and Rejected for UI,
  logs, tests, and future replay tooling

InventoryCommandCodec / InventoryCommandLog / InventoryCommandReplayer
  stable packet, byte, log, and replay boundaries for semantic inventory
  automation

InventoryCommandPacketValidator / InventoryCommandPacketByteCodec
  inventory packet-shape and fixed byte-layout boundaries that keep malformed
  automation or replay input out of semantic inventory commands

InventoryCommandByteStream
  little-endian primitive byte stream shared by inventory command packets,
  packet lists, command log frames, and command log checksums

InventoryCommandPacketListCodec
  count-prefixed packet-list payload for inventory command logs, separate from
  magic/version/checksum frame validation

InventoryCommandLogChecksum / InventoryCommandLogFrameCodec
  inventory replay-file integrity and frame boundaries for magic, version,
  packet-list payloads, and checksum validation

InventoryCommandLogFileStore
  domain persistence boundary for inventory command logs, backed by
  ByteFileStore so automation scripts can live on disk without mixing raw
  filesystem rules into codecs or dispatchers

InventoryScriptRunner
  high-level use case that loads an inventory command script and replays it
  through the normal inventory dispatcher

InventoryScriptSource
  runtime-facing source boundary that lets menus and debug tools queue inventory
  automation script paths for the active player

ActionExecutor
  validates target/range/state and applies animation commitment

MovementEvent
  observable facts such as CommandAccepted, PathStarted, StepCommitted,
  ActionExecuted, and AnimationLocked

CommandLog
  replayable list of semantic movement commands

MovementCommandSource
  runtime-facing source boundary that lets input, replay, debug, or future
  networking feed semantic movement commands into the active world queue

RuntimeInputSourceRouter
  app-edge use case that builds the current input context, drains raw input
  sources, and routes handled events into semantic session/movement queues

RuntimeInputRouter
  app-edge adapter that translates focused raw input into session or movement
  command sources, optionally resolving clicked targets into interaction
  commands, without letting hardware events into simulation code

RuntimeSessionInputRouter
  lifecycle hotkey router for pause and inventory mode toggles before movement
  input is considered

RuntimeMovementInputRouter
  gameplay input router that maps focused pointer, target, and stop-key input
  into semantic movement commands

RuntimeTargetInputRouter
  target-aware pointer router that resolves clicked tiles into interaction
  movement commands

RuntimeRawInputDrainer
  app-edge helper that drains raw input sources, skips missing sources, routes
  each event through RuntimeInputRouter, and reports how many events were handled

RawInputSource
  app-edge source boundary for raw device-like events before they are routed
  into semantic command sources

MovementCodec
  socket-free command packet encoding/decoding boundary

MovementCommandValidator
  command dispatch gate that rejects structurally incomplete movement commands
  before they can reach player control

MovementCommandEventEmitter
  command event helper that publishes accepted/rejected command facts for tests,
  replay comparison, traces, and presentation

EnemyMovement
  pressure layer with speed, attack range, windup, recovery, and enemy attack
  resolution constraints

EnemyPursuitStepper
  constrained enemy chase helper that asks EnemyPursuitStepPlanner for pursuit
  steps, delegates position commitment to ActorStepCommitter, and stops when
  step budget, blocking, or attack range stops movement

EnemyAttackRunner
  enemy attack state helper for range checks, windup timing, recovery timing,
  and combat resolution

CombatResolver
  deterministic consequence layer for executed attack actions

CombatEvent
  observable combat consequence such as Hit, Defeated, damage, and remaining HP

SimulationTick
  frame-level coordinator that drains commands, updates player movement, updates
  enemy pressure, and lets actions resolve through combat

SimulationCommandDrainer
  simulation-layer command intake stage that drains queued semantic movement
  commands through CommandDispatcher and PlayerController

SimulationPlayerUpdater
  simulation-layer player actor stage that advances path movement and executes
  ready destination actions through ActionExecutor

SimulationEnemyUpdater
  simulation-layer enemy actor stage that advances pursuit, windup, recovery,
  and enemy attack resolution against the current player target

SimulationFramePolicy
  mode-level rule set for whether a frame accepts commands, advances players,
  or advances enemies

SimulationClock / SimulationTimeStep
  converts raw frame time into actor time, supporting time scale and hit-stop
  without pushing time rules into movement, enemy, or combat systems

SimulationTimeStepBuilder
  frame-runner boundary that converts raw delta into SimulationTimeStep, using
  SimulationClock when time scale or hit-stop is active

SimulationFrameEvents / SimulationFrameRunner
  collects per-frame facts and returns frame output for presentation

SimulationFrameTickRunner
  tick-stage runner that captures movement/combat events while running
  SimulationTick, then restores the world's previous event sinks

SimulationFrameEventCapture
  scoped frame helper that temporarily installs movement/combat event capture
  sinks, forwards to existing sinks, and restores the world after the tick

SimulationEffectPipeline
  routes frame movement/combat events into effect requests and applies approved
  simulation-facing effects through EffectApplier

SimulationEffectFinalizer
  post-tick effect stage that runs frame feedback routing and applies any
  simulation-facing effect consequences such as hit-stop

SimulationFrameFinalizer
  post-tick consequence stage that synchronizes clickable targets, applies
  pickup transfers, and runs the simulation effect pipeline

SimulationTargetFinalizer
  post-tick target stage that publishes current enemy targets and removes
  defeated enemy targets

SimulationInventoryFinalizer
  post-tick inventory stage that applies accepted pickup actions and reports
  rejected transfers without hiding item ownership rules in the frame runner

SimulationSnapshot
  durable world state for save/load and debugging: players, inventories,
  equipment, enemies, floor items, combat registry data, and clickable targets,
  not transient events or effects

SnapshotBytes
  raw byte buffer type shared by save codecs, frame validation, checksums, and
  file persistence without forcing dependencies on the snapshot serializer

SnapshotByteWriter / SnapshotByteReader
  little-endian primitive byte stream helpers shared by snapshot payloads,
  frame headers, and checksum trailers

SnapshotEntityCodec
  reusable field codec for durable entity fragments such as points, targets,
  actor positions, item/equipment data, combat stats, and combatants

SnapshotPlayerCodec / SnapshotEnemyCodec
  actor-level durable serializers for player path/inventory/action state and
  enemy tuning/state timers, built on reusable entity field codecs

SnapshotVectorCodec
  count-prefixed vector serializer used by snapshot schema code to keep list
  framing separate from entity-specific field encoders

SnapshotSchemaCodec
  ordered durable snapshot schema coordinator for players, enemies, floor items,
  combat registry entries, and clickable targets

SnapshotCodec
  outer snapshot serializer that frames/unframes payload bytes and delegates the
  ordered durable section payload to SnapshotSchemaCodec

SnapshotChecksum
  checksum boundary for snapshot bytes, keeping save-file integrity checks
  testable outside the full snapshot serializer

SnapshotFrameCodec
  snapshot byte frame boundary that owns magic, version, payload framing, and
  checksum-protected validation before durable state is decoded

ByteFileStore
  shared binary filesystem boundary that writes bytes through a temp file,
  renames them into place, loads raw bytes, and reports missing or unwritable
  paths without knowing snapshot, session, or inventory meaning

SnapshotFileStore
  domain persistence boundary that saves versioned snapshot bytes through
  ByteFileStore and rejects invalid files before restore

SaveGameService
  high-level save/load use case that snapshots a world, persists it, and restores
  it without exposing serialization details to the simulation

SaveSlotService
  slot-based save/load and metadata listing for menu/debug surfaces without
  loading corrupt or missing slots into the world

NewGameSettings / NewGameWorldBuilder
  new-session setup boundary that turns start settings into initial world state
  while preserving the event sinks owned by the surrounding runtime/session

SessionWorldSlotLoader
  session-facing load boundary that replaces world state from a save slot only
  on success, preserving runtime event sinks across the replacement

SessionWorldSlotSaver
  session-facing save boundary that rejects empty sessions and persists active
  world state through SaveSlotService

SessionFrameUpdater
  session-facing frame boundary that applies lifecycle mode policy before
  advancing SimulationFrameRunner with the session clock

GameSession
  game-loop-facing owner for SimulationWorld, SimulationClock, frame updates,
  mode policy, and save slots

GameSessionMode / SessionModePolicy
  compact lifecycle state and the rules that map gameplay, pause, inventory,
  and empty sessions to simulation frame behavior

SessionModeChanger
  small transition boundary that applies only mode changes allowed by
  SessionModePolicy

SessionCommand / SessionCommandApplier
  semantic lifecycle command boundary for menu, UI, controller, replay, and
  tests to request new/load/save/mode changes without directly mutating session

SessionCommandDispatcher
  dispatches lifecycle commands through SessionCommandApplier and delegates
  observable event publication to SessionEventEmitter

SessionEventEmitter
  session event adapter that turns applied/rejected lifecycle command outcomes
  into SessionEvent payloads for an optional event sink

SessionCommandSource
  runtime-facing source boundary that lets menu actions, debug tools, scripts,
  or future live input provide lifecycle commands to the loop

SessionEvent
  observable lifecycle facts such as GameStarted, SaveCompleted, LoadFailed, and
  ModeChanged for UI, logs, tests, and replay tooling

SessionCommandLog / SessionCommandReplayer
  records and replays lifecycle commands through the same dispatcher used by
  menus, tests, and other callers

SessionCommandCodec
  stable byte boundary for lifecycle commands, with payload validation for
  replay, automation, and future networking

SessionCommandPacketValidator
  packet-shape guard that rejects malformed lifecycle command payloads before
  they become semantic session commands

SessionCommandByteStream
  little-endian primitive byte stream shared by lifecycle command packets,
  command log frames, and command log checksums

SessionCommandPacketByteCodec
  fixed-size byte boundary for lifecycle command packets, preserving explicit
  little-endian field layout and rejecting invalid decoded packet shapes

SessionCommandLogCodec
  durable byte format for lifecycle command logs, including magic, version,
  command count, packet validation, and checksum rejection

SessionCommandLogChecksum
  checksum boundary for lifecycle command log bytes, so replay-file integrity
  rules are testable outside the full log codec

SessionCommandPacketListCodec
  count-prefixed packet-list payload for lifecycle command logs, separate from
  magic/version/checksum frame validation

SessionCommandLogFrameCodec
  replay-file frame boundary that owns magic, version, and checksum-protected
  frame validation around SessionCommandPacketListCodec payloads

SessionCommandLogFileStore
  domain persistence boundary for lifecycle command logs, backed by
  ByteFileStore so replay scripts can be saved and loaded without mixing raw
  filesystem behavior into the codec

SessionScriptRunner
  high-level automation use case that loads a lifecycle script file and replays
  it through the same dispatcher used by live session actions

RuntimeLoopTypes
  app-layer contracts that group GameLoop settings, setup/output settings,
  per-frame reports, run summaries, and final results separately from the loop
  orchestration class

GameLoop
  app-facing shell that assembles runtime collaborators, executes the run
  lifecycle, and exposes results/events for tests and debug tools

RuntimeSetupSettings
  app-layer setup settings that group optional startup and configured inventory
  script paths before frame updates begin

RuntimeSetupResult
  app-layer setup result state that groups configured startup and inventory
  script attempts before frame updates begin

RuntimeSetupRunner
  app-layer use case that runs configured startup and inventory setup scripts,
  reports whether frames may begin, and preserves setup inventory command
  results for run summaries

RuntimeSourceSettings
  app-layer source settings that group raw input, session, inventory script,
  inventory command, and movement command sources for GameLoop

RuntimeInputSettings
  app-layer input context settings that group bindings, focus/action state,
  active player id, and optional target resolver for GameLoop routing

RuntimeInputContextBuilder
  app-layer context builder that combines current session state with
  RuntimeInputSettings before raw input is routed

RuntimeFrameSettings
  app-layer frame settings that group bounded frame count and fixed timestep
  for the GameLoop shell

RuntimeFrameRunner
  app-layer use case that runs one bounded frame: asks the input source router
  to route raw input, drains runtime sources, advances the session, and records
  the frame report

RuntimeFrameLoopRunner
  app-layer loop policy that runs a one-frame runner for RuntimeFrameSettings
  maxFrames, keeping bounded repetition out of the GameLoop shell

RuntimeRunExecutor
  app-layer lifecycle use case that runs configured setup, conditionally runs
  the bounded frame loop, and finalizes the GameLoopResult

RuntimeRunSummary
  app-layer aggregate run result state for routed input, runtime command
  results, queued movement counts, frames run, and final frame events

RuntimeRunRecorder
  app-layer recorder that turns per-frame runtime work into RuntimeFrameReport
  entries and RuntimeRunSummary aggregates

RuntimeRunFinalizer
  app-layer completion policy that records final session mode and applies
  configured output finalization to GameLoopResult

RuntimeSetupFailurePolicy
  app-layer policy that treats failed startup scripts and non-completed
  configured inventory setup scripts as setup failures

RuntimeOutputFailurePolicy
  app-layer policy that treats attempted-but-unsaved runtime artifact outputs
  as output failures while ignoring outputs that were never requested

RuntimeRunFailurePolicy
  app-layer policy that composes setup and output failure policies into a
  single GameLoopResult failure decision

RuntimeExitCodeMapper
  app-layer mapper that converts a run failure boolean into a process-style
  numeric exit code

RuntimeExitCodePolicy
  app-layer adapter that combines RuntimeRunFailurePolicy and
  RuntimeExitCodeMapper for GameLoopResult exit codes

RuntimeOutputSettings
  app-layer artifact settings that group optional run trace and debug bundle
  destinations for GameLoop finalization

RuntimeOutputResult
  app-layer artifact result state that groups trace and debug bundle save
  attempts after GameLoop output finalization

RuntimeArtifactOutputService
  app-layer artifact output service that applies RuntimeOutputSettings,
  saves configured run traces and debug bundles, and returns RuntimeOutputResult
  flags for the caller

RuntimeOutputFinalizer
  app-layer use case that writes RuntimeArtifactOutputService results back onto
  GameLoopResult after the run lifecycle has produced final state

RuntimeFrameReport
  per-frame report that groups routed input counts, runtime source results,
  event deltas, and simulation frame output for debug/test inspection

RuntimeFrameTrace
  readable text formatter for RuntimeFrameReport, suitable for logs, debug
  overlays, and test diagnostics

TextFileStore
  shared line-oriented text filesystem boundary that writes readable artifact
  lines through a temp file, renames them into place, and loads lines without
  knowing trace or debug bundle meaning

RuntimeFrameTraceFileStore
  domain persistence boundary for readable runtime frame trace lines, backed by
  TextFileStore

RuntimeTraceService
  high-level use case that formats and saves complete GameLoopResult frame
  traces in one call, including GameLoop-configured trace output

RuntimeDebugManifest
  readable manifest formatter for debug bundles, summarizing run counts, setup
  attempts, final mode, artifact paths, and trace save state

RuntimeDebugArtifactLayout
  app-layer bundle layout boundary that maps a debug bundle root to stable
  artifact paths such as manifest.txt and run.trace

RuntimeDebugArtifactWriter
  app-layer artifact writer that saves the run trace and manifest files, while
  reporting trace/manifest save flags back to the bundle

RuntimeDebugArtifactBundle
  app-layer debug artifact orchestrator that prepares a bundle directory,
  delegates path naming to RuntimeDebugArtifactLayout and file writes to
  RuntimeDebugArtifactWriter, and can be invoked directly or through
  GameLoopSettings

RuntimeSourceDrainer
  app-layer helper that drains runtime session, inventory script, inventory
  command, and movement sources in frame order

RuntimeSourceStream
  app-layer source-stream primitive that skips missing source slots, drains
  each source once, and preserves source ordering before semantic dispatch

RuntimeSourceDrainerSettingsBuilder
  app-layer mapper that turns loop-level source and input settings into the
  lower-level settings consumed by RuntimeSourceDrainer

EffectRouter
  maps factual movement/combat events into presentation requests without letting
  simulation systems know about UI, audio, VFX, rumble, or camera code

EffectApplier
  applies effect requests that legitimately touch simulation feel, such as
  turning a HitStop request into SimulationClock hit-stop
```
