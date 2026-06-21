# `src/core/result/Result.hpp`

Updated: 2026-06-20

Exact purpose: declare the small value-based success/failure model used where exceptions would hide deterministic control flow.

## Build Position

- priority rank: 9
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

## Ownership

This file owns:

- generic success/error status enum
- generic error payload conventions
- helper constructors

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

- `ResultStatus` values `Ok` and `Error`
- stable string `ErrorInfo::code` for generic/domain error names
- `Result<T>` value carrier
- `StatusResult` for APIs that carry no value

## Semantics

- no throwing for expected runtime failures
- callers must inspect failure before mutation
- error messages are stable enough for tests when part of acceptance

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- O(1) construction and move for small values; O(message bytes) only when carrying text.
- Cost scales with contained value movement and diagnostic string size only.

## Diagnostics And Errors

- `Result` carries stable `ErrorInfo::code` and secondary human text;
- domain-specific machine statuses live in domain enums, not in a core generic
  status enum;
- callers must inspect `status` before using `value`.

## Save Replay Multiplayer Notes

- `Result` and `StatusResult` are API return carriers, not save truth.
- Error codes appear in tool/app diagnostics; command replay relies on
  command status and rejection enums.
- No multiplayer identity or runtime state is stored here.

## Tests And Verification

- direct result tests are needed only for helper constructors or nontrivial
  accessors;
- package loader, app parser, session creation, and save/load tests cover
  result usage at API boundaries;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/result/Result.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned Types
Declare a small generic result carrier used at API boundaries:

```cpp
enum class ResultStatus : std::uint8_t { Ok, Error };

struct ErrorInfo {
  std::string code;
  std::string message;
};

template <typename T>
struct Result {
  ResultStatus status = ResultStatus::Error;
  T value{};
  ErrorInfo error{};
};
```

Declare the no-value result as:

```cpp
struct StatusResult {
  ResultStatus status = ResultStatus::Error;
  ErrorInfo error{};
};
```

Do not implement `Result<void>` in the first complete build. APIs with no
returned value use `StatusResult`.

### Semantics
- `Ok` means `value` is meaningful.
- `Error` means `error.code` is stable and machine-readable.
- Runtime legality failures such as `OutOfRange` remain command rejection reasons, not generic `Result` errors.
- Use `Result` for malformed files, invalid envelope decode, missing fixture, impossible internal state, and tool/app failures.
- Non-Ok `Result<T>` values are not authoritative. Callers must ignore `value`
  unless `status == ResultStatus::Ok`.
- `Result<T>` and `StatusResult` use normal C++ value semantics; copy/move
  behavior is whatever contained `T`, `std::string`, and `ErrorInfo` provide.

### Dependencies
Allowed: standard library strings and utility headers.
Forbidden: diagnostics module if it creates cycles, runtime modules, app code, filesystem IO.

### Save Replay Multiplayer
`Result` is not save truth. Stable `error.code` strings appear in tool output
and diagnostics. Command replay must rely on command status/rejection enums.

### Tests
Covered indirectly by package loader, session creation, save/load, and app parser tests. Add direct tests only if helper construction functions become nontrivial.
