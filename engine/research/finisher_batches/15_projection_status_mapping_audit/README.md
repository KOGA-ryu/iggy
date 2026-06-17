# 15 Projection Status Mapping Audit

Status: complete.

Goal: audit duplicate status, summary, and diagnostic mapping between preview,
package, facade, and CLI.

Scope:
- Docs gate first.
- Implement only if there is a tiny pure helper that does not change CLI output
  or result shapes.

Verification:
- Docs diff review for audit-only work.
- Focused tests if a tiny helper is added.

Audit:
- TOML facade:
  `engine/src/runtime/RuntimeGameplayTomlScenarioFacade.hpp` owns the source
  execution status enum, run summary projection, trace-frame projection,
  expectation comparison, and flattened diagnostics.
- Package facade:
  `engine/src/runtime/RuntimeGameplayTomlScenarioPackageFacade.hpp` owns the
  package status enum and issue list, then embeds the source facade result for
  scenario execution. Package statuses intentionally distinguish package read
  and manifest failures from delegated scenario failures.
- Preview model:
  `engine/src/runtime/RuntimeGameplayAuthoringPreviewModel.cpp` maps source
  facade statuses and package facade statuses into
  `RuntimeGameplayAuthoringPreviewStatus`, then copies summary, final rows,
  trace frames, expectation comparison, diagnostics, and package metadata into
  one model.
- CLI:
  `engine/apps/scenario_toml_runner/IggyScenarioTomlRunner.cpp` owns the
  current output contract: package status text, package issue text, result
  strings, exit-code mapping, summary printing, trace printing, expectation
  printing, and first-issue diagnostic snippets.
- Diagnostics:
  `engine/src/runtime/RuntimeGameplayAuthoringDiagnostics.cpp` already provides
  a flattened diagnostic projection over file, TOML, source-plan, adapter,
  conversion, profile, and nested scenario issues.
- Summary projection:
  `engine/src/runtime/RuntimeGameplayTomlScenarioSummaryProjection.cpp`
  already provides a small source-path/run/final-rows projection helper used by
  the facade and tested by
  `runtime_gameplay_toml_scenario_summary_projection_tests`.

Duplication signals:
- Preview model status mapping duplicates the semantic relationship between
  source/package facade statuses and authoring-facing statuses.
- CLI package status and package issue `ToString` functions are local to the
  CLI, while other authoring status/code text uses
  `runtimeGameplayAuthoringCodeText`.
- CLI summary and expectation printers render fields already present in
  `RuntimeGameplayTomlScenarioRunSummaryProjection` and
  `RuntimeGameplayTomlScenarioExpectationComparison`.
- Package/source parity tests compare the same summary, expectation, and trace
  fields, now through test support, but this is assertion duplication rather
  than production projection duplication.

No-helper decision:
- Do not extract CLI text mapping in this packet. It would affect observable
  output and exit-code contracts unless every rendered branch is locked by
  golden tests.
- Do not move preview status mapping yet. The existing functions are private,
  simple, and tied to package-vs-source input semantics.
- Do not merge diagnostics with CLI first-issue printing. Diagnostics preserve
  flattened entries, while the CLI intentionally prints selected first issues
  in a compact compatibility format.

Smallest future helper candidate:
- If needed, add production text helpers only for package facade status and
  package issue code, adjacent to package facade code, then update CLI tests to
  lock each string before wiring the CLI to those helpers.
- A lower-risk alternative is a preview-only test helper that asserts
  source/package status mapping without changing production code.

Recommended tests before implementation:
- Add CLI output-contract cases for every package facade status and package
  issue code that would be textified by a shared helper.
- Extend preview model tests to cover each source facade status and package
  facade status mapping.
- Keep summary projection tests focused on field copies; avoid broadening them
  into CLI rendering tests.
