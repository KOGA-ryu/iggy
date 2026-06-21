# `docs/roadmap.md`

Updated: 2026-06-20

Exact purpose: define the complete build sequence for `iggy3d` from empty standalone repo to shippable headless runtime demo, then to renderer integration readiness.

This roadmap is not a starter plan. It is the build order for the full runtime surface described in `COMPLETE_BUILD_SURFACE.md` and `PRIORITY.md`.

## Ground Rules

- Build in `/Users/kogaryu/iggy3d`.
- Do not import old `/Users/kogaryu/iggy` code.
- Keep headless runtime acceptance ahead of renderer integration.
- Every phase ends with tests or a tool command that proves the phase.
- Every gameplay mutation goes through command/session/system ownership.
- Every runtime-facing file must have a matching file plan before implementation.

## Phase 0: Repo Contract And Build Shell

Files:

- `docs/architecture.md`
- `docs/ownership.md`
- `docs/roadmap.md`
- `docs/acceptance_demo.md`
- `CMakeLists.txt`
- `cmake/iggy3d_options.cmake`
- `cmake/iggy3d_warnings.cmake`
- `cmake/iggy3d_tests.cmake`

Build work:

1. create standalone repo tree;
2. define project `iggy3d` and library target `iggy3d`;
3. define options for tests/tools/warnings;
4. register empty test/app scaffolds only when linked to owned source files;
5. add no-legacy include/link scan to review checklist or CMake failure if practical.

Exit gate:

```sh
cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build -DIGGY3D_BUILD_TESTS=ON
cmake --build /Users/kogaryu/iggy3d/build
```

## Phase 1: Core Deterministic Primitives

Files: result, diagnostics, ids, stable hash, Vec3, Ray3, Aabb3, Plane, Mat4, Transform3, and math tests.

Build work:

1. implement value types with no runtime dependencies;
2. define coordinate system X right, Y up, Z forward;
3. define stable id and hash policies;
4. add math and hash tests;
5. confirm all primitives are deterministic and finite-value guarded.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R math --output-on-failure
```

## Phase 2: Content, Config, And Fixture Seed

Files: runtime config, app config, CLI parser, package manifest/loader/validator, first-room package/scenario fixtures, fixture scenario loader, package tests.

Build work:

1. define first-party fixture schema;
2. load `package.iggy3d.toml` and `scenario.iggy3d.toml`;
3. validate required fields, versions, unique names/ids, and scenario references;
4. produce deterministic seed records for session creation;
5. prove invalid package diagnostics.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R package --output-on-failure
```

## Phase 3: Player, World, And Reset Baseline

Files: player slot/roster, entity state, world state, world tests.

Build work:

1. allocate stable entity ids from fixture order;
2. bind local player slot 0 to player entity;
3. store transforms, bounds, active flags, interaction metadata, item metadata, health, and objective references;
4. expose world mutation helpers;
5. store baseline seed/copy needed for reset.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R world --output-on-failure
```

## Phase 4: Clock, Camera, Command, Session Base

Files: clock, camera, command, command admission, command log, session state/session facade, related tests.

Build work:

1. define normal, slow, paused, and step clock semantics;
2. define first-person, third-person, tactical orbit, and tactical overhead camera semantics;
3. define command kinds including move, interact, inspect, wait, tactical toggle, pause, resume, step, retry, reset, save, load;
4. implement authority/admission read-only validation;
5. log accepted and rejected commands;
6. create session facade with reset and state access.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R "clock|camera|command|session" --output-on-failure
```

## Phase 5: Playable Gameplay Systems

Files: movement, targeting, reach, interaction, inventory, combat, AI, objective, and their tests.

Build work:

1. implement deterministic movement to explicit target points;
2. discover targets by stable entity scan;
3. gate interaction by reach from actor transform position to target entity
   transform position;
4. implement pickup/activate/inspect effects;
5. mutate inventory and objectives through owning systems;
6. implement deterministic combat and AI proposal shape even if first-room demo uses only minimal combat/AI;
7. prove retry succeeds after movement brings actor into reach.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R "movement|target|interaction|inventory|combat|ai|objective" --output-on-failure
```

## Phase 6: Full Tick, Diagnostics, And Projection

Files: runtime events, metrics, session tick, session runner, scene/debug projection, runtime summary, projection tests.

Build work:

1. implement fixed tick order;
2. emit stable runtime events;
3. collect deterministic counters;
4. generate read-only scene items and debug projection;
5. generate deterministic runtime summary;
6. prove projection does not mutate runtime.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R projection --output-on-failure
```

## Phase 7: Durability, Replay, And Multiplayer-Ready Authority

Files: save envelope, save codec, compatibility, save/load, state hash, command replay, authority, replication packets/codecs, local multiplayer session, related tests.

Build work:

1. map `SessionState` to `SaveEnvelope`;
2. encode/decode deterministic save text;
3. reject incompatible loads without mutation;
4. define canonical state hash;
5. replay command log through normal session path;
6. define authority modes and local multiplayer slot ordering;
7. round-trip replication packets without network sockets.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R "save|replay|multiplayer" --output-on-failure
```

## Phase 8: Product Proof Tools And Acceptance Demo

Files: expected summary, acceptance demo doc, headless demo app, replay tool app, complete runtime acceptance test. The validate package app remains a planned Phase 8 target until `apps/iggy3d_validate_package/main.cpp` and CMake registration exist.

Build work:

1. implement exact first-room command script;
2. prove target discovery, out-of-range rejection, movement, retry, interaction, objective completion;
3. prove slow-time tactical camera, pause, step, and resume;
4. save/load into a fresh session and compare hash;
5. reset branch to baseline and compare baseline hash;
6. replay command history and compare final hash;
7. print deterministic summary and compare to fixture.

Exit gate:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
/Users/kogaryu/iggy3d/build/iggy3d_headless_demo --package /Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml --summary /Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt --save /tmp/iggy3d_first_room_runtime.save
/Users/kogaryu/iggy3d/build/iggy3d_replay_tool --package /Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml --save /tmp/iggy3d_first_room_runtime.save --expect-summary /Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt
```

Planned but not currently runnable from this gate: `iggy3d_validate_package`, pending `apps/iggy3d_validate_package/main.cpp` and CMake target registration.

## Renderer Integration Readiness

Renderer work has two gates:

1. Backend-neutral renderer boundary, null renderer, and visual app/platform shell may start after Phase 6 projection is green. These map to Vulkan Packet 1, Packet 2, and Packet 3. Projection is required because `FrameInput` borrows `SceneProjectionResult*` and `DebugProjectionResult*`.
2. Final visible renderer acceptance waits until Phase 8 product proof is green.

`CommandReplay` is not a prerequisite for Vulkan Packet 1, Packet 2, or Packet 3. Renderer replay-invariance tests mean renderer submission must not mutate runtime hash, runtime state, command results, or replay truth. They do not require the replay tool to exist before backend-neutral renderer work starts.

Runtime must expose projection data that a future renderer can consume without owning gameplay state. First visual integration should consume `SceneProjection`, `DebugProjection`, and `CameraState` only. Vulkan backend/bootstrap starts later and follows the Vulkan file-plan packet ladder after the Packet 1, Packet 2, and Packet 3 contracts are reviewed.
