# Historical Swords: Clean-Steel Response Study

## Decision

`diagnostic_response_pass_with_geometry_gaps`

The unchanged canonical clean-steel material produces materially different
responses on four different blade sections. That establishes the intended
diagnostic result: blade geometry and semantic face ownership materially affect
the steel read. It does not accept the donor swords, promote the shader beyond
its existing status, save a new Blender donor, or establish Unreal parity.

The inspected board is
[`historical_swords_clean_steel_response_board.png`](output/historical_swords_clean_steel_response_v1/historical_swords_clean_steel_response_board.png).

## Frozen material route

Every row uses one instance of `IGGY_MAT_SwordSteel_CleanGround_v001` built by
`build_sword_steel_v1.py`:

- one Principled conductor;
- linear RGB conductor value `[0.56, 0.57, 0.58]`;
- metallic `1.0`;
- body roughness `0.38`;
- body anisotropy `0.18`;
- one `IGGY_BladeUV` tangent;
- one canonical 512x128 macro-polish image;
- one canonical 1024x256 longitudinal-grind image;
- one micrometre-distance bump;
- one-hot `sinc_sword_region` response.

The graph source and both generated images were hash-checked before and after
the build. There are no per-sword material values, node forks, colour changes,
damage layers, or hidden light changes.

## Semantic census

| Donor | Body | Fuller/recess | Bevel | Edge | Construction consequence |
| --- | ---: | ---: | ---: | ---: | --- |
| ArmingSword | 28 | 0 | 20 | 0 | compound planes exist, but the source has no separately modelled edge land |
| BastardSword | 56 | 24 | 84 | 12 | long central recess terminates before the point and exercises every shader region |
| LongSword | 52 | 0 | 80 | 12 | broad, nearly constant-thickness body gives the weakest plane separation |
| ClaymoreSword | 44 | 4 | 116 | 8 | complex forte response is local and resolves into a quiet distal blade |

The colours in the fourth column are diagnostic ownership: red body, green
fuller/recess, blue bevel, and white edge. They are not intended sword colours.

## Visual critique

### ArmingSword

The neutral and grazing views expose the compound section as a broad body band
bounded by darker bevel planes. The taper remains readable at gameplay scale.
The decisive defect is geometric: the source cross section converges directly
from bevels to a zero-width cutting line, leaving no polygons that can own the
edge response. The canonical edge lane correctly remains unused.

Implication: our blade generator must author a narrow but real edge land when
we want independently controlled edge roughness, anisotropy, polish, export
identity, or repair behavior. A bright shader mask cannot substitute for it.

### BastardSword

This is the strongest multi-geometry result. The recessed central section
creates a distinct longitudinal value/highlight channel in neutral and grazing
views, remains present in the gameplay panel, and visibly releases before the
point. The region proof tracks 24 fuller polygons rather than the first
adapter's rejected root dot.

Implication: build a fuller as a cross-section valley bounded by shoulders,
carry it through explicit stations, then raise and narrow the floor into the
body between the termination stations. The shader will reveal that sequence;
it should not draw the sequence.

### LongSword

This is the weakest geometry separation and therefore the best negative
control. Its broad body reflects as a comparatively uniform field. Neutral
light produces a large uninterrupted response, while grazing light changes
the whole plane together rather than revealing meaningful distal transitions.

Implication: a sophisticated steel graph cannot rescue a blade whose generator
changes only profile width while leaving thickness and section structure nearly
constant. Distal taper and plane transitions must be authored numerically.

### ClaymoreSword

The root shows a localized reinforced/fluted response and a short recessed
lane; both release quickly into a much simpler distal construction. That
transition remains visible under neutral and grazing light without spreading
detail over the whole blade.

Implication: complex forte geometry should be a bounded station regime. It may
own special ridges, flutes, or reinforcement near the hilt, but the generator
must deliberately resolve those features rather than extruding them to the
tip.

## What this changes in our sword generator

1. Every blade recipe needs explicit longitudinal section stations, not only a
   2D outline and extrusion depth.
2. Width taper and distal thickness taper need independent curves and numeric
   holdout checks.
3. Body, fuller, bevel, edge, and cap roles must be assigned when the section
   topology is constructed. Production code must not infer them afterward from
   face normals as this diagnostic adapter does.
4. A controllable cutting edge requires actual edge-land polygons. If the
   design intentionally ends at a zero-width line, the material contract must
   record that the edge lane is unavailable.
5. Fuller starts, shoulders, floor, narrowing, lift-out, and termination need
   separate topology events and station spans.
6. Complex forte construction should simplify into a quieter distal vocabulary
   through an authored transition, as the Claymore donor demonstrates.
7. Neutral and grazing proofs must be evaluated together. Neutral shows value
   organization; grazing exposes whether distinct planes actually redirect a
   highlight.
8. Gameplay proof must retain construction while releasing the finite grind
   pattern. Here the fullered Bastard section survives, while microfinish stays
   subordinate.

## Repair ledger

- Rejected assumption: every donor would contain an edge land. ArmingSword did
  not; the contract now allows an honest zero edge count.
- Rejected semantic proof: the first normal-sign classifier reduced the
  Bastard fuller to a root dot. It was replaced by normalized cross-section
  rise, matching the previously audited section evidence.
- Repaired proof rig: the first board used stale rotated axis dimensions and
  oversized light energy. Framing now uses the evaluated long/short in-plane
  dimensions and one shared lower-energy rig.
- Repaired camera equation: Blender orthographic scale was initially treated
  as frame height. The final route uses visible-width ownership and fixed
  longitudinal/vertical margins.
- Material repair count: zero. All changes belonged to geometry semantics or
  proof presentation.

## Remaining boundary

This adapter normalizes one whole-blade map across each donor, so the physical
span varies with blade length and width. That is acceptable for this controlled
shape-response comparison but not a finished reusable world-scale adapter.
The next real construction task is to use these lessons in our own sword
generator: author one fullered blade with explicit section stations, distal
taper, fuller termination, and a real edge land, then apply the same shader to
that owned geometry.
