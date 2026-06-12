# Movement Tests

`movement_tests` remains the broad regression executable. Focused clusters can
move into smaller executables as subsystem boundaries stabilize.
`artifact_output_tests` covers artifact output settings, result flags, output
steps, planning, one-request execution, artifact service application, and
output finalizer integration.
`artifact_text_tests` covers shared trace/manifest text helpers for policies,
headers, frame indexes, and run summaries.
`input_routing_tests` covers the controls-edge slice from pointer input through
focus/context checks into movement commands, target interaction commands, or
block reports.
`input_drain_tests` covers raw input source draining and the frame/run report
facts produced by routed or blocked input.
`command_drain_tests` covers movement command source draining, runtime world
queue intake, simulation command draining, and queued-count reporting.
`source_drain_tests` covers shared runtime source settings, active context
lookup, source stream draining, and source-drainer ordering.
`frame_source_tests` covers per-frame session, inventory, and movement source
step orchestration before simulation runs.
`frame_simulation_tests` covers simulation phase policy capture, session
advancement, frame event reporting, and lifecycle/inventory event deltas.
`frame_lifecycle_tests` covers one-frame execution, bounded frame-loop
repetition, and run-executor setup gating before finalization.
`run_result_policy_tests` covers final mode capture, setup/output failure
classification, run failure composition, and process exit-code mapping.
`game_loop_output_tests` covers configured run trace and debug bundle paths
flowing through the fully assembled `GameLoop`.
`game_loop_command_source_tests` covers session, inventory, and movement command
sources flowing through the fully assembled `GameLoop`.
`game_loop_script_source_tests` covers movement and inventory script sources
flowing through the fully assembled `GameLoop`.
`game_loop_raw_input_tests` covers raw hotkeys and pointer input flowing through
the fully assembled `GameLoop`.
`game_loop_frame_report_tests` covers frame reports produced by the fully
assembled `GameLoop`.
`runtime_frame_trace_tests` covers readable frame trace lines, trace section
ordering, movement input block summaries, and result/event text helpers.
`runtime_debug_manifest_tests` covers readable debug manifest lines and full
manifest summaries without exercising artifact writers or filesystem stores.
`runtime_trace_persistence_tests` covers trace file-store behavior and
`RuntimeTraceService` full-run trace save/load contracts.

Good first tests:

- click on walkable tile creates `WalkTo`
- click while inventory owns focus creates no command
- click while text entry is active creates no command
- PlayerActionGate reports movement block reasons
- `WalkTo` on blocked tile is rejected
- path is consumed one step at a time
- `Stop` clears the current path
- stun state rejects movement
- animation lock rejects movement
- animation cancel window allows movement again
- stand-ground rejects `WalkTo` but keeps player facing/intent available
- diagonal movement through a blocked corner is rejected
- diagonal path cost discourages unnecessary zig-zagging
- `future` updates when a step is committed
- empty tile target creates `WalkTo`
- enemy target creates `MoveThenAct(Attack)`
- item target creates `MoveThenAct(Pickup)`
- NPC target creates `MoveThenAct(Talk)`
- object target creates `MoveThenAct(Interact)`
- stand-ground plus attack target creates `StandAndAct(Attack)`
- DestinationActionBuilder maps interaction intents into action payloads and ranges
- destination action survives until path is consumed
- PlayerPathPlanner starts pathing and emits path events
- MovementCommandValidator rejects action commands without action payloads
- CommandDispatcher emits rejected events for invalid action commands
- action executor rejects invalid targets
- action executor waits when target is out of range
- action executor applies attack animation commitment
- action executor clears destination action after execution
- MoveThenAct emits CommandAccepted -> PathStarted -> StepCommitted -> DestinationActionReady -> AnimationLocked -> ActionExecuted
- ActorStepCommitter updates previous/tile/future/precise for committed steps
- PlayerPathStepper commits one path step and reports destination action readiness
- PlayerAnimationLockGate blocks movement until the cancel window and emits AnimationUnlocked
- PlayerActionRunner executes a ready destination action through ActionExecutor
- CommandLog replay produces the same movement/action event sequence
- CommandReplayer reports accepted and rejected movement command dispatches
- CommandReplayReport summarizes accepted and rejected movement replay results
- MovementCodec round-trips MoveThenAct command packets
- CommandLogCodec round-trips and replays versioned movement command logs
- CommandLogCodec rejects invalid movement log bytes
- CommandLogChecksum appends and validates movement log checksums
- CommandPacketListCodec frames counted movement packet lists and rejects invalid sizes
- CommandLogFrameCodec frames movement packet bytes and rejects invalid frame metadata
- CommandLogFileStore saves, loads, replays, and rejects bad movement command log files
- MovementScriptRunner loads and runs saved movement scripts through the dispatcher
- MovementScriptRunner distinguishes movement script load failure from command rejection
- RuntimeMovementScriptIntake runs movement scripts against an active runtime world
- RuntimeMovementScriptBatchRunner preserves ordered movement script path results
- MovementScriptSource queues and drains movement replay script paths
- EnemyPursuitStepPlanner chooses the next chase tile toward a target
- EnemyPursuitStepGate rejects map-blocked and collision-blocked pursuit tiles
- EnemyAttackRange applies tuned Chebyshev attack range
- EnemyAttackEntryPolicy decides whether an enemy can start attack windup
- EnemyPursuitBudget stops pursuit at maxStepsPerTick
- enemy pursuit obeys maxStepsPerTick
- EnemyPursuitStepper stops pursuit once attack range is reached
- EnemyPursuitResult reports budget, blocked, already-at-target, and attack-range stops
- EnemyPursuitEventEmitter publishes pursuit stop facts for frame traces
- enemy in range enters attack windup
- enemy attack windup transitions into recovery
- EnemyAttackRunner owns enemy windup, recovery, and restart timing
- EnemyAttackPhaseRunner advances windup/recovery timers and resets phase time
- EnemyAttackRestartPolicy decides whether recovery restarts windup or releases to pursuit
- EnemyAttackEventEmitter publishes windup/recovery transition facts for frame traces
- EnemyMovementReporter publishes enemy attack and pursuit reports through one boundary
- CombatResolver applies deterministic damage and defeat
- ActionExecutor attack can damage registered combat target
- CombatSystem emits Hit and Defeated events with damage and remaining HP
- Enemy attack windup resolves combat against player and emits CombatEvent
- SimulationCommandQueueDrainStep dispatches queued commands and reports drain count
- SimulationCommandDrainer dispatches queued movement commands into player controller state
- SimulationPlayerUpdater advances player movement and emits frame-visible movement events
- SimulationPlayerMovementRunner wires world services into PlayerMovement
- SimulationEnemyTargetSelector chooses the current player target or no target
- SimulationEnemyMovementRunner wires world services into EnemyMovement
- SimulationEnemyUpdater advances enemy movement against the current player target
- SimulationActorUpdater runs players before enemies and can policy-skip enemy updates
- SimulationTickPipeline drains commands before actors and can policy-skip command intake
- SimulationTick drains queued commands, updates movement, and resolves combat
- SimulationFramePolicyDescriber reports mode names, reasons, and exact frame gates
- SimulationFramePolicy can pause command draining and movement
- SimulationClock hit-stop freezes actor updates while preserving command intake
- SimulationClock time scale slows enemy attack windup
- EffectRouter maps movement events to feedback requests
- EffectRouter maps combat hits to damage, impact, and hit-stop requests
- EffectApplier applies hit-stop requests to SimulationClock
- SimulationTimeStepBuilder converts raw frame delta through optional clock rules
- SimulationEffectPipeline routes frame events and applies simulation-facing effects
- SimulationEffectFinalizer applies frame effect consequences through the pipeline
- SimulationFrameEventCapture collects frame events, forwards them, and restores sinks
- SimulationFrameFinalizer applies post-tick targets, pickups, and effects
- SimulationFrameTickRunner captures tick events and restores forwarded sinks
- SimulationTargetFinalizer synchronizes moved and defeated target state
- SimulationInventoryFinalizer applies accepted pickups and preserves rejected pickups
- SimulationFrameRunner collects events, routes effects, applies hit-stop, and preserves forwarding
- TargetRegistry resolves, removes, and falls back to empty tile targets
- TargetSynchronizer publishes current enemy targets without deleting objects
- TargetSynchronizer skips defeated combat registry enemies
- SimulationFrameRunner updates enemy target tiles after movement
- SimulationFrameRunner removes enemy targets after defeat events
- WorldEntityService spawns enemies across enemy, combat, and target registries
- WorldEntityService despawns enemies without removing unrelated world state
- WorldEntityService respawns duplicate enemy ids without stale registry entries
- WorldEntityService spawns and despawns item targets
- WorldEntityService respawns duplicate item ids without stale target entries
- InventoryService transfers executed pickups into player inventory
- InventoryService ignores invalid pickup events without consuming world items
- InventoryService rejects pickup transfer when inventory is full
- EquipmentService equips inventory items into matching slots
- EquipmentService swaps occupied equipment back into inventory
- EquipmentService rejects missing or non-equippable items
- EquipmentService unequips only when inventory has capacity
- EquipmentStatsService derives effective combat stats from equipped items only
- CombatSystem applies equipped attack and defense modifiers
- InventoryCommandDispatcher applies semantic equip and unequip commands
- InventoryCommandDispatcher preserves rejected equipment reasons
- InventoryCommandDispatcher emits inventory events for applied and rejected commands
- InventoryCommandEventEmitter maps command results into inventory events
- InventoryCommandSource queues and drains semantic inventory commands
- GameLoop dispatches inventory command sources only when a world is active
- GameLoop records inventory events from runtime inventory command sources
- InventoryCommandCodec round-trips valid commands and rejects invalid packets
- InventoryCommandPacketValidator rejects malformed inventory packet shapes
- InventoryCommandByteStream writes little-endian primitives and rejects short reads
- InventoryCommandPacketByteCodec round-trips and rejects inventory packet bytes
- InventoryCommandLog replays through InventoryCommandDispatcher
- InventoryCommandLogCodec round-trips versioned command logs and rejects corrupt bytes
- InventoryCommandLogChecksum appends and validates inventory log checksums
- InventoryCommandPacketListCodec frames counted packet lists and rejects invalid sizes
- InventoryCommandLogFrameCodec frames packet bytes and rejects invalid frame metadata
- ByteFileStore saves/loads binary bytes and cleans temp files
- InventoryCommandLogFileStore saves, loads, replays, and rejects bad files
- InventoryScriptRunner loads and runs saved inventory scripts through the dispatcher
- InventoryScriptRunner distinguishes script load failure from command rejection
- RuntimeInventoryScriptIntake runs inventory scripts against the selected runtime player
- RuntimeInventoryScriptBatchRunner preserves ordered inventory script path results
- InventoryScriptSource queues and drains inventory automation script paths
- GameLoop reports runtime inventory script source results without stopping frames
- SnapshotWriter and SnapshotReader restore durable player, enemy, and combat state
- SnapshotWriter and SnapshotReader restore player inventory state
- SnapshotWriter and SnapshotReader restore player equipment state
- SnapshotCodec preserves item combat modifiers
- SnapshotWriter and SnapshotReader restore durable item state
- SnapshotWriter and SnapshotReader restore clickable target registry state
- SnapshotCodec round-trips versioned snapshot bytes and rejects invalid or corrupted data
- SnapshotChecksum appends and validates snapshot byte checksums
- SnapshotByteStream writes little-endian primitives and rejects short reads
- SnapshotEntityCodec round-trips item/combatant fields and rejects invalid entity enums
- SnapshotPlayerCodec and SnapshotEnemyCodec round-trip durable actor state and reject invalid move states
- SnapshotVectorCodec frames counted vectors and rejects truncated vector data
- SnapshotSchemaCodec round-trips ordered snapshot sections and rejects incomplete/trailing payloads
- SnapshotFrameCodec frames payload bytes and rejects invalid snapshot frames
- SnapshotFileStore saves versioned bytes and rejects corrupt save files
- SaveGameService saves and loads SimulationWorld without replacing event sinks
- SaveSlotService lists slot metadata and distinguishes valid, corrupt, and empty slots
- SaveSlotService loads a selected slot into SimulationWorld without replacing event sinks
- NewGameWorldBuilder creates starting player world state while preserving event sinks
- SessionWorldSlotLoader replaces session world from a slot while preserving event sinks and failed-load state
- SessionWorldSlotSaver rejects empty sessions and saves active sessions
- SessionFrameUpdater applies session mode policy to frame updates
- GameSession starts a new game and advances frames
- GameSession paused mode preserves queued commands
- SessionModePolicy maps session modes to simulation modes and frame policy
- SessionModePolicy rejects activating an empty session through mode-only changes
- SessionModeChanger applies only allowed mode transitions
- GameSession save/load preserves event sinks and resets transient clock state
- GameSession failed load preserves the active world
- SessionCommandApplier maps lifecycle command outcomes to results and events
- SessionEventEmitter builds lifecycle events and ignores missing sinks
- SessionCommandDispatcher applies start, save, load, and mode commands
- SessionCommandDispatcher rejects invalid lifecycle commands
- SessionCommandDispatcher emits success and failure lifecycle events
- SessionCommandLog and SessionCommandReplayer replay lifecycle command sequences
- SessionCommandReplayer reports rejected lifecycle commands
- SessionCommandCodec round-trips lifecycle commands
- SessionCommandCodec rejects invalid lifecycle command packets
- SessionCommandPacketValidator rejects malformed lifecycle packet shapes
- SessionCommandByteStream writes little-endian primitives and rejects short reads
- SessionCommandPacketByteCodec round-trips and rejects lifecycle packet bytes
- SessionCommandLogCodec round-trips and replays lifecycle command logs
- SessionCommandLogCodec rejects invalid lifecycle log bytes
- SessionCommandLogChecksum appends and validates lifecycle log checksums
- SessionCommandPacketListCodec frames counted packet lists and rejects invalid sizes
- SessionCommandLogFrameCodec frames packet bytes and rejects invalid frame metadata
- SessionCommandLogFileStore saves, loads, and replays lifecycle command log files
- SessionCommandLogFileStore rejects corrupt and missing lifecycle command log files
- SessionScriptRunner loads and runs saved lifecycle scripts through the dispatcher
- SessionScriptRunner distinguishes script load failure from command rejection
- RuntimeStartupScriptIntake runs startup lifecycle scripts through setup dispatch
- GameLoop runs configured startup scripts and bounded frame updates
- GameLoop reports startup script load failure without ticking frames
- GameLoop runs configured inventory scripts after startup scripts
- GameLoop runs configured movement scripts after startup scripts
- GameLoop reports inventory script load failure or missing player before ticking frames
- GameLoop drains runtime inventory script sources before direct inventory command sources
- GameLoop drains runtime movement script sources before direct movement command sources
- RuntimeSourceDrainerSettingsBuilder maps loop sources and player id into drainer settings
- RuntimeSourceContext reports active world/player availability and optional receivers for source draining
- RuntimeSourceDrainer owns runtime source draining order
- RuntimeSourceStream drains nullable source lists once while preserving order
- RuntimeMovementCommandIntake queues drained runtime movement commands into the simulation world
- RuntimeInventoryCommandIntake dispatches drained runtime inventory commands or rejects them without a player
- RuntimeInventoryFrameSourceStep drains inventory scripts before direct inventory commands
- RuntimeMovementFrameSourceStep drains movement scripts before direct movement commands
- RuntimeSessionCommandIntake dispatches drained lifecycle commands in source order
- RuntimeSessionFrameSourceStep routes raw input before draining lifecycle command sources
- RuntimeLoopTypes keeps loop settings and results reusable outside GameLoop
- GameLoop builds RuntimeFrameReport entries for per-frame inspection
- RuntimeFramePolicyText formats policy lines for traces and manifests
- RuntimeRunSummaryText formats run summaries and movement input block counts for traces and manifests
- RuntimeMovementInputBlockSummary counts movement input block reasons for diagnostics
- RuntimeFrameTraceHeaderText formats per-frame trace count headers
- RuntimeRunTraceFrameHeaderText formats frame index markers for full run traces
- RuntimeFrameTraceSections formats runtime source, lifecycle, and simulation trace sections
- RuntimePlayerActionText formats player movement block reasons
- RuntimeSessionText formats session result and event lines for traces
- RuntimeInventoryText formats inventory result and event lines for traces
- RuntimeInventoryScriptText formats inventory script result lines for traces and manifests
- RuntimeMovementScriptText formats movement script replay lines for traces and manifests
- RuntimeMovementEventText formats movement event lines for traces
- RuntimeCombatText formats combat event lines for traces
- RuntimeEffectText formats effect request lines for traces
- RuntimeFrameTrace formats frame reports and mode policy into readable debug lines
- TextFileStore saves/loads readable lines and cleans temp files
- RuntimeFrameTraceFileStore saves and loads readable trace lines
- RuntimeTraceService formats and saves full GameLoopResult traces
- RuntimeSetupSettings defaults to no configured setup scripts
- RuntimeSetupResult defaults to no configured setup script attempts
- RuntimeSetupRunner runs configured movement setup scripts after startup
- RuntimeSetupRunner stops later setup and frames after configured inventory setup failure
- RuntimeSetupRunner distinguishes movement setup failure from command rejection
- RuntimeSetupFrameGate allows only non-fatal setup results to start frames
- RuntimeSourceSettings defaults to no runtime source streams
- RuntimeInputSettings defaults to primary gameplay input context
- RuntimeInputContextBuilder builds per-frame router context from session state
- RuntimeInputRouteResultBuilder names route outcomes consistently
- RuntimeInputDrainResultBuilder aggregates route outcomes for frame input reports
- RuntimeInputDrainReportRecorder copies drain results into frame and run reports
- RuntimeInventoryScriptReportRecorder records script results and flattens command results
- RuntimeInventoryCommandReportRecorder appends direct command results to frame and run reports
- RuntimeSessionCommandReportRecorder records frame and aggregate session command results
- RuntimeMovementScriptReportRecorder records frame and aggregate movement script results
- RuntimeMovementCommandReportRecorder records frame and aggregate queued movement counts
- RuntimeFrameEventReportRecorder records frame events and the latest summary snapshot
- RuntimeFrameCompletionReportRecorder stores completed frames and counts frames run
- RuntimeSetupInventoryCommandReportRecorder records setup command results without frames
- RuntimeSetupRunResultApplier copies setup results and records setup summary effects
- RuntimeEventStreamDelta slices long-lived event streams from an offset
- RuntimeFrameEventDeltaCollector captures only session/inventory events emitted during a frame
- RuntimeFramePolicyReportRecorder records the current simulation frame policy
- RuntimeFramePolicyResolver maps session mode to current simulation frame policy
- RuntimeSimulationFrameUpdater advances sessions with runtime frame settings
- RuntimeFrameSimulationPhaseRunner records policy and events while advancing simulation
- RuntimeFrameSettings defaults to zero frames at a sixty hertz timestep
- RuntimeFrameSourcePhaseRunner routes and drains runtime sources before simulation
- RuntimeFrameRunner routes sources and records one bounded frame
- RuntimeFrameLoopRunner runs the configured bounded frame count
- RuntimeRunSummary defaults to empty aggregate run state
- RuntimeRunRecorder aggregates setup inventory results without creating frames
- RuntimeRunRecorder turns frame work into frame reports and run summaries
- RuntimeRunExecutor runs setup, frames, and finalization in lifecycle order
- RuntimeFinalModeRecorder captures final session mode onto GameLoopResult
- RuntimeRunFinalizer delegates final mode capture and applies output finalization
- RuntimeSetupFailurePolicy maps setup script failures to setup failures
- RuntimeOutputFailurePolicy maps attempted unsaved artifacts to output failures
- RuntimeRunFailurePolicy composes setup and output failures into run failures
- RuntimeExitCodeMapper maps run failure booleans to process exit codes
- RuntimeExitCodePolicy adapts GameLoopResult failure state to process failures
- RuntimeOutputSettings defaults to no runtime artifact outputs
- RuntimeOutputResult defaults to no artifact save attempts
- RuntimeOutputResultBuilder records artifact attempt/save flags and snapshots them onto GameLoopResult
- RuntimeRunTraceOutputStep saves run traces and updates output flags
- RuntimeDebugBundleOutputStep saves debug bundles and updates output flags
- RuntimeOutputFinalizer writes artifact output results back onto GameLoopResult
- RuntimeArtifactOutputPlan maps output settings into ordered artifact requests
- RuntimeArtifactOutputRequestRunner executes planned artifact requests
- RuntimeArtifactOutputService applies configured artifact outputs and reports failures
- RuntimeSetupRunner separates configured setup scripts from per-frame runtime sources
- GameLoop saves configured run traces and reports trace write failures
- RuntimeDebugManifestIndexText formats debug bundle version and trace save state lines
- RuntimeDebugManifestSetupText formats setup script attempt flags for manifests
- RuntimeDebugManifestPathsText formats manifest artifact path lines
- RuntimeDebugManifestSections formats run status, setup, and runtime script manifest groups
- RuntimeDebugManifest formats debug bundle manifest lines with latest frame policy and movement script replay summaries
- RuntimeDebugArtifactLayout names stable debug bundle artifact paths
- RuntimeDebugArtifactBundleResultBuilder records bundle paths, root preparation, and write flags
- RuntimeDebugArtifactRootPreparer creates bundle roots and rejects root files
- RuntimeDebugManifestContextBuilder maps artifact paths and trace save state into manifest context
- RuntimeDebugTraceWriteStep saves bundle run traces
- RuntimeDebugManifestWriteStep saves manifest lines with trace save state
- RuntimeDebugArtifactWriter saves trace/manifest artifacts and records trace failures
- RuntimeDebugArtifactBundle saves a manifest and run trace as one debug bundle
- GameLoop saves configured debug bundles and reports bundle write failures
- QueuedSessionCommandSource drains lifecycle commands exactly once
- GameLoop drains runtime lifecycle command sources before frame updates
- GameLoop runs startup scripts before runtime lifecycle command sources
- QueuedMovementCommandSource drains movement commands exactly once
- RuntimeMovementCommandQueueStep queues built movement commands as route results
- GameLoop drains runtime movement command sources into the active world queue
- GameLoop preserves movement commands when no active world can receive them
- InputEventMatcher recognizes pressed keys and pointer events
- RuntimeInputRouter maps gameplay mouse clicks into movement commands
- RuntimeMovementInputRouter maps gameplay pointer input into movement commands
- RuntimeMovementInputContextBuilder selects player, focus, and movement block state
- RuntimeMovementIntentInputStep maps pointer movement intent into queued commands
- RuntimeBlockedPointerInputStep reports blocked pointer movement without queueing commands
- RuntimeStopMovementInputStep maps stop hotkeys into queued Stop commands
- RuntimeTargetInteractionInputStep maps target-aware pointer input into interaction commands
- RuntimeInputFocusResolver maps session modes into effective input focus
- RuntimeInputRouter blocks movement when focus or text entry owns input
- RuntimeInputRouter reports movement block reasons without routing commands
- RuntimeInputRouter maps lifecycle hotkeys into session commands
- RuntimeSessionInputRouter toggles pause and inventory lifecycle modes
- RuntimeSessionModeTogglePolicy maps lifecycle hotkeys to requested modes
- RuntimeInputRouter maps stop hotkeys into movement commands
- RuntimeInputRouter maps target-aware enemy clicks into MoveThenAct attacks
- RuntimeTargetInputRouter maps target-aware pointer input into interaction movement commands
- RuntimeInputRouter maps stand-ground target clicks into StandAndAct attacks
- QueuedRawInputSource drains raw input events exactly once
- RuntimeInputDrainResultBuilder aggregates handled events and movement block reasons
- RuntimeRawInputDrainer drains raw input sources, counts handled routed events, and reports movement block reasons
- RuntimeInputSourceRouter routes raw input sources through the current session context
- GameLoop routes raw hotkeys through session command dispatch
- GameLoop routes raw mouse clicks through movement command dispatch
- GameLoop uses the active world target registry for target-aware raw mouse input
- GameLoop routes item targets into pickup actions and transfers item ownership
- GameLoop leaves pickup items in the world when inventory is full
- GameLoop drains but does not route blocked raw movement input

Run them with:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
