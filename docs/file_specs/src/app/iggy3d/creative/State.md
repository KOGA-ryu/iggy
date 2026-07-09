# File Spec

File: `src/app/iggy3d/creative/State.hpp`

Verified at: `cd6c9663`

## Owns

- Small creative app state mirror made from core flags, tool, frame, hovered target, and selected target.
- Default state shape used by facade/core tests and UI-facing state mirrors.

## Does Not Own

- Authoritative creative document state.
- Detailed tool dispatch state.
- Selection receipts or mutation cleanup.
- Product window/app shell state.
- Save/load identity.

## Reads

- Core creative packet types from `Core.hpp`.

## Writes / Mutates

- No behavior; callers mutate `State` fields directly.

## Calls Out To / Wires Out To

- `creative::Facade` exposes and updates this state as a compatibility mirror around richer tool/selection state.
- Tests assert the default state and facade mirror stay stable.

## Called By / Entry Points

- Aggregate construction of `creative::State`.
- Grep proof: `rg -n "cr::State|creative::State|facade\\.state\\(\\)|State state" src/app/iggy3d/creative tests/unit --glob '*.{hpp,cpp}'`.

## Invariants

- Default state is disabled, inactive, clean, `Tool::Select`, frame zero, and invalid hovered/selected refs.
- This mirror must not become the authoritative document, tool, or selection store.
- Keep this packet cheap and dependency-light.

## Tests / Proof Commands

- `creative_core_tests`.
- `creative_facade_tests`.
- `rg -n "creative_core_tests|creative_facade_tests|facade\\.state\\(\\)" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/Core.hpp` unless primitive refs/flags change.
- `src/app/iggy3d/creative/Facade.*` unless mirror update policy changes.
- `src/app/iggy3d/creative/tools/Select.*` unless selected/hovered target ownership changes.

## Update When

- `State` fields, default values, facade mirror semantics, or selected/hovered target meaning changes.

## Do Not Update When

- Only document mutation, UI presentation, or product frame routing changes without changing this mirror packet.
