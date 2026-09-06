# Prompt 04: Write the Complete Coded Implementation Blueprint

Do not implement the material from prose. Write the exact tests and production
code beneath every demand before applying those changes.

## Required reading

- package `WORKSTREAM.md`;
- `workflow/03_coded_implementation/AGENTS.md`;
- `workflow/prompts/CODED_DEMAND_TEMPLATE.md`;
- package `RESEARCH_INTENT.md`, profile, patterns, generator, builder, tests,
  and latest manifests;
- installed Blender API and current node/socket behavior when Blender is in
  scope.

## Create the coded-demand document

Create or update `<package>/CODED_DEMANDS.md`.

Populate its document contract, complete texture inventory, complete node and
group inventory, and complete end-to-end shader flow.

The inventories must name:

- every generated and consumed texture;
- physical spans and metres per texel;
- formats, color spaces, bit depths, and packed channels;
- coordinate and distance behavior;
- every new or changed node;
- node types, operations, socket defaults, and exact links;
- physical material layers;
- stylization;
- target-engine consumer and parity status.

## Decompose the capability into surface components

Do not begin with one complete material graph or a list of generic noise
layers. Create causal surface-component demands for the relevant subset:

- substrate or physical material identity;
- broad macro value organization;
- medium construction, growth, weave, tooling, forging, or fracture structure;
- geometry-supplied edge, cavity, contact, cut-face, and event response;
- microstructure;
- restrained stylization;
- separately authorized damage or condition;
- packing, engine adapters, proof, manifest, save, and reopen.

Each demand must produce one reviewable result.

For every surface component, record:

- physical or manufacturing meaning and source confidence;
- eligible regions and explicitly protected regions;
- local coordinate frame and element identity;
- normalized feature-size or frequency band on the actual host;
- affected color, height, normal, roughness, AO, metalness, opacity, mask, or
  stylization lanes;
- exact composition operation and order;
- rest or absence rule;
- isolated rejection proof and cumulative proof;
- reusable classification: exact reuse, parameterized recipe, shared profile
  or motif, assembly graph, or bespoke hero layer.

Assign every visible effect to its primary owner: geometry, semantic
attribute, authored texture or mask, shader response, decal, optional overlay,
lighting, or post-process. A demand may consume another owner's semantic mask;
it may not use one lane to conceal an absent owner.

## Plan the reusable surface grammar

Before the final material assembly, write complete coded demands for the
relevant reusable primitives:

- stable local-coordinate and identity primitives;
- finite motif or mark vocabularies with repetition safeguards;
- macro value-field builders;
- directional worked-surface builders;
- real-geometry edge, cavity, contact, and cut-face masks;
- signed height and tangent-normal composition;
- roughness-response shaping independent from height;
- physical layer mixing such as oxide versus exposed conductor;
- independently switchable stylization lanes;
- channel packing and Blender/target-engine adapters.

Do not duplicate a full node graph for every candidate or material variation.
Keep one canonical reviewed group or generator responsibility and express
variants through small semantic parameter records.

## Write tests directly under each demand

Write complete executable test code before production code.

When `surface_method_contract` is required, the tests must reject:

- missing or duplicate effect order;
- an unnamed composition operation;
- absent mask or coordinate ownership;
- unproved geometry-to-map transfer;
- invalid proof IDs;
- enabled damage without eligible and protected semantics.

The tests must name:

- exact file;
- exact class or insertion context;
- profile or manifest authority;
- expected red failure;
- final semantic assertion.

Do not describe a future test. Script it.

## Write production code directly under each demand

Under the same demand, write the complete:

- profile or pattern JSON;
- generator functions;
- Blender builder functions;
- node creation and linking;
- geometry-attribute assignment;
- validation and manifest code;
- proof fixture changes;
- commands.

Every block must be ready to apply. It may not contain ellipses, TODOs,
undefined planned helpers, omitted socket setup, or pseudocode.

For Blender nodes, script:

- group interface;
- node name and `bl_idname`;
- operation or mode;
- all non-default inputs;
- every incoming and outgoing link;
- image color space;
- custom material properties;
- validation assertions.

For textures, script:

- physical-coordinate construction;
- deterministic pattern compilation;
- filtering;
- color and PBR interpretation;
- map writing;
- manifest entries;
- tests.

## Review code quality before application

For each demand:

1. trace every symbol;
2. check units;
3. check array shapes and memory;
4. check deterministic seed ownership;
5. check color space and bit depth;
6. check Blender socket availability;
7. check object and attribute domains;
8. check target-engine translation;
9. check the test fails for the intended reason;
10. check the proof exposes failure rather than flattering it.

Record this review in the demand's execution record.

Then perform one whole-blueprint visual-risk review. It must answer:

1. Does each visible structure have one causal owner?
2. Can the intended broad-to-fine hierarchy be predicted from operations and
   amplitudes?
3. Are feature-size bands genuinely distinct?
4. Is any grayscale field copied lazily into unrelated output lanes?
5. Are color, height, normal, roughness, AO, and metalness physically
   coherent?
6. Can a finite motif form faces, animals, diamonds, symbols, stripes, or
   obvious repeated silhouettes?
7. Are quiet regions explicitly protected?
8. Are all nodes live and all parameters inside deliberate envelopes?
9. Will the graph repeat safely and survive the actual asset's local frames?

Any unanswered question keeps Stage 04 incomplete.

## Optional cheap candidate selection

If a major art-direction decision remains genuinely ambiguous after static
review, encode 3–4 deliberately different parameter records, including a quiet
control. Do not create separate complete graphs.

After the coded demands are reviewed, execute only those parameter records at
reduced map and render resolution in a non-canonical sweep directory. Use the
same named real consumer, front/three-quarter camera, grazing camera,
gameplay-distance camera, lighting, exposure, and map budget. Include a
3x3-or-larger repetition proof and the decisive isolated channel or mask.

Critique one comparison board across all candidates. Reject pareidolia,
decorative symbols, repeated landmarks, visible tiles, equal-frequency noise,
and options that merely add detail. Preserve the board, ranking, champion
parameter record, and rejected reasons. Remove discarded candidate paths from
the production blueprint before freeze.

This comparison does not authorize canonical output, final resolution,
individual package synchronization, saved-file validation, complete regression
testing, or acceptance. If every option fails, revise research or the
component blueprint rather than selecting the least-bad candidate.

## Do not apply code in this stage

This stage freezes only the selected implementation blueprint. API probes and
candidate comparisons must remain read-only or temporary and record the
verified result. Production edits begin in Prompt 05.

## Freeze and open the build boundary

After Stage 04 is complete, run:

```sh
python3 assets/creative/materials/workflow/scripts/material_workflow_gate.py \
  freeze \
  --package <package> \
  --workstream <package>/WORKSTREAM.md \
  --coded-demands <package>/CODED_DEMANDS.md \
  --tier <tier> \
  --target <each-production-target> \
  --red-gate-evidence "<exact failing gate and result>" \
  --revision-note "<bounded demand set>"

python3 assets/creative/materials/workflow/scripts/material_workflow_gate.py \
  open-build \
  --package <package>
```

The second command must run before any production target changes. If it
rejects the entry, restore the reviewed ordering by revising and freezing the
blueprint; do not bypass the state file.

## Dossier update

Record:

- `CODED_DEMANDS.md` path;
- demand IDs;
- texture inventory count;
- live node inventory count;
- shader flow summary;
- code-review status;
- expected red gates;
- owning implementation prompt for every demand.

Set Stage 04 to `complete` only after every in-scope demand contains complete
test and production code.

## Exit gate

Do not advance unless:

- exact texture, node, and shader inventories exist;
- every demand has executable tests;
- every demand has executable production code directly beneath it;
- the ordered effect stack and geometry-to-map boundary are exact;
- no placeholders or undefined new helpers remain;
- code has been reviewed against current files and Blender API;
- build, validation, and proof commands are exact.
- any ambiguous major visual decision has either been resolved by one cheap
  comparison board or explicitly removed from the capability;
- only one champion semantic parameter record remains on the production route;
- `WORKFLOW_STATE.json` records the frozen blueprint and verified open build
  entry.

Then execute `05_pattern_and_map_build.md`.
