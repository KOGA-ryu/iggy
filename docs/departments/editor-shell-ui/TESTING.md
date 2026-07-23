# Editor Shell and Drafting UI Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_desktop_ui_command_tests \
  creative_desktop_model_tests \
  creative_editor_toolbox_tests \
  creative_editor_tool_glyph_tests \
  creative_editor_drafting_style_tests
ctest --test-dir build \
  -R '^(creative_desktop_ui_command_tests|creative_desktop_model_tests|creative_editor_toolbox_tests|creative_editor_tool_glyph_tests|creative_editor_drafting_style_tests)$' \
  --output-on-failure
```

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| UI-MAN-001 | UI-001 | Inspect the docked workspace, resize it, enter the 3D viewport, and use Building and Terrain without losing panels or pointer ownership | `./build/i3dc --desktop-ui` | Pending | Not yet |

### UI-MAN-001 Steps

1. Launch at the normal MacBook window size.
2. Resize narrow, wide, and full-screen.
3. Confirm Project, Inspector, Diagnostics, Toolbar, and World Layout remain
   usable without overlapping text or unreachable controls.
4. Enter and leave the 3D viewport; verify panels do not disappear and the
   pointer remains captured until explicit release.
5. Complete one Building and one Terrain action without excessive panel travel.
