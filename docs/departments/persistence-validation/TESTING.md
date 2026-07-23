# Persistence and Validation Testing

## Automated Gate

```bash
cmake --build build --target \
  save_load_tests \
  creative_document_persistence_state_tests \
  creative_document_save_section_tests \
  save_creative_document_section_tests \
  creative_world_service_tests \
  creative_world_layout_codec_tests \
  creative_world_layout_persistence_tests \
  product_save_catalog_tests \
  creative_map_validation_tests \
  creative_editor_map_validation_diagnostics_tests \
  creative_desktop_ui_command_tests \
  creative_world_layout_source_history_tests
ctest --test-dir build \
  -R '^(save_load_tests|creative_document_persistence_state_tests|creative_document_save_section_tests|save_creative_document_section_tests|creative_world_service_tests|creative_world_layout_codec_tests|creative_world_layout_persistence_tests|product_save_catalog_tests|creative_map_validation_tests|creative_editor_map_validation_diagnostics_tests|creative_desktop_ui_command_tests|creative_world_layout_source_history_tests)$' \
  --output-on-failure
```

## Required Repair Pins

- Successful Save and Save As preserve both undo and redo stacks.
- A save acknowledges live document dirty domains only after the durable commit.
- Undo away from the saved state is dirty; redo back to it is clean.
- Undo followed by a different edit at the same numeric revision remains dirty
  and invalidates every document-derived cache or stale plan.
- Keyboard and desktop save routes expose identical clean-state behavior.
- Failed save/open/new operations preserve document, source, history, active
  save id, and clean state.
- Creative load accepts supported legacy schema/section/source versions and
  rejects zero, future, malformed, package/scenario-incompatible, and
  content-hash-mismatched envelopes.
- The exact round-trip proof covers every durable document store and the encoded
  World Layout source.

## Manual Acceptance

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| PER-MAN-001 | PER-001 | Save and reopen one map containing a building, edited terrain, imported assets, transforms, and gameplay markers | `./build/i3dc --desktop-ui` | Pending | Not yet |

### PER-MAN-001 Steps

1. Create or open a map containing all named domains.
2. Make one accepted edit in each domain.
3. Save and note the visible revision and object counts.
4. Close and reopen the same save.
5. Confirm appearance, source editability, selection, history baseline, asset
   identity, and diagnostics.
6. Undo one pre-save edit, confirm the document becomes dirty, then redo and
   confirm the exact saved state becomes clean without losing history.
