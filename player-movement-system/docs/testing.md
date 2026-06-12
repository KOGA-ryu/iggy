# Testing Shape

The project started with one large `movement_tests.cpp` file because that made
early exploration simple. As the engine layers become clearer, focused test
executables should take over one cluster at a time.

## Current Pattern

`movement_tests` remains the broad regression suite.

`artifact_output_tests` owns the artifact output boundary: output settings,
output flag state, output steps, output planning, one-request artifact
execution, artifact output service application, and output finalizer result
integration.

`artifact_text_tests` owns shared artifact text helpers: policy lines, frame
headers, frame index markers, and run summary lines used by traces and
manifests.

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

`session_state_tests` owns the session state boundary: new-game world creation,
slot load/save helpers, session frame updates, mode policy/changing, paused
command preservation, and failed-load behavior that preserves the active world.

`save_snapshot_persistence_tests` owns durable world persistence: snapshot
writer/reader behavior, snapshot byte/schema/entity/player/enemy codecs,
checksums and frame validation, snapshot file-store behavior, save-game service
load/save workflow, and save-slot metadata/load behavior.

`session_lifecycle_command_tests` owns semantic session lifecycle commands:
dispatch/applier results, lifecycle event emission, replay, command packet
validation, command log framing/checksums/file-store behavior, and lifecycle
script run results.

`frame_source_tests` owns the per-frame source orchestration steps: session
input/lifecycle routing, inventory script-before-command order, movement
script-before-command order, and the combined pre-simulation source phase.

`frame_simulation_tests` owns the simulation-phase reporting boundary: frame
policy resolution/recording, session advancement, simulation event recording,
and lifecycle/inventory event deltas attached when a frame completes.

`frame_lifecycle_tests` owns the outer frame/run lifecycle: one-frame source
plus simulation sequencing, bounded frame-loop repetition, and run-executor
setup gates before finalization.

`run_result_policy_tests` owns the completed-run result policy: final mode
capture, setup/output failure classification, run failure composition, and exit
code mapping.

`game_loop_output_tests` owns the app-shell output wiring: configured run trace
and debug bundle paths flowing through the assembled `GameLoop`, including
startup-failure output and process failure on requested output write failure.

`game_loop_command_source_tests` owns assembled-loop command source wiring:
session command sources, startup-before-runtime command ordering, inventory
command sources, movement command sources, and active-world/player gating.

`game_loop_script_source_tests` owns assembled-loop script source wiring:
runtime movement script sources, runtime inventory script sources, load-failure
reporting, and active-world/player gates that preserve queued script paths.

`game_loop_raw_input_tests` owns assembled-loop raw input wiring: raw hotkeys,
raw pointer movement, world target lookup, pickup interactions, full-inventory
pickup rejection, and blocked movement input reports through `GameLoop`.

`game_loop_frame_report_tests` owns assembled-loop frame report wiring: source
results, policy, simulation events, event deltas, and run-summary mirrors for a
completed frame.

`runtime_frame_trace_tests` owns frame trace presentation: readable trace lines,
runtime source/lifecycle/simulation section order, movement input block
summaries, and result/event text helpers used by traces and debug manifests.

`runtime_debug_manifest_tests` owns debug manifest presentation: bundle index
lines, artifact path lines, setup attempt lines, run/setup/runtime script
sections, and full `RuntimeDebugManifest` summaries without writing files.

`runtime_trace_persistence_tests` owns readable trace persistence: trace
file-store save/load behavior, full run trace formatting through
`RuntimeTraceService`, and exact persisted run trace contents.

`file_store_tests` owns generic filesystem stores: byte/text save-load
behavior, temp-file cleanup, missing-file handling, and unwritable-path
failures used by higher persistence layers.

`inventory_command_persistence_tests` owns durable inventory command
automation: semantic log replay, log codec/checksum/frame validation,
log-file load/save behavior, and inventory script runner outcomes.

`runtime_debug_artifact_tests` owns debug artifact bundle writing: stable bundle
paths, bundle result flags, root preparation, trace/manifest write steps,
writer failure reporting, and end-to-end debug bundle save behavior.

`runtime_setup_run_tests` owns setup and run recording lifecycle: configured
setup defaults, setup script frame gates, setup result application, source
result report recorders, run summary defaults, and frame report aggregation.

`runtime_setup_script_tests` owns assembled setup script behavior: startup
script intake, configured startup/inventory/movement setup order, setup script
load failures, active-player gating, and the distinction between setup scripts
and runtime script sources.

`runtime_input_hotkey_tests` owns app-edge input settings and hotkey routing:
input defaults, session-derived input context, route result naming, lifecycle
hotkeys, stop hotkeys, and blocked stop-command reporting.

`runtime_source_intake_tests` owns the direct runtime source/intake adapters:
queued source drain-once behavior, source settings defaults, session command
intake, and movement script intake plus batch order.

`runtime_inventory_source_intake_tests` owns inventory-specific runtime source
and intake adapters: queued inventory command/script drains, active-player
command dispatch, script intake, and ordered inventory script batches.

```text
focused production boundary
  -> focused test executable
  -> same player_movement_system library
```

## Split Rule

Split a test cluster when it has a clear subsystem boundary and does not need a
large shared helper migration.

Good first candidates:

- movement script source/intake seams

Avoid splitting by line count alone. A smaller file is useful only when the new
test target has a clear reason to exist.

## Lesson

One large test file is acceptable while the system is forming. Once boundaries
stabilize, move tests by subsystem. That keeps build failures easier to scan
without turning test organization into a risky refactor.
