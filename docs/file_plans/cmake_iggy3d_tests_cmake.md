# `cmake/iggy3d_tests.cmake`

Updated: 2026-06-20

Exact purpose: declare the test harness helpers and register every unit and acceptance executable.

## Build Position

- priority rank: 8
- tier: Tier 0: Repo Contract And Build Shell
- module: `build system`
- file kind: `build`

## Ownership

This file owns:

- test helper function
- test source list registration
- fixture path definitions
- CTest labels

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

- unit tests label `unit`
- acceptance tests label `acceptance`
- tests link only `iggy3d` and approved test harness code

## Semantics

- test registration is explicit so missing files are configure failures
- acceptance test gets fixture path from CMake definition or argv

## Implementation Plan

1. create target/config behavior explicitly;
2. fail early when required owned files are missing;
3. keep old iggy paths out of include/link lists;
4. make test/tool options visible and documented.

## Compute Cost

- O(number of registered tests) at configure time.
- Test runtime is owned by each test executable's plan.

## Diagnostics And Errors

- missing test source files fail configure with `message(FATAL_ERROR ...)`;
- test registration must name the missing source path or duplicate test target;
- runtime `Diagnostic` types are not used by this CMake helper.

## Save Replay Multiplayer Notes

- CTest registration is not save truth and is not replay input.
- Acceptance tests execute replay/session tools through their own test bodies;
  this helper only registers executables and labels.
- Test helpers must not introduce renderer, wall-clock sleep, network service, or
  old iggy dependencies.

## Tests And Verification

- `ctest -N` must list tests in deterministic registration order;
- each test executable links only `iggy3d` plus standard test-local code;
- every test has labels including `iggy3d`.

## Completion Criteria

- `cmake/iggy3d_tests.cmake` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Exact Purpose
Own CTest registration helpers for `iggy3d` unit and acceptance tests.

### Required API
Provide these helpers:

```cmake
iggy3d_add_unit_test(test_name source_file)
iggy3d_add_acceptance_test(test_name source_file)
```

Both helpers must:
- create an executable target;
- link only `iggy3d` and standard test-local dependencies;
- apply warning policy;
- register with CTest using the executable target path;
- add deterministic labels.

### Labels
- Unit tests get labels:
  - `unit`
  - module label supplied by the caller
  - `iggy3d`
- Acceptance tests get labels:
  - `acceptance`
  - `runtime`
  - `iggy3d`

### Complete Test Registration Table

Register every test file plan in this deterministic CTest order:

| Order | Target | Source path | Labels |
| --- | --- | --- | --- |
| 1 | `math_tests` | `tests/unit/math_tests.cpp` | `unit;core;iggy3d` |
| 2 | `package_loader_tests` | `tests/unit/package_loader_tests.cpp` | `unit;content;iggy3d` |
| 3 | `world_state_tests` | `tests/unit/world_state_tests.cpp` | `unit;runtime;world;iggy3d` |
| 4 | `clock_tests` | `tests/unit/clock_tests.cpp` | `unit;runtime;clock;iggy3d` |
| 5 | `camera_mode_policy_tests` | `tests/unit/camera_mode_policy_tests.cpp` | `unit;runtime;camera;iggy3d` |
| 6 | `command_admission_tests` | `tests/unit/command_admission_tests.cpp` | `unit;runtime;command;iggy3d` |
| 7 | `session_state_tests` | `tests/unit/session_state_tests.cpp` | `unit;runtime;session;iggy3d` |
| 8 | `movement_system_tests` | `tests/unit/movement_system_tests.cpp` | `unit;runtime;movement;iggy3d` |
| 9 | `target_reach_tests` | `tests/unit/target_reach_tests.cpp` | `unit;runtime;targeting;iggy3d` |
| 10 | `inventory_system_tests` | `tests/unit/inventory_system_tests.cpp` | `unit;runtime;inventory;iggy3d` |
| 11 | `interaction_system_tests` | `tests/unit/interaction_system_tests.cpp` | `unit;runtime;interaction;iggy3d` |
| 12 | `combat_system_tests` | `tests/unit/combat_system_tests.cpp` | `unit;runtime;combat;iggy3d` |
| 13 | `ai_system_tests` | `tests/unit/ai_system_tests.cpp` | `unit;runtime;ai;iggy3d` |
| 14 | `objective_system_tests` | `tests/unit/objective_system_tests.cpp` | `unit;runtime;objective;iggy3d` |
| 15 | `projection_tests` | `tests/unit/projection_tests.cpp` | `unit;projection;iggy3d` |
| 16 | `save_load_tests` | `tests/unit/save_load_tests.cpp` | `unit;runtime;save;iggy3d` |
| 17 | `replay_state_hash_tests` | `tests/unit/replay_state_hash_tests.cpp` | `unit;runtime;replay;iggy3d` |
| 18 | `multiplayer_authority_tests` | `tests/unit/multiplayer_authority_tests.cpp` | `unit;runtime;multiplayer;iggy3d` |
| 19 | `complete_runtime_demo_tests` | `tests/acceptance/complete_runtime_demo_tests.cpp` (`tests_acceptance_complete_runtime_demo_tests_cpp.md`) | `acceptance;runtime;iggy3d` |

When `IGGY3D_BUILD_TOOLS=OFF`, unit tests remain registered. Acceptance tests
that execute app targets are skipped at configure time with one `STATUS` line
that names each skipped acceptance target.

### Forbidden Behavior
- No renderer requirement.
- No wall-clock sleeps.
- No network service.
- No old iggy executable or fixture path.

### Completion Criteria
`ctest -N` from the build directory lists every target in the complete table
above in deterministic order, except acceptance tests skipped because
`IGGY3D_BUILD_TOOLS=OFF`.
