# Terrain and Site

## Purpose

Own authored ground shape and site infrastructure, from deterministic terrain
recipes through contours and integration with buildings and placed assets.

## Owns

- Terrain fields, generation, region operations, sculpting, grading, painting,
  profiles, paths, stamps, and contours.
- Roads, paths, cliffs, plateaus, terraces, watercourses, bridges, and retaining
  edges.
- Terrain masks, previews, source provenance, and site reconciliation.
- Terrain-specific drafting and inspector surfaces.

## Does Not Own

- Building shells or room topology.
- Generic object placement and catalog import.
- Renderer implementation.
- Runtime water simulation or erosion unless explicitly promoted from deferred
  research into a product capability.

## Dependency Direction

Calls Authoring Core for recipe application and history, Building and World
Layout for controlled reconciliation, Assets and Object Composition for kit
placement, and Rendering and Preview for visual output.

## Primary Owners

- `src/app/iggy3d/creative/tools/Terrain*`
- Terrain and site files in `src/app/iggy3d/creative/recipes/`
- `apps/iggy3d_creative/EditorTerrain*`
- Terrain-specific `EditorWorldLayout*` and desktop command files

See [FILES.md](FILES.md) for the complete generated assignment.
