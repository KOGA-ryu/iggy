# Texture Goal Prompt Chain

This directory contains the detailed prompt sequence for executing one texture
or material capability. The user-facing workflow has five phases; these
eleven prompts remain the detailed manuals underneath them.

The prompts are written to be read and executed in numeric order. The active
goal supplies the material and capability; the prompts discover the existing
package, preserve one shared dossier, and carry evidence forward.

## Five-phase chain

1. `workflow/phases/01_scope_and_audit.md`
   - executes detailed Prompts 00–01;
2. `workflow/phases/02_research_and_intent.md`
   - executes Prompts 02–03;
3. `workflow/phases/03_blueprint_freeze.md`
   - executes Prompt 04, the optional cheap candidate-selection gate, and the
     hard champion-only build-entry gate;
4. `workflow/phases/04_build_prove_repair.md`
   - executes Prompts 05–09 one accepted surface component at a time, with
     fast repair and champion-only final proof modes;
5. `workflow/phases/05_acceptance_handoff.md`
   - executes Prompt 10 and automatic final audits.

Read and execute the five phase files in order. Open only the detailed prompts
owned by the active phase.

## Detailed prompt sequence

1. `00_goal_bootstrap.md`
2. `01_existing_asset_audit.md`
3. `02_reference_research.md`
4. `03_research_to_intent.md`
5. `04_coded_implementation_blueprint.md`
6. `05_pattern_and_map_build.md`
7. `06_shader_and_geometry_integration.md`
8. `07_damage_and_overlays_optional.md`
9. `08_adversarial_proof.md`
10. `09_repair_loop.md`
11. `10_final_validation_and_handoff.md`

Read `GOAL_DOSSIER_TEMPLATE.md` during bootstrap and
`CODED_DEMAND_TEMPLATE.md` before implementation.

## Execution rule

- Execute the stage; do not merely summarize its prompt.
- Read all inherited `AGENTS.md` files and the stage manual named by the
  prompt.
- Use the active material package's `WORKSTREAM.md` as the shared dossier.
- Use the active material package's `CODED_DEMANDS.md` as the reviewed
  implementation blueprint.
- Use `workflow/TEXTURE_WORKBENCH.md` as the persistent material, tool, donor,
  and strategy shelf. Resolve composite nicknames into substrate,
  construction, finish, condition, stylization, and renderer ownership before
  package planning.
- Copy the relevant at-hand tools into the dossier and retain at least two
  strategies for every unresolved major component until research, static
  review, or one cheap comparison board selects a route.
- Choose `hero-master`, `reusable-family`, or `bounded-variation` in Phase 1
  and apply that tier’s evidence burden.
- Document every texture, Blender node, socket link, shader path, test, and
  complete executable code block directly beneath the demand it satisfies.
- Decompose the capability into causal surface components before describing a
  complete graph. Each component must own its meaning, eligible and protected
  regions, local frame, feature-size band, output lanes, composition,
  rest/absence rule, and isolated rejection proof.
- When Prompt 04 leaves a major art-direction choice ambiguous, generate one
  disposable reduced-resolution board of 3–4 deliberately different
  candidates, including a quiet control. Use the named real consumer with
  fixed front/three-quarter, grazing, gameplay-distance, repetition, and
  decisive-mask views.
- Candidate sketches use temporary outputs and do not receive individual
  package synchronization, final-resolution builds, saved-file checks, or full
  test suites. Reject pareidolia, decorative symbols, visible landmarks,
  equal-frequency noise, and candidates that merely add detail. Preserve only
  the chosen semantic parameters and rejected reasons.
- Freeze and open the production boundary only after the candidate board
  selects a champion. If every candidate fails, return to research or the
  component blueprint rather than integrating the least-bad option.
- Prompts 05 through 09 may apply or revise code only through a frozen coded
  demand and matching open build entry.
- Verify existing evidence before redoing work.
- Record source paths, commands, test results, proof paths, visible findings,
  exclusions, and remaining risk in the dossier.
- Do not skip a visible phase. Detailed stages may be marked
  `verified-existing` or `skipped-out-of-scope` only with current evidence.
- Do not advance when the exit gate is red.
- When a late proof exposes an earlier failure, run `09_repair_loop.md`,
  critique the entire proof into one defect ledger, batch related corrections
  beneath the highest causal owner, return to the owning stage prompt, then
  repeat proof and final validation once the batch converges.
- Complete an active goal only when its exact objective is achieved and no
  required stage remains.
- An isolated fixture may deliver a production candidate; tier-required
  actual-target proof and explicit review are required for `accepted`.

## Branching

`07_damage_and_overlays_optional.md` is always read. If damage is outside the
goal, it records a verified skip and tests that damage defaults remain off.

`09_repair_loop.md` is always read. If every proof is green, it records
`no repair required` and advances. If not, it owns the bounded repair cycle.

## What the goal must say

A useful goal names:

- one material or construction family;
- one visible capability;
- the intended asset or scene use;
- the target quality or professional comparison family;
- the required final artifact;
- whether manual acceptance is part of completion.

The bootstrap prompt may narrow an overbroad goal to one capability, but it
must not silently change the user's intended material, style, engine, or
delivery target.
