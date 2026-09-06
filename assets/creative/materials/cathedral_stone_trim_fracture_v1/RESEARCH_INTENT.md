# Measured Chevron Voussoir Portal V1 — Material Intent

## Exact boundary

This capability represents one intact row of individual Romanesque
roll/hollow/roll chevron voussoirs on a semicircular portal order at a
1.455 m clear opening and close-to-gameplay viewing range, before jamb
reconstruction, fracture, damage, weathering, dirt, paint, soot, limewash, or
target-engine parity.

The active research ledger is
[`references/chevron_voussoir_research_v1.md`](references/chevron_voussoir_research_v1.md).
The older dogtooth/leaf/fracture atlas is a rejected prototype. Its maps,
motifs, proofs, and dormant shader lanes are baseline evidence only.

## Source-to-intent ledger

### Individual wedge construction

- Observation: Old Sarum catalogue 55 records a chevron voussoir 200 mm
  radially, 180–140 mm across its tapered ends, and 270 mm deep.
- Intent: the arch must read as masonry under clay, not as a torus segment
  carrying a decorative bitmap.
- Implementation: sixteen independent, closed radial stone volumes with a
  3 mm technical joint gap and explicit stone IDs.
- Not justified: claiming the count, radius, or joint width was surveyed at
  Old Sarum.

### Moulding anatomy

- Observation: the Old Sarum group is described as roll, hollow, roll.
- Intent: two convex crests and the intervening recession must change the
  surface normal under neutral light before color is applied.
- Implementation: an explicit front-face relief grid owns two roll crests, a
  hollow, and two quiet margins inside the measured 200 mm stone height.
- Not justified: presenting authored profile widths or depths as measured.

### Pattern phase

- Observation: the CRSBI guide says lateral face chevron normally uses one
  chevron per voussoir; direction may be centripetal or centrifugal.
- Intent: each stone must feel individually carved from a transferred
  template. The motif may not drift through a joint or be cut arbitrarily at
  springing.
- Implementation: one centripetal lateral-face chevron unit per stone.
  Chevron coordinates are stone-local and phase zero is the stone center.
- Not justified: asserting Old Sarum's lost complete order was centripetal or
  that every Romanesque chevron uses one unit per stone.

### Face, soffit, and edge

- Observation: chevron position on face, soffit, or edge is architecturally
  meaningful; Gloucester stones preserve different face/soffit relationships.
- Intent: the current capability must say which surface is carved.
- Implementation: only the front face receives chevron relief. Inner soffit,
  outer face, back, and radial joint sides remain quiet intact stone and have
  separate identities.
- Not justified: a face-to-soffit continuation, edge roll, undercut serration,
  bead, lozenge, or jamb continuation.

### Hand-worked stone body

- Observation: the accepted `cathedral_stone_v1` core already owns the named
  Santa Marina/Naranjo material family, twenty-step warm/pale/cool movement,
  proxy-bounded 64 mm body relief, an incommensurate 91 mm decorrelation
  sample, and finite tooling without invented depth.
- Intent: carved stone remains the same material body as the intact
  architecture. It gains construction and profile, not a generic cloudy
  "ornament stone" texture.
- Implementation: the trim shader consumes the accepted core's body maps and
  coordinate transforms. Each stone receives deterministic phase,
  quarter-turn, and mirror variation.
- Not justified: transferring the core's Córdoba block dimensions to the
  voussoirs or relabeling its relief proxy as a scan of Old Sarum stone.

### Color layering

- Observation: professional trim assets and BG3 architecture keep several
  related values inside one element, preserve quiet bands, and separate broad
  warm/cool grouping from local profile response.
- Intent: one stone must contain broad pale, warm, and cool passages plus
  restrained anatomy-selected ink and highlight. It may not receive one flat
  color.
- Implementation: cumulative color order is base stone family, per-stone
  warm/cool family, broad body-field movement, roll/hollow modulation,
  finite tooling color, selective structure ink, then selective highlight.
- Not justified: sampled photographed light, AO baked into albedo, universal
  black outlines, saturated orange, or arbitrary grunge.

### Live shader behavior

- Observation: the prototype packs normal, identity, stylization, and
  painterly images but does not consume them in the live material.
- Intent: every retained map and attribute must have a named consumer and an
  isolated proof.
- Implementation: the body normal enters a real tangent-space Normal Map
  node; ORM roughness enters Principled Roughness; roll/hollow and linework
  attributes alter only their documented color path; camera distance fades
  microdetail and optical linework.
- Not justified: counting a node, image, or group socket as implemented when
  it does not reach Material Output.

## Physical construction

### Survey-bounded champion

- source stone: Old Sarum catalogue 55, accession group 1972.21, stone 15;
- radial height: `0.200 m`;
- inner chord: `0.140 m`;
- catalogued outer chord: `0.180 m`;
- wall depth: `0.270 m`;
- construction: independent wedge-shaped voussoir;
- measured moulding sequence: `roll / hollow / roll`.

### Authored closure

- count: `16`;
- centreline pitch: `11.25 degrees`;
- joint gap: `0.003 m`, transferred as technical guidance from the accepted
  fine-jointed ashlar contract;
- body angle: approximately `11.042294 degrees`;
- inner radius: approximately `0.727551 m`;
- outer radius: approximately `0.927551 m`;
- calculated outer chord: approximately `0.178485 m`;
- clear opening diameter: approximately `1.455103 m`;
- outer diameter: approximately `1.855103 m`.

The complete radius and count are authored to close a reviewable semicircle.
They are not surveyed Old Sarum values.

### Authored replaceable profile

The 200 mm radial face is divided into:

1. 25 mm inner quiet margin;
2. 50 mm first-roll zone;
3. 50 mm hollow zone;
4. 50 mm second-roll zone;
5. 25 mm outer quiet margin.

Roll crest relief is authored at 12 mm and hollow depression at 6 mm relative
to the front-face zero plane. These depths exist so neutral light can test the
profile. They carry `authored_translation` status and may be replaced when a
measured section becomes available.

No arris bevel is authored. The measured specimen has no published arris
radius.

## Frequency and layer anatomy

### Construction and silhouette

- sixteen closed stone volumes;
- real radial joints;
- 200 mm order height;
- 270 mm wall depth;
- two convex chevron rolls and one hollow in actual front geometry;
- plain front-face margins, soffit, outer face, back, and joint sides.

Proof: neutral clay front, three-quarter, and grazing views plus object and
manifold assertions.

### Macro

- complete semicircular order;
- broad per-stone pale, warm, and cool family assignment;
- quiet-versus-carved value grouping;
- no wall-scale cloudy landmark.

Proof: unlit base color and gameplay-distance view.

### Medium

- one stone-local chevron unit;
- roll/hollow/roll profile;
- multiple related values inside each stone;
- deterministic body phase, quarter-turn, and mirror;
- optional finite tool-direction color/roughness contribution inherited from
  the core without geometric groove depth.

Proof: close neutral, close material, and isolated roll/hollow identity.

### Edge and event

- radial construction joints;
- selected narrow structure ink adjacent to the hollow and a subset of joint
  sides;
- selected roll-crest highlight;
- untouched quiet margins and quiet stones.

No chip, arris loss, crack, pit, scrape, stain, or damage event is active.

Proof: linework-only material and quiet-surface occupancy metrics.

### Micro

- accepted 64 mm calcarenite body field;
- incommensurate 91 mm decorrelation sample;
- sparse fossil, silicate, and visible-pore identities;
- proxy-bounded 1.13 mm body-height range;
- complete fade from close detail to zero by 3.5 m, matching the accepted
  core contract.

Proof: close material and gameplay/distance pair.

### Stylization

- structure-selected cool-brown ink, not a full outline;
- sparse warm-pale roll highlight;
- broad warm/cool washes preserved underneath;
- linework and highlight fade before they form distance noise;
- no Shader-to-RGB dependence and no baked directional light.

Proof: isolated ink, isolated highlight, neutral material, and changed-light
views.

## Coordinate and semantic contract

### Frame

- origin: arch center at `X=0`, springing height on Z, wall center on `Y=0`;
- X: horizontal opening direction;
- Y: wall depth, negative toward the acceptance camera;
- Z: vertical, with radial height above the springing line;
- each voussoir owns stone-local `u` across its angular body,
  `v` from inner to outer radius, and `w` through depth.

### Transform behavior

- dimensions are authored in object-local metres;
- object transforms may place or rotate the complete portal but may not
  change its apparent material scale;
- non-uniform object scale is rejected by validation until applied;
- stone-local UV and attributes are written before save and do not depend on a
  post-assembly bounding box;
- there is no curve deformation or Geometry Nodes route in this capability.

### Required identities

- `iggy_voussoir_id`: integer point/face provenance for values 0–15;
- `iggy_material_phase`: deterministic float per stone;
- `iggy_material_variant`: deterministic integer encoded as float;
- `iggy_carved_trim`: 1 on all champion-stone faces;
- `iggy_chevron_front`: 1 only on the sampled carved front grid;
- `iggy_chevron_roll`: continuous 0–1 roll priority;
- `iggy_chevron_hollow`: continuous 0–1 hollow priority;
- `iggy_chevron_quiet`: continuous 0–1 rest priority;
- `iggy_chevron_ink`: selective optical line priority;
- `iggy_chevron_highlight`: selective optical crest priority;
- `iggy_fracture_interior`: 0 everywhere;
- `iggy_damage_mask`: absent or 0 everywhere.

Attributes that drive a hard face decision use face or face-corner data.
Smooth profile weights may use points because they are intentionally
interpolated inside the already classified carved front.

### Boolean and fracture boundary

No Boolean or fracture operation is performed. If a later operation creates
faces, it must assign new face identities after topology evaluation. This
capability never infers new-face semantics from interpolated point data.

### Blender reconstruction

1. solve the authored closure from profile values;
2. generate each stone's radial front grid and closed volume directly;
3. validate dimensions, joint spacing, manifold closure, normals, and
   non-overlap;
4. write stable UVs and named attributes;
5. load accepted core texture dependencies at their declared color spaces;
6. build a versioned node group with exact named nodes and sockets;
7. connect every retained data lane to visible material behavior;
8. create neutral, diagnostic, and final proof scenes;
9. pack resources, save, reopen, and revalidate.

### Target-engine reconstruction

Document only. Unreal parity remains false. A future engine implementation
must reconstruct the same stone-local frame, physical spans, packed channels,
normal convention, distance fades, and explicit identities, then render an
independent comparison before parity is claimed.

## Color contract

### Accepted families

The base family comes from `cathedral_stone_v1`:

- pale cream and chalk;
- restrained warm buff and light brown;
- restrained cool gray and olive-gray;
- sparse iron-related warm accents where already authorized by the core.

### Cumulative order

1. sample the accepted intrinsic base field;
2. apply the stone's deterministic warm/pale/cool family bias;
3. mix the 64 mm and 91 mm body samples in the stone-local transformed frame;
4. add broad anatomy movement: rolls slightly warmer/lighter, hollows slightly
   cooler/darker, quiet fields close to the base family;
5. add finite tooling color only at close range;
6. add structure ink at low opacity through `iggy_chevron_ink`;
7. add roll highlight at low opacity through
   `iggy_chevron_highlight`;
8. apply no contextual overlay.

One element must traverse several related palette steps. Shade count is a
means of continuous movement, not an acceptance target by itself.

### Prohibited color

- photographed sunlight, cast shadow, or specular highlight;
- AO multiplied into base color;
- one flat value per stone;
- saturated orange;
- blue-black universal cavities;
- generic dirt, soot, lichen, damp, blood, moss, or repair;
- a color ramp driven only by geometric height.

## PBR contract

### Height

- macro 200 mm wedge and 270 mm depth: geometry, surveyed;
- medium 12 mm rolls and 6 mm hollow: geometry, authored translation;
- micro body relief: accepted 1.13 mm comparable-calcarenite proxy range;
- tool groove depth: zero, unknown;
- arris rounding: zero, unknown;
- damage depth: zero, excluded.

### Normal

- geometry normals carry wedge and roll/hollow silhouette;
- the accepted OpenGL tangent-space body normal contributes micro response;
- the Image Texture is `Non-Color`;
- it enters a `ShaderNodeNormalMap` using the same UV as the body maps;
- no RGB averaging of normals;
- no normal contribution from ink, highlight, joint color, or tooling whose
  depth is unknown.

### Roughness

- begin with the accepted intact-stone authored calibration;
- allow low-amplitude broad family and body variation;
- retain hollow-versus-roll response only when it represents finish or
  porosity, not inverted height;
- fade high-frequency roughness with the microdetail distance control;
- no wetness, polish, dust, or damage response.

### Ambient occlusion

- texture AO remains 1 for the intact stone body;
- real rendered occlusion may arise from the geometric hollow and real joints;
- AO never draws ink and never enters base color.

### Metalness and opacity

- metalness: `0`;
- opacity: `1`;
- transmission, coat, sheen, emission, and subsurface: default neutral.

## Finite authored vocabulary

- stone shapes: one source-bounded wedge envelope instantiated sixteen times;
- pattern shapes: one centripetal V unit;
- profile shapes: two roll crests, one hollow, two quiet margins;
- allowed transforms for the body field: deterministic phase offset,
  quarter-turn, and mirror;
- forbidden transforms: scaling the chevron independently of its stone,
  continuous pattern phase across joints, arbitrary motif rotation, and
  global world-aligned noise;
- family count: three broad color tendencies distributed deterministically;
- quiet variants: at least four stones receive reduced ink/highlight strength;
- champion seed: `41723`;
- no quarter-motif mirroring is needed because the V is represented by an
  analytic symmetric distance field whose two halves share one definition.

## Acceptance

### First red observable

The old asset is one smooth annular ribbon with a repeated diamond texture.
The candidate must instead be inspectable as sixteen measured-envelope,
individual radial stones with real joints and one phase-locked roll/hollow/roll
chevron per stone.

### Focused gates

- profile gate: source IDs, dimensions, authored-status labels, closure
  solution, exclusions, texture inventory, and shader contract;
- generator gate: deterministic maps, dimensions, bit depth, color spaces,
  physical spans, hashes, and no stale palette dependency;
- Blender construction gate: object count, closed volumes, measured
  dimensions, joint gap, no global bevel, attributes, and material assignment;
- node gate: exact node names/types/sockets/defaults/links and live map
  consumption;
- proof gate: required views and manifests;
- reopen gate: packed resources, object/attribute persistence, node graph,
  dimensions, exclusions, and hashes after loading the saved `.blend`.

### Required views

- neutral clay front;
- neutral clay three-quarter;
- neutral clay grazing;
- unlit base color;
- live material close;
- live material grazing;
- live material gameplay distance;
- live material far distance;
- roll/hollow/quiet diagnostic;
- ink/highlight diagnostic;
- actual portal assembly;
- repeated adjacent portal pair to expose identical landmarking.

### Rejection conditions

- fewer or more than sixteen champion voussoirs;
- one connected annular ribbon;
- UV-painted joints instead of physical gaps;
- motif phase crossing a joint;
- chevron visible only through base color or normal texture;
- any unlinked retained map or selector;
- one flat color per stone;
- universal outlines or all-stones-equal ink;
- normal map not using `Non-Color` and a tangent Normal Map node;
- high-frequency body response competing at gameplay distance;
- nonzero fracture, chip, crack, soot, limewash, lichen, damp, dirt, or repair;
- target-engine parity claimed without an engine render;
- saved-file tests passing only in memory.

### Review status

The implementation may be called a production candidate after all focused
gates pass and the proof set survives adversarial visual review. It remains
unaccepted until the user reviews the strongest and weakest proof views.
