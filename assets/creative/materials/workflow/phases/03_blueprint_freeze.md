# Phase 3: Blueprint Freeze

Execute `workflow/prompts/04_coded_implementation_blueprint.md`.

This is the strict boundary between component planning, cheap visual
selection, and production edits.

## Required order

1. Decompose the appearance into causal surface-component demands.
2. Write complete red tests and complete production code under each demand.
3. Review every symbol, unit, texture, node, socket, attribute, proof route,
   local frame, feature-size band, absence rule, and composition operation.
4. Predict the visual hierarchy from the code. Reject shared grayscale copied
   into unrelated lanes, equal-frequency fields, unprotected quiet regions,
   dormant nodes, unexplained constants, and finite motifs capable of faces,
   diamonds, symbols, stripes, or obvious repeated silhouettes.
5. If no major visual decision remains ambiguous, select the reviewed
   component parameter record and continue to step 9.
6. Otherwise generate 3–4 deliberately different reduced-resolution
   candidates in one temporary batch, including a quiet control. Candidates
   must use identical named consumer geometry, cameras, lights, exposure, map
   budget, and local frames.
7. Assemble one comparison board with actual-consumer front/three-quarter,
   grazing-light, gameplay-distance, 3x3-or-larger repetition, and decisive
   isolated-channel or mask views. Critique the whole board once and record
   every rejection reason.
8. Select one champion parameter record. If every candidate fails, return to
   Phase 2 or revise Prompt 04; do not freeze or integrate the least-bad
   candidate.
9. Remove disposable candidate code from the production blueprint, preserve
   only the champion parameters and rejection ledger, and mark Stage 04
   `complete` in `WORKSTREAM.md`.
10. Run `workflow/scripts/material_workflow_gate.py freeze`.
11. Immediately run `material_workflow_gate.py open-build`.
12. Only then edit a production target.

## Cheap candidate boundary

Candidate selection is evidence, not production:

- write candidates only to an explicit temporary or art-direction sweep
  directory;
- do not write the canonical map directory or save over the canonical
  `.blend`;
- do not synchronize full package documentation, build final-resolution maps,
  expand regression tests, or run the complete hero proof set for each option;
- do not promote an option because the beauty view hides its motif mask or
  repetition;
- preserve the board, decision, and rejected reasons long enough for review;
  the candidate implementations themselves are disposable.

`freeze` records:

- workflow tier;
- coded-demand SHA-256;
- demand IDs;
- target-file baseline hashes;
- profile, pattern, and test baselines.

`open-build` refuses entry when any target changed after freeze or the coded
demands no longer match. This makes the order enforceable rather than
aspirational.

## Repairs

When proof requires a design-changing repair:

1. critique the whole failed proof and write one defect ledger;
2. group related symptoms beneath their causal surface component;
3. update the batched demand and exact code first;
4. mark the repair batch in `WORKSTREAM.md`;
5. freeze one new revision;
6. open the new build entry;
7. apply only that revision.

Typographical documentation repairs that do not change implementation may be
made without reopening production.

## Hard exit gate

No selected champion plus `WORKFLOW_STATE.json` with a matching open build
entry means no production edit authorization. An exploratory comparison board
does not satisfy this gate.
