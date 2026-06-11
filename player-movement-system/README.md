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
  Owns startup and the game loop.

src/combat
  Resolves deterministic attack damage and target defeat.

src/input
  Converts raw input into player-facing intent.

src/focus
  Decides which surface owns input right now.

src/commands
  Defines semantic movement commands and dispatches them.

src/targeting
  Resolves what is under a tile: empty space, item, enemy, NPC, or object.

src/interaction
  Converts targets into move, attack, pickup, talk, and interact intent.

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

ActionExecutor
  validates target/range/state and applies animation commitment

MovementEvent
  observable facts such as CommandAccepted, PathStarted, StepCommitted,
  ActionExecuted, and AnimationLocked

CommandLog
  replayable list of semantic movement commands

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
  collects per-frame facts, routes them into effect requests, applies allowed
  simulation-facing effects, and returns the frame output for presentation

EffectRouter
  maps factual movement/combat events into presentation requests without letting
  simulation systems know about UI, audio, VFX, rumble, or camera code

EffectApplier
  applies effect requests that legitimately touch simulation feel, such as
  turning a HitStop request into SimulationClock hit-stop
```
