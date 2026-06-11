# Save System

The save system is a durability boundary. It decides which simulation facts are
safe to persist, how those facts become bytes, and how invalid bytes are refused
before they can mutate a running world.

The important separation is this:

```text
SimulationWorld
  -> SnapshotWriter
  -> SimulationSnapshot
  -> SnapshotCodec
  -> SnapshotFileStore
  -> file
```

Loading reverses the path:

```text
file
  -> SnapshotFileStore
  -> SnapshotCodec
  -> SimulationSnapshot
  -> SnapshotReader
  -> SimulationWorld
```

## Durable State

`SimulationSnapshot` is the named durable state of the game. It stores players,
enemies, floor items, combat registry entries, and clickable targets. It does
not store transient event history, effect requests, frame timers, runtime input
sources, or debug output.

This keeps save/load from becoming a memory dump. The save boundary persists the
facts needed to rebuild a world, not every implementation detail that happened
to exist during a frame.

## Codec Layers

The byte path is intentionally split into small layers:

```text
SimulationSnapshot
  -> SnapshotSchemaCodec
  -> SnapshotByteWriter
  -> SnapshotFrameCodec
  -> SnapshotChecksum
  -> SnapshotBytes
```

`SnapshotCodec` is the outer public API. It turns a snapshot into framed bytes
and turns framed bytes back into a snapshot. It does not know the detailed field
layout of a player or enemy.

`SnapshotFrameCodec` owns the file-level envelope: magic bytes, format version,
payload bytes, and trailing checksum. This is the first trust boundary when
loading bytes.

`SnapshotSchemaCodec` owns section order. Today the durable sections are:

```text
players
enemies
floor items
combatants
targets
```

The schema codec also requires the payload to be fully consumed. Extra trailing
bytes are rejected, because accepting unknown data silently makes save format
mistakes harder to see.

`SnapshotVectorCodec` owns count-prefixed lists. It keeps list framing separate
from player, enemy, item, and target field encoders.

`SnapshotPlayerCodec` owns durable player fields: actor position, move state,
walk path, destination action, movement modifiers, animation lock, combat stats,
inventory, equipment, and movement speed.

`SnapshotEnemyCodec` owns durable enemy fields: id, actor position, move state,
tuning, combat stats, and state timer.

`SnapshotEntityCodec` owns reusable entity fragments: points, targets, actor
positions, combat stats, equipment modifiers, item fields, optional items,
destination actions, and combatants.

`SnapshotByteWriter` and `SnapshotByteReader` own primitive byte order. Current
snapshot integers and floats are written in little-endian order.

## Validation

Save validation happens in layers:

```text
bad magic/version      -> SnapshotFrameCodec rejects
checksum mismatch      -> SnapshotFrameCodec / SnapshotChecksum rejects
missing section        -> SnapshotSchemaCodec rejects
trailing payload bytes -> SnapshotSchemaCodec rejects
invalid enum           -> entity, player, or enemy codec rejects
truncated primitive    -> SnapshotByteReader rejects
truncated vector item  -> SnapshotVectorCodec rejects
```

This matters because a load failure should stop at the smallest boundary that
can explain the problem. A corrupt checksum should not reach player decoding.
An invalid equipment slot should not look like a filesystem error.

## File And Use-Case Boundaries

`SnapshotFileStore` only persists versioned snapshot bytes. It knows how to
write and read files, and it refuses unreadable or corrupt data. It does not
know how to interpret a player, enemy, or combatant.

`SaveGameService` is the game-facing use case. It coordinates `SnapshotWriter`,
`SnapshotFileStore`, and `SnapshotReader` so game/session code can save or load
a world without knowing about byte formats or file validation details.

`SaveSlotService` adds menu-facing slot behavior: listing metadata,
distinguishing empty/corrupt/valid slots, and loading or saving selected slots.

## Design Lesson

Saving is not just serialization. It is a contract between old bytes and future
game code. Small codecs make that contract easier to test:

```text
field codec       -> does this value round-trip?
actor codec       -> does durable actor state survive?
schema codec      -> are sections present and ordered?
frame codec       -> is this byte payload trustworthy?
file store        -> did persistence succeed?
save service      -> did the world save/load workflow complete?
```

Each layer has focused tests so failures point at the boundary that broke.
