# Cathedral Stone Trim and Fracture V1

## Current capability

This canonical package now owns one production-candidate capability:

> A complete semicircular order of sixteen individual Old Sarum
> catalogue-55-sized chevron voussoirs, with real joints, one phase-locked
> centripetal lateral chevron per stone, a live layered calcarenite material,
> neutral and material proofs, and saved-file reopen validation.

The package name is retained because it is the existing canonical owner.
Fracture is not part of this candidate. The old fracture and unrelated trim
experiments are not consumed by the v2 profile, generator, node group, proof
scene, or tests.

This is a production candidate, not a claim of archaeological reconstruction.
The measured stone envelope and the authored completion are kept separate.
The workflow tier is `hero-master`; acceptance requires installation and proof
on a resolved production target plus explicit user review. That target is
currently recorded as `pending-selection: production cathedral portal
consumer`, so this isolated fixture cannot be promoted beyond candidate by
renaming its state.

## Measured envelope and authored completion

Old Sarum catalogue item 55 supplies:

- 0.200 m radial stone height;
- 0.140 m inner chord;
- 0.180 m catalogue outer chord;
- 0.270 m depth.

The source does not publish a complete arch radius, stone count, joint width,
or moulding section. The v2 pattern therefore labels the following as
replaceable authored completion:

- sixteen stones over 180 degrees;
- 11.25 degree centre pitch;
- 11.042294066 degree body angle;
- 0.003 m working joint at the 0.8275514105 m centreline radius;
- 0.7275514105 m intrados radius;
- 0.9275514105 m extrados radius;
- 1.455102821 m clear diameter;
- 1.855102821 m outer diameter;
- 0.1784852529 m derived outer chord, within 1.515 mm of the catalogue value;
- quiet/roll/hollow/roll/quiet zones of 25/50/50/50/25 mm;
- 12 mm roll crest and 6 mm hollow depression.

No arris radius is published. No bevel is authored.

## Construction

Every stone is a separate closed manifold mesh. The arch is not a connected
annular ribbon and its joints are not painted lines.

Each stone contains:

- 1,650 vertices;
- 1,648 polygons;
- 768 front relief polygons;
- a 32 by 24 sampled chevron front;
- a matching closed rear and four closed boundary surfaces;
- unit object scale;
- no Bevel or Boolean modifier;
- two stable face-corner UV layers storing local metres.

The profile is generated from one signed chevron field. That same field owns
the visible front displacement and the roll, hollow, quiet, ink, and
highlight attributes, so shader linework cannot drift away from the geometry.

## Texture family

The generator produces exactly five consumed texture lanes at 1024 square:

| Lane | Span | Format | Meaning |
| --- | ---: | --- | --- |
| basecolor | 0.512 m | RGB8 sRGB | broad twenty-shade intrinsic calcarenite colour without mortar or baked light |
| ORM | 0.512 m | RGB8 Non-Color | AO=1, bounded broad roughness, metallic=0 |
| body masks | 0.064 m | RGB8 Non-Color | fossil fragment, silicate grain, visible pore identity |
| body normal | 0.064 m | RGB8 Non-Color | OpenGL tangent-space measured-proxy body normal |
| body height | 0.064 m | gray16 Non-Color | normalized measured-proxy body height with a 0.00113 m declared range |

Only the accepted `cathedral_stone_v1` body masks, normal, and height are
inherited, byte-for-byte. Its ashlar courses, mortar pattern, photographed
light, and wall-scale construction field are not inherited. The v2 generator
authors a new trim-specific broad colour and roughness field.

## Coordinates and variation

`IGGY_StoneUV_A` and `IGGY_StoneUV_B` store local tangential and radial
coordinates in metres. They are created from the stone parameters before the
object is rotated into the arch. No bounding-box projection is used.

Four rotation/mirror variants and deterministic phase offsets are assigned
per stone:

- UV A samples the 0.512 m broad field and 0.064 m primary body field;
- UV B independently samples the incommensurate 0.091 m secondary body field;
- the same image is never aligned once across the complete arch;
- `iggy_material_variant`, `iggy_material_phase`, and
  `iggy_voussoir_id` remain live shader or diagnostic inputs.

## Blender shader

The final group is `IGGY_SH_ChevronVoussoir_v002`; the material is
`IGGY_MAT_ChevronVoussoir_v002`.

The executed Blender manifest records every final node, socket default, mode,
and link. The core material flow is:

```text
metre UV A/B
  -> broad color and ORM
  -> blended 64/91 mm body identities
  -> per-stone warm/cool variation
  -> fossil/silicate/pore color
  -> live roll/hollow color and roughness
  -> selective ink/highlight
  -> 64 mm OpenGL Normal Map at 66%
  -> 91 mm non-inverted Bump at 34%
  -> opaque dielectric Principled BSDF
```

Microdetail is full at 0.30 m and fades by smootherstep to zero at 3.50 m.
Selective graphic linework uses a separate 6.00 m limit so the moulding
remains legible at gameplay distance. Texture AO stays at one; real geometry
supplies recess and joint occlusion.

## Semantic attributes

Point domain:

- `iggy_voussoir_id`;
- `iggy_material_phase`;
- `iggy_material_variant`;
- `iggy_chevron_roll`;
- `iggy_chevron_hollow`;
- `iggy_chevron_quiet`;
- `iggy_chevron_ink`;
- `iggy_chevron_highlight`.

Face domain:

- `iggy_carved_trim`;
- `iggy_chevron_front`;
- `iggy_fracture_interior`.

`iggy_fracture_interior` is false on every face. Damage, fracture, chips,
wear, weathering, damp, lichen, soot, dust, and repair are absent.

## Layer provenance

The profile records nine machine-audited causal layers:

- construction;
- macro;
- medium;
- edge/event;
- micro;
- cumulative colour;
- height/normal;
- material response;
- stylization.

Each layer identifies its physical meaning, sources, outputs, live consumers,
isolated proof, protected-rest rule, and supporting measurement claims. This
prevents graph size from standing in for material depth: a branch that reaches
no consumer or cannot be isolated in proof is not counted as an implemented
layer.

## Proof package

Nine 900 by 900 proofs are generated under `output/proofs/`:

- neutral clay front;
- neutral clay grazing;
- live material front;
- live material grazing;
- measured close;
- distance read;
- moulding masks;
- per-stone identity;
- wireframe topology.

For fast repair loops, the builder supports:

```sh
# Geometry, maps, packed shader, blend, and manifest; no renders.
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py \
  -- --output /private/tmp/iggy-chevron-fast --proof-set none

# Render only the changed diagnostic.
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py \
  -- --output /private/tmp/iggy-chevron-changed \
  --proof-set changed --proof live_material_front
```

Fast modes reject the canonical output path. The canonical build requires all
nine proofs.

The strongest delivery views are:

- [live material front](output/proofs/cathedral_stone_trim_fracture_v1_live_material_front.png);
- [live material grazing](output/proofs/cathedral_stone_trim_fracture_v1_live_material_grazing.png);
- [measured close](output/proofs/cathedral_stone_trim_fracture_v1_measured_close.png);
- [neutral clay front](output/proofs/cathedral_stone_trim_fracture_v1_neutral_clay_front.png);
- [wireframe proof](output/proofs/cathedral_stone_trim_fracture_v1_wireframe_proof.png).

## Documentation and ownership

- `WORKSTREAM.md` records the goal, source ledger, prior prototype failure,
  repair passes, and final boundary.
- `RESEARCH_INTENT.md` translates research into geometry, colour, PBR,
  coordinate, and rejection contracts.
- `references/chevron_voussoir_research_v1.md` is the written research and
  measurement ledger.
- `REFERENCE_DELTA.md` records the remaining differences from Old Sarum,
  CRSBI, BG3, Mike Means, and Max Kutsenko rather than treating those names as
  automatic quality approval.
- `profiles/cathedral_stone_trim_fracture_v1.json` owns physical, texture,
  semantic, PBR, and shader policy.
- `patterns/cathedral_trim_fracture_atlas_v1.json` owns the v2 measured
  envelope and authored closure.
- `CODED_DEMANDS.md` contains the exact texture inventory, all final Blender
  nodes and links, complete tests, complete profile/pattern data, and complete
  generator/builder source directly beneath their demands.
- `generate_cathedral_stone_trim_fracture_v1.py` owns the five-lane texture
  build and texture manifest.
- `build_cathedral_stone_trim_fracture_v1.py` owns geometry, attributes,
  packed shader, proofs, saved blend, and Blender manifest.

## Reproduce

Generate the five texture lanes:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/cathedral_stone_trim_fracture_v1/generate_cathedral_stone_trim_fracture_v1.py
```

Build the complete packed asset and all proofs:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py
```

Run the focused generator and saved-file gates:

```sh
/Applications/Blender.app/Contents/MacOS/Blender \
  -b --factory-startup --python-exit-code 1 \
  --python tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py

/Applications/Blender.app/Contents/MacOS/Blender \
  -b assets/creative/materials/cathedral_stone_trim_fracture_v1/output/cathedral_stone_trim_fracture_v1.blend \
  --python-exit-code 1 \
  --python tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py
```

The current gates are eleven generator tests and nine reopened-blend tests,
plus thirteen workflow-policy tests.

The profile also budgets:

- 6.5 MB generated texture data;
- 16 product objects;
- 30,000 vertices and 30,000 polygons;
- 85 shader nodes and 110 links;
- five unique images;
- nine final proofs.

The final package audit rejects budget overruns, stale outputs, dormant image
or Attribute nodes, unowned geometry attributes, unsupported numeric claims,
coded-document drift, incomplete layer provenance, hash mismatch, an
unresolved production target, and accepted status without both an
actual-target proof and manual-review record.

Run the candidate audit with:

```sh
python3 assets/creative/materials/workflow/scripts/audit_material_package.py \
  --package assets/creative/materials/cathedral_stone_trim_fracture_v1 \
  --profile assets/creative/materials/cathedral_stone_trim_fracture_v1/profiles/cathedral_stone_trim_fracture_v1.json \
  --pattern assets/creative/materials/cathedral_stone_trim_fracture_v1/patterns/cathedral_trim_fracture_atlas_v1.json \
  --texture-manifest assets/creative/materials/cathedral_stone_trim_fracture_v1/output/cathedral_stone_trim_fracture_v1_manifest.json \
  --blender-manifest assets/creative/materials/cathedral_stone_trim_fracture_v1/output/cathedral_stone_trim_fracture_v1_blender_manifest.json \
  --coded-demands assets/creative/materials/cathedral_stone_trim_fracture_v1/CODED_DEMANDS.md \
  --workflow-state assets/creative/materials/cathedral_stone_trim_fracture_v1/WORKFLOW_STATE.json \
  --output-root assets/creative/materials/cathedral_stone_trim_fracture_v1/output \
  --renderer assets/creative/materials/workflow/scripts/render_coded_demands.py \
  --generator assets/creative/materials/cathedral_stone_trim_fracture_v1/generate_cathedral_stone_trim_fracture_v1.py \
  --builder assets/creative/materials/cathedral_stone_trim_fracture_v1/build_cathedral_stone_trim_fracture_v1.py \
  --generator-test tests/unit/cathedral_stone_trim_fracture_v1_generator_tests.py \
  --blend-test tests/unit/cathedral_stone_trim_fracture_v1_blend_tests.py \
  --reference-delta assets/creative/materials/cathedral_stone_trim_fracture_v1/REFERENCE_DELTA.md \
  --state candidate \
  --report assets/creative/materials/cathedral_stone_trim_fracture_v1/MATERIAL_AUDIT.json
```

## Boundary

State: production candidate delivered in Blender.

Not claimed:

- archaeological reconstruction of the missing Old Sarum profile section;
- accepted user sign-off;
- reconstructed jambs;
- fracture or damage quality;
- other trim families;
- Unreal parity.
