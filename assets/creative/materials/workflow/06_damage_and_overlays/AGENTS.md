# Stage 6: Damage, Wear, and Narrative Overlays

Damage is its own library shelf. It does not belong in the intact material
generator merely because an undamaged surface looks plain.

Read this file only when the current workstream explicitly includes one damage,
wear, repair, or narrative-overlay capability.

Every included damage capability requires a `DMG` record in
`CODED_DEMANDS.md`. Its test, motif, placement, geometry, shader, validation,
and proof code must appear directly beneath the demand before implementation.
When damage is excluded, the coded blueprint must still script the default-off
tests and shader validation.

## Preserve the intact master

- Keep the accepted intact material and geometry reproducible.
- Add damage through separate motifs, masks, geometry pieces, decals, vertex
  attributes, or overlay material functions.
- Default every optional lane to zero.
- Do not bake a universal "old" pass into a shared construction material.
- Do not use damage to conceal weak grain, stone form, metal response, rope
  construction, or geometry.

## Damage starts with cause and location

Every mark needs:

- material and component;
- physical cause;
- likely location;
- direction of force or exposure;
- size and depth envelope;
- age or repair state;
- geometry, shader, decal, or overlay representation;
- exclusion zones;
- proof that it does not repeat or float.

Examples:

- end checks start at exposed timber ends and narrow inward;
- housed joints compress bearing surfaces;
- doorway edges receive hand, shoulder, cargo, and latch contact;
- table edges receive different abrasion from floors or corridors;
- rope compression appears at knots, pulleys, clamps, and loaded contacts;
- iron polish appears at bearings, grips, latches, and repeated impacts;
- masonry fractures originate at stress, impact, settlement, or exposed ends;
- soot rises from a combustion source and responds to airflow and shielding;
- damp follows gravity, capillary paths, joints, ground contact, and drainage.

Random placement is not narrative variation.

For dressed or modular stone, default the protected face centre to quiet.
Concentrate large chips and short drag-like losses at exposed arrises, corners,
ends, joints, bearing changes, and documented impacts. Small pocks may occur
on a face only within a bounded density and cause. Use separate event families
for compact taps, elongated drags, subtractive pocks, additive aggregate, and
flattened/restored face planes.

## Damage grammar

Author separate typed families rather than one grunge mask.

### Loss

- chip;
- spall;
- broken arris;
- splinter;
- delamination;
- missing scale or coating;
- fracture interior.

Use geometry when silhouette, parallax, or deep shadow matters.

### Deformation

- dent;
- crush;
- compression;
- bend;
- rope flattening;
- peen or impact bowl.

The form should follow force and material behavior.

### Separation

- timber check;
- split;
- crack;
- mortar separation;
- fibre pullout.

Define origin, branching, taper, termination, and depth. A wandering black line
is not a crack system.

### Surface removal or polish

- scuff;
- scratch;
- abrasion;
- worn finish;
- exposed substrate;
- contact polish.

Change roughness, color, normal, and physical layer identity according to what
was removed or compacted.

### Deposit or chemical change

- rust;
- oxide;
- soot;
- dust;
- wax;
- damp;
- mineral deposit;
- dirt;
- biological growth.

Deposits need sources, accumulation logic, protected areas, and material
response. They are not universal color noise.

### Repair

- patch;
- peg;
- wedge;
- dutchman;
- strap;
- filled crack;
- replaced block or board;
- fresh versus old repair.

Repairs follow construction logic and receive their own age and material lanes.

## Finite authored library

Create a motif library with many distinct examples, including quiet variants:

- shape and silhouette;
- width, length, depth, and taper;
- orientation;
- cause and eligible surface;
- age;
- local frame;
- entry and exit falloff;
- reuse transformations;
- prohibited transformations.

A small high-quality seed shape may produce a family through mirrors, bounded
scales, shears, rotations, clipping, or recombination. Verify that transformed
members remain physically credible and do not expose obvious repetition.

Do not scatter one crack, dent, or scuff shape at random.

## Placement semantics

Use explicit geometry or scene information:

- distance to exposed end;
- edge class and arris exposure;
- load or bearing zone;
- contact and traffic lane;
- height above floor;
- doorway, table, hallway, stair, or work-surface identity;
- gravity and rain direction;
- heat or soot source;
- joint, fastener, knot, or material transition;
- fracture face versus weathered outer face.

If the runtime geometry cannot supply the placement semantics, stop and add the
required attribute contract rather than baking a contextless damage tile.

When damage is included,
`workflow_contract.surface_method_contract.damage_placement` must name:

- eligible semantic regions;
- prohibited or protected regions;
- event families;
- count or density bounds;
- size/depth/orientation bounds;
- force or exposure direction;
- quiet-area rule;
- proof IDs, including a wrong-location negative proof.

## Channel ownership

Damage may affect:

- geometry or signed height;
- normal;
- base color;
- roughness;
- AO for real recesses;
- metalness when a coating exposes a conductor;
- opacity for decals where appropriate;
- semantic masks such as cavity, fresh interior, old repair, soot, damp, or
  contact polish.

Keep each typed mask separately addressable. Do not collapse fresh and old
fracture, cavity and stain, or exposed iron and rust into one grayscale lane.

## Density and rest

- Most reusable base assets should retain substantial intact area.
- Concentrate marks where use or exposure predicts them.
- Vary count, size, clustering, and orientation.
- Allow rare large events and many quiet pieces.
- Avoid an equal-probability mark field.
- Review the damage at asset scale, not only as a cropped decal.

## Damage proof

For the selected damage capability, prove:

- intact source beside damaged variant;
- cause and eligible placement visualization;
- geometry or signed-height isolation;
- color, roughness, and physical layer changes;
- at least four visibly distinct motifs;
- reuse transforms without obvious repetition;
- a wrong-location negative test;
- close, grazing, and gameplay-distance views;
- representative asset context;
- default-off behavior in the base material.

## Damage exit gate

Accept one damage capability only when:

- it has a physical cause and placement contract;
- motif dimensions are measured or explicitly authored;
- deep loss is geometry where needed;
- typed channels remain independent;
- the intact source is unchanged;
- quiet area survives;
- contextual placement is demonstrated;
- the material does not invent damage on ineligible faces;
- the overlay is optional in Blender and the target-engine contract.
