# File Spec

Files: `src/app/iggy3d/ascii_room/Package.hpp`, `src/app/iggy3d/ascii_room/Package.cpp`

Verified at: `b49a78ae`

## Owns

- Inline package wrapper for a generated ASCII room `RoomAsset`.
- Scenario id derivation for product ASCII room runtime loops.
- Minimal `PackageLoadResult` construction for activation/new-world paths that already have an in-memory room asset.

## Does Not Own

- Package file loading, package validation, save persistence, package session seed rules, authored-room conversion, or runtime session creation.

## Reads

- `RoomAsset`, optional package id, and optional scenario id.

## Writes / Mutates

- Returns a `PackageLoadResult` with OK status, manifest fields, scenario id, and one room asset.
- Does not mutate app state, room asset input, files, or runtime session.

## Calls Out To / Wires Out To

- Used by activation, new-world launch, and saved room marker binding to feed package/session seed code.

## Called By / Entry Points

- `productAsciiRoomScenarioIdForRoom(...)`.
- `makeProductAsciiRoomPackage(...)`.
- Grep proof: `rg -n "makeProductAsciiRoomPackage|productAsciiRoomScenarioIdForRoom" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Empty room id derives the fallback scenario id.
- Empty package id uses the ASCII room authoring package id.
- Empty scenario id derives from the room id.
- This is an in-memory package adapter, not a general package loader.

## Tests / Proof Commands

- `rg -n "product_ascii_room_activation_tests|product_saved_room_marker_binding_tests|ProductNewWorldLaunch" cmake/iggy3d_tests.cmake tests src/app/iggy3d/world`.
- `rg -n "makeProductAsciiRoomPackage|productAsciiRoomScenarioIdForRoom" src tests`.

## Nearby Files Usually Not Touched

- `src/content/PackageLoader.*` unless generic package loading changes.
- `src/app/iggy3d/world/PackageSessionSeed.*` unless session seed creation changes.
- `src/app/iggy3d/ascii_room/Activation.*` unless activation needs different package fields.

## Update When

- Inline package manifest fields, scenario id derivation, room insertion, or consumer role changes.

## Do Not Update When

- Only authored-room conversion, package seed internals, or runtime session creation changes.
