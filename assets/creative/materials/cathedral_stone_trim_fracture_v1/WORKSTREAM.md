# Material Workstream Dossier

## Goal

- Goal objective: Repair the canonical
  `cathedral_stone_trim_fracture_v1` package by delivering one
  production-candidate measured chevron-voussoir portal trim capability with
  source-bounded dimensions and moulding anatomy, an explicit texture
  inventory, exact Blender node and shader graph, complete coded demands,
  neutral and live-material proofs, and saved-file reopen validation.
- Material ID: `cathedral_stone_trim_fracture_v1`
- Capability: measured chevron-voussoir portal trim
- Workflow tier: `hero-master`
- Acceptance target: `pending-selection: production cathedral portal
  consumer`; this must be resolved before acceptance
- Target asset or scene: the package's carved portal acceptance fixture,
  coordinated with the intact `cathedral_stone_v1` ashlar core
- Target quality family: measured medieval portal construction with
  Arcane-adjacent graphic value grouping and gameplay readability
- Required final artifacts: research and measurement ledger, revised intent
  and profile, authored chevron/voussoir pattern, production maps, packed
  Blender material and proof fixture, texture and Blender manifests,
  generator/contract/reopen tests, adversarial proof renders
- Manual acceptance required: no for delivery of the production candidate;
  explicit user review is still required before the capability may be called
  accepted
- Package root:
  `assets/creative/materials/cathedral_stone_trim_fracture_v1`
- Coded demands:
  `assets/creative/materials/cathedral_stone_trim_fracture_v1/CODED_DEMANDS.md`
  with synchronized complete tests, profile, pattern, generator, builder, and
  executed node/link inventory
- Champion seed or variant: preserve current deterministic champion seed
  `41723` unless measured pattern closure requires a versioned replacement
- Current status: production candidate passed the five-phase workflow
  calibration and independent candidate audit; acceptance remains unavailable
  until the target identifier, target proof, and manual review are real

## Scope

- Included:
  - one measured chevron-voussoir portal order;
  - voussoir geometry dimensions and radial construction;
  - roll, hollow, and roll moulding anatomy supported by the selected source;
  - trim-specific color, normal, roughness, height, identity, and
    stylization lanes;
  - stable local coordinates and explicit carved-trim identity;
  - exact Blender node and shader implementation;
  - neutral, close, grazing, distance, repetition, and actual-portal proofs;
  - focused generation, measurement, node, geometry, packing, save, and reopen
    tests.
- Excluded:
  - fracture-interior repair or acceptance;
  - dogtooth, leaf, cyma, cavetto, corbel, and entablature acceptance beyond
    what the measured chevron order actually requires;
  - chips, erosion, cracks, arris loss, lichen, damp, soot, dust, repair, and
    other damage or narrative overlays;
  - changes to the accepted intact `cathedral_stone_v1` core;
  - Unreal parity until an Unreal reconstruction is independently rendered
    and compared.
- Assumptions:
  - the existing package remains the canonical owner instead of creating a
    parallel package;
  - the current portal fixture is an audit donor, not accepted geometry;
  - current v1 maps and motifs remain evidence of the rejected prototype and
    are not measurement authority;
  - source-backed chevron dimensions may replace current band dimensions,
    repeat, motifs, geometry, map zoning, and shader version.
- Stop conditions:
  - do not guess an occluded moulding dimension, radius, relief depth, or
    voussoir relationship;
  - do not call a visual reconstruction measured unless every claimed numeric
    dimension has source or explicitly bounded authored status;
  - stop for direction if the selected source cannot distinguish the chevron
    order well enough to create a coherent portal capability;
  - do not alter unrelated user changes.

## Existing state

- Current geometry: a scripted annular portal and additional trim/fracture
  proof objects in
  `build_cathedral_stone_trim_fracture_v1.py`; its documented dimensions are
  prototype values rather than accepted measurements.
- Current generator:
  `generate_cathedral_stone_trim_fracture_v1.py`, schema
  `iggy3d.material.cathedral_stone_trim_fracture_v1.v1`, champion seed
  `41723`.
- Current shader:
  `IGGY_SH_CathedralTrimFracture_v001` with
  `IGGY_MAT_CathedralTrim_v001` and
  `IGGY_MAT_CathedralFracture_v001`.
- Current maps: combined trim/fracture atlas at 1536 px with base color,
  normal, ORM, 16-bit height, identity, break-age, stylization, painterly, and
  fracture-transition maps.
- Current tests:
  `tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py` and
  `tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py`.
- Current proof renders: hero, grazing, fracture close-up, mask proof, and six
  generated proof pages under the package output directory.
- Known visible failures:
  - current documentation explicitly rejects the 260-280 mm bands,
    83.3 mm dogtooth spacing, 132 mm leaf spacing, and fracture depths as
    unmeasured;
  - the four-band atlas conflates multiple unaccepted trim families with the
    fracture library;
  - current profile still points to an obsolete ambientCG palette capture
    rather than the accepted intact-stone color authority;
  - the portal arch is one smooth annular ribbon with a repeated diamond field,
    not an assembly of individual wedge-shaped voussoirs;
  - its 2.65-repeat UV treatment cuts diamonds at the springing and stretches
    one pattern continuously across forty arbitrary arch segments;
  - the jamb motif switches to a different repeated oval/leaf field, so the
    vertical and arched portions do not form one coherent chevron order;
  - the wall texture contains large cloudy landmarks that compete with the
    portal and expose repetition under the grazing proof;
  - the normal image is packed but is not consumed by the material;
  - identity, break-age, stylization, painterly, and fracture-transition
    images terminate at proof outputs and do not alter the live material;
  - `iggy_carved_trim` and `iggy_fracture_interior` Attribute nodes exist in
    the two materials but are not linked, so material selection depends on
    slots rather than the documented shader selectors;
  - the live trim relief comes only from the height image through a Bump node
    with a 0.020 m distance, a value unsupported by the present sources;
  - the generator test cannot reproduce the maps because the profile points to
    the missing
    `cathedral_stone_v1/references/ambientcg_bricks008_capture.json`;
  - the stale saved `.blend` reopens and passes its current four tests, but
    those tests prove only object counts, face-selection metadata, overlays
    defaulting off, and packed images. They do not prove measured construction,
    map consumption, or reproduction from source.
- Unrelated dirty paths to preserve:
  - the complete package and its two unit-test files were already untracked
    before this goal;
  - all other modified and untracked repository paths belong to the user and
    remain outside this goal.

## Prompt 01 live-route audit

`profile -> pattern or motif -> generator -> maps -> Blender builder -> node
group -> assigned material -> proof geometry -> saved asset`

| Link | Canonical owner | Live consumer | Audit result |
| --- | --- | --- | --- |
| source and channel policy | `profiles/cathedral_stone_trim_fracture_v1.json` | generator and builder | stale palette path makes fresh generation fail |
| band dimensions and motifs | `patterns/cathedral_trim_fracture_atlas_v1.json` and two SVGs | generator | executable but explicitly admitted prototype dimensions |
| raster maps | generator | packed node-group image nodes | nine files exist; several are proof-only |
| texture coordinates | `IGGY_TrimFractureUV` | `IGGY_AtlasUV` | live and deterministic, but the arch maps 2.65 repeats across one ribbon instead of per voussoir |
| color | base-color image | Principled Base Color | live |
| roughness | ORM green | Principled Roughness | live |
| relief | height image | Bump Height at 0.020 m | live but unmeasured |
| tangent normal | normal image | none | packed and dormant |
| identity and stylization | identity, break-age, stylization, painterly, and transition images | proof-output sockets only | packed and dormant in the live material |
| geometry selectors | face-corner attributes and two Attribute nodes | none inside the material | metadata exists; shader selector nodes are dormant |
| proof geometry | builder | saved `.blend` and four renders | stale output reopens; source regeneration fails before test execution |

### Actual target geometry

- `IGGY_CarvedArchBand_Dogtooth`: `2.9594 x 0.25 x 1.48 m`,
  164 vertices, 162 polygons, one trim material, one 8 mm two-segment Bevel
  modifier.
- `IGGY_CarvedJamb_Left` and `IGGY_CarvedJamb_Right`: each
  `0.36 x 0.23 x 1.72 m`, eight vertices, six polygons, one trim material,
  and one 10 mm two-segment Bevel modifier.
- The source arch is generated from forty equal angular segments. Those
  segments are tessellation, not modeled voussoir joints or individual stones.
- The current local frame is object-space geometry plus the explicit
  `IGGY_TrimFractureUV` face-corner UV map. The carved identity exists as
  `iggy_carved_trim`, but does not participate in the live shader.
- No Geometry Nodes or Boolean operation controls the target portal arch.

### First failing observable

The portal order is not built from measured individual wedge-shaped
voussoirs. It is one smooth annular mesh with an arbitrary 83.3 mm repeated
diamond motif and an unmeasured 8 mm bevel. The first repair must therefore
prove construction before surface decoration: a source-identified radial
assembly whose individual stones, joint angles, profile zones, dimensions,
and chevron phase can be inspected after saving and reopening.

### Candidate proving gate

A focused Blender test will reject the current prototype unless the saved
asset contains:

- a versioned measured-source identifier;
- a versioned chevron-voussoir contract;
- individually identifiable radial voussoir geometry;
- source-bounded inner and outer stone widths, height, depth, and order
  radius;
- real wedge joints rather than UV-drawn separators;
- separately addressable roll, hollow, roll, chevron facet, quiet stone, and
  joint identities;
- a material node graph that consumes its authorized color, normal,
  roughness, linework, and identity lanes;
- a proof manifest covering neutral clay, unlit base color, grazing, close,
  gameplay distance, repetition, and saved-file reopen views.

## Stage ledger

| Stage | Status | Evidence | Blocking issue |
| --- | --- | --- | --- |
| 00 Goal bootstrap | complete | Active goal, canonical package, bounded chevron capability, exclusions, current schemas, seed, and dirty-state boundary recorded here | |
| 01 Existing asset audit | complete | Live `.blend` geometry/node inspection, current hero/grazing/mask-proof visual inspection, generator red baseline, and saved-file reopen gate recorded below | Current source cannot regenerate because its palette capture is missing; this becomes a repair input, not a reason to preserve the prototype |
| 02 Reference research | complete | Eight-source written ledger, surveyed Old Sarum dimensions, independent Gloucester construction comparison, CRSBI chevron grammar, official Blender method, three professional visual comparisons, explicit image translations, closure calculation, and prohibited inferences | S01 does not publish roll diameter or relief depth; those values must remain labeled authored translation |
| 03 Research to intent | complete | Active `RESEARCH_INTENT.md` now defines the exact surface boundary, observation/intent/implementation/prohibition ledger, frequency ladder, color order, independent PBR causality, stable coordinates, finite motif vocabulary, exclusions, red observable, focused gates, proof set, and rejection conditions | Historical roll diameter and relief depth remain unknown; the versioned 25/50/50/50/25 mm and 12/6 mm profile is explicitly authored and replaceable |
| 04 Coded implementation blueprint | complete | `CODED_DEMANDS.md` contains two bounded demands, five texture rows, every final group node and link, complete generator/reopen tests, complete profile/pattern data, complete generator code, complete builder/manifest/proof code, and exact commands | |
| 05 Pattern and map build | complete | v2 profile and pattern; deterministic 1024 px basecolor/ORM plus byte-exact inherited body masks/normal/height; texture manifest proves five consumed lanes and twenty-shade palette | |
| 06 Shader and geometry integration | complete | sixteen separate closed wedge meshes; 1,650 vertices/1,648 polygons each; two metre UV layers; eleven required attributes; 79-node/103-link packed group; live Principled color, roughness, normal, linework, and distance paths | |
| 07 Damage and overlays | skipped-out-of-scope | fracture identity is explicitly false on every face; no damage, weathering, soot, damp, lichen, repair, or overlay texture/node path exists | Capability objective explicitly excludes damage and fracture |
| 08 Adversarial proof | complete | nine distinct 900 px proofs cover clay front/grazing, live front/grazing, measured close, distance, moulding masks, identity, and wireframe; source dimensions, manifold topology, shader links, packing, and proof hashes are scripted | |
| 09 Repair loop | complete | repaired quadratic edge validation, the incorrect 100% primary-normal drive, and overexposed neutral lighting; regenerated maps, blend, manifests, and all proofs after repairs | |
| 10 Final validation and handoff | complete | final 11 generator, 9 reopen, and 13 workflow-policy tests green; blueprint revision 2 frozen and opened after revision 1 exposed stale generated test-count prose; candidate package audit green across workflow state, 33 claims, nine causal layers, budgets, 17-file inventory, live routes, documentation, and hashes; accepted-state negative audit rejects the unresolved target, absent target proof, and absent manual review | Acceptance inputs are deliberately unresolved; they do not block candidate delivery |

Allowed status values:

- `pending`
- `in-progress`
- `verified-existing`
- `complete`
- `skipped-out-of-scope`
- `repair-required`
- `blocked`

## Source and measurement ledger

| ID | Source | Evidence class | Measurement or observation | Intended use | Transfer limit |
| --- | --- | --- | --- | --- | --- |
| S01 | Brodie and Algar, *Architectural and Sculptured Stonework*, Old Sarum catalogue 54–61 | surveyed and qualitative authority | chevron voussoirs span 190–230 mm radially, 115–250 mm across tapered ends, and 200–350 mm deep; catalogue 55 is 200 mm, 180–140 mm, 270 mm; pattern is roll/hollow/roll | champion stone envelope and moulding sequence | stones came from arches in the plural; complete radius, count, joint, and profile depth are absent |
| S02 | Gloucester Cathedral south-transept archaeology | surveyed and qualitative authority | worked stones prove explicit face/soffit, joint-crossing versus stone-centered phase, roll/fillet sequences, directional tooling, and a possible matching jamb stone | prevent texture phase and tooling from ignoring construction | different building, stone group, profile, and scale; dimensions do not transfer to S01 |
| S03 | Corpus of Romanesque Sculpture, *Chevron Guide* | qualitative authority | chevron is three-dimensional roll-based zigzag; lateral/frontal, face/soffit/edge, centripetal/centrifugal, normally one unit per voussoir | one phase-locked centripetal lateral-face chevron per stone | selected direction is authored; S01 does not state it |
| S04 | Official Blender manual and Python API | technical guidance | connected Spin extrusion is not separate stones; explicit meshes/attributes/UVs, Non-Color data, Normal Map node, packed save/reopen behavior | exact scripted geometry and shader route | documentation defines Blender behavior, not artistic dimensions |
| S05 | Max Kutsenko, *Church Wall Trim* | comparative finish and written workflow | blockout strips, finite motifs, height-first assembly, layered color washes conforming to height, straightened arch UVs, actual-asset proof | authoring and proof method | damage/grunge and ornament are not copied |
| S06 | Mike Means, *Ornamental Stone Trim Sheet* | comparative finish | large rails, medium motifs, fine material response, quiet bands, related color movement, separate height/normal/roughness | finish hierarchy | no measurements or pixels transfer |
| S07 | Gert-Jan van de Put, *Baldur's Gate 3 — Temple Tileset* | shipped-use comparative finish | modular high/low pieces and trims; readable radial joints, nested orders, broad warm/cool/material groups, sparse focal accents | gameplay and world-use benchmark | visual benchmark only; BG3 geometry and textures are not copied |
| S08 | Mika Kuwilsky, Gothic modular-kit breakdown | written professional workflow | dimensioned blockout, characteristic forms, small reusable baked vocabulary, silhouette pieces, clean reusable base, structural repetition breakers | bounded modular strategy | project-specific kit values do not transfer |

## Professional comparison

| Benchmark | Construction | Macro | Medium | Edges | Color | Reflection | Stylization | Repetition and use |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BG3 Temple Tileset | high/low modular wall and portal pieces; distinct arches, columns, capitals, spandrels, and trim | nested orders and openings dominate before texture | radial stone joints, carved bands, panels, and shafts retain separate identities | joints and selected arrises read without universal black outlines | pale warm limestone, cool blue-gray recesses, muted red-brown carving, blue panels, sparse gold; several values inside one stone family | broad diffuse stone versus sharper metal accents | selective contrast and color grouping exaggerate hierarchy while retaining construction | trims repeat inside many opening sizes but are interrupted by columns, panels, capitals, wall rhythm, and actual editor assemblies |
| Mike Means Ornamental Stone Trim | deliberately separated trim bands wrapped around curved and flat forms | broad rails and ledges establish the stack | egg-and-dart, arcade, foliage, leaf, and quatrefoil bands use distinct depth and frequency | crisp profiles with restrained loss and no one-width outline | cool gray body, warmer shoulders, pale granular accents, and dark recess families move inside motifs | broad rough stone with local response changes | large, medium, and fine maps preserve a sculpted graphic read | finite band vocabulary is reusable; quiet rails keep the sheet from becoming continuous ornament |
| Max Kutsenko Church Wall Trim | real portal reference translated into a trim sheet and straightened arch UV chunks | multiple nested orders and plain separators | finite floral, rope, roundel, leaf, and rail vocabularies | height and bevel separate shapes before color | layered washes conform to height; lit reference photo is rejected as albedo authority | rich roughness foundation is layered separately, though its direct albedo conversion is not adopted here | dense authored motifs remain legible through deliberate scale changes | demonstrated on an actual arch; radial joints and trim alignment remain visible |

## Material anatomy and layer contract

| Layer | Physical meaning | Scale | Orientation | Outputs | Absence or rest rule | Proof |
| --- | --- | --- | --- | --- | --- | --- |
| construction | sixteen closed wedge-shaped stones and real radial joints | 200 mm radial height, 140–178.485 mm body chords after closure, 270 mm depth, 3 mm technical joint | semicircular XZ order around the springing center | geometry, stone ID, face identities | no connected annular ribbon; no UV-drawn joints | clay front, three-quarter, grazing; object and manifold gate |
| macro | complete order and broad stone-family grouping | 1.455103 m clear diameter; four deterministic pale/warm/cool variants | per stone and complete arch | base color and material-variant attribute | no wall-scale cloudy landmark; several quiet stones | unlit color and gameplay-distance proof |
| medium | one chevron and roll/hollow/roll anatomy per stone | 25/50/50/50/25 mm authored zones, 12 mm roll crest, 6 mm hollow depression | stone-local lateral face, centripetal | geometry, roll/hollow/quiet attributes, body color/normal/roughness | motif cannot cross a joint; inner soffit/back/outer/joint faces remain quiet | close clay, close material, anatomy diagnostic |
| edge and event | construction joints plus selective optical structure lines | 3 mm physical gap; narrow authored ink and crest masks | radial joint sides and selected chevron structure | ink and highlight attributes feeding color only | at least four reduced-linework quiet stones; no universal outline | ink/highlight diagnostic and occupancy metrics |
| micro | accepted calcarenite body identity and relief proxy | 64 mm main span, 91 mm decorrelation span, 1.13 mm proxy height range | transformed stone-local UV per element | core body maps, tangent normal, local roughness | fades from full close response to zero by 3.5 m; no invented tool depth | close/gameplay/far comparison |
| stylization | graphic warm/cool grouping with selected ink and highlight | broad per-stone grouping plus narrow structure masks | construction- and anatomy-selected | final base-color composition | not baked light; no Shader-to-RGB; fades before distance noise | neutral material, changed-light, and isolated masks |
| contextual overlays | none in the intact candidate | zero | not applicable | fracture and damage values fixed at zero | soot, limewash, lichen, damp, dirt, cracks, chips, repair, and wear absent | profile, material, and reopen exclusion tests |

## Coordinate and semantic contract

- Coordinate origin: arch centre at world X=0 and the springing-height Z;
  depth is centered on local Y=0
- Axes: X spans the opening, Y is wall depth with negative Y toward the proof
  camera, and Z is vertical/radial
- Metre repeat: one chevron unit per voussoir; stone-body textures retain the
  accepted 64 mm and incommensurate 91 mm material spans from
  `cathedral_stone_v1`
- Arbitrary-length rule: not applicable to the complete portal order; any
  reusable moulding segment must retain measured radial and profile scale
- Scale-under-transform rule: apparent stone and moulding dimensions remain
  expressed in object-local metres
- Element identities: explicit stone ID, front, back, inner, outer, radial
  joint side, roll, hollow, quiet stone, and linework-priority identities
- Phase and variant: deterministic per voussoir; one chevron is locked to each
  stone, while stone-body phase, quarter-turn, and mirror vary by stone ID
- End or cut face: outside this intact-trim capability unless required by
  portal construction
- Boolean or fracture face: fracture remains excluded and default off
- Blender reconstruction: independent closed wedge volumes built from radial
  bounds and an explicit relief grid; stable face-corner UV; live named
  attributes; tangent normal through Normal Map; all images packed and
  reopened
- Target-engine reconstruction: document only; parity remains false

## Color contract

- Base families: inherit accepted intact `cathedral_stone_v1` stone-family
  authority after audit
- Within-element shade structure: several related values inside each
  voussoir, with quiet broad fields preserved
- Broad warm and cool passages: inherit the accepted Santa Marina/Naranjo
  family, with BG3 used only to judge the strength and readability of
  warm/cool grouping
- Anatomy or construction colors: rolls, hollows, tooling, and selected facets
  remain separable
- Event colors: no damage events in this capability
- Ink and highlight: selective chevron and moulding emphasis, not universal
  outlines
- Prohibited baked lighting: photographed shadow, AO, and fixed directional
  highlight

## PBR contract

- Height: 200 mm stone envelope is surveyed; roll/hollow/roll anatomy is
  authoritative; 25/50/50/50/25 mm profile zoning, 12 mm roll crest, and
  6 mm hollow depression are explicit replaceable authored translations;
  material-body relief retains only the accepted comparable-calcarenite proxy
- Normal: derived only from authorized profile or surface lanes
- Roughness: separate material response, not inverted height
- AO: geometry or justified recess only
- Metalness: zero
- Opacity: opaque
- Physical layer blends: intact limestone only
- Distance hierarchy: full stone body and selective linework at close view;
  material microdetail fades before gameplay distance; the sixteen-stone
  order, real joints, and roll/hollow rhythm must survive

## Damage and overlay contract

- Intact master path:
  `assets/creative/materials/cathedral_stone_v1`
- Included damage capability: none
- Placement semantics: explicit carved-trim identity only
- Default-off overlays: fracture, chip, crack, lichen, damp, soot, dust,
  impact, repair
- Explicitly excluded damage: all damage and fracture capabilities

## Implementation ledger

| Intent | Canonical owner | File or node | Focused test | Isolated proof |
| --- | --- | --- | --- | --- |
| Own measured and authored dimensions once | v2 profile and pattern contract | existing profile/pattern paths, versioned to v2 | `test_source_bounded_voussoir_and_authored_completion_are_explicit`; closure recomputation test | clay front and wireframe |
| Build actual voussoirs | canonical Blender builder | `build_cathedral_stone_trim_fracture_v1.py` | individual closed-wedge, dimensions, attributes, and saved-file tests | clay front/grazing and wireframe |
| Generate only consumed maps | canonical generator | `generate_cathedral_stone_trim_fracture_v1.py` | exact inventory, PNG format, deterministic hash, ORM, palette, and core-preservation tests | live front/close |
| Reuse accepted stone body only | intact core body maps and manifest | `IGGY_BodyMasks64/91`, `IGGY_BodyNormal64`, `IGGY_BodyHeight91` | byte-exact source hash and packed color-space tests | measured close/live grazing |
| Apply optical linework selectively | front/carved point and face attributes | `IGGY_SelectiveChevronInk`, `IGGY_SelectiveCrestHighlight` | live-attribute, exact-link, and exclusion tests | moulding proof and live front |
| Suppress microdetail by distance | live shader | `IGGY_DetailFullAt030mZeroAt3p5m`, `IGGY_LineFullAt030mZeroAt6m` | exact node/default/link inventory plus close and distance proof manifest | measured close and distance read |
| Prove durable saved state | Blender manifest and reopen test | packed `.blend`, nine proof hashes | nine-test saved-file reopen gate | all nine final render paths |

## Coded demand ledger

| Demand ID | Visible result | Texture rows | Node rows | Test code | Production code | Execution status |
| --- | --- | --- | --- | --- | --- | --- |
| DEM-TEX-001 | mortar-free five-lane calcarenite family with broad twenty-shade colour and accepted measured-proxy body anatomy | basecolor, ORM, body masks, body normal, body height | six image consumers plus exact coordinate, decode, and distance rows in the complete inventory | complete generator contract test file embedded under the demand | complete profile, pattern, and generator embedded under the demand | green: eleven tests |
| DEM-GEO-002 | sixteen individual measured-envelope chevron voussoirs, live 79-node group, nine proofs, and reopened saved asset | all five rows consumed | every final node, socket default, mode, and 103 links generated from the executed Blender manifest | complete 402-line reopen test file embedded under the demand | complete 1,975-line Blender geometry/shader/proof/manifest builder embedded under the demand | green: build, nine renders, nine reopen tests |

## Verification

| Gate | Command or method | Result | Evidence path |
| --- | --- | --- | --- |
| Python source compilation | `python3 -m py_compile` on the existing generator and builder | pass | both current scripts parse; source failure is data provenance, not syntax |
| Fresh generator baseline | focused Blender generator test from factory startup | fail before tests: missing `cathedral_stone_v1/references/ambientcg_bricks008_capture.json` | generator stderr and stale profile `base_palette_capture` |
| Saved-file reopen baseline | focused Blender reopen test against the existing `.blend` | pass: 4 tests | `tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py` |
| Saved target geometry audit | headless Blender object and modifier inspection | arch ribbon is 2.9594 x 0.25 x 1.48 m with 164 vertices, 162 polygons, and an 8 mm bevel; jambs are simple beveled boxes | existing packed `.blend` |
| Saved material-link audit | headless Blender node/link inspection | base color, ORM roughness, and height bump are live; normal, selector, identity, stylization, painterly, break-age, and transition behavior are not | existing packed `.blend` |
| Current grazing proof | original-resolution visual inspection | smooth ribbon, cut motif at springing, unrelated jamb rhythm, cloudy wall landmarks, weak medium stone structure | `output/cathedral_stone_trim_fracture_v1_grazing.png` |
| Current identity proof | original-resolution visual inspection | material slots can produce red/green proof, but the live selector nodes remain unlinked | `output/cathedral_stone_trim_fracture_v1_mask_proof.png` |
| Final generator gate | factory-startup Blender runs the v2 generator contract suite | pass: 11 tests | `tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py` |
| Final build gate | factory-startup Blender runs the canonical v2 builder | pass: 16 objects, 9 proofs, packed blend, texture and Blender manifests | `output/cathedral_stone_trim_fracture_v1_blender_manifest.json` |
| Final saved-file gate | headless Blender reopens the generated `.blend` and runs the v2 saved-file suite | pass: 9 tests | `tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py` |
| Workflow policy gate | `python3 -m unittest tests.unit.material_workflow_policy_tests` | pass: 13 tests covering freeze order, post-entry demand drift, placeholder rejection, omitted targets, unsupported dimensions, layer and ordered-method provenance, damage placement, dormant routes, stale outputs, budgets, and false acceptance | `tests/unit/material_workflow_policy_tests.py` |
| Fast proof modes | temporary `none` and one-proof `changed` builds; canonical `none` negative gate | pass: temporary builds produce 0 and 1 proof respectively; canonical fast build is rejected | builder `build_mode` contract and temporary manifests |
| Blueprint freeze and build entry | `material_workflow_gate.py freeze`, `open-build`, and `verify-entry` | pass: hero-master revision 2, two demand IDs, six declared production targets, verified open entry; revision history preserves the stale-prose repair | `WORKFLOW_STATE.json` |
| Geometry metrics | recomputed by builder and reopened tests | 16 separate meshes; all manifold; inner chord 0.139999999995 m; outer chord 0.178485252852 m; centreline gap 0.003000000002 m; zero bevels and Booleans | Blender manifest `geometry` and `validation` |
| Shader metrics | exact node and link inventory from executed group | 79 nodes, 103 links, six packed image nodes consuming five unique lanes, one live Normal Map, one non-inverted Bump, ten live Attribute nodes | Blender manifest `shader` |
| Layer-provenance audit | profile-to-manifest package audit | pass: construction, macro, medium, edge/event, micro, cumulative colour, height/normal, response, and stylization all name sources, outputs, consumers, proofs, rest rules, and supporting claims | profile `workflow_contract.layer_provenance` and `MATERIAL_AUDIT.json` |
| Proof integrity | builder hashes every rendered view and reopen test recomputes each hash | pass: nine present and nine unique hashes | Blender manifest `proofs` |
| Coded-demand drift | full material-specific renderer comparison inside the package auditor | pass: exact generated document SHA-256 `79184ff28b1252088164c1c2b4325ce3301a9baaf027d075235be74ad9bd9863` | `CODED_DEMANDS.md` |
| Candidate package audit | full `audit_material_package.py --state candidate` command from README | pass: all nine audit families green after the audit caught and removed one stale `.blend1` backup | `MATERIAL_AUDIT.json` |
| Accepted-state negative audit | same audit with `--state accepted`, without invented evidence | expected fail: unresolved target identifier, no actual-target proof, and no manual-review record | acceptance check output and profile `workflow_contract.acceptance` |
| Final artifact audit | package auditor plus exact output inventory | pass: blend SHA-256 `4a65204c313967c375616f26487bea4806b383ba3cff8b3c5e07e3e3289b5ca7`; 9 proofs; 17 canonical output files; no unexpected output | package output, texture manifest, Blender manifest, and `MATERIAL_AUDIT.json` |
| Intact-core boundary | workstream command/write audit | pass: no file under `cathedral_stone_v1` was edited; its body maps were read and byte-copied only | generator `copy_measured_body_lanes` and this dossier's dirty-state boundary |

## Visual review

- Strongest visible result: live-material front; the sixteen-stone order,
  actual joints, joint-locked V rhythm, broad quiet stone, and selective
  hollow/crest value grouping read before microdetail
- Strongest diagnostic result: wireframe; it proves the chevron is sampled
  geometry on sixteen closed volumes rather than a stamped annular texture
- Weakest visible result: the proof context is deliberately schematic and
  does not reconstruct historical jambs, capitals, or surrounding masonry
- Procedural signature: each stone necessarily repeats the single selected
  chevron grammar, but the joint break, four UV transforms, two independent
  phase frames, four material variants, and incommensurate 64/91 mm samples
  prevent one square material tile or one continuous arch phase
- Close-range result: 12 mm rolls and 6 mm hollows retain rounded relief; the
  repaired 66/34 normal-height split shows porous body response without
  replacing profile geometry
- Gameplay-range result: linework and actual relief preserve the V rhythm;
  body microdetail yields to construction through the 3.5 m fade
- Distance result: the complete arch silhouette, sixteen joints, and chevron
  value rhythm remain; pore-scale response is absent as contracted
- Isolated-fixture result: the canonical package builds its complete portal
  order and proof context from the same generator, profile, pattern, material,
  and save route consumed by tests
- Actual-target result: deliberately absent; the production cathedral portal
  consumer has not yet been selected, so acceptance is automatically rejected
- Professional-reference gaps: no nested portal orders, capitals, jamb
  reconstruction, scene-integrated BG3-scale masonry, damage, or Unreal
  reconstruction is claimed in this bounded capability
- Repair-pass count: 3
- Repairs: linearized the manifold audit after it stalled post-render;
  corrected the primary normal from 100% to its documented 66% share; reduced
  proof exposure so warm/cool stone grouping and hollow values remain visible
- Workflow-calibration repair count: 1
- Workflow-calibration repair: the first package audit rejected an
  unmanifested Blender `.blend1` backup; it was removed from canonical output
  and the exact 17-file audit then passed

## Final boundary

- Accepted or candidate capability: production-candidate measured
  chevron-voussoir portal order in Blender; not user-accepted
- Remaining exclusions: fracture, damage, weathering, other trim families,
  Unreal parity
- Blender parity: complete for the scoped profile, maps, geometry, live
  shader, packed save, proofs, and reopen tests
- Target-engine parity: false
- Manual review status: production candidate presented for user review; user
  acceptance is not claimed
- Next material capability: remain inside the structural sequence—either
  source-bounded jamb/capital completion for this portal or forged iron—only
  after review of this candidate
