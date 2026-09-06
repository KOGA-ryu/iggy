# Building and World Layout

## Purpose

Own building semantics from authored floor-plan intent through generated 3D
structure and its 2D plan and elevation representation.

## Owns

- Building blockouts, templates, rooms, partitions, and topology.
- Storeys, floor and ceiling slabs, exterior shells, and dimensions.
- Doors, windows, apertures, stairs, ramps, and roofs.
- Building source provenance, reconciliation, repair, and traversal checks.
- World Layout plan, elevation, hierarchy, properties, and domain-specific UI.

## Does Not Own

- Generic desktop docking and widgets.
- Terrain heightfields except building-to-terrain reconciliation.
- Generic asset catalog behavior.
- Runtime door or movement simulation after playtest preparation.

## Dependency Direction

Calls Authoring Core for document changes and provenance, Terrain and Site for
grounding/reconciliation, Assets and Object Composition for catalog identities,
and Rendering and Preview for presentation. UI must not independently recreate
building geometry.

## Exclusive Decisions

- Physical floor-to-floor height and its conversion between architectural
  meters and document grid cells.
- Finished-floor datums, slab anchor planes, clear height, facade height, and
  vertical-connector rise.
- Which authored building source produces each generated structural object.

Editor controls may collect profile or Custom values, but they must request
these decisions from Building and World Layout. Building recipes, rendering,
collision, persistence, and playtest may consume resolved geometry; they must
not independently infer storey elevation or reinterpret its units.

## Primary Owners

- `src/app/iggy3d/creative/world/WorldLayout*`
- Building-specific files in `src/app/iggy3d/creative/recipes/`
- `apps/iggy3d_creative/EditorWorldLayout*`
- `apps/iggy3d_creative/EditorDesktopWorldLayout*`

See [FILES.md](FILES.md) for the complete generated assignment.
