# Authoring Core Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_core_tests \
  creative_document_create_tests \
  creative_document_mutation_tests \
  creative_facade_tests \
  creative_facade_mutation_tests \
  creative_recipe_tests \
  creative_map_template_tests \
  creative_world_service_tests \
  creative_world_layout_source_history_tests
ctest --test-dir build \
  -R '^(creative_core_tests|creative_document_create_tests|creative_document_mutation_tests|creative_facade_tests|creative_facade_mutation_tests|creative_recipe_tests|creative_map_template_tests|creative_world_service_tests|creative_world_layout_source_history_tests)$' \
  --output-on-failure
```

## Manual Acceptance

Authoring Core is accepted through visible workflows owned by Building,
Terrain, Assets, and Persistence rather than through a standalone UI.

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| AUT-MAN-001 | AUT-002 | No standalone manual surface; exercise through domain acceptance workflows | Headless only | Not Required | Not applicable |
