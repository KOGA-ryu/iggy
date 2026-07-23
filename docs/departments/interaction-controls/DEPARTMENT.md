# Interaction and Controls

## Purpose

Own reusable human-input semantics. Features request actions such as confirm,
cancel, navigate, rotate, or adjust; they do not reinterpret raw keyboard,
touchpad, or controller inputs independently.

## Owns

- Input contexts, semantic action IDs, priority, chord consumption, and routing.
- Keyboard, mouse, touchpad, and gamepad bindings and persistence.
- View navigation, pointer capture, selection, transform, gizmos, measurements,
  tool wheel, action hints, and tool-option sequencing.
- Shared repeat, gesture, interruption, and confirm/cancel behavior.
- Conflict detection and controller ergonomics.

## Does Not Own

- Domain geometry or mutation policy.
- Generic desktop layout.
- Renderer implementation.
- Platform event acquisition below the semantic input boundary.

## Dependency Direction

May call Authoring Core and domain commands through semantic dispatchers. UI and
domain tools consume semantic actions. Raw SDL button or axis values must not
leak into domain policy.

## Primary Owners

- `src/app/iggy3d/creative/input/`
- Interaction-oriented files in `src/app/iggy3d/creative/tools/`
- `apps/iggy3d_creative/EditorControls*`, `EditorInteraction*`, gizmo,
  transform, selection, tool, and gamepad owners
- Shared frontend input declarations

See [FILES.md](FILES.md) for the complete generated assignment.
