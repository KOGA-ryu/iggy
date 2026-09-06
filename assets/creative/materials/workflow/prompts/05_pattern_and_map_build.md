# Prompt 05: Build the Authored Pattern and Texture Maps

Implement the intent through one canonical deterministic pattern and map route.

## Required reading

- package `WORKSTREAM.md`;
- package `CODED_DEMANDS.md`;
- `workflow/04_pattern_and_map_authoring/AGENTS.md`;
- package profile and `RESEARCH_INTENT.md`;
- existing generator and focused generator tests.

## Verify frozen build entry

Before changing the pattern, profile, generator, or tests, run:

```bash
python3 assets/creative/materials/workflow/scripts/material_workflow_gate.py \
  verify-entry \
  --package <package-directory>
```

Stop if the command is red. Return to Prompt 04, revise the coded demand,
freeze the revised blueprint, and open a new build entry. A prior successful
freeze is not permission to work after a coded-demand or target-baseline
change.

## Red evidence

Run the scripted red gates from the relevant coded demands. Record the
failures in the dossier and in each demand's execution record.

## Author pattern data

Create or repair:

- finite motif or pattern sources;
- physical dimensions;
- local identities and frames;
- family, phase, and variant;
- quiet regions;
- bounded transforms;
- construction-aware placement.

Reuse a small high-quality source shape through declared transforms where that
matches the material. Do not reuse one obvious landmark unchanged.

Apply the reviewed profile, pattern, generator, validation, and generator-test
blocks from `CODED_DEMANDS.md`. If current file context requires a change,
update the coded demand first and review the revised code before applying it.
Do not improvise an undocumented alternate implementation.

## Compile layers

Implement and save isolated:

1. pattern or construction;
2. macro;
3. medium;
4. edges and selected events;
5. micro;
6. cumulative color;
7. height and normal;
8. roughness, AO, and metalness;
9. stylization masks;
10. optional overlay eligibility.

Keep damage disabled unless the active goal includes it.

Execute layers in the profile's recorded effect order. Persist generated masks,
manual-correction masks, directional transforms, and bake identities as
separate deterministic inputs. Do not flatten them merely to simplify the
compiler.

## Quality requirements

- Use stable local frames.
- Keep real feature scales independent of resolution.
- Use several related shades inside each element.
- Preserve quiet areas.
- Break repetition through bounded structure, not random noise.
- Band-limit subpixel features.
- Keep base color free of baked light and AO.
- Keep roughness causal and distinct from height.
- Encode height with a declared metre range and suitable bit depth.
- Document every packed channel.

## Generator tests

Prove the relevant subset:

- deterministic champion;
- pattern closure;
- dimensions and counts;
- measured ranges;
- variant bounds;
- local-frame coverage;
- quiet-area survival;
- color-family use;
- seam metrics;
- height amplitude;
- normal orientation;
- roughness, AO, and metalness rules;
- output dimensions, bit depth, and channel packing;
- source, profile, and pattern provenance;
- damage and excluded overlays remain off.

Run the focused tests until green. Do not move to shader work with a red
generator contract.

## Low-resolution visual inspection

Generate reduced-resolution proofs before the final maps. Inspect:

- pattern regularity;
- within-element shades;
- medium structure;
- rest areas;
- repeated landmarks;
- edge behavior;
- base-color value grouping;
- microdetail density.

Repair the generating layer, not the proof layout.

## Final map build

After the hierarchy reads:

- generate final-resolution maps;
- write the manifest;
- record resolution, physical span, metres per texel, seed, version, hashes,
  and constraints;
- verify source images or AI experiments have not entered runtime maps unless
  explicitly authorized and documented.

## Dossier update

Record:

- executed coded-demand IDs;
- canonical pattern and generator;
- layer-to-file ownership;
- test commands and counts;
- map and manifest paths;
- low-resolution findings and repairs;
- final-resolution metrics;
- remaining shader-dependent proof.

Set Stage 05 to `complete` only when final maps and focused generator gates are
green.

## Exit gate

Do not advance unless:

- the pattern is authored and deterministic;
- each frequency band has an isolated output;
- map causality matches the intent;
- effect order and geometry-to-map transfer match the method contract;
- within-element color variation is visible;
- repetition and rest have been inspected;
- final maps and manifest exist;
- frozen build entry remains valid;
- focused generator tests pass.

Then execute `06_shader_and_geometry_integration.md`.
