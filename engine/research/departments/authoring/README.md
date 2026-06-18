# Authoring Department

## Charter

Own source-plan TOML, package manifests, authoring facades, preview model,
diagnostics, expectations, fixture contracts, and developer CLI workflows.

## Roles

- Planner: maintains authoring packets and decides when to pull from raw queue
  versus scoped stretches.
- Builder: implements parser/validator/converter/facade/fixture work.
- Researcher: checks format policy, fixture coverage, and authoring ergonomics.
- Reviewer: gates declarative-vs-runtime boundaries and output contracts.
- Finisher: trims test ceremony, docs drift, and projection duplication.
- Apprentice/Spark: table inventories, negative fixture matrices, CI lane scans.

## Bucket

The legacy authoring bucket remains in
`engine/research/authoring_batches/README.md`. Department planners may continue
using that packet list, but should group related packets into scoped stretches.

Near-term open authoring lanes:

1. ResourceId namespace policy.
2. Frame ordering policy.
3. Multi-actor/profile fixture pack.
4. Existing interaction effect fixture pack.
5. Authoring diff report.
6. Warning channel gate.

## Hard Stops

- No TOML dependency unless a packet explicitly opens that policy.
- No JSON/machine-output surface unless separately gated.
- No generic scripting or inferred semantics.
- No package discovery/dependency system.
- No CLI output change without output-contract tests.

## Verification

Focused source-plan/TOML/facade/CLI/manifest tests by slice, then full engine
CTest before integration for code changes.
