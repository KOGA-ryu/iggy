# File Spec

File: `src/content/PackageManifest.hpp`

Verified at: `33a36cc1`

## Owns

- `PackageManifest` and `PackageAssetRef`, the lightweight manifest packet shared by package loading, validation, and in-memory package adapters.

## Does Not Own

- Manifest parsing, path validation, asset file loading, scenario parsing, package validation, or runtime session creation.

## Reads

- No runtime data directly; this header defines packet shape only.

## Writes / Mutates

- No functions mutate state here.
- Loaders and app adapters populate the packet.

## Calls Out To / Wires Out To

- No calls.
- Included by `PackageLoader`, `PackageValidator`, and ASCII room package adapter code.

## Called By / Entry Points

- Grep proof: `rg -n "PackageManifest|PackageAssetRef" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Manifest carries package id, schema version, required runtime schema, scenario path, and asset refs only.
- It is content metadata, not runtime session state or save state.

## Tests / Proof Commands

- `rg -n "PackageManifest|PackageAssetRef|package_loader_tests" cmake/iggy3d_tests.cmake tests/unit src/content`.

## Nearby Files Usually Not Touched

- `src/content/PackageLoader.*` unless parsing changes.
- `src/content/PackageValidator.*` unless validation changes.
- `src/app/iggy3d/ascii_room/Package.*` unless in-memory manifest construction changes.

## Update When

- Manifest fields or ownership move/change.

## Do Not Update When

- Only parser, validator, or session seed logic changes without changing manifest shape.
