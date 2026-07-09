# File Spec

Files:

- `src/runtime/physics/PhysicsDebugSnapshot.hpp`
- `src/runtime/physics/PhysicsDebugSnapshot.cpp`

Verified at: `9288e13b`

## Owns

- Physics debug snapshot request/config/result packet shape.
- Snapshot status names and reason codes.
- Copying selected `PhysicsFrameStats` counters into a debug-facing packet.
- Deriving warning and activity flags for failed packets, broadphase pressure, contacts, solver activity, applied deltas, kinematic/player hits, penetration, and impulse thresholds.

## Does Not Own

- Physics simulation, frame stats accumulation, or source packet generation.
- Projection, app HUD formatting, receipt recording, or render output.
- Save/hash persistence.

## Reads

- Caller-owned `PhysicsFrameStats`.
- Debug config enable flag and warning thresholds.
- Stats ok/status/reason, counters, impulse magnitudes, penetration, and player/motor facts.

## Writes / Mutates

- Local `PhysicsDebugSnapshot` packet only.
- No mutation of `PhysicsFrameStats`, simulation state, app state, save state, or render state.

## Calls Out To / Wires Out To

- `ProjectionRefresh.cpp` builds a physics debug snapshot for app-facing projection/debug surfaces.
- `PhysicsFrameStats.*` is the upstream packet source.
- Warning flags are consumed downstream as observability, not simulation truth.

## Called By / Entry Points

- Direct API: `isValidPhysicsDebugSnapshotConfig(...)` and `buildPhysicsDebugSnapshot(...)`.
- Grep proof: `rg -n "buildPhysicsDebugSnapshot|PhysicsDebugSnapshot" src/runtime src/app tests/unit`.

## Invariants

- Disabled config returns ok disabled snapshot without copying counters.
- Invalid config returns non-ok invalid config and does not copy counters.
- Missing stats returns non-ok missing stats.
- Failed upstream stats still copy counters for inspection and set warnings.
- Zero thresholds disable that threshold family.
- Debug snapshots are off-save/off-hash observability packets.

## Tests / Proof Commands

- `rg -n "physics_debug_snapshot_tests" cmake tests/unit`.
- `rg -n "readyStatsCopyRepresentativeCounters|failedStatsStillCopyForInspection|warningFlagsUseThresholds|quietStatsHaveNoWarnings" tests/unit/physics_debug_snapshot_tests.cpp`.

## Nearby Files Usually Not Touched

- `src/runtime/physics/PhysicsFrameStats.*` unless source counters or stats status semantics change.
- `src/app/iggy3d/gameplay/ProjectionRefresh.*` unless projection consumption changes.
- App debug HUD/receipt files unless presentation fields change.

## Update When

- Snapshot fields, status names, config validation, copied counters, activity flags, warning thresholds, or failed-stats handling changes.

## Do Not Update When

- Source physics systems change internals but frame stats packet semantics stay stable.
- App presentation changes without changing snapshot contract.
