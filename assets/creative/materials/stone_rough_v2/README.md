# stone_rough_v2

`stone_rough_v2` is a deterministic, seamless, layered material exercise. It
replaces the former "colorized noise" approach for this one material with an
authored construction:

1. periodic staggered stone cells;
2. cell IDs and distance-to-edge fields;
3. mortar gaps and rounded height profiles;
4. edge-local chips, face pits, and selected cracks;
5. constrained per-stone color plus mineral and deposited-dirt layers;
6. roughness and AO derived from the same physical masks;
7. tangent-space normals derived from metre-scaled height.

The generator produces:

- `stone_rough_v2_basecolor.png`: sRGB color without baked lighting;
- `stone_rough_v2_normal.png`: linear OpenGL/Y+ tangent-space normals;
- `stone_rough_v2_orm.png`: linear AO, roughness, and metallic in R, G, B;
- `stone_rough_v2_height.png`: 16-bit linear height;
- `stone_rough_v2_manifest.json`: scale, height reconstruction, palette, and
  parameter provenance;
- `proofs/stone_rough_v2_breakdown.png`: visual index for the eight-chapter
  material book;
- `proofs/stone_rough_v2_book/*.png`: full-page studies of pattern language,
  joints, relief, damage, color, response, scale, and the final read;
- `stone_rough_v2_material_book.md`: long-form reasoning behind every chapter;
- `proofs/stone_rough_v2_tiling.png`: repeated base-color and lit proofs.
- `proofs/stone_rough_v2_pattern_variations.png`: twelve deterministic
  variations around the authored pattern, with the compiled choice outlined
  in gold.

## Generate

The repository does not carry a separate NumPy environment. Use Blender's
bundled Python in background mode:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/stone_rough_v2/generate_stone_rough_v2.py \
  -- --resolution 1024
```

The default tile represents `1.6 m × 1.6 m`. Change `--seed`,
`--resolution`, or `--tile-size-m` to make a recorded variant.

## Hand-author and vary the pattern

The canonical stone composition lives in
`patterns/fieldstone_hand_authored_v1.json`. It is not a finished texture. It
is an editable layout recipe containing normalized site positions and three
semantic roles:

- `anchor`: large stones that establish the composition;
- `medium`: connective stones that carry its rhythm;
- `infill`: small stones that close awkward gaps.

Each role controls influence and how far a generated variation may move or
resize it. Variation `0` preserves the authored positions exactly. Positive
variation indices apply deterministic, role-bounded changes:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/stone_rough_v2/generate_stone_rough_v2.py \
  -- \
  --resolution 1024 \
  --pattern-variation 5
```

Use the pattern-variation proof sheet to compare the first twelve choices.
The manifest records the recipe, selected variation, role counts, cell-area
variation, and reconstruction settings.

The current checkpoint compiles one selected variation into the PBR maps.
World placement across several compatible compiled variants is deliberately a
separate next step; simply repeating the selected texture still repeats its
large landmarks.

## Drive color from a reference image

The `~/font` reference-style lane can turn an image into editable color
evidence:

```sh
cd ~/font
.venv/bin/python -m glyph_lab.cli reference-style-recipe \
  --image /absolute/path/to/stone-reference.png \
  --out /tmp/stone-reference-style \
  --grid-size 128 \
  --palette-size 5
```

Use a crop or image that contains the material itself, rather than a complete
scene containing unrelated materials. Then pass the exported recipe to the
generator:

```sh
cd ~/iggy3d
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/stone_rough_v2/generate_stone_rough_v2.py \
  -- \
  --resolution 1024 \
  --inkblotter-style \
  /tmp/stone-reference-style/reference_style_recipe.json
```

The adapter accepts:

- `glyph_lab.reference_style_recipe.v0`;
- `glyph_lab.reference_transfer_palette.v0`;
- a JSON object with a top-level `palette` of `#RRGGBB` colors.

For PBR safety, explicit linework, shadow, and highlight lanes are not used as
pigment. Near-black and near-white colors are also rejected. The remaining
midtone palette is "de-lit" by compressing luminance differences around its
weighted median while preserving hue, then its evidence weights control the
stone colors. Pattern, height, damage, normals, AO, and roughness remain
physically authored by this generator.

This separation is intentional: a normal reference photograph contains
lighting, but it does not directly contain reliable height or roughness data.

## Focused test

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python tests/unit/stone_rough_v2_generator_tests.py
```

The test covers authored pattern validation, deterministic variation,
meaningful cell-area diversity, pattern and texture seams, channel
completeness, layer relationships, Inkblotter palette filtering, and packaged
PNG contracts.

The package test also verifies that all eight material-book chapters and the
written companion are present. The old compact breakdown sheet is no longer
the primary explanation of the material.
