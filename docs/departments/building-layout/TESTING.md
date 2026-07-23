# Building and World Layout Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_editor_building_blockout_tests \
  creative_building_authoring_workflow_tests \
  creative_building_refinement_workflow_tests \
  creative_world_layout_building_usability_tests \
  creative_world_layout_building_traversal_tests
ctest --test-dir build \
  -R '^(creative_editor_building_blockout_tests|creative_building_authoring_workflow_tests|creative_building_refinement_workflow_tests|creative_world_layout_building_usability_tests|creative_world_layout_building_traversal_tests)$' \
  --output-on-failure
```

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| BLD-MAN-001 | BLD-001 | Stage a two-storey building and inspect scale, slabs, walls, rooms, openings, stairs, and roof in plan and 3D | `./build/i3dc --desktop-ui` | Pending | Not yet |

### BLD-MAN-001 Steps

1. Open Building Blockout in the Create surface.
2. Stage a footprint with two storeys, stairs, doors, windows, and a gable roof.
3. Inspect it in plan view, elevation, and the 3D viewport.
4. Confirm floors meet wall bases, ceiling heights are consistent, exterior
   walls read continuously, openings cut their host walls, and stairs connect
   both levels.
5. Save, close, reopen, and confirm the authored source remains editable.
6. Record every visual, scale, interaction, and persistence defect against the
   relevant `BLD-*` item before marking the test passed.
