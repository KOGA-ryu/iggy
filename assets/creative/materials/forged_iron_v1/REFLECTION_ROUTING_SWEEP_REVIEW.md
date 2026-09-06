# v004.1 Clean-Metal Reflection-Routing Review

## Decision

**Select C's uniform component-tangent direction response as a calibration
recipe. Reject B's new roughness field and D's added complexity.**

The selected operation is narrowly bounded:

```text
oxide anisotropy amount = 0.28
anisotropy rotation = 0.50
tangent ownership:
  flat leaf   -> longitudinal stock U
  rolled eye -> circumferential roll U
  pintle      -> axial pin U
```

The values are authored review values, not measured constants. No texture
pattern supplies them. C uses a uniform four-by-four control image and enables
the existing source multiplier only inside a disposable in-memory copy of the
v004 response group. The saved group remains disabled and unchanged.

This is not an integrated material, accepted donor, or Unreal-parity result.

## Geometry-first finding

The geometry board is
`output/forged_iron_v004_1_reflection_routing_sweep_v1/forged_iron_v004_1_geometry_moving_strip_board.png`.
Its columns are full clay under a left strip, full clay under a right strip,
pivot clay under a left strip, pivot clay under a right strip, and the actual
construction tangent encoded as RGB.

### Leaf planes

The leaves are genuinely broad planar objects. Reversing the strip moves one
large highlight across the assembly without revealing secondary forged plane
breaks. Their openwork apertures, taper, thickness, and boundary bevels remain
readable, but the major faces are intentionally quiet. A material may alter
the reflection shape; it may not claim that it repaired missing geometric
faceting.

### Pivot

The pintle and rolled eyes carry nearly ideal cylindrical highlights. The
black horizontal interruptions are real bearing gaps. The bright vertical band
is controlled mainly by smooth cylinder geometry. That clean geometric read is
not a texture failure, and this material gate does not attempt to hide it with
rings, dents, octagonal normals, or roughness bands.

### Tangent ownership

The tangent proof is coherent. The broad leaf field stays longitudinal; the
pivot changes to the roll and axial frames as required. The narrow cyan band at
the pivot is a deliberate construction-class transition rather than a random
UV rotation. The proof supports direction-dependent response as a legitimate
shader owner.

## Material board

The A--D board is
`output/forged_iron_v004_1_reflection_routing_sweep_v1/forged_iron_v004_1_reflection_routing_board.png`.
Columns are absolute roughness, direction-response amount, close neutral,
moving strip left, moving strip right, pivot close, and gameplay distance.

### A — v004.1 control

A preserves the selected-C palette and complete dielectric oxide, but it also
retains the known rejected broad-normal field and zero oxide anisotropy. It is
the frozen comparison control, not the recommended response.

### B — construction-owned roughness only

B obeys the new ownership contract:

- no compact support or closed mask;
- no normal or height source;
- one open cubic-Hermite field per parent strap lineage;
- one non-ring axial/circumferential passage on the pintle;
- source roughness mean preserved by scalar DC compensation only;
- all values stay inside `0.58--0.80`.

The actual deltas are restrained. The leaf fields span about `0.020--0.021`
roughness peak to peak. Knuckle additions span about `0.003--0.010`; the
pintle addition spans `0.0307`. No stripe or cloud appears in the isolated
proof.

That technical cleanliness is not enough. B is nearly indistinguishable from
A in the lit asset:

| View | PSNR versus A |
| --- | ---: |
| close neutral | 57.61 dB |
| moving strip left | 63.42 dB |
| moving strip right | 63.70 dB |
| pivot close | 57.40 dB |
| gameplay | 64.62 dB |

The field adds ownership, code, images, and parameters without a persuasive
material improvement. B is rejected rather than amplified into visible bands.

### C — uniform direction-dependent reflection only

C changes one causal response. It copies the existing v004 group in memory,
changes the already-present `Worked_Luster_Disabled` multiplier from zero to
one, and feeds uniform `R=0.28`, `G=0.50`, `B=1.0`. The component tangent is
already connected to the oxide Principled shader.

The result changes highlight width and energy under both opposed strip
positions without drawing a surface pattern:

| View | PSNR versus A |
| --- | ---: |
| close neutral | 44.03 dB |
| moving strip left | 39.28 dB |
| moving strip right | 39.47 dB |
| pivot close | 44.40 dB |
| gameplay | 49.52 dB |

The response remains subordinate. It does not create brushed-aluminium lines,
a candy-cane roll, a fixed highlight, grooves, scratches, rust, polished
islands, or gameplay speckle. It also does not pretend to repair the smooth
pivot geometry.

C is selected because it produces the requested uniform angle-dependent metal
behavior without inventing a visible pattern.

### D — roughness plus direction

D is visually C plus an unnecessary B. Relative to C it measures `57.74 dB`
close, `64.09/63.63 dB` under the two strip positions, `57.78 dB` at the
pivot, and `64.69 dB` at gameplay distance. The new roughness layer has no
persuasive contribution, so D is rejected.

## Frozen absences and source safety

- v004.1 source hash unchanged before and after thirty-three renders;
- saved `IGGY_SH_ConnectedOxideForgedIron_v004` topology hash unchanged;
- saved `Worked_Luster_Disabled` multiplier remains exactly zero;
- source colour and normal images unchanged in every row;
- no new normal, height, oxide height, conductor exposure, damage, scratches,
  dents, rust, polish, or baked light direction;
- no candidate `.blend` and no AI-generated imagery.

## Exact next gate

Integrate only C into a new noncanonical v004.2 candidate:

1. open saved v004.1;
2. copy the shared v004 group as
   `IGGY_SH_ConnectedOxideForgedIron_v004_2`;
3. preserve every node and link, changing only the anisotropy multiplier from
   zero to one;
4. replace each zero-luster control image with one packed uniform
   `(0.28, 0.50, 1.0)` control image;
5. keep v004.1 colour, roughness, broad normal, full oxide coverage, zero
   conductor exposure, and all geometry unchanged;
6. save and reopen v004.2;
7. render paired v004.1/v004.2 close, opposed-strip, pivot, gameplay,
   direction-amount, and source-normal proofs;
8. reject the integration if the saved result creates tangent seams, changes
   a frozen image, modifies v004.1, narrows highlights into brushed-metal
   lines, or claims to fix clay-owned pivot geometry.

The selected response becomes a material candidate only after that separate
integration demand and reopen proof pass.
