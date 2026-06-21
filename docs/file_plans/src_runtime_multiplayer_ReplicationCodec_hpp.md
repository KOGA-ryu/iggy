# `src/runtime/multiplayer/ReplicationCodec.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic encoding/decoding of replication packets without owning transport.

## Build Position

- priority rank: 127
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `header`

## Ownership

This file owns:

- packet wire format
- version checks
- decode diagnostics
- round-trip tests

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

- stable field order
- bounded string lengths
- little-endian or text format explicitly documented

## Semantics

- codec does not apply commands
- invalid packet rejects before state mutation
- same packet values encode identically

## Detailed Design Contract

Declare `ReplicationEncodeResult encodeReplicationPacket(const
ReplicationPacket& packet)` and `ReplicationDecodeResult
decodeReplicationPacket(std::string_view bytes)`.

Declare codec statuses: `Ok`, `EncodeFailed`, `DecodeFailed`, `MissingField`,
`DuplicateKey`, `UnsupportedVersion`, `InvalidKind`, `InvalidSlot`,
`InvalidSequence`, `InvalidPacket`, `InvalidEnum`, and `PacketTooLarge`.

Declare exact result values:

```cpp
struct ReplicationEncodeResult {
  ReplicationCodecStatus status = ReplicationCodecStatus::Ok;
  ReplicationPacketStatus packetStatus = ReplicationPacketStatus::Ok;
  std::string encodedText;
  std::string diagnosticField;
  std::uint32_t diagnosticOffset = 0;
  std::size_t commandIndex = 0;
  std::string diagnostic;
};

struct ReplicationDecodeResult {
  ReplicationCodecStatus status = ReplicationCodecStatus::Ok;
  ReplicationPacketStatus packetStatus = ReplicationPacketStatus::Ok;
  ReplicationPacket packet;
  std::string diagnosticField;
  std::uint32_t diagnosticLine = 0;
  std::uint32_t diagnosticOffset = 0;
  std::size_t commandIndex = 0;
  std::string diagnostic;
};
```

Result validity:

- `status == Ok`: `encodedText` or decoded `packet` is valid, and
  `packetStatus == ReplicationPacketStatus::Ok`;
- parse/grammar failures use `ReplicationCodecStatus` values such as
  `MissingField`, `DuplicateKey`, `UnsupportedVersion`, `InvalidKind`,
  `InvalidSlot`, `InvalidSequence`, `InvalidEnum`, `DecodeFailed`, or
  `PacketTooLarge`; decoded packet output is invalid;
- packet-value validation failures use `status == InvalidPacket` and carry the
  exact `ReplicationPacketStatus` in `packetStatus`; decoded packet output is
  invalid for decode and encoded text is invalid for encode;
- non-`Ok` results keep payload fields ignored except for diagnostic
  field/line/offset/command-index context.

First complete build format is locked to deterministic tagged text. Binary
replication packets are explicitly out of scope for the first complete build.

Text rules:

- UTF-8 text;
- LF newlines;
- no trailing spaces;
- first line exactly `iggy3d.replication_packet.v1`;
- one ordered `key=value` field per following line;
- repeated command fields use deterministic indexes such as
  `command.0.commandId=...`, `command.0.kind=...`,
  `command.0.sequence=...`, and
  `command.0.playerSlot=...`.

Required field order after the version line:

1. `packet.kind`;
2. `packet.sequence`;
3. `packet.sourceSlot`;
4. `packet.tick`;
5. `package.id`;
6. `scenario.id`;
7. `command.count`;
8. indexed command record fields in command index order:
   `command.N.commandId`, `command.N.kind`, `command.N.sequence`,
   `command.N.playerSlot`, `command.N.actor`, `command.N.hasTargetEntity`,
   `command.N.targetEntity`, `command.N.hasTargetPoint`,
   `command.N.targetPoint`, `command.N.retrySourceCommandId`,
   `command.N.issuedTick`, `command.N.scheduledTick`,
   `command.N.admission`, and `command.N.rejection`;
9. `admission.status`;
10. `rejection.reason`;
11. `state.hash`.

Decode flow is exact:

1. parse deterministic tagged text;
2. return `ReplicationCodecStatus` for serialized grammar/field failures,
   including missing field, duplicate serialized key, unsupported text version,
   invalid integer, invalid enum/kind, malformed hash/id text, trailing data,
   malformed indexed command fields, and packet too large;
3. build a `ReplicationPacket` value only after parse succeeds;
4. run packet validation owned by `ReplicationPacketStatus`;
5. if packet validation fails, return codec status `InvalidPacket` and carry the
   exact packet validation status for diagnostics.

Encode flow is exact:

1. validate the constructed `ReplicationPacket` value before writing text;
2. if packet validation fails, return codec status `InvalidPacket`, carry the
   exact packet validation status, and emit no partial output;
3. if validation succeeds, write fields in the required order.

`state.hash` is exactly 16 lowercase hex digits. Command legality belongs to
authority/admission after decode, not this codec.

The codec owns bytes only: no command execution, no authority decisions, no
session mutation, no sockets, no reliability policy, no encryption/compression.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(packet bytes).
- Any future optimization must preserve deterministic ordering, command replay, and state hash behavior.

## Diagnostics And Errors

- use `ReplicationCodecStatus`, `ReplicationPacketStatus`, and diagnostic
  field/line/offset/command-index context for expected failures;
- do not use logging text as the only machine-readable outcome;
- surface enough context for acceptance and replay failures to identify the failing command, entity, or file.

## Save Replay Multiplayer Notes

- replication packets are not save truth and are not loaded into `SaveEnvelope`;
- decoded commands still pass through authority/admission before replay or
  runtime execution;
- state hash fields in packets are comparison values only.

## Tests And Verification

- paired unit or acceptance test listed in `PRIORITY.md` must cover this file where behavior is nontrivial;
- success and failure paths must be deterministic;
- no test may depend on renderer, wall-clock timing, network service, or old iggy code.

## Completion Criteria

- `src/runtime/multiplayer/ReplicationCodec.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
