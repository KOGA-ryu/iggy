# Dev Tools Taxonomy Plan

## Objective

Define exact dev tools categories, readouts, and allowed commands so Dev Tools becomes useful without becoming a hidden gameplay authority. This is adjacent to Menu Usefulness v1 because menus now provide access to dev tools, but each category needs a stable purpose.

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.*`
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.*`
- `/Users/kogaryu/iggy3d/src/projection/debug/*`
- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/dev_tools_menu_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_menu_usefulness_smoke.cpp`

## Data Ownership

- Dev tools own category model, selected category, row labels, read-only versus command-capable classification.
- App owns command execution and runtime/editor access.
- Runtime/editor systems own actual state and mutations.
- Debug projection/HUD owns visible rendering of selected readouts.

## Category Order and Content

1. Runtime
   - Shows lifecycle, outcome, tick, command counts, current state hash.
   - Commands: none v1.
2. Input
   - Shows input owner, input backend, gamepad availability/name, last UI action.
   - Commands: none v1.
3. Movement
   - Shows player position, grounded state, slope state, traversal attempt/reason.
   - Commands: none v1.
4. World
   - Shows world entity count, active key/dummy state, room id/source.
   - Commands: none v1.
5. Editor
   - Shows editor open/tool/selected id/type, floor/wall counts, last edit status, runtime surface/traversal counts.
   - Commands: open editor if current app already supports it; no direct geometry mutation from dev tools v1.
6. Renderer
   - Shows backend, window mode, draw count, frames presented, debug HUD status.
   - Commands: none v1.
7. Save
   - Shows save root, selected save id, compatible/corrupt counts, last save/load/delete status.
   - Commands: refresh save list only if safe and non-mutating.
8. Combat
   - Shows combatant count, training dummy HP/defeated state, last attack status.
   - Commands: none v1.
9. Physics Later
   - Placeholder category disabled until movement/physics packet lands.

If existing enum names are Session/Player/Collision/Spells/Camera/Performance, map them to this taxonomy through labels and receipts without breaking existing tests, then migrate names in a later cleanup.

## Command Contracts

Each dev row has:

```text
dev_tools_row.<index>.category=<category>
dev_tools_row.<index>.label=<label>
dev_tools_row.<index>.access=read_only|command
dev_tools_row.<index>.enabled=true|false
dev_tools_row.<index>.disabled_reason=<reason-or-none>
```

Allowed v1 commands:

- `dev_tools.open_editor` if editor available.
- `dev_tools.refresh_save_list` if it only re-reads save metadata.
- `dev_tools.back` closes overlay to parent menu or gameplay.

No v1 commands mutate gameplay, save files, combat, movement, or renderer state.

## Receipt Fields

```text
dev_tools_open=true|false
dev_tools_parent=starter|pause|direct|none
dev_tools_category=<category>
dev_tools_category_label=<label>
dev_tools_row_count=<integer>
dev_tools_readout_visible=true|false
dev_tools_readout_count=<integer>
dev_tools_selected_row=<row|none>
dev_tools_selected_access=read_only|command|none
dev_tools_command=<command|none>
dev_tools_command_status=<status>
dev_tools_input_blocking=true|false
window_launch_count=0
```

## No-Go Surfaces

- No hidden runtime mutation through dev tools.
- No editor geometry mutation from dev tools v1.
- No profiler dependency or GPU capture dependency.
- No window launches.
- No JSON.

## Builder Packet Boundaries

Packet 1: Taxonomy model, labels, readout permissions, receipts, no-window tests.
Packet 2: Hook selected readouts into HUD/debug projection.
Packet 3: Add command-capable rows one by one with explicit tests.

## Focused Tests

- Unit: category order, label, read-only/command access, disabled reason.
- Smoke: open dev tools from pause, select Renderer/Input/Editor/Save, verify readouts and `gameplay_input_suppressed=true`.

## Open Questions

Whether to rename current categories immediately. Conservative default: keep current enum values stable and add taxonomy labels/aliases first.
