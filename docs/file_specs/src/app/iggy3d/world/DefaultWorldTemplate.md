# File Spec

Files: `src/app/iggy3d/world/DefaultWorldTemplate.hpp`, `src/app/iggy3d/world/DefaultWorldTemplate.cpp`

Verified at: `266066cb`

## Owns

- Product world template data packet defaults.
- Default product world template factory.
- Dev override product world template factory.

## Does Not Own

- Package path lookup.
- Package metadata loading.
- World creation validation.
- Runtime session creation.

## Reads

- Optional dev package path and scenario id for override template construction.

## Writes / Mutates

- Returns `ProductWorldTemplate` values.
- Does not mutate app/window/runtime state.

## Calls Out To / Wires Out To

- No external calls beyond string construction.

## Called By / Entry Points

- Product world template operations and world creation tests.
- Focused proof: `rg -n "defaultProductWorldTemplate|devOverrideProductWorldTemplate" src/app tests/unit`.

## Invariants

- Default packet identifies the built-in product world.
- Dev override uses the package path as package id when provided.
- Empty dev override scenario falls back to `default`.
- This file is a data/defaults seam; package probing belongs elsewhere.

## Tests / Proof Commands

- `rg -n "defaultProductWorldTemplate|devOverrideProductWorldTemplate" tests/unit/product_world_creation_tests.cpp src/app`.
- `rg -n "product_world_creation_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/world/ProductWorldTemplateOperations.*` unless option-based lookup changes.
- `src/content/PackageLoader.*` unless package metadata contracts change.
- `src/app/iggy3d/world/Creation.*` unless template fields consumed by creation change.

## Update When

- Template packet fields, default values, or dev override defaults change.

## Do Not Update When

- Only package lookup, save behavior, or world setup UI behavior changes without changing template defaults.
