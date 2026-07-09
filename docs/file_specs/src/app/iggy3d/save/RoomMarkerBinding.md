# File Spec

Files: `src/app/iggy3d/save/RoomMarkerBinding.hpp`, `src/app/iggy3d/save/RoomMarkerBinding.cpp`

Verified at: `ac496769`

## Owns

- Binding authored-room markers from a loaded saved room into a runtime `Session`.
- `ProductSavedRoomMarkerBindingResult` receipt packet.
- Merge logic for marker-derived entities, objectives, and combatants.
- Baseline/hash refresh after marker binding mutates loaded session state.

## Does Not Own

- Save-file decoding or catalog flow.
- ASCII room package generation internals.
- Runtime entity/objective/combat system semantics beyond merge admission.
- Active-room loading or session launch orchestration.

## Reads

- `ProductActiveRoomState` loaded/authored-room facts and room id.
- Current `Session` state, identity, and state hash.
- Scenario seeds produced by `makeProductAsciiRoomPackage` and `buildProductPackageSessionSeed`.
- Runtime world, objectives, combat, and state-hash contracts.

## Writes / Mutates

- May replace session state through `Session::replaceStateFromLoad`.
- Adds missing non-player entities, objectives, and combatants to a candidate session state.
- Refreshes baseline world/combat/objectives and current hash in the candidate state.
- Writes only the binding result packet otherwise.

## Calls Out To / Wires Out To

- Calls `makeProductAsciiRoomPackage`.
- Calls `buildProductPackageSessionSeed`.
- Calls `computeStateHash`.
- Called by product session launch and recorded into save receipts.

## Called By / Entry Points

- `bindSavedRoomMarkersToSession`.
- `ProductSessionLaunch.cpp` records the binding result during launch.
- `SaveSessionStore.hpp` embeds the result in save/session state.
- Focused proof: `rg -n "bindSavedRoomMarkersToSession|ProductSavedRoomMarkerBindingResult|saved_marker_bind|RoomMarkerBinding" src/app/iggy3d src/runtime tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Unloaded or non-authored rooms return ok without requesting marker binding.
- Authored rooms with zero markers return `saved_marker_bind_no_markers`.
- Existing stable-name entities/objectives/combatants are counted, not duplicated.
- Invalid combatant seed data fails before replacing session state.
- Session replacement happens only after a candidate state is fully merged and baseline/hash refreshed.

## Tests / Proof Commands

- `product_saved_room_marker_binding_tests` covers applied binding, idempotent no-op binding, and skipped inactive inputs.
- Save receipt fields are exposed through `SaveStateFields.cpp`.
- `rg -n "product_saved_room_marker_binding_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/ascii_room/Package.*` unless marker package generation changes.
- `src/app/iggy3d/world/PackageSessionSeed.*` unless seed ownership changes.
- `src/runtime/session/Session.*` unless load replacement contracts change.
- `src/app/iggy3d/world/ProductSessionLaunch.*` unless launch timing changes.

## Update When

- Marker merge rules, binding result fields, session replacement timing, baseline/hash refresh, or launch/save receipt integration changes.

## Do Not Update When

- Only save catalog display, slot selection, or unrelated save-codec schema changes.
