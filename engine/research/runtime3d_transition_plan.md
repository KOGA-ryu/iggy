# Runtime3D Transition Plan

Updated: 2026-06-20

Superseded for new implementation by `engine/research/iggy3d_side_repo_plan.md`.
New 3D runtime work should target the standalone `iggy3d` repo at
`/Users/kogaryu/iggy3d`, not `engine/src/runtime3d/` in this repo. Keep this
document only as historical context for the earlier in-repo split plan.

Historical in-repo path from the superseded plan: `engine/src/runtime3d/`.

This plan makes 3D its own runtime. Existing 2D runtime, scene, camera,
interaction, and product-loop code can be used as reference material or copied
when the semantics survive the move, but 2D should not be the authoritative
base for the 3D product runtime.

For per-file construction details, use
`engine/research/runtime3d_file_plans/INDEX.md`. That directory follows
one-file-one-document: each planned header, source file, CMake include, test,
and later native integration file has its own document describing what it must
do from start to finish.

## File Paths To Create First

Start with these paths before building gameplay features:

- `engine/src/runtime3d/Runtime3DSessionState.hpp`
- `engine/src/runtime3d/Runtime3DSessionState.cpp`
- `engine/src/runtime3d/Runtime3DSession.hpp`
- `engine/src/runtime3d/Runtime3DSession.cpp`
- `engine/src/runtime3d/Runtime3DClock.hpp`
- `engine/src/runtime3d/Runtime3DClock.cpp`
- `engine/src/runtime3d/Runtime3DCommand.hpp`
- `engine/src/runtime3d/Runtime3DCommand.cpp`
- `engine/src/runtime3d/Runtime3DCommandAdmission.hpp`
- `engine/src/runtime3d/Runtime3DCommandAdmission.cpp`
- `engine/src/runtime3d/Runtime3DWorldState.hpp`
- `engine/src/runtime3d/Runtime3DWorldState.cpp`
- `engine/src/runtime3d/Runtime3DEntityId.hpp`
- `engine/src/runtime3d/Runtime3DEntityState.hpp`
- `engine/src/runtime3d/Runtime3DEntityState.cpp`
- `engine/src/runtime3d/Runtime3DTransform.hpp`
- `engine/src/runtime3d/Runtime3DCollisionVolume.hpp`
- `engine/src/runtime3d/Runtime3DInteractionVolume.hpp`
- `engine/src/runtime3d/Runtime3DTargetQuery.hpp`
- `engine/src/runtime3d/Runtime3DTargetQuery.cpp`
- `engine/src/runtime3d/Runtime3DCameraState.hpp`
- `engine/src/runtime3d/Runtime3DCameraState.cpp`
- `engine/src/runtime3d/Runtime3DCameraModePolicy.hpp`
- `engine/src/runtime3d/Runtime3DCameraModePolicy.cpp`
- `engine/src/runtime3d/Runtime3DRayProjection.hpp`
- `engine/src/runtime3d/Runtime3DRayProjection.cpp`
- `engine/src/runtime3d/Runtime3DSceneProjection.hpp`
- `engine/src/runtime3d/Runtime3DSceneProjection.cpp`
- `engine/src/runtime3d/Runtime3DSaveEnvelope.hpp`
- `engine/src/runtime3d/Runtime3DSaveEnvelope.cpp`
- `engine/src/runtime3d/Runtime3DSaveLoad.hpp`
- `engine/src/runtime3d/Runtime3DSaveLoad.cpp`
- `engine/src/runtime3d/Runtime3DLegacy2DAdapter.hpp`
- `engine/src/runtime3d/Runtime3DLegacy2DAdapter.cpp`
- `engine/cmake/iggy_runtime3d_sources.cmake`
- `engine/cmake/iggy_runtime3d_tests.cmake`

Do not place new 3D runtime files in `engine/src/runtime/` unless the file is a
shared save, package, or command utility intentionally reused by both runtime
lanes. The 3D lane needs an obvious source boundary.

## Most Likely Existing Files Needed

Use these as reference or integration surfaces:

- `engine/src/runtime/RuntimeGameplayProductLoop.hpp`
- `engine/src/runtime/RuntimeGameplayProductLoop.cpp`
- `engine/src/runtime/RuntimeGameplayProductFrameRequest.hpp`
- `engine/src/runtime/RuntimeGameplayProductFrameRequest.cpp`
- `engine/src/runtime/RuntimeGameplayProductInputAdapter.hpp`
- `engine/src/runtime/RuntimeGameplayProductInputAdapter.cpp`
- `engine/src/runtime/RuntimeGameplayProductInputAccumulator.hpp`
- `engine/src/runtime/RuntimeGameplayProductInputAccumulator.cpp`
- `engine/src/runtime/RuntimeGameplayProductInputFrameTargetContext.hpp`
- `engine/src/runtime/RuntimeGameplayProductInputFrameTargetContext.cpp`
- `engine/src/runtime/RuntimeGameplayProductInputFrameTargetAction.hpp`
- `engine/src/runtime/RuntimeGameplayProductInputFrameTargetAction.cpp`
- `engine/src/runtime/RuntimeGameplayProductInteractionTargetQuery.hpp`
- `engine/src/runtime/RuntimeGameplayProductInteractionTargetQuery.cpp`
- `engine/src/runtime/RuntimeGameplaySnapshot.hpp`
- `engine/src/runtime/RuntimeGameplaySnapshot.cpp`
- `engine/src/runtime/RuntimeGameplaySaveSlotStore.hpp`
- `engine/src/runtime/RuntimeGameplaySaveSlotStore.cpp`
- `engine/src/runtime/RuntimeSessionSnapshot.hpp`
- `engine/src/runtime/RuntimeSessionSnapshot.cpp`
- `engine/src/runtime/RuntimeSaveFileEnvelope.hpp`
- `engine/src/runtime/RuntimeSaveFileEnvelope.cpp`
- `engine/src/runtime/RuntimeSaveSlotPathPolicy.hpp`
- `engine/src/runtime/RuntimeSaveSlotPathPolicy.cpp`
- `engine/src/scene/interaction/InteractionReach2D.hpp`
- `engine/src/scene/interaction/InteractionReach2D.cpp`
- `engine/src/scene/interaction/InteractionTargetQuery2D.hpp`
- `engine/src/scene/interaction/InteractionTargetQuery2D.cpp`
- `engine/src/scene/inventory/InventoryState2D.hpp`
- `engine/src/scene/inventory/InventoryState2D.cpp`
- `engine/src/scene/level/LevelTileMap.hpp`
- `engine/src/scene/level/LevelRuntimeBuilder.hpp`
- `engine/src/scene/level/LevelRuntimeBuilder.cpp`
- `engine/apps/native_play/NativeProductSession.hpp`
- `engine/apps/native_play/NativeProductSession.cpp`
- `engine/apps/native_play/IggyNativePlay.cpp`
- `engine/apps/native_play/NativePlayMath.hpp`
- `engine/apps/native_play/NativeSceneDrawList.hpp`
- `engine/apps/native_play/NativeVulkanRenderer.hpp`
- `engine/apps/native_play/NativeVulkanRenderer.cpp`
- `engine/content/demos/product_loop_demo/package.toml`
- `engine/tests/runtime_product_loop_acceptance_demo_tests.cpp`
- `engine/tests/runtime_gameplay_product_frame_request_tests.cpp`
- `engine/tests/runtime_gameplay_product_input_frame_target_action_tests.cpp`
- `engine/tests/runtime_gameplay_product_pointer_projection_tests.cpp`

Use the 2D files to answer semantic questions. Copy code only when the names,
units, invariants, and data ownership still match 3D. Most 2D code should be
ported as behavior, not included directly.

## Shared Math Paths

Promote app-local 3D math out of `engine/apps/native_play/NativePlayMath.hpp`
before runtime3d depends on it.

Create these shared math files:

- `engine/src/core/math/Vec3.hpp`
- `engine/src/core/math/Vec3.cpp`
- `engine/src/core/math/Ray3.hpp`
- `engine/src/core/math/Ray3.cpp`
- `engine/src/core/math/Aabb3.hpp`
- `engine/src/core/math/Aabb3.cpp`
- `engine/src/core/math/Mat4.hpp`
- `engine/src/core/math/Mat4.cpp`
- `engine/src/core/math/Transform3.hpp`
- `engine/src/core/math/Transform3.cpp`

Semantics:

- `Vec3`: world-space value type, meters or tile-units converted through a
  runtime3d scale policy.
- `Ray3`: origin and normalized direction, used for camera picking and
  target discovery.
- `Aabb3`: broad-phase collision and interaction query volume.
- `Mat4`: backend-free matrix for camera view/projection and renderer handoff.
- `Transform3`: position, yaw/pitch/roll or rotation representation, and scale
  if needed for runtime scene projection.

Ownership:

- Core math owns pure value types and deterministic operations.
- Runtime3D owns how those values become gameplay state.
- Native renderer owns GPU interpretation and buffer upload.

## Build Registration Paths

Add these files:

- `engine/cmake/iggy_runtime3d_sources.cmake`
- `engine/cmake/iggy_runtime3d_tests.cmake`

Modify:

- `engine/CMakeLists.txt`

Rules:

- Include `iggy_runtime3d_sources.cmake` after core sources and before native
  app build surfaces consume engine symbols.
- Include `iggy_runtime3d_tests.cmake` under `IGGY_BUILD_TESTS` with
  `IGGY_TEST_DEFAULT_LABELS runtime3d`.
- Do not mix runtime3d tests into `iggy_runtime_tests.cmake` except for
  transitional adapter tests that intentionally prove old-runtime interop.

Expected test files:

- `engine/tests/runtime3d_session_state_tests.cpp`
- `engine/tests/runtime3d_clock_tests.cpp`
- `engine/tests/runtime3d_command_admission_tests.cpp`
- `engine/tests/runtime3d_world_state_tests.cpp`
- `engine/tests/runtime3d_legacy_2d_adapter_tests.cpp`
- `engine/tests/runtime3d_camera_mode_policy_tests.cpp`
- `engine/tests/runtime3d_ray_projection_tests.cpp`
- `engine/tests/runtime3d_target_query_tests.cpp`
- `engine/tests/runtime3d_scene_projection_tests.cpp`
- `engine/tests/runtime3d_save_load_tests.cpp`
- `engine/tests/runtime3d_acceptance_demo_tests.cpp`

## Runtime3D Ownership

Runtime3D owns:

- session lifecycle;
- player slots and actor authority;
- world/entity state;
- stable 3D entity ids;
- transforms and interaction/collision volumes;
- simulation clock modes;
- admitted command records;
- target discovery and reach/range validation;
- real-time versus tactical camera mode state;
- runtime scene projection facts;
- save/load envelope semantics;
- replay and future multiplayer command boundaries.

Runtime3D does not own:

- SDL, Qt, or OS input events;
- mouse capture and window focus;
- UI widgets or HUD drawing;
- Vulkan resources;
- mesh upload or shader state;
- authoring-tool mutation UX;
- raw 2D render commands;
- app-local scripted-control parsing, except through test adapters.

Native shell owns transient input collection and passes normalized intent data
to runtime3d. Renderer draws resolved scene facts and camera matrices. Old 2D
runtime is a source and adapter, not an authority.

## 3D Session Semantics

`Runtime3DSessionState` is the top-level state for 3D play.

Shape:

```text
Runtime3DSessionState
  package identity
  scenario identity
  content compatibility metadata
  lifecycle state
  clock state
  player slots
  world state
  camera mode state
  current selection/target state
  command log
  initial/retry snapshot
  save compatibility data
```

Lifecycle values:

- `NotLoaded`
- `Loading`
- `PlayingRealtime`
- `PlanningTactical`
- `Paused`
- `Completed`
- `Failed`
- `IncompatibleSave`

Rules:

- Real-time first-person or close third-person play and tactical slow-time play
  are modes of one session.
- Tactical view changes command planning and camera projection, not world truth.
- Retry restores the initial runtime3d snapshot.
- Reset reloads or rebuilds the session from accepted package content.
- Load replaces the session only after package/save compatibility passes.

## Camera And Time Semantics

Camera mode enum:

```text
Runtime3DCameraMode
  RealtimeFirstPerson
  RealtimeThirdPersonClose
  TacticalOrbit
  TacticalOverhead
```

Clock mode enum:

```text
Runtime3DClockMode
  Normal
  Slow
  Paused
  StepRequested
```

Mode rules:

- Normal clock uses `RealtimeFirstPerson` or `RealtimeThirdPersonClose`.
- Slow-time enters `TacticalOrbit` or `TacticalOverhead`.
- Paused tactical planning may keep tactical camera input alive while automatic
  simulation ticks stop.
- Returning to normal restores the previous real-time camera mode.
- Camera mode switching clears transient movement and pointer state.
- Camera is runtime3d presentation/input policy, not renderer state.

Minimum builder packet:

- `Runtime3DClock` decides whether a tick should run.
- `Runtime3DCameraModePolicy` maps clock/session mode to active camera mode.
- `Runtime3DRayProjection` turns a camera ray into a world target candidate.
- `Runtime3DTargetQuery` validates reach/range/action constraints.

## Command Semantics

Runtime3D command records must be serializable from the start.

Shape:

```text
Runtime3DCommandRecord
  command id
  player slot id
  actor entity id
  command kind
  target entity id or world point
  issued tick
  sequence number
  admission status
  rejection reason
```

Required command kinds for the demo:

- move;
- interact;
- inspect;
- wait;
- toggle tactical mode;
- step tactical tick;
- retry;
- reset;
- save;
- load.

Rules:

- Raw input events do not enter command records.
- Rejected commands are recorded with reasons and no mutation.
- Reach/range checks live in command admission, not in the native shell.
- Single-player is a local authoritative session using the same command model
  that multiplayer will replicate later.

## World And Entity Semantics

`Runtime3DWorldState` owns all gameplay-visible 3D entities.

Entity shape:

```text
Runtime3DEntityState
  stable entity id
  entity kind
  owner/runtime source id
  transform
  collision volume
  interaction volume
  asset/model ref
  faction/team
  tags
  persistence flags
```

Entity kinds:

- player;
- ally;
- enemy;
- pickup;
- door;
- wall;
- floor;
- prop;
- objective;
- tactical marker;
- camera anchor.

Initial collision can be simple:

- capsule or AABB for actors;
- AABB for doors, pickups, props, and walls;
- ground plane or tile-derived slabs for floor.

Do not wait for a full physics engine. The first runtime3d demo needs stable
volumes and deterministic reach/range queries.

## Legacy 2D Adapter

Use `Runtime3DLegacy2DAdapter` only as a temporary bridge.

Adapter inputs may read:

- `RuntimeGameplayProductLoopState`;
- `RuntimeGameplayState`;
- `LevelTileMap`;
- current player position;
- NPC actor positions;
- interaction targets;
- inventory/pickup/door data;
- current product demo package identity.

Adapter outputs:

- `Runtime3DSessionState`;
- `Runtime3DWorldState`;
- `Runtime3DEntityState` records;
- optional command replay seed data for acceptance tests.

Rules:

- The adapter is one-way for the first build: 2D state to runtime3d state.
- Runtime3D gameplay code must not call 2D interaction/reach functions during
  normal command admission once the equivalent 3D function exists.
- Every copied 2D behavior needs a matching runtime3d test that states the new
  semantic owner.
- The adapter should be deleted or isolated once authored 3D package load exists.

## Native Play Integration Paths

Prefer a new native session wrapper instead of expanding the old one forever:

- `engine/apps/native_play/Native3DProductSession.hpp`
- `engine/apps/native_play/Native3DProductSession.cpp`

Modify:

- `engine/apps/native_play/IggyNativePlay.cpp`
- `engine/apps/native_play/NativeSceneDrawList.hpp`
- `engine/apps/native_play/NativeVulkanRenderer.hpp`
- `engine/apps/native_play/NativeVulkanRenderer.cpp`
- `engine/cmake/iggy_native_play.cmake`

Native integration rules:

- Native shell reads SDL keyboard, mouse, controller, and window events.
- Native shell normalizes those events into runtime3d input or command proposals.
- Runtime3D returns camera view/projection data and scene projection data.
- Renderer consumes `viewProjection` and draw items only.
- Native shell may own mouse capture and relative mouse deltas, but runtime3d
  owns yaw/pitch/orbit/target semantics after deltas are normalized.

Real-time controls:

- WASD or stick movement;
- mouse/controller look;
- interact;
- inspect;
- tactical-mode toggle.

Tactical controls:

- orbit/pan/zoom;
- cursor/ray target selection;
- command issue;
- step tick;
- resume real time.

## Transition Phases

### Phase 0: Source Boundary

Create `engine/src/runtime3d`, `iggy_runtime3d_sources.cmake`, and
`iggy_runtime3d_tests.cmake`.

Exit criteria:

- empty or minimal runtime3d files build through `iggy_engine`;
- one smoke test proves runtime3d test registration;
- no changes to 2D runtime behavior.

### Phase 1: Shared 3D Math

Promote pure `Vec3`, `Ray3`, `Aabb3`, `Mat4`, and `Transform3` into
`engine/src/core/math`.

Exit criteria:

- native play can still build using either migrated math or a compatibility
  alias;
- runtime3d has no dependency on `engine/apps/native_play/NativePlayMath.hpp`;
- math tests prove identity, multiply, look-at/perspective if those functions
  are promoted.

### Phase 2: Runtime3D State Kernel

Implement session, world, entity id, entity state, and clock state.

Exit criteria:

- runtime3d can create an empty loaded session;
- runtime3d can insert player, floor, wall, door, pickup, and enemy entities;
- clock policy can return normal, slow, paused, and one-step decisions.

### Phase 3: Legacy 2D To 3D Demo Adapter

Build `Runtime3DLegacy2DAdapter` against the current product-loop demo.

Exit criteria:

- product-loop demo package can become a runtime3d session;
- expected 3D entities appear for floor, walls, door, key, NPC, and player;
- adapter tests declare which 2D semantics were copied and which were not.

### Phase 4: Runtime3D Commands And Reach

Implement serializable command records and 3D command admission.

Exit criteria:

- in-range interaction succeeds;
- out-of-range interaction is rejected without mutation;
- rejected commands produce deterministic reason codes;
- command records do not contain SDL/Qt/native event values.

### Phase 5: Camera And Tactical Mode

Implement real-time first-person, close third-person, and tactical camera state.

Exit criteria:

- normal mode uses first-person or close third-person;
- slow-time switches to tactical camera;
- returning to normal restores the previous real-time camera;
- ray projection can select a ground point or interaction volume;
- transient movement input clears on mode switch.

### Phase 6: Scene Projection

Implement `Runtime3DSceneProjection` and feed native draw items from runtime3d.

Exit criteria:

- renderer receives semantic draw items from runtime3d;
- player, floor, walls, door, key, NPC, target marker, and selected target can
  appear in the native draw path;
- renderer does not inspect gameplay state directly.

### Phase 7: Save/Load

Implement runtime3d save envelope and minimal save/load.

Exit criteria:

- save records package/scenario identity, clock mode, camera mode, world state,
  command log summary, and compatibility metadata;
- load validates compatibility before replacing active state;
- save/load does not persist raw input events or renderer matrices.

### Phase 8: Acceptance Demo

Implement a runtime3d acceptance test and native scripted demo.

Exit criteria:

- load demo;
- discover target;
- reject out-of-reach interaction;
- move/reposition into reach;
- interact with pickup or door;
- enter slow time and tactical camera;
- issue a tactical command;
- pause/step/resume;
- save/load;
- retry/reset;
- produce deterministic state summary.

## Compute And Storage Budgets

Initial acceptable costs:

- camera mode policy: `O(1)` per frame;
- clock policy: `O(1)` per frame;
- command admission: `O(command + candidate targets)`;
- naive target query: `O(interaction volumes)` until a spatial index is needed;
- scene projection: `O(renderable entities)`;
- save/load: `O(world entities + command records + package metadata)`;
- legacy adapter: `O(tile count + actors + interaction targets)`.

Budget rules:

- The demo can start with linear scans because entity counts are small.
- Add spatial indexing only when profiling or test fixture scale proves a need.
- Do not store renderer draw lists in saves.
- Command logs should be bounded or checkpointed before long-running sessions.
- Whole-state copy APIs are acceptable for the demo, but large vectors should be
  watched as 3D content grows.

## Builder Packet Rules

Every runtime3d builder packet must include:

- purpose;
- exact file paths touched;
- semantic owner;
- data owner;
- write scope;
- 2D source files referenced or copied from;
- copied behavior versus changed behavior;
- compute cost;
- storage cost if save/session data changes;
- tests added or changed;
- verification command;
- known follow-up packet.

Block a packet if:

- it adds 3D runtime authority under `engine/src/runtime/` without a stated
  reason;
- it makes runtime3d include native app headers;
- it persists raw input or renderer state;
- it uses camera mode as gameplay truth;
- it lets renderer own target/reach/interaction semantics;
- it expands old 2D runtime behavior while claiming to build the 3D runtime;
- it lacks a test for new runtime3d semantics.
