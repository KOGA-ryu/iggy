# Control Binding Policy Plan

## Objective

Define stable action names, ownership stack, keyboard/PS5/controller/Codex mappings, and chord policy before more menus, editor tools, and gameplay verbs are added. This is adjacent to Menu Usefulness v1 because menu rows become useful only if all input paths resolve to the same semantic actions.

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/app/frontend/MenuInput.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/MenuInput.cpp`
- `/Users/kogaryu/iggy3d/src/app/input/GamepadSystemControls.hpp`
- `/Users/kogaryu/iggy3d/src/app/input/GamepadSystemControls.cpp`
- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/menu_input_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/gamepad_system_controls_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_menu_usefulness_smoke.cpp`
- `/Users/kogaryu/iggy3d/README.md`

## Data Ownership

- `MenuInput` owns normalized UI actions and owner priority.
- `GamepadSystemControls` owns PS5/DualSense system chords and edge behavior.
- Visual app owns hardware event collection and translates it into `MenuInput` or gameplay commands.
- Runtime receives only semantic gameplay commands; it must not know keyboard, PS5, SDL, or Codex-control keys.
- Codex-control files own deterministic scripted input for tests only.

## Semantics and Invariants

Input owner priority:

1. boot/starter/opening menu
2. pause menu
3. settings menu
4. dev tools overlay/menu
5. editor
6. gameplay

Only the active owner consumes actions. If a menu owns input, gameplay movement/look/interact/attack/jump must be suppressed.

Normalized action names:

```text
ui.up
ui.down
ui.left
ui.right
ui.confirm
ui.back
ui.next_tab
ui.previous_tab
ui.pause
ui.quit
ui.dev_tools
ui.editor_toggle
ui.debug_overlay
```

Gameplay action names remain separate:

```text
game.move_forward
game.move_back
game.move_left
game.move_right
game.look_yaw
game.look_pitch
game.interact
game.attack
game.jump
game.dash
game.crouch
game.reset
```

Codex-control names should mirror semantic actions:

```text
menu.owner=<owner>
menu.input=<up|down|left|right|confirm|back|next_tab|previous_tab>
menu.input_frames=<csv-frames>
game.action=<action>
game.action_frames=<csv-frames>
```

Keep existing direct keys compatible while routing them through the shared model.

## Keyboard Defaults

- Up/down/left/right: arrows and W/A/S/D when a menu owns input.
- Confirm: Enter or Space.
- Back: Esc.
- Pause: Esc only while gameplay owns input.
- Dev tools: F1 only if developer tools are enabled and a higher-priority owner is not active.
- Editor toggle: existing editor key/chord only if gameplay or dev tools permits it.

## PS5/DualSense Defaults

- D-pad or left stick: menu navigation.
- Cross: confirm.
- Circle: back.
- Options: pause while gameplay owns input.
- Create+Options: hard quit, edge-triggered, consumes both buttons.
- Create+Circle/East: editor toggle when editor is enabled and menus do not own input.
- Direct dev overlay shortcut is opt-in and disabled by default.

## Joy-Con Future Notes

Do not implement Joy-Con special cases yet. Reserve semantic mapping through the same action names. Future Joy-Con left/right split must not bypass owner priority.

## Chord Policy

- Chords are edge-triggered.
- Chords consume component buttons after activation.
- Destructive chords require either a confirmation screen or a reserved hard-quit combo.
- Hard quit: PS5 Create+Options only.
- Dev tools: F1 keyboard, menu row, or explicit opt-in controller route.
- Editor toggle: never active while starter, pause, settings, or delete confirmation owns input.
- Destructive actions such as delete save require confirm/back, never a single accidental chord.

## Receipt Fields

```text
input_owner=<owner>
input_source=keyboard|gamepad|codex|scripted|none
ui_action=<action|none>
ui_action_consumed=true|false
ui_action_blocked_reason=<reason|none>
gameplay_input_suppressed=true|false
gamepad_options_opens=pause
gamepad_create_options_quit=true
gamepad_editor_chord=create+east
codex_control_status=<status>
window_launch_count=0
```

## No-Go Surfaces

- No runtime dependency on devices or key names.
- No duplicate per-menu button policy when `MenuInput` can own it.
- No new window tests.
- No JSON or old repo dependency.

## Builder Packet Boundaries

Packet 1: Normalize `MenuInput`, route existing menu/gamepad/Codex paths, add unit tests and receipt fields.
Packet 2: Add remappable binding tables after settings persistence exists.
Packet 3: Add future controller families and profile-specific glyphs.

## Focused Tests

- `menu_input_tests`: owner priority, keyboard mapping, Codex mapping, action consume/block behavior.
- `gamepad_system_controls_tests`: preserve Options pause, Create+Options quit, Create+East editor, no repeat while held.
- No-window smoke: menu action suppresses gameplay and emits `window_launch_count=0`.

## Open Questions

No blocking open question. Conservative default is fixed bindings with semantic action names, not user-remappable controls yet.
