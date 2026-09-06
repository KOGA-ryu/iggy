# Brick Masonry V1

> **Measurement status: research prototype, not a complete brick asset.**
> The audit in
> [`../MEASURED_CONSTRUCTION_DOSSIER.md`](../MEASURED_CONSTRUCTION_DOSSIER.md)
> measures the current rendered faces at 208.91–220.80 × 58.48–74.29 mm with
> 10–11.5 mm joints. Those values do not yet belong to one surveyed brick
> family, and this package has no brick depth, header face, frog, special-brick,
> or geometry-placement contract. Its 1.5–4.5 mm corner rounds and
> 0.6–2.9 mm crowns also remain unsupported authored values.

`brick_masonry_v1` is an authored stylized fired-clay wall material. Its
canonical composition is
`patterns/kiln_fired_running_bond_v1.json`: twenty courses, seven wrapped
positions per course, twenty reusable champion faces, and 140 independently
colored bricks.

The material targets an Arcane/Borderlands-adjacent read without treating
stylization as a generic black outline or a noise overlay. Physical material
construction and illustrative emphasis remain distinct:

1. the authored bond owns course height, course drift, brick placement, and
   champion identity;
2. each champion owns proportions, corner rounding, crown, forming history,
   inclusion density, fired-skin strength, line priority, and brush direction;
3. each individual brick owns a twenty-control fired-clay shade family;
4. kiln gradient, heat cloud, clay body, and fired skin continuously sample
   that family;
5. laminations, compression drags, pits, and mineral inclusions modify color,
   height, and roughness through different responses;
6. recessed mortar owns a separate body, aggregate, trowel, contact-cavity,
   color, height, and roughness state;
7. primary contour fragments, secondary form lines, and tertiary forming
   strokes compose into a separately exported ink mask;
8. highlight strokes and detail priority remain separate from ink so a
   renderer can tune the style without regenerating the PBR maps.

## Research translated into intent

- Rosen Kazlachev's
  [Arcane Brick Wall breakdown](https://rosko.artstation.com/blog/YjnN/free-material-tutorial-arcane-brick-wall-in-substance-designer)
  demonstrates art-directed brick masks, directional color masks, height
  blending, and outline accents derived from selected normal directions. This
  package translates that into authored champion roles, direction-bearing
  color fields, and explicit structural-line outputs.
- John F. Clifford's University of Surrey thesis,
  [High temperature reactions and colour development in brick clays](https://openresearch.surrey.ac.uk/esploro/outputs/doctoral/High-temperature-reactions-and-colour-development/99516275302346),
  identifies iron-rich clay and iron oxide as major fired-brick color sources
  and relates color development to firing temperature. This package therefore
  treats red, ochre, umber, and violet variation as kiln and clay history, not
  arbitrary per-pixel color noise.
- The Brick Development Association's
  [Mortar for Brickwork Technical Guide](https://www.brick.org.uk/uploads/downloads/08.-Mortar-for-Brickwork-Technical-Guide-2023.f1678701358.pdf)
  distinguishes compressed, weather-struck, flush, and recessed joint
  profiles. The current recipe uses a compressed shallow recess with its own
  trowel and aggregate signals.

No third-party imagery or texture data is stored in this package.

## Linework contract

Linework is not an edge-detection filter over the final image.

- **Primary lines** select fragments of brick arrises using authored
  per-brick side weights, variable line widths, broken coverage, and champion
  priority.
- **Secondary lines** describe clay laminations, pit and mineral rims, and
  intrinsic over-fired fissures.
- **Tertiary lines** describe sparse forming and compression drags.
- **Ink** is a deep brown-violet, not universal black.
- **Highlight strokes** select restrained opposing arris fragments.
- **Detail priority** keeps calm bricks quiet and preserves focal marks through
  later renderer or mip decisions.

The base color includes a restrained authored ink contribution so the material
has an illustrated identity in a conventional PBR pipeline. The same ink is
also exported independently for engines that support a dedicated stylization
lane.

## Outputs

- `brick_masonry_v1_basecolor.png`: sRGB color, including structural ink but no
  baked directional light;
- `brick_masonry_v1_normal.png`: OpenGL/Y+ tangent normal;
- `brick_masonry_v1_orm.png`: AO, roughness, metallic;
- `brick_masonry_v1_height.png`: 16-bit normalized height with metre range in
  the manifest;
- `brick_masonry_v1_stylization.png`: ink, highlight stroke, detail priority;
- `brick_masonry_v1_brush_direction.png`: signed XY brush direction remapped to
  zero through one;
- `proofs/brick_masonry_v1_construction.png`;
- `proofs/brick_masonry_v1_color_anatomy.png`;
- `proofs/brick_masonry_v1_linework.png`;
- `proofs/brick_masonry_v1_response.png`;
- `proofs/brick_masonry_v1_tiling.png`;
- `proofs/brick_masonry_v1_variations.png`;
- `brick_masonry_v1_manifest.json`.

## Generate

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/brick_masonry_v1/generate_brick_masonry_v1.py \
  -- \
  --resolution 2048 \
  --pattern-variation 0
```

## Focused verification

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python tests/unit/brick_masonry_v1_generator_tests.py
```

The gate proves recipe completeness, deterministic bounded variation, survival
of all 140 brick identities, periodic seams, twenty-shade ownership and use
inside one isolated brick, smooth shade interpolation, separate clay and mortar
fields, non-uniform line selection, coherent PBR maps, 16-bit height output,
and complete proof packaging.
