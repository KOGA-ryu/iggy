# Stage 7: Proof, Comparison, and Acceptance

Proof exists to expose failure. A polished contact sheet, long explanation, or
large script is not evidence that the material is good.

The proof must also audit `CODED_DEMANDS.md`: every executed demand must point
to its live file or node, green test, manifest entry, and isolated render.
Documented code that was never applied and live code that was never documented
are both repair failures.

## Freeze the review candidate

Before final proofs:

- record profile schema and champion seed;
- generate final-resolution maps;
- record source, profile, pattern, and map hashes;
- build the canonical shader;
- pack the images;
- save the `.blend`;
- reopen it in a separate headless process;
- run focused generator, contract, and reopened-asset tests.

Do not adjust hidden defaults after rendering without rebuilding the manifest
and proofs.

## Plate 1: Matched professional comparison

- Link at least two legally referenced professional examples.
- Match material role, crop scale, and approximate light direction as closely
  as practical.
- Compare silhouette, pattern, macro value, medium structure, edges, color,
  reflection, stylization, repetition, and scene use.
- Annotate at least five concrete visible differences.
- State which gaps are in geometry, maps, shader, lighting, or context.
- Do not redistribute unlicensed benchmark textures.

Reject the candidate if the comparison is superficial or if the candidate
only resembles the benchmark because of dramatic lighting.

## Plate 2: Neutral geometry and form under light

Show the relevant subset:

- neutral clay geometry before the material is assigned;
- front, back, both sides, and ends for an asset;
- flat plane, angled plane, cylinder or sphere, and corner for a tile;
- frontal, three-quarter, and grazing white light;
- height-only and normal-only response;
- wireframe or topology when geometry is part of the capability.

The material cannot be accepted if weak construction is hidden by color.

## Plate 3: Frequency and causality ladder

Show:

1. pattern or construction only;
2. macro only;
3. macro plus medium structure;
4. edge and event structure;
5. micro only;
6. cumulative height;
7. final normal;
8. final physical response.

Annotate physical span, amplitude, and intended viewing-distance band for each
level. A reviewer should be able to name what every visible band represents.

## Plate 4: Color and stylization

Show:

- unlit base color;
- cumulative color layers;
- one physical element enlarged so within-element shade variation is visible;
- desaturated value grouping;
- 64-pixel or representative gameplay thumbnail;
- structural ink;
- highlight strokes;
- brush or fibre direction;
- detail priority or protected rest;
- cel-ramp previews under at least three light directions when relevant.

Reject:

- one color per element;
- baked light or AO;
- generic black outlines;
- stylization visible only in an isolated mask but absent from the live shader;
- color noise that destroys the broad value composition.

## Plate 5: Reflection, PBR, and packing

Show:

- roughness under frontal, three-quarter, and grazing highlights;
- AO isolated from base color;
- metal and dielectric layers separately when relevant;
- normal bands isolated and combined;
- height with metre range;
- ORM or other packed channels individually and packed;
- wet, polish, or coating response only when that capability is in scope.

Report useful percentiles and ranges as regression evidence. Do not accept an
unconvincing reflection story because the numbers fall inside a broad range.

## Plate 6: Repetition, distance, and world use

Show:

- 1x1, 4x4, and 16x16 repetitions;
- at least two compatible variants together;
- full resolution, half resolution, and representative mip or distance views;
- close, gameplay, and distant camera positions;
- a material-appropriate corner, trim, curved surface, or asset transition;
- the actual target asset or a representative modular assembly;
- one optional overlay example only when overlays are in scope.

Look for:

- square-tile landmarks;
- repeated knots, cracks, stains, plates, or tool marks;
- moire and diagonal dot chains;
- distance speckle;
- lost medium structure;
- swimming coordinates;
- stretched features;
- seams after transforms;
- material scale changes between different-sized objects.

## Lighting requirements

Use at least:

- neutral material light;
- grazing light;
- one game-like light.

Keep exposure and color management recorded. Do not use cinematic darkness,
fog, bloom, depth of field, or heavy color grading to pass the material gate.

## Actual asset requirement

The final candidate must be shown on the geometry it is intended to serve:

- the referenced door for structural wood and forged hardware;
- a measured beam for timber;
- a curve-deformed bridge, hoist, or lashing for rope;
- a wall corner, opening, or modular run for masonry and plaster;
- the corresponding prop, building part, or traversal surface for other
  materials.

A sphere or flat tile is diagnostic, not final acceptance.

## Automated gates

Use the smallest focused set that proves:

- source and profile parsing;
- measurement and pattern invariants;
- deterministic map generation;
- output format and channel packing;
- shader nodes, links, coordinates, and defaults;
- geometry and semantic attributes;
- packed images;
- final render set;
- reopened saved file.

Do not run a broad CTest loop. Do not repeat a green gate without a reason.

## Brutally honest review

After inspecting every proof, write:

- strongest visible achievement;
- weakest visible area;
- comparison gaps still present;
- any procedural signature that remains obvious;
- any unsupported claim still tempting;
- whether the material survives close, gameplay, and distant ranges;
- whether it works on the actual asset;
- repair-pass count;
- exact next repair in priority order.

Do not praise effort, line count, research volume, or technical complexity.
Judge the rendered surface.

## Acceptance decision

Use one result:

- `rejected`: visible or contract failure invalidates the capability;
- `repair requested`: bounded defects remain and the repair route is known;
- `production candidate`: automated gates and adversarial visual proofs pass,
  awaiting user or designated visual acceptance;
- `accepted`: manual authority explicitly accepts the bounded capability.

Automated green is not manual acceptance. Do not advance damage or the next
material capability merely because the scripts run.

## Final handoff

Report:

- exact capability and boundary;
- key research and measurement authorities;
- final map resolution and physical spans;
- material and node-group names;
- generated `.blend`, maps, manifest, and best proof images;
- all focused test counts and reopen result;
- whether Unreal or another target-engine parity is verified;
- excluded damage and overlays;
- honest remaining risks;
- next capability.

If the package was already untracked, say that Git cannot establish a reliable
prior LOC delta. Never invent a production delta.
