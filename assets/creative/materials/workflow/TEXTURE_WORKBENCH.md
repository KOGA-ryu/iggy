# Texture Surface Workbench

This is the at-hand bench for scripted texture, material, shader, mask, and
surface work. A material goal should not begin by rediscovering its available
tools or by treating a named appearance as one indivisible texture.

The bench has four jobs:

1. define the project’s material and surface families in causal language;
2. distinguish substrates, construction systems, finishes, conditions, and
   stylization;
3. inventory reusable scripted art tools and package-specific donors;
4. keep at least two viable construction strategies available for every major
   visual problem before production code is frozen.

This document is a planning registry, not evidence that every listed family is
researched, implemented, or accepted. A material card becomes production
authority only after its selected references, measurements, consumer,
component blueprint, code, and proofs are recorded in the owning package.

## 1. Surface ontology

A user-facing name such as `iron_rusted`, `wood_painted_red`, or
`stone_mossy` is usually a composition:

```text
substrate
  + construction or manufacturing state
  + finish or coating
  + optional condition or deposit
  + independently controlled stylization
  + lighting and post-process outside the texture
```

### 1.1 Substrate

The physical body exposed when every coating and deposit is removed:

- ferrous conductor;
- copper alloy;
- timber;
- limestone or other rock;
- fired clay;
- lime or gypsum binder;
- plant or animal fibre;
- hide;
- glass;
- ceramic body;
- wax;
- water;
- soil, snow, or biological tissue.

The substrate owns its intrinsic optical identity, pores, inclusions, growth,
grain, microfacets, or other material-specific anatomy. It does not own later
rust, paint, moss, soot, polish, dirt, or damage.

### 1.2 Construction or manufacturing state

How the substrate was made into the visible surface:

- sawn, hewn, planed, split, turned, or end-cut wood;
- drawn, forged, rolled, ground, brushed, cast, or polished metal;
- quarried, split, dressed, tooled, carved, or fractured stone;
- moulded, fired, rubbed, glazed, or broken ceramic and brick;
- spun yarn, twisted rope, woven cloth, felt, or leather;
- scratch, brown, and finish plaster coats;
- bond, joint, mortar, trim, and modular assembly.

Construction is not generic variation. It establishes the local frame,
feature sizes, eligible regions, and the order in which later layers exist.

### 1.3 Finish or coating

A deliberately applied or process-created surface layer:

- oxide or forge scale;
- bluing, blackening, patina, oil, wax, lacquer, varnish, or polish;
- paint or primer;
- ceramic glaze;
- limewash or pigment wash;
- protective seal;
- thin film or residue.

A finish may have different metalness, roughness, normal, thickness, colour,
and failure behavior from the substrate. It must be mixable or removable
without rewriting the substrate.

### 1.4 Condition, deposit, and damage

History applied after the intact construction:

- atmospheric corrosion, tarnish, rust, or verdigris;
- abrasion, contact polish, scratches, dents, chips, cracks, checks, fray,
  edge loss, and fracture;
- dust, dirt, soot, grease, oil, blood, wax drips, damp, lime bloom, lichen,
  moss, mould, or salt;
- repair, replacement, patch, repointing, or recoating.

Condition requires a cause, eligible region, protected region, chronology,
and separate switch. It is never a universal quality pass.

### 1.5 Stylization

Art-direction outputs that remain separable from the physical surface:

- structural ink;
- highlight strokes;
- brush or fibre direction;
- broad pigment grouping;
- value-band priority;
- protected quiet regions;
- detail priority for distance and mips;
- traversal or gameplay emphasis.

Lighting ramps, screen-space outlines, shadows, bloom, and post-process remain
renderer responsibilities.

## 2. Required material-card contract

Before a family enters production, its package must answer:

1. **Name and taxonomy:** substrate, construction, finish, condition, and
   stylization components.
2. **Named consumer:** actual object, surface, dimensions, orientation, camera,
   and gameplay distance.
3. **Material history:** source material, manufacturing sequence, finish, age,
   maintenance, exposure, and exclusions.
4. **Reference coverage:** authority, view coverage, construction evidence,
   feature-size evidence, professional benchmark, and unresolved ambiguity.
5. **Surface anatomy:** macro, medium, event, micro, response, and rest.
6. **Local frames:** metre coordinates, element identity, direction, phase,
   deformation behavior, and Boolean or cut-face behavior.
7. **Output ownership:** geometry, attribute, colour, height, normal,
   roughness, AO, metalness, opacity, coating, decal, stylization, lighting,
   or post-process.
8. **Layer order:** exact composition operation and absence rule for every
   component.
9. **Strategy set:** at least two plausible implementations for every
   unresolved major component, plus selection criteria.
10. **Tool selection:** existing reusable primitives, package donors, missing
    tools, and tools explicitly rejected.
11. **Proof:** isolated mask, cumulative result, neutral, grazing, moving or
    changed light, gameplay distance, repetition, real consumer, and rejection
    conditions.

## 3. Project material census

Status meanings:

- **donor exists:** useful code or evidence exists, not necessarily accepted;
- **partial family:** several components exist but the causal family is not
  complete;
- **unbuilt:** no owned production family;
- **overlay gated:** preserve the need, but do not fold it into an intact
  master.

### 3.1 Structural organic materials

| Family | Normalized composition | Primary consumers | Current state |
| --- | --- | --- | --- |
| Structural oak side grain | oak growth volume + conversion face + sawn/hewn/planed finish | beams, posts, braces, shelves, tables, bed and furniture stock | donor exists in `structural_oak_timber_v1` |
| Oak end grain | same growth volume sampled across rings + pore/ray exposure + saw state | beam ends, tenons, cut boards, broken stock | donor exists in `structural_oak_joinery_v1` |
| Oak door boards | oak side grain + board conversion + planing/sawing + assembly phase | giant-house door leaves | partial family in `structural_oak_door_v1` |
| Wood joinery faces | side grain + end/cross grain + housing, mortise, tenon, peg, wedge and cut semantics | doors, stairs, furniture, traps, building kits | partial family in `structural_oak_joinery_v1` |
| Wide plank floor | timber + board layout + edge joint + finish + traffic history | houses, halls, platforms | older donor in `wood_plank_v2`; construction family unresolved |
| Softwood/pine | conifer ring and resin anatomy + saw/plane state | cheap furniture, crates, roof and utility stock | unbuilt |
| Walnut/dark hardwood | diffuse-porous or semi-ring anatomy + dark heartwood + finish | fine furniture, chests, interiors | unbuilt |
| Driftwood/weathered wood | wood substrate + salt/water exposure + fibre erosion + bleaching | shore, river, ruins | overlay-gated composition |
| Painted wood | wood + primer/paint + brush history + adhesion failure | doors, signs, furniture and factions | unbuilt coating family |
| Lacquered/varnished wood | wood + clear or tinted film + polishing and film roughness | fine furniture, handles and reliquaries | unbuilt coating family |
| Bark | species-specific periderm plates/fibres + trunk growth + weather | trees, logs, tree forts | unbuilt; layered-graph reference only |
| Thatch/reed | stems + bundles + overlap + tie/compression + weather | roofs, mats, rural props | unbuilt |

### 3.2 Fibre, cloth, hide, and paper

| Family | Normalized composition | Primary consumers | Current state |
| --- | --- | --- | --- |
| Plant-fibre rope | fibre ribbons + counter-twisted yarn + strands + rope lay | bridges, hoists, traps, lashings | strong donor in `rope_plant_fibre_v1` |
| Fine cord/twine | same hierarchy with different strand/yarn count and fuzz scale | ties, snares, small props | parameter-family candidate |
| Wool cloth | wool fibres + spun yarn + weave/knit + nap | blankets, clothing props, banners | unbuilt |
| Linen canvas | flax fibre + yarn + plain weave + finish | sacks, sails, tents, upholstery | unbuilt |
| Burlap sack | coarse bast yarn + open plain weave + compression and lint | storage, market, clutter | unbuilt |
| Velvet | woven ground + directional pile + crushed/contact states | noble interiors, reliquaries | unbuilt |
| Tapestry/heraldic cloth | woven ground + coloured yarn design + edge construction | walls, banners, furniture | unbuilt |
| Leather | collagen hide + grain/corium face + tanning + oil/wax finish | straps, books, bags, armour props | unbuilt |
| Parchment | prepared skin + fibre direction + scraping + translucency | books, scrolls, notebook | unbuilt |
| Fur | skin support + guard hairs + underfur + direction and clumping | rugs, costume props | unbuilt; geometry/groom dependency |

### 3.3 Ferrous and non-ferrous metals

| Family | Normalized composition | Primary consumers | Current state |
| --- | --- | --- | --- |
| Forged ferrous conductor | iron/steel conductor + microfacet finish + forging/planishing direction | hinges, straps, hasps, cages, tools | current calibration target; substrate response incomplete |
| Intact forge or mill scale | ferrous substrate + continuous oxide coverage + thickness/composition + optional compression | forged door hardware and structural iron | continuous coverage/thickness donor accepted; mixer integration remains gated |
| Bright worked steel | steel conductor + ground/polished or brushed finish | blades, tools, mechanisms | old flat-albedo experiment is not a current donor |
| Cast iron | ferrous body + casting skin + mould/graphite/porosity evidence | hearths, cauldrons, weights and machines | unbuilt |
| Blued/blackened iron | steel + conversion coating + oil/wax response | weapons, fittings and maintained hardware | unbuilt finish family |
| Rusted iron | ferrous substrate + remaining finish + corrosion products + pitting/spall chronology | neglected props and ruins | condition family; not an intact master |
| Gold/brass | copper-alloy conductor + polish/hammer/cast finish + optional tarnish | loot, fixtures, ornament | unbuilt |
| Copper | copper conductor + worked finish + oxide/patina states | cookware, roofing, vessels | unbuilt |
| Bronze | copper-tin conductor + casting/hammer finish + patina | statues, fittings, weapons | unbuilt |
| Silver/tarnished silver | silver conductor + polish + sulphide tarnish | loot, tableware and reliquaries | unbuilt |

### 3.4 Stone, masonry, clay, and plaster

| Family | Normalized composition | Primary consumers | Current state |
| --- | --- | --- | --- |
| Dressed calcarenite ashlar | measured blocks + micritic matrix + sand/silicate + fossil + pore + tooling | cathedral/castle walls | substantial donor in `cathedral_stone_v1` |
| Cathedral carved trim | stone body + measured profile + block/voussoir construction + carving finish | arches, jambs, mouldings | partial candidate in `cathedral_stone_trim_fracture_v1` |
| Rubble limestone masonry | placed stones + face families + lime mortar + construction transition | giant-house walls | donor in `lime_plaster_masonry_v1` |
| Generic fieldstone | explicit fitted layout + stone families + mortar | walls, foundations and paths | older prototype in `stone_rough_v2` |
| Fired-clay brick | selected brick family + moulding/firing warp + fired skin + inclusions | towns, hearths, industrial and domestic walls | face-study donor in `brick_masonry_v1` |
| Lime mortar | lime/sand binder + aggregate + joint tooling + cure | brick, rubble and stone joints | partial donors in brick/plaster packages |
| Lime plaster | masonry key + scratch coat + brown coat + finish coat + trowel history | giant-house and domestic walls | strong construction donor in `lime_plaster_masonry_v1` |
| Sandstone | grain-supported stone + bedding + cement + dressing/weather | ruins, desert and carved work | unbuilt |
| Granite | interlocking mineral crystals + cut/thermal/tooled finish | civic, mountain and monument work | unbuilt |
| Slate | foliated cleavage + split planes + edge thickness + wet response | roofs, floors and ledges | unbuilt |
| Flagstone | quarried slabs + fitted layout + bedding/mortar + traffic finish | floors, yards, paths | unbuilt |
| Cobble | rounded/split units + packed bedding + mortar/soil infill | streets, drains and yards | unbuilt |
| Marble | crystalline carbonate + veins + cut/polish state | elite interiors, sculpture and altars | unbuilt |
| Cave rock | lithology + fracture system + mineral deposition + damp | caves and underground spaces | unbuilt |
| Ceramic body and glaze | fired clay body + throwing/moulding + glaze layer + kiln response | vessels, tiles and props | unbuilt |

### 3.5 Ground, biological, transparent, liquid, and emissive families

| Family | Normalized composition | Primary consumers | Current state |
| --- | --- | --- | --- |
| Dirt path | mineral soil fractions + compaction + traffic + moisture | paths, yards and floors | unbuilt |
| Sand | grain population + ripple/packing state + moisture | shores, deserts and fill | unbuilt |
| Gravel | graded clasts + fines + packing and traffic | paths, drains and yards | unbuilt |
| Forest floor | soil + litter generations + twigs + damp/decay | woodland and tree forts | unbuilt |
| Grass/leaf foliage | plant tissue + venation + cuticle + species structure | terrain and vegetation | unbuilt; alpha/geometry contract gated |
| Snow/ice | crystal or compacted body + melt/refreeze + contamination | winter surfaces | unbuilt |
| Clear/stained glass | dielectric body + thickness + roughness + colour/inclusions + lead came where relevant | windows, vessels and props | transparency route gated |
| Water | volume/surface dielectric + normals + absorption + foam/flow | rivers, marsh, wells | renderer/VFX family gated |
| Wax | translucent dielectric + crystalline body + mould/drip history | candles, seals and polish | unbuilt |
| Flame/embers/runes | emissive source shape + temperature/colour + opacity/VFX | lighting and magic | renderer/VFX family gated |
| Skin and hair | biological tissue or fibres + anatomy + grooming | characters and mannequins | character pipeline gated |

### 3.6 Reusable condition and storytelling overlays

These are library components, not replacement materials:

- oxide loss and exposed conductor;
- contact polish;
- atmospheric rust and verdigris;
- soot source and plume;
- dust deposition and removal;
- damp, capillary rise, drainage edge, and wetness;
- moss, lichen, mould, and biological colonization;
- wax drip, grease, oil, fingerprints, and hand contact;
- chipped paint, lacquer wear, glaze crazing, and coating delamination;
- wood checks, splits, crushed fibres, scuffs, dings, scratches, and repairs;
- stone fracture, spall, arris loss, pocks, impact, repointing, and inserts;
- rope compression, fray, strand break, fuzz, and wetting;
- cloth staining, abrasion, pilling, tears, patches, and seams.

## 4. Detailed surface-family cards

### 4.1 Structural wood family

#### Visual description

One timber contains many related shades, not one colour assigned per board.
Its visible figure is the intersection of a three-dimensional growth volume
with a conversion face. Earlywood, latewood, vessels, rays, heartwood,
sapwood, knots, reaction growth, conversion angle, saw direction, tool finish,
oxidation, and coating affect different lanes. Grain must bend around branch
intersections and remain quiet where the growth volume produces calm straight
stock.

#### Component plan

1. shared virtual-log growth coordinates;
2. variable earlywood/latewood ring table;
3. branch intersections and knot-driven field deflection;
4. vessel/pore and ray populations by species;
5. side, radial, tangential, and end-face samplers;
6. conversion and manufacturing finish;
7. several related pigment contributions inside one element;
8. independent height, normal, roughness, and finish response;
9. joinery/end/cut semantic masks;
10. optional coating, damage, and condition.

#### Strategies

- **Analytic growth volume:** evaluate rings, knots, rays, and pores in
  three-dimensional local coordinates, then sample any face. Best for
  arbitrary beams, cuts, and Boolean-created faces.
- **Finite authored grain tracks:** draw a limited vocabulary of ring sweeps,
  knot fields, ray flecks, and pore bands with bounded transforms. Best for
  hero doors and painterly control.
- **Measured/reference field proxy:** derive palette, ring-density, pore-size,
  and direction statistics from a clean reference crop without copying its
  photographed lighting. Best for species calibration.

The champion may combine all three, but each owns a distinct decision.

### 4.2 Forged ferrous family

#### Visual description

Clean ferrous metal is optically uniform compared with its oxide and
contamination. Its identity is carried by conductor reflection, microfacet
roughness, and manufacturing direction. Forging and planishing create broad
orientation changes and directional response. High-temperature oxide is a
continuous material layer with variable thickness, composition, compression,
fracture, and removal—not a set of decorative islands.

#### Component plan

1. engine-calibrated ferrous conductor;
2. baseline statistical microfacet finish;
3. local tangent and anisotropic worked response;
4. broad forging/planishing planes;
5. continuous oxide coverage;
6. oxide thickness/composition response;
7. oxide microstructure and compression;
8. separately authorized fracture/spall;
9. geometry-supplied face, bevel, aperture, barrel, pin, and contact identity;
10. optional blackening, oil, rust, soot, and polish.

#### Strategies

- **Layered BSDF:** mix a conductor shader and oxide shader with continuous
  coverage. Best physical explanation and strongest Blender proof.
- **Metal/roughness material blend:** precompute colour, roughness, metalness,
  normal, and coverage for direct Unreal-compatible packing. Best common
  denominator.
- **Hybrid response:** use standard packed maps for engine parity while
  retaining a Blender-only analytic anisotropic proof group. Best when the
  engine route lacks the complete optical model.

Finite oxide stamps, polar harmonic plates, and repeated compact motifs are
prohibited by the hinge calibration.

### 4.3 Stone and masonry family

#### Visual description

Stone identity begins with lithology and construction, not a cell diagram.
Each block or slab is a volume with bedding, minerals, pores, fossils or
crystals, face planes, tooling, arrises, returns, and joints. Mortar is a
second material negotiating the units. Large planes and rests precede edge
events and microstructure.

#### Component plan

1. selected geological family and measured construction population;
2. explicit units, bond, depth, joints, corners, returns, and openings;
3. per-unit local frames and phase;
4. matrix, grain/inclusion, pore, vein/fossil, and colour populations;
5. face plane, tooling, and dressed/split finish;
6. mortar body, profile, aggregate, tooling, and cure;
7. trim or carved profile as geometry;
8. optional fracture, repair, deposits, damp, soot, and lichen.

#### Strategies

- **Modular geometry vocabulary:** individual measured units with local
  materials. Best where joints, silhouettes, returns, traversal, or breaks
  matter.
- **Authored layout atlas:** finite units rasterized into height, identity,
  normal, and colour. Best for broad flat coverage and controlled tiling.
- **Trim-sheet or signed-profile system:** swept or sampled profiles with
  phase-locked material coordinates. Best for arches, mouldings, bands, and
  modular edges.

### 4.4 Fired clay, ceramic, mortar, and plaster

#### Visual description

Fired clay contains forming, drying, inclusion, firing, skin, and core
histories. Lime mortar and plaster contain binder, aggregate, coat order,
tooling, cure, and exposure. A chipped plaster wall is a construction-state
transition, not one random black-and-white mask.

#### Component plan

1. material recipe and dimensional family;
2. moulding, forming, drying, firing, or coat sequence;
3. unit/body identity and local frame;
4. aggregate/inclusion populations;
5. surface skin versus interior/cut body;
6. joint or coat transition;
7. trowel, brush, rub, glaze, or kiln response;
8. optional cracks, chips, efflorescence, damp, soot, and repair.

#### Strategies

- **Construction-state geometry attributes:** faces explicitly identify stone,
  mortar, scratch, brown, finish, glaze, or exposed body. Best for Boolean and
  traversal safety.
- **Layered material mask:** continuous coverage and thickness blend surface
  coats over a substrate. Best for intact coat variation and gradual loss.
- **Finite trowel/forming pass vocabulary:** authored signed passes compile
  pressure, direction, colour, height, and roughness. Best for visible hand.

### 4.5 Rope, cloth, hide, and fibrous materials

#### Visual description

Fibrous surfaces are nested construction systems. Rope is fibre into yarn,
yarn into strand, and strand into rope with opposing twist. Cloth is fibre into
yarn and yarn into weave, knit, felt, or pile. Leather and parchment preserve
different layers and directions of skin. Fuzz is a close-distance silhouette
option, not universal alpha noise.

#### Component plan

1. fibre material and cross-section;
2. yarn count, twist, irregularity, and colour;
3. strand, weave, knit, pile, or hide construction;
4. compression/contact and seam semantics;
5. body colour and dye;
6. fibre normal, sheen, roughness, and optional geometry;
7. distance hierarchy;
8. optional fray, pilling, tears, stains, wetness, and repair.

#### Strategies

- **Curve/Geometry Nodes construction:** real strands, yarn crowns, piles, or
  hero fibres. Best for silhouette and deformation.
- **Analytic periodic coordinate material:** accumulated length and
  circumference/weave phase drive nested relief. Best for scalable continuous
  surfaces.
- **Finite track atlas:** authored bundle and fibre paths with merges, burial,
  fades, and quiet zones. Best for art-directed irregularity without full
  strand geometry.

### 4.6 Ground, foliage, water, glass, wax, and emissive families

These require renderer and geometry decisions before their textures:

- ground mixes graded particles, compaction, litter, moisture, and traffic;
- foliage needs species geometry, venation, cuticle, translucency, and alpha
  policy;
- water needs reflection/refraction, absorption, flow coordinates, normals,
  depth, shoreline, and foam ownership;
- glass needs thickness, IOR, roughness, colour, inclusions, lead or frame
  construction, and transmission;
- wax needs crystalline body, subsurface/transmission approximation, moulding,
  wick, melt, drip, soot, and polish;
- emissive surfaces need source geometry, colour temperature, emitted response,
  bloom/post ownership, and optional VFX.

For these families, a flat albedo tile is not a meaningful master. Their
material cards remain gated until the consuming renderer path is named.

## 5. Scripted art-tool bench

Tool status:

- **shared:** reusable without importing package-specific material meaning;
- **donor:** useful implementation exists but needs extraction or parameter
  review;
- **missing:** build once before the dependent champion;
- **rejected:** demonstrated failure; do not reuse.

### 5.1 Evidence, measurement, and palette tools

| Tool | Status | Current owner | Use |
| --- | --- | --- | --- |
| Source and measurement ledger | shared | `reference_measurements_v1.json`, package research ledgers | distinguish measured, laboratory, guidance, proxy, authored, unknown |
| Image-to-text observation | shared method | research workflow | describe construction, colour families, scale and ambiguity without copying a photograph |
| Reference palette capture | donor | `cathedral_stone_v1/capture_cathedral_stone_reference_v1.py` | classify cleaned colour families and render palette proofs |
| Inkblotter reference-style recipe | external donor | `~/font` workflow | palette and broad style evidence; never anatomy or photographed-light transfer |
| Shade-family builder | donor | brick, wood, and stone generators | create many related shades within one physical element |

Missing:

- a single source-scoring worksheet for authority, coverage, construction,
  frequency, benchmark relevance, and ambiguity;
- a conductor-value adapter that records the material model behind each
  published metal value.

### 5.2 Coordinates, identity, and semantic masks

| Tool | Status | Current owner | Use |
| --- | --- | --- | --- |
| Metre-based periodic coordinates | shared | `pattern_lab_common.py` and package generators | scale-stable tile and feature sampling |
| Element ID, seed, phase, variant | donor | wood, brick, stone, rope packages | break global continuity and preserve unit identity |
| Virtual growth coordinates | donor | `structural_oak_timber_v1` | consistent side/end/cut wood sampling |
| Curve-local rope coordinates | donor | `rope_plant_fibre_v1` | accumulated length, circumference and diameter |
| Face/point semantic attributes | donor | plaster, joinery, cathedral and iron builders | construction identity, cut faces, traversal, aperture, contact |
| Boolean face classification | donor | `structural_oak_joinery_v1` | transfer original semantics and classify new bearing/cut faces |
| Local UV in metres | donor | cathedral trim/voussoir builder | stable modular profile sampling |

Missing:

- one shared Blender attribute writer/validator;
- one shared local-frame contract for planar, cylindrical, curve, volumetric,
  and modular-element consumers;
- one reusable semantic edge/cavity/contact mask group that consumes real
  geometry identity instead of inventing curvature wear.

### 5.3 Layout, motif, and finite-mark tools

| Tool | Status | Current owner | Use |
| --- | --- | --- | --- |
| Explicit unit-layout JSON | donor | plank, brick, rubble and ashlar packages | authored construction before rasterization |
| Role-bounded variation | donor | `stone_rough_v2` and modular packages | anchor/medium/infill or champion/sibling movement envelopes |
| Finite track vocabulary | donor | rope bundle/fibre recipes | authored paths, widths, merges, splits, burial and fade |
| Finite trowel passes | donor | `lime_plaster_masonry_v1.finite_trowel_field` | signed pressure, direction, colour and response |
| Finite tooling impacts | donor | cathedral stone tooling system | measured-envelope passes per block |
| Wood branch/knot events | donor | structural oak and plank generators | deform growth rather than stamp a dark spot |
| Modular rotations/mirrors/phases | donor | stone trim, rope and masonry | reuse without complete graph duplication |
| Open-rail broad forging planes | shared | irregular authored height rails and non-periodic normal conversion | broad worked orientation without closed hammer-head stamps |
| Continuous intact oxide fields | shared | open thermal-thickness rails + optional bounded compression modes + explicit constant coverage/metalness/height lanes | dielectric coating organization without invented flakes or relief |
| Explicit surface-layer mixer | shared | two complete material responses + one explicit exposure/coverage mask | substrate/coating composition without interpolating one half-metal BSDF |
| Compact oxide motif plates | rejected | forged-iron v003 route | created diamonds and cat-face pareidolia |

Missing:

- motif silhouette analysis and repeated-landmark rejection;
- connected-film coverage/fracture grammar for oxide, paint and glaze;
- a common finite-stroke authoring schema for tool marks, fibres, ink and
  highlight strokes.

### 5.4 Scalar fields, masks, filtering, and colour

Shared primitives in `pattern_lab_common.py`:

- `smoothstep`;
- `mix`;
- `periodic_value_noise` and `periodic_value_noise_rect`;
- `periodic_fbm` and `periodic_fbm_rect`;
- `periodic_gaussian_blur`;
- `srgb_to_linear` and `linear_to_srgb`;
- metre-scaled `height_to_normal`;
- PNG writing and proof resizing;
- neutral dielectric single- and multi-light preview helpers;
- clean-conductor single- and multi-light preview helpers with RGB F0,
  angle-dependent Fresnel, controllable neutral environment, and no diffuse
  fallback.

These are brushes and filters, not material anatomy. Periodic noise may
modulate an authored construction; it may not define the construction by
itself.

Package donors add:

- polygon inclusion and distance-to-edge fields;
- path and broad-field masks;
- periodic windows and segment envelopes;
- palette interpolation, quantization and per-element sampling;
- quiet masks and detail-priority masks;
- signed pressure, height, coverage and construction-state fields;
- per-element shifted, rotated and incommensurate sampling.

Missing:

- signed-distance primitives shared across polygons, splines, strokes and
  volumes;
- connected-component, symmetry, motif-silhouette and repetition metrics;
- blue-noise or stratified event placement with semantic eligibility;
- vector-field warp and divergence/curl controls;
- colour-space-aware gradient-map and palette-ramp library;
- band-limited mip-aware synthesis helpers.

### 5.5 Height, normal, roughness, metalness, and layer composition

| Tool | Status | Use |
| --- | --- | --- |
| Metre-scaled height-to-normal | shared | derive a normal from authorized physical relief |
| Non-periodic anisotropic height-to-normal | shared | derive host-bound normals with independent X/Y metre spacing and no opposite-edge wrap |
| Anisotropic height-to-normal | donor in rope | different physical scale along and across construction |
| Signed height composition | package donors | raise, recess, union, subtract or height-blend causal layers |
| Roughness interpretation per component | partial donors | interpret structure without copying height |
| ORM packing | package donors | engine-facing AO, roughness and metalness |
| Physical material mixing | partial in forged iron | substrate/oxide or substrate/coating mixture |
| Conductor-aware analytic preview | shared | isolate clean metallic F0, Fresnel and roughness response before coatings |
| Component-owned tangent UV builder | shared | longitudinal leaves, boundary-following cut faces, circumferential barrels, and axial pins |

Missing:

- reusable reoriented-normal or equivalent tangent-normal compositor;
- standard substrate/coating/condition material-layer node group;
- declared roughness response profiles for conductor, stone, wood, fibre,
  plaster, glaze, wax and deposits;
- engine adapter proving Blender and Unreal interpret the same packed lanes.

### 5.6 Geometry and Blender construction tools

Available donors:

- explicit brick, stone, voussoir, timber, tenon, peg, fastener and hinge mesh
  builders;
- exact Boolean joinery with cut-face semantics;
- curve-native rope Geometry Nodes;
- node-group, link, image-loading and material assembly helpers;
- proof cameras, area lights, neutral materials and render routing;
- node/link inventories, manifests, hashes, packed images and reopen tests.

Missing:

- shared geometry/material fixture library for plane, corner, cylinder, sphere,
  cut volume and actual-consumer adapters;
- one canonical shader-group builder rather than repeated package utilities;
- standard neutral, grazing, moving-light and gameplay-distance rigs;
- automatic low-resolution comparison-board assembly.

### 5.7 Stylization and art-direction tools

Available donors:

- structural ink, highlight, brush direction and detail-priority outputs in
  brick, wood, rope, stone and plaster packages;
- finite tool linework and material-specific direction fields;
- shade-family and broad warm/cool palette contributors;
- quiet-field masks.

Missing:

- a shared value-band response group for physical-to-cel translation;
- a shared structural-line selector that consumes material semantics without
  becoming universal edge detection;
- an authored override format for removing or restoring generated strokes;
- distance/mip rules for ink, highlights and protected rest.

### 5.8 Candidate selection, proof, and audit tools

Available:

- package proof sheets;
- neutral/grazing/scale/tiling views;
- manifest hashes and output inventories;
- workflow freeze/open-build gate;
- package audit for provenance, routes, performance, hashes and acceptance.

Missing:

- one candidate runner taking 3–4 semantic parameter records;
- one fixed actual-consumer camera/light manifest;
- front/three-quarter, grazing, gameplay, repetition and decisive-mask board
  generation;
- visual defect ledger template;
- donor/component census updated after acceptance.

The forged-iron calibration proves why these missing tools are high priority:
the isolated repeated mask rejected three candidates in one inexpensive board
before any full-resolution integration.

## 6. Multi-strategy deck

Every major unresolved component must retain at least two strategies until the
cheap selection gate or static review chooses one.

| Problem | Strategy A | Strategy B | Strategy C | Selection rule |
| --- | --- | --- | --- | --- |
| Large silhouette/form | real geometry | displacement/height | normal-only suggestion | geometry when parallax, contact or shadow must survive |
| Modular construction | individual measured modules | authored identity atlas | trim/profile system | choose from joint, return, editability and repetition needs |
| Macro value | hand-authored broad regions | correlated low-frequency field | per-element palette family | reject any route that destroys quiet regions |
| Directional structure | analytic local coordinate | finite spline/track vocabulary | vector-field warp | choose by required continuity and art-directability |
| Unit irregularity | role-bounded authored siblings | parameterized recipe | several compatible compiled variants | never unrestricted random scale/rotation |
| Microstructure | statistical band-limited field | measured proxy scan/statistics | finite micro-mark vocabulary | must disappear at its declared distance |
| Colour | measured/reference palette | authored multi-shade family | physically computed response | no photographed light or one shade per element |
| Relief | signed physical height | tangent normal detail | shader microfacet response | do not put an optical effect into height |
| Normal composition | derive once from cumulative height | valid detail-normal composition | separate BSDF lobe without normal | never add encoded RGB normals directly |
| Roughness | component response curves | manufacturing-direction field | coating/deposit material mix | never copy height grayscale |
| Metal/oxide | layered conductor and oxide BSDF | packed metal/roughness blend | hybrid analytic proof plus packed runtime | choose by parity and performance evidence |
| Tool marks | geometry/Boolean or sculpt | finite height atlas | analytic signed passes | choose by size, silhouette and reuse |
| Cracks/fracture | geometry/topology | signed branching height | decal/overlay | cracks require origin, branching and interior ownership |
| Edge response | real bevel/face normal | semantic edge attribute | selected authored stroke | generic curvature is evidence only, not policy |
| Contact/wear | explicit contact/motion mask | authored consumer mask | later decal | never universal curvature wear |
| Coating loss | connected film fracture/removal | hand-authored mask atlas | semantic event decals | start from continuous coating, not floating chips |
| Repetition control | multiple compatible variants | incommensurate phase/span | finite non-tiled overlays | prove 3x3, 4x4 and gameplay distance |
| Stylized linework | semantic structural masks | finite authored strokes | renderer outline | material-specific lines stay asset-owned |
| Stylized value bands | renderer response curve | material priority mask | restrained pigment grouping | never bake scene lighting into colour |
| Damage | geometry event library | signed texture event library | decals/overlays | require force, exposure, traffic or repair cause |

If two strategies can be combined without duplicating ownership, state the
division explicitly. Example: geometry owns a large stone chip silhouette,
while a height atlas owns its small fracture lips and a decal owns fresh dust.

## 7. Workbench execution sequence

1. Select the normalized material card rather than a composite nickname.
2. Bind it to the actual consumer and material history.
3. Inventory substrate, construction, finish, condition and stylization.
4. Pull the relevant tools and donors from Sections 5 and 6 into
   `WORKSTREAM.md`.
5. Mark each donor as exact reuse, parameterized recipe, shared profile/motif,
   assembly graph, bespoke hero layer, missing tool, or rejected route.
6. Write at least two strategies for each unresolved major component.
7. Research only the missing evidence or missing tool; do not reopen unrelated
   cabinets.
8. Decompose the selected route into exact component-coded demands.
9. When a visual choice remains ambiguous, run one reduced-resolution batch
   including a quiet control on the actual consumer.
10. Select one champion route and discard the candidate implementations.
11. Freeze only the champion blueprint.
12. Build substrate, macro, medium, geometry/event response, microstructure,
    stylization and optional condition as separately accepted components.
13. Assemble, prove on the actual consumer, register reusable donors, and
    return new tools to this bench.

## 8. Immediate priority from the current hinge calibration

The next workbench slice is not another complete iron texture. It is the
ferrous tool cluster:

1. conductor-aware neutral preview — accepted shared diagnostic;
2. stable local tangent and anisotropy direction — accepted shared donor;
3. broad forging-plane response — accepted shared donor;
4. continuous oxide coverage and thickness — accepted shared donor;
5. substrate/oxide material mixer — accepted shared donor;
6. isolated moving-light, grazing, gameplay and repetition proofs.

Each should be tested independently on the existing hinge. Rust, pits,
scratches, edge wear, soot, grime and damage remain outside this slice.

### 8.1 Accepted capability: clean-conductor diagnostic

The first capability is now owned by
`pattern_lab_common.render_conductor_preview` and
`pattern_lab_common.render_conductor_single_light`.

Its contract is deliberately narrower than a complete metal material:

- input colour is linear RGB normal-incidence conductor reflectance, F0;
- the direct response is a GGX-style microfacet lobe with RGB Schlick
  Fresnel;
- the substrate has no diffuse fallback;
- roughness changes reflection width and peak response;
- the view direction, diagnostic environment, light direction, light colour,
  and intensity are explicit inputs;
- the older dielectric helpers retain their prior numeric result;
- oxide, blackening, oil, anisotropy, worked direction, scratches, corrosion,
  damage and engine parity are not claimed.

Focused tests prove a black-F0 conductor contributes no diffuse light, an iron
F0 preserves its channel relationship, sharp and broad roughness values
produce different peak/spread behavior, repeated evaluation is deterministic,
and the legacy dielectric preview is unchanged.

The actual-consumer proof is
`forged_iron_v1/output/conductor_preview_v1/comparison_board.png`. It uses the
same openwork hinge geometry, cameras, neutralized target lights, F0
`(0.56, 0.57, 0.58)`, and roughness `0.38` for both columns. The left column
misuses that colour as dielectric diffuse reflectance; the right treats it as
a clean conductor. The front view changes modestly. The grazing view is
decisive: the conductor produces coherent dark face reflection and bright
bevel/aperture response, while the dielectric remains chalky and evenly lit.

This accepts a reusable diagnostic brush, not the finished hinge material.
The conductor is intentionally too clean and bright for intact historical
hardware. Continuous forge scale must later cover it as its own dielectric
layer, with exposed conductor appearing only through a separately authorized
coverage or removal mask.

### 8.2 Accepted capability: component-owned tangent and anisotropy

`pattern_lab_common.component_tangent_uv` now generates metre-scaled
per-corner UV frames whose U coordinate owns the material tangent. It has
three explicit construction modes:

- `longitudinal_leaf` follows stock length on broad faces, then switches to
  the local boundary on ends, bevels, side walls, and aperture lands;
- `circumferential_barrel` unwraps each cylindrical wall polygon locally so
  its tangent follows the barrel without a 360-degree seam jump, while
  bearing/end faces use a planar frame;
- `axial_pin` follows the pin axis on its wall while its end faces use a
  planar frame.

Per-corner ownership is required. One vertex may participate in a broad face,
bevel, and cut wall whose manufacturing directions are incompatible. A single
interpolated point vector would force one face to inherit another face's
answer.

Focused synthetic-mesh tests prove that the leaf broad face follows length,
the leaf cut face retains a non-degenerate boundary tangent, a barrel polygon
crossing the angular seam takes the short unwrap, and the pin wall follows its
axis. The actual hinge proof assigns:

- both leaves to `longitudinal_leaf`;
- all five knuckles to `circumferential_barrel`;
- the pintle to `axial_pin`.

The isolated direction proof is
`forged_iron_v1/output/tangent_anisotropy_preview_v1/tangent_direction_front.png`.
The moving-strip selection proof is
`forged_iron_v1/output/tangent_anisotropy_preview_v1/close_moving_strip_board.png`.

Four clean-conductor strengths were compared under identical geometry,
roughness, F0, camera, and moving strip light:

| Candidate | Strength | Decision |
| --- | ---: | --- |
| A | `0.00` | isotropic control; no manufacturing direction |
| B | `0.18` | selected restrained default; coherent close response and quiet whole-hinge read |
| C | `0.35` | rejected as default; begins to read intentionally machined |
| D | `0.60` | rejected; elongated reflection dominates the material |

Eevee produced pixel-identical candidates for this proof, with zero mean and
maximum difference. It is therefore rejected as anisotropy evidence for this
setup. The selection uses a 32-sample Cycles diagnostic. This proves the
Blender tangent and lobe only; it does not prove Unreal parity.

The canonical forged-iron blend is intentionally unchanged. Its current oxide
branch and conductor branch share parts of the old worked-response graph.
Installing this selected tangent there before the substrate/oxide mixer is
repaired could apply conductor-style directional response to the dielectric
oxide. The tool is accepted; material-layer consumption remains gated.

### 8.3 Accepted capability: open-rail broad forging planes

The former forged-iron macro route sums closed superellipse hammer-head
footprints with alternating signed heights. That primitive remains prohibited:
softening or reducing its contrast cannot remove its ability to form stamped
diamonds, faces, spots, and repeated silhouettes.

`pattern_lab_common.broad_forging_plane_field` replaces the causal primitive
with hand-authored open rails. Each rail contains an ordered set of
longitudinal position/height knots. Piecewise-linear interpolation gives every
interval one broad slope; interpolation between neighboring cross-stock rails
forms the two-dimensional plane field. No event has a closed boundary.

`pattern_lab_common.height_to_normal_nonperiodic` converts the field using
independent X and Y metre spacing. Unlike the seamless-tile normal helper, it
does not sample the opposite host edge when evaluating a real leaf boundary.

Focused tests prove:

- identical authored rails are deterministic;
- the field stays inside the authored signed-height envelope;
- a center rail contains only the small number of slope transitions implied
  by its knots;
- a known plane produces the expected tangent normal;
- first and last host columns retain that normal instead of wrapping.

The actual-consumer calibration uses the approved `3.112 x 0.410 m` moving
hinge leaf, three irregular cross-stock rails, four to six knots per rail, and
a `760 x 100` non-periodic field. Four signed-height amplitudes were compared
with the accepted clean conductor and tangent:

| Candidate | Giant-leaf amplitude | Decision |
| --- | ---: | --- |
| A | `0 mm` | rejected; perfect flat control |
| B | `2 mm` | rejected as default; too close to flat for a dependable read |
| C | `6 mm` | selected giant-leaf calibration; broad close response and quiet whole-hinge read |
| D | `18 mm` | rejected; rail transitions become bands and the plate reads warped |

The isolated selected normal is
`forged_iron_v1/output/broad_forging_plane_preview_v1/candidate_c_normal.png`.
It contains open low-angle plane changes, not hammer stamps. The moving-strip
selection proof is
`forged_iron_v1/output/broad_forging_plane_preview_v1/close_moving_strip_board.png`;
the whole-hinge rest proof is
`forged_iron_v1/output/broad_forging_plane_preview_v1/comparison_board.png`.

Six millimetres is an authored giant-host translation, not a published
universal property of forged iron. A different stock length, thickness,
fabrication scale, or silhouette needs its own bounded rail envelope. The
proof uses the normal lane only; geometry still owns silhouette, thickness,
rolled edges, and real deformation.

The canonical forged-iron generator and blend remain unchanged. Their closed
superellipse macro events are not promoted by this acceptance. Replacing that
frozen route requires a separate material revision after the substrate/oxide
ownership is corrected.

### 8.4 Accepted capability: continuous intact oxide fields

`pattern_lab_common.continuous_oxide_layer_fields` owns the intact coating
contract before any substrate/oxide shader mix:

- coverage is exactly `1.0` over every eligible intact region;
- thickness is a metre-valued optical/process proxy built from open,
  hand-authored thermal rails;
- optional longitudinal compression modes are bounded to at most twenty
  percent of the declared thickness envelope;
- oxide metalness is exactly `0.0`;
- surface height is exactly `0.0`, because a roughly `50 um` process
  thickness is not permission to emboss the mesh;
- thickness response and compression response remain separate lanes so a
  shader can weight them independently.

The accepted hinge calibration uses a declared `20-90 um` envelope. Its
selected open thermal field occupies `33.19-70.70 um`; these values are a
forging-process proxy, not a profilometer scan of the specific reference
hinge. The helper fails closed when rails leave the envelope, coordinates are
invalid, or combined compression amplitude exceeds its cap.

The actual-consumer proof is
`forged_iron_v1/output/continuous_oxide_preview_v1/comparison_board.png`.
Every candidate uses the approved `3.112 x 0.410 m` moving leaf, identical
cameras, and identical per-view lights. The decisive lane proof is
`forged_iron_v1/output/continuous_oxide_preview_v1/lane_board.png`.

| Candidate | Construction | Decision |
| --- | --- | --- |
| A | uniform `50 um` intact film | retained as the quiet calibration control; omits broad process history |
| B | open thermal-thickness rails | selected default; continuous, quiet, and causally sufficient |
| C | B plus two bounded longitudinal modes | retained as an optional recipe; no decisive actual-asset gain, so disabled by default |
| D | C with exaggerated colour and roughness interpretation | rejected; converts valid broad thickness evidence into decorative bands |

The selection deliberately favors B over C. A mathematically valid extra
signal does not earn default ownership when it produces no defensible visible
improvement. This is the cheap-candidate rule working as intended.

The legacy `scale_plates` and `_irregular_plate` route remains prohibited for
intact oxide. It began from disconnected closed motifs and therefore invented
coverage loss, diamonds, and cat-face pareidolia before any shader could
interpret it. Softer contrast cannot repair the wrong causal primitive.

The proof also found an execution-boundary failure: generated float images
must be refreshed before Cycles samples them. Without that refresh, every
candidate read the default white image buffer and appeared identical. The
comparison was rejected and rerun; no candidate was selected from the invalid
board.

This accepts an isolated coating-data donor, not the completed iron material.
It does not expose the conductor, add rust, pitting, scale fracture, scratches,
edge wear, damage, soot, or grime. It does not displace geometry or generate a
normal. The canonical forged-iron generator, profile, and `.blend` remain
unchanged. Next is a separately reviewed substrate/oxide mixer that must
preserve conductor Fresnel beneath continuous dielectric coverage and prove
its Blender/Unreal ownership independently.

### 8.5 Accepted capability: explicit substrate/oxide mixer

`pattern_lab_common.compose_surface_layer_responses` defines the renderer-
independent composition contract. Its RGB inputs are already-evaluated oxide
and conductor responses. It does not build, recolour, reroughen, renormalize,
or otherwise reinterpret either material. Its only semantic input is
`exposed_conductor_mask`:

- `0.0` returns the oxide response exactly and makes the result invariant to
  the hidden conductor;
- `1.0` returns the conductor response exactly with no oxide leak;
- fractional values are filtered boundary coverage, not a third material;
- oxide and conductor weights always sum to one;
- compiled metalness equals the explicit exposure mask for data adapters;
- mask values outside zero to one fail instead of silently changing material
  identity through clipping.

The selected Blender topology uses two independent Principled BSDFs. The
oxide branch is dielectric with metalness zero. The iron branch is a
conductor with iron normal-incidence reflectance, its own roughness, and the
accepted component tangent. A `Mix Shader` consumes only the explicit exposure
mask. Output-channel adapters may compile colour, roughness, and metalness
maps from the same weights, but the render route must continue to mix complete
lobes.

The actual-consumer comparison is
`forged_iron_v1/output/surface_layer_mixer_preview_v1/comparison_board.png`.
The lane proof is
`forged_iron_v1/output/surface_layer_mixer_preview_v1/lane_board.png`. All
columns use the approved hinge, identical cameras, and identical per-view
lights. The first row uses zero exposure everywhere. The two close rows use a
proof-only smooth longitudinal split from U `0.36` to `0.64`; that split is a
diagnostic witness, not authored wear.

| Candidate | Topology | Decision |
| --- | --- | --- |
| A | one Principled whose colour, roughness, and metalness interpolate | rejected; boundary pixels become an invented half-metal material |
| B | complete oxide and conductor BSDFs mixed by explicit coverage | selected; exact endpoints and single material-identity owner |
| C | B with a forced twenty-percent conductor minimum | rejected; intact oxide visibly inherits forbidden metallic brightness |
| D | metallic conductor under a transparent clear-coat lobe | rejected; clear coat cannot represent opaque approximately `50 um` forge scale |

A and B are intentionally close at the intact endpoint. That does not make A
acceptable: endpoint appearance cannot validate the incorrect intermediate
topology. C and D are deliberately stronger rejection witnesses and visibly
show why an intact oxide shader may not inherit substrate glint merely to look
more metallic.

This accepts a mixer donor, not a placement mask. Hammer activity, broad
forging planes, thickness variation, curvature, and generic edge masks remain
ineligible to expose conductor. Contact polish, abrasion, scale fracture, and
damage require separately authorized, geometry-aware masks with their own
causal proof.

The canonical forged-iron generator, profile, and `.blend` remain unchanged.
The present package still contains legacy exposure ownership inside its frozen
graph; replacing it is a separate integration revision. Unreal parity remains
unproven until two complete Unreal material functions are composed with the
same explicit mask and compared under matched lighting.
