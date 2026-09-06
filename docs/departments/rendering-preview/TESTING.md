# Rendering and Preview Testing

## Automated Gate

```bash
cmake --build build --target \
  render_projection_input_tests \
  render_command_recording_tests \
  render_memory_budget_policy_tests \
  creative_capture_stability_smoke
ctest --test-dir build \
  -R '^(render_projection_input_tests|render_command_recording_tests|render_memory_budget_policy_tests|creative_capture_stability_smoke)$' \
  --output-on-failure
```

## Manual Acceptance

### Native UI capture boundary (FM-001 / REN-009)

The native Hunt game uses an empty scene and the external UI recording hook.
Its capture must contain real, nonuniform pixels and record
`external_ui_recorded=1`. A hook receipt alone cannot prove that pixels were
copied to the readback buffer. Creative capture retains
`external_ui_recorded=0`.

The focused gate for this extension is `first_move_capture_smoke`,
`i3dc_capture_smoke`, and `vulkan_empty_frame_smoke`, using the offscreen SDL
driver. On macOS the tools' sandbox may deny Metal/MoltenVK initialization;
run that same bounded gate with GPU access to distinguish access failure from
renderer failure. This does not authorize a visible window or a broad suite.

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| REN-MAN-001 | REN-002 | Inspect held, valid, invalid, and accepted placement states without scene lag or stale geometry | `./build/i3dc` | Pending | Not yet |

### REN-MAN-001 Steps

1. Select a material and a catalog asset.
2. Inspect the held-object view and valid placement ghost.
3. Aim at rejected and absent targets.
4. Place, remove, undo, and redo continuously while moving the camera.
5. Confirm actual geometry replaces the ghost immediately and idle aiming does
   not trigger visible stalls.
