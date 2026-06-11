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
