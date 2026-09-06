# Sword Material Prompt Chain

These prompts specialize the repository material workflow for swords. Execute
them in numeric order. They do not replace
`assets/creative/materials/workflow/phases/`; each prompt supplies the
sword-specific questions, artifacts, rejection rules, and build decisions that
the matching general phase must carry.

## Chain

1. `00_consumer_census_and_typology.md`
2. `01_blade_geometry_uv_and_semantics.md`
3. `02_clean_steel_and_polish_stack.md`
4. `03_blade_identity_forks.md`
5. `04_decoration_fittings_and_stylization.md`
6. `05_condition_and_use_history_optional.md`
7. `06_shader_engine_and_channel_contract.md`
8. `07_proof_acceptance_and_donor_registration.md`

## Inheritance rule

Every prompt reads the artifacts produced by all earlier prompts. It records
new evidence in `WORKSTREAM.md`, new causal intent in `RESEARCH_INTENT.md`, and
implementation-ready demands in `CODED_DEMANDS.md`. Do not restart the asset
description or rediscover tools at every step.

## Execution boundary

- Prompt 00 selects one actual sword or declares an isolated-fixture boundary.
- Prompts 01 and 02 build the shared intact foundation.
- Prompt 03 selects exactly one construction-specific blade identity. `none`
  is a valid choice for homogeneous clean steel.
- Prompt 04 adds only authored decoration or stylization justified by the
  selected consumer.
- Prompt 05 is always read; it defaults to a verified skip.
- Prompts 06 and 07 integrate and prove only accepted components.
- Major unresolved visual choices receive one cheap comparison board. They do
  not create parallel production graphs.

The chain may produce a diagnostic production candidate without a Box asset.
It may not claim actual-sword acceptance until the named sword mesh is
available and passes Prompt 07.
