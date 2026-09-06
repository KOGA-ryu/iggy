# Measured Construction Dossier

This dossier replaces visual guessing with a source ledger. It does not claim
that one historical building is the only valid design. It does require that one
coherent construction family be selected before a model or texture recipe is
called final.

The machine-readable evidence is in
[`reference_measurements_v1.json`](reference_measurements_v1.json). Every
number is labelled as surveyed, catalogued, laboratory-characterized,
project-measured, derived, or unsupported. A museum-object envelope is not
silently converted into a plank thickness. A portfolio image is not silently
converted into a metre value.

## The first audit result and correction

The original cathedral-stone recipe was not measurement-faithful high-quality
ashlar.

| Feature | Current recipe | Published evidence | Finding |
| --- | ---: | ---: | --- |
| Bed joint | 14 mm | high-quality ashlar: a few millimetres | too wide |
| Perpend joint | 11 mm | high-quality ashlar: a few millimetres | too wide |
| Universal mortar recess | 8 mm | intact high-quality ashlar commonly reads narrow and flush | wrong base state |
| Course height | 360–640 mm | surveyed Cordoba family: 320–430 mm, commonly 400–410 mm | upper courses outside family |
| Block length | 480–1240 mm | surveyed Cordoba family: 780–1130 mm, commonly 1000–1100 mm | short and long outliers outside family |
| Tooling | recipe-defined patterns | measured Caen-stone marks spaced 1–4 mm | recipe needs physical spacing |

This does not mean every cathedral used the Cordoba dimensions. It means the
recipe cannot cite tight historic ashlar while using dimensions that belong to
neither its cited family nor the Historic England joint description. The
correct response is to choose a named construction family, not average several
buildings into generic fantasy stone.

The intact `cathedral_stone_v1` construction core has now been replaced with a
bounded Córdoba-family translation:

- forty cyclic source designs and sixty-nine physical six-face blocks;
- 0.80–1.12 m face lengths;
- 0.38–0.41 m face heights;
- 0.17–0.30 m block depths;
- 3 mm flush joints from published numeric ashlar technical guidance;
- broken bond with at least 300 mm separation between adjacent-course joints;
- no invented arris radius or tool-groove depth.

The exact course arrangement and 300 mm separation gate remain labelled as
authored translations rather than surveyed dimensions. The 3 mm joint is a
published technical working value, but it is not misrepresented as a
building-specific Córdoba measurement.

The intact surface is now a second, separately proved capability. It selects
the Santa Marina/Naranjo biocalcarenite rather than generic limestone:

- 13% quarry-sample porosity;
- 30–40% fossil components;
- a non-degraded Naranjo composition of 80% calcite, 15% quartz, 3% feldspar,
  and 2% clay;
- a micritic matrix, sandy silicate population, fragmented fossil population,
  sparse visible pore identity, and sparse iron-oxyhydroxide color;
- a 4 m macro coordinate and independent 64 mm/91 mm material-body samples;
- a separate 16-bit stone-body height reconstructed from published
  multi-focus metrology of a comparable yellow coarse Sabucina calcarenite:
  0.10 mm Sa, 0.13 mm Sq, and 1.13 mm total height across a 3 x 1.5 mm scan;
- flat construction-plane, tooling, arris, mortar, and damage depth because
  those distinct lanes remain unmeasured.

The relief transfer is explicitly a comparable-material proxy, not a Santa
Marina/Naranjo or medieval dressed-face scan. Roughness remains an authored
neutral/grazing-light calibration. Damage, fracture, ornament, and
`cathedral_stone_trim_fracture_v1` remain later capabilities.

## Stone: what one block actually contains

### Primary mass

For the measured early-Gothic Cordoba family:

- face length is commonly 1.00–1.10 m, with a measured 0.78–1.13 m range;
- face height is commonly 0.40–0.41 m, with a measured 0.32–0.43 m range;
- depth is commonly 0.20–0.21 m, with a measured 0.17–0.30 m range.

Those three dimensions describe a volume, not a painted rectangle. Any exposed
return, reveal, arch, break, or missing neighbour must reveal believable stone
depth. A flat card with a block-shaped normal map is not a substitute when the
silhouette or contact is visible.

Another measured dataset prevents us from turning the Cordoba family into a
false universal rule. A study of 4,316 ashlar observations in historic
limestone buildings around Álava and Treviño reports a mean face height of
264 mm with a 111 mm standard deviation, and a mean face length of 527 mm with
a 284 mm standard deviation. The same study reports mean lengths of about
420 mm at Armentia and Treviño and 530 mm at San Vicentejo, and relates the
difference to quarry distance.

That is not permission to sample width and height independently from a bell
curve. It is evidence that block population changes with building, quarry,
phase, and logistics. The script must select a surveyed site population and
preserve its relationships.

### Contact and bond

The geometry/material contract needs separate information for:

- the dressed face;
- top and bottom bed faces;
- left and right perpend faces;
- the hidden tail or depth of the block;
- mortar occupancy;
- the actual surface of a flush joint;
- later joint loss or repointing.

The intact joint and the aged joint are different states. An 8 mm dark trench
under every block makes the base construction false before weathering begins.
Weathered-back mortar belongs in a phase or condition mask, not in the
definition of ashlar.

### Tooled face

The excavated Caen-stone assemblage records medieval diagonal tooling with
marks 1–4 mm apart. Some visible faces also carried closely set vertical claw
marks. That requires:

- a physical millimetre coordinate;
- direction owned per block or banker-mason pass;
- pressure and missed strokes inside the selected tool family;
- no centimetre-scale noise pretending to be chisel work;
- no identical stroke phase continuing across a mortar joint.

At normal game distance these marks belong mainly in normal and roughness. A
hero subset may use displacement only after groove depth is measured. The
current intact core preserves the 1–4 mm spacing evidence as identity and
direction data but emits zero tool height.

### Stone body

The selected Córdoba body is not hypothetical oolitic limestone. The Santa
Marina study identifies Tortonian biocalcarenite and biomicrite with carbonated
micritic cement, abundant fragmented foraminifera, algae, bryozoans, bivalves,
and echinoid material. Its hand-specimen figures use 50 mm scale bars and its
thin sections use 1 mm scale bars.

The script therefore keeps three non-interchangeable sampling populations:

1. the 4 m construction/macro field for block color, micritic clouding, sandy
   blooms, and sparse iron color;
2. the 64 mm body field for microfossils and silicate grains;
3. sparse millimetre-to-centimetre fossil sections and visible pores read from
   the scaled hand-specimen figures.

Each block phases, quarter-turns, and mirrors the body field independently,
then blends 64 mm and incommensurate 91 mm samples. The body texture cannot
continue unchanged across joints or fall into a visible 64 mm metronome inside
one long block.

The reported 30–40% fossil fraction is represented as calcareous inclusions,
not black speckle. The 13% porosity value describes the rock volume, not the
percentage of visible black surface holes. Visible pore identity is therefore
much sparser and has no invented height.

The older oolitic evidence remains useful only for a different explicit stone
family. The British Geological Survey defines ooids as coated grains below
2 mm, and published Portland-limestone material describes shell fragments
around 5 mm. Those values cannot be mixed into Santa Marina as additional
“detail.”

A single noise texture still cannot represent these populations. Mineral
identity, feature scale, block phase, and optical response remain separate
decisions.

### Repairs and breaks

The Wells Cathedral survey recorded 48 medieval repairs: 39 fitted inserts and
9 joined irregular breaks. The smallest recorded socket face was 25 × 12 mm;
the largest insert was 382 × 51 mm; most repaired breaks were no more than
63 mm long.

The implications are concrete:

- repair geometry is sparse;
- it clusters where expensive moulded work was worth saving;
- a fitted insert owns a boundary, a bedding material, and a face finish;
- a joined break is not the same shape as a rectangular insert;
- repair scale ranges from a small socket to a long thin replacement;
- the repair is attached to a specific construction mistake or loss.

The current 180–850 mm fracture cells, 1.1–2.7 mm lifted rims, and 2.6–4.2 mm
cavities are authored numbers, not surveyed facts. They stay marked
unsupported until a scaled fracture survey is selected.

### Moulding and ornament

Historic Environment Scotland catalogues:

- a 1266–1300 Elgin Cathedral arch-moulding fragment with an overall
  229 × 398 × 274 mm envelope;
- a fifteenth-century Elgin jamb fragment with surviving dogtooth ornament and
  an overall 285 × 260 × 130 mm envelope.

These envelopes establish that the fragments are substantial pieces of stone.
They do not reveal the cross-section or ornament repeat. The current
260–280 mm bands, 83.3 mm dogtooth spacing, and 132 mm leaf spacing remain
unsupported because those values came from our recipe.

A valid moulding asset needs a scaled section for every:

- fillet;
- roll;
- hollow or cavetto;
- cyma;
- rebate;
- dogtooth body and undercut;
- leaf body, vein, return, and background recess;
- voussoir or straight-block joint through that profile.

Until those sections are measured, the stored SVG is a drawing exercise, not a
reconstruction.

## Brick: a separate construction family

Stone ashlar and fired brick cannot share one generic block generator.

Historic England's Waltham Abbey comparison material records medieval Great
Bricks:

- 290–380 mm long;
- 145–195 mm wide;
- 32–90 mm thick;
- the earlier thin group is 32–50 mm thick.

The Ightfield Hall survey records different hand-made brick populations:

- early foundation bricks: 235–240 × 100–105 × 55 mm;
- infill bricks: 230–235 × 110–115 × 65–70 mm.

The same survey records the geometry that a clean cuboid omits: irregular
shape, softened arrises, pebble and flint inclusions, rough faces, and shallow
drying creases. These are manufacturing facts, not a random damage pass.

### Audit of the current brick generator

The current `brick_masonry_v1` recipe is physically measurable:

- 1.6 m square tile;
- 20 course pitches from 74.21 to 86.18 mm;
- 7 brick pitches of 228.57 mm across the tile;
- rendered face lengths from 208.91 to 220.80 mm;
- rendered face heights from 58.48 to 74.29 mm;
- 11.5 mm bed joints and 10 mm perpends;
- authored corner radii from 1.5 to 4.5 mm;
- authored face crowns from 0.6 to 2.9 mm.

Those numbers overlap parts of several real brick populations, but they do not
describe one of the surveyed families in this dossier. If the Ightfield infill
brick is selected, seven 230–235 mm bricks plus seven 10 mm perpends require a
row pitch of roughly 1.68–1.715 m. The current 1.6 m tile cannot contain that
family at its stated count.

More importantly, the current asset only defines a visible face. It has no
brick depth, header face, frog state, cut-brick rule, arch special, or geometry
placement contract. Its corner radii and crowns are authored guesses. The
generator is therefore a face-pattern study, not a complete brick asset.

A brick recipe must therefore declare:

- period and brick family;
- working dimensions and measured variation;
- bond;
- bed and perpend joint;
- frog or no frog;
- moulded, cut, or rubbed special bricks;
- firing warp and arris character;
- inclusion and drying-mark populations;
- header and stretcher exposure;
- cut or broken interior.

## Wood: the member is not only a brown surface

### Two measured medieval doors

The Westminster Abbey vestibule door provides a detailed construction
authority:

- five vertical oak boards;
- board widths 225–390 mm;
- board thickness 40 mm;
- current door 1270 × 1980 mm;
- reconstructed original door about 1350 × 2490 mm;
- housed ledges 140 mm wide at their ends, bowing to 90 mm at the centre;
- a 40 mm chisel recorded in the ledge housing;
- rebated board edges, flush housings, pegs, saw marks, router marks, and
  cross-grain finishing evidence.

The Bremen Tower door in Tallinn supplies a different valid family:

- three vertical oak planks;
- door about 1600 mm high and exactly 807 mm wide in the published figure;
- board thickness 62–65 mm;
- dated AD 1400–1410.

Those sources show why “medieval plank door” is not one recipe. The board
count, widths, thickness, edge joint, ledges, covering, ironwork, and finish
must come from the chosen family.

### Measured floor construction

A National Trust survey of a late-medieval open-hall conversion records:

- boards commonly 229–254 mm wide and 25 mm thick;
- some boards up to 381 mm wide;
- joists 150 × 120 mm at 470 mm centres;
- another recorded joist group 171 × 127 mm at 445 mm centres;
- mortises 150 × 35 mm and 70 mm deep;
- chamfered lower edges and plain notched chamfer stops;
- wooden tongues inserted into opposing board-edge grooves in one area.

The Hampton Court report records an earlier worn floor at about
200 × 20 mm per board and later boards around 140 mm wide. Reused boards retain
doubled nail holes.

The generator needs to preserve those relationships. A wide early board, a
narrow replacement, a doubled nail hole, a tongue-and-groove edge, a butt
joint, and a housed ledge are separate construction events.

### Audit of the current giant-house door

The evaluated `SINC_DemoDoorAssembly` is:

- 2.168492 m wide;
- 0.410999 m deep overall;
- 4.8 m high;
- 61 disconnected evaluated mesh components.

Its three principal boards measure:

| Board | Length | Width | Thickness |
| --- | ---: | ---: | ---: |
| 1 | 4.800 m | 0.739415 m | 0.128749 m |
| 2 | 4.760 m | 0.715435 m | 0.127676 m |
| 3 | 4.763999 m | 0.680092 m | 0.131020 m |

Those values are actual project geometry. They are not a uniform enlargement
of the Met, Westminster, or Tallinn door.

The Met object used for colour reference is catalogued as
1740 × 1092 × 165 mm overall and 79.4 kg. Its 165 mm depth is the complete
assembled wood-and-iron object. Treating that number as plank thickness would
be a category error.

The mismatch is measurable. Relative to the Met object's full catalogue
envelope, the project assembly is 1.9858 times as wide, 2.4909 times as deep,
and 2.7586 times as high. Relative to the Tallinn door it is 2.6871 times as
wide and exactly 3 times as high, while its board thickness is only
1.9642–2.1132 times the Tallinn board thickness. Relative to the reconstructed
Pyx door, its board length is 1.9116–1.9277 times the door height while its
board thickness is 3.1919–3.2755 times the Pyx board thickness.

Those ratios do not prove the giant door is structurally wrong. They prove it
was not uniformly scaled from any one of the references currently named in the
package.

The giant door therefore needs an explicit adaptation sheet. It must state:

- the approved giant body scale;
- why the door height, width, and thickness do or do not scale uniformly;
- the structural source for board thickness;
- the structural source for board count and width;
- the source for ledges and braces;
- every housing, rebate, tongue, peg, hinge, pintle, nail, and clearance.

Without that sheet, the current dimensions are known but the design logic is
not.

## Iron members

Two catalogued medieval or late-medieval hinge examples establish real scale:

- British Museum strap hinge, circa 1418–1434: 172 × 35 mm;
- Met strap hinge, fifteenth–sixteenth century: 349 × 38 mm.

Length and width alone still do not establish plate thickness, taper, hole
diameter, barrel, pintle, nail, or mounting clearance. Those remain required
measurements for the selected hardware family.

## The required workflow from this point

### 1. Select one construction family

The family must name a place, period, material, and assembly. “BG3 cathedral”
is a visual target, not a measurement source. A valid scope would be:

> Early-Gothic oolitic-limestone ashlar using the measured Cordoba block
> family, with a separately measured English moulding family.

If two families are deliberately combined, the combination is an art-direction
decision and is labelled as such. It is not presented as reconstruction.

### 2. Build the measurement ledger before geometry

Every visible form gets:

- source ID;
- value or measured range;
- period and location;
- evidence class;
- confidence and known error;
- destination lane: silhouette geometry, supporting geometry, UV/mask,
  height/normal, roughness, or colour;
- LOD survival requirement;
- unresolved state when evidence is missing.

An unresolved field remains `unknown`. It is not filled with a plausible
number.

### 3. Account for every visible form

For an ashlar wall, the checklist includes:

- block volume, not face only;
- course sequence;
- bond and joint interruption;
- bed and perpend faces;
- mortar occupancy and profile;
- arris radius or intact sharpness;
- face tooling and its spacing;
- end or return face;
- mineral grains and inclusions;
- moulding profile and block joints through it;
- repairs;
- fracture interior and transition;
- condition state.

For a plank assembly, it includes:

- individual board length, width, and thickness;
- conversion orientation and grain axis;
- edge joint;
- end grain;
- saw, axe, shave, router, and abrasion evidence;
- ledge or brace section;
- housing;
- peg or nail;
- movement gap and shrinkage;
- later replacement or reuse evidence;
- hardware interface.

### 4. Separate measured construction from controlled imperfection

Variation is permitted only inside a named measured envelope or a documented
art-direction envelope. The script can select and place variants. It cannot
invent the basic anatomy with unrestricted noise.

### 5. Do not call a build complete because it executes

Completion requires:

- no unsupported dimension affecting the visible result;
- a reference-coverage ledger with every visible form accounted for;
- neutral clay proof for silhouette and construction;
- close material proof for millimetre-scale detail;
- gameplay-distance proof for value grouping;
- clean and condition-specific variants kept separate;
- direct comparison against the selected source and the BG3 quality target;
- explicit remaining risk.

The measured intact ashlar geometry now passes the volume, bond, neutral-proof,
and explicit-risk portion of that gate. It does not yet pass final surface,
corner, opening, arch, trim, export, or Unreal-parity acceptance. The door
colour package can remain an accepted colour study, but its giant-scale
construction still needs the adaptation ledger above.
