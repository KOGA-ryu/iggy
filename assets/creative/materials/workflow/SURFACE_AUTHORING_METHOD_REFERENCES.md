# Surface Authoring Method References

## Source boundary

This note translates four user-supplied workflow images into written methods.
It separates readable instructions from interpretation. Small or illegible
labels are not reconstructed from guesswork.

## Reference A: layered realism for metallic surfaces

Visible credit: Adrien Roose, “Add Realism to Metallic Surfaces,” Pro Tip
number 004.

### Readable method

The article demonstrates two late-stage surface treatments:

1. Create a fill layer that affects gloss while excluding unrelated channels.
2. Change the gloss blending mode to `Overlay`, or use `Vivid Light` for a
   stronger effect. The article notes that this primarily affects bright
   tones.
3. Paint the effect with a soft brush. A hard brush may be used only when its
   edge is softened with the smudge tool.
4. Smooth along the metal plane and adjust intensity conservatively.
5. For a second treatment, add a curvature generator and constrain its mask
   effect to cavities.
6. Add a dirt effect and set its blending mode to `Difference` to diversify
   the mask.
7. Add a blur filter to soften the mask.
8. Add a final paint layer in pass-through mode so black/white painting can
   remove or restore the generated effect; soften edits with smudge.

### Translation into our workflow

This is evidence for an ordered, editable mask stack, not evidence that
`Overlay`, `Vivid Light`, or `Difference` is physically correct in every
renderer. In a metal/roughness pipeline:

- translate legacy gloss edits into a declared roughness or coating response;
- preserve the generator result and the manual correction as separate masks;
- identify whether the effect represents oil, compacted scale, polish, dirt,
  or another physical layer;
- restrict cavity response to real or authorized relief;
- prevent manual cleanup from becoming an undocumented destructive paint
  pass;
- prove the effect under a moving or changed highlight, not only in base
  colour.

## Reference B: reusable directional brushing

Visible credit: Adrien Roose, “Brushing Effects Part 1,” Pro Tip number 012.

### Readable method

The article defines brushing as a directional surface treatment that creates
linear grain. Its demonstrated stack:

1. Begin with a directional brushing filter that supports arbitrary angular
   rotation.
2. Place a transform filter above the effect and disable unwanted tiling so
   the direction can be manipulated.
3. Add the first effect to a material through the normal channel.
4. Create an anchor point for the first effect.
5. Create a second fill/effect layer and compose it through a normal-detail
   blending mode.
6. Duplicate the effect, move the copy in the stack, and rotate it through a
   transform filter.
7. Reference the original anchor so the two directional contributions remain
   independently transformable while still combining.

The article calls the method experimental and warns that it may expose tiling.

### Translation into our workflow

- Brushing uses a component-local tangent frame, never an accidental global
  image direction.
- Direction angle, phase, physical line spacing, amplitude, and anisotropic
  response are separate controls.
- A second direction is a separate contribution with its own transform and
  mask; do not rotate normal-map RGB as though it were a colour image.
- Tangent normals combine through a valid normal-detail operation.
- The brushing stack requires cylinder/curved-surface proof, changed-angle
  proof, close/grazing proof, and a repetition proof.
- Tiling suppression may use bounded phase, incommensurate spans, finite
  brush families, or semantic breaks at component boundaries.

## Reference C: finite modular stone sculpt and bake

Visible credit: Fanny Vergne, “Using ZBrush for stylized texturing.”

### Visibly established workflow

- A finite floor layout is blocked from rectangular slabs of several sizes.
- Individual slab identities are separated with flat colours before sculpt.
- Slabs are prepared for ZBrush at comparable working resolution.
- Individual stone forms are sculpted rather than treating the floor as one
  undifferentiated noise plane.
- Several reusable slab variants and at least one carved accent are assembled
  into the final tile.
- The tile is baked into ambient-occlusion, normal, and height outputs.
- ZBrush renders and baked maps are layered during texturing.
- The finished texture is demonstrated on a floor surface.

The screenshot does not make every brush name, subdivision value, or export
setting legible. Those details remain unsupported.

### Translation into our workflow

1. Author the bond/layout and unit dimensions first.
2. Assign stable block IDs and local frames.
3. Define a finite slab vocabulary by size and construction role.
4. Sculpt broad silhouette and medium face planes per source slab.
5. Keep enough distinct slab variants to prevent one landmark from exposing
   the reuse.
6. Reassemble the finite units into a seam-safe tile or modular kit.
7. Decide which sculpted frequencies remain geometry and which are baked.
8. Bake normal, height, AO, and identity from the same evaluated source.
9. Build colour and roughness from block identity, material family, and
   sculpt-derived masks rather than from a global dirt field.
10. Prove the repeated assembly at 1x, 4x, and 16x and on an actual consumer.

## Reference D: construction-aware stone chipping

Visible title: “ZBrush Stone Chipping.” Visible credit: Frozan.

### Readable method

1. Start with a low-poly mesh that is predominantly quads and has roughly
   even polygon size so subdivision behaves predictably.
2. In ZBrush, divide several times with smoothing disabled to preserve hard
   boundaries, then enable smoothing for later divisions to establish a
   controlled bevel.
3. After sufficient subdivision, use the MalletFast brush with varied taps
   and short drags to create nonuniform impacts.
4. Use a Standard brush with a spray-like alpha; subtractive strokes create
   small pocks and additive strokes create small raised stone variation.
5. Use Flatten to calm the interior face.
6. Keep major chips and dense pocking away from the protected centre unless a
   specific impact cause justifies them. Exposed edges are the primary wear
   and chipping zones.

### Translation into our workflow

The tutorial supplies an event vocabulary:

- `tap`: compact impact bowl or small chip;
- `short_drag`: elongated arris loss;
- `subtractive_spray`: small pocks;
- `additive_spray`: small aggregate or stone lumps;
- `flatten`: protected rest and dressed-face recovery.

Scripts must represent those as separate families with bounded size,
orientation, count, and eligible semantics. Edge exposure is an input, not a
post-hoc excuse. Centre-face damage defaults low or zero unless impact,
fastener, traffic, or another source changes the placement contract.

## Required ordered method contract

Every new or materially revised `hero-master` or `reusable-family` profile
must add `workflow_contract.surface_method_contract` with:

- `version`;
- an ordered `effect_stack`;
- `geometry_to_map_transfer`;
- a `damage_placement` object, explicitly disabled when out of scope.

Every effect records:

- stable ID and order;
- physical role;
- operation or composition method;
- inputs and mask sources;
- coordinate frame and physical scale authority;
- outputs and live consumers;
- quiet/absence rule;
- isolated proof IDs;
- any legacy-workflow translation, such as gloss to roughness.

The contract distinguishes an authored method from an attractive screenshot
or a large but causally opaque node graph.

