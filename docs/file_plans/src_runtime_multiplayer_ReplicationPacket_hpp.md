# `src/runtime/multiplayer/ReplicationPacket.hpp`

Updated: 2026-06-20

Exact purpose: declare transport-neutral replicated command/session packet data.

## Build Position

- priority rank: 126
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `header`

## Ownership

This file owns:

- packet type enum
- sequence/ack fields
- command payload fields
- state hash payload fields

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- core/config/runtime peer headers according to ownership
- content seed data only at session creation boundaries
- no app, projection, renderer, tests, or old iggy includes

## Data Contract

- join hello, command batch, snapshot summary, ack, and reject packet values
- byte-independent value model

## Semantics

- packet data is not a socket
- serialization lives in `ReplicationCodec`
- packet command order is stable

## Detailed Design Contract

Declare:

```cpp
enum class ReplicationPacketKind : std::uint8_t {
  CommandSubmit,
  CommandAck,
  CommandReject,
  StateHashReport,
  SnapshotNotice,
  Heartbeat,
};

enum class ReplicationPacketStatus : std::uint8_t {
  Ok,
  InvalidPacket,
  UnsupportedVersion,
  PacketTooLarge,
  InvalidCommandId,
  DuplicateCommandId,
  InvalidSequence,
  InvalidSlot,
  InvalidRetrySource,
  InvalidTargetFlags,
  InvalidAdmissionState,
  InvalidStateHash,
};

struct ReplicationCommandRecord {
  CommandId commandId = kInvalidCommandId;
  CommandKind kind = CommandKind::None;
  CommandSequence sequence = kInvalidCommandSequence;
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
  EntityId actor;
  bool hasTargetEntity = false;
  EntityId targetEntity;
  bool hasTargetPoint = false;
  Vec3 targetPoint;
  CommandId retrySourceCommandId = kInvalidCommandId;
  CommandTick issuedTick = kInvalidCommandTick;
  CommandTick scheduledTick = kInvalidCommandTick;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
};

struct ReplicationPacket {
  std::uint64_t packetSequence = 0;
  ReplicationPacketKind kind = ReplicationPacketKind::Heartbeat;
  PlayerSlotId sourcePlayerSlot = kInvalidPlayerSlotId;
  CommandTick sourceTick = kInvalidCommandTick;
  std::string packageId;
  std::string scenarioId;
  std::vector<ReplicationCommandRecord> commands;
  CommandAdmissionStatus admission = CommandAdmissionStatus::Pending;
  CommandRejectionReason rejection = CommandRejectionReason::None;
  std::string stateHashHex;
};
```

Codec field mapping is exact:

- `packet.kind` maps to `ReplicationPacket::kind`;
- `packet.sequence` maps to `packetSequence`;
- `packet.sourceSlot` maps to `sourcePlayerSlot`;
- `packet.tick` maps to `sourceTick`;
- `package.id` maps to `packageId`;
- `scenario.id` maps to `scenarioId`;
- `command.count` maps to `commands.size()`;
- `command.0.commandId`, `command.0.kind`, `command.0.sequence`,
  `command.0.playerSlot`, `command.0.actor`, `command.0.hasTargetEntity`,
  `command.0.targetEntity`, `command.0.hasTargetPoint`,
  `command.0.targetPoint`, `command.0.retrySourceCommandId`,
  `command.0.issuedTick`, `command.0.scheduledTick`,
  `command.0.admission`, and `command.0.rejection` map to
  `ReplicationCommandRecord`;
- `admission.status` maps to packet-level `admission`;
- `rejection.reason` maps to packet-level `rejection`;
- `state.hash` maps to `stateHashHex` and is encoded as exactly 16 lowercase hex
  digits.

Command id ownership is locked for the first complete build:

- `ReplicationCommandRecord::commandId` carries a `CommandRecord::commandId`;
- `CommandSubmit` packets created from the local/session queue carry the already
  assigned session command id;
- client-proposed command ids require authority/session remapping before they
  can enter the first complete runtime path;
- `CommandAck` and `CommandReject` packet command ids are authoritative session
  command ids;
- `retrySourceCommandId` always references a `CommandRecord::commandId`, never a
  command sequence or vector index.

Packet-level admission/rejection is authoritative for `CommandAck` and
`CommandReject` packets. Per-command admission/rejection is authoritative for
`CommandSubmit` command records and must be preserved for replay diagnostics.
For single-command ack/reject packets, packet-level status must match
`commands[0]` when a command record is present; mismatch is `InvalidPacket`.

Packet validation ownership is exact:

- `ReplicationPacketStatus` validates already-constructed packet values, not
  serialized text grammar;
- unsupported packet value version maps to `UnsupportedVersion`;
- packet command count or encoded size over the first-build limit maps to
  `PacketTooLarge`;
- invalid or zero command ids map to `InvalidCommandId`;
- duplicate command ids inside one packet map to `DuplicateCommandId`;
- invalid, zero, duplicate, or non-monotonic command sequences inside the packet
  map to `InvalidSequence`;
- invalid source or command player slot maps to `InvalidSlot`;
- invalid retry source command id, retry source that points to a later command,
  or retry source that is not present in the packet when packet-local validation
  requires it maps to `InvalidRetrySource`;
- contradictory target flags, set target ids with `hasTargetEntity=false`, set
  target points with `hasTargetPoint=false`, or missing target data for commands
  that require it map to `InvalidTargetFlags`;
- pending/accepted/rejected admission and rejection pair mismatches map to
  `InvalidAdmissionState`;
- malformed packet-level state hash value maps to `InvalidStateHash`;
- ack/reject packet-level and single-command admission/rejection mismatch maps
  to `InvalidPacket`.

Invariants:

- packets are value objects only and never own sockets, channels, threads, or
  platform handles;
- command arrays preserve the already-merged order supplied by local/session
  authority; the codec never reconstructs order from tick, slot, or local
  sequence fields;
- command ids must be non-invalid and unique inside a packet when command
  records are present;
- packet sequence is transport-independent ordering data for future online play;
- no packet bypasses authority/admission; decoded commands still go through the
  normal path;
- packet values are not save truth in the first complete build.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(command payload count).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- packet value validation uses `ReplicationPacketStatus` plus the declared
  admission/rejection fields on `ReplicationPacket` and
  `ReplicationCommandRecord`.
- this header does not declare a separate diagnostic object for packet value
  validation.
- logging text is never the only machine-readable outcome.

## Save Replay Multiplayer Notes

- replication packets are value-only multiplayer/replay transport inputs, not
  save truth;
- decoded commands must pass through authority/admission before execution;
- packet state hash fields are comparison values and never mutate runtime state.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/ReplicationPacket.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
