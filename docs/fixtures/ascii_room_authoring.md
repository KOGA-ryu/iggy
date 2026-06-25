# ASCII Room Authoring Fixture

## Source Fixture

- Fixture: `fixtures/rooms/ascii/training_room.iggyroom.txt`
- Format: engine-owned ASCII room source text (`.iggyroom.txt`)
- Source truth: the ASCII file is the authored room input for this proof path.

## Glyph Vocabulary

- `#`: wall
- `.` and space: floor
- `+`: door marker on floor
- `s`: secret door marker on floor
- `P`: player spawn marker
- `N`: NPC spawn marker
- `M`: monster spawn marker
- `$`: treasure marker
- `K`: key marker
- `T`: trap marker
- `E`: exit marker
- `?`: inspect marker

## Coordinates

Grid columns map to world `+X`; grid rows map to world `+Z`; world `Y` is height/elevation. Row 0 is north/top and column 0 is west/left. The current compile path centers the room on the origin by default.

## Expected Facts

`training_room.iggyroom.txt` is 7 columns by 5 rows. It contains one player spawn, one NPC spawn, one door marker, one treasure marker, and one exit marker. Compiling the fixture produces 15 floor records, 20 wall records, and 5 sidecar markers.

## Current Pipeline

```text
ASCII source -> parser -> semantic grid -> authored-room compile
```

The authored-room compile emits `SaveAuthoredRoomSection` floor/wall geometry and keeps spawn/object/door semantics as sidecar marker records.


## Room Asset Text Fixture

- Fixture: `fixtures/rooms/ascii/training_room.room.iggy3d.toml`
- The room asset text fixture is generated from `training_room.iggyroom.txt` through the current parser, semantic grid, authored-room compiler, RoomAsset bridge, and `writeAsciiRoomAssetText(...)`.
- `ascii_room_asset_text_fixture_tests` verifies exact byte-for-byte exporter parity and parses the checked-in text through `parseRoomAssetText(...)`.


## Package Loader Fixture

- Fixture package: `fixtures/demos/ascii_training_room/package.iggy3d.toml`
- The package-local room asset at `fixtures/demos/ascii_training_room/assets/rooms/training_room.room.iggy3d.toml` is byte-for-byte equal to the shared ASCII-generated room asset fixture.
- `ascii_room_package_fixture_tests` proves the checked-in room text loads through `loadPackage(...)` with minimal mesh/material libraries. This is package-loader proof only; it does not wire Product App, AppShell, runtime entity spawning, or renderer display.


## Runtime Collision Proof

- Test: `ascii_room_runtime_collision_tests`
- The test loads `fixtures/demos/ascii_training_room/package.iggy3d.toml`, builds a `SpatialSurfaceSet`, and proves the ASCII package room contributes walkable floor surfaces, actor blocker wall surfaces, projectile blocker wall surfaces, and spawn-offset sampling through existing runtime collision queries.
- This is runtime collision proof only; it does not wire Product App, AppShell, runtime entity spawning, renderer display, saves, or gameplay launch.

## Deferred Work

- Runtime package bridge
- Product UI selection
- Renderer display
- Runtime entity binding
- Wall-run optimization
