# `src/projection/scene/SceneItem.hpp`

Updated: 2026-06-20

Exact purpose: declare renderer-facing read-only scene item data derived from runtime state.

## Build Position

- priority rank: 103
- tier: Tier 6: Full Session Loop Diagnostics Projection
- module: `src/projection/scene`
- file kind: `header`

## Ownership

This file owns:

- scene item id
- semantic kind
- transform
- bounds
- visibility/debug flags
- asset reference string

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

- entity id reference
- no renderer GPU handle
- asset name is optional inert string

## Semantics

- scene items are derived from runtime and can be regenerated
- renderer may consume but not write them back into runtime

## Detailed Design Contract

Declare `enum class SceneItemKind : std::uint8_t` with at least `Player`,
`Pickup`, `Interactable`, `ObjectiveMarker`, `TacticalMarker`, and `DebugOnly`.

Declare `SceneItem` as a pure value projection with: stable runtime entity id,
kind, world transform, world bounds, active/visible flags, optional inert asset
reference, optional item id, optional objective id, optional interaction id, and
optional owning player slot.

Invariants and ownership:

- `SceneItem` never owns gameplay truth, renderer handles, GPU buffers, file
  paths, raw input, or command admission state;
- asset references are stable ids only, not loaded materials or mesh handles;
- item order is the stable order emitted by `SceneProjection`;
- projection fields are copied from `SessionState` and can be discarded at any
  time;
- no `SceneItem` field is written into `SaveEnvelope` or `StateHash`;
- multiplayer consumes owning slot/entity identity only as read-only metadata.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) per projected entity.
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- assert expected failures through this file's declared machine-readable result contract;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- scene items are derived projection data, regenerated from `SessionState`;
- scene items are excluded from `SaveEnvelope` and `StateHash`;
- multiplayer may read slot/entity ids from projection output but must not treat
  projection as authority.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/projection/scene/SceneItem.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
