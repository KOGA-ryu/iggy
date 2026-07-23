# Authoring Core Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_core_tests \
  creative_document_create_tests \
  creative_document_remove_tests \
  creative_document_mutation_tests \
  creative_facade_tests \
  creative_facade_mutation_tests \
  creative_recipe_tests \
  creative_map_template_tests \
  creative_world_service_tests \
  creative_world_layout_source_history_tests
ctest --test-dir build \
  -R '^(creative_core_tests|creative_document_create_tests|creative_document_remove_tests|creative_document_mutation_tests|creative_facade_tests|creative_facade_mutation_tests|creative_recipe_tests|creative_map_template_tests|creative_world_service_tests|creative_world_layout_source_history_tests)$' \
  --output-on-failure
```

## Manual Acceptance

Authoring Core is accepted through visible workflows owned by Building,
Terrain, Assets, and Persistence rather than through a standalone UI.

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| AUT-MAN-001 | AUT-002 | No standalone manual surface; exercise through domain acceptance workflows | Headless only | Not Required | Not applicable |

## AUT-008 Targeted Evidence

The prescribed AUT-008 CTest expression passed 9 of 10 tests. The following
targets passed: `creative_facade_mutation_tests`, `creative_facade_tests`,
`creative_document_create_tests`, `creative_editor_asset_reload_tests`,
`creative_editor_placement_tests`, `creative_authored_asset_tests`,
`creative_desktop_ui_command_tests`, `creative_world_layout_tests`, and
`creative_world_layout_diagnostics_tests`. `creative_editor_attachment_tests`
was not run because its executable could not be linked from the pre-existing
empty `libiggy3d_creative_app.a` archive.

The remaining AUT-002 inventory is 21 direct
`document = std::move(staged)` publications. AUT-008 leaves those publications
for AUT-002 as required.
