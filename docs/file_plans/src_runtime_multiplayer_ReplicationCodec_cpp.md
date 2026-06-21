# `src/runtime/multiplayer/ReplicationCodec.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic encoding/decoding of replication packets without owning transport.

## Build Position

- priority rank: 128
- tier: Tier 7: Durability Replay Multiplayer
- module: `src/runtime/multiplayer`
- file kind: `source`

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

Implementation algorithm:

1. before encoding, validate the constructed `ReplicationPacket` with packet
   validation rules owned by `ReplicationPacketStatus`;
2. if packet validation fails, return codec status `InvalidPacket`, carry the
   exact packet validation status, and emit no partial output;
3. write `iggy3d.replication_packet.v1` as the first line;
4. encode `packet.kind`, `packet.sequence`, `packet.sourceSlot`,
   `packet.tick`, `package.id`, `scenario.id`, and `command.count` in that
   order;
5. encode command payloads in deterministic index order using keys such as
   `command.0.commandId`, `command.0.kind`, `command.0.sequence`,
   `command.0.playerSlot`, `command.0.actor`,
   `command.0.hasTargetEntity`, `command.0.targetEntity`,
   `command.0.hasTargetPoint`, `command.0.targetPoint`,
   `command.0.retrySourceCommandId`, `command.0.issuedTick`,
   `command.0.scheduledTick`, `command.0.admission`, and
   `command.0.rejection`;
6. encode `admission.status`, `rejection.reason`, and `state.hash` last;
7. during decode, reject duplicate keys, missing required keys, unsupported text
   version, invalid enum names, malformed ids/sequences, packet too large,
   trailing data, command-count/key inconsistencies, and invalid command indexes
   with `ReplicationCodecStatus`;
8. build the packet value only after parse succeeds;
9. run packet validation; duplicate command ids, invalid command ids,
   packet-level ack/reject mismatch, invalid retry source, invalid target flags,
   invalid packet state hash, or invalid admission/rejection pair returns codec
   status `InvalidPacket` and the exact `ReplicationPacketStatus`;
10. return decoded packet values without touching `Session`, `Authority`, or
    `CommandLog`.

Diagnostics include packet sequence, field name, command index, line/offset,
codec status, and packet validation status when codec status is `InvalidPacket`.
`ReplicationEncodeResult` and `ReplicationDecodeResult` populate the exact
fields declared by `ReplicationCodec.hpp`; non-`Ok` results leave encoded text
or decoded packet ignored except for diagnostic field/line/offset/command-index
context.
The codec is allowed to be used in unit tests and future tools without any
network service.

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. return structured status/diagnostics for expected failures;
4. avoid hidden static mutable state and wall-clock reads.

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

- `src/runtime/multiplayer/ReplicationCodec.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.
