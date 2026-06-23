# World Creation Flow Plan

## Objective

Define useful New World, Continue, Existing Saves, Delete, Settings, Dev Tools, and Exit behavior for the starter screen. This is adjacent to Menu Usefulness v1 because the starter menu becomes the save/world hub once rows are actionable.

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/app/frontend/StarterScreen.*`
- `/Users/kogaryu/iggy3d/src/app/frontend/SaveSlotModel.*`
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveFileStore.*`
- `/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/**`
- `/Users/kogaryu/iggy3d/tests/unit/starter_screen_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/save_slot_model_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_starter_screen_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_opening_menu_smoke.cpp`

## Definitions

World:

- User-facing identity and display name attached to a save lineage.
- Owns display metadata such as world id/name and creation time if available.
- Does not replace `.iggy3d.save` in v1.

Save:

- Concrete `.iggy3d.save` file carrying runtime/session/authored-room state.
- Save metadata drives Continue/Existing Saves rows.

## Data Ownership

- `SaveFileStore` owns save file creation, listing, reading, deleting.
- `SaveSlotModel` owns preview metadata and compatibility classification.
- Starter screen owns selected row/save index and confirmation state.
- Runtime session owns loaded gameplay state after explicit launch.
- Settings/dev tools remain app/frontend-owned overlays reachable from starter.

## Starter Row Semantics

Order:

1. Continue
2. New World
3. Existing Saves
4. Delete Save
5. Settings
6. Dev Tools
7. Exit

Continue:

- Enabled only with at least one compatible save.
- Loads most recent compatible save.
- Disabled reason `no_compatible_save`.

New World:

- Creates new save with generated display name if no text entry exists.
- Suggested display name: `World <n>`.
- Creates `.iggy3d.save`, then launches runtime.
- Does not mutate package fixtures.

Existing Saves:

- Opens save list child panel.
- Rows show compatible, incompatible, corrupt if preview is possible.
- Incompatible/corrupt saves are disabled but visible.

Delete Save:

- Requires selected save and confirmation.
- Disabled reason `no_save_selected` or `save_delete_blocked`.
- Confirmation has `confirm` and `back` actions.

Settings:

- Opens settings parent `starter`.

Dev Tools:

- Opens starter dev tools/readout parent `starter`.

Exit:

- Opens confirmation or requests exit with explicit receipt.

## Save Metadata

Required preview fields:

```text
world_id=<id|none>
world_display_name=<name|none>
package_id=<id>
scenario_id=<id>
current_tick=<integer>
last_played=<stable-text|none>
saved_state_hash=<hash|none>
authored_floor_count=<integer>
authored_wall_count=<integer>
compatibility=<compatible|incompatible_package|incompatible_scenario|decode_failed|load_failed|unknown>
path=<path>
```

If current save format lacks world id/display name, derive stable values from save id/path for v1 and record `world_metadata_source=derived`.

## Command Contracts

Codex-control names:

```text
starter.select=continue|new_world|existing_saves|delete_save|settings|dev_tools|exit
starter.execute=true
starter.save_select=<save-id|index>
starter.delete_confirm=true|false
starter.back=true
```

Keep existing `opening_menu.*` keys as compatibility aliases.

## Receipt Fields

```text
frontend_screen=starter|new_world|load_save|delete_confirm|settings|starter_dev_tools|exit_confirm|gameplay
starter_world_suppressed=true
starter_selected_action=<action>
starter_selected_enabled=true|false
starter_selected_disabled_reason=<reason-or-none>
frontend_launch_requested=true|false
frontend_return_to_title_requested=true|false
save_count=<integer>
compatible_save_count=<integer>
corrupt_save_count=<integer>
selected_save_id=<id|none>
selected_save_compatible=true|false|unavailable
world_id=<id|none>
world_display_name=<name|none>
world_metadata_source=save|derived|none
delete_confirmation_required=true|false
delete_confirmed=true|false
window_launch_count=0
```

## No-Go Surfaces

- No separate world database in v1.
- No package/schema churn.
- No world thumbnails yet.
- No window launches.
- No JSON.

## Builder Packet Boundaries

Packet 1: Starter save hub row behavior, metadata, confirmation, no-window smokes.
Packet 2: Add explicit world display name/id fields to saves if product requires it.
Packet 3: Add thumbnails and richer world metadata after renderer capture policy exists.

## Focused Tests

- Continue disabled/enabled behavior.
- New World creates save and launches only after explicit action.
- Existing Saves shows disabled corrupt/incompatible rows.
- Delete requires confirmation.
- Return to title from gameplay reopens starter without auto-loading world.

## Open Questions

Whether to persist first-class `world_id` now or derive from save id. Conservative default: derive in packet 1, persist explicitly in packet 2.
