# Forged Iron v004.2 Geometry-Normal Control Review

## Decision

**Select row B, geometry normals only, as the normal-ownership recipe. Do not
alter or promote the saved v004.2 candidate in this experiment.**

Row A is the saved v004.2 material with the inherited broad normal applied at
Strength 1. Row B is a disposable in-memory group copy with only that existing
Strength set to 0. The source normal remains packed and hash-identical; no new
normal, height, colour, roughness, pattern, or condition field was authored.

## Proof

The paired actual-hinge board is:

`output/forged_iron_v004_2_geometry_normal_control_v1/forged_iron_v004_2_geometry_normal_control_board.png`

It uses two independently reopened rows and seven aligned columns: close
neutral, moving strip left, moving strip right, pivot close, gameplay distance,
applied normal amount, and frozen source normal. The inherited moving-strip clay
board is hash-registered as the geometry baseline and was not regenerated.

| Physical panel | B against A PSNR |
| --- | ---: |
| Close neutral | 44.862448 dB |
| Moving strip left | 50.282430 dB |
| Moving strip right | 48.363634 dB |
| Pivot close | 44.721258 dB |
| Gameplay distance | 51.861939 dB |

The values confirm a bounded response change. They do not choose the art
direction; the following visual causality does.

## Complete visual critique

1. Row B removes faint cloudy and dent-like shading from the long leaf planes.
   The broad intact-oxide value organization and selected direction-dependent
   reflection remain live, so the surface does not become a flat colour fill.
2. Both opposed moving-strip views preserve their large highlight placement.
   The cleanup is therefore not a one-light trick or a baked light direction.
3. Aperture boundaries, round holes, leaf joins, bevel lands, and the division
   between moving and fixed leaves remain equally readable. Those structures
   continue to derive from actual geometry.
4. B improves the pivot most clearly. Its cylinder keeps a continuous highlight
   while losing the low-amplitude vertical mottling supplied by the inherited
   broad normal. The black bearing gaps do not move or soften.
5. The difference nearly disappears at gameplay distance. That is desirable:
   the removed normal was broad enough to perturb form but too weak and generic
   to establish material identity at distance.
6. All eight component base-colour, roughness, source-normal, and direction-
   control float hashes match. The source-normal proof pixels also match. The
   exact graph diff is one default: `Broad_Forging_Normal_Decode:0:Strength`,
   from `1.0` to `0.0`.
7. The openwork can still resemble repeated face-like silhouettes. That is
   literal consumer geometry, not a material motif, and this shader gate neither
   hides nor repairs it.

## What this does not prove

- It does not accept forged iron as a finished material.
- It does not approve the hinge geometry or smooth pivot construction.
- It does not add forged plane breaks, microstructure, wear, oxidation change,
  damage, rust, scratches, pits, or exposed conductor.
- It does not establish a matched professional-reference plate, donor status,
  Unreal parity, or user acceptance.

## Next exact gate

Author a separate v004.3 integration demand derived from saved v004.2. Copy the
shared group under a new version, change only the same Normal Map Strength from
one to zero, retain the packed source normal for provenance but leave it
inactive, save/reopen the candidate, and render a paired v004.2/v004.3 proof.
Do not invent a replacement normal field during that integration. The saved
v004.2 file remains the current preferred repair candidate until that gate is
completed.
