# 03. Command To Simulation

This layer answers one question: once commands are queued, how do they enter the
world update?

The important design choice is that runtime systems can queue commands, but
simulation owns when those commands are consumed. That keeps input, replay, and
debug tools from moving actors directly.

```text
MovementCommandSource
  -> RuntimeMovementCommandIntake
  -> SimulationWorld::commandQueue
  -> SimulationCommandDrainer
  -> SimulationCommandQueueDrainStep
  -> CommandDispatcher
  -> PlayerController
```

## What Each Boundary Owns

`RuntimeMovementCommandIntake` moves drained runtime commands into the active
world queue. It preserves command order and reports how many commands were
queued.

`SimulationCommandDrainer` wires simulation-world services for command intake:
players, map, collision, pathfinding, and movement event recording.

`SimulationCommandQueueDrainStep` owns the FIFO drain loop. It pops queued
movement commands, sends each one through `CommandDispatcher`, and reports how
many commands left the queue.

`CommandDispatcher` validates and applies a single command. Accepted commands
go through `PlayerController`; rejected commands still emit command events.

## Test Boundary

`command_drain_tests` covers this handoff without running a full game loop. It
asserts that runtime command intake preserves order in the world queue, that the
simulation drain step empties the queue through `CommandDispatcher`, and that
queued command counts are recorded for frame and run reports.

`source_drain_tests` covers the source side before commands reach this boundary:
runtime source lists are mapped once, nullable slots are skipped, session
commands can create the active world before movement commands are drained, and
movement scripts feed the same simulation queue path.

## Why Simulation Owns Consumption

Controls should feel immediate, but commands should still wait for the
simulation phase. That gives every command source the same rules:

```text
input command
replay command
debug command
future network command
  -> same world command queue
  -> same dispatcher
  -> same player controller
```

The queue drain count is small but useful. It lets frame reports distinguish
"nothing was queued" from "commands were queued and consumed" without teaching
the app layer how movement was applied.

## Lesson

Do not let input mutate the world. Input creates commands. Runtime queues them.
Simulation consumes them at the frame boundary. That is the line that keeps
movement deterministic enough for replay, debugging, and later networking.
