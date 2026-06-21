# `docs/ownership.md`

Updated: 2026-06-20

Exact purpose: define immutable ownership boundaries for `iggy3d` data,
mutation, computation, persistence, projection, diagnostics, app code, and
future multiplayer.

## Authority Of This Document

This document decides who owns each category of data and who is allowed to
mutate it. `docs/architecture.md` defines dependency shape. This document
defines mutation and state truth.

If a builder cannot tell which file owns a field, mutation, or computation, the
builder must update this document before implementing the ambiguous behavior.

## Global Rule

Runtime owns gameplay truth.

Apps collect input, choose fixture paths, call public APIs, print output, and
set process exit codes.

Content creates validated seed data before runtime starts.

Projection derives read-only output from runtime state.

Save/load persists runtime truth.

Replay proves runtime determinism.

No old `/Users/kogaryu/iggy` module owns any `iggy3d` state.

## Ownership Categories

Every value must fit one of these categories.

### Authoritative Runtime Truth

Owned by runtime modules. May be saved, replayed, hashed, and projected.

Examples:

- entity ids and transforms;
- player slots;
- clock mode;
- camera mode;
- command log;
- inventory items;
- combat health;
- AI state;
- objective status.

### Validated Seed Data

Owned by content before session creation. Copied into runtime-owned state when a
session is created.

Examples:

- package id;
- scenario id;
- seed entities;
- seed objectives;
- spawn binding;
- initial item/combat/objective metadata.

### Durable Save Truth

Owned by runtime/save. Built from authoritative runtime truth.

Examples:

- `SaveEnvelope`;
- schema version;
- runtime version;
- package/scenario ids;
- serialized world/player/clock/camera/subsystem state;
- command log;
- state hash metadata.

### Derived Output

Owned by projection, diagnostics, or app output. Regenerable from authoritative
state.

Examples:

- scene items;
- debug markers;
- runtime summary;
- metric report;
- renderer-facing matrices;
- future UI prompts.

### Transient External State

Owned by app/tool/render/input layers, not runtime truth.

Examples:

- CLI paths;
- raw keyboard/mouse/controller deltas;
- app verbosity;
- process exit code;
- renderer handles;
- GPU resources;
- socket handles.

## Module Ownership Matrix

| Module | Owns | May Mutate | Must Not Own |
| --- | --- | --- | --- |
| `core` | math, ids, result/status, diagnostics values, stable hash primitives | only local value construction | gameplay state, file IO, renderer state |
| `config` | deterministic runtime defaults | config value construction | CLI parsing, save payloads, gameplay mutation |
| `content` | package manifests, loaders, validators, scenario seed records | seed data before session creation | active session state, renderer upload, old schemas |
| `runtime/world` | entity collection, ids, transforms, bounds, active flags, baseline world copy | world/entity fields through `WorldState` API | command legality, save encoding, renderer ids |
| `runtime/player` | player slots, slot kinds, actor bindings | roster records through `PlayerRoster` API | input devices, network sockets, actor movement |
| `runtime/clock` | normal/slow/paused/step state | `ClockState` through `Clock` helpers | wall-clock timing, render frame pacing |
| `runtime/camera` | semantic camera state and previous realtime camera | `CameraState` through `CameraModePolicy` | raw mouse deltas, GPU matrices |
| `runtime/command` | command records, admission status, rejection reason values | command value construction and admission result values | execution side effects, raw input events |
| `runtime/session` | lifecycle, aggregate state, command/log coordination, reset/load transaction boundary | `SessionState` by delegating to owners | package parsing, CLI parsing, rendering |
| `runtime/movement` | movement command execution | actor transform through `WorldState` API | pathfinding/navmesh internals until added behind API, animation |
| `runtime/targeting` | target discovery and reach validation | no gameplay mutation | interaction effects, renderer picking ownership |
| `runtime/interaction` | pickup/open/activate/inspect effects | world/inventory/objective through owning APIs | target discovery, UI prompts |
| `runtime/inventory` | item stack truth | `InventoryState` through `InventorySystem` | pickup target validation, UI slot layout |
| `runtime/combat` | health, damage, defeat | combat/world defeat state through owning APIs | animation, unseeded randomness, AI planning |
| `runtime/ai` | AI memory/state and deterministic proposals | AI state bookkeeping, command proposal creation | direct world mutation, authority bypass |
| `runtime/objective` | objective records and outcome | `ObjectiveState` through `ObjectiveSystem` | UI notification, content parsing |
| `runtime/save` | envelope, codec, compatibility, load transaction mapping | save/load conversion and all-or-nothing session replacement through `Session` | renderer/app/raw input state |
| `runtime/replay` | command replay, state hash verification | replay session instance only through public session APIs | command legality shortcuts, fixture parsing internals |
| `runtime/multiplayer` | authority policy, packet values, local slot ordering | local command queues and packet values | sockets, platform transport, direct gameplay mutation |
| `runtime/diagnostics` | runtime events, metrics, summaries | derived diagnostic buffers/counters | gameplay causality, save truth unless explicit |
| `projection` | scene/debug output | projection buffers only | runtime mutation, GPU resources, command legality |
| `apps/tools` | CLI, file paths, process orchestration, output | app-local config and process output | gameplay rules, direct runtime state writes |
| `tests` | assertions and fixtures used by tests | test-local state | production runtime behavior |

## State Field Ownership

### `SessionState`

Owned by `runtime/session`.

Contains by value or owned aggregate:

- lifecycle;
- world state;
- player roster;
- clock state;
- camera state;
- command log;
- inventory state;
- combat state;
- AI state;
- objective state;
- baseline reset snapshot;
- package/scenario ids;
- transient event/metric buffers if held at session scope.

`SessionState` does not own:

- raw input;
- app file paths;
- renderer resources;
- package parser internals;
- projection output as truth.

### `WorldState`

Owned by `runtime/world`.

Owns:

- entity allocation;
- stable entity ids;
- entity vector/order;
- entity transforms;
- entity bounds;
- entity active/persistent flags;
- per-entity semantic metadata needed by runtime systems.

Only `WorldState` APIs may mutate entity collection or entity fields. Systems
must request mutations through those APIs.

### `EntityState`

Owned inside `WorldState`.

May contain:

- `EntityId`;
- stable name;
- entity kind;
- transform;
- bounds;
- active flag;
- targetability flags;
- interaction id/kind;
- item id/count metadata;
- combat metadata;
- objective reference.

It must not contain:

- renderer object id;
- GPU handle;
- UI widget id;
- raw input state;
- old 2D object pointer.

### `PlayerRoster`

Owned by `runtime/player`, held by `SessionState`.

Owns:

- player slot id;
- slot kind: local, remote, AI, observer;
- actor entity binding;
- authority-facing player identity.

It must not own:

- input devices;
- sockets;
- movement state;
- UI focus.

### `ClockState`

Owned by `runtime/clock`, held by `SessionState`.

Owns:

- clock mode: normal, slow, paused;
- fixed tick index;
- slow-time scale setting if stored at runtime;
- pending step request;
- previous non-paused mode if needed for resume.

It must not own wall-clock timestamps or render delta time.

### `CameraState`

Owned by `runtime/camera`, held by `SessionState`.

Owns:

- camera mode: first-person, third-person, tactical orbit, tactical overhead;
- previous realtime camera mode;
- target entity or target point;
- semantic yaw/pitch/distance values;
- transient input-clear request flag.

It must not own:

- GPU view/projection matrices as renderer resources;
- raw mouse deltas;
- input sensitivity settings unless those become runtime config;
- renderer camera object.

### `CommandLog`

Owned by `runtime/replay`, held by `SessionState`.

Owns:

- submitted command records;
- accepted command records;
- rejected command records;
- deterministic sequence values;
- source command id for retry;
- enough data to replay rejection and acceptance behavior.

It must not own:

- raw key/mouse events;
- app command-line paths;
- network socket payload bytes after decode into value packets.

## Mutation Rules

### General Mutation Rule

Only the owning module mutates its state.

Cross-module mutation must use an owning API. For example:

- movement mutates actor transform only by calling `WorldState`;
- interaction mutates inventory only by calling `InventorySystem`;
- interaction completes objectives only by calling `ObjectiveSystem`;
- combat defeat mutates world active/defeated state only through world/combat
  APIs;
- save/load replaces session state only through `Session`.

### App Mutation Rule

Apps/tools may:

- parse CLI;
- call `Session` public methods;
- call package loader/validator;
- call save/replay tools;
- print summaries;
- choose process exit code.

Apps/tools must not:

- edit `WorldState` directly;
- mark objectives complete directly;
- bypass command admission;
- inject accepted commands directly into system execution;
- write renderer/input state into save data.

### Projection Mutation Rule

Projection may:

- allocate scene/debug projection buffers;
- copy runtime values into projection values;
- compute read-only derived matrices or markers.

Projection must not:

- mutate session/world/subsystem state;
- decide command legality;
- mark an entity active/inactive;
- persist projection as save truth.

### Diagnostics Mutation Rule

Diagnostics may:

- append events;
- increment deterministic counters;
- build summaries;
- carry stable error/rejection codes.

Diagnostics must not:

- make gameplay decisions;
- cause mutations;
- replace command admission;
- become the only source of a persisted fact.

## Command Ownership

`runtime/command/Command.hpp` owns command value shape.

`runtime/multiplayer/Authority` owns authority checks.

`runtime/command/CommandAdmission` owns read-only legality checks.

`runtime/replay/CommandLog` owns append order and history.

`runtime/session` owns dispatching accepted commands to systems.

Gameplay systems own execution.

Required command rule order:

1. create command value;
2. check authority;
3. check admission legality;
4. append accepted or rejected command to log;
5. execute only accepted command through session/system path;
6. emit diagnostic/runtime event;
7. update state hash/summary after tick.

No system may execute a command that skipped authority/admission/logging.

## Target Interaction Ownership

Targeting is split deliberately:

- `TargetQuery` discovers possible targets.
- `ReachQuery` decides whether an actor can reach a target.
- `CommandAdmission` uses target/reach results to accept or reject.
- `InteractionSystem` applies accepted effects.

This prevents renderer picking, UI prompts, or interaction effects from becoming
the source of target truth.

Acceptance-sensitive ownership:

- `TargetQuery` must identify `gold_key` as a valid target.
- `ReachQuery` must reject the initial interaction as `OutOfRange`.
- `InteractionSystem` must not run for that rejected command.
- Retry must re-enter admission and only then execute interaction.

## Pause Retry Reset Ownership

Pause:

- command value owned by command module;
- legality owned by admission/session rules;
- state mutation owned by `Clock`;
- tick blocking owned by `SessionRunner`/`Clock`.

Step:

- command value owned by command module;
- step permission owned by clock/session rules;
- exactly one tick execution owned by `SessionRunner`.

Retry:

- references a rejected command id in `CommandLog`;
- reuses the original command intent;
- runs through authority and admission again;
- app code does not decide retry success.

Reset:

- baseline seed/snapshot owned by `SessionState`;
- reset operation owned by `Session`;
- world/player/clock/camera/subsystem states are restored through owning APIs or
  owned aggregate replacement;
- reset branch proof must not mutate the completed demo session unexpectedly.

## Persistence Ownership

Save truth includes:

- package id;
- scenario id;
- save schema version;
- runtime compatibility version;
- session lifecycle needed to resume;
- world/entity state;
- player roster;
- clock state;
- camera state;
- inventory state;
- combat state;
- AI state;
- objective state;
- command log;
- state hash metadata.

Save truth excludes:

- scene projection;
- debug projection;
- runtime summary output;
- app CLI paths;
- raw input events;
- renderer handles;
- GPU resources;
- wall-clock timestamps;
- sockets;
- old `iggy` state.

Load transaction ownership:

- `SaveCodec` parses bytes/text into `SaveEnvelope`;
- `SaveCompatibility` validates version compatibility;
- `SaveLoad` maps envelope to runtime state;
- `Session` applies the loaded state all-or-nothing;
- failed load leaves existing session unchanged.

## Replay Ownership

Replay owns proof, not special gameplay behavior.

`CommandReplay` must:

- create a fresh baseline session from the same fixture seed;
- resubmit each command through normal authority/admission/session flow;
- require rejected commands to reject for the same reason;
- require accepted commands to reach the same state;
- compare final state hash;
- report first mismatch.

Replay must not:

- mutate the original source session;
- inject state snapshots directly except for the initial baseline;
- bypass admission to force commands through;
- ignore rejected command records.

## Multiplayer Ownership

The runtime starts single-player but preserves multiplayer-ready ownership.

`PlayerRoster` owns player slots and actor bindings.

`Authority` owns whether a slot may issue a command.

`ReplicationPacket` owns transport-neutral packet values.

`ReplicationCodec` owns packet serialization.

`LocalMultiplayerSession` owns local merge ordering for multiple slots.

Runtime does not yet own:

- sockets;
- NAT traversal;
- platform sessions;
- remote clock sync;
- matchmaking.

Network transport must later feed decoded command/packet values into the same
authority/admission/session path.

## Compute Ownership

Compute costs are owned by the module performing the computation:

- entity lookup: `WorldState`, O(entity count);
- target discovery: `TargetQuery`, O(entity count);
- reach validation: `ReachQuery`, O(entity lookup plus constant math);
- movement: `MovementSystem`, O(entity lookup);
- interaction: `InteractionSystem`, O(entity lookup plus subsystem mutation);
- inventory: `InventorySystem`, O(item stack count);
- objective evaluation: `ObjectiveSystem`, O(objective count plus referenced
  state lookups);
- AI proposals: `AiSystem`, O(AI actor count times relevant scans);
- state hash: `StateHash`, O(saved state size);
- save/load: `SaveLoad`, O(session state size);
- replay: `CommandReplay`, O(command count times session operation cost);
- scene projection: `SceneProjection`, O(entity count);
- debug projection: `DebugProjection`, O(entity count plus event count);
- local multiplayer merge: `LocalMultiplayerSession`, O(command count) or
  O(command count log command count) depending on implementation.

Any future acceleration must preserve stable iteration order and replay/state
hash results.

## Diagnostics Ownership

Diagnostics are owned by the module that can explain the failure most precisely.

Examples:

- package parse errors: `content`;
- package semantic errors: `PackageValidator`;
- command authority rejection: `Authority`;
- command legality rejection: `CommandAdmission`;
- out-of-range rejection: `ReachQuery` result carried through admission;
- interaction unsupported: `InteractionSystem`;
- save version mismatch: `SaveCompatibility`;
- replay divergence: `CommandReplay`;
- summary mismatch: acceptance app/test.

Acceptance-sensitive diagnostic values must be stable enum/code values.

Required stable values include:

- `OutOfRange`;
- invalid actor;
- invalid target;
- unauthorized slot;
- incompatible save;
- replay divergence;
- package validation failure.

## Forbidden Ownership Transfers

These are explicitly forbidden:

- renderer owns gameplay camera truth;
- app owns command legality;
- projection owns active/inactive entity state;
- diagnostics own gameplay cause;
- save codec owns gameplay migration decisions without compatibility policy;
- AI mutates world directly;
- multiplayer packet decode bypasses admission;
- retry bypasses reach checks;
- reset is implemented as app-side manual field edits;
- old `iggy` data becomes runtime truth.

## Builder Checklist

Before implementing or reviewing a file, verify:

1. Which module owns this data?
2. Which API is allowed to mutate it?
3. Is this save truth, seed data, derived data, or transient data?
4. Does replay need this field?
5. Does state hash include this field?
6. Does projection derive from it?
7. Does multiplayer authority care about it?
8. What is the expected compute cost?
9. What stable diagnostic explains failure?
10. Is any old `iggy` dependency being introduced?

If any answer is unclear, update this document and the file-specific plan before
writing code.

## Completion Gate

Ownership is complete when:

- every runtime field has one owner;
- every mutation path goes through the owning API;
- save truth and derived truth are separated;
- command admission cannot be bypassed;
- reset/retry/pause/load behavior is owned by runtime, not app code;
- projection is read-only;
- replay uses normal runtime paths;
- multiplayer packet values feed authority/admission, not systems directly;
- no old `/Users/kogaryu/iggy` module owns or mutates `iggy3d` state.
