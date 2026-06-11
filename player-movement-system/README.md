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
  -> PathFinder::findPath
  -> WalkPath
  -> PlayerMovement::update
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
  -> PlayerMovement
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

WalkPath
  fixed-size queue of committed tile steps

PathCostTuning
  pathfinding taste, such as diagonal vs axis-aligned preference

DiagonalCornerPolicy
  prevents diagonal movement through blocked corners

MovementModifiers
  stand-ground and similar player-controlled constraints

AnimationLock
  movement cancel timing and commitment windows

DestinationAction
  action to perform after movement reaches range

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

InventoryCommandLogFileStore
  file persistence boundary for inventory command logs, so automation scripts
  can live on disk without mixing filesystem rules into codecs or dispatchers

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

RuntimeInputRouter
  app-edge adapter that translates focused raw input into session or movement
  command sources, optionally resolving clicked targets into interaction
  commands, without letting hardware events into simulation code

RuntimeRawInputDrainer
  app-edge helper that drains raw input sources, skips missing sources, routes
  each event through RuntimeInputRouter, and reports how many events were handled

RawInputSource
  app-edge source boundary for raw device-like events before they are routed
  into semantic command sources

MovementCodec
  socket-free command packet encoding/decoding boundary

EnemyMovement
  pressure layer with speed, attack range, windup, recovery, and enemy attack
  resolution constraints

CombatResolver
  deterministic consequence layer for executed attack actions

CombatEvent
  observable combat consequence such as Hit, Defeated, damage, and remaining HP

SimulationTick
  frame-level coordinator that drains commands, updates player movement, updates
  enemy pressure, and lets actions resolve through combat

SimulationFramePolicy
  mode-level rule set for whether a frame accepts commands, advances players,
  or advances enemies

SimulationClock / SimulationTimeStep
  converts raw frame time into actor time, supporting time scale and hit-stop
  without pushing time rules into movement, enemy, or combat systems

SimulationFrameEvents / SimulationFrameRunner
  collects per-frame facts and returns frame output for presentation

SimulationFrameEventCapture
  scoped frame helper that temporarily installs movement/combat event capture
  sinks, forwards to existing sinks, and restores the world after the tick

SimulationEffectPipeline
  routes frame movement/combat events into effect requests and applies approved
  simulation-facing effects through EffectApplier

SimulationFrameFinalizer
  post-tick consequence stage that synchronizes clickable targets, applies
  pickup transfers, and runs the simulation effect pipeline

SimulationSnapshot
  durable world state for save/load and debugging: players, inventories,
  equipment, enemies, floor items, combat registry data, and clickable targets,
  not transient events or effects

SnapshotCodec
  versioned byte boundary for snapshots, with magic/version validation before
  restored data is trusted

SnapshotFileStore
  file persistence boundary that saves versioned snapshot bytes and rejects
  invalid files before restore

SaveGameService
  high-level save/load use case that snapshots a world, persists it, and restores
  it without exposing serialization details to the simulation

SaveSlotService
  slot-based save/load and metadata listing for menu/debug surfaces without
  loading corrupt or missing slots into the world

GameSession
  game-loop-facing owner for SimulationWorld, SimulationClock, frame updates,
  mode policy, and save slots

SessionCommand / SessionCommandDispatcher
  semantic lifecycle command boundary for menu, UI, controller, replay, and
  tests to request new/load/save/mode changes without directly mutating session

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

SessionCommandLogCodec
  durable byte format for lifecycle command logs, including magic, version,
  command count, packet validation, and checksum rejection

SessionCommandLogFileStore
  file persistence boundary for lifecycle command logs, so replay scripts can be
  saved and loaded without mixing filesystem behavior into the codec

SessionScriptRunner
  high-level automation use case that loads a lifecycle script file and replays
  it through the same dispatcher used by live session actions

RuntimeLoopTypes
  app-layer contracts that group GameLoop settings, setup/output settings,
  per-frame reports, run summaries, and final results separately from the loop
  orchestration class

GameLoop
  app-facing shell that runs optional startup and inventory scripts, drains raw
  input, session, inventory script, inventory command, and movement sources,
  advances bounded frames, optionally saves run traces/debug bundles, and
  exposes results/events for tests and debug tools

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
  app-layer use case that runs one bounded frame: routes raw input, drains
  runtime sources, advances the session, and records the frame report

RuntimeRunSummary
  app-layer aggregate run result state for routed input, runtime command
  results, queued movement counts, frames run, and final frame events

RuntimeRunRecorder
  app-layer recorder that turns per-frame runtime work into RuntimeFrameReport
  entries and RuntimeRunSummary aggregates

RuntimeRunFinalizer
  app-layer completion policy that records final session mode and applies
  configured output finalization to GameLoopResult

RuntimeExitCodePolicy
  app-layer policy that translates GameLoopResult setup and output failures
  into process-style success/failure exit codes

RuntimeOutputSettings
  app-layer artifact settings that group optional run trace and debug bundle
  destinations for GameLoop finalization

RuntimeOutputResult
  app-layer artifact result state that groups trace and debug bundle save
  attempts after GameLoop output finalization

RuntimeOutputFinalizer
  app-layer use case that applies RuntimeOutputSettings to a GameLoopResult and
  reports trace or debug bundle write failures

RuntimeFrameReport
  per-frame report that groups routed input counts, runtime source results,
  event deltas, and simulation frame output for debug/test inspection

RuntimeFrameTrace
  readable text formatter for RuntimeFrameReport, suitable for logs, debug
  overlays, and test diagnostics

RuntimeFrameTraceFileStore
  file persistence boundary for readable runtime frame trace lines

RuntimeTraceService
  high-level use case that formats and saves complete GameLoopResult frame
  traces in one call, including GameLoop-configured trace output

RuntimeDebugArtifactBundle
  app-layer debug artifact writer that prepares a bundle directory, saves the
  run trace, writes a manifest describing the captured run, and can be invoked
  directly or through GameLoopSettings

RuntimeSourceDrainer
  app-layer helper that drains runtime session, inventory script, inventory
  command, and movement sources in frame order

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
