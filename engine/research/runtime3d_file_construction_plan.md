# Runtime3D File Construction Plan

Updated: 2026-06-20

This document is now an overview only. The canonical builder contract is one
document per implementation file under:

`engine/research/runtime3d_file_plans/`

Start at `engine/research/runtime3d_file_plans/INDEX.md`, then open the
document that matches the file being built. One implementation file maps to one
planning document.

This is the file-by-file construction manual for the new 3D runtime lane.
Runtime3D lives under `engine/src/runtime3d/`. Current 2D runtime files are
reference material or temporary adapter inputs only.

The builder should not infer file ownership from the old 2D runtime. Every file
below names its purpose, owned data, construction steps, implementation rules,
tests, and stop conditions.

## Construction Rules

- Build 3D authority in `engine/src/runtime3d/`.
- Keep reusable math in `engine/src/core/math/`.
- Keep platform, SDL, Vulkan, and renderer headers out of runtime3d.
- Keep raw input events out of command records and saves.
- Treat camera mode as presentation/input policy, not gameplay truth.
- Treat command admission as the owner of reach, range, and target validity.
- Keep `engine/src/runtime/` behavior unchanged unless a later adapter packet
  explicitly needs a read-only bridge.
- Use value types, plain structs, enums, and pure helpers first. Add object
  lifetime or inheritance only when the runtime has a real need.

## Build Order

The order below is the intended start-to-finish construction order. It is not a
small-slice plan; it is the complete skeleton route.

1. Core math files.
2. Runtime3D primitive value files.
3. World/entity state files.
4. Clock and camera mode files.
5. Command and admission files.
6. Query/projection files.
7. Save/load placeholder files.
8. Legacy 2D adapter placeholder files.
9. Session files.
10. CMake source and test registration.
11. Unit/smoke tests.
12. Later native integration files.

## Core Math Files

### `engine/src/core/math/Vec3.hpp`

Purpose: define the shared 3D vector value type used by runtime3d, scene
projection, camera math, and native rendering bridges.

Owns:

- `struct Vec3`;
- scalar-free data layout: `float x`, `float y`, `float z`;
- method declarations for `lengthSquared`, `length`, and `normalized`;
- operator declarations for `+`, `-`, `*`, `/`, `==`;
- `Dot` and `Cross` declarations.

Construction:

- Mirror the style of `Vec2.hpp`.
- Keep namespace `iggy`, not `iggy::runtime3d`.
- Include only `<cmath>` if needed by inline declarations; prefer the `.cpp`
  for math bodies.
- Do not add game units here. Runtime3D scale policy decides whether one unit
  means one tile, one meter, or another authored scale.

Tests:

- Add core math tests later when operations become non-trivial.
- Runtime3D tests may use `Vec3` immediately for transform defaults.

Stop condition:

- Do not put matrices, camera policy, or collision semantics in this file.

### `engine/src/core/math/Vec3.cpp`

Purpose: implement pure `Vec3` operations.

Owns:

- length calculation;
- zero-safe normalization;
- arithmetic operators;
- `Dot`;
- `Cross`.

Construction:

- Match `Vec2.cpp` layout.
- Return zero vector from `normalized()` when magnitude is zero.
- Keep equality exact for now, matching `Vec2`.

Compute cost:

- `O(1)` for every operation.

Stop condition:

- Do not add epsilon comparisons until a caller proves the need.

### `engine/src/core/math/Ray3.hpp`

Purpose: shared ray type for camera picking and target projection.

Owns:

- `struct Ray3`;
- `Vec3 origin`;
- `Vec3 direction`;
- `pointAtDistance(float distance)` declaration.

Construction:

- Mirror `Ray2.hpp`.
- Do not force normalization in the struct constructor; projection/admission
  policies decide when a ray must be normalized.

Stop condition:

- No collision queries here. It is a value type only.

### `engine/src/core/math/Ray3.cpp`

Purpose: implement `Ray3::pointAtDistance`.

Construction:

- Include `core/math/Ray3.hpp`.
- Return `origin + direction * distance`.

Compute cost:

- `O(1)`.

### `engine/src/core/math/Aabb3.hpp`

Purpose: shared 3D axis-aligned bounding box type for broad-phase target,
interaction, and collision volumes.

Owns:

- `struct Aabb3`;
- `Vec3 min`;
- `Vec3 max`;
- `contains(Vec3 point)` declaration;
- `overlaps(Aabb3 other)` declaration.

Construction:

- Mirror `Aabb2.hpp`.
- Keep inclusive min/max behavior unless tests establish another convention.

Stop condition:

- Do not add physics response or sweep logic here.

### `engine/src/core/math/Aabb3.cpp`

Purpose: implement containment and overlap checks.

Construction:

- `contains` returns true when point components are inside inclusive min/max.
- `overlaps` returns true when all axes overlap.
- Treat malformed boxes deterministically but do not normalize min/max silently
  yet.

Compute cost:

- `O(1)`.

### `engine/src/core/math/Mat4.hpp`

Purpose: backend-free matrix type for camera view/projection and renderer
handoff.

Owns:

- `struct Mat4`;
- `std::array<float, 16> values`;
- declarations for `Identity`, `Multiply`, `Translation`, `Scale`,
  `RotationX`, `RotationY`, `Perspective`, and `LookAt` when promoted.

Construction:

- Move semantics from `engine/apps/native_play/NativePlayMath.hpp` only if the
  builder needs runtime3d camera matrices to compile.
- Keep column-major behavior consistent with current native renderer math.
- Put the type in namespace `iggy`.

Stop condition:

- Do not add Vulkan, shader, or push-constant types.

### `engine/src/core/math/Mat4.cpp`

Purpose: implement pure matrix helpers.

Construction:

- Copy only pure math from `NativePlayMath.hpp`.
- Preserve current signs for perspective until renderer tests say otherwise.
- Keep all functions deterministic and allocation-free.

Compute cost:

- `Multiply`: fixed `O(16 * 4)`.
- Other helpers: `O(1)`.

### `engine/src/core/math/Transform3.hpp`

Purpose: shared transform data shape used by runtime3d entities and scene
projection.

Owns:

- `struct Transform3`;
- `Vec3 position`;
- `Vec3 rotation`;
- `Vec3 scale`, default `{ 1.0F, 1.0F, 1.0F }`.

Construction:

- Keep rotation as Euler yaw/pitch/roll in the skeleton.
- Document by field names or comments only if ambiguity appears in tests.
- Do not add quaternion support in the skeleton.

Stop condition:

- No matrices or renderer model transforms unless a later scene projection file
  needs a helper.

### `engine/src/core/math/Transform3.cpp`

Purpose: translation unit for `Transform3`.

Construction:

- Empty implementation is acceptable if all data is POD.
- Keep the `.cpp` because CMake registration and future helpers will need a
  stable place.

## Runtime3D Primitive Files

### `engine/src/runtime3d/Runtime3DEntityId.hpp`

Purpose: stable entity id wrapper for runtime3d state, commands, targets, and
saves.

Owns:

- `struct Runtime3DEntityId`;
- `std::uint32_t value`;
- invalid/default value `0`;
- equality operators;
- helper `valid()` or `IsValid(Runtime3DEntityId)`.

Construction:

- Include `<cstdint>`.
- Namespace `iggy::runtime3d`.
- Keep id generation out of the id type. World/session files decide ids.

Data ownership:

- The id owns only the stable value.
- `Runtime3DWorldState` owns allocation/upsert behavior.

Stop condition:

- No strings, UUIDs, save metadata, or actor ownership here.

### `engine/src/runtime3d/Runtime3DTransform.hpp`

Purpose: runtime-facing transform alias or wrapper for 3D entities.

Owns:

- `using Runtime3DTransform = Transform3` or a thin struct wrapping `Transform3`;
- runtime comment/contract that transform is authoritative gameplay placement,
  not renderer-only model placement.

Construction:

- Include `core/math/Transform3.hpp`.
- Prefer a thin runtime-specific type if later save compatibility needs a
  separate name.

Data ownership:

- Runtime3D owns entity transforms.
- Renderer may receive copied transforms but cannot mutate them as truth.

Stop condition:

- Do not include native renderer math.

### `engine/src/runtime3d/Runtime3DCollisionVolume.hpp`

Purpose: minimal runtime3d collision volume description.

Owns:

- `enum class Runtime3DCollisionVolumeKind`;
- `struct Runtime3DCollisionVolume`;
- supported skeleton kinds: `None`, `Aabb`, `Capsule`;
- `Aabb3 bounds`;
- capsule fields only if needed: `Vec3 base`, `Vec3 tip`, `float radius`.

Construction:

- Include `core/math/Aabb3.hpp` and `core/math/Vec3.hpp`.
- Use `None` default.
- Keep broad-phase only in the skeleton.

Stop condition:

- No collision response, physics stepping, or spatial index yet.

### `engine/src/runtime3d/Runtime3DInteractionVolume.hpp`

Purpose: volume used for target discovery and interact reach validation.

Owns:

- `enum class Runtime3DInteractionVolumeKind`;
- `struct Runtime3DInteractionVolume`;
- kind `None` and `Aabb`;
- `Aabb3 bounds`;
- optional `float reachRadius` only if command admission needs it.

Construction:

- Keep it separate from collision volume. Collision blocks movement; interaction
  answers whether an action can target something.

Stop condition:

- No inventory or effect application here.

## Entity And World Files

### `engine/src/runtime3d/Runtime3DEntityState.hpp`

Purpose: define authoritative runtime3d entity facts.

Owns:

- `enum class Runtime3DEntityKind`;
- `struct Runtime3DEntityState`;
- id;
- kind;
- transform;
- collision volume;
- interaction volume;
- `std::string assetRef`;
- faction/team placeholder if needed;
- `bool persistent`;
- targetability flag if target query needs it.

Construction:

- Include `Runtime3DEntityId.hpp`, `Runtime3DTransform.hpp`,
  `Runtime3DCollisionVolume.hpp`, and `Runtime3DInteractionVolume.hpp`.
- Use explicit defaults: invalid id, `Prop` or `Player` only if tests require;
  prefer `Prop`/`Floor` only after deciding.
- Keep data serializable by shape: no pointers, no refs, no callbacks.

Data ownership:

- This file owns per-entity state shape.
- It does not own entity collection, id allocation, or command mutation.

Stop condition:

- No renderer draw items here.

### `engine/src/runtime3d/Runtime3DEntityState.cpp`

Purpose: implementation home for entity helper functions.

Owns:

- optional `bool IsRuntime3DTargetable(Runtime3DEntityKind kind)`;
- optional text conversion only if tests require deterministic summaries.

Construction:

- Start minimal.
- Add helpers only when multiple files need the same rule.

### `engine/src/runtime3d/Runtime3DWorldState.hpp`

Purpose: own the runtime3d entity collection.

Owns:

- `struct Runtime3DWorldState`;
- `std::vector<Runtime3DEntityState> entities`;
- declarations for `find(Runtime3DEntityId)`;
- declarations for `add` or `upsert`;
- optional `nextEntityId` only if world owns id allocation now.

Construction:

- Keep vectors by value.
- Provide const and mutable find helpers if needed.
- If adding `nextEntityId`, default it to `1` because `0` is invalid.

Data ownership:

- World owns entity storage and lookup.
- Session owns the world.

Compute cost:

- Skeleton lookup: `O(entity count)`.
- Upsert: `O(entity count)`.

Stop condition:

- No spatial index until entity count or profiling requires it.

### `engine/src/runtime3d/Runtime3DWorldState.cpp`

Purpose: implement world collection helpers.

Construction:

- Linear scan for find/upsert.
- `add` should append and return the id or reference/result.
- `upsert` replaces an entity with same id, otherwise appends.
- Reject/ignore invalid ids deterministically if the helper returns status.

Tests:

- `runtime3d_world_state_tests.cpp` proves add/find/upsert.

## Clock Files

### `engine/src/runtime3d/Runtime3DClock.hpp`

Purpose: define simulation clock state and tick-decision policy.

Owns:

- `enum class Runtime3DClockMode`;
- `struct Runtime3DClockState`;
- `struct Runtime3DClockTickDecision`;
- `class Runtime3DClock` or pure free functions.

State fields:

- `Runtime3DClockMode mode = Runtime3DClockMode::Normal`;
- `float timeScale = 1.0F`;
- `float accumulatedSeconds = 0.0F`;

Decision fields:

- `bool runAutomaticTick`;
- `bool consumedStepRequest`;
- `Runtime3DClockState state`;

Construction:

- Clock policy is pure: input state and optional elapsed seconds produce result.
- Normal runs automatic ticks.
- Slow runs automatic ticks with slow scale, but skeleton may only report the
  mode and time scale.
- Paused does not run automatic ticks.
- StepRequested runs exactly one tick and returns mode Paused.

Data ownership:

- Session owns clock state.
- Clock file owns clock transition semantics.

Stop condition:

- No native wall-clock API here.

### `engine/src/runtime3d/Runtime3DClock.cpp`

Purpose: implement deterministic clock decisions.

Construction:

- Switch on `Runtime3DClockMode`.
- Clamp invalid/negative timeScale only if tests demand it; otherwise preserve
  explicit state and let validation come later.
- For StepRequested, set `consumedStepRequest = true`, `runAutomaticTick = true`,
  and returned state mode `Paused`.

Tests:

- `runtime3d_clock_tests.cpp`.

## Camera Files

### `engine/src/runtime3d/Runtime3DCameraState.hpp`

Purpose: define runtime3d camera state for real-time and tactical modes.

Owns:

- `enum class Runtime3DCameraMode`;
- `struct Runtime3DCameraModeState`;
- `struct Runtime3DCameraPose` if useful;
- yaw, pitch, orbit distance, target entity id, target point fields.

Required defaults:

- active mode: `RealtimeThirdPersonClose` unless product config chooses FPS;
- previous real-time mode: `RealtimeThirdPersonClose`;
- tactical mode default: `TacticalOrbit` or `TacticalOverhead`;
- orbit distance conservative default, for example `8.0F`;
- pitch/yaw `0.0F`.

Construction:

- Include `core/math/Vec3.hpp` and `Runtime3DEntityId.hpp`.
- Keep camera state serializable: no raw mouse deltas, no OS input, no renderer
  resources.

Data ownership:

- Runtime3D owns mode and semantic pose.
- Native shell owns raw deltas and capture.
- Renderer consumes derived matrices.

### `engine/src/runtime3d/Runtime3DCameraState.cpp`

Purpose: implementation home for camera helpers.

Owns:

- optional helper `IsRealtimeCameraMode`;
- optional helper `IsTacticalCameraMode`;
- optional normalization helper for previous real-time mode.

Construction:

- Keep helpers pure and testable.
- Do not compute view/projection matrices here unless `Runtime3DRayProjection`
  or scene projection needs it.

### `engine/src/runtime3d/Runtime3DCameraModePolicy.hpp`

Purpose: define how clock/session mode maps to camera mode.

Owns:

- `struct Runtime3DCameraModePolicyInput`;
- `struct Runtime3DCameraModePolicyResult`;
- `class Runtime3DCameraModePolicy`.

Input fields:

- current camera state;
- clock mode;
- session lifecycle;
- preferred tactical camera mode if needed.

Result fields:

- updated camera state;
- `bool changedMode`;
- optional `bool clearedTransientInput`.

Construction:

- Include `Runtime3DCameraState.hpp`, `Runtime3DClock.hpp`, and
  `Runtime3DSessionState.hpp` only if that does not create a cycle. If it would
  cycle, define a tiny lifecycle enum include boundary or pass lifecycle enum
  from a separate header.
- To avoid cycles, prefer putting lifecycle enum in `Runtime3DSessionState.hpp`
  and include it only in the `.cpp`, or create a small
  `Runtime3DSessionLifecycle.hpp` later if needed.

Rules:

- Normal clock restores previous real-time camera.
- Slow clock switches to tactical.
- PlanningTactical lifecycle switches to tactical.
- Paused keeps tactical if already tactical, otherwise uses tactical default.
- Realtime camera changes update `previousRealtimeMode`.

### `engine/src/runtime3d/Runtime3DCameraModePolicy.cpp`

Purpose: implement camera mode transition semantics.

Construction:

- Use helper functions from `Runtime3DCameraState.cpp`.
- Do not inspect renderer state.
- Do not mutate world state.
- Return explicit changed-mode flag for native shell to clear held input.

Tests:

- `runtime3d_camera_mode_policy_tests.cpp`.

## Command Files

### `engine/src/runtime3d/Runtime3DCommand.hpp`

Purpose: define serializable player/session command records.

Owns:

- `enum class Runtime3DCommandKind`;
- `enum class Runtime3DCommandAdmissionStatus`;
- `enum class Runtime3DCommandRejectionReason`;
- `struct Runtime3DCommandRecord`.

Command fields:

- `std::uint64_t commandId`;
- `std::uint64_t sequence`;
- `std::uint32_t playerSlot`;
- `Runtime3DEntityId actor`;
- `Runtime3DCommandKind kind`;
- `bool hasTargetEntity`;
- `Runtime3DEntityId targetEntity`;
- `bool hasTargetPoint`;
- `Vec3 targetPoint`;
- `std::uint64_t issuedTick`;
- admission status;
- rejection reason.

Construction:

- Include `<cstdint>`, `core/math/Vec3.hpp`, and `Runtime3DEntityId.hpp`.
- Default command is inert and rejected/unadmitted until admission runs.
- Keep raw input out.

Data ownership:

- Commands own request facts and admission result.
- Session owns command log.

### `engine/src/runtime3d/Runtime3DCommand.cpp`

Purpose: implementation home for command helper functions.

Owns:

- optional `bool RequiresActor(Runtime3DCommandKind)`;
- optional `bool IsSessionCommand(Runtime3DCommandKind)`;
- optional text conversion for test diagnostics.

Construction:

- Keep helpers deterministic.
- Do not mutate session/world here.

### `engine/src/runtime3d/Runtime3DCommandAdmission.hpp`

Purpose: define command validation/admission surface.

Owns:

- `struct Runtime3DCommandAdmissionInput`;
- `struct Runtime3DCommandAdmissionResult`;
- `class Runtime3DCommandAdmission`.

Input fields:

- `Runtime3DWorldState world`;
- proposed `Runtime3DCommandRecord`;

Result fields:

- admitted/rejected command record;
- status/reason;
- no mutation result yet.

Construction:

- Include `Runtime3DCommand.hpp` and `Runtime3DWorldState.hpp`.
- Do not include session if world is enough.

Rules:

- Invalid actor id rejects for actor commands.
- Session commands such as save/load/reset can be admitted without actor if the
  policy says so.
- No mutation on reject.

### `engine/src/runtime3d/Runtime3DCommandAdmission.cpp`

Purpose: implement basic command validation.

Construction:

- Switch on command kind.
- For move/interact/inspect/wait, require a valid actor id and entity exists.
- For interact, if target exists, require target id exists.
- Return command with status and reason filled.
- Keep reach/range skeletal until `Runtime3DTargetQuery` and volumes are ready.

Tests:

- `runtime3d_command_admission_tests.cpp` should be added even if the first
  builder packet does not include it.

## Query And Projection Files

### `engine/src/runtime3d/Runtime3DTargetQuery.hpp`

Purpose: target discovery over runtime3d world state.

Owns:

- `struct Runtime3DTargetQueryInput`;
- `struct Runtime3DTargetQueryResult`;
- `class Runtime3DTargetQuery`.

Input fields:

- world;
- actor id or source point;
- command/action kind;
- optional ray;
- max range;

Result fields:

- status;
- target entity id;
- target point;
- reason if no target.

Construction:

- Start with simple linear scan over entities.
- Prefer targetable entity kinds: pickup, door, enemy, objective, tactical
  marker.
- Exclude invalid ids and source actor.

Compute cost:

- `O(entity count)` skeleton.

Stop condition:

- No spatial tree yet.

### `engine/src/runtime3d/Runtime3DTargetQuery.cpp`

Purpose: implement target query.

Construction:

- Implement first-match or nearest-match, but document the tie behavior in
  tests.
- Use interaction volume if present; otherwise use transform position.
- Return explicit no-target status.

Tests:

- `runtime3d_target_query_tests.cpp`.

### `engine/src/runtime3d/Runtime3DRayProjection.hpp`

Purpose: convert camera ray input into runtime3d target candidates.

Owns:

- `struct Runtime3DRayProjectionInput`;
- `struct Runtime3DRayProjectionResult`;
- `class Runtime3DRayProjection`.

Input fields:

- ray;
- ground plane height or world query surface;
- optional world pointer for volume picking later.

Result fields:

- status;
- hit point;
- hit entity id if available;

Construction:

- Skeleton can project ray to a ground plane at `y = 0`.
- Guard parallel rays with an explicit no-hit status.

Compute cost:

- Ground plane projection: `O(1)`.
- Entity picking later: `O(entity count)` until indexed.

Stop condition:

- Do not require renderer matrices here. Native shell or camera state should
  provide the ray.

### `engine/src/runtime3d/Runtime3DRayProjection.cpp`

Purpose: implement ray-to-world projection.

Construction:

- If direction.y is zero, return no hit.
- Solve distance to plane.
- Reject negative distance unless backward picking is explicitly desired.

Tests:

- `runtime3d_ray_projection_tests.cpp`.

## Scene Projection Files

### `engine/src/runtime3d/Runtime3DSceneProjection.hpp`

Purpose: convert authoritative runtime3d world state into renderer-facing scene
facts without exposing gameplay internals to the renderer.

Owns:

- `enum class Runtime3DSceneItemKind`;
- `struct Runtime3DSceneItem`;
- `struct Runtime3DSceneProjectionInput`;
- `struct Runtime3DSceneProjectionResult`;
- `class Runtime3DSceneProjection`.

Scene item fields:

- entity id;
- item kind;
- transform;
- assetRef;
- selected/targeted flags;

Construction:

- Do not use Vulkan types.
- Do not output save truth.
- Keep item ordering stable: entity vector order first.

Compute cost:

- `O(entity count)`.

### `engine/src/runtime3d/Runtime3DSceneProjection.cpp`

Purpose: implement deterministic scene item projection.

Construction:

- Map entity kinds to scene item kinds.
- Skip non-renderable entities only if explicitly marked.
- Preserve stable order.

Tests:

- `runtime3d_scene_projection_tests.cpp`.

## Save Files

### `engine/src/runtime3d/Runtime3DSaveEnvelope.hpp`

Purpose: define runtime3d save envelope shape before encoding format exists.

Owns:

- `struct Runtime3DSaveEnvelope`;
- version fields;
- package/scenario identity strings;
- lifecycle;
- clock state;
- camera mode state;
- world state;
- command log summary or full command vector.

Construction:

- Include runtime3d state headers, not file IO.
- Keep save data separated from active session mutation.

Storage cost:

- `O(world entities + command records + metadata)`.

Stop condition:

- No binary codec details here.

### `engine/src/runtime3d/Runtime3DSaveEnvelope.cpp`

Purpose: implementation home for save-envelope helpers.

Construction:

- Add default version helper if useful.
- Add compatibility helper only when load policy needs it.

### `engine/src/runtime3d/Runtime3DSaveLoad.hpp`

Purpose: define save/load conversion semantics between active session state and
save envelope.

Owns:

- `struct Runtime3DSaveResult`;
- `struct Runtime3DLoadInput`;
- `struct Runtime3DLoadResult`;
- `class Runtime3DSaveLoad`.

Construction:

- Save builds an envelope from state.
- Load validates envelope compatibility before returning replacement state.
- No filesystem in skeleton.

Stop condition:

- Do not reuse `RuntimeGameplaySaveSlotStore` directly until envelope semantics
  are proven.

### `engine/src/runtime3d/Runtime3DSaveLoad.cpp`

Purpose: implement in-memory save/load conversion.

Construction:

- Save returns envelope and status.
- Load checks version and package/scenario identity if input provides expected
  metadata.
- Return incompatible status without mutation.

Tests:

- `runtime3d_save_load_tests.cpp`.

## Legacy Adapter Files

### `engine/src/runtime3d/Runtime3DLegacy2DAdapter.hpp`

Purpose: temporary bridge from current 2D/product-loop demo state into
runtime3d state.

Owns:

- adapter input/result structs;
- `class Runtime3DLegacy2DAdapter`;
- placeholder status values.

Construction:

- Include old runtime headers only here, not across runtime3d.
- Keep this file visibly temporary.
- First result can be `NotImplemented` if source dependencies are not stable
  during the skeleton packet.

Rules:

- One-way bridge: 2D state to runtime3d state.
- No runtime3d code should depend on 2D adapters for normal operation.

### `engine/src/runtime3d/Runtime3DLegacy2DAdapter.cpp`

Purpose: implement bridge when phase reaches product-loop demo conversion.

Construction:

- Phase 0 skeleton may return placeholder status.
- Phase 3 implementation maps player, NPCs, floor, walls, pickups, doors, and
  demo package identity into `Runtime3DSessionState`.
- Document every copied 2D behavior in tests.

Tests:

- `runtime3d_legacy_2d_adapter_tests.cpp`.

## Session Files

### `engine/src/runtime3d/Runtime3DSessionState.hpp`

Purpose: top-level state for runtime3d play.

Owns:

- `enum class Runtime3DSessionLifecycle`;
- `struct Runtime3DSessionState`;

State fields:

- package identity string;
- scenario identity string;
- lifecycle;
- clock state;
- camera mode state;
- world state;
- command log vector;
- initial/retry snapshot marker or TODO field;
- current selected entity/point if needed.

Construction:

- Include `Runtime3DClock.hpp`, `Runtime3DCameraState.hpp`,
  `Runtime3DWorldState.hpp`, and `Runtime3DCommand.hpp`.
- Keep default lifecycle `NotLoaded`.
- Keep state serializable by shape.
- Avoid recursive full snapshot inside the struct until retry snapshot design is
  explicit; use a separate snapshot/envelope later.

Data ownership:

- Session owns world, clock, camera state, and command log.

### `engine/src/runtime3d/Runtime3DSessionState.cpp`

Purpose: implementation home for session-state helpers.

Owns:

- optional `bool IsPlayableLifecycle`;
- optional helper to create an empty loaded session.

Construction:

- Keep helpers pure.
- Do not load packages or touch filesystem.

### `engine/src/runtime3d/Runtime3DSession.hpp`

Purpose: session operation facade for lifecycle transitions.

Owns:

- `class Runtime3DSession`;
- methods such as `createEmpty`, `setLifecycle`, `applyClockDecision`, or
  `admitCommand` only when useful.

Construction:

- Include `Runtime3DSessionState.hpp`.
- Keep operations value-in/value-out to match current runtime style.
- Do not own native input or renderer integration.

### `engine/src/runtime3d/Runtime3DSession.cpp`

Purpose: implement session operations.

Construction:

- Start with `createEmptyLoadedSession` or equivalent.
- Later apply commands through command admission and world mutation.
- Keep reset/retry/load semantics explicit and test-covered.

Tests:

- `runtime3d_session_state_tests.cpp`.

## CMake Files

### `engine/cmake/iggy_runtime3d_sources.cmake`

Purpose: register runtime3d implementation files in `IGGY_ENGINE_SOURCES`.

Construction:

- Use repo style: `list(APPEND IGGY_ENGINE_SOURCES ...)`.
- List every runtime3d `.cpp` file.
- Do not list headers.
- Keep ordering primitive-to-session where possible.

Must include:

- `src/runtime3d/Runtime3DSessionState.cpp`
- `src/runtime3d/Runtime3DSession.cpp`
- `src/runtime3d/Runtime3DClock.cpp`
- `src/runtime3d/Runtime3DCommand.cpp`
- `src/runtime3d/Runtime3DCommandAdmission.cpp`
- `src/runtime3d/Runtime3DWorldState.cpp`
- `src/runtime3d/Runtime3DEntityState.cpp`
- `src/runtime3d/Runtime3DCameraState.cpp`
- `src/runtime3d/Runtime3DCameraModePolicy.cpp`
- `src/runtime3d/Runtime3DTargetQuery.cpp`
- `src/runtime3d/Runtime3DRayProjection.cpp`
- `src/runtime3d/Runtime3DSceneProjection.cpp`
- `src/runtime3d/Runtime3DSaveEnvelope.cpp`
- `src/runtime3d/Runtime3DSaveLoad.cpp`
- `src/runtime3d/Runtime3DLegacy2DAdapter.cpp`

### `engine/cmake/iggy_runtime3d_tests.cmake`

Purpose: register runtime3d tests.

Construction:

- Use `iggy_add_test`.
- Keep fixture compile definitions near tests that need fixtures.
- Do not register runtime3d tests in old runtime test file.

Initial tests:

- `runtime3d_session_state_tests`
- `runtime3d_clock_tests`
- `runtime3d_world_state_tests`
- `runtime3d_camera_mode_policy_tests`

Full planned tests:

- `runtime3d_command_admission_tests`
- `runtime3d_legacy_2d_adapter_tests`
- `runtime3d_ray_projection_tests`
- `runtime3d_target_query_tests`
- `runtime3d_scene_projection_tests`
- `runtime3d_save_load_tests`
- `runtime3d_acceptance_demo_tests`

### `engine/CMakeLists.txt`

Purpose: include runtime3d source and test registration.

Construction:

- Include `cmake/iggy_runtime3d_sources.cmake` after core sources.
- If dependencies require old runtime or scene before runtime3d, choose the
  compiling include order and document why in the builder summary.
- Under `IGGY_BUILD_TESTS`, set `IGGY_TEST_DEFAULT_LABELS runtime3d` and include
  `cmake/iggy_runtime3d_tests.cmake`.
- Restore or unset default labels as the surrounding CMake already does.

Stop condition:

- Do not reshuffle unrelated test registration.

### `engine/cmake/iggy_core_sources.cmake`

Purpose: register promoted core math `.cpp` files if created.

Construction:

- Add `src/core/math/Vec3.cpp`, `Ray3.cpp`, `Aabb3.cpp`, `Mat4.cpp`, and
  `Transform3.cpp` only if those files are created.
- Keep existing 2D math entries unchanged.

## Test Files

### `engine/tests/runtime3d_session_state_tests.cpp`

Purpose: prove default and minimal playable session state.

Construction:

- Use existing test style with simple `Expect` helper.
- Assert default `Runtime3DSessionState.lifecycle == NotLoaded`.
- Create or set a session to `PlayingRealtime`.
- Add a player entity to world.
- Assert entity exists and lifecycle remains playable.

No-go:

- Do not load packages or native app code.

### `engine/tests/runtime3d_clock_tests.cpp`

Purpose: prove clock decision semantics.

Construction:

- Normal returns `runAutomaticTick = true`.
- Slow returns `runAutomaticTick = true` and keeps/uses slow mode.
- Paused returns `runAutomaticTick = false`.
- StepRequested returns one tick decision, `consumedStepRequest = true`, and
  resulting mode `Paused`.

No-go:

- No sleeping, wall-clock calls, or SDL.

### `engine/tests/runtime3d_world_state_tests.cpp`

Purpose: prove entity collection behavior.

Construction:

- Add player entity with id 1.
- Find id 1.
- Upsert same id with new transform or kind.
- Assert only one entity exists for that id and fields changed.
- Assert invalid/missing id returns null or no result.

Compute expectation:

- Linear scan acceptable and documented.

### `engine/tests/runtime3d_camera_mode_policy_tests.cpp`

Purpose: prove real-time and tactical camera transitions.

Construction:

- Start in `RealtimeThirdPersonClose`.
- Normal keeps real-time camera.
- Switch to Slow and assert tactical camera.
- Return to Normal and assert previous real-time camera restored.
- Start in `RealtimeFirstPerson`, enter tactical, return to Normal, and assert
  FPS is restored.
- Assert changed-mode flag when mode changes.

No-go:

- No renderer matrices.

### `engine/tests/runtime3d_command_admission_tests.cpp`

Purpose: prove command validity and rejection semantics.

Construction:

- Actor command with invalid actor rejects.
- Actor command with existing actor admits.
- Interact command with missing target rejects.
- Session command such as Save or Reset can admit without actor if policy says
  so.
- Rejected command does not mutate world.

### `engine/tests/runtime3d_legacy_2d_adapter_tests.cpp`

Purpose: prove bridge behavior when adapter is implemented.

Construction:

- Skeleton can assert default adapter returns placeholder status.
- Phase 3 asserts product-loop demo maps player, floor, wall, door, pickup,
  and NPC to runtime3d entities.
- Test names must state copied semantics.

### `engine/tests/runtime3d_ray_projection_tests.cpp`

Purpose: prove ray-to-ground or ray-to-volume target candidates.

Construction:

- Ray from above toward ground hits expected point.
- Parallel ray returns no hit.
- Ray pointing away returns no hit.

### `engine/tests/runtime3d_target_query_tests.cpp`

Purpose: prove target discovery and tie behavior.

Construction:

- World with pickup and door returns first or nearest according to chosen rule.
- Actor is not returned as its own target.
- Non-targetable floor/wall ignored unless action allows it.

### `engine/tests/runtime3d_scene_projection_tests.cpp`

Purpose: prove renderer-facing scene facts derive from world state.

Construction:

- World with player, floor, wall, pickup produces stable scene items.
- Asset refs copy through.
- Selected/target flags copy through when available.
- Ordering is stable.

### `engine/tests/runtime3d_save_load_tests.cpp`

Purpose: prove in-memory save/load semantics before file IO.

Construction:

- Save a session with package/scenario identity, clock, camera, and world.
- Load compatible envelope into a replacement state.
- Incompatible version or package returns incompatible and does not produce an
  active replacement.

### `engine/tests/runtime3d_acceptance_demo_tests.cpp`

Purpose: prove the whole runtime3d loop once enough pieces exist.

Construction:

- Create/load demo state.
- Discover target.
- Reject out-of-reach interaction.
- Move/reposition into reach.
- Interact with pickup or door.
- Enter slow time and tactical camera.
- Issue tactical command.
- Pause/step/resume.
- Save/load.
- Retry/reset.
- Assert deterministic state summary.

## Later Native Integration Files

These files are not part of the first skeleton unless the builder explicitly
reaches native integration. They still need a start-to-finish plan now.

### `engine/apps/native_play/Native3DProductSession.hpp`

Purpose: native shell wrapper around `Runtime3DSessionState`.

Owns:

- app-local loaded runtime3d session;
- transient normalized input accumulation;
- native timing bridge;
- calls into runtime3d clock, camera mode, command admission, and scene
  projection.

Construction:

- Include runtime3d headers.
- Do not expose SDL events in runtime3d types.
- Own raw mouse deltas only long enough to normalize them.

### `engine/apps/native_play/Native3DProductSession.cpp`

Purpose: implement native-to-runtime3d session bridge.

Construction:

- Build session from legacy adapter first.
- Step clock according to native frame/tick timing.
- Submit command proposals to runtime3d.
- Clear transient input when camera mode changes.
- Produce scene projection result for renderer.

### `engine/apps/native_play/IggyNativePlay.cpp`

Purpose: wire runtime3d session into the native app loop.

Construction:

- Add mode or option to choose runtime3d path.
- Add mouse motion and relative mouse capture for FPS/third-person.
- Map tactical toggle, step tick, save/load, retry/reset.
- Keep renderer consuming view/projection and draw items only.

Stop condition:

- Do not put command admission or reach checks in this file.

### `engine/apps/native_play/NativeSceneDrawList.hpp`

Purpose: temporary renderer-facing draw item bridge.

Construction:

- Add adapter from `Runtime3DSceneProjectionResult` to native draw items.
- Preserve old path while runtime3d is behind a switch.
- Avoid gameplay inspection in renderer code.

### `engine/apps/native_play/NativeVulkanRenderer.hpp`

Purpose: renderer interface accepts runtime3d-derived camera matrices and draw
items.

Construction:

- Keep `viewProjection` as renderer input.
- Do not include runtime3d state if draw items are already native-facing.

### `engine/apps/native_play/NativeVulkanRenderer.cpp`

Purpose: draw runtime3d-projected scene items.

Construction:

- Add model-slot handling only after scene projection emits stable item kinds.
- Keep GPU resources and fallback mesh policy renderer-owned.

### `engine/cmake/iggy_native_play.cmake`

Purpose: register native runtime3d integration sources when added.

Construction:

- Add `Native3DProductSession.cpp`.
- Keep Vulkan/SDL/glslc gating unchanged.

## Verification Commands

For skeleton packets:

```sh
cmake --build <existing-build-dir> --target runtime3d_session_state_tests
cmake --build <existing-build-dir> --target runtime3d_clock_tests
cmake --build <existing-build-dir> --target runtime3d_world_state_tests
cmake --build <existing-build-dir> --target runtime3d_camera_mode_policy_tests
ctest --test-dir <existing-build-dir> -R "runtime3d_"
```

If no build directory exists, configure one according to current repo practice
and then run the same targets.

## Builder Completion Definition

A file is complete for its phase only when:

- it compiles;
- it has a matching test or is covered by a named integration test;
- its ownership matches this document;
- it does not pull native app, renderer, SDL, Qt, or Vulkan into runtime3d;
- it states placeholder status where behavior is intentionally deferred;
- it is registered in the right CMake file;
- it leaves old 2D runtime behavior unchanged.
