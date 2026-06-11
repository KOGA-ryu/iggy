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
still legal, commits the player position, and reports whether arrival should
hand control to the action executor.

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
MovementEvent -> EventRecorder
```

That means the same input, network, replay, and test paths all exercise the same
movement command pipeline.

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
  -> EnemyPursuitStepper
  -> pursuit step budget
  -> attack range
  -> EnemyAttackRunner
  -> attack windup
  -> attack recovery
```

The goal is not simply to reach the player. The goal is to stay inside a fair
reaction window: readable enough to answer, fast enough to matter.
`EnemyPursuitStepper` owns that chase budget and stops when the enemy reaches
attack range, leaving the next frame to start windup through `EnemyAttackRunner`.

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

The tick layer is where the separate systems become one frame:

```text
CommandQueue
  -> SimulationCommandDrainer
  -> CommandDispatcher
  -> PlayerController
  -> SimulationPlayerUpdater
  -> PlayerMovement
  -> ActionExecutor
  -> SimulationEnemyUpdater
  -> EnemyMovement
  -> CombatSystem
```

The order matters. Commands are drained first so fresh input can affect this
frame. `SimulationPlayerUpdater` then owns the player-side actor wiring:
`PlayerMovement` plus the `ActionExecutor` that resolves ready destination
actions. Player movement updates before enemy movement so enemies respond to the
latest committed player position. `SimulationEnemyUpdater` owns the enemy-side
actor wiring and target guard before calling `EnemyMovement`. Actions and combat
consequences happen inside those actor updates, but still publish events instead
of directly owning UI, audio, VFX, or networking.

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
sinks, and returns the collected events. That leaves `SimulationFrameRunner` to
coordinate time-step building and post-tick finalization.

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
  -> file
```

Loading runs the reverse direction:

```text
file
  -> SnapshotFileStore
  -> SnapshotCodec validation
  -> SimulationSnapshot
  -> SnapshotReader
  -> SimulationWorld
```

The file store does not know what a player, enemy, or combatant means. It only
knows how to write codec bytes and refuse unreadable or corrupt data. That keeps
save format rules separate from filesystem failure rules.

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

SessionModePolicy maps session mode to frame policy:

```text
Gameplay  -> commands and actors advance
Paused    -> commands and actors stop
Inventory -> commands and actors stop
Empty     -> no active world updates
```

Failed loads do not destroy the active world. Successful new/load operations
reset transient clock state, but preserve event sinks. That distinction keeps
durable state, frame feel, and external observers from bleeding into each other.

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
  -> temp file
  -> rename into place
  -> file bytes
  -> SessionCommandLogCodec
  -> SessionCommandLog
```

This keeps three jobs separate:

```text
SessionCommandLogCodec      validates replay-file structure
SessionCommandLogFileStore  handles file IO and missing/corrupt files
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

## 34. Game Loop Shell

The app layer can now run a minimal bounded loop:

```text
GameLoopSettings
  -> RuntimeSetupSettings
  -> optional startup script
  -> SessionScriptRunner
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
startup run   -> SessionCommandDispatcher
inventory file -> InventoryCommandLogFileStore
inventory run -> InventoryCommandDispatcher
frame update  -> GameSession::update
events        -> SessionEventRecorder / InventoryEventRecorder
```

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
```

Those scripts run before frame updates. The matching `RuntimeSetupResult`
captures whether each configured script ran and what result it produced.

`RuntimeSourceSettings` groups the live input/source streams:

```text
GameLoopSettings::sources
  -> RawInputSource[]
  -> SessionCommandSource[]
  -> InventoryScriptSource[]
  -> InventoryCommandSource[]
  -> MovementCommandSource[]
```

That keeps setup scripts, output artifacts, and runtime sources from becoming
one flat settings bag. Raw input sources are routed by `RuntimeInputSourceRouter`;
command and script sources are delegated to `RuntimeSourceDrainer`.

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
runtime command sources drain
frame update ticks
```

If the inventory script file cannot load, the loop stops before ticking frames.
If the script loads but an inventory command rejects, the loop still reports a
completed inventory script with rejected command results.

`RuntimeSetupResult` keeps those setup facts together:

```text
GameLoopResult::setup
  -> startup script attempted/result
  -> configured inventory script attempted/result
```

`RuntimeSetupRunner` owns the configured setup phase:

```text
RuntimeSetupSettings
  -> RuntimeSetupRunner
  -> RuntimeSetupRunResult
  -> setup result
  -> setup inventory command results
  -> frames allowed?
```

Startup script load failure stops before configured inventory setup. Configured
inventory setup failure also stops before frames. A loadable inventory script
with rejected commands still allows frames, because the file and setup pipeline
worked and the rejection is command-level data.

That separates one-time setup automation from per-frame runtime sources. A
failed setup script can stop the loop before frames begin, while runtime script
source failures are reported per frame and the loop keeps ticking.

`RuntimeExitCodePolicy` translates completed run facts into a process-style
exit code:

```text
GameLoopResult
  -> RuntimeExitCodePolicy
  -> 0 or 1
```

Setup load failures and requested artifact write failures return failure.
Command-level rejections inside a loadable setup script remain command results,
so they do not automatically make the process fail.

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
  -> drain movement command sources
```

`RuntimeSourceStream` owns the source-list mechanics that are common across
semantic source types: preserve order, skip missing source slots, drain each
source once, and return the drained items as a flat list. That lets
`RuntimeSourceDrainer` focus on what drained items mean: dispatch session
commands, run inventory scripts, dispatch inventory commands, or queue movement
commands into the active world.

`RuntimeFrameRunner` owns the one-frame order around that drainer:

```text
RuntimeFrameRunner
  -> RuntimeInputSourceRouter routes raw input sources
  -> drain session command sources
  -> drain inventory script sources
  -> drain inventory command sources
  -> drain movement command sources
  -> GameSession::update
  -> RuntimeRunRecorder
```

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
  -> if frames allowed, RuntimeFrameLoopRunner
  -> RuntimeRunFinalizer
```

`GameLoop` still owns collaborator assembly and event/result access.
`RuntimeRunExecutor` owns lifecycle order, `RuntimeFrameLoopRunner` owns how
many bounded frames run, `RuntimeFrameRunner` owns one frame's mechanics,
`RuntimeInputSourceRouter` owns raw source routing for the frame, and
`RuntimeSourceDrainer` owns the repeated semantic source mechanics plus the
active-world checks needed before inventory and movement sources can safely
mutate state.

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
  -> RuntimeTraceService
  -> RuntimeDebugArtifactBundle
```

`RuntimeRunFinalizer` records the final session mode, then delegates configured
artifact writes to `RuntimeOutputFinalizer`. That keeps `RuntimeRunExecutor`
focused on lifecycle timing. Output finalization owns the app artifact policy
and exposes the same success/failure flags on
`GameLoopResult::output`.

`GameLoopSettings::output.runTracePath` lets the app shell persist a full run
trace after the loop exits:

```text
GameLoopSettings::output.runTracePath
  -> GameLoopResult
  -> RuntimeTraceService
  -> run.trace
```

This save happens through the same finalization path whether the loop finishes
its frames or stops early during startup/inventory setup. `GameLoopResult`
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
  -> manifest.txt
  -> run.trace
```

`GameLoopResult::output` reports `debugBundleSaveAttempted` and
`debugBundleSaved`. `GameLoop::run` also returns failure when a requested bundle
cannot be written. The loop still does not know how a manifest is formatted or
how a trace is serialized; it only decides that configured debug artifacts
should be finalized after the run.

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

`RuntimeFrameTrace` exports that structured report into deterministic text:

```text
frame rawInput=0 sessionResults=0 inventoryScripts=1 ...
inventoryResult[0] type=Applied command=EquipItem ...
inventoryEvent[0] type=Equipped ...
movementEvent[0] type=CommandAccepted ...
```

The trace formatter is intentionally downstream of the report. It does not
drive gameplay or mutate state. It only turns already-recorded runtime facts
into readable lines.

`RuntimeFrameTraceFileStore` persists those readable lines:

```text
RuntimeFrameReport
  -> RuntimeFrameTrace
  -> RuntimeFrameTraceFileStore
  -> frame.trace
```

This store is plain text on purpose. Unlike save files or command logs, trace
files are for humans and tooling to inspect what happened during a run, not for
restoring gameplay state.

`RuntimeTraceService` is the use-case layer for full run traces:

```text
GameLoopResult
  -> frameReports[]
  -> RuntimeFrameTrace
  -> RuntimeFrameTraceFileStore
  -> run.trace
```

It adds a run-level summary before the per-frame lines. That gives a caller one
method for “save the trace for this run” while keeping formatting and filesystem
behavior testable as separate pieces.

`RuntimeDebugArtifactBundle` is the next app-layer wrapper around that trace:

```text
GameLoopResult
  -> RuntimeDebugArtifactBundle
  -> manifest.txt
  -> run.trace
```

The bundle owns directory preparation and manifest writing. The trace still
goes through `RuntimeTraceService`. That split keeps the replay/debug artifact
shape outside gameplay code while leaving a clear place to add future files,
such as replay command logs or session metadata.

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

The app edge now has a small adapter from raw input to semantic command sources:

```text
RawInputEvent
  -> RuntimeInputRouter
  -> RuntimeSessionInputRouter
  -> RuntimeMovementInputRouter
  -> RuntimeTargetInputRouter
  -> SessionCommandSource or MovementCommandSource
  -> GameLoop
  -> dispatcher / command queue
```

Movement input still uses the lower-level movement path:

```text
RawInputEvent
  -> InputFocus
  -> InputMapper
  -> PlayerIntent
  -> PlayerActionGate
  -> IntentCommandBuilder
  -> MovementCommandSource
```

When a target resolver is attached, clicks can use the interaction path instead:

```text
RawInputEvent
  -> screenToTile
  -> TargetResolver
  -> InteractionIntentBuilder
  -> InteractionCommandBuilder
  -> MovementCommandSource
```

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
  -> InventoryCommandLogFrameCodec
  -> InventoryCommandLogCodec
  -> magic
  -> version
  -> command count
  -> packet[]
  -> InventoryCommandLogChecksum
  -> checksum
```

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
  -> temp file
  -> rename into place
  -> file bytes
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
InventoryCommandLogFileStore  handles file IO and missing/corrupt files
InventoryCommandReplayer      applies loaded commands through the dispatcher
```

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
