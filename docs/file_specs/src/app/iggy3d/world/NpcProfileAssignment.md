# File Spec

Files: `src/app/iggy3d/world/NpcProfileAssignment.hpp`, `src/app/iggy3d/world/NpcProfileAssignment.cpp`

Verified at: `10135e4e`

## Owns

- Product NPC behavior profile assignment packet and table validation.
- Assignment status names and resolve results.
- Default behavior profile resolution for unassigned NPC stable names.

## Does Not Own

- Runtime AI behavior profile definitions.
- Package/session seed construction.
- NPC behavior execution.
- Authoring UI for assignments.

## Reads

- Assignment table rows and requested NPC stable entity name.
- Runtime NPC behavior profile id validity through `isValidNpcBehaviorProfileId(...)`.

## Writes / Mutates

- Returns validation and resolution result packets.
- Does not mutate tables, packages, runtime state, or app/window state.

## Calls Out To / Wires Out To

- `runtime/ai/NpcBehaviorProfile.hpp` for behavior profile id validation.

## Called By / Entry Points

- `src/app/iggy3d/world/PackageSessionSeed.cpp` validates authored/explicit profile assignments and resolves NPC profile ids during seed construction.
- Tests call validation and resolution functions directly.
- Focused proof: `rg -n "validateProductNpcProfileAssignments|resolveProductNpcProfileAssignment|ProductNpcProfileAssignment" src/app tests/unit`.

## Invariants

- Empty stable entity names are invalid.
- Invalid behavior profile ids reject the whole assignment table.
- Duplicate stable entity names reject the table.
- Missing or empty assignment table resolves valid entities to the default profile.
- Unmatched valid entities also resolve to the default profile.

## Tests / Proof Commands

- `rg -n "product_npc_profile_assignment_tests|product_package_session_seed_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "InvalidProfileId|DuplicateEntityName|Defaulted" tests/unit/product_npc_profile_assignment_tests.cpp src/app/iggy3d/world`.

## Nearby Files Usually Not Touched

- `src/runtime/ai/NpcBehaviorProfile.*` unless valid profile id vocabulary changes.
- `src/app/iggy3d/world/PackageSessionSeed.*` unless assignment consumption changes.
- `src/content/*` unless package-authored AI actor schema changes.

## Update When

- Assignment packet fields, validation rules, defaulting policy, status names, or runtime profile id compatibility changes.

## Do Not Update When

- Only AI behavior execution or package seed entity synthesis changes without changing assignment contracts.
