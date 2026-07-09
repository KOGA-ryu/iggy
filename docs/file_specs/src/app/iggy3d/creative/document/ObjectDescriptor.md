# File Spec

Files: `src/app/iggy3d/creative/document/ObjectDescriptor.hpp`, `src/app/iggy3d/creative/document/ObjectDescriptor.cpp`

Verified at: `1ffca70b`

## Owns

- Creative object-kind policy table.
- Object categories, profiles, shape kinds, projection profiles, occupancy kinds, runtime anchor semantics, palette visibility, dirty flags, defaults, and descriptor packets.
- Descriptor lookup, descriptor spans, capability predicates, dirty flag selection, and mutation allowance gates.
- String conversions for descriptor policy enums.

## Does Not Own

- Raw `CreativeObject` storage.
- Object mutation payload application.
- Document id allocation, parent graph storage, or revision increments.
- Bake/projection/render algorithms that consume descriptor policy.
- Save file schema.

## Reads

- `CreativeObjectKind` and mutation kind categories from mutation helpers.
- Static descriptor sections grouped by object category.
- Capability flags and object defaults embedded in the descriptor table.

## Writes / Mutates

- Returns descriptor references and spans over static descriptor storage.
- Computes dirty flag masks for creation and mutation.
- Does not mutate runtime document state.

## Calls Out To / Wires Out To

- Calls mutation category helpers such as transform, shape, relationship, logic, navigation, testing, sensory, and gameplay mutation predicates.
- Called by document creation, mutation admission, room bake classification, spatial projection, palette/tools, save/load, and tests.

## Called By / Entry Points

- `describeObject(...)`.
- `allObjectDescriptors()`.
- `dirtyFlagsForCreation(...)`.
- `dirtyFlagsForMutation(...)`.
- `descriptorAllowsMutation(...)`.
- `objectShowsInAuthoringBrushPalette(...)`.
- Grep proof: `rg -n "describeObject|allObjectDescriptors|dirtyFlagsForCreation|descriptorAllowsMutation|objectShowsInAuthoringBrushPalette|ObjectDescriptor" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Every concrete object kind must have one descriptor row or fall back intentionally to `Unknown`.
- Descriptor rows are the object policy source of truth; do not add parallel per-kind policy tables in UI or bake code.
- Palette visibility is descriptor data.
- Runtime anchor semantic strings must stay compatible with bake reachability and room asset anchor consumers.
- Dirty flags must include serialization/preview/identity where needed for document and UI proof.
- Mutation allowance must combine generic mutation support with descriptor capabilities.

## Tests / Proof Commands

- `creative_object_descriptor_tests`.
- `creative_document_mutation_tests`.
- `creative_document_room_bake_tests`.
- `rg -n "creative_object_descriptor_tests|creative_document_mutation_tests|creative_document_room_bake_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/Object.*` unless object kinds or serialized ids change.
- `src/app/iggy3d/creative/mutation/Mutation.*` unless mutation kind taxonomy changes.
- `src/app/iggy3d/creative/adapters/RoomBake.*` unless runtime bake policy changes.
- `src/app/iggy3d/creative/tools/Palette.*` unless palette consumption changes.

## Update When

- Descriptor rows, categories, profiles, shape/projection/occupancy policy, runtime anchor semantics, dirty flags, defaults, palette visibility, or mutation admission changes.

## Do Not Update When

- Only a downstream consumer changes how it uses an unchanged descriptor contract.
