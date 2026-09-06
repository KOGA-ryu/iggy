# Measured Cathedral Ashlar Core

This package owns three deliberately separate capabilities:

1. an intact, source-bounded facing wythe made from individual stone volumes;
2. an intact Santa Marina/Naranjo biocalcarenite surface built from separate
   matrix, sand/silicate, fossil, pore-identity, iron-color, and
   scale-bearing stone-body relief layers;
3. a finite-impact fine oblique layage finish with measured-envelope mark
   length, width, spacing, and density, but no invented groove depth.

It is no longer four large boxes wearing a generic masonry image, and the
surface no longer borrows ambientCG `Bricks008` as a substitute for knowing
the selected stone. The wall core, corner bond, openings, arches, carved
mouldings, damage, arris wear, and tool-groove depth remain outside the
accepted scope until each has its own measurement authority.

## Measurement authority

The canonical ledger is
[`../reference_measurements_v1.json`](../reference_measurements_v1.json).

| Property | Working asset | Evidence |
|---|---:|---|
| Stone face length | 0.80–1.12 m | Inside the surveyed Córdoba range of 0.78–1.13 m |
| Stone face height | 0.38–0.41 m | Inside the surveyed Córdoba range of 0.32–0.43 m |
| Stone depth | 0.17–0.30 m | Uses the complete surveyed Córdoba range |
| Bed joint | 3 mm | Published ashlar working value from the NSW government mortar guide |
| Perpendicular joint | 3 mm | Same numeric technical guidance |
| Joint profile | Flush | Historic England high-quality ashlar guidance |
| Tool-mark spacing | 1–4 mm evidence; 2–4 mm selected in the current designs | Measured Caen-stone fragments |
| Straight-hammer edge / impact length | 60–94 mm | Moulis archaeological survey of medieval layage |
| Fine impact width | 1–2 mm | Same survey |
| Visible impact density | 720–2420/m²; mean 1369/m² | Same survey |
| Tool-mark depth | Not authored | No measured depth has been established |
| Arris radius | Not authored | No measured radius has been established |
| Stone lithology | Tortonian biocalcarenite/biomicrite | Santa Marina petrographic study |
| Likely quarry | Naranjo | Building-to-quarry mineralogical comparison |
| Porosity | 13% | Naranjo quarry sample |
| Fossil components | 30–40% | Naranjo quarry sample |
| Non-degraded mineral fractions | 80% calcite, 15% quartz, 3% feldspar, 2% clay | Naranjo sample |
| Surface figure scales | 50 mm macro; 1 mm thin section | Published scaled figures |
| Relief proxy scan | 3 x 1.5 mm at 100x | Digital multi-focus microscopy of fresh yellowish Sabucina calcarenite |
| Relief filter boundary | 0.8 mm | Published roughness/waviness separation |
| Relief mean absolute height, Sa | 0.10 mm | Fresh Sabucina reference surface |
| Relief RMS height, Sq | 0.13 mm | Fresh Sabucina reference surface |
| Relief peak-to-valley height, Sz | 1.13 mm | Fresh Sabucina reference surface |
| Optical roughness | Authored calibration only | No optical roughness or gloss measurement was published |

The dimensional family comes from thirteen- and fourteenth-century parish
churches in Córdoba. It must not be described as a universal English, French,
or fantasy-cathedral block standard.

The exact ten-course arrangement is an authored translation constrained by
those measurements. It is not a stone-for-stone copy of a surveyed elevation.

## What physically exists

The Blender fixture contains:

- ten measured courses;
- forty cyclic source designs exercised across sixty-nine physical blocks;
- one closed six-face mesh for every visible block;
- block-specific face length, face height, depth, tool spacing, tool direction,
  identity, and variant attributes;
- separate 3 mm mortar bodies for beds and perpendicular joints;
- no large hidden “wall block” pretending that a bitmap supplies construction;
- no bevel modifier, because the arris radius is unknown;
- no fracture, chip, edge-loss, lichen, damp, soot, or carved-ornament pass.

The selected broken bond keeps every adjacent-course perpendicular joint at
least 300 mm from the closest joint above or below. That threshold is an
acceptance rule derived from the selected block dimensions; it is not
presented as a surveyed historic measurement. The source-backed construction
principle is that upper-course block bodies cover the joints below.

## Evidence categories

The package distinguishes six kinds of values:

1. **Surveyed:** published face length, face height, depth, and tooling spacing.
2. **Laboratory characterized:** Santa Marina/Naranjo mineral, fossil, and
   porosity measurements.
3. **Qualitative authority:** fine flush joints and broken-bond construction.
4. **Technical guidance:** the numeric 3 mm joint width; this is not described
   as a measured Córdoba joint.
5. **Explicit proxy transfer:** Sabucina calcarenite topography constrains only
   the scale and distribution of stone-body relief. It is not relabelled as a
   Santa Marina scan.
6. **Surveyed and experimental comparative finish:** straight-hammer mark
   length, fine width, visible density, overlap, and face-working organization
   from eleventh- and twelfth-century southern Lorraine.
7. **Authored translation:** exact course sequence, color ordering, tool
   direction, per-block density choice, and the 300 mm anti-stacking gate.

Still unknown are the wall core, through-stones, corner quoins, arris radius,
tool depth, whole-face plane deviation, opening construction, and
period-specific repairs.

Unknown values are omitted rather than replaced with plausible-looking noise.

## Surface and geometry separation

The build renders the neutral construction proof first. Only after those
renders exist does it replace the neutral block materials with
`IGGY_MAT_CathedralStone_v006` and render the layered surface. The saved
`.blend` contains the live runtime material; the neutral renders remain the
evidence that color did not disguise the construction.

The surface uses two coordinate systems:

- a 4 m architecture/macro field for course-scale and block-scale color;
- a 64 mm material-body field for fossils, silicate grains, and visible pore
  identity;
- a separate 16-bit 64 mm body-height field, blended with an incommensurate
  91 mm sample using the same block-local transformation;
- a 256 mm tooling-detail field whose RGB channels hold sparse, average, and
  dense finite-impact variants at 720, 1369, and 2420 marks/m².

Those independent scales are organized into a deliberate visual hierarchy:

1. the bond, physical joints, and twenty-step warm/pale/cool block families
   establish the distant read;
2. authored piecewise graphic planes create several related values inside one
   block without pretending those color changes are measured displacement;
3. selective packed ink and arris-highlight lanes reinforce only chosen plane
   changes instead of outlining every stone uniformly;
4. measured-proxy body relief, fossils, visible pore color, and finite tooling
   are fully active within 0.30 m and fade smoothly to zero by 3.5 m.

The shader samples the construction maps in stable object-space metres. It
does not derive the material coordinate from generated geometry position, so
object transforms and arbitrary modular placement do not cause the surface to
swim. Boolean and fracture faces still require explicit face-class attributes;
coordinate stability does not invent correct cut-face semantics.

At 1024 px the body field resolves 0.0625 mm per texel. Each physical block
applies its own phase, quarter-turn, and mirror variant before sampling that
field. The 91 mm decorrelation sample prevents the 64 mm source tile from
stamping a visible metronomic repeat inside a long block.

The licensed Santa Marina facade photograph supplies relative sunlit
warm/cool/value families only. Twelve 48 px stone-interior samples exclude
mortar, sky, shadow, mouldings, obvious cracks, repairs, and eroded cavities.
Their exposure is normalized before Inkblotter palette reduction. The raw
photograph is neither retained nor copied into runtime maps.

The petrographic study supplies the layer identities and proportions:

1. micritic carbonate matrix;
2. sandy silicate population;
3. abundant fragmented calcareous fossils;
4. sparse visible pore identity;
5. restrained iron-oxyhydroxide color.

Thirteen-percent bulk porosity is not painted as thirteen-percent black holes.
The visible-pore channel remains much sparser. It contributes localized deeper
valleys inside the proxy-bounded height distribution rather than turning the
bulk porosity percentage into surface-hole coverage.

The relief proxy is a published digital multi-focus scan of a fresh yellowish
coarse-grained Sabucina calcarenite. The 3 x 1.5 mm measured field reports
0.10 mm Sa, 0.13 mm Sq, -0.39 height skew, 3.16 kurtosis, and 1.13 mm total
height. A 0.8 mm robust Gaussian filter separates grain-scale roughness from
broader waviness. The generator reconstructs those two bands separately and
reports its achieved statistics in the manifest.

This is an explicit material-class transfer. It does not claim Sabucina is the
Naranjo quarry stone, and it cannot authorize medieval tool depth, block-plane
deformation, arris rounding, recessed mortar, damage, weathering, or optical
PBR roughness.

The Caen-stone source establishes 1–4 mm spacing and diagonal direction. The
Moulis survey independently establishes 60–94 mm straight-hammer edges,
1–2 mm fine impacts, and a historical density envelope from 720 to 2420
visible impacts/m². Its experimental work also shows that most blows overlap
or disappear during finishing, which is why the maps use finite, layered
passes instead of evenly spaced full-face lines.

The 256 mm tooling field resolves 0.25 mm per texel at 1024 px. Each block
selects a density channel, rotates it by an explicit face angle, and gives it
an independent phase. Slight fan variation and imperfect ends live inside
each pass. This is comparative medieval evidence, not a claim that one
specific Córdoba mason produced these exact marks.

Neither source establishes groove depth. The tooling lane therefore changes
only restrained color and roughness at inspection distance. It emits no normal
or height depth and fades out before its millimetre rhythm becomes screen-space
noise.

The live ink and highlight are optical value-grouping layers, not baked
lighting. They do not alter metallic, AO, measured body-height amplitude, or
provenance. Damage and narrative grime remain completely disabled.

## Files

- `patterns/cathedral_ashlar_courses_v1.json` is the measured-envelope course,
  bond, depth, joint, and tooling-spacing recipe.
- `test_measured_ashlar_contract.py` is the focused no-guessing gate.
- `generate_cathedral_stone_v1.py` produces the separable surface channels and
  source/translation manifest.
- `capture_cathedral_stone_reference_v1.py` measures the licensed facade
  color families with the existing Inkblotter palette reducer and writes only
  numeric statistics plus an attributed palette proof.
- `references/santa_marina_biocalcarenite_capture.json` joins those relative
  color measurements to the published Santa Marina/Naranjo material body.
- `build_cathedral_stone_v1.py` produces the full-volume neutral geometry
  proof, then assigns and proves the layered runtime shader.
- `profiles/cathedral_stone_v1.json` states the engine-facing contract and
  records every excluded or unknown lane.
- `RESEARCH_INTENT.md` gives the construction reasoning and next evidence
  requirements.

## Reproduce

Run the focused measurement gate:

```sh
/Users/kogaryu/font/.venv/bin/python \
  assets/creative/materials/cathedral_stone_v1/test_measured_ashlar_contract.py
```

Build the neutral Blender fixture headlessly:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/cathedral_stone_v1/build_cathedral_stone_v1.py \
  -- --texture-resolution 1024 --render-width 1200 --render-height 900
```

## Current acceptance boundary

Accepted:

- source-bounded block dimensions;
- actual individual block volumes;
- broken bond without stacked adjacent-course joints;
- separate narrow flush joint bodies;
- complete measurement attributes and manifest;
- neutral, grazing, close, and gameplay-distance proof renders.
- named Santa Marina/Naranjo biocalcarenite body;
- four independent body layers plus sparse iron color;
- separate 4 m and 64 mm shader coordinate systems;
- per-block phase, quarter-turn, and mirror detail variation;
- finite 60–94 mm tooling impacts with measured width and three measured
  density levels;
- per-block tooling angle, phase, and density attributes;
- non-metal PBR contract with no baked lighting;
- 16-bit proxy-bounded body height plus derived body normal;
- measured relief targets of 0.10 mm Sa, 0.13 mm Sq, and 1.13 mm range;
- neutral architectural height, tooling depth, damage depth, and stone-body AO;
- licensed facade attribution with no raw photo pixels in runtime maps.

Not accepted:

- a reconstruction-grade roughness value; the current range is explicitly a
  neutral/grazing-light calibration;
- corner or opening construction;
- arch/voussoir construction;
- carved trim;
- any damage or weathering;
- Santa Marina/Naranjo-specific face topography;
- any unmeasured block-plane deformation, bevel, tool depth, mortar recession,
  or damage depth;
- Unreal reconstruction and Blender/Unreal parity.
