# Assets and Object Composition

## Purpose

Own the complete path from discoverable asset identity to accurate placement,
composition, replacement, and reusable authored objects.

## Owns

- Static-mesh and authored-asset catalogs, import, reload, and stable IDs.
- Material, catalog asset, attachment, socket, and structural placement.
- Placement grids, snapping, clearance, orientation, contact, and ghost plans.
- Scatter, patterns, connected fill, extrusion, volume operations, and prefabs.
- Blender generation tools and checked-in Creative asset content.
- Future custom-object authoring, publishing, and replacement contracts.

## Does Not Own

- Generic input semantics.
- Building topology or terrain deformation.
- Low-level Vulkan resource management.
- Runtime gameplay behavior attached to placed objects.

## Dependency Direction

Calls Authoring Core for changes and history, Interaction and Controls for
semantic gestures, and Rendering and Preview for ghosts and catalog previews.
Building and Terrain may consume asset identities but must not duplicate import
or replacement policy.

## Primary Owners

- `src/app/iggy3d/creative/assets/`
- `src/app/iggy3d/creative/spatial/`
- Asset and volume files in `src/app/iggy3d/creative/tools/`
- `apps/iggy3d_creative/EditorAsset*`, `EditorCatalog*`, and placement owners
- `assets/` and asset-generation tools

See [FILES.md](FILES.md) for the complete generated assignment.
