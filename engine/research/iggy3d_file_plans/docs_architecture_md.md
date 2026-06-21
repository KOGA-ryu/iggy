# `docs/architecture.md`

Updated: 2026-06-20

Exact purpose: define the standalone architecture of `iggy3d` so builders can
construct a complete runtime without importing old `iggy` code, old 2D
semantics, or renderer-owned gameplay behavior.

## Authority Of This Document

This is the first project document every builder must read. It defines
architectural law for the standalone `/Users/kogaryu/iggy3d` repo.

Individual file plans define local implementation shape, but they cannot
reverse:

- dependency direction;
- data ownership;
- runtime mutation rules;
- save/replay boundaries;
- command admission rules;
- headless-first acceptance order;
- no-legacy dependency rules.

If implementation work discovers a real architectural conflict, this document
must be updated before code works around the conflict.

## Product Target

`iggy3d` is a standalone 3D runtime/gameplay engine for a real-time tactical
game that can slow into planning/tactical play.

The first complete product proof is headless and deterministic. Renderer work
is allowed only after the headless loop proves:

- package and scenario load;
- session creation;
- target discovery;
- out-of-range rejection;
- movement into reach;
- retry of rejected command;
- interaction execution;
- inventory and objective mutation;
- realtime camera mode;
- slow-time tactical camera mode;
- pause, step, and resume;
- reset to baseline;
- save/load round-trip;
- command replay;
- deterministic state hash and summary.

## Standalone Repo Rule

Canonical repo path:

```text
/Users/kogaryu/iggy3d
```

`iggy3d` may read `/Users/kogaryu/iggy` as historical reference only.

Forbidden:

- include headers from old `/Users/kogaryu/iggy`;
- link old `iggy` libraries or CMake targets;
- import old CMake helper files;
- wrap old 2D runtime state;
- adapt old scene modules;
- use old native play shell code as a dependency;
- make old 2D save, target, reach, inventory, camera, or renderer behavior the
  source of truth.

Allowed:

- manually copy a small pure idea only after it becomes owned `iggy3d` code;
- read old docs/source for naming or cautionary examples;
- recreate useful demo behavior as explicit `iggy3d` fixture data.

## Build Outputs

Required library target:

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

Required verification commands:

```sh
cmake -S /Users/kogaryu/iggy3d -B /Users/kogaryu/iggy3d/build -DIGGY3D_BUILD_TESTS=ON
cmake --build /Users/kogaryu/iggy3d/build
ctest --test-dir /Users/kogaryu/iggy3d/build --output-on-failure
```

## Module Dependency Direction

Allowed dependency direction:

```text
core
  <- config
  <- content
  <- runtime
  <- projection
  <- app/tools
  <- tests
```

Meaning:

- `core` depends on the C++ standard library only unless a future dependency is
  explicitly approved in the build docs.
- `config` may depend on core value types.
- `content` may depend on core diagnostics/result/id/math helpers.
- `runtime` may depend on core, config, and content seed types only at session
  creation boundaries.
- `projection` may depend on core and runtime read-only state.
- apps/tools may depend on public library modules.
- tests may depend on public library modules and fixtures.

Forbidden dependency direction:

- `core` depending on any other `iggy3d` module;
- `runtime` depending on projection, app, tools, tests, renderer, or old `iggy`;
- `projection` depending on app, tools, renderer API, tests, or old `iggy`;
- app code validating gameplay legality directly;
- content code mutating an active session after creation;
- any production target depending on tests.

## Directory Architecture

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
  src/
    app/
      AppConfig.hpp
      AppConfig.cpp
      CliParser.hpp
      CliParser.cpp
    config/
      RuntimeConfig.hpp
    content/
      PackageManifest.hpp
      PackageLoader.hpp
      PackageLoader.cpp
      PackageValidator.hpp
      PackageValidator.cpp
      FixtureScenarioLoader.hpp
      FixtureScenarioLoader.cpp
    core/
      diagnostics/
      hash/
      ids/
      math/
      result/
    projection/
      debug/
      scene/
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
  fixtures/
    demos/
      first_room/
        package.iggy3d.toml
        scenario.iggy3d.toml
        expected_summary.txt
  tests/
    unit/
    acceptance/
  apps/
    iggy3d_headless_demo/
    iggy3d_validate_package/
    iggy3d_replay_tool/
```

## Runtime Data Categories

Every runtime value must fit one of these categories.

### Authoritative State

Authoritative state is the single source of gameplay truth:

- `SessionState`;
- `WorldState`;
- `EntityState`;
- `PlayerRoster`;
- `ClockState`;
- `CameraState`;
- `CommandLog`;
- inventory state;
- combat state;
- AI state;
- objective state.

Only owning runtime systems may mutate these values.

### Seed Data

Seed data comes from validated package/fixture content:

- package manifest;
- scenario seed entities;
- scenario objectives;
- default player slot binding;
- baseline reset state.

Seed data is read during session creation. After session creation, runtime owns
its own copy of all gameplay truth.

### Durable State

Durable state is what save/load persists:

- package and scenario ids;
- session lifecycle fields needed to resume;
- world/entity state;
- player roster;
- clock and camera state;
- command log;
- inventory, combat, AI, and objective state;
- compatibility metadata;
- state hash metadata.

### Transient State

Transient state may be useful during a run but is not gameplay truth:

- current app CLI paths;
- process exit status;
- verbose diagnostic buffers;
- runtime metrics if not explicitly saved in a debug section;
- temporary query results;
- app input state.

### Derived State

Derived state must be regenerated from authoritative state:

- scene projection;
- debug projection;
- runtime summary;
- renderer-facing camera matrices;
- future GPU handles;
- UI prompts.

Derived state cannot be used as save truth.

## Runtime Lifecycle

The session lifecycle is owned by `runtime/session`.

Required lifecycle states:

- `Loading`: package/fixture data has not yet created a playable session.
- `Playing`: normal or slow-time runtime can advance.
- `Paused`: automatic advancement is stopped; explicit step may advance one
  tick.
- `Complete`: objective/outcome reached the acceptance success state.
- `Failed`: unrecoverable runtime or content error for the current session.

Reset is an operation, not a permanent lifecycle state. Reset restores the
validated baseline seed, clears transient events, restores default clock/camera,
and produces a deterministic baseline hash.

## Runtime Loop

The complete runtime loop is deterministic:

1. parse app/tool configuration;
2. load package manifest;
3. validate package and scenario;
4. build deterministic scenario seed;
5. create session from seed and `RuntimeConfig`;
6. bind player slot 0 to player actor;
7. receive command proposal from app script, raw input adapter, AI proposal, or
   future network packet;
8. apply authority check;
9. apply command admission;
10. append accepted or rejected command to `CommandLog`;
11. process session-control command effects such as pause, resume, tactical
    toggle, retry, reset, save request, or load request;
12. advance fixed session tick when clock rules allow it;
13. execute owning gameplay systems in fixed tick order;
14. emit runtime events and metrics;
15. evaluate objective and lifecycle outcome;
16. update deterministic state hash;
17. generate runtime summary;
18. generate read-only scene/debug projection.

Steps 1 through 4 are content/app setup. Steps 5 through 18 are runtime and
derived-output operation. File IO belongs to content/app/save codec boundaries,
not to gameplay systems.

## Tick Order

`SessionTick` owns the exact tick order. It must be stable across platforms and
replay runs.

Required tick order:

1. gather accepted commands scheduled for this tick;
2. dispatch movement commands to `MovementSystem`;
3. dispatch interaction commands to `InteractionSystem`;
4. dispatch combat commands to `CombatSystem`;
5. run AI proposal generation for future admission;
6. evaluate objective state and lifecycle outcome;
7. collect runtime events and deterministic metrics;
8. refresh state hash and summary inputs.

Rules:

- rejected commands never reach system execution;
- AI proposals are command proposals for later admission, not direct mutation;
- renderer and app code are not part of tick order;
- wall-clock time is not read inside the tick;
- entity iteration order must be stable.

## Command Architecture

All gameplay mutation starts from a command or deterministic system tick.

```text
raw input / app script / AI proposal / future network packet
  -> CommandRecord
  -> Authority
  -> CommandAdmission
  -> CommandLog
  -> SessionTick or session-control handler
  -> owning runtime system
```

Required command kinds:

- `Move`;
- `Interact`;
- `Inspect`;
- `Wait`;
- `ToggleTacticalMode`;
- `Pause`;
- `Resume`;
- `StepTacticalTick`;
- `Retry`;
- `Reset`;
- `Save`;
- `Load`.

Command rules:

- raw keyboard, mouse, controller, or network events are not gameplay commands;
- commands are serializable values;
- command ids and sequences are deterministic;
- authority rejection happens before gameplay legality checks;
- admission is read-only;
- every accepted and rejected command is logged;
- rejected commands do not mutate state;
- retry references a previous rejected command id and re-admits the original
  command intent under current state;
- reset restores the baseline session through `Session`, not through app code;
- save/load commands request runtime save/load behavior but app/tool code owns
  file paths and process IO.

## Targeting And Reach Architecture

Target discovery and reach validation are separate.

Target discovery:

- owned by `runtime/targeting/TargetQuery`;
- scans active world entities in deterministic order;
- filters by command kind and entity targetability;
- ignores invalid/self/inactive entities unless a command explicitly allows
  them;
- picks nearest valid target; equal distance breaks by lower `EntityId`.

Reach validation:

- owned by `runtime/targeting/ReachQuery`;
- checks actor position against target point or closest point on target bounds;
- produces exact failure reason `OutOfRange` for acceptance-sensitive rejection;
- does not execute interaction effects.

Interaction execution:

- owned by `runtime/interaction/InteractionSystem`;
- runs only after command admission and reach validation;
- mutates world/inventory/objective through their owning APIs.

## Camera And Time Architecture

Product camera requirement:

- normal realtime play uses first-person or close third-person camera;
- slow-time/planning mode switches to tactical camera;
- returning to normal restores previous realtime camera.

Clock modes:

- `Normal`: realtime semantic play mode;
- `Slow`: tactical/planning semantic play mode;
- `Paused`: no automatic tick advancement;
- step request: advances exactly one tick while paused.

Camera modes:

- `FirstPerson`;
- `ThirdPerson`;
- `TacticalOrbit`;
- `TacticalOverhead`.

Ownership:

- `runtime/clock` owns normal, slow, paused, and step behavior;
- `runtime/camera` owns semantic camera mode and previous realtime camera;
- `CameraModePolicy` maps clock/control transitions into camera transitions;
- projection may derive renderer-facing camera facts;
- renderer may consume matrices later but cannot own camera truth.

Transition rules:

- entering slow-time stores current realtime camera and switches to tactical;
- leaving slow-time restores stored realtime camera;
- pausing preserves current camera mode;
- stepping while paused does not change camera by itself;
- changing camera mode requests transient input clearing so raw input deltas do
  not leak across modes.

## Save Load Replay Architecture

Save/load and replay are first-class runtime requirements.

Save architecture:

- `SessionState` is the source for save truth;
- `SaveEnvelope` is the durable versioned container;
- `SaveCodec` encodes/decodes deterministic storage text or bytes;
- `SaveCompatibility` rejects incompatible envelopes before mutation;
- `SaveLoad` applies load as an all-or-nothing transaction.

Replay architecture:

- `CommandLog` stores accepted and rejected command history;
- `CommandReplay` rebuilds from baseline fixture and resubmits commands through
  normal admission/execution;
- `StateHash` verifies deterministic equality;
- divergence reports the first mismatching command, tick, rejection reason, or
  state hash.

Forbidden in save/replay:

- renderer handles;
- raw input events;
- app file paths;
- projection output;
- unordered container traversal as canonical order;
- old `iggy` save data.

## Multiplayer Architecture

The first product is single-player, but the runtime must preserve multiplayer
shape.

Single-player:

- player slot 0;
- local authoritative mode;
- one actor bound to the local player slot.

Multiplayer-ready concepts:

- stable player slot ids;
- slot kind: local, remote, AI, observer;
- authority policy;
- command sequence per slot;
- deterministic command merge order;
- replication packet values;
- replication codec;
- local multiplayer coordinator for pre-network testing.

Networking is not required for the headless acceptance demo. Sockets and
transport are outside runtime ownership until the packet/value model is proven.

## Projection Architecture

Projection is a read-only consumer of runtime truth.

Scene projection owns:

- stable scene item list;
- entity id references;
- transform and bounds copies;
- semantic scene item kind;
- inert asset reference strings if present.

Debug projection owns:

- target query visualization data;
- reach/bounds visualization data;
- camera mode diagnostics;
- command rejection markers;
- objective/session diagnostic markers.

Projection must not:

- mutate runtime state;
- own GPU handles;
- require renderer API;
- decide command legality;
- become save truth.

## Diagnostics Architecture

Diagnostics explain runtime behavior. They do not cause behavior.

Required diagnostic surfaces:

- structured `Diagnostic` for content/app/save/replay failures;
- `RuntimeEvent` for command accepted/rejected, movement, interaction, pickup,
  objective completion, clock/camera change, save/load, reset;
- `RuntimeMetrics` for deterministic counters;
- `RuntimeSummary` for acceptance output.

Acceptance-sensitive diagnostics must use stable codes or enum values. Human
text can improve, but codes such as `OutOfRange` must not drift without test and
doc updates.

## Compute Strategy

The first complete runtime favors deterministic simple algorithms over
premature acceleration.

Required baseline costs:

- entity lookup: O(entity count);
- target discovery: O(entity count);
- reach check: O(entity lookup plus constant math);
- movement command: O(entity lookup);
- interaction command: O(entity lookup plus inventory/objective operation);
- inventory operation: O(item stack count);
- save/load: O(session state size);
- state hash: O(saved state size);
- replay: O(command count times session operation cost);
- scene/debug projection: O(entity count plus event count).

Future acceleration is allowed only if:

- external behavior is unchanged;
- stable iteration order is preserved;
- state hash remains deterministic;
- replay reaches the same result;
- tests cover the accelerated path.

## Acceptance Architecture

The first acceptance demo is the architectural gate for runtime completion.

Fixture:

```text
fixtures/demos/first_room
```

Required proof:

1. validate package and scenario;
2. create session;
3. bind player slot 0;
4. start in realtime `ThirdPerson` camera;
5. discover `gold_key` target;
6. reject initial interact as `OutOfRange`;
7. move player into reach;
8. retry rejected interact and succeed;
9. mutate inventory and objective;
10. enter slow-time tactical camera;
11. issue tactical move;
12. pause, step once, and resume;
13. return to normal realtime camera;
14. save and load into fresh session;
15. reset branch to baseline;
16. replay command log from baseline;
17. match deterministic expected summary and state hash.

## Completion Gate

This architecture is satisfied only when:

- every file in `INDEX.md` exists in `/Users/kogaryu/iggy3d`;
- every file builds in the standalone repo;
- all unit tests pass;
- the complete runtime acceptance test passes;
- `iggy3d_headless_demo` emits the expected summary;
- `iggy3d_validate_package` validates the first-room fixture;
- `iggy3d_replay_tool` verifies the command log and state hash;
- no production target includes or links old `/Users/kogaryu/iggy` code.
