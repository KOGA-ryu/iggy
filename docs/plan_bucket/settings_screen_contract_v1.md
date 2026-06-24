# Settings Screen Contract v1

## Objective

Define the durable settings screen before more menu code is written.

The settings screen is shared by the starter menu and the in-game pause menu.
It must not be implemented as scattered `if` branches in `AppShell.cpp`.

## Access Points

Settings can be opened from:

- starter menu;
- in-game pause menu.

Starter settings return to the starter menu.

Pause settings return to the pause menu.

Settings must always suppress gameplay input while open.

Settings is a child surface. Parent-return rules are fixed:

- starter settings returns to starter;
- pause settings returns to pause;
- selector/dialog children opened inside settings return to settings.

## Top-Level Tab Order

The settings screen uses this tab order:

1. Input
2. Controls
3. Camera
4. Gameplay
5. Video / Display
6. Audio
7. Accessibility
8. Developer

This order matches the current `FrontendSettingsTab` shape and should remain
stable unless the user explicitly changes it.

## Screen Layout

The settings screen should display:

- tab strip across the top or left side;
- selected tab contents in the main area;
- focused setting row;
- current value;
- changed/dirty indicator when a setting differs from its applied value;
- Apply;
- Restore Defaults;
- Back.

The screen should not use nested cards. It should be a stable utility panel.

The settings panel should sit near center screen with a slight right-side bias.
It must leave the game/world context readable behind or beside it.

## Input Tab

The Input tab should include:

- input backend: auto, keyboard, gamepad, scripted;
- detected gamepad name;
- gamepad connection state;
- active input owner;
- controller glyph/profile later;
- PS5/DualSense profile status;
- Joy-Con profile status later;
- controller remap entry point later.

V1 should be read/write for input backend and read-only for detected device
details.

## Controls Tab

The Controls tab should include:

- look sensitivity;
- controller look sensitivity;
- invert look;
- air control tuning visibility later;
- crouch toggle/hold choice later;
- sprint toggle/hold choice later;
- jump binding display later;
- interact binding display later;
- attack/cast binding display later.

V1 should include look sensitivity, controller look sensitivity, and invert
look. Binding remap can be a later screen.

## Camera Tab

The Camera tab should include:

- default camera mode: first person, third person, tactical;
- field of view later;
- third-person distance later;
- tactical camera height later;
- tactical transition speed later;
- camera shake amount later;
- camera bob amount later.

V1 should include camera mode only if the runtime can honestly honor it.
Otherwise it should be visible as read-only or deferred.

## Gameplay Tab

The Gameplay tab should include:

- difficulty;
- pause on focus loss;
- tactical slow-time behavior later;
- auto-save policy later;
- interaction prompt visibility later;
- combat feedback verbosity later.

V1 can expose difficulty as read-only until difficulty affects runtime rules.

## Video / Display Tab

The Video / Display tab should include:

- renderer request: auto, null, Vulkan;
- window mode: no-window, window;
- resolution later;
- fullscreen/windowed later;
- vsync later;
- brightness/gamma later;
- HUD scale later;
- debug overlay visibility if not developer-only.

V1 should not pretend `renderer=vulkan` is renderer proof for the product app.
It is a request/settings value unless a renderer packet proves otherwise.

## Audio Tab

The Audio tab should include:

- master volume;
- music volume later;
- effects volume later;
- voice/dialog volume later;
- output device later;
- mute on focus loss later.

If audio is unavailable, audio rows should be disabled with
`audio_unavailable`.

## Accessibility Tab

The Accessibility tab should include:

- high contrast;
- reduced motion;
- subtitle/caption visibility later;
- text scale later;
- colorblind palettes later;
- hold-to-confirm destructive actions later;
- input assist later.

V1 should include high contrast and reduced motion.

## Developer Tab

The Developer tab should include:

- a single `Dev Tools: On / Off` toggle.

The toggle enables or disables dev tools as one group. V1 must not expose
individual dev-tool category toggles in settings.

When enabled, dev tools use the runtime overlay/layer contract defined in:

```text
docs/plan_bucket/dev_tools_contract_v1.md
```

Developer settings must not be mixed into player-facing settings unless the
setting is genuinely useful to normal play.

## Persistence Semantics

Each row must declare one persistence class:

- `runtime_only`: applies for current run only;
- `settings_file`: persists to user settings later;
- `save_file`: belongs to a save/world file;
- `deferred`: visible but not implemented;
- `read_only`: informational only.

V1 may keep most values `runtime_only`, but the row must still declare the
intended long-term class.

## Apply Semantics

Settings screen owns draft values.

Apply copies draft values into active frontend settings.

Restore Defaults resets draft values to defaults.

Back without apply should either discard draft changes or ask for confirmation
later. V1 should discard unapplied changes and report that behavior in the
status/receipt.

Renderer changes may mark `renderer_change_pending` instead of taking effect
immediately.

## Data Ownership

Settings menu owns:

- selected tab;
- selected row;
- draft values;
- dirty state;
- row enabled state;
- disabled reasons;
- persistence class.

Runtime owns gameplay rules.

Renderer owns renderer capability and actual display behavior.

Input systems own detected device state.

Save files own save/world-specific settings.

## Router Ownership

Settings navigation belongs in the product frontend router:

- tab previous/next;
- row previous/next;
- value left/right;
- apply;
- restore defaults;
- back to parent surface.

`AppShell.cpp` should call the router and should not own settings routing
branches.

Mouse input must use semantic route actions:

- click a stable row hit region -> focus or confirm that row;
- wheel scrolls the active settings list;
- click outside stable hit regions is ignored for V1;
- hover only changes focus when the view model exposes stable hit regions.

## Receipt Fields

Settings receipts should prove:

```text
settings_open=true|false
settings_parent=starter|pause|none
settings_selected_tab=<tab>
settings_selected_row=<row>
settings_row_enabled=true|false
settings_disabled_reason=<reason>
settings_persistence=<runtime_only|settings_file|save_file|deferred|read_only>
settings_dirty=true|false
settings_apply_available=true|false
settings_restore_defaults_available=true|false
settings_back_target=starter|pause|none
```

Existing fields such as `settings_selected_tab`, `input_owner`, and
`gameplay_input_suppressed` should be reused where possible.

## Test Plan

Unit tests should prove:

- tab order;
- row ownership per tab;
- disabled audio rows when audio is unavailable;
- read-only or deferred gameplay rows;
- draft/apply/restore behavior;
- back target from starter settings;
- back target from pause settings;
- gameplay input suppression while settings is open;
- renderer change marks pending when appropriate.

Smoke tests should prove with no-window receipts:

- starter settings opens;
- pause settings opens;
- tab changes;
- apply updates active settings when supported;
- restore defaults updates draft;
- back returns to the correct parent;
- `window_launch_count=0`.

## Stop Rules

Stop before implementation if the plan requires:

- renderer/Vulkan changes;
- runtime gameplay rule changes;
- save schema changes;
- real window proof;
- editor automation;
- broad input remapping implementation.

## V1 Decision Defaults

- Back discards unapplied changes for now.
- Gameplay difficulty is visible but read-only until runtime rules exist.
- Camera mode is writable only for modes already honored by product camera
  state.
- Developer tab remains available in internal builds.

## Coding Method

Settings should be implemented as tab and row descriptors consumed by a router.
Do not implement each tab as a separate `AppShell.cpp` branch.

Recommended modules:

```text
src/app/frontend/SettingsScreenModel.hpp
src/app/frontend/SettingsScreenModel.cpp
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
```

Each settings row should declare:

```text
id
label
tab
value_kind
current_value
draft_value
enabled
disabled_reason
persistence_class
left_action
right_action
confirm_action
```

## File Ownership

| File area | Owns | Must not own |
| --- | --- | --- |
| `SettingsScreenModel.*` / `SettingsMenu.*` | tabs, rows, draft values, dirty state, persistence class | renderer backend behavior, runtime gameplay rules |
| `MenuInput.*` | semantic actions such as tab, row, confirm, back | settings mutation policy |
| `ProductFrontendRouter.*` | route settings actions to model operations | direct device handling |
| `ReceiptBuilder.*` | proof fields from settings model and route result | settings decisions |
| `AppShell.cpp` | call router and apply active settings after route result | row-level settings logic |

## Inputs And Outputs

Settings router input:

```text
FrontendState
MenuActionFrame
FrontendSettings active_settings
SettingsDraftState
DeviceCapabilitySummary
RendererCapabilitySummary
```

Settings router output:

```text
SettingsRouteResult
```

The result should report accepted/ignored, selected tab, selected row, dirty
state, apply request, restore request, back target, and status.

## Long-Term Fit

The same settings model must work from starter and pause. It should also survive
future persistent settings, controller remapping, video options, audio rows, and
accessibility rows because every row declares ownership and persistence class up
front.
