# `src/core/diagnostics/Diagnostic.hpp`

Updated: 2026-06-20

Exact purpose: declare structured diagnostics that explain package validation, command rejection, save/load, replay, and app failures.

## Build Position

- priority rank: 10
- tier: Tier 1: Core Contracts
- module: `core`
- file kind: `header`

## Ownership

This file owns:

- diagnostic severity
- domain
- stable code
- message text
- file/line/column location fields

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

- `DiagnosticSeverity` info, warning, error
- `DiagnosticDomain` content, runtime, command, save, replay, app
- `Diagnostic` with severity, domain, stable code, message, and source
  location

## Semantics

- diagnostics are append-only facts
- diagnostics do not mutate runtime state
- acceptance tests assert stable code values rather than full prose
- domain-specific context such as entity id, command id, tick, player slot, or
  objective id lives in typed runtime result/event structs; core diagnostics
  remain generic to avoid dependency cycles.

## Implementation Plan

1. declare only the public API, value types, enums, and function signatures owned by this file;
2. keep inline behavior limited to trivial constexpr/value helpers;
3. include the minimum headers required for complete type declarations;
4. document invariants in names and type shape rather than comments wherever possible.

## Compute Cost

- Construction cost is O(code bytes + message bytes + file path bytes).
- Copy/move cost is standard string/vector value cost only.

## Diagnostics And Errors

- `Diagnostic::code` is the machine-readable contract.
- `Diagnostic::message` is human-readable secondary text.
- Diagnostics explain failures; they do not decide command admission or mutate
  runtime state.

## Save Replay Multiplayer Notes

- `Diagnostic` is not authoritative save truth.
- Tool outputs, validation results, and replay reports include diagnostics when
  failures occur.
- Command replay and multiplayer authority use domain status/rejection enums,
  not diagnostic prose.

## Tests And Verification

- package loader, validator, app parser, save/load, and replay tests assert
  stable diagnostic codes where failure behavior is part of the contract;
- direct diagnostic tests are needed only if helper constructors become
  nontrivial;
- tests must not depend on renderer, wall-clock timing, network service, or old
  iggy code.

## Completion Criteria

- `src/core/diagnostics/Diagnostic.hpp` exists in `/Users/kogaryu/iggy3d`;
- it builds without old `iggy` dependencies;
- its behavior is covered by the ranked test or acceptance file;
- it follows `docs/architecture.md` and `docs/ownership.md` without redefining ownership.

## Detailed Contract

### Owned Types
Declare diagnostic severity, domain, stable code, location, and message fields:

```cpp
enum class DiagnosticSeverity : std::uint8_t {
  Info,
  Warning,
  Error,
};

enum class DiagnosticDomain : std::uint8_t {
  Core,
  Content,
  Runtime,
  Command,
  Save,
  Replay,
  App,
};

struct DiagnosticLocation {
  std::string file;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
};

struct Diagnostic {
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  DiagnosticDomain domain = DiagnosticDomain::Core;
  std::string code;
  std::string message;
  DiagnosticLocation location;
};
```

Required helpers:

```cpp
Diagnostic makeDiagnostic(
    DiagnosticDomain domain,
    DiagnosticSeverity severity,
    std::string code,
    std::string message,
    DiagnosticLocation location = {});

bool hasLocation(const Diagnostic& diagnostic);
```

### Semantics
- `code` is the machine-readable contract.
- `message` is human-readable and not used for tests except smoke checks.
- File locations use empty `file` and line/column `0` when absent; content
  parsers fill stable source paths and 1-based line/column where known.
- Runtime command rejection reasons are not diagnostics unless projected into tool output.

### Dependencies
Allowed: standard library only.
Forbidden: content parser, runtime, app, projection, renderer, old iggy.

### Tests
Package validator and app tools must assert codes for bad package/scenario fields.
