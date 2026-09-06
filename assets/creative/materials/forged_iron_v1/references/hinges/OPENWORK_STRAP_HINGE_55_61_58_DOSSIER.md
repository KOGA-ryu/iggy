# Met 55.61.58 Openwork Strap Hinge — Pre-Geometry Dossier

## Decision

Do not improve the existing GH-018 leaf by adding more detail to its sine
ribbons. Its underlying construction read is wrong.

The replacement must begin as a connected openwork plate with five botanical
cells, explicit negative apertures, distinct end transitions, alternating
knuckle barrels, and a separate pintle. The line work must come from controlled
changes in web width, bevel width, bevel rise, crown, and face level. Material,
roughness, rust, damage, and dramatic lighting are excluded from this gate.

This dossier is deliberately upstream of Blender generation. It exists so the
construction can be judged before a script turns a wrong interpretation into
thousands of plausible-looking polygons.

## Source hierarchy

### Catalogue authority

The primary object is
[The Met 55.61.58](https://www.metmuseum.org/art/collection/search/475521), a
European iron strap hinge dated to the fifteenth–sixteenth century. The
catalogued overall dimensions are `40.6 × 4.1 cm`.

Those dimensions describe the complete catalogued object shown in the
additional photograph: a short fixed leaf, alternating hinge barrels, and a
long decorated moving leaf. They do **not** describe only the decorated moving
leaf.

### Primary image

`met_55_61_58_primary_original.jpg` is the colour installed view. It is the
authority for:

- broad face and bevel highlights;
- two continuous side rails;
- connected ornament;
- pad, boss, and pointed-terminal silhouettes;
- the difference between iron, exposed wood, cast shadow, and dark recess.

It is not a dimensional side view. It cannot establish stock thickness, rear
relief, a bevel angle, or the exact manufacturing method.

### Additional image

`met_55_61_58_additional_original.jpg` shows the complete paired objects
55.61.57 and 55.61.58 against a light ground. It resolves anatomy that the
installed crop obscures:

- the pivot is at the short plain leaf;
- the pivot uses alternating short knuckle barrels;
- the decorated moving leaf occupies only part of the catalogued length;
- the pattern contains five full cells;
- the pattern is a connected openwork web;
- the pointed pierced diamond is the tail terminal, not the rolled eye;
- the cell pattern has several types of aperture, not one repeated lens.

The photograph is low-resolution and is not an orthographic survey. Its
proportions are therefore recorded as ranges, never as false millimetre
precision.

## Correct object anatomy

The complete reusable hinge system has four owners:

1. **Fixed leaf** — short plain tapered plate with two attachment holes.
2. **Moving leaf** — long framed openwork plate with reinforced end zones.
3. **Pintle** — independent axial pin.
4. **Fasteners** — reusable nails or rivets seated through true holes.

The moving leaf itself has these semantic zones, from pivot to tail:

1. alternating moving-leaf knuckle segments;
2. pivot-side reinforced pad;
3. first authored half-cell transition;
4. two continuous structural side rails;
5. five full botanical cells;
6. four inter-cell X junctions;
7. last authored half-cell transition;
8. tail-side reinforced pad;
9. two rail extensions;
10. a pointed diamond terminal with a true bore.

The two end transitions are not cropped middle cells. They terminate load paths,
receive fasteners, and negotiate different geometry. Each needs its own
authored outline.

## Scale correction

The current GH-018 recipe assigns `4.06 m` to the moving leaf, because it took
the Met's `0.406 m` overall measurement and multiplied it by the giant-house
scale of ten. That is the first hard error.

The additional image gives the following honest starting ranges:

| Quantity | Real object | Giant-house scale | Status |
|---|---:|---:|---|
| complete hinge length | `0.406 m` | `4.06 m` | museum catalogued |
| complete hinge width | `0.041 m` | `0.41 m` | museum catalogued |
| long moving leaf | `0.30–0.33 m` | `3.00–3.30 m` | image-derived |
| decorative field | `0.175–0.195 m` | `1.75–1.95 m` | image-derived |
| full cell pitch | `0.035–0.039 m` | `0.35–0.39 m` | image-derived |

The range is intentional. The clay proof must be registered over the museum
image before one value is frozen.

## What the pattern actually is

### It is not a wave

The installed photograph can superficially read as two wavy lines because
light catches the beveled web and the door is visible through the holes. The
additional light-ground photograph separates positive iron from negative
space. It shows a five-bay botanical grammar.

Each full cell contains three principal apertures:

- one narrow central lancet;
- one curved flank leaf on the left;
- one curved flank leaf on the right.

The iron left around them forms:

- an outer pointed leaf cage;
- an axial bridge;
- an upper node;
- a lower node.

Each junction between cells contains a second grammar:

- four diagonal petal apertures;
- two small rail-side lunes;
- an X-shaped positive web;
- a short central bridge;
- two rail ties.

The junction has material area. It is not the mathematical intersection of two
zero-intent curves.

### Negative space is a first-class target

The shape of the white ground in the paired photograph is as important as the
black iron. The script must validate both:

- central lancets stay narrow and pointed;
- flank leaves have a broad belly and a faster return at the tip;
- diagonal petals aim into the X junction without touching;
- rail lunes leave quiet, consistent breathing room next to the frame;
- no aperture closes after beveling;
- no two bevel shoulders overlap.

If the positive web looks plausible but these openings do not match the
reference rhythm, the result fails.

## Minimal authored shape library

The entire repeated field begins from one hand-authored cubic Bézier quarter:

`Q0_leaf_quarter`

Its four control points in normalized local space are:

```text
tip            P0 = (0.00, 0.00)
departure      P1 = (0.14, 0.00)
shoulder pull  P2 = (0.34, 0.76)
belly          P3 = (0.50, 1.00)
```

The curve is mirrored about the longitudinal belly and the aperture
centerline. That produces a complete closed leaf aperture without separately
drawing its other three quarters.

Five named shapes then derive from that one authored quarter:

| Shape | Transform from Q0 | Use |
|---|---|---|
| `S1_central_lancet_aperture` | two mirrors, narrow cross scale | one central aperture per cell |
| `S2_flank_leaf_aperture` | mirrors, `18°` rotation, light shear | paired interior leaves |
| `S3_junction_petal_aperture` | mirrors, `32°` rotation, stronger shear | four petals per junction |
| `S4_rail_lune_aperture` | mirrors, shallow rotation, clip at rail boundary | two small side gaps per junction |
| `S5_terminal_diamond` | mirrors, sharpened longitudinal tips | tail terminal and separate true bore |

These numbers are explicit first-pass intent, not an algorithm claiming to
have rediscovered the artefact. They are easy to adjust against an overlay. A
generic wave formula is not.

### Why this reuse is legitimate

Reuse happens at the level of a controlled visual grammar:

- the same pointed departure;
- the same shoulder acceleration;
- the same broad belly;
- the same fast return;
- different scale, shear, rotation, and clipping by role.

That makes the shapes related without making them identical. Once the nominal
grammar passes, bounded control-point offsets can give the five cells sibling
variation. Variation cannot begin before the nominal grammar exists.

## Section grammar: how the leaf receives line work

The current asset sweeps one eight-sided rectangle along each whole path. A
ribbon therefore has one width, one thickness, and one bevel forever. That
cannot produce the reference's botanical taper.

The new web has six named stations:

| Station | Width / nominal | Bevel / local width | Bevel rise / thickness | Crown / thickness | Flat face |
|---|---:|---:|---:|---:|---:|
| root node | `1.30` | `0.16` | `0.08` | `0.02` | `0.68` |
| throat | `0.88` | `0.22` | `0.10` | `0.03` | `0.56` |
| shoulder | `1.02` | `0.26` | `0.13` | `0.04` | `0.48` |
| belly | `0.84` | `0.30` | `0.17` | `0.05` | `0.40` |
| return | `0.94` | `0.25` | `0.13` | `0.04` | `0.50` |
| tip node | `1.24` | `0.18` | `0.08` | `0.02` | `0.64` |

The ratios encode a specific drawing:

- the root widens to carry load into a junction;
- the throat compresses quickly;
- the shoulder reopens before the bend;
- the belly becomes narrow in plan but spends more of that width on the bevel;
- the return regains a little face;
- the tip widens and flattens into the next node.

Under neutral light, two deliberate crease loops produce the sharp graphic
read:

1. the **bevel shoulder**, where the quiet face breaks toward an aperture;
2. the **aperture lip**, where the bevel ends and the true cut wall begins.

Their separation changes by station. A single global bevel modifier cannot
author this proportion.

### Rails are not leaf branches

The two structural rails stay visually quieter:

- `58–70%` broad flat land;
- bevel width only `12–18%` of rail width;
- bevel rise only `6–10%` of stock thickness;
- no botanical crown.

This creates hierarchy. The frame remains a calm load-bearing boundary while
the cell web supplies the sharper line rhythm.

### Honest depth limits

The museum record supplies no stock thickness. The first proof therefore uses
an authored `4–6 mm` real-object range, scaled to `40–60 mm` for the giant
house. That range is explicitly provisional.

The first rear face remains planar. The front receives only the bounded relief
ratios above. Alternating over-under crossings are disabled because neither
museum image proves a weave.

## Existing GH-018 comparison

The accepted baseline remains useful as evidence of what was tested, but not as
a geometric donor for the ornament.

Headless inspection of its saved `.blend` found:

- Blender `5.1.1`;
- eight GH-018 runtime objects;
- `3024` runtime vertices;
- `3120` runtime polygons;
- runtime union extent `4.20222 × 0.283323 × 0.41 m`;
- two rails, two sine ribbons, two generic pads, one neck, and one rolled eye;
- no live modifiers.

Its source recipe specifies:

- moving-leaf length `4.06 m`;
- five sine cycles over a `2.975845 m` ribbon field;
- constant ribbon half-width `0.021 m`;
- constant rail half-width `0.026 m`;
- constant thickness `0.036 m`;
- constant ribbon bevel `0.0038 m`;
- constant rail bevel `0.0045 m`;
- artificial crossing depth `±0.010 m`.

### Detailed verdict

| Reference requirement | Existing baseline | Verdict |
|---|---|---|
| complete hinge inside catalogued envelope | moving leaf alone uses catalogue length | reject |
| five botanical cells | about ten simple lens openings | reject |
| connected pierced web | two separate ribbons and two rails | reject |
| central/flank/junction/rail apertures | one aperture type | reject |
| botanical width taper | one ribbon width | reject |
| changing bevel and face level | one bevel section | reject |
| coplanar connected nodes | cosine over-under crossings | reject |
| alternating knuckle barrels | one full curl | reject |
| pointed pierced tail | eye/tail reading reversed | reject |
| distinct end transitions | two generic rectangular pads | reject |
| true holes | six true fastener holes | retain principle |
| separate reusable fasteners | proof fasteners remain separate | retain |
| named datums | pivot and tail datums exist | retain principle |
| applied runtime meshes | no live construction modifiers | retain |
| multiple proof views | front/rear/three-quarter/macro/mounted | retain |

This is why adding extra bevel segments or surface detail to the current model
would not close the quality gap. It would refine the wrong sentence.

## Planned script, before it is written

The next script has a narrow job:

1. create Q0 as an inspectable cubic Bézier quarter;
2. derive S1–S5 only through declared transforms;
3. lay out five cells and four junctions in a measured moving-leaf envelope;
4. author the pivot and tail transitions rather than crop a repeated bay;
5. make one connected 2D outer plate with true aperture loops;
6. create a separate bevel-shoulder loop and aperture-lip loop for every
   opening;
7. evaluate width and elevation from the named section stations;
8. make the rear plane and cut walls;
9. author alternating short knuckle barrels around one pivot datum;
10. apply construction transforms and leave no live modifiers;
11. validate topology and proportions;
12. stop and show the script before building or rendering.

The implementation can use Blender's curve and mesh APIs, but the recipe owns
the design. Blender's
[curve taper system](https://docs.blender.org/manual/en/3.0/modeling/curves/properties/geometry.html)
supports varying width along a curve, and its
[Bevel modifier](https://docs.blender.org/manual/en/latest/modeling/modifiers/generate/bevel.html)
supports weighted selection and custom profiles. Neither feature, by itself,
decides where a root, throat, shoulder, belly, return, or tip belongs. Those
decisions stay in this authored profile.

For the hero openwork plate, the planned compiler will build the two crease
loops directly rather than depend on one global bevel. That keeps the important
leaf proportions explicit and testable.

## Prohibited shortcuts

- no sine or wave formula used as the source pattern;
- no global random point jitter;
- no Noise or Voronoi displacement;
- no roughness used to imply missing relief;
- no rust, scratches, dings, or damage;
- no AI-generated image;
- no material pass before the clay geometry passes;
- no render before the user sees the script;
- no claim that an unmeasured thickness or forging method came from the
  museum.

## Acceptance order

### Gate 1 — Reference overlay

- five full cells register to the paired photograph;
- all six negative-space roles remain legible;
- the pivot and tail occupy the correct ends;
- the complete assembly fits the `4.06 × 0.41 m` giant envelope;
- the moving leaf no longer consumes the complete catalogue length.

### Gate 2 — Neutral clay

- the botanical leaf reads without material contrast;
- the bevel shoulder and aperture lip produce sharp continuous line work;
- station changes are visible without becoming inflated;
- junctions look forged and connected, not mathematically crossed;
- rails remain broad and quiet;
- the rear, aperture walls, terminal bore, and segmented knuckles are real.

### Gate 3 — Runtime

- closed manifold meshes;
- true through holes;
- stable metre coordinates;
- no unapplied construction modifiers;
- moving leaf, fixed leaf, pintle, and fasteners retain separate ownership.

Only after those gates pass does the accepted forged-iron shader attach. The
shader is not allowed to rescue the geometry.
