# Stage 5: Blender Shader and Engine Integration

This stage turns authored maps and geometry semantics into a live reusable
material. Blender nodes are executable data flow. Treat their sockets,
coordinates, color spaces, attributes, and physical responses with the same
precision as code.

## Execute the coded demands

Read the package `CODED_DEMANDS.md` before editing. Apply the reviewed geometry
attribute, node-group, shader, validation, proof, and reopen-test code under
the owning demand.

The document must name every changed node, `bl_idname`, operation, non-default
socket, link, map, color space, custom property, and target-engine consumer.
Update and re-review the demand before deviating from that graph.

## Inspect the live route

Before editing:

- identify the actual geometry used for acceptance;
- inspect transforms, modifiers, Geometry Nodes, Booleans, UV maps, named
  attributes, face classes, and material slots;
- identify the material currently assigned after evaluation;
- distinguish source geometry from proof copies and exported meshes;
- find the canonical node group and all live consumers;
- reject an adapter that recalculates material policy differently from the
  profile.

Do not build a beautiful disconnected shader beside the material the asset
actually uses.

## Versioned node architecture

- Give the public material and reusable groups stable versioned names.
- Name nodes by semantic role, not by node type alone.
- Keep coordinate reconstruction, map decoding, physical layer response,
  normal combination, stylization, and overlays in understandable stages.
- Reuse a group when it represents one coherent function.
- Do not create a forest of tiny groups that hides data flow.
- Record the public inputs, defaults, physical units, and output meanings.

Tests should verify the nodes and links that carry the contract, not merely
that a material with the expected name exists.

## Coordinates and scale

Use the profile's declared coordinate source:

- local or object position in metres;
- authored UV map with documented metre scale;
- accumulated curve distance;
- connected-component frame;
- block, board, strand, yarn, or face identity;
- explicit phase and variant attributes.

Validate:

- object resizing does not change apparent feature size unless intentionally
  parameterized;
- arbitrary length does not stretch a square tile;
- Geometry Nodes evaluation does not erase required attributes;
- Boolean and fracture faces receive explicit semantic values;
- mirror, rotation, and phase variation remain deterministic;
- end grain or cut face can be addressed independently.

Do not infer semantic face identity from a generated-position gradient or
interpolated triangle mask.

## Texture data handling

- Base color images use sRGB interpretation.
- Normal, height, ORM, identity, direction, stylization, and overlay masks use
  Non-Color data.
- Decode signed direction fields explicitly.
- Use the manifest's metre range for height amplitude.
- Use the correct normal-map convention.
- Pack every required review image into the saved `.blend`.
- Keep texture interpolation and extension mode deliberate.

A packed image proves portability, not correctness. Validate the node socket
that consumes it.

## Combine relief by scale

Keep macro, medium, and micro normal contributions separate long enough to
control them.

- Geometry supplies structural relief when possible.
- Bump or normal maps supply bounded surface relief.
- Combine tangent normals with a correct method such as whiteout or reoriented
  normal blending; do not average RGB.
- Preserve a scalar control for each meaningful band.
- Do not let micro normal strength soften block, plank, rope, or hardware
  silhouettes.
- Do not author unmeasured depth merely to make a grazing proof dramatic.

Directional brushing is a tangent-frame operation. Expose direction angle,
phase, physical spacing, and amplitude independently. When two brushing
directions are composed, transform each contribution before valid
tangent-normal detail blending; never rotate or average normal RGB as colour.
Prove the result on curved geometry and under a changed highlight.

## Build physical material layers

Use separate physical responses when the material actually contains different
layers:

- dielectric oxide or forge scale over conductive iron;
- varnish, wax, paint, or dirt over wood;
- wet film over porous stone or plaster;
- exposed aggregate versus binder when the evidence supports it.

Blend layer responses by semantic masks. A single gray metallic shader with a
roughness map is not a forged-scale system.

## Roughness and reflection

- Start with broad material response.
- Apply medium finish, porosity, fibre, scale, or aggregate behavior.
- Add sparse event-specific changes.
- Keep close micro variation subordinate.
- Inspect with frontal, three-quarter, and grazing highlights.
- Ensure a flat strap still reads flat and a dressed stone still reads planar.

Roughness numbers are regression guards. The proof highlight decides whether
the material response reads.

Legacy gloss tutorials may use `Overlay`, `Vivid Light`, `Difference`, anchor
points, or pass-through paint layers. Translate each step into the current
physical pipeline: identify the represented layer, convert gloss intent to
roughness where needed, retain generator and manual-correction masks
separately, and document the exact operation. A legacy blend-mode name alone
is not material causality.

## Distance hierarchy

Declare which bands survive:

- distant: silhouette, construction, broad value groups, selected structural
  ink;
- gameplay: medium material structure, important edges, selected highlights;
- close: pores, fibres, tooling, aggregate, scale lips;
- microscope or hero inspection: full measured micro relief.

Use explicit distance or footprint controls when needed. Fade color,
roughness, height, normal, and tooling contributions consistently enough that
one band does not remain as color speckle after its relief disappears.

Do not solve distance noise by removing the medium structure that carries the
material read.

## Stylization lanes must be live

Wire the authored lanes into controllable material behavior:

- structural ink changes a bounded optical color or material response;
- highlight strokes affect selected values, not every upward-facing pixel;
- brush or fibre direction can control anisotropic or painterly response;
- detail priority protects quiet regions;
- traversal tint remains optional and semantic.

Validate that the final shader output actually consumes these lanes. Dormant
textures and disconnected nodes do not count.

Do not bake a fixed light direction into the material. Renderer cel ramps and
screen outlines remain separate from material-specific line selection.

## Blender-to-engine boundary

Document the translation as operations, not as "export the Blender shader":

- coordinate reconstruction;
- identity and variant inputs;
- texture sampling and channel decode;
- height amplitude;
- normal combination;
- physical material-layer blending;
- stylization controls;
- distance behavior;
- overlay inputs.

Name the intended Unreal material functions or equivalent runtime units.
Blender proof is not Unreal parity. Keep parity false until the target engine
renders the same material on representative geometry and passes a comparison.

## Headless build and save

The canonical builder should:

1. load the profile and generated manifest;
2. construct or reopen the exact proof geometry;
3. build the versioned node groups;
4. assign semantic attributes and materials;
5. render neutral geometry before the shader can hide it;
6. render the live material under required views and lights;
7. validate nodes, links, attributes, image packing, defaults, and exclusions;
8. save the `.blend`;
9. hash the saved file and emit a Blender manifest.

After saving, reopen the file in a separate headless process and run the asset
tests. Do not rely only on in-memory state from the builder.

## Integration exit gate

The shader is ready for proof review when:

- it is assigned to the actual acceptance geometry;
- coordinates are stable and scale-bearing;
- semantic face and component lanes survive evaluation;
- physical layer responses are causal;
- normal bands are combined correctly;
- authored stylization lanes are live;
- distance hierarchy is explicit;
- required images are packed;
- unsupported overlays default off;
- the saved file reopens and passes focused node and geometry tests;
- target-engine parity is reported honestly.
