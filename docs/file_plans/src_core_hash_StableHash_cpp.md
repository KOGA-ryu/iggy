# `src/core/hash/StableHash.cpp`

Updated: 2026-06-20

Exact purpose: implement deterministic non-cryptographic hashing for state hashes, replay verification, and fixture summaries.

## Build Position

- priority rank: 13
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `source`

## Ownership

This file owns:

- hash seed
- byte append rules
- primitive append helpers
- quantized float append policy

It must not own:

- no includes, links, generated code, or schema adapters from old `/Users/kogaryu/iggy`;
- no renderer-owned gameplay truth;
- no raw input events persisted as gameplay commands;
- no hidden global mutable state;
- no nondeterministic time, random, filesystem, or container-order behavior inside runtime logic.

## Allowed Dependencies

- C++ standard library only unless explicitly justified
- no `src/runtime`, `src/content`, `src/projection`, or app includes

## Data Contract

- 64-bit FNV-1a with fixed constants
- helpers for integers, bools, strings, ids, and quantized meters

## Semantics

- same logical state must hash identically on supported platforms
- floating point values are quantized to millimeters before hashing
- hash input order is explicitly sorted or stored order is deterministic

## Implementation Plan

1. include the paired header first;
2. implement every declared operation with deterministic ordering;
3. reject non-finite values before hashing in owning validators or state-hash
   visitors;
4. avoid hidden static mutable state and wall-clock reads.

## Compute Cost

- O(number of hashed fields plus bytes of strings).
- Cost is linear in explicitly appended bytes and values; no hidden traversal or
  platform state is allowed.

## Diagnostics And Errors

- hash helpers do not emit diagnostics;
- callers must validate non-finite floats before hashing;
- invalid hash input context is reported by the owning subsystem.

## Save Replay Multiplayer Notes

- `StableHash` produces deterministic hash values only; it is not itself save
  truth and emits no diagnostics.
- Owning subsystems such as `StateHash`, `SaveLoad`, `CommandReplay`, package
  validation, or tools may use hash values inside their own diagnostics.
- Runtime owners define which fields are appended and in what order.
- Multiplayer comparisons may use the value as proof, not as a cryptographic
  signature.

## Tests And Verification

- `math_tests` or hash-focused unit tests cover constants, byte ordering,
  string length hashing, quantized float rounding, ordered input differences,
  and no address-dependent hashing;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/hash/StableHash.cpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Implementation Algorithm
Use fixed 64-bit FNV-1a for the first build.

Required constants:
- offset basis `14695981039346656037`;
- prime `1099511628211`.

Byte append algorithm:

1. start every `StableHasher` at offset basis;
2. for each byte: `hash ^= byte`;
3. multiply by prime using unsigned 64-bit wraparound;
4. expose the current hash through `value()`.

### Float Quantization
`addFloatQuantized(value, scale)` must:
1. receive finite values only; owning validators reject non-finite values before
   hashing;
2. multiply by scale;
3. round to nearest signed integer deterministically;
4. hash the signed integer representation through `addI64`.

### String Hashing
Hash length and bytes in order. Do not rely on null termination.

### Tests
Math/hash tests must prove:
- same inputs produce same hash;
- insertion order changes hash when order matters;
- quantized `1.0004` and `1.0005` behavior is documented by tests;
- no hash helper uses memory addresses.
