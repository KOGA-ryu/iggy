# Rendering and Preview

## Purpose

Own faithful, performant visual communication from authored state to the 3D
viewport, drafting overlays, captures, and GPU submission.

## Owns

- Scene projection, preview proxies, overlays, wireframes, and render roles.
- Revision-based scene caches and bounded preview frame data.
- Placement ghosts, held-object presentation, invalid-state feedback, and
  selection visualization.
- Mesh generation, render resources, shader contracts, Vulkan recording, and
  capture stability.
- Render diagnostics and resource-budget policy.

## Does Not Own

- Authoring semantics or mutation decisions.
- Tool input bindings.
- Domain geometry recipes.
- Persisted document state for transient previews.

## Dependency Direction

Consumes read-only domain and document snapshots. It may call Foundation and
Build plus renderer internals. It must not write Creative document state or
feed preview-only geometry into room signatures, collision, or persistence.

## Primary Owners

- `src/render/`, `src/projection/`, and `shaders/`
- `src/app/iggy3d/creative/render/` and overlay owners
- Preview, overlay, wireframe, culling, scene-cache, and capture files in the
  Creative app

See [FILES.md](FILES.md) for the complete generated assignment.
