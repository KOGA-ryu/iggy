# Prompt 01: Blade Geometry, UV, and Semantic Ownership

## Objective

Ensure the blade can create the reflection structure of a sword before maps or
lighting attempt to imitate it.

## Required semantic vocabulary

Evaluate and explicitly include or exclude:

- blade body or `ji`;
- fuller floor and fuller shoulder;
- central ridge or `shinogi`;
- primary bevel;
- cutting edge and edge land;
- spine or `mune`;
- ricasso;
- shoulder and guard contact;
- tip, kissaki, yokote, and boshi region where applicable;
- engraving, inlay, maker mark, and pattern-weld eligibility;
- first and second blade faces.

## Geometry decision

For each visible feature choose one owner:

1. **Direct geometry:** silhouette, plane transition, real recess, edge land,
   fuller, ridge, deep engraving, chip, or feature casting a moving shadow.
2. **High-to-low transfer:** shallow engraving, controlled hammering, or
   relief whose silhouette is unimportant but parallax must remain coherent.
3. **Semantic attribute:** construction identity without relief.
4. **Texture mask:** surface-only response variation.

Record why the rejected lanes cannot own the feature.

## UV and tangent contract

- U runs shoulder-to-tip in metres.
- V runs across the blade section.
- Important shells remain straight enough for directional finishing.
- Face one and face two are independently addressable unless the source proves
  exact manufacturing symmetry.
- Tip and ricasso receive explicit coordinates rather than bounding-box
  guesses.
- Tangent direction survives object transforms and engine export.
- Texel density is stated for hero and gameplay use.
- Final triangulation is frozen before normal baking.

## Two implementation strategies

### Strategy A: semantic corner attributes

Use face-corner one-hot attributes for body, fuller, bevel, edge, ricasso, and
tip. This is preferred when geometry is available and the engine adapter can
preserve or bake the attributes.

### Strategy B: authored region atlas

Bake or paint a non-interpolating region map from verified face selections.
Use this when the target engine cannot consume the attributes. Preserve the
attribute source as the canonical construction record.

## Required proofs

- clay front and three-quarter views;
- grazing-light plane proof;
- semantic colour proof;
- wireframe or topology proof;
- UV direction proof;
- both blade faces;
- tip and ricasso close-ups.

## Exit gate

No material work begins while a required fuller, bevel, ridge, edge, or tip is
only a shaded stripe. Unknown geometry remains absent and documented.
