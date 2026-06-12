# Movement Flow

## 1. Raw Input

The game receives hardware-specific input:

```text
mouse left click at screen position
keyboard WASD
controller stick direction
touch tap
replay command
network command
```

This belongs in `src/input`.

## 2. Intent

Raw input becomes a player intent:

```text
MoveTo(tile)
MoveDirection(direction)
StopMoving
```

Intent is still local. It can be blocked by UI focus, pause state, death,
stun, inventory mode, or targeting rules.

## 3. Action Gate

Before intent becomes a command, it must pass the player action gate:

```text
alive?
not paused?
not stunned?
not animation locked?
gameplay owns movement input?
```

This is the local equivalent of DevilutionX's `CanPlayerTakeAction()` plus its
UI movement router.

`PlayerActionGate` reports the first `PlayerActionBlockReason`, not just a
boolean. That keeps the actual command builders simple while preserving the
reason movement was blocked: focus ownership, pause state, app-level animation
lock, uncancellable animation commitment, or stun. In a real game those reasons
become UI feedback, debug traces, controller hints, and tuning facts.
Runtime input routing carries that reason on `RuntimeInputRouteResult` for
movement-shaped input. The input is still not marked handled and no command is
queued, but tests and future debug surfaces can tell why the player's intent did
not become a command.

## 4. Command

Accepted intent becomes a semantic command:

```text
WalkTo { playerId, destination }
Stop { playerId }
```

This is the same kind of boundary DevilutionX uses with `CMD_WALKXY`.
The command does not know which device created it.

## 5. Dispatch

The dispatcher validates and routes commands to the correct system:

```text
WalkTo -> PlayerController::walkTo
       -> PlayerPathPlanner
Stop   -> PlayerController::stop
```

`PlayerController` owns command semantics such as stand-ground and stop.
`PlayerPathPlanner` owns the narrower path setup: finding a route, storing
pathing state, and publishing path-started or immediate-action-ready events.
Before any route runs, `MovementCommandValidator` rejects incomplete action
commands such as `MoveThenAct` without a destination action. The
`MovementCommandEventEmitter` publishes the accepted or rejected command fact.

## 6. Simulation

The movement system updates the player every tick:

```text
current tile
future tile
path queue
movement speed
animation lock
collision checks
state transitions
```

This is where movement should feel boring and reliable.

## 7. Feel Constraints

Movement also carries small rules that shape feel:

```text
diagonal path cost
corner clipping policy
stand-ground modifier
cancel windows
future-position commitment
```

These are not polish after the fact. They define whether movement feels fair,
readable, and skill expressive.

## 8. Destination Actions

Some movement is pure locomotion:

```text
empty tile -> WalkTo
```

But many game actions are movement plus a queued destination action:

```text
enemy  -> MoveThenAct(Attack)
item   -> MoveThenAct(Pickup)
NPC    -> MoveThenAct(Talk)
object -> MoveThenAct(Interact)
```

Stand-ground can transform the same target into:

```text
enemy or direction -> StandAndAct(Attack)
```

This keeps click, controller, touch, replay, and network behavior on one shared
semantic command path.

## 9. Action Execution

Movement does not directly perform attacks, pickups, dialogue, or object use.
It only delivers the player into `Acting`.

`DestinationActionBuilder` is the narrow translation from an interaction intent
to the action payload movement will carry: attack, pickup, talk, interact, and
the range each one requires.

```text
PlayerMovement consumes path
  -> PlayerAnimationLockGate
  -> PlayerPathStepper
  -> PlayerMoveState::Acting
  -> PlayerActionRunner
  -> ActionExecutor
  -> ActionRules
  -> AnimationLock
```

`PlayerPathStepper` is deliberately narrower than pathfinding. Pathfinding picks
a route; the stepper consumes one queued route tile, checks whether that tile is
still legal, delegates position mutation to `ActorStepCommitter`, and reports
whether arrival should hand control to the action executor.

`ActorStepCommitter` is shared by player and enemy movement. It does not choose
where to go, check collision, emit events, or change move state. It only commits
the already-approved tile into `ActorPosition`: old tile becomes previous, and
tile/future/precise become the next tile.

`PlayerAnimationLockGate` runs before path stepping. It lets animation
commitment block movement for a predictable window, then emits
`AnimationUnlocked` when input may affect the actor again.

`PlayerActionRunner` owns the small but important handoff from movement state to
action execution. It decides whether an arrived player should idle or enter
`Acting`, then delegates the target/range/consequence rules to `ActionExecutor`.

The executor owns questions like:

```text
is target still valid?
is target in range?
is player allowed to act?
how long is the action commitment?
should the destination action clear?
```

## 10. Events

State changes publish semantic events:

```text
CommandAccepted
CommandRejected
PathStarted
PathBlocked
StepCommitted
DestinationActionReady
ActionExecuted
ActionRejected
AnimationLocked
AnimationUnlocked
```

Events let tests, debug tools, UI, audio, VFX, replay, and telemetry observe the
movement system without being hardwired into it.

## 11. Replay

Replay records semantic commands, not raw hardware input:

```text
MovementCommand -> CommandLog
CommandLog -> CommandReplayer -> CommandDispatcher
CommandDispatcher -> MovementCommandDispatchResult
MovementCommandDispatchResult -> CommandReplayReport
MovementEvent -> EventRecorder
```

That means the same input, network, replay, and test paths all exercise the same
movement command pipeline. `MovementCommandDispatchResult` gives replay tools a
direct accepted/rejected report without scraping the event stream, while
`CommandReplayReport` summarizes the whole replay as accepted/rejected counts.
`MovementEvent` still records the frame-visible consequences.

Movement replay can also cross a durable byte boundary:

```text
CommandLog
  -> CommandLogCodec
  -> CommandLogFrameCodec
  -> CommandPacketListCodec
  -> MovementCodec packet bytes
  -> CommandLogChecksum
```

`CommandPacketListCodec` owns the counted packet-byte payload. `CommandLogFrameCodec`
owns the replay-file shell: magic bytes, version, payload, and checksum. The
top-level `CommandLogCodec` is the only layer that turns decoded packet bytes
back into semantic `MovementCommand` values, so corrupt or invalid command logs
are rejected before they reach `CommandReplayer`.

Saving a movement replay adds one more edge boundary:

```text
CommandLog
  -> CommandLogCodec
  -> CommandLogFileStore
  -> ByteFileStore
```

Loading reverses that path. A missing or corrupt `.imcl` file returns no
`CommandLog`, so replay does not have to know about filesystem errors or bad
byte streams.

The script runner is the use-case layer over files and replay:

```text
movement script path
  -> MovementScriptRunner
  -> CommandLogFileStore
  -> CommandLog
  -> CommandReplayer
  -> CommandDispatcher
```

`MovementScriptRunner` reports load failure separately from command rejection.
That distinction matters because a valid movement script can still ask for an
illegal action in the current world state, and that should be visible as replay
data rather than confused with a missing or corrupt file.

`RuntimeMovementScriptIntake` is the runtime-world adapter above that runner. It
checks whether an active world exists, builds the player controller and command
dispatcher for that world, and then lets `MovementScriptRunner` own file loading
and replay.

`RuntimeMovementScriptBatchRunner` is the ordered batch layer above the intake.
Once runtime movement script sources have been drained into a flat path list, it
runs each path against the active world and returns one result per path without
stopping the batch on load failure or command rejection.

`MovementScriptSource` is the runtime-facing queue boundary for those script
paths:

```text
debug menu / test / automation
  -> MovementScriptSource
  -> movement script path[]
```

It does not load files or dispatch commands. It only gives app/runtime code the
same drain-once source shape already used by movement commands and inventory
scripts.

## 12. Network Codec

Networking should transmit semantic commands, not raw input:

```text
MovementCommand
  -> MovementPacket
  -> bytes
  -> MovementPacket
  -> MovementCommand
  -> CommandDispatcher
```

The codec is deliberately socket-free. It only proves that command data is
explicit, compact, and stable enough to cross a network boundary.

## 13. Enemy Pressure

Enemies build on player movement by adding constrained pressure:

```text
enemy position
  -> EnemyPursuitStepPlanner
  -> EnemyPursuitStepGate
  -> EnemyPursuitStepper
  -> EnemyPursuitBudget
  -> EnemyAttackRange
  -> EnemyPursuitResult
  -> EnemyMovementReporter
  -> EnemyPursuitEventEmitter
  -> MovementEvent::EnemyPursuitStopped
  -> EnemyAttackRunner
  -> EnemyAttackEntryPolicy
  -> EnemyAttackPhaseRunner
  -> EnemyAttackRestartPolicy
  -> EnemyAttackResult
  -> EnemyMovementReporter
  -> EnemyAttackEventEmitter
  -> MovementEvent::EnemyAttackTransitioned
  -> attack windup
  -> attack recovery
```

The goal is not simply to reach the player. The goal is to stay inside a fair
reaction window: readable enough to answer, fast enough to matter.
`EnemyPursuitStepPlanner` chooses the next chase tile only.
`EnemyPursuitStepGate` answers whether that proposed tile is both walkable and
unblocked. `EnemyPursuitBudget` names the max-steps-per-tick pressure limit so
enemy speed is explicit tuning, not an accidental loop counter.
`EnemyPursuitStepper` owns the chase flow and stops when the enemy reaches
`EnemyAttackRange`, leaving the next frame to start windup through
`EnemyAttackRunner`. It returns `EnemyPursuitResult` so tests, traces, and future
AI choices can tell whether pursuit stopped from budget, blocking, already being
at the target, or reaching attack range. `EnemyPursuitEventEmitter` publishes
that outcome as `MovementEvent::EnemyPursuitStopped`, which means frame capture
and runtime traces can explain enemy pressure without peeking into AI internals.
The attack runner reuses the same range rule through `EnemyAttackEntryPolicy`
when deciding whether to enter windup. `EnemyAttackPhaseRunner` owns active
windup/recovery timer advancement and phase timer resets. `EnemyAttackRestartPolicy`
owns the recovery-exit choice: restart windup if the target is still in range, or
release the frame so pursuit can resume. `EnemyAttackRunner` still owns attack
sequencing, returns `EnemyAttackResult`, and `EnemyAttackEventEmitter` publishes
windup/recovery transitions as `MovementEvent::EnemyAttackTransitioned`.
`EnemyMovementReporter` is the single reporting boundary that owns both
publishing paths. Pursuit uses the same `ActorStepCommitter` as player pathing
once a pursuit step is known to be legal.

## 14. Combat Resolution

Movement creates opportunity. Action execution commits the move. Combat creates
the consequence:

```text
DestinationAction::Attack
  -> ActionExecutor
  -> CombatSystem
  -> CombatResolver
  -> CombatResult
```

The current resolver is deterministic:

```text
damage = max(1, attackPower - defense)
target.hp -= damage
hp <= 0 -> defeated
```

That keeps combat testable before adding randomness, status effects, hit chance,
or equipment rules.

Combat also emits consequence events:

```text
CombatHit(damage, remainingHp, target)
CombatDefeated(damage, remainingHp = 0, target)
CombatRejected(target)
```

These events are for UI, floating damage numbers, audio, VFX, replay comparison,
and tests.

Enemy attacks use the same consequence path:

```text
EnemyMovement
  -> EnemyAttackRunner
  -> attack windup completes
  -> CombatSystem::resolveEnemyAttack
  -> CombatResolver
  -> CombatEvent
  -> recovery
```

That keeps player and enemy attacks on the same damage rules.

## 15. Simulation Tick

Focused lesson:
[03. Command To Simulation](movement/03-command-to-simulation.md).

The tick layer is where the separate systems become one frame:

```text
CommandQueue
  -> SimulationTickPipeline
  -> SimulationCommandDrainer
  -> SimulationCommandQueueDrainStep
  -> CommandDispatcher
  -> PlayerController
  -> SimulationActorUpdater
  -> SimulationPlayerUpdater
  -> SimulationPlayerMovementRunner
  -> PlayerMovement
  -> ActionExecutor
  -> SimulationEnemyUpdater
  -> SimulationEnemyTargetSelector
  -> SimulationEnemyMovementRunner
  -> EnemyMovement
  -> CombatSystem
```

The order matters. `SimulationTickPipeline` applies the frame policy at the
tick-stage boundary. Commands are drained first so fresh input can affect this
frame. `SimulationActorUpdater` then applies the actor policy and owns the actor
order: players before enemies. `SimulationPlayerUpdater` delegates the
player-side actor wiring to `SimulationPlayerMovementRunner`, which builds
`PlayerMovement` plus the `ActionExecutor` that resolves ready destination
actions. Player movement updates before enemy movement so enemies respond to the
latest committed player position. `SimulationEnemyUpdater` owns target choice
through `SimulationEnemyTargetSelector`, then delegates world-service wiring to
`SimulationEnemyMovementRunner`. These runners keep dependency wiring testable
instead of hidden inside actor movement. Actions and combat consequences happen
inside those actor updates, but still publish events instead of directly owning
UI, audio, VFX, or networking.

This is the first point that starts to look like a small game loop.

## 16. Frame Policy

Not every frame is a gameplay frame. Pause, inventory, replay, and prediction
all need different answers to the same question:

```text
should this frame accept commands?
should players advance?
should enemies advance?
```

`SimulationFramePolicy` keeps those answers at the tick boundary. That prevents
pause, inventory, and replay rules from leaking into pathfinding, combat, enemy
AI, or raw input mapping.

`SimulationFramePolicyDescriber` is the inspection side of the same idea. It
turns a `SimulationMode` into a name, a short reason, and the exact
`SimulationFramePolicy` gates that the tick pipeline will use. That is useful
for debug overlays, tests, logs, tutorials, or network diagnostics because those
surfaces can explain the mode without duplicating policy logic.

The current modes are:

```text
Gameplay          commands, players, enemies
Replay            commands, players, enemies
NetworkPrediction commands, players
Paused            nothing advances
Inventory         nothing advances
```

## 17. Time Control

Frame policy answers what may run. Time control answers how much time each
running system receives.

```text
raw frame delta
  -> SimulationClock
  -> SimulationTimeStepBuilder
  -> SimulationTimeStep
  -> SimulationTick
  -> SimulationTickPipeline
  -> SimulationActorUpdater
  -> player delta / enemy delta / animation delta
```

The clock currently supports:

```text
time scale  slows or freezes actor time
hit-stop    consumes raw time before actors advance
```

This lets impact freeze movement and enemy windup without teaching pathfinding,
commands, combat, or input mapping about hit-stop. Commands can still be
accepted during hit-stop, but actor updates wait until actor time resumes.
`SimulationTimeStepBuilder` is the frame-runner boundary that decides whether
raw delta passes straight through or is consumed by `SimulationClock` first.

## 18. Effect Routing

Movement and combat events are factual. Effects are requests for presentation or
feel:

```text
MovementEvent::StepCommitted -> EffectRequest::Footstep
MovementEvent::PathBlocked   -> EffectRequest::BlockedFeedback
CombatEvent::Hit             -> DamageNumber + HitImpact + HitStop
CombatEvent::Defeated        -> DamageNumber + DefeatCue + HitStop
```

The split matters:

```text
simulation says what happened
effect routing says how the player should notice
presentation systems decide how to render it
```

This keeps combat free of UI, audio, VFX, rumble, camera shake, and time-control
dependencies while still giving hits, steps, and blocked paths immediate feel.

## 19. Effect Application

Routing creates requests. Application lets selected requests affect engine feel:

```text
CombatEvent::Hit
  -> EffectRouter
  -> EffectRequest::HitStop
  -> EffectApplier
  -> SimulationClock::triggerHitStop
```

That is the narrow bridge back into simulation. Combat still does not know that
hit-stop exists, and the clock still does not know that combat exists.

## 20. Frame Event Pipeline

The frame runner turns the pieces into a normal loop:

```text
SimulationFrameRunner
  -> SimulationTimeStepBuilder
  -> SimulationClock::step
  -> SimulationFrameTickRunner
  -> SimulationFrameEventCapture
  -> SimulationTick
  -> SimulationFrameEvents
  -> SimulationFrameFinalizer
  -> SimulationTargetFinalizer
  -> TargetSynchronizer
  -> SimulationInventoryFinalizer
  -> InventoryService
  -> SimulationEffectFinalizer
  -> SimulationEffectPipeline
  -> EffectRouter / EffectApplier
  -> frame output for presentation
```

`SimulationFrameTickRunner` owns the tick-stage event boundary. It installs
frame event capture, runs `SimulationTick`, restores the world's previous event
sinks, and returns the collected events. `SimulationTick` adapts raw/default
tick inputs into `SimulationTickPipeline`, which is the named gameplay order:
command intake first, actor updates second. That leaves `SimulationFrameRunner`
to coordinate time-step building and post-tick finalization.

This gives each frame a clean consequence phase:

```text
state changes happen during tick
facts are collected as events
event capture forwards to existing observers and restores world sinks
target and inventory consequences are reconciled after the tick
effect requests are derived after the tick
approved simulation-facing effects are applied
presentation can consume the remaining frame output
```

The runner temporarily redirects movement and combat events into
`SimulationFrameEvents`, forwards them to any existing sinks, then restores the
world sinks after the tick. That keeps event capture local to a frame without
stealing events from tests, debug tools, UI, or telemetry.

## 21. Snapshot State

Save/load starts by naming durable state:

```text
players
enemies
combat registry
```

Those are state. These are not:

```text
movement events
combat events
effect requests
damage numbers
hit sparks
footsteps
```

`SnapshotWriter` copies durable state out of `SimulationWorld`.
`SnapshotReader` restores that state into a world.

This split matters because events and effects are consequences of a frame, not
facts that should be restored later. A save file should restore where the world
is, not replay the footstep sound that happened while saving.

## 22. Snapshot Codec

An in-memory snapshot is useful for tests and debugging. A mature save boundary
also needs a stable byte format. The detailed save architecture is documented in
[`save-system.md`](save-system.md); the short flow is:

```text
SimulationSnapshot
  -> SnapshotCodec
  -> SnapshotSchemaCodec
  -> serialized durable sections
  -> SnapshotFrameCodec
  -> magic bytes
  -> format version
  -> SnapshotChecksum
```

The codec validates before trusting data:

```text
bad magic       -> reject
bad version     -> reject
truncated bytes -> reject
invalid enum    -> reject
checksum mismatch -> reject
```

This is save compatibility work. Serialization is not just copying memory. It
is a contract about what durable state means, how sections are ordered, and
which old or corrupt bytes the game is willing to load.

## 23. Snapshot File Store

File persistence sits outside snapshot shape and byte encoding:

```text
SimulationSnapshot
  -> SnapshotCodec
  -> SnapshotFileStore
  -> ByteFileStore
  -> file
```

Loading runs the reverse direction:

```text
file
  -> ByteFileStore
  -> SnapshotFileStore
  -> SnapshotCodec validation
  -> SimulationSnapshot
  -> SnapshotReader
  -> SimulationWorld
```

The file store does not know what a player, enemy, or combatant means. It only
knows that snapshot bytes should cross a binary file boundary. ByteFileStore owns
the raw temp-file write, rename, and byte load mechanics. That keeps save format
rules, domain save/load intent, and filesystem failure rules separate.
`file_store_tests` owns this generic filesystem contract so snapshot, command
log, and trace tests can stay focused on their domain formats.

## 24. Save Game Service

The use-case layer is what game code should call:

```text
SaveGameService::saveWorld
  -> SnapshotWriter
  -> SnapshotFileStore
  -> SnapshotCodec
  -> file
```

Loading is the reverse:

```text
SaveGameService::loadWorld
  -> SnapshotFileStore
  -> SnapshotCodec
  -> SnapshotReader
  -> SimulationWorld
```

This keeps the simulation from knowing about file paths, byte formats, temp
files, or version checks. The world owns state; the save service owns the save
workflow.

## 25. Save Slots

Save slots separate menu-facing metadata from the save payload:

```text
slot id
  -> path
  -> occupied?
  -> valid?
  -> player tile / hp / enemy count
```

The slot service lets a menu or debug tool list saves without restoring them
into the active world:

```text
SaveSlotService::listSlots
  -> SnapshotFileStore validation
  -> SaveSlotMetadata
```

Only explicit load goes into `SimulationWorld`:

```text
SaveSlotService::loadSlot
  -> SaveGameService::loadWorld
```

`save_snapshot_persistence_tests` owns this durable persistence layer: snapshot
shape, byte/schema/entity/player/enemy codecs, frame/checksum validation,
snapshot files, save-game workflow, and save-slot metadata/load behavior.

This is the same separation mature games need: the save selection screen should
be able to show valid, empty, and corrupt slots without mutating gameplay state.

## 26. Game Session

The session owns the objects a real game loop needs:

```text
GameSession
  -> SimulationWorld
  -> SimulationClock
  -> NewGameWorldBuilder
  -> SessionModePolicy
  -> SessionModeChanger
  -> SessionWorldSlotLoader
  -> SessionWorldSlotSaver
  -> SessionFrameUpdater
  -> SimulationFrameRunner
  -> SaveSlotService
```

It exposes lifecycle actions:

```text
startNewGame
  -> NewGameWorldBuilder
saveToSlot
  -> SessionWorldSlotSaver
loadFromSlot
  -> SessionWorldSlotLoader
setMode
  -> SessionModeChanger
update
  -> SessionFrameUpdater
```

SessionModePolicy maps session mode to simulation mode and frame policy:

```text
Gameplay  -> Gameplay policy, commands and actors advance
Paused    -> Paused policy, commands and actors stop
Inventory -> Inventory policy, commands and actors stop
Empty     -> Paused policy, no active world updates
```

Failed loads do not destroy the active world. Successful new/load operations
reset transient clock state, but preserve event sinks. That distinction keeps
durable state, frame feel, and external observers from bleeding into each other.
`session_state_tests` owns this layer: creating the world, loading/saving
through session helpers, mode policy/changing, paused update behavior, and
failed-load preservation. Command codecs and lifecycle scripts sit above it.

## 27. Session Commands

Session commands give lifecycle actions the same semantic boundary movement
already has:

```text
menu click / hotkey / controller / replay
  -> SessionCommand
  -> SessionCommandDispatcher
  -> SessionCommandApplier
  -> GameSession
  -> SessionEventEmitter
  -> SessionEventSink
```

The command layer covers:

```text
StartNewGame
SaveSlot
LoadSlot
SetMode
```

It returns a result:

```text
Applied
Rejected
```

That matters because lifecycle commands can fail. Saving an empty session,
loading a missing/corrupt slot, or setting a mode without a payload should be
visible to UI, tests, logs, and replay tools.

## 28. Session Events

Lifecycle commands produce session events:

```text
StartNewGame -> GameStarted
SaveSlot     -> SaveCompleted / SaveFailed
LoadSlot     -> LoadCompleted / LoadFailed
SetMode      -> ModeChanged / ModeChangeRejected
```

These are facts about the lifecycle layer. They let menu UI, logs, replay
tools, and tests observe what happened without reaching into `GameSession` or
duplicating dispatcher rules.

`SessionEventEmitter` owns the event payload edge: copy the command type,
optional slot id, and optional mode into the emitted `SessionEvent`, while
cleanly ignoring missing sinks. That keeps `SessionCommandDispatcher` focused
on dispatch order instead of event formatting.

This mirrors the lower-level split:

```text
movement command -> movement event
combat action    -> combat event
session command  -> session event
```

## 29. Session Replay

Session replay records lifecycle commands, not UI clicks. The detailed replay
architecture is documented in [`lifecycle-replay.md`](lifecycle-replay.md); the
short path is:

```text
SessionCommand
  -> SessionCommandLog
  -> SessionCommandReplayer
  -> SessionCommandDispatcher
  -> GameSession
  -> SessionEvent
```

That keeps lifecycle replay on the same path as normal menu/controller actions.
It can reproduce success and failure:

```text
StartNewGame -> Applied
SaveSlot     -> Applied / Rejected
LoadSlot     -> Applied / Rejected
SetMode      -> Applied / Rejected
```

This is useful for tests, debug tools, deterministic boot flows, and eventually
network/session synchronization.

## 30. Session Command Codec

Session commands now have a stable byte boundary:

```text
SessionCommand
  -> SessionCommandPacket
  -> SessionCommandPacketValidator
  -> SessionCommandPacketByteCodec
  -> bytes
  -> SessionCommandPacketByteCodec
  -> SessionCommandPacket
  -> SessionCommandPacketValidator
  -> SessionCommand
```

The codec validates command shape:

```text
StartNewGame may carry new-game settings
SaveSlot     must carry a slot id
LoadSlot     must carry a slot id
SetMode      must carry a valid mode
unexpected payload fields are rejected
invalid enum values are rejected
wrong byte sizes are rejected
```

This gives lifecycle automation and future networking the same kind of explicit
boundary that movement commands already have.

## 31. Session Command Log Codec

A single lifecycle command can cross a byte boundary. A full lifecycle script
needs framing. The log byte stack is split into packet bytes, counted packet
lists, frame metadata, and checksum validation:

```text
SessionCommandLog
  -> SessionCommandLogFrameCodec
  -> magic
  -> version
  -> SessionCommandPacketListCodec
  -> counted SessionCommandPacket[]
  -> SessionCommandLogChecksum
  -> checksum
```

The log codec rejects:

```text
bad magic
bad version
truncated bytes
checksum mismatch
invalid embedded command packet
wrong command count / payload size
```

This is the durable replay-file shape: not raw UI input, but semantic lifecycle
commands that can be replayed through the same dispatcher as live session
requests.

## 32. Session Command Log File Store

The codec defines valid bytes. The file store defines how those bytes cross the
filesystem:

```text
SessionCommandLog
  -> SessionCommandLogCodec
  -> SessionCommandLogFileStore
  -> ByteFileStore
  -> temp file
  -> rename into place
  -> file bytes
  -> ByteFileStore
  -> SessionCommandLogFileStore
  -> SessionCommandLogCodec
  -> SessionCommandLog
```

This keeps three jobs separate:

```text
SessionCommandLogCodec      validates replay-file structure
ByteFileStore               handles raw binary file IO and temp-file rename
SessionCommandLogFileStore  maps raw bytes to missing/corrupt lifecycle logs
SessionCommandReplayer      applies loaded commands through GameSession
```

The important lesson is that durable automation is still not raw input. A saved
boot script, debug reproduction, or test fixture should contain semantic session
commands and then pass through the same dispatcher as live menu/controller
actions.

## 33. Session Script Runner

Once a lifecycle command log can live on disk, a higher-level use case can run
it:

```text
script file
  -> SessionCommandLogFileStore
  -> SessionCommandLog
  -> SessionCommandReplayer
  -> SessionCommandDispatcher
  -> GameSession
  -> SessionEvent
```

The runner reports two separate kinds of failure:

```text
LoadFailed  the script file was missing or invalid, so no command dispatched
Completed   the script loaded and every command produced an Applied/Rejected result
```

That distinction matters. A corrupt boot script is a file/format problem. A
valid script that tries to load an empty slot is a lifecycle rule problem. They
should be visible at different levels, because UI, tests, and tooling recover
from them differently.
`session_lifecycle_command_tests` owns this full lifecycle-command pipeline:
live dispatch, event emission, replay, byte/log codecs, file-store behavior,
and script execution. That keeps menu/save/load automation out of broad
movement tests while preserving one semantic path for live and replayed session
requests.

## 34. Game Loop Shell

The app layer can now run a minimal bounded loop:

```text
GameLoopSettings
  -> RuntimeSetupSettings
  -> optional startup script
  -> RuntimeStartupScriptIntake
  -> optional inventory script
  -> InventoryScriptRunner
  -> RuntimeSourceSettings
  -> RuntimeInputSettings
  -> RuntimeFrameSettings
  -> GameSession
  -> fixed number of frame updates
  -> GameLoopResult
```

It intentionally does not parse devices, mutate player state directly, or load
files by hand. It composes existing boundaries:

```text
startup file  -> SessionCommandLogFileStore
startup run   -> RuntimeStartupScriptIntake -> SessionCommandDispatcher
inventory file -> InventoryCommandLogFileStore
inventory run -> InventoryCommandDispatcher
frame update  -> GameSession::update
events        -> SessionEventRecorder / InventoryEventRecorder
```

`RuntimeStartupScriptIntake` is the app-layer adapter above the lifecycle
script runner. It wires startup replay to the same `SessionCommandDispatcher`
used by live session commands, while `SessionScriptRunner` keeps owning file
loading and replay.

`RuntimeInventoryScriptIntake` is the runtime-player adapter above the
inventory script runner. It checks whether the selected player exists, wires the
inventory event sink into `InventoryCommandDispatcher`, and then lets
`InventoryScriptRunner` own file loading and replay.

`RuntimeInventoryScriptBatchRunner` is the ordered batch layer above the intake.
Once runtime inventory script sources have been drained into a flat path list,
it runs each path against the selected player and returns one result per path
without stopping the batch on load failure or command rejection.

This is the first runtime-facing shell around the movement/session system. It
is still testable because the loop is bounded and reports what happened instead
of hiding behavior behind an infinite platform loop.

`RuntimeLoopTypes.hpp` keeps the app-facing contracts separate from the shell
that executes them. `GameLoop` owns the orchestration, while the settings,
setup results, frame reports, output results, and run summary stay in plain
data types that tests, trace writers, output finalizers, and exit-code policy
can share without depending on the loop class itself.

`RuntimeSetupSettings` groups one-time configured setup scripts:

```text
GameLoopSettings::setup
  -> optional startup script
  -> optional configured inventory script
  -> optional configured movement script
```

Those scripts run before frame updates. The matching `RuntimeSetupResult`
captures whether each configured script ran and what result it produced.

`RuntimeSourceSettings` groups the live input/source streams:

```text
GameLoopSettings::sources
  -> RawInputSource[]
  -> SessionCommandSource[]
  -> MovementScriptSource[]
  -> MovementCommandSource[]
  -> InventoryScriptSource[]
  -> InventoryCommandSource[]
```

That keeps setup scripts, output artifacts, and runtime sources from becoming
one flat settings bag. Raw input sources are routed by `RuntimeInputSourceRouter`;
command and script sources are delegated to `RuntimeSourceDrainer`.
`game_loop_command_source_tests` covers the assembled-loop command-source edge:
runtime lifecycle commands can create/change sessions, startup scripts run
before runtime lifecycle commands, inventory commands need an active player,
and movement commands need an active world before they can reach simulation.
`game_loop_script_source_tests` covers the assembled-loop script-source edge:
runtime movement scripts replay against an active world, runtime inventory
scripts replay against an active player, load failures are reported without
stopping frames, and queued paths are preserved when no active receiver exists.

`RuntimeInputSettings` groups the active routing context:

```text
GameLoopSettings::input
  -> RuntimeInputBindings
  -> FocusState
  -> PlayerActionContext
  -> player id
  -> optional TargetResolver
```

Sources answer “where do commands/events come from?” Input settings answer “who
is controlling, what has focus, and how should raw input become commands?”

`RuntimeInputContextBuilder` turns those settings plus current session state
into the context used by `RuntimeInputRouter`:

```text
GameSession
  -> current mode
  -> active world, if any
  -> default world target registry, if any
RuntimeInputSettings
  -> player id
  -> focus/action state
  -> optional target resolver override
```

That keeps target fallback and focus/session state wiring out of the frame
runner. The router receives a complete context and only decides what a raw
event means inside it.
`runtime_input_hotkey_tests` owns this app-edge input context plus the boring
hotkey routes, so pointer movement and target interaction tests can stay focused
on gameplay-shaped input.

`RuntimeFrameSettings` groups the bounded loop controls:

```text
GameLoopSettings::frame
  -> maxFrames
  -> fixedDeltaSeconds
```

The shell still runs a testable bounded loop. The frame settings simply keep
the “how many frames?” and “how much simulated time per frame?” questions in
one place.

The inventory script runs after the startup script and before frame updates:

```text
startup script creates/loads the world
inventory script runs against the active player inventory
movement script dispatches replayed movement commands
runtime command sources drain
frame update ticks
```

If the inventory script file cannot load, the loop stops before ticking frames.
If the script loads but an inventory command rejects, the loop still reports a
completed inventory script with rejected command results.

The configured movement script follows the same setup rule. It runs after
startup and configured inventory setup, against the active world. A missing file
or no active world stops before frames. A loadable movement script whose command
rejects still allows frames, because that rejection is replay data, not setup
plumbing failure.

`RuntimeSetupResult` keeps those setup facts together:

```text
GameLoopResult::setup
  -> startup script attempted/result
  -> configured inventory script attempted/result
  -> configured movement script attempted/result
```

`RuntimeSetupRunner` owns the configured setup phase:

```text
RuntimeSetupSettings
  -> RuntimeSetupRunner
  -> RuntimeStartupScriptIntake
  -> RuntimeSetupFrameGate
  -> RuntimeSetupRunResult
  -> RuntimeSetupRunResultApplier
  -> setup result
  -> setup inventory command results
  -> frames allowed?
```

Startup script load failure stops before configured inventory setup. Configured
inventory setup failure also stops before configured movement setup and frames.
Configured movement setup failure also stops before frames. Loadable inventory
or movement scripts with rejected commands still allow frames, because the file
and setup pipeline worked and the rejection is command-level data.
`RuntimeSetupRunner` delegates the frame-start decision to
`RuntimeSetupFrameGate`, which uses `RuntimeSetupFailurePolicy` so setup gating
and run failure reporting use the same status rules.

`RuntimeSetupRunResultApplier` owns the handoff from setup execution to the run
result: copy the setup result, record setup inventory command results into the
summary, and return the frame gate. That keeps `RuntimeRunExecutor` focused on
lifecycle order instead of setup report plumbing.
`runtime_setup_script_tests` owns the assembled setup-script behavior around
this phase: startup intake, setup script order, load/no-player failures, and
the guarantee that configured setup movement scripts are not counted as runtime
movement script source results.
`runtime_setup_run_tests` owns this setup-to-run boundary along with the report
recorders that turn setup and frame work into `RuntimeRunSummary` and
`RuntimeFrameReport` facts.

That separates one-time setup automation from per-frame runtime sources. A
failed setup script can stop the loop before frames begin, while runtime script
source failures are reported per frame and the loop keeps ticking.

`RuntimeExitCodePolicy` translates completed run facts into a process-style
exit code:

```text
GameLoopResult
  -> RuntimeExitCodePolicy
  -> RuntimeRunFailurePolicy
  -> RuntimeExitCodeMapper
  -> RuntimeSetupFailurePolicy
  -> RuntimeOutputFailurePolicy
  -> 0 or 1
```

Setup load failures and requested artifact write failures return failure.
RuntimeRunFailurePolicy composes the setup and output decisions.
RuntimeSetupFailurePolicy owns the setup rule: startup load failure fails, and
configured inventory and movement setup must complete. RuntimeOutputFailurePolicy
owns the artifact-output rule: only attempted outputs that did not save are
failures. RuntimeExitCodeMapper maps that run-failure boolean to 0 or 1, and
RuntimeExitCodePolicy adapts the whole `GameLoopResult` to that mapper.
Command-level rejections inside a loadable setup script remain command or replay
results, so they do not automatically make the process fail.
`run_result_policy_tests` covers this completed-run policy boundary directly,
without invoking artifact writers or frame execution. That keeps "did the app
run fail?" separate from "how did this frame execute?" and "how are artifacts
written?"

Runtime inventory script sources are different from the configured setup script:

```text
menu / debug tool
  -> InventoryScriptSource
  -> script path
  -> InventoryScriptRunner
  -> InventoryCommandResult[]
```

The loop drains runtime inventory script sources after runtime session commands
and before direct inventory command sources:

```text
for each frame
  -> drain SessionCommandSource[]
  -> if world exists, drain InventoryScriptSource[]
  -> if world exists, drain InventoryCommandSource[]
  -> if world exists, drain MovementCommandSource[]
  -> GameSession::update
```

Configured inventory script failure stops setup. Runtime inventory script
failure is reported in `GameLoopResult::summary.runtimeInventoryScriptResults`
and the loop keeps ticking. That lets a debug menu try a script without taking
down the frame loop.

Runtime movement script sources use the same per-frame source idea for `.imcl`
replay files:

```text
debug tool / automation
  -> MovementScriptSource
  -> script path
  -> MovementScriptRunner
  -> CommandReplayReport
```

They run against the active world before direct runtime movement command
sources are queued. Their `MovementScriptRunResult` entries are stored in
`GameLoopResult::summary.runtimeMovementScriptResults` and each
`RuntimeFrameReport`, while `movementCommandsQueued` remains reserved for
commands placed into the world's command queue.

`RuntimeSourceDrainer` now owns that runtime source order:

```text
RuntimeSourceDrainerSettingsBuilder
  -> RuntimeSourceSettings
  -> RuntimeInputSettings::playerId
  -> RuntimeSourceDrainerSettings

RuntimeSourceDrainer
  -> RuntimeSourceStream drains source lists
  -> drain session command sources
  -> drain inventory script sources
  -> drain inventory command sources
  -> drain movement script sources
  -> drain movement command sources
```

`RuntimeSourceStream` owns the source-list mechanics that are common across
semantic source types: preserve order, skip missing source slots, drain each
source once, and return the drained items as a flat list. That lets
`RuntimeSourceDrainer` focus on what drained items mean: dispatch session
commands, run inventory scripts, dispatch inventory commands, or queue movement
commands into the active world. Movement scripts sit between inventory commands
and direct movement command queues: they dispatch decoded replay commands
through the normal movement dispatcher, then the next `GameSession::update`
advances any resulting player state.
`RuntimeSourceContext` owns the repeated active world/player checks used by
runtime sources. Inventory scripts and commands need a selected player; movement
scripts and movement commands need an active world. It exposes references for
callers that require an active receiver and nullable pointers for intake
boundaries that report missing receivers as normal runtime results. Keeping
that context check in one helper makes the source-draining rules easier to
compare.
`runtime_source_intake_tests` owns the direct source adapter contracts below
the frame source phase: queued sources drain once, intake adapters preserve
order and report missing receivers, and batch script runners continue after
load failures.

`RuntimeFrameRunner` owns the one-frame order around that drainer:

```text
RuntimeFrameRunner
  -> RuntimeFrameSourcePhaseRunner
  -> RuntimeFrameSimulationPhaseRunner
  -> RuntimeRunRecorder
```

`RuntimeFrameSourcePhaseRunner` owns the ordered pre-simulation source phase:
route raw input, drain session commands, run inventory scripts, dispatch
inventory commands, run movement scripts, and queue direct movement commands.
That keeps the frame runner focused on the frame lifecycle around the phase.
`RuntimeSessionFrameSourceStep` owns the session subsection of that phase:
route raw input first so hotkeys can enqueue lifecycle commands, then drain and
dispatch routed plus configured lifecycle command sources.
`RuntimeInventoryFrameSourceStep` owns the inventory subsection of that phase,
so script-driven inventory commands are recorded before direct inventory command
sources.
`RuntimeMovementFrameSourceStep` owns the movement subsection: run movement
scripts first, then queue direct movement commands for simulation.

`frame_source_tests` covers these subsection contracts directly. That keeps the
test focus on pre-simulation ordering without pulling in the full frame runner,
loop runner, or run executor lifecycle.

`RuntimeFrameSimulationPhaseRunner` owns the ordered simulation phase: record
the current frame policy, advance the active session, and record the resulting
simulation events. That keeps policy selection and event collection together as
the named phase that follows source intake.

`frame_simulation_tests` covers that phase plus the supporting report
recorders, so policy capture, event snapshots, and frame-completion deltas can
fail independently from source orchestration and full frame-loop lifecycle.

`RuntimeFramePolicyResolver` owns the app bridge from `GameSession` mode to
`SimulationFramePolicyDescription`. The simulation phase asks for the current
frame policy, records it, then advances the session; frame lifecycle code no
longer needs to know how session modes map to simulation gates.

`RuntimeSimulationFrameUpdater` owns the app bridge from frame settings to
session simulation. It advances `GameSession` by
`RuntimeFrameSettings::fixedDeltaSeconds` and returns the
`SimulationFrameEvents` that runtime reports store.

`RuntimeFrameLoopRunner` owns the bounded repetition policy around that
one-frame runner:

```text
RuntimeFrameLoopRunner
  -> repeat RuntimeFrameRunner::runFrame maxFrames times
```

`RuntimeRunExecutor` owns the high-level run lifecycle around setup, frame
loop, and finalization:

```text
RuntimeRunExecutor
  -> RuntimeSetupRunner
  -> RuntimeSetupRunResultApplier
  -> if frames allowed, RuntimeFrameLoopRunner
  -> RuntimeRunFinalizer
```

`frame_lifecycle_tests` covers this outer lifecycle boundary directly: one
frame must route/drain sources before simulation, the loop must repeat that
bounded frame the configured number of times, and the run executor must skip
frames after setup failure while still finalizing the result.

`GameLoop` still owns collaborator assembly and event/result access.
`RuntimeRunExecutor` owns lifecycle order, `RuntimeFrameLoopRunner` owns how
many bounded frames run, `RuntimeFrameRunner` owns one frame's mechanics,
`RuntimeInputSourceRouter` owns raw source routing for the frame, and
`RuntimeSourceDrainer` owns the repeated semantic source mechanics plus the
active-world checks needed before inventory and movement sources can safely
mutate state.
`RuntimeFinalModeRecorder` captures the final `GameSession` mode before output
artifacts are written, so traces and debug bundles describe the completed run
state instead of an earlier lifecycle moment.

`RuntimeRunSummary` keeps the cross-frame aggregates together:

```text
GameLoopResult::summary
  -> configured setup inventory command results
  -> runtime inventory script results
  -> raw input routed
  -> session and inventory command results
  -> movement commands queued
  -> frames run
  -> last frame events
```

That is different from `RuntimeFrameReport`. The summary answers “what happened
across the whole bounded run?” while the frame report answers “what happened on
this specific frame?” Configured setup inventory command results belong in the
summary, but they do not create frame reports because they ran before frames
began.

`RuntimeRunRecorder` owns the translation from per-frame work to those two
reporting shapes:

```text
begin frame
  -> remember event offsets
record source results and frame events
finish frame
  -> RuntimeFrameReport
  -> RuntimeRunSummary
```

That keeps the frame runner from manually copying every source result into two
places. The frame runner decides the runtime order, and the recorder decides
how that work becomes inspectable run data.
The focused setup/run tests cover recorder defaults and aggregation without
running the full assembled `GameLoop`, leaving assembled-loop report checks in
`game_loop_frame_report_tests`.

`RuntimeInputDrainReportRecorder` owns one slice of that reporting translation:
raw input drain results become the frame's routed-input count and movement block
reasons, then also accumulate into the run summary. That keeps source routing,
drain aggregation, and report recording as separate app-layer facts.

`RuntimeInventoryScriptReportRecorder` owns another slice: runtime inventory
script results are preserved as script results, while each script's command
results are flattened into the inventory command reports for the frame and the
run summary. That gives debug tools both views: "which scripts ran?" and "which
inventory commands actually applied or rejected?"

`RuntimeInventoryScriptIntake` owns the app-to-replay handoff for drained
inventory script paths. The source drainer decides when script paths are
drained; the intake decides whether a selected player can receive replayed
inventory commands and wires that player into `InventoryScriptRunner`.

`RuntimeInventoryScriptBatchRunner` owns the ordered runtime inventory script
batch. It keeps `RuntimeSourceDrainer` focused on source gating and path
draining while the batch runner turns the drained path list into ordered
`InventoryScriptRunResult` entries.

`RuntimeInventoryFrameSourceStep` owns the frame-level inventory source order:
drain inventory scripts, record script results and their flattened command
results, then drain and record direct inventory commands.

`RuntimeInventoryCommandReportRecorder` owns direct runtime inventory command
results. It appends those results after any script-flattened command results, so
the frame report preserves source order while the run summary keeps one
aggregate inventory command list.

`RuntimeInventoryCommandIntake` owns the app-to-inventory handoff for drained
inventory commands. When the selected player exists, commands dispatch through
`InventoryCommandDispatcher`; when the world exists but the selected player does
not, the same drained commands become rejected command results and rejected
inventory events.

`RuntimeSessionCommandIntake` owns the app-to-session handoff for drained
lifecycle commands. Runtime session sources and routed hotkeys both become a
single ordered command list, then the intake dispatches each command through
`SessionCommandDispatcher` before the frame records the results.

`RuntimeSessionFrameSourceStep` owns the frame-level session source order:
record raw input routing, drain routed and configured lifecycle command sources,
and record dispatch results before inventory or movement sources run.

`RuntimeSessionCommandReportRecorder` owns session command results. The frame
report receives the current frame's lifecycle command results, while the run
summary appends them to the cross-frame lifecycle history.

`RuntimeMovementScriptReportRecorder` owns runtime movement script results. It
records which movement scripts ran on the frame and across the run without
inflating `movementCommandsQueued`, which is reserved for direct movement
command source intake.

`RuntimeMovementScriptIntake` owns the app-to-replay handoff for drained
movement script paths. The source drainer decides when script paths are drained;
the intake decides whether there is a world that can receive replayed movement
commands and wires that world into `MovementScriptRunner`.

`RuntimeMovementScriptBatchRunner` owns the ordered runtime script batch. It
keeps `RuntimeSourceDrainer` focused on source gating and path draining while
the batch runner turns the drained path list into ordered
`MovementScriptRunResult` entries.

`RuntimeMovementFrameSourceStep` owns the frame-level movement source order:
drain movement scripts, record replay results, then drain direct movement
commands and record only the queued command count. Movement scripts describe
replay dispatch; direct movement commands describe queued simulation input.

`RuntimeMovementCommandReportRecorder` owns that direct queued movement command
count. The frame gets the count for this frame, while the run summary
accumulates command intake across frames.

`RuntimeMovementCommandIntake` owns the app-to-simulation handoff for drained
movement commands. Runtime sources produce semantic movement commands; the
intake step queues them into the active `SimulationWorld` command queue and
returns the queued count that frame reports use.

`RuntimeFrameEventReportRecorder` owns simulation frame event reporting. It
stores the current frame's event batch on the frame report and mirrors it to
`RuntimeRunSummary::lastFrameEvents`, making that summary field explicitly the
latest frame snapshot rather than an event log.

`RuntimeFrameCompletionReportRecorder` owns the final frame report handoff. It
attaches the session and inventory event deltas captured during the frame,
stores the completed frame report on the run result, and increments the run's
finished-frame count.

`RuntimeFrameEventDeltaCollector` owns the event-stream cursor for a frame. At
frame start it snapshots the current session and inventory event counts; at
frame completion it returns only the events emitted after that snapshot. That
keeps lifecycle delta math out of the report writer.

`RuntimeEventStreamDelta` owns the offset slicing rule used by the frame event
delta collector. Focused lesson:
[04. Frame Events To Reports](movement/04-frame-events-to-reports.md).

`RuntimeFramePolicyReportRecorder` owns the frame policy report assignment. The
frame runner still chooses the policy from session mode, while the recorder
stores the chosen policy description on the current frame report.

`RuntimeFramePolicyResolver` owns the session-mode to simulation-policy choice.
That keeps the frame runner's source order readable while preserving a named
place to test gameplay, inventory pause, paused, and empty-session gates.

`RuntimeSetupInventoryCommandReportRecorder` owns setup-phase inventory command
reporting. Setup commands run before bounded frames begin, so their command
results are appended to the run summary without creating a frame report or
incrementing `framesRun`.

`RuntimeSetupRunResultApplier` uses that recorder as the bridge between
`RuntimeSetupRunResult` and `GameLoopResult`. The executor can then treat setup
as one lifecycle gate: apply setup, run frames only when allowed, finalize.

`RuntimeSetupFrameGate` owns the positive setup question: can bounded frame
updates start from this accumulated setup result? It keeps the setup runner from
spreading direct failure-policy checks across every configured setup step.

`RuntimeOutputSettings` groups the app shell's optional artifact destinations:

```text
GameLoopSettings::output
  -> optional runTracePath
  -> optional debugBundlePath
```

`RuntimeOutputResult` mirrors that shape on the result side:

```text
GameLoopResult::output
  -> run trace save attempted/saved
  -> debug bundle save attempted/saved
```

`RuntimeOutputFinalizer` applies those settings after the loop has produced a
`GameLoopResult`:

```text
GameLoopResult
  -> RuntimeRunFinalizer
  -> RuntimeOutputFinalizer
  -> RuntimeArtifactOutputService
  -> RuntimeArtifactOutputPlan
  -> RuntimeArtifactOutputRequestRunner
  -> RuntimeRunTraceOutputStep
  -> RuntimeTraceService
  -> RuntimeDebugBundleOutputStep
  -> RuntimeDebugArtifactBundle
```

`RuntimeRunFinalizer` records the final session mode, then delegates configured
artifact writes to `RuntimeOutputFinalizer`. That keeps `RuntimeRunExecutor`
focused on lifecycle timing. `RuntimeOutputFinalizer` writes output results back
onto `GameLoopResult`, while `RuntimeArtifactOutputService` owns the app
artifact write policy. `RuntimeArtifactOutputPlan` owns the ordered request list
for enabled artifact outputs. Focused lesson:
[07. Artifact Output Order](movement/07-artifact-output-order.md).
`artifact_output_tests` covers this output boundary from settings and flag
updates through service/finalizer integration, without requiring a full
`GameLoop` run.
`RuntimeArtifactOutputRequestRunner` owns execution of one planned artifact
request.
`RuntimeRunTraceOutputStep` owns the run-trace artifact
transition: mark the trace as attempted, snapshot the in-progress
`GameLoopResult` for the trace writer, then record whether the write saved.
`RuntimeDebugBundleOutputStep` owns the same transition for debug bundles,
including the snapshot passed to the bundle writer. `RuntimeOutputResultBuilder`
owns the generic flag transition behind those artifact steps.
`RuntimeOutputFailurePolicy` interprets those output flags when exit-code logic
needs to know whether a requested artifact failed.

`GameLoopSettings::output.runTracePath` lets the app shell persist a full run
trace after the loop exits:

```text
GameLoopSettings::output.runTracePath
  -> GameLoopResult
  -> RuntimeTraceService
  -> run.trace
```

This save happens through the same finalization path whether the loop finishes
its frames or stops early during startup, inventory, or movement setup. `GameLoopResult`
reports both `output.runTraceSaveAttempted` and `output.runTraceSaved`, and
`GameLoop::run` returns failure when a requested trace cannot be written. That
makes trace output useful for command-line tools without making movement,
input, or inventory systems know about files.

`GameLoopSettings::output.debugBundlePath` follows the same app-shell rule for
a full debug bundle:

```text
GameLoopSettings::output.debugBundlePath
  -> GameLoopResult
  -> RuntimeDebugArtifactBundle
  -> RuntimeDebugArtifactLayout
  -> RuntimeDebugArtifactWriter
  -> RuntimeDebugManifest
  -> manifest.txt
  -> run.trace
```

`GameLoopResult::output` reports `debugBundleSaveAttempted` and
`debugBundleSaved`. `GameLoop::run` also returns failure when a requested bundle
cannot be written. The loop still does not know how a manifest is formatted or
how a trace is serialized; it only decides that configured debug artifacts
should be finalized after the run.
`game_loop_output_tests` covers this app-shell wiring directly: configured
trace and bundle paths save after normal runs and startup failures, while
invalid requested paths surface as failed output and process failure.

Each bounded frame now also produces a report:

```text
RuntimeFrameReport
  raw input routed this frame
  session command results this frame
  inventory script results this frame
  inventory command results this frame
  movement commands queued this frame
  movement/combat/effect frame events
  session event delta
  inventory event delta
```

`GameLoopResult` still keeps aggregate fields for convenience. The frame report
answers a different question: “what happened during this specific app frame?”
That is the shape a debug overlay, test harness, or trace logger wants when the
runtime shell starts coordinating several command and event streams.
`game_loop_frame_report_tests` covers the assembled-loop report handoff: source
results, policy, event deltas, simulation events, and summary mirrors all agree
after a real bounded frame completes.

`RuntimeFrameTrace` exports that structured report into deterministic text:

```text
frame rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=1 ...
policy mode=Gameplay acceptCommands=1 updatePlayers=1 updateEnemies=1 ...
inventoryResult[0] type=Applied command=EquipItem ...
inventoryEvent[0] type=Equipped ...
movementEvent[0] type=CommandAccepted ...
```

The trace formatter is intentionally downstream of the report. It does not
drive gameplay or mutate state. It only turns already-recorded runtime facts,
including the frame policy chosen for that mode, into readable lines.
`runtime_frame_trace_tests` owns that presentation boundary so report capture,
trace spelling, and trace persistence can keep changing independently.
`RuntimeFramePolicyText` owns the exact policy line spelling so traces and
debug bundle manifests stay consistent while still choosing numeric or word
booleans for their audience.
`RuntimeRunSummaryText` does the same for the run-level count summary: the trace
uses counts only, while the manifest asks for the same line with final mode
included. Its aggregate counts include both routed raw input and blocked
movement-shaped input.
`RuntimeMovementInputBlockSummary` turns the stored block reasons into a small
reason distribution for manifests and future debug UI. That keeps "how many
inputs were blocked?" separate from "why were they blocked?" without making
tools scrape detailed frame trace lines.
`RuntimeFrameTraceHeaderText` owns the per-frame count summary at the top of
each frame report: routed input, source results, queued movement, gameplay
events, session events, and inventory events.
`RuntimeFrameTraceSections` owns the deterministic order after the header and
policy: runtime source results first, lifecycle events second, simulation
events last. Movement input blocks are part of the runtime source section:
they explain why movement-shaped input was drained but did not become a routed
command. That keeps the trace readable as a frame story instead of a bag of
mixed event streams.
`RuntimePlayerActionText` owns the spelling for action block reasons, so focus,
pause, animation commitment, and stun vocabulary stays consistent between tests
and traces.
`RuntimeSessionText` owns the spelling for session command results and lifecycle
events, keeping startup/save/load/mode names consistent in runtime traces.
`RuntimeInventoryText` owns the spelling for inventory command results and
equipment events, keeping equip/unequip result names consistent in runtime
traces.
`RuntimeInventoryScriptText` owns the spelling for configured and runtime
inventory script outcomes: status, result count, applied commands, and rejected
commands.
`RuntimeMovementScriptText` owns the spelling for configured and runtime
movement script replay outcomes: status, result count, accepted commands, and
rejected commands. Traces use these formatters for per-frame script details,
while `RuntimeDebugManifest` uses them for setup and aggregate runtime
summaries. That makes a debug bundle useful even before opening the full run
trace.
`RuntimeMovementEventText` owns the spelling for movement events themselves:
player, tile, accepted command type, pursuit stop reason, pursuit steps, and
attack transition. That keeps the movement debug vocabulary next to movement
event types instead of mixing it into the whole-frame trace assembler.
`RuntimeCombatText` owns the spelling for combat event outcomes: hit/defeat/
rejection type, damage, and remaining hit points. Combat remains a gameplay
system, but its trace vocabulary is now isolated from the whole-frame trace
assembler.
`RuntimeEffectText` owns the spelling for effect requests: feedback type and
tile. Effects are presentation-facing, but tracing them beside movement and
combat helps show which simulation moments produced visible or timed feedback.

`RuntimeFrameTraceFileStore` persists those readable lines:

```text
RuntimeFrameReport
  -> RuntimeFrameTrace
  -> RuntimeFrameTraceFileStore
  -> TextFileStore
  -> frame.trace
```

This store is plain text on purpose. Unlike save files or command logs, trace
files are for humans and tooling to inspect what happened during a run, not for
restoring gameplay state. TextFileStore owns the raw line-oriented temp-file
write, rename, and load mechanics so trace classes can stay focused on runtime
meaning.
`file_store_tests` covers the shared text-store filesystem behavior separately
from trace formatting.

`RuntimeTraceService` is the use-case layer for full run traces:

```text
GameLoopResult
  -> frameReports[]
  -> RuntimeFrameTrace
  -> RuntimeFrameTraceFileStore
  -> TextFileStore
  -> run.trace
```

It adds a run-level summary before the per-frame lines. That gives a caller one
method for “save the trace for this run” while keeping formatting and filesystem
behavior testable as separate pieces.
`runtime_trace_persistence_tests` owns that boundary: line formatting is already
covered by trace tests, while this target proves readable traces survive the
file-store and full-run service boundary.

`RuntimeRunTraceFrameHeaderText` owns the `frame[n]` marker between run-level
summary and frame-level trace details. Focused lesson:
[05. Reports To Artifacts](movement/05-reports-to-artifacts.md).

`RuntimeDebugArtifactBundle` is the next app-layer wrapper around that trace:

```text
GameLoopResult
  -> RuntimeDebugArtifactBundle
  -> RuntimeDebugArtifactLayout
  -> RuntimeDebugArtifactWriter
  -> RuntimeDebugManifest
  -> TextFileStore
  -> manifest.txt
  -> run.trace
```

The bundle owns artifact assembly. The layout owns stable artifact path names.
`RuntimeDebugArtifactBundleResultBuilder` owns bundle result state: paths, root
preparation, and trace/manifest write flags. `RuntimeDebugArtifactRootPreparer`
owns bundle directory preparation. The writer owns trace/manifest write attempts
and reports their save flags. The manifest formatter owns readable manifest
lines.
`runtime_debug_artifact_tests` owns this artifact-writing boundary, so lower
trace and manifest tests can focus on line content while bundle tests focus on
paths, write results, and saved files.
`RuntimeDebugTraceWriteStep` owns the bundle trace-file write through the trace
service.
`RuntimeDebugManifestContextBuilder` maps the writer's artifact paths and trace
save result into the manifest context, so `RuntimeDebugArtifactWriter` does not
also own manifest context structure.
`RuntimeDebugManifestWriteStep` owns the manifest-file write: build context,
format the manifest, and persist readable lines. Focused lesson:
[06. Debug Bundle Manifest](movement/06-debug-bundle-manifest.md).
`RuntimeDebugManifestIndexText` owns the top manifest index lines: bundle
version and trace save state. That keeps artifact identity and save status
separate from run gameplay summaries.
`RuntimeDebugManifestSetupText` owns the setup attempt flags inside that
manifest, keeping startup, inventory setup script, and movement setup script
attempts readable as lifecycle state rather than artifact plumbing.
`RuntimeDebugManifestPathsText` owns the artifact path lines in the manifest:
bundle root, manifest filename, and trace filename. That keeps path reporting
consistent with the layout/writer boundaries without letting the manifest own
filesystem behavior.
`RuntimeDebugManifestSections` owns the deterministic order of the remaining
manifest groups: run status, setup details, and runtime script aggregates.
The manifest includes the latest frame policy summary so a bundle can explain
why the run accepted commands or advanced actors without opening the full trace.
That line is formatted through `RuntimeFramePolicyText`, the same boundary used
by `RuntimeFrameTrace`.
`runtime_debug_manifest_tests` owns the manifest presentation contract. Bundle
writer tests only need to prove those lines get persisted in the right artifact,
not re-prove every manifest section spelling.
Run-level routed-input and blocked-movement counts go
through `RuntimeRunSummaryText` for the same reason. Blocked movement reason
distribution goes through `RuntimeMovementInputBlockSummary`, so the manifest
can answer the common tuning question without opening the trace.
The trace still goes through `RuntimeTraceService`. That split keeps
replay/debug artifact shape outside gameplay code while leaving a clear place to
add future files, such as replay command logs or session metadata.

## 35. Runtime Session Command Sources

Startup scripts are only one way lifecycle commands enter the app. Menus,
debug consoles, controller shortcuts, platform events, and future network code
need the same kind of source boundary:

```text
menu / debug / script / network / live input
  -> SessionCommandSource
  -> SessionCommand
  -> SessionCommandDispatcher
  -> GameSession
```

The loop drains command sources before each frame:

```text
for each frame
  -> drain SessionCommandSource[]
  -> dispatch SessionCommand[]
  -> GameSession::update
  -> render/debug output
```

The useful constraint is that sources produce semantic lifecycle commands, not
raw device events. That keeps the app loop from learning about every possible
caller, and it gives tests a clean way to stand in for menu/UI/runtime behavior.

## 36. Runtime Movement Command Sources

The same source idea applies to gameplay movement:

```text
mouse / controller / replay / debug / network
  -> MovementCommandSource
  -> MovementCommand
  -> CommandQueue
  -> SimulationTick
  -> CommandDispatcher
  -> PlayerController
```

The app loop drains movement sources only when a session has an active world:

```text
for each frame
  -> drain SessionCommandSource[]
  -> if world exists, drain MovementCommandSource[]
  -> push MovementCommand[] into SimulationWorld::commandQueue
  -> GameSession::update
```

That avoids losing movement commands before a world exists, and it keeps the
normal simulation path intact. A movement source never moves the player
directly; it only supplies intent to the command queue that the simulation
already owns.

## 37. Runtime Input Router

Focused lesson: [01. Input To Intent](movement/01-input-to-intent.md). Keep this
section as the full reference sweep; use the focused doc for the shorter reading
path.

Next focused lesson:
[02. Intent To Command](movement/02-intent-to-command.md).

The app edge now has a small adapter from raw input to semantic command sources:

```text
RawInputEvent
  -> RuntimeInputRouter
  -> InputEventMatcher
  -> RuntimeSessionInputRouter
  -> RuntimeInputFocusResolver
  -> RuntimeMovementInputRouter
  -> RuntimeStopMovementInputStep
  -> RuntimeTargetInputRouter
  -> RuntimeBlockedPointerInputStep
  -> RuntimeMovementIntentInputStep
  -> SessionCommandSource or MovementCommandSource
  -> GameLoop
  -> dispatcher / command queue
```

Movement input still uses the lower-level movement path:

```text
RawInputEvent
  -> InputEventMatcher
  -> RuntimeInputFocusResolver
  -> RuntimeMovementInputContextBuilder
  -> InputFocus
  -> RuntimeBlockedPointerInputStep
  -> RuntimeMovementIntentInputStep
  -> InputMapper
  -> PlayerIntent
  -> PlayerActionGate
  -> IntentCommandBuilder
  -> MovementCommandSource
```

`InputEventMatcher` owns the boring device-shape checks: pressed keys, pressed
mouse clicks, and pressed touch taps. Routers then decide what those shapes mean.
That keeps "is this a pressed pointer?" separate from "does this become WalkTo,
Interact, Pause, or Stop?"

`RuntimeInputFocusResolver` owns the app-level control lockout: paused sessions
force menu focus, inventory sessions force inventory focus, and gameplay keeps
the provided focus state. That makes the common game rule visible: the same
button can mean movement during gameplay and nothing for movement while a menu
or inventory owns controls.

`RuntimeMovementInputContextBuilder` owns the per-event movement input state:
select the active player, apply session focus rules, preserve action context,
and compute the movement block reason that later input steps reuse.

`RuntimeMovementIntentInputStep` owns the generic movement route after
target-specific pointer input has first chance: map raw input to `PlayerIntent`,
ask the action gate to build a movement command, then queue that command.

`RuntimeBlockedPointerInputStep` owns blocked pointer reporting for the generic
movement path. It does not handle the input or queue a command; it returns the
movement block reason so frame reports can explain why movement-shaped input was
ignored.

`RuntimeStopMovementInputStep` owns the stop-hotkey route. It maps the configured
key to `PlayerIntentType::StopMoving`, asks the same action gate for permission,
and queues a `Stop` command at the player's current tile.

When a target resolver is attached, clicks can use the interaction path instead:

```text
RawInputEvent
  -> screenToTile
  -> TargetResolver
  -> RuntimeTargetInteractionInputStep
  -> InteractionIntentBuilder
  -> InteractionCommandBuilder
  -> MovementCommandSource
```

`RuntimeTargetInteractionInputStep` owns the inner interaction route: convert
the clicked screen position to a tile, resolve the target at that tile, build
interaction intent, build the movement command, and queue it.

That produces richer commands:

```text
empty tile     -> WalkTo
enemy/player   -> MoveThenAct(Attack)
item           -> MoveThenAct(Pickup)
npc            -> MoveThenAct(Talk)
object         -> MoveThenAct(Interact)
stand-ground   -> StandAndAct(Attack)
```

Lifecycle hotkeys produce session commands:

```text
pause key      -> SessionCommand::SetMode(Paused / Gameplay)
inventory key  -> SessionCommand::SetMode(Inventory / Gameplay)
```

`RuntimeSessionModeTogglePolicy` owns the requested mode for those hotkeys. It
does not apply the transition; it only decides what `SetMode` command to queue.
The session layer still validates and applies mode changes later. That keeps
input routing as a command producer instead of turning it into session state
mutation.
The stop hotkey follows the same rule on the movement side: it queues a `Stop`
command when the player can act, or reports the block reason without handling
the event when focus, pause, or action gates prevent movement.

The key lesson is that raw input stays at the app edge. Once routing finishes,
the rest of the engine still sees boring semantic commands with the same focus,
gate, dispatcher, and simulation rules as replay or tests.

## 38. Raw Input Sources In The Loop

The loop can now accept raw input sources directly:

```text
RawInputSource
  -> RawInputEvent[]
  -> RuntimeInputSourceRouter
  -> RuntimeRawInputDrainer
  -> RuntimeInputRouter
  -> RuntimeSessionInputRouter
  -> RuntimeMovementInputRouter
  -> RuntimeTargetInputRouter
  -> RuntimeBlockedPointerInputStep
  -> RuntimeMovementIntentInputStep
  -> routed SessionCommandSource / MovementCommandSource
  -> GameLoop drains command sources
  -> GameSession / SimulationWorld
```

`RuntimeInputSourceRouter` owns the frame-facing input route: build the current
input context from session state, drain raw input sources, and route handled
events into the semantic queues. `RuntimeRawInputDrainer` owns the lower-level
source-stream mechanics: drain each configured raw input source once, skip
missing source slots, route every event, and count only events the router
handled. `RuntimeInputRouter` still owns the meaning of a single event.
`RuntimeInputDrainResultBuilder` owns the aggregation step from individual
route outcomes to frame-level raw input counts and movement block reason lists.
`RuntimeInputRouteResultBuilder` owns the tiny result vocabulary those routers
return: unhandled, queued session command, queued movement command, and blocked
movement input. Keeping that vocabulary explicit matters because blocked
movement input is diagnostic, not a handled command.

That gives the app layer three input levels:

```text
raw input source       device-like events, only valid at the app edge
session command source lifecycle commands such as SetMode
movement command source gameplay movement commands such as WalkTo
```

The ordering is deliberate:

```text
startup script
  -> each frame:
     -> drain raw input sources
     -> route raw input into semantic queues
     -> dispatch session commands
     -> queue movement commands
     -> advance simulation frame
```

Blocked raw movement input is still drained, but it does not become a command.
That keeps stale clicks from firing later after focus changes, while preserving
the block reason for frame and summary diagnostics. `game_loop_raw_input_tests`
covers that assembled-loop edge directly: raw keys become lifecycle commands,
raw clicks become movement or interaction commands, world item targets can feed
pickup behavior, and blocked pointer input drains without mutating movement.
the rule that the simulation only receives commands that passed focus and action
gates.

## 39. World Target Registry

Target-aware input now has concrete world state behind it:

```text
SimulationWorld
  -> TargetRegistry
  -> TargetResolver
  -> RuntimeInputRouter
  -> InteractionIntentBuilder
  -> InteractionCommandBuilder
```

The registry stores clickable semantic targets:

```text
TargetType
TargetId
tile
```

A lookup either returns the registered target or an empty-tile target at the
clicked tile:

```text
registered enemy at { 4, 2 } -> TargetType::Enemy
no target at { 6, 2 }        -> TargetType::EmptyTile
```

`GameLoop` uses the active world's registry by default when routing raw pointer
input. That means runtime clicks can become `MoveThenAct(Attack)`,
`MoveThenAct(Pickup)`, `MoveThenAct(Talk)`, `MoveThenAct(Interact)`, or plain
`WalkTo` without the app loop knowing about enemies, items, NPCs, or objects.

The registry is also part of `SimulationSnapshot`, so save/load preserves what
the player can click. This keeps target state durable while still keeping
transient events, effects, and input history out of saves.

## 40. Target Synchronization

Clickable enemy targets are derived state. They should follow enemy movement and
combat life/death instead of being hand-maintained forever:

```text
enemies
  -> TargetSynchronizer
  -> TargetRegistry enemy targets
```

The synchronizer does two jobs after each frame:

```text
syncEnemyTargets
  remove stale enemy targets
  add current living enemy targets at their current tiles

removeDefeatedTargets
  observe CombatEventType::Defeated
  remove that target from the clickable registry
```

It preserves non-enemy targets:

```text
items / NPCs / objects stay registered
enemy targets are rebuilt from enemy + combat state
```

This matters because movement, combat, and input are now connected. If an enemy
moves, the click target must move with it. If combat defeats a target, the next
click should not keep attacking a dead thing just because stale target data was
left behind.

## 41. World Entity Service

Spawning an enemy is not just `world.enemies.push_back(...)`.

An enemy exists across multiple world tables:

```text
Enemy vector
  position, tuning, movement state

CombatRegistry
  attackable stats by target id

TargetRegistry
  clickable target at a tile
```

An item has fewer side tables, but it still needs coordinated lifecycle:

```text
Item vector
  id, tile

TargetRegistry
  clickable pickup target at a tile
```

`WorldEntityService` turns those into one operation:

```text
EnemySpawnRequest
  -> spawnEnemy
  -> enemies[]
  -> combat registry
  -> target registry

ItemSpawnRequest
  -> spawnItem
  -> items[]
  -> target registry
```

Despawning reverses the same ownership:

```text
despawnEnemy(id)
  -> remove enemy from enemies[]
  -> remove enemy combatant
  -> remove enemy click target

despawnItem(id)
  -> remove item from items[]
  -> remove item click target
```

Duplicate spawns replace the old entity-owned state first. That prevents stale
target tiles or old combat stats from surviving when an entity is recreated.

## 42. Inventory Pickup Transfer

Pickup ownership happens after the action layer succeeds:

```text
MoveThenAct(Pickup)
  -> PlayerMovement reaches target tile
  -> ActionExecutor emits ActionExecuted(Pickup, target)
  -> SimulationInventoryFinalizer
  -> InventoryService
  -> player.inventory.items
```

The transfer consumes world state:

```text
world item
  -> player inventory item
  -> remove from world.items
  -> remove item target from TargetRegistry
```

Capacity is checked at transfer time:

```text
inventory has room
  -> transfer item
  -> remove floor item

inventory full
  -> reject transfer
  -> keep floor item
  -> keep clickable item target
```

This keeps the important order intact:

```text
raw input
  -> target-aware command
  -> path/range/action rules
  -> successful pickup action
  -> inventory ownership
```

Inventory does not trust clicks directly. A stale or invalid pickup event does
not consume the item. That distinction matters because UI and input can request
things, but only the simulation/action layer decides what actually happened.
Likewise, a pickup animation can execute while inventory transfer is rejected
because the ownership layer has no capacity.

Save/load now preserves both:

```text
world.items              floor items still available to pick up
player.inventory.items   items already owned by the player
player.inventory.capacity
```

## 43. Equipment Slots

Some inventory items are equippable:

```text
Item
  id
  tile
  optional EquipmentSlot
  combat modifiers
```

Equipment is explicit player state:

```text
Inventory
  items[]
  capacity
  equipment.weapon
  equipment.armor
  equipment.accessory
```

`EquipmentService` owns the transfer rules:

```text
equip(itemId)
  -> item must exist in inventory
  -> item must declare an equipment slot
  -> remove item from inventory
  -> move previous equipped item back to inventory
  -> place new item into its slot

unequip(slot)
  -> slot must contain an item
  -> inventory must have capacity
  -> move equipped item back to inventory
  -> clear slot
```

This keeps equipment separate from pickup. Pickup only answers “does the player
own the item now?” Equipment answers “where is the owned item applied?” That
separation matters because later UI, hotkeys, auto-equip, and class rules can
share the same equipment service instead of rewriting slot behavior.

## 44. Effective Combat Stats

Equipment does not rewrite the player's base combat stats. Instead, combat asks
for a derived view when an attack is resolved:

```text
Player
  base CombatStats
  Inventory
    equipped items
      attack modifier
      defense modifier

EquipmentStatsService
  base stats + equipped modifiers
  -> effective CombatStats
```

Only equipped items count. Items sitting in the inventory bag are owned by the
player, but they are not applied to combat.

That split gives the engine three separate questions:

```text
InventoryService
  does the player own the item?

EquipmentService
  is the item in an active slot?

EquipmentStatsService
  what numbers should combat use right now?
```

The combat system uses the derived stats at the moment of resolution:

```text
player attacks enemy
  player base attack + equipped attack modifiers

enemy attacks player
  player base defense + equipped defense modifiers
```

Damage still writes back to `player.combatStats.hitPoints`. Attack and defense
are derived reads; current health remains durable player state.

## 45. Inventory Commands

UI, controller bindings, replay tools, and debug menus should not mutate
equipment slots directly. They should issue semantic inventory commands:

```text
InventoryCommand
  EquipItem(itemId)
  UnequipSlot(slot)
```

The dispatcher owns command validation and delegates rule checks:

```text
InventoryCommandDispatcher
  -> valid command payload?
  -> EquipmentService
  -> InventoryCommandResult
```

That gives the inventory side the same shape movement already has:

```text
raw input / UI / replay
  -> semantic command
  -> dispatcher
  -> service rules
  -> result
```

The important split is:

```text
InventoryCommand
  what the player or UI requested

EquipmentService
  whether that request is legal for the current inventory state
```

So an input binding can ask for “equip item 930” without knowing whether the
item exists, whether it is equippable, whether another item must be swapped, or
whether unequipping has enough bag capacity.

## 46. Runtime Inventory Command Sources

Inventory commands now have a runtime source boundary:

```text
menu / controller shortcut / replay / debug
  -> InventoryCommandSource
  -> InventoryCommand
  -> InventoryCommandDispatcher
  -> EquipmentService
  -> InventoryCommandResult
```

The game loop drains inventory command sources only when a world exists:

```text
for each frame
  -> drain SessionCommandSource[]
  -> if world exists, drain InventoryCommandSource[]
  -> if world exists, drain MovementCommandSource[]
  -> GameSession::update
```

That is the same app-edge pattern as movement commands. The source does not
equip an item directly. It supplies intent, and the dispatcher/service path owns
validation and mutation.

Preserving commands when there is no active world matters. Menu/controller input
can arrive before a new game or load completes; the app loop should not silently
consume inventory requests when no player inventory exists.

## 47. Inventory Events

Inventory commands now produce observable events:

```text
InventoryCommand
  -> InventoryCommandDispatcher
  -> InventoryCommandResult
  -> InventoryCommandEventEmitter
  -> InventoryEvent
```

The event stream reports:

```text
Equipped
Unequipped
Rejected
```

Each event carries the command type, whether the command applied or rejected,
the equipment service result, the item id, and the slot. That makes the event
useful to UI and logs without giving those systems ownership of inventory
mutation.

This mirrors the rest of the engine:

```text
movement command -> movement event
combat action    -> combat event
session command  -> session event
inventory command -> inventory event
```

The command result still returns to the caller immediately. The event is the
recordable fact for observers. That distinction is what keeps UI, tests, debug
logs, and later replay code from reaching into inventory internals.

`InventoryCommandEventEmitter` owns that translation. The dispatcher uses it
after normal equipment service results, and runtime source draining uses the
same emitter when an inventory command must be rejected because the selected
player does not exist. That keeps the observer event shape consistent across
normal command dispatch and app-level availability failures.

## 48. Inventory Command Codec And Replay

Inventory commands now have a stable byte boundary:

```text
InventoryCommand
  -> InventoryCommandPacket
  -> InventoryCommandPacketValidator
  -> InventoryCommandPacketByteCodec
  -> bytes
  -> InventoryCommandPacketByteCodec
  -> InventoryCommandPacket
  -> InventoryCommandPacketValidator
  -> InventoryCommand
```

The codec validates command shape:

```text
EquipItem   must carry an item id
UnequipSlot must carry an equipment slot
unexpected payload fields are rejected
invalid enum values are rejected
wrong byte sizes are rejected
```

Command logs store semantic inventory requests:

```text
InventoryCommandLog
  -> InventoryCommandLogCodec
  -> InventoryCommandLogFrameCodec
  -> magic
  -> version
  -> InventoryCommandByteStream
  -> InventoryCommandPacketListCodec
  -> counted InventoryCommandPacket[]
  -> InventoryCommandLogChecksum
  -> checksum
```

`InventoryCommandByteStream` owns little-endian primitive reads and writes for
the inventory replay byte stack. Packet bytes, packet-list counts, frame
metadata, and trailing checksums all use the same primitive boundary instead of
each codec carrying private read/write helpers.

Replay uses the same dispatcher as live inventory commands:

```text
InventoryCommandLog
  -> InventoryCommandReplayer
  -> InventoryCommandDispatcher
  -> EquipmentService
  -> InventoryEvent
```

That is the important boundary: replay does not equip items by editing slots.
It replays the same semantic requests that a menu, controller shortcut, or debug
tool would issue. If the current inventory state rejects the command, replay
observes the same rejection path and event stream as live runtime input.

## 49. Inventory Command File Store

The inventory command log codec defines valid bytes. The file store defines how
those bytes cross the filesystem:

```text
InventoryCommandLog
  -> InventoryCommandLogCodec
  -> InventoryCommandLogFileStore
  -> ByteFileStore
  -> temp file
  -> rename into place
  -> file bytes
  -> ByteFileStore
  -> InventoryCommandLogFileStore
  -> InventoryCommandLogCodec
  -> InventoryCommandLog
```

The store rejects missing or corrupt files before any replay happens:

```text
missing file -> no log
bad bytes    -> no log
valid file   -> InventoryCommandLog
```

That keeps three jobs separate:

```text
InventoryCommandLogCodec      validates replay-file structure
ByteFileStore                 handles raw binary file IO and temp-file rename
InventoryCommandLogFileStore  maps raw bytes to missing/corrupt inventory logs
InventoryCommandReplayer      applies loaded commands through the dispatcher
```

`inventory_command_persistence_tests` owns this durable inventory automation
boundary so the broad movement suite can stay focused on live inventory command
semantics and world interaction.

This is the same pattern as session boot scripts. Inventory automation can now
live on disk while still replaying semantic commands through the normal
equipment rules and inventory event stream.

## 50. Inventory Script Runner

The script runner is the use-case layer over file storage and replay:

```text
script file
  -> InventoryCommandLogFileStore
  -> InventoryCommandLog
  -> InventoryCommandReplayer
  -> InventoryCommandDispatcher
  -> EquipmentService
  -> InventoryEvent
```

It reports two different outcomes:

```text
LoadFailed  the script file was missing or invalid, so no command dispatched
Completed   the script loaded and every command produced an Applied/Rejected result
```

That distinction matters. A corrupt inventory script is a file/format problem.
A valid script that asks to equip a non-equippable item is an inventory rule
problem. UI, tests, and automation tools should be able to see those failures
at different layers.
