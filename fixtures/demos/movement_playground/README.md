# Movement Playground

Product app launch:

```sh
cmake --build build --target iggy3d_app -j 8
./build/iggy3d --window --input auto --save-root "$HOME/.iggy3d/saves" --print-render-receipt
```

No-window product receipt proof:

```sh
./build/iggy3d --no-window --print-render-receipt
```

Product scripted gameplay receipt:

```sh
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

Compatibility/Test Shell:

Use this only for old package visual shell mechanics that the product app has
not absorbed yet, including the visual dev menu and the existing control-file
example below.

```sh
./build/iggy3d_visual_demo --renderer vulkan --window --interactive --dev-menu --input auto --package fixtures/demos/movement_playground/package.iggy3d.toml --print-render-receipt
```

Controls:

- WASD moves, mouse drag or right stick looks, Escape quits on keyboard;
- F3 toggles the world debug telemetry in the window title until text rendering exists;
- Space jumps on keyboard when the dev menu is closed; hold WASD during jump for limited air control;
- Shift dashes on keyboard when the dev menu is closed; hold WASD to choose dash direction;
- F fires a spell projectile on keyboard when the dev menu is closed;
- hold C or Left Ctrl to crouch on keyboard;
- South/Cross jumps on SDL gamepad when the dev menu is closed; hold left stick during jump for limited air control;
- Right Shoulder/R1 dashes on SDL gamepad when the dev menu is closed; hold left stick to choose dash direction;
- Right Trigger/R2 fires a spell projectile on SDL gamepad when the dev menu is closed;
- hold left stick click to crouch on SDL gamepad;
- Launch with `--dev-menu`; F1 toggles the dev menu state. While open, a small
  HUD appears and 1-8 selects walk, crouch, jump,
  dash, spell projectile, vault, clamber, or wire-walk, and Space/Enter executes.
- On SDL gamepad, Options toggles the dev menu state, D-pad left/right cycles,
  and South/Cross executes while the menu is open.
- On PS5/DualSense, Create + Options quits the app.
- On SDL gamepad, Create + East toggles editor mode.

Compatibility/Test Shell control file:

The product app has neutral automation/script-control support, but this README
keeps the old visual shell `--codex-control` example until the equivalent
movement-playground product command is verified and documented.

```sh
cat >/tmp/iggy3d.control <<'EOF'
dev_menu.open=true
debug_overlay.open=true
dev_menu.select=jump
mechanic.execute=true
move.forward=1
look.yaw_delta=0.100
look.pitch_delta=0.050
EOF

./build/iggy3d_visual_demo --renderer vulkan --window --interactive --dev-menu \
  --codex-control /tmp/iggy3d.control \
  --package fixtures/demos/movement_playground/package.iggy3d.toml \
  --print-render-receipt
```

Supported control keys are `dev_menu.open`, `dev_menu.select`, `mechanic`,
`mechanic.execute`, `debug_overlay.open`, `jump`, `dash`, `stance`,
`move.forward`, `move.right`, `player.position`, `player.position_meters`,
`player.position_ft`, `codex_probe.visible`, `codex_probe.position`,
`codex_probe.position_meters`, `codex_probe.position_ft`, `look.yaw_delta`,
`look.pitch_delta`, `interact`, `attack`, `reset`, and `quit`.

Purpose:

- flat 96 ft by 96 ft authored room with grid strips on floor and walls;
- player spawn anchored to the original obstacle cluster, with expanded open floor around it;
- coarse arena grid over the full floor, with denser local grid in the original obstacle section;
- jump pads and a marked gap lane;
- moderate 20 degree slope probe lane for movement policy telemetry;
- clamber wall lineup with low, mid, high, too-high, and narrow slot cases;
- vault rail with posts;
- dash lane with start/end strips;
- elevated wire-walk rail with supports;
- spell target blocks with projectile blocker surfaces;
- wall-run/clamber wall;
- walkable/blocker/projectile spatial surfaces for later movement mechanics.

Current runtime truth:

- this fixture is a playable/renderable test arena;
- first-person kinematic walking, hold-to-crouch stance, runtime jump, limited air control, runtime dash, flat/moderate slope telemetry, debug telemetry, runtime-owned Arcane Bolt casting, deterministic projectile motion, attackable entity hurt-volume impacts, combat damage, and dev-menu spell projectile visuals work against authored room surfaces;
- slope telemetry reports runtime-owned `movement_horizontal_distance_meters`, `movement_vertical_delta_meters`, `movement_grade_percent`, and `slope_travel_direction` values; the debug overlay mirrors those facts with `debug_` receipt fields and a `GRADE` HUD line;
- vault is a runtime traversal mechanic against the authored rail lane;
- clamber is a runtime traversal mechanic against registered wall/ledge slots authored with
  `clamber` traversal tags, measured wall height, usable width, range, facing, landing
  ground, and clearance gates;
- wire-walk is a runtime traversal mechanic against registered rail slots authored as
  `wire` rails, using range, facing, rail-top centerline attach, actor clearance gates,
  continuous rail-axis movement, endpoint clamps, and jump detach back to airborne motor
  control;
- jump/interact input resolves traversal intent before ordinary jump motor logic, so a
  local clamber slot can consume jump while open-floor jump remains a motor fallback;
- traversal preview reports the next ready or blocked traversal candidate without mutating
  world state, including slot kind, height band, range, facing dot, and landing id.
