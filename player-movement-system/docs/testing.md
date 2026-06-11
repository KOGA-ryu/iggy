# Testing Shape

The project started with one large `movement_tests.cpp` file because that made
early exploration simple. As the engine layers become clearer, focused test
executables should take over one cluster at a time.

## Current Pattern

`movement_tests` remains the broad regression suite.

`artifact_output_tests` owns the first extracted cluster: artifact output
settings, output flag state, output steps, output planning, and one-request
artifact execution.

`artifact_text_tests` owns pure trace and manifest formatter contracts. These
tests exercise strings produced from runtime reports without touching artifact
writers, filesystem stores, or frame execution.

`input_routing_tests` owns controls-edge checks: pointer input, focus
resolution, movement context building, blocked pointer reporting, and
target-aware interaction routing before commands reach simulation code.

`input_drain_tests` owns raw input source draining and drain reporting. These
tests verify that queued raw events are consumed once, routed through the
current session context, and summarized into frame/run input reports.

`command_drain_tests` owns the movement command handoff from runtime command
sources into `SimulationWorld::commandQueue`, then through the simulation drain
loop and queued-count reporting.

`source_drain_tests` owns the shared runtime source mechanics: source settings
mapping, active world/player lookup, nullable source-list draining, and source
drainer ordering before frame orchestration records the results.

`frame_source_tests` owns the per-frame source orchestration steps: session
input/lifecycle routing, inventory script-before-command order, movement
script-before-command order, and the combined pre-simulation source phase.

`frame_simulation_tests` owns the simulation-phase reporting boundary: frame
policy resolution/recording, session advancement, simulation event recording,
and lifecycle/inventory event deltas attached when a frame completes.

`frame_lifecycle_tests` owns the outer frame/run lifecycle: one-frame source
plus simulation sequencing, bounded frame-loop repetition, and run-executor
setup gates before finalization.

```text
focused production boundary
  -> focused test executable
  -> same player_movement_system library
```

## Split Rule

Split a test cluster when it has a clear subsystem boundary and does not need a
large shared helper migration.

Good first candidates:

- run finalization, exit-code, and failure-policy reporting

Avoid splitting by line count alone. A smaller file is useful only when the new
test target has a clear reason to exist.

## Lesson

One large test file is acceptable while the system is forming. Once boundaries
stabilize, move tests by subsystem. That keeps build failures easier to scan
without turning test organization into a risky refactor.
