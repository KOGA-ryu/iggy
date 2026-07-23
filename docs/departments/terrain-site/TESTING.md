# Terrain and Site Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_terrain_operation_tests \
  creative_terrain_region_recipe_tests \
  creative_terrain_generation_tests \
  creative_terrain_contour_tests \
  creative_editor_world_layout_topography_tests
ctest --test-dir build \
  -R '^(creative_terrain_operation_tests|creative_terrain_region_recipe_tests|creative_terrain_generation_tests|creative_terrain_contour_tests|creative_editor_world_layout_topography_tests)$' \
  --output-on-failure
```

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| TER-MAN-001 | TER-001 | Generate terrain, select a region, grade and sculpt it, then inspect contours and building contacts | `./build/i3dc --desktop-ui` | Pending | Not yet |

### TER-MAN-001 Steps

1. Generate a terrain patch with a fixed seed.
2. Select one rectangular region and preview a raise or lower operation.
3. Apply grade and sculpt operations with visible feathering.
4. Enable contours and inspect the result from plan and 3D views.
5. Place or generate a building at the edited edge and inspect grounding.
6. Undo and redo the complete operations, then save and reopen.
