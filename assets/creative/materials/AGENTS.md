# Texture and Material Agent Guidance

This file governs every material package under `assets/creative/materials/`.
It supplements the repository-level `AGENTS.md`; it does not weaken the
Creative-only, direct-work, focused-verification, uncommitted-delivery, or
unrelated-change preservation rules.

The job is not to manufacture a plausible square image. The job is to turn
material research into a reusable, scale-bearing material system whose
geometry contract, texture maps, shader behavior, stylization lanes, and proof
scene agree.

## Required workflow reading

Read the relevant stage file completely before performing that stage:

1. `workflow/01_reference_research/AGENTS.md`
2. `workflow/02_material_intent/AGENTS.md`
3. `workflow/03_coded_implementation/AGENTS.md`
4. `workflow/04_pattern_and_map_authoring/AGENTS.md`
5. `workflow/05_shader_integration/AGENTS.md`
6. `workflow/06_damage_and_overlays/AGENTS.md` only when damage or narrative
   surface history is in scope
7. `workflow/07_proof_and_acceptance/AGENTS.md`

Also read these shared benchmarks before starting a new material family or
claiming a major quality milestone:

- `QUALITY_BENCHMARK.md`
- `MEASURED_CONSTRUCTION_DOSSIER.md`
- `workflow/TEXTURE_WORKBENCH.md`
- `workflow/LAYERED_GRAPH_REFERENCE.md`
- `workflow/SURFACE_AUTHORING_METHOD_REFERENCES.md`

Read the package's existing `README.md`, `RESEARCH_INTENT.md`, profile,
patterns, tests, and latest manifest before modifying an existing material.

## Goal-driven prompt chain

When an explicit material goal is active, read
`workflow/prompts/README.md` and execute the five visible phase files under
`workflow/phases/`. Each phase routes to the relevant detailed prompts from
`00_goal_bootstrap.md` through `10_final_validation_and_handoff.md`.

The chain maintains `<material-package>/WORKSTREAM.md` as the persistent
evidence dossier. Phase 1 must classify the work as `hero-master`,
`reusable-family`, or `bounded-variation`; do not apply the full hero burden
to a color preset, or the variation burden to new anatomy.

Phase 3 is the selection and hard production boundary. Review the complete
component-coded blueprint first. When a major visual decision is still
ambiguous, compare 3–4 disposable reduced-resolution candidates on the named
consumer before freezing anything. Only the selected semantic parameter
record enters the frozen production blueprint. Then freeze the coded demands
with `workflow/scripts/material_workflow_gate.py`, open the verified build
entry, and only then edit production targets. When proof exposes a
design-changing failure, revise and freeze a batched repair demand before
applying the repair.

Phase 1 must open the texture workbench before package planning. Copy the
selected normalized material card, relevant at-hand tools, donor status, and
at least two strategies for every unresolved major component into
`WORKSTREAM.md`. Do not repeatedly rediscover a donor during implementation.
If the required tool is missing, make that reusable tool the bounded
dependency before the material component that needs it.

Use `workflow/scripts/audit_material_package.py` during Phase 4 and Phase 5.
It must reject stale canonical outputs, dormant texture or Attribute nodes,
unowned geometry attributes, unsupported numeric claims, performance-budget
overruns, coded-demand drift, hash mismatch, and accepted status without the
tier-required actual-target proof.

Every new or materially revised `hero-master` or `reusable-family` profile
must declare `workflow_contract.surface_method_contract`. It records the
ordered effect stack, composition operations, mask sources, coordinate frames,
geometry-to-map transfer, damage placement state, consumers, and proofs.

## Governing principles

### A material is a system

Keep these responsibilities distinct:

- geometry owns silhouette, real cavities, joins, deep breaks, and features
  whose parallax or shadow must survive a viewpoint change;
- texture maps own repeatable surface evidence at declared physical scales;
- the shader owns coordinate reconstruction, physical response, map
  combination, distance behavior, and adjustable stylization;
- explicit attributes or masks own semantic identities such as end grain,
  cut face, exposed metal, mortar, fracture interior, repair, contact, or
  traversal emphasis;
- overlays own optional contextual history such as soot, dust, damp, polish,
  blood, wax, or localized damage.

Do not ask one lane to conceal the absence of another. A normal map cannot
repair a weak silhouette. Dark base color cannot replace cavity geometry.
Screen-space outlines cannot invent material-specific drawing.

### Component-built appearance workflow

Treat a material like a successfully component-built prop: understand,
specify, code, isolate, accept, and only then assemble each causal surface
component. A complete material graph is not the first unit of authorship.

Use this order:

1. **Consumer and reference investment gate.** Bind the capability to a named
   real asset, visible surface, dimensions, local frame, cameras, lights, and
   gameplay distance. Score the reference set for authority, view coverage,
   construction evidence, frequency evidence, professional relevance, and
   ambiguity. Research weak evidence instead of filling it with procedural
   invention.
2. **Surface anatomy decomposition.** Name causal components rather than
   generic noise layers. For each component record its physical or
   manufacturing meaning, source confidence, eligible and protected regions,
   local coordinate frame, normalized feature-size band, affected output
   lanes, exact composition operation, absence rule, and isolated rejection
   proof.
3. **Lane and ownership decision.** Assign every visible effect to exactly one
   primary owner: geometry, semantic attribute, authored texture or mask,
   shader response, decal, optional overlay, lighting, or post-process. A lane
   may consume another lane's semantic mask; it may not conceal that the
   owning lane is missing.
4. **Reusable surface grammar.** Plan coordinate primitives, finite mark
   vocabularies, macro value fields, directional worked-surface builders,
   geometry-supplied edge/cavity/contact masks, height/normal composition,
   roughness shaping, physical material-layer mixing, stylization lanes,
   packing, and engine adapters. Classify each as exact reuse, parameterized
   recipe, shared profile or motif, assembly graph, or bespoke hero layer.
5. **Complete coded blueprint.** Beneath every component demand, write the
   exact tests and complete production code or node inventory: inputs,
   outputs, node types, links, coordinates, masks, parameters, seeds,
   composition order, channel meanings, color spaces, and host attributes.
   Review the complete graph statically before Blender or full map generation.
6. **Cheap candidate selection.** Only when a major decision remains
   ambiguous, build 3–4 deliberately different reduced-resolution candidates
   in one disposable batch, including a quiet control. Use identical real
   consumer geometry, cameras, lights, exposure, and map budgets. Show
   front/three-quarter, grazing, gameplay-distance, 3x3-or-larger repetition,
   and the decisive isolated channel or mask. Critique the board once, reject
   failures, and retain only the decision plus rejection reasons.
7. **One component at a time.** Build broad to fine: substrate identity,
   macro value, medium construction, geometry or event response,
   microstructure, restrained stylization, then separately authorized
   condition. Execute the reviewed code, inspect isolated and cumulative
   proofs, record the repair count, and accept or repair the component before
   assembly.
8. **Cumulative assembly.** Assemble only accepted components. Preserve
   separate switches and owners for damage, stylization, contact polish, soot,
   damp, rust, and other narrative layers.
9. **Actual-asset acceptance.** Swatches and spheres prove components, not the
   finished capability. Final acceptance requires the named consumer under
   neutral, grazing, and gameplay lighting, plus channel isolation,
   repetition evidence, direct reference comparison, and stated shortcomings.
   Blender proof does not establish engine parity.
10. **Batched repair and donor registration.** Critique the complete failed
    proof before editing. Build one defect ledger covering macro value,
    construction read, response, repetition, symbols or pareidolia, hierarchy,
    distance read, and host integration. Bundle related corrections beneath
    the highest causal owner, run no-proof or one-changed-proof builds while
    converging, and regenerate the full proof set once. After acceptance,
    register reusable components in a cross-material primitive census rather
    than preserving only the final graph.

Exploration is cheap and batched. Production is narrow and exact.
Full-resolution maps, synchronized documentation, saved-file verification,
and focused regression gates are required for the selected champion or a
newly accepted reusable component, not for every visual guess.

### Research becomes intent before code

For every visible behavior, preserve this chain:

`source observation -> bounded interpretation -> authored intent -> script or
node implementation -> isolated proof -> final observable result`

If a visible behavior cannot name its source, intended role, physical scale,
and proof, it is not ready to enter the champion material.

### Measurements replace convenient guessing

Author in metres. Record the scale of blocks, boards, rings, fibres, pores,
tool marks, pits, joints, cracks, and repeats when the evidence supports them.
Classify every value as one of:

- surveyed or catalogued;
- laboratory characterized;
- published technical guidance;
- qualitative authority;
- explicit proxy transfer;
- authored translation;
- unknown and omitted.

Never silently promote an authored translation or proxy into a measured fact.
When a dimension is unknown, omit that geometric or relief claim, find better
evidence, or use a clearly labeled proxy with a narrow transfer boundary.

### One capability per workstream

Choose one visible material capability, such as:

- intact longitudinal oak grain;
- oak end grain;
- forged scale over iron;
- three-strand rope construction;
- intact dressed ashlar;
- lime plaster body;
- one fracture-interior family;
- one contact-polish overlay.

Do not simultaneously build the intact material, every damage type, every
weather condition, a trim sheet, decals, and an engine master material.
Finish and prove one capability, preserve its neutral source, then add the
next lane.

### Layering must be causal

Do not describe a stack as layered merely because it contains many noises.
Every layer needs:

- a material or construction meaning;
- its own frequency and amplitude band;
- a relationship to local orientation or element identity;
- a color, height, normal, roughness, AO, metalness, mask, or stylization role;
- an explanation of where it is absent;
- a proof that isolates it.

It also needs an explicit order and composition operation. “Blend” is not
specific enough: state whether the layer is geometry union, signed-height
composition, linear colour interpolation, roughness modulation, physical
layer mixing, tangent-normal detail composition, or another exact operation.

Shared structure may drive multiple outputs, but those outputs must interpret
it differently. Roughness must not be a grayscale copy of height. Base color
must not contain photographed light or AO. Metalness must follow physical
layer identity.

### Broad-to-fine visual hierarchy

Build and evaluate in this order:

1. construction and silhouette;
2. macro value and material-family grouping;
3. medium planes, growth, weave, tooling, or fracture structure;
4. selected edges and material events;
5. microstructure;
6. optional stylization;
7. contextual damage and overlays.

Protect quiet areas. More marks do not imply more quality. Microdetail must
not erase large value groups at gameplay distance.

### Local frames beat global noise

Each physical element needs a stable local frame and identity where relevant:
board, block, strand, yarn, metal component, plaster patch, cut face, or trim
segment. Grain follows the tree. Tooling follows the worked face. Rope twist
follows accumulated curve distance. Mortar negotiates neighboring units.

Global fields may modulate a composition. They may not be the primary author
of every element.

### Stylization is authored, not sprayed on

Arcane-, Borderlands-, BOTW-, or BG3-adjacent work requires controlled value
grouping and deliberate mark selection, not generic grunge or a universal
outline. Keep optional stylization lanes independently addressable:

- structural ink;
- highlight strokes;
- brush or fibre direction;
- broad pigment variation;
- detail priority or protected rest;
- traversal or gameplay emphasis when explicitly required.

Lighting and screen-space outlines remain renderer concerns. Material-specific
mark placement remains an asset concern.

## Source and AI policy

- Prefer written technical sources, catalogues, conservation documents,
  standards, papers, official manuals, and detailed artist breakdowns.
- Do not watch a video as the research method. Obtain and inspect a transcript
  when a video is uniquely useful.
- Translate reference images into written observations, measurements,
  silhouette diagrams, palettes, or explicit masks. Do not treat "looks like
  the reference" as an implementation plan.
- Do not copy third-party runtime maps into the repository.
- Do not generate AI reference or production images unless the user explicitly
  authorizes that experiment in the current task.
- An authorized AI experiment must remain isolated and source-labeled. It
  cannot establish anatomy, dimensions, material science, historical
  construction, or acceptance quality.
- If the user says no AI imagery, treat that as a hard constraint.

## Package ownership

A mature package normally contains:

- `WORKSTREAM.md`: active goal state, evidence ledger, stage status, and
  current acceptance boundary;
- `CODED_DEMANDS.md`: exact texture, node, shader, test, and executable
  production code placed beneath every implementation demand;
- `workflow/scripts/render_coded_demands.py`: optional synchronization and
  final drift check when a package embeds complete executed source files and
  an exact Blender node/link inventory in `CODED_DEMANDS.md`;
- `README.md`: current capability, reproduction commands, accepted boundary,
  and honest remaining risks;
- `RESEARCH_INTENT.md`: source observations translated into intent,
  implementation, and unsupported claims;
- `profiles/`: physical scales, coordinate contract, output channels,
  defaults, unknowns, and engine translation;
- `references/`: source ledgers, numeric captures, licenses, transcripts, and
  text analyses, not an uncurated image dump;
- `patterns/` or `motifs/`: finite authored structures and bounded variation
  parameters;
- one canonical deterministic generator;
- one canonical Blender build and proof route when Blender integration exists;
- focused generator, contract, and reopened-asset tests;
- `output/`: generated maps, packed `.blend`, manifests, and proof renders.

Do not create empty directories or boilerplate files just to satisfy this
shape. Add an artifact only when it owns a real decision or proves a real
boundary.

## Canonical ownership rules

- One profile owns the declared physical and semantic contract.
- `CODED_DEMANDS.md` owns the reviewed code blueprint for the active
  capability; live code may not silently diverge from it.
- When `CODED_DEMANDS.md` embeds complete files, re-render it after each
  approved implementation or repair and run the renderer with `--check`
  during final validation. A generated document is not permission to skip
  the pre-implementation code review.
- One pattern or motif source owns each authored structural vocabulary.
- One generator owns compilation of those sources into maps.
- One Blender builder owns the canonical node graph and proof fixture.
- Tests consume the same profile and manifest; they do not duplicate the
  material recipe as independent constants.
- A proof adapter may realize geometry or assign attributes, but it must not
  become a second material-authoring path.
- Generated manifests report truth; do not hand-edit them.

## Required pre-build brief

Before implementing a new material capability, write or update the package
intent so it answers:

1. What exact object or construction family receives the material?
2. Which visible surface is in scope?
3. What physical size and orientation does that surface have?
4. Which reference family and professional quality benchmarks apply?
5. What are the macro, medium, and micro structures?
6. Which color families exist inside one physical element?
7. What changes height, normal, roughness, AO, and metalness, and why?
8. Which semantic masks or attributes are required?
9. Which viewing distances must retain each detail band?
10. Which damage, weather, and narrative lanes are excluded?
11. How will Blender and the target engine reconstruct the material?
12. What exact proof would cause rejection?

Proceed without a separate approval pause when the user's scope already
answers these questions and the remaining assumptions are bounded. Stop and
request direction only when a missing choice would materially change the
asset family or authorize unsupported external use.

## Implementation sequence

Use this order unless the material supplies a documented reason to change it:

1. bind the capability to a named consumer and fixed viewing conditions;
2. score and research the physical, construction, and professional references;
3. decompose the surface into causal components and assign lane ownership;
4. write the source-to-intent, measurement, absence, and rejection ledger;
5. plan the reusable surface grammar before the finished material;
6. document every texture, node, shader flow, test, and complete production
   code directly beneath its component-coded implementation demand;
7. statically review hierarchy, frequency separation, channel coherence,
   quiet regions, motif risk, parameter envelopes, and local-frame stability;
8. when ambiguity remains, run one reduced-resolution comparison batch on the
   real consumer and reject symbols, pareidolia, obvious repetition,
   equal-frequency noise, or candidates that merely add detail;
9. place only the selected semantic parameter record into the blueprint, then
   freeze and open the production build;
10. build and accept substrate, macro, medium, event, micro, and stylization
    components individually, preserving damage as a separate authorization;
11. assemble accepted components through the reusable named shader graph;
12. render adversarial close, grazing, gameplay-distance, repetition,
    isolated-channel, and actual-consumer proofs;
13. critique the whole proof, batch related corrections by causal owner, and
    regenerate the full set only after the batch converges;
14. build final-resolution maps, reopen the saved asset, run focused tests,
    compare to the professional references, and state visible shortcomings;
15. register accepted reusable components and cross-material donors before
    calling the bounded capability accepted.

## Prohibited shortcuts

Reject these approaches:

- one flat shade per board, block, or component;
- one noise field stretched across unrelated pieces;
- evenly spaced sine-wave wood grain;
- candy-cane rope stripes without nested opposite-hand construction;
- Voronoi stone presented as fitted masonry;
- black lines presented as cracks without fracture structure;
- random damage placed without force, exposure, traffic, contact, or repair
  logic;
- roughness copied from height;
- AO baked into base color;
- photographic or generated lighting baked into base color;
- uniform rust, universal grime, or fuzz over the complete asset;
- arbitrary bevels, groove depths, or pit sizes disguised as "stylized";
- a material called seamless after only a single-tile view;
- a material called high quality because maps and explanatory pages exist;
- a Blender node tree assumed to equal Unreal parity;
- acceptance based only on the generator's own flattering proof.
- finite masks whose repeated silhouettes read as faces, animals, diamonds,
  symbols, stripes, or other unintended decorative language;
- accepting a patterned candidate because it is subtle on the beauty render
  when its isolated 3x3 mask exposes repeated landmarks;
- rerunning a full production build for each isolated user observation instead
  of collecting one complete defect ledger and applying one reviewed batch;
- using additional microdetail to rescue failed substrate, macro value,
  construction, or material-response components.

The forged-iron hinge calibration is the prohibited reference case: compact
oxide-scale motifs formed diamond and cat-face readings when repeated, and
serial full-build tweaks turned every small observation into an expensive
repair cycle. Future forged-iron work must expose motif masks in the cheap
selection board and must batch the whole critique before production repair.

## Testing and proof

For a known regression, define the failing observable before the repair. For a
new capability, add contract tests alongside the first implementation and make
the final asset pass them before handoff.

Tests should cover the relevant subset of:

- deterministic output for a seed and variant;
- physical scale and measurement envelopes;
- pattern closure and seam behavior;
- local-frame continuity;
- bounded mark density and quiet-area coverage;
- map dimensions, color spaces, bit depth, and packed channels;
- absence of forbidden or unsupported lanes;
- node names, types, data flow, coordinate sources, and custom properties;
- packed images and saved-file provenance;
- reopened `.blend` geometry, attributes, and material graph;
- expected proof set and nontrivial file sizes.

Numerical gates prevent regression. They do not substitute for visual review.

## Headless and resource policy

- Use deterministic Python generation and headless Blender builds.
- Do not launch a window unless the user explicitly requests interactive
  inspection.
- Use no-proof or one-changed-proof temporary builds during repair loops.
- Fast or targeted builds must reject the canonical output directory.
- Start with reduced map and render resolution when evaluating composition.
- Build final-resolution maps and proofs only after the hierarchy reads.
- Stream or release large intermediate arrays when practical on the target
  Mac.
- Preserve the editable source, pack required images, save, reopen, and
  validate the evaluated result.

## Acceptance language

Use these terms precisely:

- `research prototype`: the method or evidence is incomplete;
- `structured prototype`: deterministic lanes exist but visible quality is
  below the benchmark;
- `production candidate`: the bounded capability passes its tier’s isolated,
  close, distance, repetition, save, and reopen proofs;
- `accepted`: the tier-required actual-target or real-consumer proof is green
  and the user or documented manual-review authority accepts the candidate.

An isolated fixture can establish a candidate. It cannot establish
`hero-master` acceptance.

Do not call work "Arcane-like", "Borderlands-like", "BG3-quality",
"professional", or "high quality" merely because that was the target. Name
what the proof actually establishes.

## Artifact surfacing and lifecycle catalog

Before showing a material image, recommending a donor, reporting what is
current, or choosing an artifact for another build, read
`MATERIAL_CATALOG.json` and obey its lifecycle classification.

`MATERIAL_CATALOG.json` is a generated read-only snapshot of the authoritative
Surface Foundry registry. Do not hand-edit it. Regenerate it with Surface
Foundry's `export-workbench-snapshot` command so cross-repository work and user
rulings remain one decision system. Use `lineage_views.current` for default
presentation, `lineage_views.workbench` for active component evidence, and
`lineage_views.archive` only when development history is explicitly requested.

- An accepted master outranks every newer candidate, preview, sweep, repair,
  or diagnostic.
- Use the entry's `preferred_proof` by default. Do not choose an image by file
  modification time, lexicographic order, output proximity, or the presence of
  a newer manifest.
- A production candidate must be labelled as a candidate.
- A structured prototype, research prototype, rejected repair, or superseded
  package may be shown only when the user explicitly asks for prototypes or
  development history.
- A component proof may establish a reusable donor without becoming the
  presentation image for the complete material.
- When conversation context and a package manifest disagree about acceptance,
  preserve the lower status until explicit user acceptance is bound in the
  catalog. Never infer acceptance from a passing test or finished render.

For forged iron specifically, the current non-prototype presentation owner is
`forged_iron_v1/output/forged_iron_v1.blend` and the root
`forged_iron_v1_*` proof set. This does not imply acceptance. Nested `preview`,
`sweep`, `cumulative`, and `repair` outputs remain quarantined development
history unless the catalog is explicitly revised after user review.

For sword steel, the Master Sword cyan-response study and its regional
Inkblotter colour evidence live in Surface Foundry but remain registered in the
same current lineage. A repository-local file search is not a complete sword
material inventory.

## Capability checkpoint

Stop after one material capability. Report:

- canonical owner and exact accepted boundary;
- source and measurement authorities;
- layers implemented and layers explicitly excluded;
- production files and file-count or LOC effect where Git can establish it;
- generated maps, shader asset, and visual proofs;
- focused verification performed;
- honest remaining visual and engine risks;
- next material capability.

Keep changes uncommitted unless the user explicitly asks for a commit.
