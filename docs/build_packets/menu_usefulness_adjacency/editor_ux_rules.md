# Editor UX Rules Plan

## Objective

Define practical, durable editor behavior for selection, transforms, grid snap, object types, lock/hidden states, and input rules. This is adjacent to Menu Usefulness v1 because editor controls must cooperate with the same input ownership and menu/dev access stack.

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/content/authoring/EditableRoomDocument.*`
- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.*`
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.*`
- `/Users/kogaryu/iggy3d/tests/unit/editable_room_document_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_editor_control_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_editor_manipulation_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_editor_save_load_smoke.cpp`

## Data Ownership

- `EditableRoomDocument` owns authored primitives and validation.
- `EditableRoomSession` owns undoable edit commands.
- Visual app owns cursor/probe/ghost/selection routing and receipts.
- Runtime room/collision/traversal are rebuilt outputs, not editor truth.
- Save file owns persisted authored room state.
- Renderer/HUD/dev tools display state only.

## Selection Rules

- Deterministic id selection remains the test path: `editor.select=<id>`.
- Visible UI may cycle selection later, but test/control path must not depend on ray hit order.
- Missing selected id returns `room_edit_missing_primitive`.
- Delete/load resets selection to `none` unless a specific restore rule is implemented and tested.
- Selected readout must include id, type, center/size or start/end/height/thickness.

## Object Types v1

- Floor: center, size, story index, semantics, hidden, locked.
- Wall: start, end, bottomY, height, thickness, story index, semantics, hidden, locked.
- Future objects such as props, anchors, lights, NPC markers, doors, and plugs are deferred.

## Grid and Snap

- Default grid snap: `0.25m` unless the current editor already has a stable snap value.
- Codex-control values are meters.
- App may translate world-space control values to room-local document values.
- Snap affects app input/cursor; document validation still rejects non-finite and invalid geometry.

## Transform Rules

- Move floor: change center X/Z, preserve Y/story/size/semantics.
- Resize floor: change size X/Z around center anchor, preserve thickness/Y/story.
- Move wall: translate start/end together in X/Z.
- Stretch wall: move one endpoint along the wall axis only.
- Rotate wall: 90-degree X/Z rotation around center if axis-aligned validation remains simple.
- Set height/thickness: positive finite values only.

Every transform must be undoable/redoable through content-authoring commands, not app-local mutation.

## Locked / Hidden / Disabled

- Locked primitives reject delete and transform with `room_edit_locked_primitive`.
- Hidden primitive policy must be explicit. Conservative default: hidden is still editable unless locked.
- Disabled/non-editable future states must use explicit status, not silent no-op.

## Input Semantics

Codex-control remains the primary deterministic authoring test path:

```text
editor.open=true|false
editor.tool=select|place_floor|place_wall|semantics|delete
editor.preset=floor|solid_wall|clamber_wall|projectile_wall
editor.cursor=<x>,<y>,<z>
editor.select=<id>
editor.apply_frames=<csv-frames>
editor.delete_frames=<csv-frames>
editor.undo_frames=<csv-frames>
editor.redo_frames=<csv-frames>
editor.transform_frames=<csv-frames>
editor.move_selected=<dx>,<dy>,<dz>
editor.resize_floor=<sx>,<sz>
editor.move_wall=<dx>,<dy>,<dz>
editor.stretch_wall_start=<x>,<y>,<z>
editor.stretch_wall_end=<x>,<y>,<z>
editor.rotate_wall_90=left|right
```

Keyboard/controller authoring can be added later after no-window controls prove behavior.

## Receipt Fields

```text
editor_open=true|false
editor_tool=<tool>
editor_selected_id=<id|none>
editor_selected_type=none|floor|wall
editor_last_command=<command>
editor_last_status=<status>
editor_last_transform_command=<command|none>
editor_last_transform_status=<status|none>
editor_transform_count=<integer>
editor_floor_count=<integer>
editor_wall_count=<integer>
editor_runtime_room_rebuilt=true|false
editor_runtime_surface_count=<integer>
editor_runtime_traversal_slot_count=<integer>
editor_authored_room_saved=true|false
editor_authored_room_loaded=true|false
window_launch_count=0
```

## No-Go Surfaces

- No pretty UI requirement.
- No Blender/import pipeline.
- No renderer/Vulkan changes.
- No full physics engine.
- No slope/clamber/vault/wire-walk expansion beyond preserving existing tags/slots.
- No JSON.

## Builder Packet Boundaries

Packet 1: Selection/transform rules, undo/redo, save/load round trip, no-window smoke.
Packet 2: Keyboard/controller editor manipulation once deterministic commands are green.
Packet 3: Add new authored object classes such as props/doors/anchors.

## Focused Tests

- Unit: transform commands validate, mutate, undo, redo.
- Smoke: create floor/wall, manipulate, save, load, verify geometry and rebuilt counts.
- Dev tools: World/Editor category shows selected id/type/readout.

## Open Questions

No blocking open question. Conservative default is Codex-control first, user-facing editor input later.
