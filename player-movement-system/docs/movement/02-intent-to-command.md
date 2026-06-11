# 02. Intent To Command

This layer answers one question: once input has gameplay meaning, how does it
become something the simulation can consume?

The important design choice is that intent builders and runtime queues have
different jobs. Builders decide what command should exist. The runtime queue
step stores that command and reports what happened to the input route.

```text
PlayerIntent or InteractionIntent
  -> IntentCommandBuilder or InteractionCommandBuilder
  -> MovementCommand
  -> RuntimeMovementCommandQueueStep
  -> MovementCommandSource
  -> GameLoop
  -> SimulationWorld::commandQueue
```

## What Each Boundary Owns

`IntentCommandBuilder` maps generic player intent to movement commands. Walk
intent becomes `WalkTo`; stop intent becomes `Stop`; impossible intent returns
no command.

`InteractionCommandBuilder` maps target-aware intent to movement commands.
Empty tiles walk, enemies attack, items pick up, NPCs talk, and objects
interact. Stand-ground can turn an attack into `StandAndAct`.

`RuntimeMovementCommandQueueStep` owns the app-edge handoff after a command is
built. It enqueues concrete commands into a `QueuedMovementCommandSource` and
returns a route result that says a movement command was queued. Missing optional
commands stay unhandled.

`MovementCommandSource` is the runtime-facing queue. It lets input, replay,
debug tools, and later network code all feed semantic movement commands into
the same simulation path.

## Why The Queue Step Exists

Input routes should not each invent their own answer for "what happened after a
command was built?" Stop, generic walking, and target interaction should all
agree on the route result vocabulary:

```text
no command       -> unhandled
queued command   -> handled + queuedMovementCommand
blocked movement -> movementBlockReason
```

That makes frame reports reliable. A frame can count queued movement commands
without knowing whether they came from a stop key, a mouse click, a touch tap,
or a target interaction.

## Lesson

The engine gets easier to reason about when every layer changes vocabulary only
once. Raw input becomes intent. Intent becomes command. Runtime queueing turns a
command into a frame-visible route result. The simulation only sees commands.
