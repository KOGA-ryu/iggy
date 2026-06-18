# Integration Decision 0001: Real Department Workflow Test

## Result

Pass. The local Mac department workflow was tested with real completed branch
work from Runtime and AI/NPC.

## Department Floor

The visible floor is six department Terminal windows:

- Runtime
- AI/NPC
- Authoring
- UI/Product
- Platform
- Integration

Each department window has exactly three Codex-hosted tmux tabs:

- `planner`
- `builder`
- `reviewer`

All tabs run Codex CLI with local full-access defaults:

```sh
codex -C <worktree> --sandbox danger-full-access --ask-for-approval never
```

Validate with:

```sh
engine/tools/iggy-dept-up.sh --check
```

## Branches Integrated

1. Runtime cleanup:
   - Source worktree: `/Users/kogaryu/iggy-finisher`
   - Source branch: `codex/finisher-runtime-cleanup`
   - Source head: `bbf45d33 Extract runtime inventory event count projection`
   - Merge commit: `Merge runtime report count cleanup`

2. AI/NPC region fixture:
   - Source worktree: `/Users/kogaryu/iggy-builder-aimap`
   - Source branch: `codex/builder-ai-map-regions`
   - Source head: `48566a8e Add region AI map fixture coverage`
   - Merge commit: `Merge AI map region fixture coverage`

## Gate

Reviewer gate returned PASS. Recommended order was runtime cleanup first, then
AI/NPC fixture coverage. No rebase was required because changed file sets were
non-overlapping with current integration docs/tooling changes.

## Verification

Focused runtime lane:

```sh
cmake --build engine/build --target runtime_gameplay_frame_report_tests runtime_policy_gameplay_frame_report_tests
ctest --test-dir engine/build -R "^(runtime_gameplay_frame_report_tests|runtime_policy_gameplay_frame_report_tests)$" --output-on-failure
```

Focused AI/NPC region lane:

```sh
cmake -S engine -B engine/build
cmake --build engine/build --target runtime_gameplay_ascii_source_plan_region_fixture_tests runtime_gameplay_ascii_source_plan_region_ai_map_promoter_tests runtime_gameplay_ascii_source_plan_toml_file_reader_tests runtime_gameplay_ascii_source_plan_profile_scenario_converter_tests
ctest --test-dir engine/build -R "^(runtime_gameplay_ascii_source_plan_region_fixture_tests|runtime_gameplay_ascii_source_plan_region_ai_map_promoter_tests|runtime_gameplay_ascii_source_plan_toml_file_reader_tests|runtime_gameplay_ascii_source_plan_profile_scenario_converter_tests)$" --output-on-failure
```

Full integration lane:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

Full CTest passed: `342/342`.

## Boundary

This validates the communication and merge workflow. It does not imply every
department should run active builders all the time. The planner/builder/reviewer
tabs are available; the head planner still decides which tabs receive work so
the Mac compute budget stays under control.
