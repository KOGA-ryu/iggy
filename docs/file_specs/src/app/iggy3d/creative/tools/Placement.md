# File Spec

Files: `src/app/iggy3d/creative/tools/Placement.hpp`, `src/app/iggy3d/creative/tools/Placement.cpp`

Verified at: `ff909047`

## Owns

- Pure creative create-placement offset policy.
- `CreativePlacedCreateRequest` packet wrapping a document create request plus offset proof fields.
- Default object placement offset along positive X based on document object count and document snap step.
- Coherent transform/bounds offset application for descriptor defaults.

## Does Not Own

- Document mutation application.
- Descriptor table contents.
- Palette slot selection.
- UI command dispatch.
- Snap validation beyond reading document snap step.
- Runtime room bake or rendering.

## Reads

- `CreativeDocument::objectCount()`.
- `CreativeDocument::documentSnapSettings()`.
- `describeObject(kind)` descriptor defaults and capability flags.

## Writes / Mutates

- Builds a `CreativePlacedCreateRequest`.
- Does not mutate the document or descriptor defaults.

## Calls Out To / Wires Out To

- Calls `describeObject(...)`.
- Used by creative UI command frame create handlers before facade document creation.
- Tests check creation placement through direct placement and product UI create paths.

## Called By / Entry Points

- `buildPlacedCreateRequest(...)`.
- Grep proof: `rg -n "buildPlacedCreateRequest|CreativePlacedCreateRequest|CreativePlacement" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Existing object count zero yields no offset.
- Unknown object descriptors yield no override offsets.
- Bounds defaults and transform defaults must move by the same offset when both exist.
- Offset proof flags must reflect exactly which overrides were applied.
- This helper prepares a request only; it must not mutate the document directly.

## Tests / Proof Commands

- `creative_placement_tests`.
- `product_creative_ui_command_frame_tests`.
- `creative_facade_mutation_tests`.
- `rg -n "creative_placement_tests|buildPlacedCreateRequest|CreativePlacedCreateRequest" cmake/iggy3d_tests.cmake tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/ObjectDescriptor.*` unless descriptor defaults or flags change.
- `src/app/iggy3d/creative/document/DocumentMutation.*` unless create request schema changes.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless create command flow changes.
- `src/app/iggy3d/creative/tools/Palette.*` unless palette selection and placement policy are deliberately coupled.

## Update When

- Create-placement offset policy, offset proof fields, transform/bounds coherence rules, or descriptor default consumption changes.

## Do Not Update When

- Only object creation execution, palette row labels, or render/bake output changes after the placed create request is built.
