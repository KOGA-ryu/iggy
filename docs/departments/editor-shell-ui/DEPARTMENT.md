# Editor Shell and Drafting UI

## Purpose

Own the shared workspace in which domain tools are discovered, configured,
observed, and tested. It presents domain state without becoming a second source
of business behavior.

## Owns

- Desktop shell, docking layout, menus, status bar, and common panel lifecycle.
- Shared outliner, inspector framework, history surface, diagnostics layout,
  widgets, selection presentation, and responsive sizing.
- Drafting glyph and style vocabulary.
- Generic toolbox and tool-presentation infrastructure.
- Keyboard focus, modal presentation, and panel visibility state.

## Does Not Own

- Building, terrain, asset, or gameplay semantics.
- Raw controller interpretation.
- Document mutations outside typed command dispatch.
- Projection geometry or Vulkan recording.

## Dependency Direction

Reads domain presentation models and emits semantic commands. Domain-specific
panels remain with their domain departments. No widget may directly mutate the
document or reproduce a domain recipe.

## Primary Owners

- Generic `apps/iggy3d_creative/EditorDesktop*` files
- `apps/iggy3d_creative/EditorDraftingStyle.*`
- Shared toolbox, glyph, frame, bootstrap, and shell owners
- `src/app/iggy3d/creative/ui/`

See [FILES.md](FILES.md) for the complete generated assignment.
