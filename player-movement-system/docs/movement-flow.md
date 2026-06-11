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
Stop   -> PlayerController::stop
```

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

```text
PlayerMovement consumes path
  -> PlayerMoveState::Acting
  -> ActionExecutor
  -> ActionRules
  -> AnimationLock
```

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
  -> pursuit step budget
  -> attack range
  -> attack windup
  -> attack recovery
```

The goal is not simply to reach the player. The goal is to stay inside a fair
reaction window: readable enough to answer, fast enough to matter.

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
  -> CommandDispatcher
  -> PlayerController
  -> PlayerMovement
  -> ActionExecutor
  -> EnemyMovement
  -> CombatSystem
```

The order matters. Commands are drained first so fresh input can affect this
frame. Player movement updates before enemy movement so enemies respond to the
latest committed player position. Actions and combat consequences happen inside
those movement updates, but still publish events instead of directly owning UI,
audio, VFX, or networking.

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
  -> SimulationClock::step
  -> SimulationTick
  -> SimulationFrameEvents
  -> EffectRouter
  -> EffectApplier
  -> frame output for presentation
```

This gives each frame a clean consequence phase:

```text
state changes happen during tick
facts are collected as events
effect requests are derived after the tick
approved simulation-facing effects are applied
presentation can consume the remaining frame output
```

The runner temporarily redirects movement and combat events into
`SimulationFrameEvents`, forwards them to any existing sinks, then restores the
world sinks after the tick. That keeps event capture local to a frame without
stealing events from tests, debug tools, UI, or telemetry.
