# File Spec

Files: `src/app/iggy3d/creative/document/Block.hpp`, `src/app/iggy3d/creative/document/Block.cpp`

Verified at: `7b40370f`

## Owns

- Editable room document block/face overlay projection.
- Floor, wall, and object face overlay packets.
- Measurement labels for editable room primitives.
- Selection and hover flagging for projected room-editor primitives.
- Primitive kind/face names for block overlays.

## Does Not Own

- CreativeDocument object projection.
- Editable room document mutation.
- Runtime room bake or collision.
- UI hit testing or rendering.
- Object descriptor semantics.

## Reads

- `EditableRoomDocument` floors, walls, and objects.
- Grid step and optional selected/hovered primitive refs from `BlockRequest`.
- Primitive ids/source indexes for stable refs.

## Writes / Mutates

- Builds `BlockView` faces, labels, counters, status, and reason code.
- Does not mutate the editable room document.

## Calls Out To / Wires Out To

- Consumed by creative/product block UI and tests.
- Uses core `Vec3` math and editable-room authoring packets.
- Separate from CreativeDocument wireframe and spatial projection.

## Called By / Entry Points

- `kindName(...)`.
- `faceName(...)`.
- `buildBlockView(...)`.
- Grep proof: `rg -n "buildBlockView|BlockView|FaceOverlay|kindName\\(|faceName\\(" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Missing document and invalid grid step fail closed with no faces or labels.
- Floor emits one top face and width/depth labels.
- Wall and object boxes emit six faces.
- Selection and hover compare kind plus id when present, otherwise source index.
- Output is deterministic and must not mutate the input document.

## Tests / Proof Commands

- `product_creative_block_tests`.
- `creative_block_tests.cpp`.
- `rg -n "product_creative_block_tests|buildBlockView|FaceOverlay" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/content/authoring/EditableRoomDocument.*` unless authoring document schema changes.
- `src/app/iggy3d/creative/document/DocumentWireframe.*` unless CreativeDocument projection is deliberately merged.
- `src/app/iggy3d/creative/adapters/RoomBake.*` unless editable-room bake semantics change.
- `src/app/iggy3d/creative/ui/UiFrame.*` unless block overlay consumption changes.

## Update When

- Block overlay packets, face/label generation, selection/hover matching, status semantics, or editable-room primitive consumption changes.

## Do Not Update When

- Only CreativeDocument wireframe, runtime bake, or UI styling changes without changing editable-room block projection.
