# Authoring V1 Release Candidate

Authoring V1 is the supported, test-owned authored-scenario lane for the
current engine. It covers explicit single-file TOML scenarios and explicit
package paths that wrap one main TOML scenario file.

This is not a product save format, editor mutation format, package registry,
asset catalog, or scripting language.

## Accepted Inputs

- Single-file source plans with `format_id = "iggy:ascii-source-plan"` and
  `version = 1`.
- Explicit authored package directories or explicit `package.toml` paths with
  `format_id = "iggy:authored-scenario-package"`, `version = 1`, display-only
  metadata, and one relative `main` scenario file.

Package mode delegates to the same TOML scenario facade as one-file execution.
It does not add gameplay semantics.

## Supported TOML Surface

Authoring V1 supports the checked-in table/key surface covered by schema
snapshot tests and canonical fixtures:

- Top-level source-plan identity: `format_id`, `version`, `source_id`,
  `source_ref`.
- Grid and safety policy: `[grid]`, `[no_claims]`, `[promotion]`.
- Scenario facts: `[[legend]]`, `[[cells]]`, `[[regions]]`, `[[profiles]]`.
- Authored frame inputs: `[[frame_controls]]`,
  `[[frame_player_commands]]`.
- Interaction and pickup facts: `[[interaction_targets]]`, `[[item_drops]]`.
- Expectations: `[expect]`, `[[expect_trace_frames]]`,
  `[[expect_inventory_stacks]]`, `[[expect_interaction_targets]]`,
  `[[expect_actor_states]]`, `[expect_player_state]`.

The TOML reader intentionally supports a project subset. Unsupported TOML
features are diagnostics, not compatibility promises.

## Execution Surfaces

- `RuntimeGameplayTomlScenarioFacade`: run, lint, check, and trace over an
  explicit TOML file.
- `RuntimeGameplayTomlScenarioPackageFacade`: package manifest validation and
  delegation to the TOML scenario facade.
- `RuntimeGameplayAuthoringPreviewModel`: read-only runtime/tooling projection
  over explicit TOML or package paths.
- `iggy_scenario_toml_runner`: development CLI harness for explicit TOML and
  package paths.

## Accepted Modes

- Run: convert and execute authored frames, then expose summary counts and
  final rows.
- Lint: parse, adapt, and validate without executing frames.
- Check: run and compare embedded expectations.
- Trace: run and capture per-frame rows/counts from existing facade
  projections.

## Acceptance Fixtures

Canonical single-file fixtures cover:

- movement only;
- multi-frame movement;
- player and NPC movement;
- interaction toggle;
- item pickup;
- mixed movement/pickup/interaction;
- required-item interaction;
- blocked NPC movement;
- shared destination movement.

Package fixtures cover:

- basic movement;
- player pickup;
- delegated negative source-plan diagnostics.

Regression fixtures cover parser/type/source-plan/conversion/profile-style
failures through stable diagnostic paths.

## Required Verification

Authoring V1 release-candidate verification is:

```sh
cmake -S engine -B engine/build
cmake --build engine/build
ctest --test-dir engine/build --output-on-failure
git diff --check
```

The current acceptance suite includes:

- canonical manifest sweep across run, trace, lint, and check where embedded
  expectations exist;
- CLI output contract snapshots;
- content error-code tests;
- TOML schema/subset/version policy tests;
- source-plan, converter, facade, package facade, preview model, and package
  parity tests;
- negative fixture catalog coverage.

## Non-Goals

- UI/Edi implementation or mutation APIs.
- Save/load changes or authored package save/load roundtrip.
- Runtime autorun or product loop integration.
- Package discovery, dependency resolution, asset catalogs, or multiple
  scenarios per package.
- JSON, a TOML dependency, or full TOML compliance.
- Generic scripting, event language, dialogue/quest/combat systems, or new
  gameplay semantics.
- Broad wrapper/report/ledger framework changes.

## Release-Candidate Decision

Authoring V1 is release-candidate ready for engine/test/dev-tool use when the
verification above is green. Product UI/editor consumption must remain
read-only until a separate UI/Edi integration gate accepts ownership and
mutation boundaries.
