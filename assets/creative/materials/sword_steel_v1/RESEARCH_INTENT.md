# Sword Steel Research to Intent

## Named consumer and evidence

- Consumer contract: `docs/blender/sword_geometry_nodes_handoff_v0_1.md`,
  WPN-001 clean one-handed sword blade.
- Measured envelope proxy: Met 1978.145a,b, blade length 0.889 m and reported
  width 0.0318 m in `reference_dimensions_v1.tsv`.
- Substrate donor: accepted clean conductor preview, linear RGB F0
  `(0.56, 0.57, 0.58)` and roughness `0.38`.
- Direction donor: accepted component tangent preview, anisotropy `0.18`.

The Met object is a scimitar and supplies only a measured proof envelope. It
does not authorize the WPN-001 straight silhouette, section, fuller, finish,
or cultural interpretation. Those remain the local WPN contract or explicit
proof-fixture translations.

## Causal surface anatomy

| Component | Meaning | Owner | Lanes | Absence rule |
| --- | --- | --- | --- | --- |
| Clean conductor | exposed steel optical identity | one Principled BSDF | colour/F0, metalness | never replaced by diffuse grey |
| Blade regions | different stock-removal and polishing histories | `sinc_sword_region` corner attribute | roughness, anisotropy, finish strength | no position-gradient inference |
| Blade tangent | hilt-to-tip manufacturing direction | `IGGY_BladeUV` | tangent, texture coordinate | never world-space guessed |
| Ground finish | restrained longitudinal abrasive tracks | one authored 16-bit scalar field | roughness and micrometre bump through separate remaps | absent from colour and silhouette |
| Condition | later history | excluded | none | exact zero / absent |

## Strategies and selection

### Region ownership

1. **Selected:** one-hot corner attribute. It remains constant per face and
   adds no image map.
2. Rejected for v1: baked region texture. It adds a second texture and UV
   dependence without helping the diagnostic fixture.

### Ground finish

1. **Selected:** one new finite, deterministic, non-random longitudinal track
   field at the blade's physical width. It is the only new map.
2. Rejected: reuse the forged-iron worked-response atlas unchanged. Its
   2.436 by 0.492 m host and planishing history are wrong for a 31.8 mm ground
   blade.
3. Rejected: Wave/Noise/Voronoi nodes. They create an undocumented procedural
   finish and duplicate the already accepted scripted-map workflow.

## Professional sword-workflow findings

- Strong sword workflows lock the blade UV direction and retain continuous
  shells so finish and hamon masks follow manufacture instead of world space.
- A clean blade begins with plain metal and linear brushing; construction
  patterns are optional identity forks, not mandatory visual interest.
- Broad finish organization, resolvable abrasive passes, and sub-texel
  microstructure are separate frequency bands.
- Sub-texel scratches belong to anisotropic microfacet response when the map
  resolution cannot resolve them without aliasing.
- Finish strength changes by plane: body, fuller, bevel, and edge do not receive
  one undifferentiated scratch overlay.
- Hamon, hada, pattern welding, engraving, patina, and damage remain separate
  capabilities and default to absent.

## Revised intent

The blade must read as clean exposed steel from the conductor response first.
At grazing light the body remains broad and controlled, the bevel reflection
narrows, and the edge carries the cleanest continuous highlight. Longitudinal
grind is subordinate: visible close, coherent under changing light, and gone
at gameplay distance. It never becomes painted stripes.

The revised clean-polish stack has three causal bands:

1. one authored low-frequency control-lattice field supplies broad polish
   organization to roughness only;
2. the finite 47-track field supplies resolvable medium grinding to roughness
   and micrometre bump;
3. the accepted tangent anisotropy supplies the unresolved scratch population
   without a third aliased texture.

Geometry regions independently weight the macro and medium fields so the edge
remains quieter and cleaner than the body. No new colour, metalness, identity,
condition, or damage lane enters this capability.

## Actual arming-sword correction

The BATMAN factory replaces the former unknown consumer with a measured
0.780x0.048x0.006 m blade. Its source proves a lenticular section, an outer
22-percent secondary bevel, a 0.4 mm edge land, and no fuller. A face audit
also proves the source's `blade_edge` zone is spatially wrong on 28 side
polygons even though its count looks plausible. The actual-consumer adapter
therefore derives body, bevel, and edge from normalized local cross-section
position and uses source zones only as mismatch evidence. This preserves the
research-to-intent chain without editing the production geometry.
