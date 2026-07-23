# Authoring Core

## Purpose

Own the deterministic state-changing center of Creative. UI, controllers, and
2D drafting surfaces request semantic operations; this department validates and
applies them to the document with explicit receipts, history, and provenance.

## Owns

- Creative document identity, objects, sections, and revisions.
- Atomic mutation batches, rollback behavior, and mutation receipts.
- Undo and redo history contracts.
- Shared semantic recipe interfaces and application.
- Provenance connecting generated output to its authored source.
- Cross-domain world-authoring orchestration that is not itself building,
  terrain, asset, or gameplay policy.

## Does Not Own

- ImGui widgets or input bindings.
- Building, terrain, or asset-specific geometry policy.
- Renderer resources.
- Runtime AI, physics, or playtest behavior.

## Dependency Direction

May call Foundation and Build. It may use domain recipe contracts, but domain
departments must not bypass mutation and history ownership when changing the
document. UI and interaction departments call this department through semantic
commands or recipes.

## Primary Owners

- `src/app/iggy3d/creative/document/`
- `src/app/iggy3d/creative/mutation/`
- `src/app/iggy3d/creative/history/`
- Shared files in `src/app/iggy3d/creative/recipes/`
- `src/app/iggy3d/creative/world/WorldService.*`

See [FILES.md](FILES.md) for the complete generated assignment.
