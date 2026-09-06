# Research Intent: Intact Measured Ashlar

## The question this pass answers

Can the asset read as fitted historic ashlar when color, damage, ornament, and
dramatic lighting are not allowed to rescue it?

The previous answer was no. Four large wall slabs carried a repeating masonry
image. The visible courses existed in pixels, while the actual mesh had no
individual stones, block depth, joint bodies, or bond.

This pass changes the representation before adding detail.

## Selected construction family

The dimensional authority is the measured early-Gothic Córdoba church family:

- observed face length: 0.78–1.13 m;
- common face length: 1.00–1.10 m;
- observed face height: 0.32–0.43 m;
- common face height: 0.40–0.41 m;
- observed depth: 0.17–0.30 m;
- common depth: 0.20–0.21 m.

The current forty block designs use:

- face length: 0.80–1.12 m;
- face height: 0.38–0.41 m;
- depth: 0.17–0.30 m.

Most stones remain near the published common dimensions. Less-common depths
exercise the complete surveyed range without pretending that an independently
random length, height, and depth sample is a real surveyed block.

The course layout is synthetic. Its constraints are historical; its exact
sequence is authored. That distinction remains in both the recipe and output
manifest.

## Selected stone body

The surface is not generic “cathedral limestone.” The selected building family
now has a named material authority: the published characterization of Santa
Marina de Aguas Santas in Córdoba.

The dominant west-facade material is yellowish clastic sandy
biocalcarenite, accompanied by sandy fine-grained biomicrite. The study links
the fabric most closely to the Naranjo quarry and reports:

- 13% porosity;
- 30–40% fossil components;
- abundant fragmented foraminifera plus algae, bryozoans, bivalves, and
  echinoid material;
- a non-degraded Naranjo sample comprising 80% calcite, 15% quartz, 3%
  feldspar, and 2% clay;
- visible iron oxyhydroxides responsible for the more saturated yellow color.

Conglomeratic biocalcarenite and biosparite occur as uncommon lithotypes. They
are excluded from the default body and reserved for explicit variants; they
cannot be sprinkled over every stone to create artificial variety.

The paper's hand-specimen figures carry 50 mm scale bars and its thin sections
carry 1 mm scale bars. Those are lateral-frequency evidence, not relief-depth
evidence.

## Relief proxy and transfer boundary

The missing relief evidence now comes from a different, explicitly named
authority: Raneri et al.'s digital multi-focus microscopy of fresh yellowish
coarse-grained Sabucina calcarenite. It is a comparable material-class proxy,
not a scan of Santa Marina, Naranjo quarry stone, or a medieval dressed face.

The study measured three 3 x 1.5 mm spots at 100x magnification. Its fresh
reference surface reports:

- 0.10 mm arithmetical mean height, Sa;
- 0.13 mm root mean square height, Sq;
- -0.39 height skew, meaning the main body is interrupted by sparser deeper
  valleys;
- 3.16 height kurtosis;
- 0.55 mm maximum peak height, Sp;
- 0.58 mm maximum valley depth, Sv;
- 1.13 mm total height, Sz;
- 0.51 texture aspect ratio;
- 2.53 fractal dimension.

The study separates surface components with a 0.8 mm robust Gaussian filter.
Its roughness component carries 0.11 mm Sq; its longer-scale waviness carries
0.05055 mm Sq. The published 3D map shows irregular angular and elongated
grains rather than round cells.

The script therefore reconstructs two periodic bands:

1. sub-0.8 mm grain, fossil-boundary, silicate, and sparse pore-valley relief;
2. broader waviness above that cutoff.

It then calibrates the combined field to the measured fresh-reference range
and records the achieved Sa, Sq, skew, kurtosis, peak, valley, seam, and
metres-per-pixel values. No research-figure pixels enter the runtime texture.

The proxy authorizes only stone-body relief scale and distribution. It does
not authorize:

- whole-block face tilt or plane deformation;
- medieval tool-groove depth;
- arris rounding;
- mortar recession;
- erosion, chips, cracks, or other damage;
- a reconstruction-grade optical roughness value.

## Two-scale shader translation

A 4 m map and a 1 mm fossil cannot be the same sampling problem.

The material therefore has:

- a 4 m construction/macro coordinate for courses, block families, micritic
  clouding, sandy blooms, and sparse iron color;
- a 64 mm body coordinate for fossil fragments, silicate grains, and visible
  pore identity and 16-bit body height, plus an incommensurate 91 mm
  decorrelation sample of the same masks and relief.

At 1024 px the source body field samples 0.0625 mm per texel. Blender
reconstructs it with per-block phase, quarter-turn, and mirror variation, then
blends the 64 mm and 91 mm samples. Unreal must reproduce the same coordinate
operations rather than sampling one world-aligned micro tile across every
stone.

The first shader implementation exposed all of those bands at all viewing
distances. That was physically layered but visually undirected: microscopic
fossil and tooling evidence became speckle in close views, while the wall read
as one pale field at gameplay distance.

Version 6 therefore makes the visual hierarchy a runtime contract:

- twenty quantized values remain available inside every captured stone family,
  while each intact block uses a quiet four-to-six-step neighborhood rather
  than spanning the complete dark-to-light family;
- deterministic per-block selection changes the family base without assigning
  one flat color to an entire block;
- five authored graphic plane classes shift related values within each block;
- only selected plane boundaries receive the packed structural-ink lane;
- only selected high arrises receive the packed highlight lane;
- stone-body color, body-height, body-roughness, pore response, and finite
  tooling are fully active from the surface to 0.30 m and use a smooth
  distance fade to zero by 3.5 m.

This fade is not permission to delete the measured microstructure. It preserves
the inspection-distance evidence while preventing it from flattening the
large-value composition. The medium plane colors and selective linework remain
visible because they carry the stylized read; the micro bands disappear
because they carry material evidence that the screen cannot resolve.

All construction maps are sampled from stable object-space metres. New Boolean
or fracture faces must still be assigned explicit face-class attributes; a
stable coordinate does not turn triangle interpolation into semantic
continuity.

The 30–40% fossil observation becomes a calcareous identity population, not
black noise. Quartz, feldspar, and clay form a separate cooler silicate lane.
The 13% porosity value describes the rock body; only a small subset is exposed
as visible surface openings, so the pore-identity mask remains sparse.

## Color and PBR boundary

The 2024 Wikimedia Commons quality photograph of Santa Marina is sampled only
inside evenly sunlit stone faces. Inkblotter reduces the normalized samples to
relative color families. Source pixels are discarded, and photographed
sunlight, cast shadow, mortar, repairs, cracks, and eroded cavities never enter
Base Color.

The engine contract follows Unreal's dielectric PBR model:

- Metallic is 0;
- Specular remains the normal dielectric 0.5 value;
- Base Color contains no AO or photographed lighting;
- the stone-body AO lane is 1;
- roughness variation carries the optical response;
- the current roughness range is an authored neutral/grazing-light
  calibration, because the material study publishes no optical roughness or
  gloss measurement.

The four-metre construction height remains flat. Stone-body height and normal
now use the measured Sabucina proxy. Tool marks, arrises, mortar, damage, and
whole-face plane deformation remain flat because neither the Santa Marina
study nor the proxy measures those distinct construction and finishing lanes.

## Bond logic

Historic Environment Scotland describes ashlar as square, true units in
equal-height courses and a broken bond where the blocks above cover the
perpendicular joints below.

The generator therefore owns explicit course offsets and exact block lengths.
It rejects:

- a generic rectangular grid;
- stacked joints in adjacent courses;
- a tile seam that creates a full-height vertical joint;
- independent random placement without course closure;
- blocks that leave the surveyed dimensional envelope.

The present layout guarantees at least 300 mm between the closest
perpendicular joints in adjacent courses, including the vertical tile wrap.
The 300 mm gate is a project acceptance threshold, not a published historic
dimension.

## Joint logic

Historic England describes high-quality ashlar joints as narrow—sometimes only
a few millimetres—and commonly flush.

The former recipe used:

- 14 mm bed joints;
- 11 mm perpendicular joints;
- a universal 8 mm recess.

Those values created graphic grooves closer to a game-stone outline than
fitted ashlar.

The current intact pass uses a published 3 mm ashlar working width from the New
South Wales government mortar guide and zero recess. Three millimetres is
numeric technical guidance, not a measurement from one Córdoba elevation. A
building-specific measured joint must still replace it before the system is
declared reconstruction-grade.

Mortar is physical geometry in the proof. It is not a dark line baked across a
large wall slab.

## Block volume

Every visible stone is a closed six-face rectangular volume. The front faces
are aligned to one wall plane; the backs vary according to the recorded depth
field. Each mesh carries:

- physical block identity;
- cyclic source-design identity;
- course identity;
- face length;
- face height;
- depth;
- tool-spacing selection;
- tool-direction selection;
- material phase and variant;
- explicit zero values for damage and narrative overlays.

No bevel is applied. The selected sources establish square and true edges but
do not publish an intact arris radius.

The fixture represents one facing wythe. It does not infer rubble fill,
backing stone, ties, through-stones, or total wall thickness.

## Tooling frequency

Measured Caen-stone fragments show characteristically diagonal tool marks with
1–4 mm spacing. Some visible faces also carried closely set vertical claw
marks.

The current coherent tooling family is fine oblique straight-hammer layage.
Moulis's archaeological survey and experimental reconstruction add the
missing scale-bearing structure:

- straight-hammer edge / finite impact length: 60–94 mm;
- fine impact width: 1–2 mm;
- visible historical density: 720–2420 impacts/m², mean 1369 impacts/m²;
- selected spacing inside passes: 1–4 mm;
- direction: diagonal, with authored sign and angle changes by face;
- organization: parallel passes with slight fan variation and finite,
  imperfect ends;
- height/normal depth: exactly zero;
- mask/color/roughness identity: retained in a separate 256 mm field.

The depth stays zero because no quantitative groove-depth authority has been
established. Making the normal “look right” by choosing 0.5 or 1 mm would
repeat the same unsupported practice this pass is intended to stop.

The three packed channels are not generic noise variants. They represent 720,
1369, and 2420 visible impacts/m². The shader selects one channel per block,
rotates it by the block's explicit tooling angle, and changes phase before
sampling. The marks stop because each strike is a 60–94 mm capsule, not
because a random breakup texture happens to erase an infinite stripe.

## Resolution budget

For a four-metre surface:

| Atlas width | Metres per texel | Millimetres per texel |
|---:|---:|---:|
| 1024 | 0.00390625 | 3.90625 |
| 2048 | 0.001953125 | 1.953125 |
| 4096 | 0.0009765625 | 0.9765625 |
| 8192 | 0.00048828125 | 0.48828125 |

A profile needs multiple samples across its width. Consequently:

- 1 mm tool marks do not belong in a four-metre 4K atlas;
- 2–4 mm marks remain marginal at 4K;
- the implemented 256 mm detail field provides 0.25 mm/texel at 1024;
- metre-scaled shader evaluation uses that field as the reusable close-range
  route;
- gameplay-distance shading should suppress marks that cannot be sampled
  cleanly rather than aliasing them into stripes.

## Visual hierarchy

The intact core is evaluated in this order:

1. wall plane and silhouette;
2. course rhythm;
3. broken bond;
4. true block depth under grazing light;
5. narrow flush joints;
6. restrained block-to-block body color;
7. proxy-bounded stone-body relief at the distance that can resolve it;
8. tooling identity at the distance that can resolve it.

Damage, grime, stylized ink, and micro-noise are not permitted to precede that
hierarchy.

## Why the neutral proof still exists

The Blender build renders neutral limestone value variants before it assigns
the runtime surface material. Both proof families are retained.

This is intentional:

- painted joints cannot disguise missing stones;
- baked height cannot disguise zero block depth;
- edge ink cannot disguise a stacked bond;
- dramatic color cannot disguise a weak course rhythm;
- damage cannot manufacture construction detail.

After the neutral geometry passes, the separately named surface renders
evaluate the shader without invalidating that construction evidence.

## Required evidence before the next construction layers

### Corner bond

Needed:

- one named building or surveyed corner;
- quoin face length, face height, and bed depth;
- alternating-orientation or bonding sequence;
- relationship to the facing wythe and wall core.

### Openings and arches

Needed:

- opening span and wall thickness;
- jamb bond;
- voussoir count or wedge dimensions;
- ring depth;
- intrados/extrados radii;
- springing and skewback geometry;
- mortar dimensions through the arch depth.

### Santa Marina-specific face relief and arrises

Needed:

- scaled close-range Santa Marina or Naranjo capture or scan to replace the
  current material-class proxy;
- intact arris radius distribution;
- face-plane deviation;
- tool-groove width and depth;
- tool angle and border treatment.

### Wall core

Needed:

- total wall thickness;
- backing-wythe dimensions;
- rubble or fill grading;
- through-stone frequency and placement;
- tie/cramp details where historically appropriate.

## Completion rule

The intact ashlar core may only be called complete when:

- every geometry dimension has an authority or an explicit authored-status
  label;
- all forty source designs and all physical proof blocks are closed volumes;
- no adjacent course stacks perpendicular joints;
- mortar is physical and flush;
- measured proxy relief is isolated from unknown block-plane, arris, mortar,
  tool, and damage depth;
- close, grazing, neutral, and distance proofs agree;
- the same construction attributes survive export and Unreal reconstruction.

The current milestone satisfies the first six items in Blender. Export and
Unreal parity remain open.
