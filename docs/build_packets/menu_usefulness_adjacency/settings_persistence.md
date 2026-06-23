# Settings Persistence Plan

## Objective

Define which settings remain runtime-only and which can persist to disk after Menu Usefulness v1 stabilizes the settings model. This is adjacent because Apply/Restore/Back becomes more useful when the app can later remember safe user preferences without mixing settings into runtime save truth.

## Likely Source Files Later

- `/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.cpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.*`
- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/AppConfig.*` if current app config is the right seam
- `/Users/kogaryu/iggy3d/tests/unit/settings_menu_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/settings_store_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_menu_usefulness_smoke.cpp`

## Data Ownership

- Frontend settings own app preferences.
- Runtime session does not own settings.
- `.iggy3d.save` files own world/session/authored-room progress, not global settings.
- Settings store owns reading/writing one app settings file.
- Visual app owns applying settings to current app state.

## Runtime-Only Settings v1

These can be changed during a run but do not require persistence in the first persistence packet:

```text
settings_parent
settings_tab
settings_selected_row
settings_renderer_change_pending
settings_window_mode_change_pending
current_menu_owner
```

## Persisted Settings v1

Persist these once the store is introduced:

```text
input_backend=keyboard|gamepad|auto
look_sensitivity=<float>
invert_look=true|false
controller_look_sensitivity=<float>
dev_tools_enabled=true|false
debug_overlay_enabled=true|false
high_contrast=true|false
reduced_motion=true|false
master_volume=<float>
preferred_renderer=auto|null|vulkan
preferred_window_mode=no_window|window
```

## Proposed File Path and Format

Default path should be under the same user/app root that save roots already use, not in the repo unless tests override it:

```text
<save-root>/settings/iggy3d_settings.toml
```

Test override path can live under `/tmp`.

TOML-style shape:

```toml
schema = "iggy3d.settings.v1"

[input]
backend = "auto"
look_sensitivity = 1.000
invert_look = false
controller_look_sensitivity = 1.000

[developer]
dev_tools_enabled = true
debug_overlay_enabled = true

[accessibility]
high_contrast = false
reduced_motion = false

[audio]
master_volume = 1.000

[video]
preferred_renderer = "auto"
preferred_window_mode = "no_window"
```

No JSON. Unknown keys are ignored with receipt diagnostics. Invalid known values reject the file and fall back to defaults.

## Apply / Restore / Back Semantics

- Settings menu edits a draft.
- Apply validates draft, copies to active settings, and writes settings file if persistence is enabled.
- Restore Defaults resets draft and active settings to defaults; if persistence exists, writes defaults.
- Back discards draft changes unless Apply already ran.
- Renderer/window changes are pending if they require restart or window recreation; do not launch a window to apply them in tests.

## Receipt Fields

```text
settings_persistence=disabled|enabled
settings_file_path=<path|none>
settings_file_loaded=true|false
settings_file_saved=true|false
settings_file_status=<status>
settings_schema=iggy3d.settings.v1|none
settings_unknown_key_count=<integer>
settings_invalid_key=<key|none>
settings_apply_requested=true|false
settings_restore_defaults_requested=true|false
settings_back_requested=true|false
window_launch_count=0
```

## No-Go Surfaces

- No settings inside `.iggy3d.save` unless explicitly world-specific later.
- No renderer/window recreation in no-window tests.
- No user profiles in v1.
- No JSON.

## Builder Packet Boundaries

Packet 1: Add settings store, TOML-style parser/writer, load/apply/restore/back tests, no-window smoke.
Packet 2: Add settings profiles and per-world overrides only if product need is proven.
Packet 3: Add UI glyphs/rebind UI after Control Binding Policy packet 2.

## Focused Tests

- Unit: valid settings load, missing file defaults, invalid enum defaults with status, apply saves, restore saves defaults, back discards.
- Smoke: app starts with settings path override, applies gamepad + invert look, exits no-window, restarts no-window and reports loaded values.

## Open Questions

Whether settings path should be shared with save root or app config root. Conservative default: save-root sibling `settings/` with CLI override for tests.
