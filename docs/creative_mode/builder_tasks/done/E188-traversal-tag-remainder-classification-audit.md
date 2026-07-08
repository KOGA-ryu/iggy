# E188: TraversalTag Remainder Classification Audit

## Status

Done.

## Context

E126 added the shared `TraversalTag` catalog. E181-E184 and E187 routed the
known traversal payload emitters/validators/consumers that were safe to migrate.

The current grep still reports traversal-looking string literals, but many are
probably not catalog payloads:

- parser role/shape names;
- display/readout strings;
- stable object id suffixes;
- save-format field suffixes;
- local gameplay/material tags copied beside traversal tags;
- render/debug role labels.

Do not blindly convert these. This card exists to classify the remaining hits
and produce exact next implementation cards only where centralizing the catalog
would reduce real drift without changing file formats or display contracts.

## Scope

Read-only audit only.

Inspect remaining non-test hits from:

```sh
rg -n '"(walkable|blocker|projectile_blocker|opening|clamber|clamber_candidate|vault|wire_walk|no_player|debug_only)"' \
  src --glob '*.cpp' --glob '*.hpp' \
  | rg -v 'test|TraversalTag\.(cpp|hpp)'
```

As of E187 review, expected files included:

- `src/content/assets/RoomAsset.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`
- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
- `src/app/iggy3d/creative/adapters/RoomBake.cpp`
- `src/runtime/movement/MovementTraversalSlots.cpp`
- `src/runtime/movement/MovementTraversal.cpp`
- `src/runtime/player/PlayerMotor.cpp`
- `src/runtime/collision/CollisionQuery.cpp`
- `src/runtime/save/SaveCodec.cpp`
- `src/runtime/ai/ReasoningGraph.cpp`
- `src/projection/debug/DebugProjection.cpp`
- `src/render/vulkan/BufferImageResources.cpp`

If the list has drifted, report the current list instead of forcing this list.

## Deliverable

Append the audit to this card and move it to `done/`.

The audit must classify every remaining hit into one of these buckets:

1. **Traversal payload still worth migrating**: literal is a durable traversal
   tag id written to or read from `RoomAsset`/authored traversal tag payloads.
2. **Parser/serializer file-format literal**: role/shape/save-field names where
   changing to `TraversalTag` would blur file-format ownership.
3. **Display/debug/readout literal**: UI, debug, receipt, stringifier, mechanic
   name, phase name, or diagnostic text.
4. **Stable id suffix**: object/surface ids where byte stability is the contract.
5. **Local gameplay/material/terrain vocabulary**: tags intentionally not in the
   traversal catalog.
6. **Needs owner decision**: ambiguous case with the exact owner question.

For each hit or grouped set of same-policy hits, include:

- file and function/area;
- literal(s);
- classification bucket;
- reason the catalog should or should not own it;
- if implementation is recommended, the exact follow-up card title and minimal
  file/test scope.

## Required Checks

Run:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

No build, no CTest, and no receipt golden run are required for this read-only
audit unless you edit source/test/CMake files, which you should not.

## Non-Scope

- Do not edit source, tests, CMake, receipt golden, or production docs.
- Do not route parser role strings, display strings, stable ids, or save-format
  suffixes unless this audit first proves they are traversal payload drift.
- Do not change `TraversalTag` catalog values.
- Do not change movement slot construction, clamber fallback policy, RoomAsset
  validation, ASCII serialization behavior, save/load format, ProductAppWindowState,
  renderer/Vulkan, or receipt keys.
- Do not stage, commit, push, or launch a window.

## Completion Brief Template

- Card moved to done:
- Files inspected:
- Current remaining grep count:
- Classification summary by bucket:
- Recommended implementation cards, if any:
- Owner decisions needed, if any:
- Checks run:
- Confirmation of no source/test/CMake/receipt golden/window changes:

## Completion Brief

- Card moved to done: yes, after appending this brief.
- Files inspected:
  - `src/content/assets/RoomAsset.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`
  - `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`
  - `src/app/iggy3d/creative/adapters/RoomBake.cpp`
  - `src/runtime/movement/MovementTraversalSlots.cpp`
  - `src/runtime/movement/MovementTraversal.cpp`
  - `src/runtime/player/PlayerMotor.cpp`
  - `src/runtime/collision/CollisionQuery.cpp`
  - `src/runtime/save/SaveCodec.cpp`
  - `src/runtime/ai/ReasoningGraph.cpp`
  - `src/projection/debug/DebugProjection.cpp`
  - `src/render/vulkan/BufferImageResources.cpp`
- Current remaining grep count: 36 hits across 12 source files from:
  - `rg -n '"(walkable|blocker|projectile_blocker|opening|clamber|clamber_candidate|vault|wire_walk|no_player|debug_only)"' src --glob '*.cpp' --glob '*.hpp' | rg -v 'test|TraversalTag\.(cpp|hpp)'`
- Classification summary by bucket:
  - Bucket 1, traversal payload still worth migrating: 0 hits.
  - Bucket 2, parser/serializer file-format literal: 13 hits.
  - Bucket 3, display/debug/readout literal: 19 hits.
  - Bucket 4, stable id suffix: 3 hits.
  - Bucket 5, local gameplay/material/terrain vocabulary: 1 hit.
  - Bucket 6, needs owner decision: 0 hits.

### Hit Classification

- `src/content/assets/RoomAsset.cpp`, `parseShape(...)`, line 218:
  - Literal: `opening`.
  - Bucket: 2, parser/serializer file-format literal.
  - Reason: this is the persisted RoomAsset spatial-surface shape parser, not a traversal tag payload. `Opening` is also a surface shape token, so routing through `TraversalTag` would blur file-format ownership.
  - Recommended implementation: none.

- `src/content/assets/RoomAsset.cpp`, `parseRole(...)`, lines 230, 234, 238, 242:
  - Literals: `walkable`, `blocker`, `projectile_blocker`, `opening`.
  - Bucket: 2, parser/serializer file-format literal.
  - Reason: these are persisted RoomAsset spatial-surface role parser tokens. E182 already routed role-required traversal tag validation through `TraversalTag`; the role parser must remain owned by `RoomSpatialSurfaceRole`.
  - Recommended implementation: none.

- `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`, `shapeName(...)`, line 108:
  - Literal: `opening`.
  - Bucket: 2, parser/serializer file-format literal.
  - Reason: this serializes a `RoomSpatialSurfaceShape` field, not a traversal tag payload.
  - Recommended implementation: none.

- `src/app/iggy3d/ascii_room/AsciiRoomAssetText.cpp`, `roleName(...)`, lines 116, 118, 120, 122, 124:
  - Literals: `walkable`, `blocker`, `projectile_blocker`, `opening`.
  - Bucket: 2, parser/serializer file-format literal.
  - Reason: these serialize the `RoomSpatialSurfaceRole` field. E183 already routed role-derived traversal tag export ordering through `TraversalTag`; the role text itself remains file-format vocabulary.
  - Recommended implementation: none.

- `src/runtime/save/SaveCodec.cpp`, `writeAuthoredRoomSemantics(...)` and `readAuthoredRoomSemantics(...)`, lines 885 and 1547:
  - Literal: `walkable`.
  - Bucket: 2, parser/serializer file-format literal.
  - Reason: this is the authored-room boolean field key suffix `walkable`, not a traversal tag id. Changing it would change save-format key ownership.
  - Recommended implementation: none.

- `src/projection/debug/DebugProjection.cpp`, `phaseName(...)` and runtime HUD line assembly, lines 149 and 227:
  - Literals: `wire_walk`, `walkable`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: these are runtime debug HUD/readout strings: motor phase and ground walkability display. They do not write or validate authored traversal payloads.
  - Recommended implementation: none.

- `src/runtime/movement/MovementTraversalSlots.cpp`, `movementTraversalSlotKindName(...)`, lines 552, 554, 556, 558:
  - Literals: `vault`, `clamber`, `wire_walk`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: this is the movement slot kind stringifier. E184 deliberately left slot-kind display output byte-identical and separate from traversal-tag parsing.
  - Recommended implementation: none.

- `src/runtime/movement/MovementTraversal.cpp`, `traversalMechanicName(...)`, lines 955, 957, 959, 961:
  - Literals: `vault`, `clamber`, `wire_walk`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: this is the traversal mechanic stringifier used by gameplay/debug receipts. Mechanic names are not traversal payload parsing.
  - Recommended implementation: none.

- `src/runtime/player/PlayerMotor.cpp`, `playerMotorPhaseName(...)`, line 360:
  - Literal: `wire_walk`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: this is a player motor phase stringifier, not a traversal tag payload.
  - Recommended implementation: none.

- `src/runtime/collision/CollisionQuery.cpp`, `collisionSurfaceRoleName(...)` and `collisionSurfaceShapeName(...)`, lines 202, 204, 206, 208, 210, 220:
  - Literals: `walkable`, `blocker`, `projectile_blocker`, `opening`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: these stringify runtime collision roles/shapes. They intentionally mirror role names but are owned by runtime collision diagnostics, not `TraversalTag`.
  - Recommended implementation: none.

- `src/runtime/ai/ReasoningGraph.cpp`, `reasoningEdgeKindName(...)`, line 125:
  - Literal: `walkable`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: this is a reasoning graph edge-kind stringifier. It is not reading or writing RoomAsset/authored traversal tags.
  - Recommended implementation: none.

- `src/render/vulkan/BufferImageResources.cpp`, material color role dispatch, line 174:
  - Literal: `opening`.
  - Bucket: 3, display/debug/readout literal.
  - Reason: this is a render role label used for color/material routing. It is not a traversal payload and should not pull in `TraversalTag`.
  - Recommended implementation: none.

- `src/runtime/movement/MovementTraversalSlots.cpp`, legacy fallback affordance detection, line 378:
  - Literal: `vault`.
  - Bucket: 4, stable id suffix.
  - Reason: this checks legacy mesh ids containing `vault` for a fallback rail affordance. E184 explicitly preserved this id-substring fallback and kept it separate from authored traversal tags.
  - Recommended implementation: none.

- `src/app/iggy3d/creative/adapters/RoomBake.cpp`, stable surface id construction, lines 370 and 413:
  - Literals: `walkable`, `projectile_blocker`.
  - Bucket: 4, stable id suffix.
  - Reason: these are stable `RoomSpatialSurface.id` suffixes produced by `stableObjectId(...)`; the traversal payload on the same surfaces already uses `traversalTagId(TraversalTag::Walkable)` and `traversalTagId(TraversalTag::ProjectileBlocker)`.
  - Recommended implementation: none.

- `src/app/iggy3d/ascii_room/AsciiRoomToAuthoredRoom.cpp`, `objectSemantics(...)`, line 101:
  - Literal: `clamber`.
  - Bucket: 5, local gameplay/material/terrain vocabulary.
  - Reason: the durable traversal tag next to it already comes from `tagString(TraversalTag::Clamber)` on line 100. This hit is the copied local gameplay tag for the movement clamber ledge proxy, intentionally separate from traversal payload ownership.
  - Recommended implementation: none.

### Recommended Implementation Cards

- None. The current grep contains no remaining durable traversal payload emitter, validator, or consumer that should be migrated to `TraversalTag` without changing parser, serializer, display, id, or local gameplay ownership.

### Owner Decisions Needed

- None from this audit. If a future cleanup wants shared stringifiers for movement mechanic names, collision role names, or render/debug role labels, that should be a separate owner-specific vocabulary card, not a `TraversalTag` migration.

### Checks Run

- `git -C /Users/kogaryu/iggy3d diff --check`

### Confirmation

- No source files were edited.
- No test files were edited.
- No CMake files were edited.
- Receipt golden was not edited or run.
- No window was launched.
