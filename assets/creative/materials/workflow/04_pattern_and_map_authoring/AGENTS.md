# Stage 4: Pattern, Layer, and Map Authoring

This stage compiles the material intent into deterministic, editable texture
data. Procedural generation is a production method, not permission to replace
observation with randomness.

## Execute the coded demands

Read the package `CODED_DEMANDS.md` before editing. Apply the reviewed pattern,
profile, generator, map-writing, validation, and generator-test code under the
owning demand.

If current source context reveals that the documented code is incomplete or
incorrect, update the demand and place the revised complete code beneath it
before applying the change. Do not repair live code through an undocumented
alternate path.

## Author a finite vocabulary first

Identify the reusable authored units before writing the compiler:

- board or block layouts;
- ring-width epochs;
- knots, rays, pores, and fibre tracks;
- rope strand and yarn paths;
- tool passes and finite impacts;
- forge-scale plates and hammer families;
- plaster strokes and aggregate zones;
- crack, chip, repair, or overlay motifs when those later capabilities are in
  scope.

Store meaningful parameters, not final pixels, where possible. A finite motif
may be mirrored, rotated, stretched within a measured envelope, clipped, or
combined to create variants. The transformation must be declared and preserve
the motif's construction logic.

Do not begin from undirected global noise and search for meaning after the
fact.

## Separate authoring from compilation

The authored pattern owns:

- identities;
- dimensions;
- local positions and directions;
- family and variant selection;
- event start and end;
- rest regions;
- transformation limits.

The compiler owns:

- rasterization;
- filtering and band limiting;
- layer combination;
- map encoding;
- proof extraction;
- metrics;
- deterministic output.

Do not bury the entire artistic vocabulary as anonymous constants inside one
long pixel loop.

## Stable local frames

For every physical element:

- derive or load its local coordinate frame;
- evaluate directional patterns in that frame;
- store identity, phase, and variant explicitly;
- keep a deterministic relationship between pattern and geometry;
- prevent one global cloud or stripe field from crossing unrelated elements.

Use incommensurate repeat spans, bounded rotations, mirrors, or phase offsets
where they suppress repetition without changing the material's average
physical scale.

## Build broad to fine

Compile and save isolated stages:

1. pattern or construction mask;
2. macro value or volume;
3. medium structure;
4. edge and selected events;
5. microstructure;
6. cumulative physical maps;
7. stylization lanes;
8. optional overlays.

If the final map looks wrong, the isolated outputs must reveal which band
caused the failure.

The saved layer order must agree with
`workflow_contract.layer_provenance`. A layer is not implemented merely
because a graph branch exists: it must produce a named output, reach a named
consumer, retain an isolated proof, and obey its rest rule.

The executed compiler order must also match
`workflow_contract.surface_method_contract.effect_stack`. Preserve generated
masks and manual corrections as separate deterministic inputs. Do not flatten
a curvature mask, dirt breakup, blur, and hand correction into one anonymous
bitmap.

For sculpt-to-texture workflows, record
`geometry_to_map_transfer`: source mesh, retained silhouette/medium forms,
baked height/normal/AO/identity outputs, cage or projection boundary, and
proof. The bake is a frequency transfer, not permission to discard source
geometry logic.

## Pattern variation

Variation should change composition, not material identity.

Good variation includes:

- course or plank sequence;
- family selection;
- measured ring-width epochs;
- knot or tool-event placement from a finite library;
- local phase;
- bounded width, length, pressure, or density;
- rare quiet or focal pieces;
- compatible warm and cool arrangements.

Bad variation includes:

- independent random values for every pixel;
- random rotations that violate grain or gravity;
- changing real feature scale with texture resolution;
- identical damage probability everywhere;
- unbounded noise amplitude;
- seeds that alter acceptance measurements.

Champion output must be reproducible from recorded seed, pattern revision, and
profile.

## Preserve areas of rest

Define a rest or detail-priority field before adding medium and micro layers.
Use it to keep some surfaces quiet. Ensure focal marks do not cover every
element.

Record coverage statistics for structural marks, but judge their spatial
composition visually. Five percent of a surface can still look uniform if the
five percent is evenly sprayed.

## Filtering and screen behavior

- Evaluate widths in metres before converting to pixels.
- Band-limit features below the current texel footprint.
- Preserve area or integrated response for important subpixel structures.
- Prevent diagonal fibres, pores, or tool marks from becoming dotted
  one-pixel chains.
- Generate normals at the declared physical span.
- Test representative downsample and mip behavior.
- Fade or suppress unresolved microdetail through the shader rather than
  letting it become distance noise.

## Color compilation

Compose color from named masks and palettes:

- convert sRGB evidence to linear before physical blending;
- preserve the accepted palette family;
- create several related values inside one physical element;
- blend broad passages before narrow structure;
- use material events to alter color selectively;
- keep photographed shadow, highlight, and AO out;
- avoid orange saturation, black cavities, and uniform gray as shortcuts.

Save cumulative color stages so a reviewer can see what each layer contributes.

## Physical map compilation

### Base color

- intrinsic material color only;
- no scene light;
- no AO;
- no fake shadow under every edge;
- no universal grime.

### Height

- 16-bit for scale-bearing relief unless a package proves a different format;
- explicit metre minimum, maximum, and zero;
- separate signed masks when later geometry or decals need positive and
  negative interpretation.

### Normal

- generated at the correct metres-per-pixel;
- explicit OpenGL orientation;
- seam tested;
- no unsupported silhouette claim.

### ORM or equivalent

- document channel meanings;
- keep AO, roughness, and metalness independently inspectable;
- retain binary physical layer identity where required;
- do not treat packing as the only source copy.

### Semantic and stylization masks

Pack only compatible data:

- identity and topology lanes;
- structure-selected ink;
- highlight strokes;
- brush or fibre direction;
- detail priority;
- overlay eligibility.

Record each channel in the profile and manifest.

## Deterministic tests

Test the relevant invariants before the final render:

- repeat closure and edge seams;
- pattern dimensions and element counts;
- measured ranges;
- per-element identity and local-frame coverage;
- bounded densities and amplitudes;
- quiet-region survival;
- expected shade or palette use;
- non-identical variants with stable champion seed;
- finite values and legal ranges;
- exact output dimensions and bit depths;
- correct packed channels;
- source and profile hashes in the manifest;
- prohibited damage or overlay lanes remain absent.

Do not freeze aesthetically arbitrary pixel hashes unless byte identity is
itself the contract.

## Common failure diagnosis

- If the result looks like laminate, replace the grain anatomy rather than
  adding scratches.
- If stone looks like a patio generator, replace the construction pattern and
  edge families rather than adding noise.
- If rope looks striped, implement nested opposite-hand strand and yarn
  construction rather than warping the stripes.
- If metal looks like black plastic, separate dielectric scale or oxide from
  conductive exposed metal.
- If every surface looks equally busy, repair detail priority and rest masks.
- If close-up looks photographic but gameplay view looks flat, add medium
  value structure and distance hierarchy.
- If gameplay view is noisy, remove or fade microdetail instead of blurring the
  entire material.

## Authoring exit gate

Do not proceed to final shader acceptance until:

- the champion seed is deterministic;
- each frequency band has an isolated proof;
- each physical output has a causal source;
- local frames and feature scales survive arbitrary element size;
- pattern repetition is not obvious in 4x and 16x views;
- color contains layered within-element variation;
- unsupported lanes remain zero;
- all maps have declared formats and manifest entries.
- the layer-provenance ledger matches the executed graph and proof set.
