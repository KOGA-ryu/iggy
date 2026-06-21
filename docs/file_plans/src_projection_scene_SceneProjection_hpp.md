# `src/projection/scene/SceneProjection.hpp`

Updated: 2026-06-20

Exact purpose: declare read-only projection from `SessionState` to renderer-facing scene items and camera facts.

## Build Position

- priority rank: 104
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/projection/scene`
- file kind: `header`

## Ownership

This file owns:

- runtime-to-scene conversion
- stable item order
- camera projection handoff values

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core values
- runtime state read-only headers
- no app, renderer API, or mutation dependencies

## Data Contract

- one item per active renderable entity
- camera mode/target facts
- optional objective/interaction highlighting flags

## Semantics

- projection never mutates runtime
- projection has no GPU/API dependency
- old renderer bridge is forbidden

## Detailed Design Contract

Declare `SceneProjectionConfig` with booleans `includeInactive`,
`includeObjectiveMarkers`, `includeTacticalMarkers`, and `includeDebugOnly`.
Defaults: inactive false, objective/tactical markers true, debug-only false.

Declare `SceneProjectionResult` with `std::vector<SceneItem> items`,
`playerCount`, `pickupCount`, `interactableCount`, `markerCount`,
`debugOnlyCount`, and the source state hash/tick copied from `SessionState`.

Declare `buildSceneProjection(const SessionState& state, const
SceneProjectionConfig& config)`.

Invariants:

- the function is read-only and cannot mutate `SessionState`, diagnostics,
  metrics, command log, or save state;
- output order is stable runtime entity order followed by stable marker order;
- projection does not perform command admission, reach checks, physics, file IO,
  renderer calls, or asset loading;
- projection is regenerated after load/replay and is never durable truth.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(entity count).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- scene projection is derived, regenerated, and excluded from `SaveEnvelope`;
- projection output is excluded from `StateHash`;
- replay and multiplayer consume authoritative `SessionState`, not projection
  output.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/projection/scene/SceneProjection.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
