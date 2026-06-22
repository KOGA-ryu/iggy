# Movement Playground

Launch:

```sh
./build/iggy3d_visual_demo --renderer vulkan --window --interactive --input auto --package fixtures/demos/movement_playground/package.iggy3d.toml --print-render-receipt
```

Controls:

- WASD moves, mouse drag or right stick looks, Escape/Start quits;
- F3 toggles the world debug telemetry in the window title until text rendering exists;
- Space jumps on keyboard when the dev menu is closed; hold WASD during jump for limited air control;
- Shift dashes on keyboard when the dev menu is closed; hold WASD to choose dash direction;
- hold C or Left Ctrl to crouch on keyboard;
- South/Cross jumps on SDL gamepad when the dev menu is closed; hold left stick during jump for limited air control;
- Right Shoulder/R1 dashes on SDL gamepad when the dev menu is closed; hold left stick to choose dash direction;
- hold left stick click to crouch on SDL gamepad;
- F1 toggles the dev menu state; while open, 1-8 selects walk, crouch, jump,
  dash, spell projectile, vault stub, clamber stub, or wire-walk stub, and Space/Enter executes.
- On SDL gamepad, Start + North toggles the dev menu state, D-pad left/right cycles,
  and South executes while the menu is open.

Codex control:

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
`move.forward`, `move.right`, `look.yaw_delta`, `look.pitch_delta`, `interact`,
`attack`, `reset`, and `quit`.

Purpose:

- flat 96 ft by 96 ft authored room with grid strips on floor and walls;
- player spawn anchored to the original obstacle cluster, with expanded open floor around it;
- coarse arena grid over the full floor, with denser local grid in the original obstacle section;
- jump pads and a marked gap lane;
- clamber block and stepped ledges;
- vault rail with posts;
- dash lane with start/end strips;
- elevated wire-walk rail with supports;
- spell target blocks with projectile blocker surfaces;
- wall-run/clamber wall;
- walkable/blocker/projectile spatial surfaces for later movement mechanics.

Current runtime truth:

- this fixture is a playable/renderable test arena;
- first-person kinematic walking, hold-to-crouch stance, runtime jump, limited air control, runtime dash, debug telemetry, deterministic projectile motion, and dev-menu spell projectile visuals work against authored room surfaces;
- clamber, vault, and wire-walk mechanics are authored as test zones, not implemented ability modes yet.
