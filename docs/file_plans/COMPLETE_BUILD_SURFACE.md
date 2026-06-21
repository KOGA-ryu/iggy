# iggy3d Complete Build Surface

Updated: 2026-06-20

Exact purpose: define what "complete runtime" means for `iggy3d`. This is the anti-starter document. If a system is required for a finished runtime, it must appear here and have a file plan in this directory.

## Complete Means

Complete means the standalone `/Users/kogaryu/iggy3d` repo can build, test, run, save, load, reset, replay, and validate a deterministic tactical/realtime gameplay loop without old `iggy` dependencies.

Complete does not mean "renderer finished." Rendering is a downstream consumer. The runtime is complete when gameplay truth, command flow, durability, acceptance proof, and renderer-facing projection are all owned and tested.

## Hard Product Requirements

- single-player first, with player slot and authority shape that can support multiplayer;
- realtime play with first-person or close third-person semantic camera;
- slow-time/planning mode that switches to tactical camera;
- target discovery;
- reach-gated command admission and interaction execution;
- pause, step, retry, reset;
- minimal durable save/load;
- deterministic command replay and state hash;
- acceptance demo with exact summary output;
- no old legacy build dependency.

## Required Build Surface

- standalone `CMakeLists.txt`;
- local CMake options, warnings, and test helpers;
- library target `iggy3d`;
- current app targets `iggy3d_headless_demo` and `iggy3d_replay_tool`;
- planned Phase 8 app target `iggy3d_validate_package`, pending `apps/iggy3d_validate_package/main.cpp` and CMake registration;
- unit and acceptance tests registered through CTest;
- fixture files under `fixtures/demos/first_room`.

Forbidden build inputs:

- `iggy_engine`;
- old `/Users/kogaryu/iggy` include paths;
- old native play targets;
- old scene/runtime CMake files;
- old 2D adapters.

## Required Runtime Truth

Authoritative runtime truth must include:

- session lifecycle and baseline reset seed;
- world and entity state;
- player roster and player-to-actor bindings;
- clock mode, slow-time, pause, and step state;
- semantic camera state and previous realtime camera memory;
- command log with accepted and rejected records;
- inventory, combat, AI, and objective state;
- save envelope state;
- replay/state-hash state.

Derived data excluded from save truth:

- scene projection;
- debug projection;
- app CLI config;
- renderer handles;
- raw input events;
- transient diagnostics unless a debug save section is explicitly added.

## Required Runtime Execution Surface

The finished runtime must execute this deterministic loop:

1. load and validate package/fixture data;
2. create session from deterministic scenario seed;
3. bind player slot 0 to player actor;
4. accept command proposals from app or AI;
5. apply authority checks;
6. apply command admission checks;
7. append accepted and rejected records to command log;
8. update clock/camera state for control commands;
9. execute movement, interactions, combat, AI proposals, and objective evaluation in fixed order;
10. emit runtime events/metrics;
11. update deterministic state hash and summary;
12. project read-only scene/debug data.

Rejected commands must not mutate gameplay state. Retry must re-evaluate the referenced rejected command under current state. Reset must restore the validated baseline fixture state through session-owned reset logic.

## Required Acceptance Flow

The complete acceptance proof must demonstrate:

1. package and scenario validation;
2. session creation;
3. default realtime third-person camera, with first-person supported by config;
4. target discovery of `gold_key`;
5. out-of-range interact rejected with `OutOfRange`;
6. movement into reach;
7. retry of the rejected interaction succeeds;
8. inventory gains `gold_key` and objective completes;
9. slow-time tactical camera transition;
10. tactical movement command;
11. pause, single step, and resume;
12. save envelope creation;
13. load into a fresh session with same hash;
14. reset branch restores fixture baseline;
15. command replay reaches final expected hash;
16. deterministic final summary matches `expected_summary.txt`.

## Required File Surface

The complete file surface is every document listed in `INDEX.md` and ranked in `PRIORITY.md`:

- project docs: architecture, ownership, roadmap, acceptance;
- build files: CMake root and helpers;
- core: result, diagnostics, ids, hash, math;
- content: package manifest, loader, validator, fixture loader, fixtures;
- runtime: player, world, clock, camera, command, session, movement, targeting, interaction, inventory, combat, AI, objective, save, replay, multiplayer, diagnostics;
- projection: scene and debug projection;
- apps: headless demo, package validator, replay tool;
- tests: unit coverage for every subsystem and one complete acceptance test.

## Completion Gate

The runtime is complete only when all of these pass in `/Users/kogaryu/iggy3d`:

```sh
cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build -DIGGY3D_BUILD_TESTS=ON
cmake --build /Users/kogaryu/iggy3d/build
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
/Users/kogaryu/iggy3d/build/iggy3d_headless_demo --package /Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml --summary /Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt --save /tmp/iggy3d_first_room_runtime.save
/Users/kogaryu/iggy3d/build/iggy3d_replay_tool --package /Users/kogaryu/iggy3d/fixtures/demos/first_room/package.iggy3d.toml --save /tmp/iggy3d_first_room_runtime.save --expect-summary /Users/kogaryu/iggy3d/fixtures/demos/first_room/expected_summary.txt
```

`iggy3d_validate_package` is planned, but it is not part of the current runnable gate until its app source and CMake target exist.

No target may include or link old `/Users/kogaryu/iggy` code at this gate.
