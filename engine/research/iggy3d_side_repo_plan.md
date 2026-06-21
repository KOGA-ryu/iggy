# iggy3d Side Repo Plan

Updated: 2026-06-20

Canonical repo name: `iggy3d`.

Canonical local path:

```text
/Users/kogaryu/iggy3d
```

This supersedes the in-repo `engine/src/runtime3d/` plan for new 3D runtime
implementation. The old `iggy` repo remains reference material only. `iggy3d`
must not require the old legacy build, old runtime, old scene modules, old CMake
files, or old native play shell.

## Product Rule

`iggy3d` is a fresh standalone 3D runtime/gameplay repo.

It may read old `iggy` docs and source for ideas, but it does not include,
link, or build against old `iggy` code.

## Hard No-Dependency Rules

`iggy3d` must not:

- include headers from `/Users/kogaryu/iggy/engine/src/runtime`;
- include headers from `/Users/kogaryu/iggy/engine/src/scene`;
- include headers from `/Users/kogaryu/iggy/engine/apps/native_play`;
- link `iggy_engine`;
- include old `iggy` CMake files;
- depend on the old 2D product loop;
- depend on the old 2D target, reach, inventory, camera, save, or renderer
  systems;
- persist raw input events or renderer state;
- treat old 2D exports as authoritative game state.

## Allowed Use Of Old `iggy`

Allowed:

- read docs for prior decisions;
- read source for naming or cautionary examples;
- manually copy small pure math ideas only when they are renamed and owned by
  `iggy3d`;
- import neutral fixture data only after it is written as explicit `iggy3d`
  fixture data.

Not allowed:

- adapter layer from old 2D state;
- include-path bridge into old repo;
- old runtime tests as `iggy3d` tests;
- old native renderer as the first app dependency.

## Repo Shape

```text
iggy3d/
  CMakeLists.txt
  cmake/
    iggy3d_options.cmake
    iggy3d_warnings.cmake
    iggy3d_tests.cmake
  docs/
    architecture.md
    ownership.md
    roadmap.md
    acceptance_demo.md
    file_plans/
  src/
    app/
    config/
    content/
    core/
      diagnostics/
      hash/
      ids/
      math/
      result/
    runtime/
      ai/
      camera/
      clock/
      combat/
      command/
      diagnostics/
      interaction/
      inventory/
      movement/
      multiplayer/
      objective/
      player/
      replay/
      save/
      session/
      targeting/
      world/
    projection/
      debug/
      scene/
  tests/
    unit/
    acceptance/
  fixtures/
    demos/
      first_room/
  apps/
    iggy3d_headless_demo/
    iggy3d_validate_package/
    iggy3d_replay_tool/
```

## Build Target Names

Library target:

```text
iggy3d
```

Required executables:

```text
iggy3d_headless_demo
iggy3d_validate_package
iggy3d_replay_tool
```

Required CMake options:

```text
IGGY3D_BUILD_TESTS
IGGY3D_BUILD_TOOLS
IGGY3D_WARNINGS_AS_ERRORS
```

Initial build commands:

```sh
cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build -DIGGY3D_BUILD_TESTS=ON
cmake --build /Users/kogaryu/iggy3d/build
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
```

## Complete Implementation Surface

Start with standalone repo files, not old `engine/src/runtime3d` paths.

The complete one-file-one-document construction contracts are tracked in
`engine/research/iggy3d_file_plans/INDEX.md`, with complete-build scope in
`engine/research/iggy3d_file_plans/COMPLETE_BUILD_SURFACE.md`.
Build order is tracked in
`engine/research/iggy3d_file_plans/PRIORITY.md`.

Do not treat any smaller list as the implementation scope. The complete scope is
137 planned repo files, plus the index and priority documents that rank them.

The implementation surface includes:

- project docs;
- build system files;
- core result/diagnostic/id/hash/math primitives;
- config and app support;
- content package loading, validation, fixture scenario loading, and first-room
  fixture files;
- player/world state;
- clock/camera command/session runtime loop;
- movement, targeting, reach, interaction, inventory, combat, AI, and objective
  systems;
- save/load, command replay, state hash, authority, replication packet/codecs,
  and local multiplayer coordination;
- runtime diagnostics, metrics, and deterministic summary;
- read-only scene/debug projection;
- headless demo, package validator, replay tool;
- unit tests for every subsystem and one complete acceptance test.

## Removed From Fresh Start

Do not build these concepts in `iggy3d`:

- `Runtime3DLegacy2DAdapter`;
- old product-loop adapter;
- old 2D scene adapter;
- old native play adapter;
- old save-slot bridge;
- old render-command bridge.

If old demo behavior is still useful, recreate it as:

```text
fixtures/demos/first_room/
```

with explicit `iggy3d` fixture files.

## Naming

Public repo/project name: `iggy3d`.

Namespace options:

- preferred for C++: `iggy3d`;
- acceptable if style demands nesting later: `iggy::three_d`;
- avoid: `runtime3d` as the top-level namespace.

File/class names should drop the long `Runtime3D` prefix where the repo context
already owns 3D. For example:

- `SessionState`
- `WorldState`
- `EntityState`
- `Clock`
- `CameraModePolicy`
- `CommandAdmission`
- `SceneProjection`

Use `3D` only where it disambiguates from a general concept inside the same
repo.

## First Acceptance Demo

The first demo is headless and deterministic.

It must prove:

- session creation;
- player entity creation;
- target discovery;
- out-of-range command rejection;
- movement into reach;
- retry of the rejected interaction;
- in-range interaction acceptance and inventory/objective mutation;
- real-time camera mode;
- slow-time tactical camera mode;
- pause, step, and resume;
- save/load envelope conversion;
- reset to fixture baseline;
- deterministic command replay;
- deterministic final state summary.

Renderer integration comes after the headless loop is proven.
