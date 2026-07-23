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

## Primary Owners

- `src/app/iggy3d/creative/world/WorldLayout*`
- Building-specific files in `src/app/iggy3d/creative/recipes/`
- `apps/iggy3d_creative/EditorWorldLayout*`
- `apps/iggy3d_creative/EditorDesktopWorldLayout*`

See [FILES.md](FILES.md) for the complete generated assignment.
