# File Spec

Files: `src/app/iggy3d/creative/adapters/RoomBake.hpp`, `src/app/iggy3d/creative/adapters/RoomBake.cpp`

Verified at: `791c40db`

## Owns

- Conversion from `CreativeDocument` objects into a `RoomAsset`.
- `CreativeRoomBakeRequest`, bake status enums, receipt/result packets, and source-link records for meshes, anchors, and spatial surfaces.
- Object classification for hidden/editor-only/room-metadata/anchor/static-mesh/unsupported/no-bounds cases.
- Runtime room geometry role mapping for floor, wall, and prop objects.
- Bake status and reason-code assignment for missing, invalid, empty, and accepted bakes.

## Does Not Own

- Creative document mutation or object creation.
- Active room installation or collision freshness refresh.
- Greedy floor merge implementation details.
- Reachability flood-fill implementation details.
- Renderer mesh emission.

## Reads

- `CreativeDocument` object list, object visibility, transforms, bounds, kinds, and descriptors from `describeObject(...)`.
- `CreativeRoomBakeRequest` id/source/include-hidden/reachability settings.
- Traversal tag ids for walkable and blocker surface tags.

## Writes / Mutates

- Writes `CreativeRoomBakeResult` containing `RoomAsset`, bake receipt, reachability receipt, and source-link vectors.
- Populates room static meshes, anchors, spatial surfaces, source metadata, and bake counters.
- Does not mutate the source document.

## Calls Out To / Wires Out To

- Calls `buildRoomBakeGreedyFloorPlan(...)` for mergeable structural floor candidates.
- Calls `initialCreativeRoomBakeReachabilityReceipt(...)` and `validateCreativeRoomBakeReachability(...)`.
- Supplies `RoomAsset` output to product creative baked active-room refresh.

## Called By / Entry Points

- `buildRoomAssetFromCreativeDocument(...)`.
- `creativeRoomBakeBoundsAreValid(...)`.
- `toString(CreativeRoomBakeStatus)` and `toString(CreativeRoomBakeReachabilityStatus)`.
- Grep proof: `rg -n "CreativeRoomBake|buildRoomAssetFromCreativeDocument|creative_room_bake|RoomBakeGreedy|GreedyFloor" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Missing or invalid documents return rejected receipts and do not fabricate runtime room content.
- Hidden objects are skipped unless `includeHidden` is set.
- Editor-only objects and room metadata objects are not baked as runtime room content.
- Point objects bake only as supported runtime anchors with valid finite positions.
- Static geometry requires supported shape policy and finite bounds that fit `float`.
- Floor bakes produce walkable spatial surfaces; structural or collision non-floor bakes produce blocker surfaces.
- Bake receipt counts must match emitted room vectors and source-link vectors.

## Tests / Proof Commands

- `creative_document_room_bake_tests`.
- `product_creative_world_launch_tests`.
- `product_creative_no_window_bake_scenario_tests`.
- `rg -n "creative_document_room_bake_tests|product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/adapters/RoomBakeGreedyFloors.*` unless floor merge policy changes.
- `src/app/iggy3d/creative/adapters/RoomBakeReachability.*` unless reachability receipt behavior changes.
- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless descriptor-to-runtime bake policy changes.
- `src/content/assets/RoomAsset.hpp` unless the room asset schema changes.

## Update When

- Document-to-room conversion, bake result packets, status/reason codes, source-link semantics, object classification, or emitted room asset rules change.

## Do Not Update When

- Only active-room install, window receipts, UI commands, or renderer draw behavior changes after a room asset has already been baked.
