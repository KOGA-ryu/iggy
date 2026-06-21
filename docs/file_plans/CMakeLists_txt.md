# `CMakeLists.txt`

Updated: 2026-06-20

Exact purpose: define the standalone `iggy3d` CMake project, library, apps, fixtures, and test entry points without old `iggy` dependencies.

## Build Position

- priority rank: 5
- tier: Tier 0: Repo Contract And Build Shell
- module: `build system`
- file kind: `build`

## Ownership

This file owns:

- project name/version
- C++ standard
- library target `iggy3d`
- app targets
- test enablement
- explicit local helper inclusion

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- CMake only
- owned source/test paths in `/Users/kogaryu/iggy3d`
- no old iggy targets or include directories

## Data Contract

- requires C++20 or later
- adds `src` include directory only for this repo
- includes local cmake helper files
- registers apps and tests when options allow

## Semantics

- configure must fail if an old iggy include path/target is required
- library sources are explicit and grouped by module
- fixtures are referenced by tests/apps as source tree data

## Implementation Plan

1. create target/config behavior explicitly;
2. fail early when required owned files are missing;
3. keep old iggy paths out of include/link lists;
4. make test/tool options visible and documented.

## Compute Cost

- Configure cost is O(number of explicitly listed source/test files).
- Build cost is compiler driven and outside this planning file's runtime
  authority.

## Diagnostics And Errors

- configure failures use `message(FATAL_ERROR "...")` with the missing file,
  forbidden target, or forbidden path named in the message;
- CMake status output is informational only and must not mask failed
  registration;
- runtime `Diagnostic` types are not used by CMake files.

## Save Replay Multiplayer Notes

- CMake files are build configuration only and are not save truth.
- They do not affect replay determinism except by controlling which source files
  build into the local targets.
- They must not introduce multiplayer, socket, renderer, or old iggy
  dependencies.

## Tests And Verification

- configure from a clean build directory must succeed with default options;
- `cmake --build` must build `iggy3d`, tools, and registered tests;
- `ctest -N` must list registered tests in the order from
  `cmake/iggy3d_tests.cmake`;
- no generated target metadata may contain `/Users/kogaryu/iggy`.

## Completion Criteria

- `CMakeLists.txt` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Build Shape
- `cmake_minimum_required(VERSION 3.20)` or newer.
- `project(iggy3d VERSION 0.1.0 LANGUAGES CXX)`.
- Set `CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, and do not use compiler extensions.
- Include local helper files in this order:
  - `cmake/iggy3d_options.cmake`
  - `cmake/iggy3d_warnings.cmake`
  - `cmake/iggy3d_tests.cmake` after `enable_testing()` when tests are enabled.
- Define one library target named `iggy3d`.
- Define app targets only when `IGGY3D_BUILD_TOOLS` is ON:
  - `iggy3d_headless_demo`
  - `iggy3d_validate_package`
  - `iggy3d_replay_tool`.
- Enable CTest and add unit/acceptance tests only when `IGGY3D_BUILD_TESTS` is ON.
- Do not define install/export rules in the first complete build.

### Source Registration
Production sources for the `iggy3d` library must be listed explicitly in this
deterministic first-build order. Do not use broad recursive globbing for
production sources. Builders must see new files in code review.

```cmake
target_sources(iggy3d
  PRIVATE
    src/core/hash/StableHash.cpp
    src/core/math/Vec3.cpp
    src/core/math/Transform3.cpp
    src/core/math/Aabb3.cpp
    src/core/math/Ray3.cpp
    src/core/math/Plane.cpp
    src/core/math/Mat4.cpp
    src/app/AppConfig.cpp
    src/app/CliParser.cpp
    src/content/PackageLoader.cpp
    src/content/PackageValidator.cpp
    src/content/FixtureScenarioLoader.cpp
    src/runtime/player/PlayerRoster.cpp
    src/runtime/world/WorldState.cpp
    src/runtime/clock/Clock.cpp
    src/runtime/camera/CameraModePolicy.cpp
    src/runtime/command/CommandAdmission.cpp
    src/runtime/replay/CommandLog.cpp
    src/runtime/session/Session.cpp
    src/runtime/movement/MovementSystem.cpp
    src/runtime/targeting/TargetQuery.cpp
    src/runtime/targeting/ReachQuery.cpp
    src/runtime/inventory/InventorySystem.cpp
    src/runtime/interaction/InteractionSystem.cpp
    src/runtime/combat/CombatSystem.cpp
    src/runtime/ai/AiSystem.cpp
    src/runtime/objective/ObjectiveSystem.cpp
    src/runtime/diagnostics/RuntimeMetrics.cpp
    src/runtime/session/SessionTick.cpp
    src/runtime/session/SessionRunner.cpp
    src/projection/scene/SceneProjection.cpp
    src/projection/debug/DebugProjection.cpp
    src/runtime/diagnostics/RuntimeSummary.cpp
    src/runtime/save/SaveCompatibility.cpp
    src/runtime/save/SaveCodec.cpp
    src/runtime/save/SaveLoad.cpp
    src/runtime/replay/StateHash.cpp
    src/runtime/replay/CommandReplay.cpp
    src/runtime/multiplayer/Authority.cpp
    src/runtime/multiplayer/ReplicationCodec.cpp
    src/runtime/multiplayer/LocalMultiplayerSession.cpp
)
```

Header-only first-build plans are intentionally not listed in `target_sources`.
They include core result/diagnostics/ids, `RuntimeConfig`, package manifest,
player slot, entity state, clock/camera state, command types, session state,
movement command, interaction definition, inventory/combat/AI/objective state,
save envelope, replay/multiplayer packet headers, runtime events, scene item,
and projection/debug headers. App entry points under `apps/` are not part of the
`iggy3d` library; they are registered on their tool targets.

### Include And Link Rules
- Public include directory: `${CMAKE_CURRENT_SOURCE_DIR}/src` only.
- No include directory may contain `/Users/kogaryu/iggy`.
- No target may link `iggy_engine`, old native play, Qt, SDL, Vulkan, renderer libraries, or network libraries.
- `iggy3d` is the only production library target in the first build.

### Diagnostics And Failure Behavior
Configure must fail with a clear CMake error when:
- a required local helper file is missing;
- a required app source is missing while tools are ON;
- an old iggy include path or link target is introduced;
- tests are ON but local test helper registration is missing.

### Tests
CMake behavior is validated indirectly by:
- configure/build from a clean `build/` directory;
- all CTest tests registered through `cmake/iggy3d_tests.cmake`;
- acceptance tool targets present when tools are enabled.

### Completion Criteria
A builder can run the three canonical commands from `docs/architecture.md` and see no old-iggy target, include path, or linked dependency in generated target metadata.
