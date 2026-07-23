# Persistence and Validation Testing

## Automated Gate

```bash
cmake --build build --target \
  save_load_tests \
  save_creative_document_section_tests \
  product_save_catalog_tests \
  creative_map_validation_tests
ctest --test-dir build \
  -R '^(save_load_tests|save_creative_document_section_tests|product_save_catalog_tests|creative_map_validation_tests)$' \
  --output-on-failure
```

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
