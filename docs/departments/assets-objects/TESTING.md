# Assets and Object Composition Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_editor_placement_tests \
  creative_editor_attachment_tests \
  creative_asset_scatter_tests \
  creative_authored_asset_tests \
  creative_editor_asset_reload_tests
ctest --test-dir build \
  -R '^(creative_editor_placement_tests|creative_editor_attachment_tests|creative_asset_scatter_tests|creative_authored_asset_tests|creative_editor_asset_reload_tests)$' \
  --output-on-failure
```

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| AST-MAN-001 | AST-002 | Find, preview, place, transform, duplicate, replace, delete, undo, and reload a catalog asset | `./build/i3dc --desktop-ui` | Pending | Not yet |

### AST-MAN-001 Steps

1. Find one generated kit asset in the catalog.
2. Verify the held preview and target ghost communicate placement clearly.
3. Place it on ground, against a compatible surface, and at a rejected target.
4. Select, move, rotate, scale, duplicate, and replace the placed object.
5. Undo and redo the gesture groups.
6. Save, reload, and confirm the same asset identity, transform, and appearance.
