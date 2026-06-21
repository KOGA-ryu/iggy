# `src/core/hash/StableHash.hpp`

Updated: 2026-06-20

Exact purpose: declare deterministic non-cryptographic hashing for state hashes, replay verification, and fixture summaries.

## Build Position

- priority rank: 12
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

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

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

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

- `src/core/hash/StableHash.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Required Header Shape

```cpp
#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "core/math/Vec3.hpp"
```

All declarations live in `namespace iggy3d`.

### Owned Types And Functions
Declare deterministic 64-bit FNV-1a hash primitives:

```cpp
using StableHashValue = std::uint64_t;

inline constexpr StableHashValue kStableHashOffsetBasis = 14695981039346656037ULL;
inline constexpr StableHashValue kStableHashPrime = 1099511628211ULL;

struct StableHasher {
  StableHasher();
  void addByte(std::uint8_t value);
  void addU64(std::uint64_t value);
  void addI64(std::int64_t value);
  void addBool(bool value);
  void addString(std::string_view value);
  void addFloatQuantized(float value, float scale = 1000.0F);
  StableHashValue value() const;
};
```

Also declare:

```cpp
StableHashValue stableHashBytes(std::span<const std::uint8_t> bytes);
void addEnumByte(StableHasher& hasher, std::uint8_t value);
void addVec3Quantized(StableHasher& hasher, Vec3 value, float scale = 1000.0F);
```

### Semantics
- Hashing is deterministic across platforms for the accepted value domain.
- Floats are quantized before hashing; no raw binary float hashing.
- Ordered containers must hash in deterministic iteration order.
- Hash algorithm is 64-bit FNV-1a. It must not be changed without updating this
  file, replay/state-hash docs, and tests in the same patch.

### Dependencies
Allowed: standard fixed-width integer/string/span headers and `core/math/Vec3.hpp`.
Forbidden: runtime systems, filesystem, random, platform APIs.

### Save Replay Multiplayer
State hash is used to compare save/load and replay results. It is diagnostic proof, not a cryptographic signature.
