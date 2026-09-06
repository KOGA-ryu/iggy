# Structural oak timber v1

This material is the first reusable timber surface built specifically for the
approved `rough_hewn_timber_beam_v1` geometry. It is not a square wood tile, a
photograph, a hardwood-floor pattern, or a generic procedural noise graph.

The governing idea is simple but strict:

> Every visible face is a different cut through one timber-local growth
> volume.

One pith path and one annual-ring table drive the four longitudinal faces and
both transverse ends. A ring seen bending along a side therefore has an
identifiable continuation on either end. The end grain is not an unrelated
circle texture pasted onto the beam.

## What transferred from the rope work

The rope material established several useful authoring methods that are not
specific to rope:

1. **Describe physical hierarchy before marks.** Rope was strand, yarn, and
   fibre before it was color. This timber is growth volume, annual increment,
   earlywood/latewood, ray, vessel, knot response, and tool signature before it
   is linework.
2. **Use metres as the causal coordinate.** Longitudinal events live in timber
   metres, not an arbitrary square UV frequency. Resizing a beam changes the
   distance sampled; it does not silently change pore or ring scale.
3. **Author finite tracks.** Twenty-seven selected grain tracks have an
   explicit radius, physical width, start, length, strength, and palette role.
   They do not come from “noise scale.”
4. **Give tracks local events.** Tracks can bury, fade, or flatten over a
   bounded interval. This creates loss, reappearance, and local quietness
   without pretending every interruption is damage.
5. **Keep causal lanes separate.** Ring identity, selected track identity,
   knot influence, ray response, tool response, and quiet-field response are
   inspectable maps. A convincing combined image is not allowed to hide a
   broken underlying layer.
6. **Author a champion before variation.** This package owns one deliberately
   composed beam. Later variants may change pith phase, epoch sequence, and
   passage selection only inside bounded rules derived from this champion.

What did *not* transfer is equally important: there is no helix, strand
periodicity, yarn crown, candy-cane rhythm, or evenly repeated ridge family in
the wood.

## Physical model

The champion represents ring-porous white oak, `Quercus alba`.

- The pith is outside the rectangular cant. That is why the end grain presents
  broad off-centre arcs instead of a decorative bullseye.
- The pith has nine controlled longitudinal positions. It drifts by
  millimetres rather than following a perfectly straight cylinder.
- Sixty-four explicit annual increments form the champion sequence. Eight
  bounded epoch scales extend that sequence across the required cross-section.
- The compiled beam contains 832 annual increments, enough to cover the most
  distant corner of the 400 mm authoring square.
- Authored annual increments range from 0.29 to 2.06 mm before bounded epoch
  scaling. The represented range remains anchored to the observed 0.12 mm
  suppressed and 2 mm-plus vigorous extremes.
- Earlywood vessel diameter is centred at 0.154 mm with a 0.028 mm standard
  deviation.
- Latewood vessel diameter is centred at 0.015 mm with a 0.003 mm standard
  deviation. Individual latewood vessels are below the atlas footprint and
  contribute grouped density rather than enlarged dots.
- Rays use two size classes: explicit wide aggregate rays and a restrained
  narrow-ray family.

Features smaller than 0.85 texel are represented with area-preserving
coverage. They may receive a filtered response, but the generator never turns
a 0.154 mm vessel into a 2 mm decorative hole just to make it obvious.

## Color construction

Color is not “brown plus grain.”

The exact twenty-color palette already approved for
`structural_oak_door_v1` is loaded from that material's profile. It is not
copied into a second palette that can drift. Every face combines:

1. a persistent middle field;
2. annual-increment color movement;
3. one or more long warm, cool, ochre, charcoal, pale, grey, or umber
   passages;
4. shorter face-specific passages;
5. selected grain-track color;
6. knot-local darkening;
7. sparse ray flecks;
8. sparse broad-axe signature;
9. deliberate low-contrast quiet fields.

The result contains many adjacent shades inside one piece of wood, blended at
different physical scales. The four faces do not receive one flat shade each.
Their larger value groups differ because they are different cuts through the
cant and because the reference hierarchy uses broad, asymmetric color fields.

## Side-grain construction

Each side pixel reconstructs its timber-local `X/Y/Z` position, then evaluates:

1. interpolated pith position at longitudinal metre `X`;
2. elliptical radial distance from the pith;
3. longitudinal radius drift;
4. annual-ring index and within-ring phase;
5. earlywood-to-latewood phase;
6. selected finite grain tracks;
7. track burial, fade, and flatten events;
8. branch-knot coordinate deflection;
9. side-visible ray flecks;
10. selected tool-signature lines;
11. quiet-field attenuation.

The front live knot and top pin knot are read from the approved beam profile.
They do not receive pasted ovals. Each knot changes the coordinates used to
evaluate nearby growth, so the surrounding grain flows around the branch
intersection before the separate recessed knot geometry is shaded.

## End-grain construction

The left and right atlas tiles sample the same pith and annual-ring table at
`X=0.0 m` and `X=4.2 m`.

Their visible hierarchy is:

1. broad annual arcs;
2. earlywood/latewood grouping;
3. selected ring boundary emphasis;
4. explicit wide rays;
5. filtered narrow rays;
6. area-preserved earlywood vessels;
7. restrained end-face darkening.

The geometry owns seasoning checks. The material does not redraw them as black
cracks. This separation prevents doubled damage and keeps a clean material
usable on an unbroken beam.

## Tool signature

The geometry profile contains 57 measured hewing observations. Geometry uses
those observations to fit broad face planes, not to stamp 57 identical dents.

This material selects 17 of those observations for shallow linework and normal
response:

- five front;
- five back;
- three top;
- four bottom.

Their position, direction, finite length, and physical width come from the
beam's existing observation table. They remain surface signature, not cavities
or arbitrary damage.

## Generated lanes

### Side atlas

- `structural_oak_timber_v1_side_basecolor.png`
- `structural_oak_timber_v1_side_normal.png`
- `structural_oak_timber_v1_side_height.png`
- `structural_oak_timber_v1_side_identity.png`
  - R: annual-ring identity
  - G: selected grain tracks
  - B: knot influence
- `structural_oak_timber_v1_side_response.png`
  - R: side ray flecks
  - G: selected tool signature
  - B: quiet fields

Rows, from top to bottom, are front, back, top, and bottom.

### End atlas

- `structural_oak_timber_v1_end_basecolor.png`
- `structural_oak_timber_v1_end_normal.png`
- `structural_oak_timber_v1_end_height.png`
- `structural_oak_timber_v1_end_identity.png`
  - R: ring boundary
  - G: combined ray response
  - B: area-filtered earlywood vessels

Columns, from left to right, are left end and right end.

No roughness texture is generated. The Blender proof uses one uniform
roughness value of `0.76`; color, anatomy, and geometry must carry the read.

## Blender material

`build_structural_oak_timber_v1.py` opens the approved clay beam and creates
`IGGY_MAT_StructuralOakTimberV1`.

The shader:

- reads `IGGY_TimberUV` in physical metre form;
- reads `sinc_timber_face_id`;
- converts longitudinal metres to side-atlas U;
- routes front/back/top/bottom into explicit side rows;
- routes left/right ends into independent end columns;
- keeps the hewn arris class as an explicit quiet wood lane;
- mixes side and end base color;
- mixes side and end tangent normals;
- feeds a Principled dielectric with constant roughness;
- uses no AO bake in base color and no procedural noise node.

The face-ID route is intentional. It avoids a painted triangular mask that
would blur across Booleans or change when the beam is resized.

## Build

Generate the maps:

```sh
/Applications/Blender.app/Contents/Resources/5.1/python/bin/python3.13 \
  assets/creative/materials/structural_oak_timber_v1/generate_structural_oak_timber_v1.py
```

Apply them to the approved beam and render the review set:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  --background --factory-startup \
  --python \
  assets/creative/materials/structural_oak_timber_v1/build_structural_oak_timber_v1.py
```

## Explicit exclusions

- AI-generated imagery;
- photographic projection;
- random noise;
- a square repeating tile;
- one color per face;
- a uniform grain comb;
- roughness texture;
- damage texture;
- checks, chips, dings, scuffs, scratches, stains, soot, wetness, or grime;
- lighting baked into base color.

Those are not missing work hidden under the label “later.” They are different
material systems or scene overlays and must remain independently addressable.

