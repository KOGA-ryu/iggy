# E188: TraversalTag Remainder Classification Audit

## Status

Ready.

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
