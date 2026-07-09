# File Spec

Files: `src/app/iggy3d/creative/CreativeBlankStageSession.hpp`, `src/app/iggy3d/creative/CreativeBlankStageSession.cpp`

Verified at: `116e4a9e`

## Owns

- Product creative blank-stage runtime session setup.
- Minimal creative session seed with local player and inert objective.
- Startup/package/load/session-create proof fields for the creative blank stage.
- Active room/collision clearing and creative fly camera framing for origin entry.

## Does Not Own

- Product demo room session creation.
- Creative document creation/open/save behavior.
- Active room bake refresh.
- Session runtime internals or objective system implementation.
- Creative fly movement kernel.

## Reads

- Product app window state.
- Session creation API and scenario seed structures.
- Active room and collision stores.
- Creative fly anchor store helpers.

## Writes / Mutates

- Mutates active session optional on successful session creation.
- Mutates frontend startup/load/session proof fields.
- Clears active room and active room collision state.
- Bumps active room revision and creative world epoch.
- Seeds creative fly origin anchor and sets camera yaw/pitch.

## Calls Out To / Wires Out To

- Calls `Session::create(...)`.
- Calls `activeRoom(...)`, `activeRoomCollision(...)`, and `bumpActiveRoomRevision(...)`.
- Calls `bumpCreativeWorldEpoch(...)` and `seedCreativeFlyAnchorFromOrigin(...)`.
- Creative world launch/open operations call these helpers to avoid loading the demo room.

## Called By / Entry Points

- `createCreativeBlankSession(...)`.
- `frameCreativeStageCameraOnOrigin(...)`.
- Focused proof: `rg -n "createCreativeBlankSession|frameCreativeStageCameraOnOrigin|creative_blank_stage" src/app tests`.

## Invariants

- Creative blank stage must not install the first-room demo active room.
- Session seed must include one local player and an inert objective so session creation succeeds without gameplay completion.
- Successful setup marks gameplay active and runtime session created while leaving active room unloaded.
- Origin framing bumps creative epoch before seeding the fly anchor.
- Startup/status fields are product proof mirrors, not package loader truth.

## Tests / Proof Commands

- `rg -n "product_creative_world_launch_tests|blank stage" cmake/iggy3d_tests.cmake tests/unit src/app`.
- `rg -n "creative_blank_stage|seedCreativeFlyAnchorFromOrigin|runtimeSessionCreateStatus" tests/unit/product_creative_world_launch_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless launch/open setup changes.
- `src/app/iggy3d/view/CreativeFlyAnchorStore.*` unless anchor seeding changes.
- `src/app/iggy3d/gameplay/ActiveRoomState.*` unless active room clearing changes.
- `src/runtime/session/Session.*` unless session creation requirements change.

## Update When

- Blank-stage session seed, startup proof fields, active-room clearing, creative fly origin framing, or launch/open blank-stage ownership changes.

## Do Not Update When

- Only creative document contents, fly movement math, or product demo session launch behavior changes without changing blank-stage setup.
