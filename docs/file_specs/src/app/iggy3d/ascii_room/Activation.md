# File Spec

Files: `src/app/iggy3d/ascii_room/Activation.hpp`, `src/app/iggy3d/ascii_room/Activation.cpp`, `src/app/iggy3d/ascii_room/AsciiRoomActivationState.hpp`

Verified at: `b49a78ae`

## Owns

- Product ASCII room preview activation into an active runtime `Session`.
- `ProductAsciiRoomActivationResult` and `ProductAsciiRoomActivationState` fields for activation status, package/scenario ids, entity counts, and runtime hash proof.
- Recording activation results into `window.creativeAuthoring.asciiRoomActivation`.

## Does Not Own

- ASCII source parsing, grid construction, authored-room conversion, room-asset baking, package seed rules, session internals, gameplay command execution, or receipt formatting.

## Reads

- `ProductAppWindowState::creativeAuthoring.asciiRoomDraft`.
- Preview authoring result from `buildProductAsciiRoomAuthoring(...)`.
- Active-room state, generated package, package session seed, and created session state hash.

## Writes / Mutates

- Rebuilds active room from ASCII authoring and bumps active-room revision.
- Refreshes active-room collision before and after session creation.
- Replaces `activeSession` on success.
- Sets `window.gameplay.runtimeSessionCreated`, `window.gameplay.gameplayActive`, frontend launch status fields, and activation mirror fields.

## Calls Out To / Wires Out To

- `productAsciiRoomAuthoringRequestFromDraft(...)`.
- `buildProductAsciiRoomAuthoring(...)`.
- `recordProductAsciiRoomPreview(...)`.
- `buildProductActiveRoomFromAsciiAuthoring(...)`.
- `makeProductAsciiRoomPackage(...)`.
- `buildProductPackageSessionSeed(...)`.
- `Session::create(...)`.
- `ensureActiveRoomCollisionFresh(...)`.

## Called By / Entry Points

- `activateProductAsciiRoomPreview(...)`.
- Automation dispatch and unit window support call this surface.
- Grep proof: `rg -n "activateProductAsciiRoomPreview|ProductAsciiRoomActivation|asciiRoomActivation" src tests cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Failed activation records a status/reason and must not pretend a session was created.
- Preview state is recorded before activation proceeds into package/session creation.
- Active-room collision refresh must follow active-room rebuild and successful session install.
- `gameplayActive` is set only after successful session creation.
- Runtime hash proof comes from the installed session.

## Tests / Proof Commands

- `rg -n "product_ascii_room_activation_tests|product_ascii_authoring_smoke" cmake/iggy3d_tests.cmake tests`.
- `rg -n "ascii_room_activated|asciiRoomActivation\\.runtimeHash|sessionCreated" tests/unit/product_ascii_room_activation_tests.cpp tests/smoke/product_ascii_authoring_smoke.cpp`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/Authoring.*` unless preview pipeline outputs change.
- `src/app/iggy3d/world/PackageSessionSeed.*` unless package seed semantics change.
- `src/runtime/session/Session.*` unless session creation API changes.

## Update When

- Activation flow, result/state fields, active session install behavior, collision refresh points, or gameplay launch proof changes.

## Do Not Update When

- Only ASCII parsing, room asset baking, package seed internals, receipt field ordering, or gameplay controller behavior changes.
