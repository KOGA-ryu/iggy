# Lime Plaster over Giant Rubble Masonry V1

This package is a reusable wall-material system, not a painted photograph and
not a single chipped-plaster mask. It reconstructs five physically distinct
surface states:

1. exposed structural stone;
2. brushed lime mortar;
3. the keyed scratch coat;
4. the floated brown coat;
5. the thin lime finish.

The construction state belongs to geometry. The shader reads that state and
selects the corresponding physical response. It never invents plaster loss
from generic noise, and damp, soot, dust, and edge grime remain off until a
level-specific story calls for them.

No AI-generated or sampled image is a canonical input. Historical AI
experiments remain in `references/` only as inert archive material. The build
rejects an intent file that enables AI imagery, sampled pixels, or a legacy AI
input.

## Measured construction

The layer stack follows the traditional three-coat proportions documented in
the US National Park Service's
[Preservation Brief 21](https://www.nps.gov/orgs/1739/upload/preservation-brief-21-flat-plaster.pdf):

| State | Added depth | Cumulative depth |
| --- | ---: | ---: |
| structural masonry | 0 mm | 0 mm |
| flush lime mortar | 0 mm | 0 mm |
| scratch coat | 9.525 mm | 9.525 mm |
| brown coat | 9.525 mm | 19.050 mm |
| finish coat | 3.175 mm | 22.225 mm |

The finish is thin enough to read as the final skin rather than another
equally thick color band. The scratch coat carries diagonal keying. The brown
coat is sand-rich and comparatively level. The lime-rich finish is smoother
but still matte and hand worked.

Historic England's
[repointing guidance](https://historicengland.org.uk/images-books/publications/repointing-brick-and-stone-walls/)
is translated into an independently shaded mortar material: recessed mortar
has its own color, aggregate, brush direction, roughness, height, and contact
occlusion instead of being a dark line drawn around every stone. The written
research-to-intent translation is recorded in
`references/LIME_PLASTER_MASONRY_RESEARCH_V2.md`.

## Authored scale and pattern

Everything is authored in metres:

- tile span: 4 m;
- 7 constructed courses;
- 36 placed stones;
- stone widths: 0.38-1.14 m;
- course heights: 0.45-0.70 m;
- bed joints: 50 mm;
- perpendicular joints: 38 mm;
- mortar recess: 10 mm.

The rubble layout is explicit JSON, not Voronoi cells. Five champion outlines
provide 8-12 authored points each:

- quiet bedded slab;
- broad load-bearing limestone plane;
- laminated bedding face;
- fractured cool face;
- projecting traversal ledge.

Mirroring, phase, course tilt, and millimetre-scale sibling offsets reuse those
champions without changing the construction bond. Each placed stone retains
its identity, pigment family, face angle, relief, facet strength, line
priority, roughness, and traversal role. The five pigment families each own a
20-entry shade ramp, but any one quiet face deliberately uses only a bounded
middle subset. This keeps the material layered without turning every stone
into a jewel.

The plaster field contains 13 finite, metre-authored trowel passes. Each pass
has a centre, length, width, angle, and signed pressure. Their compression
fields, broken edges, and direction vectors drive color, form normal, detail
normal, roughness, and the packed brush-direction map. Low-frequency lime
variation supports those marks; it is not the primary form.

## Geometry contract

The acceptance wall owns:

- `iggy_plaster_layer_state` on the `FACE` domain;
- `iggy_plaster_coverage` on the `POINT` domain;
- `iggy_transition_edge` on the `POINT` domain;
- `iggy_traversal_id` on the `POINT` domain;
- `iggy_material_phase` and `iggy_material_variant`;
- optional `iggy_damp_mask` and `iggy_soot_mask`, both defaulting to zero.

Categorical construction state is face data so a triangle cannot interpolate
stone into brown coat. After a Boolean or other topology-changing operation,
the new faces must receive an explicit construction state. Continuous edge
and overlay lanes may be regenerated independently.

Five projecting traversal stones are real geometry. Their silhouettes and
depth survive flat lighting and distance; the material ID is an additional
readability lane, not a substitute for shape.

## Shader architecture

The Blender asset contains four named groups:

- `IGGY_SH_LimePlasterMasonry_v001` selects physical materials;
- `IGGY_SH_PlasterMasonrySurfaceData_v002` reads the authored maps and geometry
  lanes in object-space metres;
- `IGGY_SH_PlasterMasonryNormalCombine_v002` whiteout-combines independent
  form and detail normals;
- `IGGY_SH_PlasterMasonryHeight_v002` decodes 16-bit height into metres.

Stone, mortar, and plaster have separate Principled dielectric responses with
zero metalness and IOR values of 1.52, 1.48, and 1.46. One tangent-space
Normal Map consumes the selected whiteout-combined normal. Form remains
readable at distance; detail fades smoothly from full strength at 1 m to zero
at 8 m. The material uses one metre-valued Displacement node and no Bump node.
There are no runtime Noise or Voronoi nodes.

The coordinate source is the object's own position, scaled by 0.25 for the
4 m tile, plus the per-instance phase lane. Moving an object therefore does
not make the material swim.

## Generated data

The generator writes 21 declared outputs:

- plaster base color, ORM, 16-bit height, combined normal, form normal, and
  detail normal;
- masonry base color, ORM, 16-bit height, combined normal, form normal, detail
  normal, and mortar/ink/traversal masks;
- packed stylization and brush-direction lanes;
- six proof chapters for color, construction, linework, response, scale, and
  transition;
- a manifest recording dimensions, source hashes, measurements, metrics, and
  the no-AI source policy.

The packed Blender file embeds the 12 maps used by the live shader. Combined
normal maps remain available for other engines, while Blender consumes the
separated form/detail pair.

## Build

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --python-exit-code 1 \
  --python assets/creative/materials/lime_plaster_masonry_v1/build_lime_plaster_masonry_v1.py \
  -- --texture-resolution 1536 --render-width 1200 --render-height 900
```

The builder starts from the accepted forged-iron asset so the structural-oak
door and forged hardware remain present in the same acceptance scene. It
produces:

- neutral door hero;
- transition close-up;
- gameplay-distance view;
- grazing-light response;
- construction-state proof;
- traversal proof;
- normal proof;
- height proof;
- game-light view.

## Focused verification

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python tests/unit/lime_plaster_masonry_v1_generator_tests.py

/Applications/Blender.app/Contents/MacOS/Blender \
  -b assets/creative/materials/lime_plaster_masonry_v1/output/lime_plaster_masonry_v1.blend \
  --python-exit-code 1 \
  --python tests/unit/lime_plaster_masonry_v1_blend_tests.py
```

The first gate verifies the authored bond, dimensions, deterministic
variation, 20-shade palette ownership, trowel events, form-caused linework,
PBR coherence, seam continuity, transition order, and output bit depth. The
reopen gate verifies face-domain state, object-space coordinates, three
dielectric shaders, 12 packed maps, form/detail normal composition,
distance-faded detail, metre displacement, the absence of generic procedural
nodes, preservation of the accepted oak/iron systems, and all nine proofs.
