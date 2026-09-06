# Stage 2: Material Intent and Measurement

This stage turns research into a complete, testable design. The output is not
marketing prose. It is the causal specification for the pattern, maps, shader,
geometry attributes, proofs, and exclusions.

## Define the exact surface

Start with a bounded sentence:

`This capability represents [surface or layer] on [object/construction family]
at [physical scale and viewing range], before [excluded capabilities].`

Examples of useful boundaries:

- intact tangential and radial white-oak grain before finish, damage, dirt, or
  end-grain overlays;
- compact forge scale over conductive iron before rust, polish, soot, or
  scratches;
- regular-lay three-strand plant fibre before compression, wetness, and fray;
- intact fine-jointed ashlar before trim, fracture, lichen, damp, or repair.

Avoid boundaries such as "stylized wood" or "castle stone". They hide too many
independent decisions.

## Source-to-intent ledger

For every important observation, record four fields:

1. `Observation`: what the source actually establishes.
2. `Intent`: what the authored material must visibly communicate.
3. `Implementation`: which finite pattern, algorithm, attribute, map, or node
   realizes it.
4. `Not justified`: the tempting visual inventions that remain prohibited.

The ledger should make it possible to review a script branch without guessing
why it exists.

## Physical and semantic profile

The canonical profile should declare the relevant subset of:

- tile or longitudinal repeat in metres;
- element dimensions and variation envelopes;
- material-body detail spans;
- feature widths, lengths, density, direction, and depth;
- height ranges in metres;
- roughness or optical status and its evidence;
- metallic layer identities;
- coordinate source and orientation;
- seed, phase, family, variant, and identity attributes;
- cut, end, fracture, trim, repair, contact, and overlay attributes;
- output maps, channel packing, bit depth, and color space;
- default overlay values;
- Blender reconstruction rule;
- target-engine reconstruction rule;
- unknown and prohibited lanes.

Do not scatter these constants through generators, builders, and tests.

## Frequency ladder

Design the surface as independently reviewable bands.

Record these bands in `workflow_contract.layer_provenance`, not only in prose.
For every layer, name its physical meaning, sources, outputs, live consumers,
isolated proofs, rest rule, and supporting measurement-claim IDs. Use
`workflow/LAYERED_GRAPH_REFERENCE.md` as the minimum graph-reading discipline:
translate visible branch structure without inventing unreadable node names or
parameters.

For a new or materially revised hero/reusable material, also write
`workflow_contract.surface_method_contract`. Layer provenance answers what a
layer means; the surface-method contract answers exactly when and how it is
composed. Record order, operation, masks, coordinate frame, outputs,
consumers, quiet rule, and proofs.

### Construction and silhouette

Examples:

- board width, block bond, mortar body, rope lobes, hammer-forged strap
  thickness, plaster-to-stone transition;
- real joins, cavities, deep breaks, exposed ends, and silhouette fray.

### Macro

Examples:

- heartwood and sapwood passages;
- block-to-block warm and cool families;
- broad forge-scale tone fields;
- strand-to-strand fibre tendency;
- plaster application and cure fields.

The macro layer must read at thumbnail and gameplay distance.

### Medium

Examples:

- growth-ring sweeps, rays, and knot deflection;
- dressed face planes and finite tool passes;
- yarn bundles, contact valleys, and compression;
- hammer families and compact-scale plates;
- trowel movement and aggregate zones.

This band normally carries the material identity.

### Edge and event

Examples:

- arris changes, board shoulders, mortar contact, strand valleys, knot halos,
  scale lips, selected tool starts and stops.

Events are selective. Preserve untouched rests.

### Micro

Examples:

- pores, vessel response, stone grains, fine fibres, oxide skin, plaster
  particles.

Microdetail needs declared metres-per-texel and viewing-distance behavior. It
must not dominate the macro composition.

## Color architecture

Do not assign one color to one board, block, or metal piece.

Design color as a cumulative stack whose layers can be isolated:

1. base material family;
2. broad warm/cool or heartwood/sapwood passages;
3. construction- or anatomy-driven color;
4. medium directional structure;
5. selected event colors;
6. optional ink and highlights;
7. contextual overlays, disabled in the intact base.

Use enough related shades to produce blended movement inside one element. The
exact count is material-specific; ten to twenty controlled shades may be
appropriate where the captured family supports them, but a shade count is not
itself quality. Protect broad coherent values and avoid saturated caricature.

Every layer must state whether it changes hue, value, saturation, or some
combination. Base color may include intrinsic cavity material darkening but
not scene lighting or ambient occlusion.

## PBR causality

Specify each output independently.

### Height

- Use metres and a declared zero plane.
- Separate macro shape, medium relief, edge relief, and micro relief.
- Do not assign depth to a mark merely because the mark is dark.
- Deep features that need parallax or silhouettes belong to geometry.

### Normal

- Derive from authorized height or an explicitly authored directional normal
  contribution.
- Combine scale bands correctly; do not average normal RGB.
- Use OpenGL tangent-space orientation unless the package declares another
  target.

### Roughness

- Begin with the material's broad optical state.
- Modify it through finish, porosity, fibre direction, aggregate, scale,
  exposed substrate, contact, polish, or wetness.
- Filter high frequencies before adding intentional small variation.
- Do not invert height and call the result roughness.

### Ambient occlusion

- Use only for physically sheltered relationships represented by geometry or
  justified depth.
- Keep AO out of base color.
- Do not use AO to draw graphic outlines.

### Metalness

- Treat it as physical layer identity.
- Rust, forge scale, paint, wax, dirt, and stone are dielectric.
- Exposed iron or another conductor uses the conductor response.
- Blend physical responses by a semantic exposure mask.

## Coordinate contract

Choose coordinates based on construction:

- timber-local longitudinal and cross-section metres;
- stone-block-local coordinates;
- accumulated rope curve distance and circumference;
- connected-component iron coordinates;
- plaster wall metres plus explicit transition masks;
- dedicated end-grain or cut-face frames.

Declare:

- origin;
- axis meanings;
- physical repeat;
- scale behavior under object transforms;
- per-element phase and variant;
- how arbitrary length is handled;
- how Boolean, fracture, and newly generated faces receive semantic identity;
- how Blender and Unreal reproduce the same contract.

Do not rely on post-assembly bounding boxes when resizing would change apparent
feature scale. Do not infer a cut face from interpolated triangle values.

## Stylization contract

Decide which stylized marks belong to the asset:

- structure-selected ink;
- sparse arris or fibre highlights;
- brush direction;
- pigment blocks;
- protected rest or detail priority;
- optional gameplay tint.

State which marks remain visible at each distance and which fade. Keep them
separate from baked light. A stylization map that the live shader does not
consume is not implemented.

## Unknowns and exclusions

List every tempting unsupported feature:

- unknown bevel radius;
- unknown tool depth;
- unknown wall core;
- unmeasured scratch or pit size;
- speculative wear;
- inferred repairs;
- generic grime;
- engine parity not yet verified.

Default excluded lanes to zero. Tests should reject accidental activation.

## Intent exit gate

The design is ready for scripting when:

- the surface boundary fits in one sentence;
- every visible band has a material meaning and scale;
- color contains multiple controlled contributions inside one element;
- height, normal, roughness, AO, and metalness have different causal rules;
- coordinate and semantic attributes are explicit;
- stylization and damage are separate;
- Blender and target-engine reconstruction are described;
- unknowns cannot silently enter the champion.
- every visible layer has machine-auditable source-to-output provenance.
