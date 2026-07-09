# File Spec

Files: `src/runtime/ai/NpcBehaviorProfile.hpp`, `src/runtime/ai/NpcBehaviorProfile.cpp`

Verified at: `abe51abb`

## Owns

- Built-in NPC behavior profile catalog shape and lookup.
- `NpcBehaviorProfileId`, `NpcBehaviorProfile`, `NpcBehaviorProfileCatalog`, resolve request/result, and resolve status names.
- Profile-id validation and conversion from profile tuning fields into `NpcBehaviorConfig`.
- The bundled `default`, `melee_training`, and `passive` profile rows.

## Does Not Own

- Session tick scheduling, actor state mutation, package NPC assignment, or debug projection.
- Guard decision scoring, alert stepping, sound event production, or perception execution.
- Save/catalog persistence of profile assignments.

## Reads

- Profile rows supplied by a caller-owned `NpcBehaviorProfileCatalog`.
- `NpcBehaviorSystem` config validation, alert profile validation, sound config fields, and personality weights stored on profile rows.

## Writes / Mutates

- No external state.
- Returns resolved profile/config packets and deterministic reason codes.

## Calls Out To / Wires Out To

- Calls `isValidNpcBehaviorConfig(...)` and `isValidAlertProfile(...)` during resolve.
- `Session.cpp` builds the built-in catalog and resolves each actor profile before behavior decisions.
- `NpcBehaviorDebugSnapshot.*` resolves profiles for debug rows.
- App package/world code validates assignment ids with `isValidNpcBehaviorProfileId(...)`.

## Called By / Entry Points

- `resolveNpcBehaviorProfile(...)`, `makeBuiltInNpcBehaviorProfileCatalog(...)`, and `configFromNpcBehaviorProfile(...)` are the public runtime entry points.
- Grep proof: `rg -n "resolveNpcBehaviorProfile|makeBuiltInNpcBehaviorProfileCatalog|configFromNpcBehaviorProfile" src tests`.

## Invariants

- Profile ids are non-empty and limited to lower-case letters, digits, and underscore.
- Empty or null catalogs resolve as `CatalogEmpty`; missing ids resolve as `Missing`.
- Invalid behavior config or alert profile resolves as `InvalidConfig`.
- `configFromNpcBehaviorProfile(...)` copies behavior config fields only; sound and personality tuning remain profile-side.
- Built-in catalog ordering and names are deterministic.

## Tests / Proof Commands

- `rg -n "npc_behavior_profile_tests|stealth_tuning_readout_tests|npc_behavior_debug_snapshot_tests" cmake tests`.
- `rg -n "resolveNpcBehaviorProfile|NpcBehaviorProfileResolveStatus" src/runtime tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/NpcBehaviorSystem.*` unless behavior config fields change.
- `src/runtime/ai/NpcAlertSystem.*` unless alert profile validation changes.
- `src/runtime/session/Session.cpp` unless profile resolution timing or actor wiring changes.
- `src/app/iggy3d/world/NpcProfileAssignment.*` unless package assignment validation changes.

## Update When

- Profile fields, built-in profile rows, id validation, resolve status/reason codes, or behavior-config derivation changes.

## Do Not Update When

- A caller changes when it asks for a profile but the profile contract and resolve semantics stay the same.
