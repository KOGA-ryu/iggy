# AI/NPC Outbox 0001: Region AI-Map Fixture Merge Brief

## Source

- Worktree: `/Users/kogaryu/iggy-builder-aimap`
- Branch: `codex/builder-ai-map-regions`
- Head: `48566a8e Add region AI map fixture coverage`

## Summary

Builder added regression fixture coverage for authored source-plan regions and
the existing explicit C++ region-to-AiMap promotion path. The branch reports
focused region/promoter/TOML/converter tests and full engine CTest passing.

## Files Reported

- `engine/tests/fixtures/runtime/ascii_source_plan/region_ai_map_room.toml`
- `engine/tests/runtime_gameplay_ascii_source_plan_region_fixture_tests.cpp`
- `engine/cmake/iggy_runtime_tests.cmake`
- `engine/tests/fixtures/runtime/ascii_source_plan/README.md`
- `engine/research/authoring_batches/11_ai_map_region_fixtures/README.md`

## Requested Integration Action

Merge after the runtime cleanup branch unless reviewer finds no shared surface
risk. Keep the branch regression-only: no default CLI/facade region promotion,
no triggers, no proximity, and no scripting semantics.

## Suggested Focused Verification

- Build and run the new region fixture test.
- Run region promoter, TOML file-reader, and source-plan converter tests.
- Run full integration verification after merge.
