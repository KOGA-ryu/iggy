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

Sol independently configured `/tmp/iggy3d-aut008-review-sol` and built `i3dc`
plus the ten prescribed targets. The exact focused CTest expression passed
10/10:

- `creative_facade_mutation_tests`
- `creative_facade_tests`
- `creative_document_create_tests`
- `creative_editor_attachment_tests`
- `creative_editor_asset_reload_tests`
- `creative_editor_placement_tests`
- `creative_authored_asset_tests`
- `creative_desktop_ui_command_tests`
- `creative_world_layout_tests`
- `creative_world_layout_diagnostics_tests`

The clean build disproved the prior empty-archive blocker; that failure belonged
to a stale build tree. Source gates found no mutable Facade document accessor,
no app-facing mutation options, and no retired editor-local hierarchy
publication.

The remaining AUT-002 inventory is 20 direct same-document staging
assignments, five canonical `commitStagedMutation` call sites, and the Facade
batch-create replacement path. AUT-002A and AUT-002B own those repairs.

## AUT-005 Targeted Evidence

The accepted lifecycle gate adds these persistence-facing targets to the
ten-test Authoring Core gate:

- `creative_document_persistence_state_tests`
- `creative_document_save_section_tests`
- `creative_world_layout_persistence_tests`
- `creative_desktop_ui_command_tests`
- `creative_creator_task_workflow_tests`
- `creative_editor_world_layout_tests`

The resulting 16/16 CTest gate proves exact live-save acknowledgement,
desktop/keyboard checkpoint parity, New/Open replacement behavior, and failure
atomicity. Save history clearing remains explicitly outside AUT-005.
