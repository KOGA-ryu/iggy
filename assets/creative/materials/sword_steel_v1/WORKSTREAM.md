# Sword Steel Workstream

## Scope

- ID: `sword_steel_v1`
- Tier: `reusable-family`
- Capability: clean multi-band sword polish over the accepted steel substrate
- Current proof consumer: WPN-001 diagnostic blade fixture
- Selected actual consumer: BATMAN factory-generated `arming_sword_v1`
- Actual blade: 0.780 m long, 0.048 m shoulder width, 0.006 m shoulder
  thickness, lenticular section, outer-22-percent secondary bevel, no fuller
- Damage and condition: excluded
- Unreal parity: false

## Donor ledger

| Donor | Reuse | Boundary |
| --- | --- | --- |
| clean conductor F0 | exact | optical identity only |
| conductor roughness 0.38 | exact for body | other regions are manufacturing-finish remaps |
| component tangent anisotropy 0.18 | exact for body | bevel and edge increase directionality |
| `component_tangent_uv` concept | recipe transfer | fixture installs its own metre-consistent `IGGY_BladeUV` |

## Live route

`profile -> analytic proof blade + corner region attribute -> authored macro
polish field + finite medium-grind field + tangent micro-response -> one
metallic BSDF -> actual proof views -> saved blend -> separate-process
validation`

## Sword prompt chain

The package-specific chain lives at `prompts/README.md`. The active execution
uses Prompts 00 through 02, then 06 and 07. Prompts 03 through 05 are read as
explicit `none` or verified skips for this capability.

Prompt 00 has now been executed in `CONSUMER_CENSUS.md`. It selects the
factory-generated `arming_sword_v1` flagship as the first actual target and
records the exact worker source, hashes, Blender version, evaluated dimensions,
topology, semantic zones, family order, and non-destructive mesh-intake gate.
The old `HeroSword_v1.blend` is explicitly excluded from this route.

## Workbench and strategies

| Component | At-hand tools | Selected strategy | Preserved alternative |
| --- | --- | --- | --- |
| substrate | accepted conductor and tangent donors | exact reuse | none required |
| blade regions | source-zone audit, normalized cross-section position, and `sinc_sword_region` corner attribute | local half-width owns side-face body/bevel/edge; face orientation/position owns caps; source zones own no new semantic; fuller is exact zero | authored region atlas for engine fallback |
| macro polish | NumPy, `write_png_gray16`, blade UV | authored 5x8 control lattice interpolated over local U/V | hand-painted broad field on actual sword |
| medium grinding | existing finite-track compiler | retain and region-weight | licensed/reference-derived directional mask |
| microstructure | accepted tangent anisotropy | BSDF microfacet response | band-limited detail normal only if resolvable |

The first failing observable is the absence of a macro-polish field and the
uniform medium-track strength across every blade region.

## Actual-consumer intake evidence

| Evidence | Result |
| --- | --- |
| Assembly source | `linux-worker:/home/kogaRyu/blender-refs/tools/assemble_arming_sword.py`, SHA-256 `767515f2...3392` |
| Blade source | `generators/sg/parts/blade.py`, SHA-256 `e2fa5851...6322` |
| Evaluated object | `SW_blade.001`; 270 vertices; 536 edges; 268 all-quad polygons |
| Bounds and axes | X width 0.048 m; Y thickness 0.006 m; Z blade length 0.780 m |
| Construction | lenticular body, straight secondary bevel, 0.4 mm edge land, no fuller |
| Factory zones | counts are `blade` 224, `blade_edge` 36, `ricasso` 8, but a face audit found one center strip mislabeled as edge and the negative edge strip unlabeled |
| Existing appearance | palette/vertex colour, roughness 0.8, smooth shading, optional inverted-hull outline |

The first cheap worker execution proved that the current
`finish_arming_sword.py` cannot complete: it requests `SW_grip.data.attributes["zone"]`,
but the generated grip no longer contains that attribute. This is an existing
factory integration defect, not a steel-shader defect. The measured assembly
still passes. The matched baseline therefore executes the assembly directly
and reconstructs only the blade's documented palette, vertex-colour dirty
pass, roughness 0.8, and smooth shading. It does not claim the broken full
finish script executed and does not add an outline to either comparison side.

The first repaired board also exposed Blender's default 2 m `Cube` occluding
the complete sword because the assembly script, unlike the broken finish
script, does not perform factory-scene hygiene. The adapter removes only the
named defaults `Cube`, `Light`, and `Camera` before executing the assembly.
This is a proof-scene repair and does not alter factory sword geometry.

With the cube removed, the next cheap board rejected the initial 650/210/900
area-light envelope because it saturated the baseline, clean steel, clay, and
grazing views to white. The actual geometry-derived region proof passed and
showed no green fuller. Reflection proofs return to the accepted diagnostic
energy envelope: key 22, fill 5, grazing 24, with the same dark neutral world.
This repair changes no material socket or texture value.

The energy repair removed numeric clipping but exposed an aspect-ratio error:
the proof cards were 0.78 m wide and only 0.055 m tall. The narrow blade
therefore reflected a single broad white source across nearly its entire
width. The final studio geometry uses tall vertical strips: key 0.06x0.68 m,
fill 0.12x0.50 m, and grazing 0.018x0.70 m. This is the causal reflection-card
orientation for a long ground blade and remains a proof-only change.

The vertical-card board preserved a narrow edge line but still placed the
clean conductor's reflected card at display ceiling. The final exposure is
therefore `-1.25` stops under the same AgX Medium High Contrast transform. This
is applied uniformly to baseline and every cumulative physical proof; no F0,
roughness, anisotropy, map, or region gain is reduced to rescue the render.

Final-resolution review rejected only the close grazing panel: its 24-energy
strip remained directly coaligned with camera and face reflection, producing
a featureless white card. The bounded repair moves that strip 0.18 m off-axis
and lowers it to energy 8, with fill 0.4. Front, three-quarter, gameplay,
exposure, shader, maps, and semantic regions remain unchanged.

The next decision gate is not another material variation. It is whether the
same accepted clean-steel graph reads correctly on this exact blade under one
matched baseline/material proof set.

## Workflow stages

| Stage | Status | Evidence |
| --- | --- | --- |
| 00 scope | complete | diagnostic WPN-001 boundary and reusable-family tier |
| 01 audit | complete | v1 manifest, tests, builder, donors, and stale coded compiler reviewed |
| 02 research | complete | sword artist, weapon artist, material, and engine findings recorded |
| 03 intent | complete | three-band ownership and exclusions frozen in research intent |
| 04 coded blueprint | complete | DEM-SWORD-POLISH-001 and DEM-SWORD-POLISH-002 |
| 05 map build | complete | 16-bit 512x128 macro lattice field and 16-bit 1024x256 finite 47-track grind field |
| 06 shader integration | complete | one Principled BSDF, two packed images, one tangent, one region-weighted bump |
| 07 damage | verified-existing | absent and tested |
| 08 proof | complete | neutral, grazing, gameplay, clay, regions, macro, and medium proofs plus comparison board |
| 09 repair | complete | macro isolation contrast repaired without increasing the production response |
| 10 handoff | complete | fresh Blender reopen, Python compile, focused profile/source/topology tests, and stated limitations |

## Licensed multi-sword geometry study

| Gate | Status | Evidence |
| --- | --- | --- |
| provenance | complete | Clint Bellanger Historical Swords Set, selected CC BY 3.0, 26,102,712-byte archive, SHA-256 locked |
| source boundary | complete | source re-hashed after inspection; no source save, mutation, texture use, runtime material, AI imagery, acceptance, or Unreal claim |
| construction census | complete | four swords; blade, guard, grip, and pommel ownership; Claymore guard resolves to four pieces |
| section audit | complete | exact X/Z mesh intersections at 12%, 25%, 50%, 75%, and 88% blade length; 20 sections total |
| visual board | complete | 2400x2240; whole, isolated hilt, oblique blade, topology, and five-section chart per sword |
| visual ruling | usable reference study | Bastard fuller termination, Arming compound taper, Claymore forte transition, LongSword constant-thickness negative control |
| production/engine status | excluded | no donor is a production sword; material transfer and Unreal parity are separate gates |

The next bounded capability is one matched, cheap response board using the
existing clean-steel graph unchanged on these four blade geometries. It must
answer whether body/fuller/bevel/edge construction differences remain legible;
it may not fork the shader or convert donor geometry into a production asset.

### Multi-geometry response result

| Gate | Status | Evidence |
| --- | --- | --- |
| canonical graph | unchanged | one Principled, zero Mix Shader, two byte-identical maps, one tangent, one bump, four consumers |
| semantic adapter | diagnostic pass | Arming 28/0/20/0, Bastard 56/24/84/12, Long 52/0/80/12, Claymore 44/4/116/8 body/fuller/bevel/edge polygons |
| neutral/grazing proof | diagnostic pass | Bastard fuller response and Claymore forte transition separate; Long remains the uniform negative control |
| gameplay proof | diagnostic pass | section differences survive while finite grind remains subordinate |
| source/canonical mutation | none | both before/after hashes match frozen authorities |
| packaging/acceptance | excluded | no saved blend, new donor, manual acceptance, or Unreal parity |

The diagnostic response gate is complete. The next owned-geometry capability
is one fullered blade with authored section stations, separate profile/distal
taper, explicit fuller lift-out, and a real edge land.

## Actual-consumer checkpoint

| Stage | Status | Evidence |
| --- | --- | --- |
| source intake | complete | hash-locked BATMAN assembly, blade generator, and finish source; Blender 5.1.1 |
| factory execution | complete with isolated donor defect | assembly passes; whole-sword finish fails on missing grip `zone`, so only the documented old blade baseline is reconstructed |
| semantic adapter | complete | 176 body, 0 fuller, 56 bevel, 36 edge polygons; 28 source-zone disagreements recorded |
| shader reuse | complete | canonical one-Principled/two-image/one-tangent/one-bump graph; no material fork |
| actual proof | production candidate | matched baseline/front plus three-quarter, off-axis grazing, gameplay, clay, regions, macro, and medium at 720x1080 and 48 Cycles samples |
| saved-file gate | complete | Blender 5.1.1 separate-process reopen; both finish images packed |
| focused tests | complete | 9 profile, source, manifest, output, and reopen tests pass |
| engine parity | excluded | Unreal remains unverified |

### Actual-consumer visual review

- Strongest result: the off-axis grazing crop shows a broad dark lenticular
  body, one controlled reflection, and continuous bright cutting edges. It no
  longer reads as the old flat charcoal palette blade.
- Semantic result: the isolated panel has red body, blue bevel/edge, and no
  green fuller. The source's misleading `blade_edge` strip does not enter the
  live shader.
- Distance result: the gameplay sword retains the clean bright blade identity
  while the 47-track band releases; the tracks remain visible only in the
  isolated close proof.
- Weakest area: the front highlight is necessarily narrow on a 48 mm blade and
  can still approach white in the strongest reflection. The three-quarter and
  grazing views carry most of the section read.
- Remaining context defect: guard, grip, pommel, and fittings are not part of
  this capability, and the factory's whole-sword finish route is currently
  broken. They remain neutral proof context.
- Repair count: six cheap proof/factory repairs after the first source build:
  isolate broken finish, remove the default cube, correct energy, correct
  reflection-card aspect, set shared exposure, and move the grazing strip
  off-axis. Only the selected final candidate received the 48-sample render
  and saved-file gate.
- Decision: `production candidate`, awaiting the user's visual ruling. Damage,
  decoration, alternate tiers, and Unreal translation remain excluded.

## Acceptance boundary

The diagnostic package can prove shader causality and donor reuse. It cannot
claim production sword acceptance because no finished WPN-001 asset exists.
The material passes this checkpoint only if it preserves flat grind planes,
keeps the edge highlight continuous, shows no coloured or symbolic pattern,
and releases the finish at gameplay distance.

## Proof review

- Result: accepted as a diagnostic production candidate, not as an actual-sword
  acceptance.
- Strongest read: the clean conductor changes from a broad pale reflection in
  the neutral view to grouped dark-grey planes under grazing light without an
  oxide, stain, or colour texture doing the work.
- Geometry proof: the clay view separates fuller floor, body lands, bevels,
  and edge; the corner attribute assigns all four regions without interpolated
  triangular masks.
- Macro proof: the 5x8 authored control lattice produces slow, asymmetrical
  polish drift with no repeated motif, diamond, face, or decorative symbol.
  It affects roughness only and remains subordinate to the conductor response.
- Finish proof: 47 finite longitudinal grinding tracks vary in position,
  width, pressure, interruption, and lengthwise envelope. They affect only a
  restrained roughness delta and micrometre bump, and disappear at gameplay
  distance. The anisotropic BSDF owns the unresolved microstructure rather
  than a third aliased scratch map.
- Rejected failure: the first render saturated the conductor into a white bar
  and the first clay proof hid the grind planes. Lighting, framing, and the
  clay proof were repaired as one proof-system batch; the material ownership
  contract did not expand.
- Rejected proof failure: the first macro-isolation render was too compressed
  to diagnose. Its proof material was contrast-stretched while the production
  macro amplitude remained unchanged.
- Repair count: 4 focused build/proof repairs, including one proof-only repair
  in this multi-band slice.
- Verification: Python compilation passed; three focused tests passed; Blender
  5.1.1 reopened the saved file in a fresh process and validated both packed
  images and the live node topology.
- Remaining weakness: the diagnostic fixture has no guard, grip, pommel, or
  production sword topology, so reflection continuity across a real assembled
  weapon and Unreal anisotropy parity remain unproved. The general material
  package auditor also expects the newer split generator/pattern/Blender
  manifest schema; this legacy-shaped package has not been misrepresented as
  passing that broader audit.
