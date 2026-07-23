# Interaction and Controls TODO

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| INT-001 | Navigate, inspect, and pointer capture | Integrated | Manual Test Needed | Camera, viewport capture, and layout tests exist | Run keyboard, touchpad, and PS5 navigation baseline |
| INT-002 | Selection and pick-block | Integrated | Needs Audit | Ordered selection and pick paths exist | Prove single, multi, stale, hidden, locked, and overlapping selection |
| INT-003 | Measurement | Integrated | Needs Audit | Measurement kernel and annotation paths exist | Complete snapping, persistent annotations, units, and edit behavior |
| INT-004 | Move, rotate, and scale | Integrated | Needs Audit | Shared transform commands and gizmo exist | Prove local and world axes, pivots, numeric entry, snapping, and grouped undo |
| INT-005 | Group, duplicate, clipboard, delete, undo, and redo | Integrated | Needs Audit | Shared commands and history exist | Verify deterministic selection and provenance through every operation |
| INT-006 | Semantic input parity | Integrated | Needs Audit | InputRouter and control persistence exist | Build a complete action-to-device matrix with no unresolved conflicts |
| INT-007 | Tool wheel and option sequencing | Integrated | Needs Audit | Radial wheel, square-cycle, and hints exist | Prove context stability, focus, cancellation, and discoverability |
| INT-008 | Gesture and repeat primitives | Stable Recipe | Needs Audit | Placement stroke and repeat kernels exist | Consolidate press, hold, release, interruption, and primary precedence |
| INT-009 | User-remappable controls | Prototype | Planned | Controls UI and persistence exist | Complete conflict resolution, reset, labels, and imported controller profiles |
