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

## Deferred Work

- Runtime package bridge
- Product UI selection
- Renderer display
- Runtime entity binding
- Wall-run optimization
