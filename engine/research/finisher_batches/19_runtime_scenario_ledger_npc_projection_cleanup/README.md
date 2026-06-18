# 19 Runtime Scenario Ledger NPC Projection Cleanup

Status: complete.

Goal: remove duplicated scenario result and scenario ledger NPC aggregate
projection setup while preserving runtime behavior and public report fields.

Scope:
- Replace the private scenario runner count copier.
- Replace the private scenario ledger frame and summary NPC aggregate copying.
- Delegate NPC control, movement, and refresh folds to
  `RuntimeNpcOrchestrationAggregates`.
- Keep final-state ledger facts and ledger event append logic local to
  `RuntimeGameplayScenarioLedger.cpp`.

Implementation:
- Added `RuntimeGameplayScenarioReportProjection`, a small helper for scenario
  NPC report facts and applying them to scenario results, ledger summaries, and
  ledger frame rows.
- Left public scenario result, ledger, and ledger frame structs unchanged.
- Left CLI output, ledger event text/order, save/load, and gameplay semantics
  unchanged.

Verification:
- Focused scenario runner, scenario ledger, and NPC orchestration aggregate
  tests.
- Full CTest because production runtime files changed.
- `git diff --check`
