# Prompt 09: Repair the Highest-Impact Visible Failure

This prompt prevents shallow acceptance and endless unstructured iteration.

## Required reading

- package `WORKSTREAM.md`;
- package `CODED_DEMANDS.md`;
- Stage 08 visual review;
- proof images;
- professional benchmark comparison;
- stage manual that owns the failing behavior.

## If no repair is required

Verify that Stage 08 is `production-candidate`, all six proof groups exist,
focused tests are green, and no claim exceeds evidence.

Record `no repair required after adversarial proof` and set Stage 09 to
`complete`. Then execute Prompt 10.

## If repair is required

Critique the entire proof before editing. Create one defect ledger with these
rows even when a row is green:

- macro value organization;
- construction or manufacturing read;
- physical material response;
- repetition and visible tiling;
- symbols, decorative motifs, and pareidolia;
- feature-frequency hierarchy and quiet regions;
- close, gameplay, and distance read;
- consumer geometry, local-frame, and host integration;
- presentation-only issues.

Then rank causal owners by impact:

1. wrong construction or silhouette;
2. wrong scale, coordinate, or semantic identity;
3. missing macro value hierarchy;
4. missing or synthetic medium material structure;
5. weak edge or event design;
6. wrong physical response;
7. repetitive pattern;
8. microdetail density;
9. stylization selection;
10. presentation-only issue.

Choose the highest-impact causal surface component. Bundle every related
symptom that shares that owner into one reviewed repair batch. Do not cross
into unrelated owners, and do not run a complete production cycle for each
line item independently.

## Trace ownership

Map the failure to its canonical stage:

- research gap -> Prompt 02;
- incomplete intent or unsupported assumption -> Prompt 03;
- incomplete texture, node, shader, test, or production code -> Prompt 04;
- pattern, color, maps, or filtering -> Prompt 05;
- coordinate, attribute, material response, distance, or live stylization ->
  Prompt 06;
- contextual damage or overlay -> Prompt 07;
- missing or misleading proof -> Prompt 08.

Write a bounded repair statement and batch:

`[Visible failure] is caused by [canonical owner]. Repair [specific behavior]
without changing [protected accepted behavior]. Prove it with [test and
render].`

List every defect-ledger row addressed by that statement, every protected
accepted component, and the exact reduced proof that will expose collateral
damage.

## Red and green evidence

Before editing:

- preserve the failing proof;
- add or identify the focused failing test where practical;
- record the protected invariants.
- add or revise the coded demand, including exact test and production code,
  before applying the repair.

After editing:

- run the owning stage's focused tests;
- regenerate no proof or only the changed low-resolution proof while the batch
  is still converging;
- visually inspect;
- revise the same batch if related symptoms remain;
- once the batch converges, rebuild final maps and asset if the repair changes
  them;
- repeat reopen validation once;
- repeat Prompt 08 completely enough to detect collateral damage once.

## Repair discipline

- Do not add generic detail to compensate for missing structure.
- Do not move damage into the intact base.
- Do not weaken measurement or exclusion gates to make tests pass.
- Do not preserve a stale duplicate route.
- Do not call a repair complete from code inspection alone.
- Do not add more detail when substrate, macro value, construction, response,
  repetition, or host integration is the failing owner.
- Count each completed repair batch in the dossier, with the number of ledger
  defects closed, rather than counting each user observation as a new cycle.

## Loop

After one repair batch:

1. update the implementation ledger;
2. update `CODED_DEMANDS.md`;
3. update tests and manifests;
4. set the owning stage back to `complete`;
5. rerun Prompt 08;
6. return to Prompt 09.

Continue by causal batch until Stage 08 yields `production-candidate`, the
goal's repair budget or explicit scope is reached, or a genuine external
decision is required.

Do not mark the goal blocked merely because the repair is difficult. Follow
the active goal's blocker rules.

## Exit gate

Set Stage 09 to `complete` only when:

- Stage 08 is `production-candidate`;
- no required proof is missing;
- remaining defects are explicitly outside the bounded capability;
- tests and final artifacts match the latest repair.

Then execute `10_final_validation_and_handoff.md`.
