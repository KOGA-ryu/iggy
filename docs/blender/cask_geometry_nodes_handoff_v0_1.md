# Cask Geometry Nodes handoff — CSK-001 v0.3 (first reference-backed kit spec)

Companion to WPN-001 and the GH-007 door research. Same Blender 5.1.1 target (build `b70da489d7f4`), same `SINC_` library, same Python/idempotency contract (WPN-001 §3 applies unchanged — do not restate it, follow it). Target asset: **CSK-001, a coopered cask**, plus the vessel family that derives from it.

This is also the first spec written against the reference library, so it carries two sections WPN-001 does not have: **§7 Evidence and parameters**, where every number states where it came from and how much to trust it, and **§8 Redline framing**, which fixes the camera so renders land next to their source plate for comparison without anyone opening Blender.

## The central conclusion

GH-007 found the door resists Geometry Nodes because hewn timber is irregular by nature. WPN-001 found the sword suits them because a blade is *ground* — its ideal form is analytic. A cask is a third thing, and the most node-native of all: it is **assembled** geometry. The whole vessel is one part repeated by a rule, wrapped in rings whose position is derived rather than authored. There is no character to fake and no irregularity to invent; the form is a consequence of the joint closing.

The escape hatch is correspondingly narrow. Geometry Nodes deliver the entire clean cask. Direct geometry is needed only for what *use* does to one — sprung or missing staves, a burst hoop, char inside a fire-toasted barrel, bung and tap holes with torn grain. Build the clean vessel here; damage stays authored, exactly as GH-007 concluded for adze planes.

Why this asset first: it has the highest reuse-to-evidence ratio in the library. Every yard, cellar, cart and market square needs one, coopering is rule-governed rather than stylistic, and — see §7 — a single anchor measurement propagates across the entire vessel family by a cube-root rule.

## Conventions

Component-local axes, consistent with GH-007 stock and WPN-001:

- Cask, stave, hoop, head: **Z = height** (the axis of revolution), X/Y radial.
- Cask datum: **z = 0 at the bottom chime**, so the vessel occupies `[0, Height]`. Bilge sits at `z = Height/2` unless `Bilge Offset` moves it.
- Stave-local frame: X = stave thickness (radial), Y = stave width (tangential), Z = height. A stave is authored **standing at radius 0** and placed by the assembly; it is never authored in place.

Connection notation and socket-identifier rules are identical to GH-007 and WPN-001:

```text
NodeType["Output"] → NodeType["Input"]
```

Sockets resolve by runtime `identifier`, never numeric index.

## 1. The bilge function (read this before any group)

Everything in this spec derives from one analytic profile. `GeometryNodeCurvePrimitiveArc` is **absent from the catalog** (verified, §6), so the bilge is *not* an arc primitive. It is a radius field over normalised height:

```text
t          = SplineParameter["Factor"]                    # 0..1 up the cask
belly(t)   = sin(pi * t) ** Bilge Power                   # 0 at both heads, 1 at mid
r(t)       = Head Radius + (Bilge Radius - Head Radius) * belly(t)
```

`Bilge Power` ≈ 1.0 gives a full, near-circular belly; higher values pull the bulge tighter to the middle; 0 degenerates to a straight cylinder (useful for tubs, see §5).

This function is the **single source of truth for radius**. Stave outer face, hoop radius, and head placement all sample it. Hoop radius in particular is *derived, never authored* — an authored hoop radius is the defect that makes a cask read as a stack of unrelated rings.

## 2. Recommended reusable groups

### `SINC_GN_CaskStave`

One stave, authored standing, radius handled by the assembly.

- Spine: `GeometryNodeMeshLine` → `GeometryNodeMeshToCurve` → `GeometryNodeResampleCurve` (Count = `Stave Segments`, default 12 — enough to carry the bilge without faceting).
- Cross-section: `GeometryNodeCurvePrimitiveQuadrilateral` sized `Stave Thickness` × stave width, where stave width = `2 * r(t) * sin(pi / Stave Count)`. Width therefore **varies with height**, which is what makes staves taper toward the heads. Drive it with `ShaderNodeMath` off the same `belly(t)`.
- Edge bevel: each vertical edge is canted by `180 / Stave Count` degrees so that N staves close on themselves. Apply via the quadrilateral's corner positions, not by any bevel node — there is no bevel node.
- Croze: the inner face steps inward by `Croze Depth` over a band of `Croze Width` centred at `Chime Height` from each end. Because the stave is swept from a cross-section along height, the croze is a profile feature, not a cut. **Do not attempt to boolean or knife it.**
- Sweep: `GeometryNodeCurveToMesh` (Profile = the quadrilateral, Fill Caps = true).

Outputs the stave plus a `stave_index` attribute via `GeometryNodeStoreNamedAttribute` for later per-stave variation and for damage masks.

### `SINC_GN_CaskHoop`

No torus primitive exists (§6), so a hoop is a swept circle, as GH-007 handled riser rings.

- `GeometryNodeCurvePrimitiveCircle` (Resolution = `Stave Count * 2`, so hoop facets align to stave joints rather than beating against them).
- Radius = `r(Hoop Height Fraction)` **plus** `Stave Thickness * 0` — the hoop rides the *outer* face, so it samples the bilge function directly. Add `Hoop Proud` (default 2 mm) so it stands off the staves and catches a highlight.
- Profile: `GeometryNodeCurvePrimitiveQuadrilateral` sized `Hoop Width` × `Hoop Gauge` → `GeometryNodeCurveToMesh`.
- Iron hoops taper in width toward the chime on better work: allow `Hoop Width` to be driven per instance from the hoop's height fraction.

**`Hoop Material` and `Hoop Grouping` are separate parameters, and both are load-bearing** (added v0.2 on measured evidence, §7):

- `Hoop Material` — `iron` (dark, narrow, riveted, stands proud) or `wood` (pale, wide, flat, lapped and bound, nearly flush). A wooden-hooped vessel with iron-hoop styling reads as the wrong object entirely.
- `Hoop Grouping` — `even` (equal spacing up the body) or `clustered` (tight bands of 3–5 hoops at each end plus a pair at the waist, with bare wood between). **Do not assume even spacing.** The measured churn carries roughly eleven wooden hoops in three clusters; spacing them evenly would produce a vessel that exists nowhere.

### `SINC_GN_CaskHead`

A head is **not a disc**. It is two to four boards, edge-doweled, then cut round — the seams are visible and they matter at close range.

- `GeometryNodeMeshGrid` of `Head Board Count` strips → trimmed to a circle of radius `r(Chime Height) - Croze Depth` by a radial field, `GeometryNodeFillCurve` for the cap.
- Seat the head **inside** the croze: its plane sits at `Chime Height` from the end, never flush with the chime.
- Both heads are the same group, one mirrored; drive with `GeometryNodeFlipFaces` on the lower.

### `SINC_GN_CaskAssembly`

- `GeometryNodeMeshCircle` (Vertices = `Stave Count`) supplies the placement ring.
- `GeometryNodeInstanceOnPoints` places `SINC_GN_CaskStave`; `GeometryNodeRotateInstances` turns each to face outward by `360 / Stave Count * index`.
- `GeometryNodeJoinGeometry` adds hoops and heads; `GeometryNodeRealizeInstances` then `GeometryNodeMergeByDistance` (threshold 0.1 mm) closes the joints.
- `GeometryNodeSetShadeSmooth` false on stave faces — a cask reads as faceted boards, and smoothing it is the fastest way to make it look like a plastic barrel.

### Shader reuse (no new work)

Mask contracts are shared with GH-007 and WPN-001. Oak staves take the existing timber shader driven by `stave_index`; hoops take the WPN-001 ferrous shader with roughness biased up (hoop iron is not blade steel). No new shader groups.

## 3. Python and idempotency contract

WPN-001 §3 applies unchanged.

## 4. Fragile approaches to reject

- **Do not lathe or revolve a profile.** There is no lathe node, and more importantly a cask is *not* a surface of revolution — it is discrete boards. A revolved cask has no joints, so hoops sit on a lie and the silhouette from directly above is wrong.
- **Do not model hoops as tori.** No torus node; and an authored hoop radius drifts off the bilge. Hoops sample `r(t)`.
- **Do not cut the croze.** No knife, bisect, boolean or inset node. It is part of the stave's swept profile.
- **Do not randomise stave widths independently.** They must sum to exactly 360° or the vessel will not close. If per-stave width variation is wanted, vary, then renormalise so the sum is preserved — and assert it (§9).
- **Do not use noise displacement for "wood character".** GH-007's finding stands: Geometry Nodes noise reads as melted wood.
- **Do not smooth-shade the staves.** See above.

## 5. The derivation chain (where the leverage is)

**Read this correction first (v0.2).** The cube-root rule below is real but it only holds *within a shape class*. Measured objects (§7) show three distinct classes that scaling cannot bridge:

| Class | Profile | Measured height ÷ diameter | Hoops |
|---|---|---|---|
| **Storage cask** — barrel, hogshead, butt, tun | bulged (bilge) | ~1.4 (standard, still unmeasured) | iron, even |
| **Portable keg** — keg, rundlet, costrel | near-cylindrical, bilge almost absent | **1.95 (measured)** | iron, even, many for the size |
| **Tapered woodenware** — churn, piggin, bucket, tub | truncated cone, no bilge, wide end down | **3.13 measured (churn)** | **wood, clustered** |

Scaling a barrel down does not produce a churn; the profile and the hooping are different in kind. Treat the ladder below as the storage-cask class only, and take tapered woodenware from its own measured parameters.

Within a class, shape is held constant, so **linear scale is the cube root of the capacity ratio** — one anchor measurement propagates across that class.

### v0.3 — the storage-cask ladder is now solved, not guessed

Cask capacities are **legally defined**, which means the dimensions do not have to be found in a museum: they can be solved. Using Kepler's barrel rule for the internal volume,

```text
V = (pi * h / 12) * (2*D^2 + d^2)          # D = bilge dia, d = head dia, h = height
```

with the class ratios `h = 1.40 D` and `d = 0.86 D`, this reduces to `V = k·D³` and inverts directly. External dimensions add one stave thickness each side. Computed against the English beer ladder:

| Vessel | gal | litres | Internal D × h (mm) | **External D × h (mm)** |
|---|---:|---:|---|---|
| Firkin | 9 | 40.9 | 344 × 482 | **388 × 526** |
| Kilderkin | 18 | 81.8 | 434 × 607 | **478 × 651** |
| **Barrel** | 36 | 163.7 | 546 × 765 | **590 × 809** |
| Hogshead | 54 | 245.5 | 625 × 875 | **669 × 919** |
| Butt / pipe | 108 | 491.0 | 788 × 1103 | **832 × 1147** |
| Tun | 216 | 982.0 | 993 × 1390 | **1037 × 1434** |

Two checks were run and both pass. Kepler's rule collapses to the exact cylinder volume when `d = D`, and the firkin-to-tun diameter ratio comes out at 2.8845 against a predicted `(216/9)^⅓ = 2.8845` — so the cube-root rule is internally consistent with the solver.

**Unexpected cross-validation.** The v0.1 figure-derived estimate from the 1915 cooperage photograph was 0.60–0.70 m tall, while this solver puts a full barrel at 0.809 m — an apparent conflict. It is not one: 0.65 m is the **kilderkin** row, dead centre. The photograph was showing half-barrels, not barrels. Two independent methods agree once the vessel is correctly identified, which is the strongest evidence in this spec.

`Capacity` should therefore be an **input** to the storage-cask graph, with `Bilge Radius` solved from it, rather than a dimension the artist types. That also makes the volume assertion in §9 possible.

Taking the English beer barrel (36 gal) as anchor scale 1.000:

| Vessel | Capacity | Linear scale | Notes |
|---|---|---|---|
| Firkin | 9 gal | 0.630 | quarter barrel |
| Kilderkin | 18 gal | 0.794 | half barrel |
| **Barrel** | **36 gal** | **1.000** | anchor |
| Hogshead | 54 gal | 1.145 | |
| Butt / pipe | 108 gal | 1.442 | two hogsheads |
| Tun | 216 gal | 1.817 | rarely moved once full |

Non-liquid derivatives reuse the same groups with structural switches rather than new graphs:

- **Tub** — `Bilge Power` → 0.3 (nearly straight, slight taper), upper head suppressed, `Height / Bilge Diameter` → ~0.6.
- **Bucket** — tub proportions, stronger taper, plus a bail: `GeometryNodeCurveSpiral` is *not* right here; use a resampled line bent by a radius field, with ears instanced at two stave indices.
- **Churn** — tall tub, `Height / Bilge Diameter` → ~2.2, lid with a centre hole.
- **Half-barrel planter / trough** — barrel trimmed at the bilge; reuses staves and lower hoops untouched.

## 6. Node support

Verified 2026-07-28 against [catalog.json](/Users/kogaryu/Documents/Codex/2026-07-25/sinc/work/blender-capabilities/catalog.json) (Blender 5.1.1, build `b70da489d7f4`). Everything on the GH-007 and WPN-001 confirmed lists carries over. Additionally confirmed present and load-bearing here:

```text
GeometryNodeInstanceOnPoints
GeometryNodeRotateInstances
GeometryNodeScaleInstances
GeometryNodeRealizeInstances
GeometryNodeJoinGeometry
GeometryNodeTransform
GeometryNodeSetPosition
GeometryNodeMeshLine
GeometryNodeMeshCylinder
GeometryNodeCurveToPoints
GeometryNodeStoreNamedAttribute
GeometryNodeSetMaterial
GeometryNodeSetShadeSmooth
ShaderNodeMapRange
ShaderNodeVectorMath
```

Confirmed **absent**, with the consequence already folded into the design:

- `GeometryNodeCurvePrimitiveArc` — **there is no arc primitive.** The bilge is the analytic radius field in §1. Any implementation that reaches for an arc node is working from a stale assumption and should stop.
- `GeometryNodeMeshTorus` — no torus; hoops are swept circles (§2).
- Still no bevel/chamfer, lathe, knife/bisect, inset, loop-cut, bridge or solidify — unchanged from GH-007. Stave edge cant and the croze are handled analytically; nothing here may claim those nodes.

## 7. Evidence and parameters

Every parameter carries a confidence tier, because the library holds three different kinds of reference and they do not license the same claims:

- **Measured** — an accessioned object with published dimensions.
- **Figure-derived** — recovered from a depiction by scaling against a human figure, using the centesimal proportional canon in the anatomy volume (Bourgery pl. 1). Carries stated uncertainty.
- **Standard** — a documented historical convention, not an observation of a specific object.
- **Assumed** — a working default with no source. Flagged so it can be replaced.

**v0.2 update — measured coopered vessels now exist.** The v0.1 gap ("no cask in the dataset, the grep was matching *casket*") is partly closed. A Met Open Access pass found three American cedar vessels of 1700–1800 with published dimensions, all inspected at full resolution:

| Object | Met | Dimensions | What it settles |
|---|---|---|---|
| **Keg** — cedar and iron | [329](https://www.metmuseum.org/art/collection/search/329) | 52.1 × 26.7 cm | The measured anchor for the portable-keg class. Staved, six iron hoops, bung with turned stopper between two staples, carrying bail over the top. **The bilge is almost absent** — a keg is near-cylindrical, so `Head/Bilge Radius` ≈ 0.95, not the 0.86 assumed for barrels. |
| **Churn** — cedar | [2066](https://www.metmuseum.org/art/collection/search/2066) | 63.5 cm h, 20.3 × 17.8 cm | A **tapered truncated cone, wide end down, no bilge at all**, height ÷ diameter **3.13** — not the 2.2 "tall tub" v0.1 assumed. Roughly eleven **wooden** hoops in three clusters (base, waist, rim), flat and nearly flush. Lid with centre hole and a dasher through it. |
| **Piggin** — cedar | [5641](https://www.metmuseum.org/art/collection/search/5641) | H 15.9 cm | Small tapered vessel of the same woodenware class; one stave runs long as the handle. |

**Still unmeasured: the storage cask itself.** No accessioned barrel or hogshead has been found. The barrel row below stays figure-derived, and §10 item 1 stands — but it now has a measured sibling class to check against rather than nothing.

| Parameter | Value / range | Tier | Source |
|---|---|---|---|
| `Height` (barrel, 36 gal) | **0.809 m external** | **standards-derived** | Solved from legal capacity via Kepler's barrel rule, §5. Supersedes the v0.1 figure estimate. |
| `Bilge Diameter` (barrel) | **0.590 m external** | **standards-derived** | Same solve. Whole ladder firkin→tun in §5. |
| `Capacity` | 9–216 gal | **standards-derived** | English beer ladder; legally defined, so exact. Drives the solve. |
| (cross-check) photo estimate | 0.60–0.70 m | figure-derived | Lakes Creek cooperage ca. 1915. Matches the **kilderkin** row (0.651 m), not the barrel — the photo showed half-barrels. |
| `Stave Count` | 12–22, default 16 | figure-derived | Countable on the head-on barrels in the same photograph. |
| `Hoop Count` | 4–8, default 6 | figure-derived | Same photograph; working barrels carry more hoops than the decorative three. |
| `Head Radius / Bilge Radius` | 0.82–0.90, default 0.86 | standard | Cooperage practice. Not observed on a measured object. |
| `Height / Bilge Diameter` | 1.30–1.50, default 1.40 | standard | Cooperage practice. |
| `Stave Thickness` | 18–28 mm, default 22 mm | standard | Cooperage practice; machine-dressed staves hold a consistent gauge — see Hale below. |
| `Bilge Power` (storage cask) | 0.8–1.4, default 1.0 | assumed | Shape control, no source. Tune against the photograph. |
| `Bilge Power` (portable keg) | ~0 (near-cylindrical) | **measured** | Met 329 — the keg barely bulges. |
| `Head/Bilge Radius` (keg) | ~0.95 | **measured** | Met 329. |
| `Height / Diameter` (keg) | 1.95 | **measured** | Met 329, 52.1 ÷ 26.7 cm. |
| `Height / Diameter` (churn) | 3.13 | **measured** | Met 2066, 63.5 ÷ 20.3 cm. Replaces the assumed 2.2. |
| `Taper` (woodenware) | wide end down, no bilge | **measured** | Met 2066 and 5641 — truncated cone, not a bulged tub. |
| `Hoop Material` | iron \| wood | **measured** | Met 329 iron; Met 2066 wood. |
| `Hoop Grouping` | even \| clustered | **measured** | Met 2066 — ~11 wooden hoops in three clusters, not evenly spaced. |
| `Croze Depth` / `Croze Width` | 4 mm / 8 mm | assumed | Must be less than `Stave Thickness`; asserted in §9. |
| `Chime Height` | 25–40 mm | assumed | Overhang of stave beyond the head. |
| `Hoop Proud` | 2 mm | assumed | Standoff so the hoop catches light. |
| Capacity ladder | see §5 | standard | English beer and wine cask units. |

**Correction on the Hale patent.** An earlier note in this lane described *J. Hale's Patent Drawing for a Method of Making Casks, Barrels, Tubs, etc.* (21 June 1828) as a constructional source for cask geometry. Inspection shows it is filed under "Wood Working / Special Work Machinery / **Dressing Staves**" and depicts a stave-dressing machine — a rotating cutter head, belt drive and frame — in three views. It gives no cask proportions. Its one usable inference is that dressed staves hold a consistent machine-cut thickness and edge bevel, which supports treating `Stave Thickness` as a single value rather than a per-stave variable.

**Arrangement evidence** (from the same photograph, and binding on any scene that stacks casks): barrels stack on their sides in a pyramid and **cradle on the bilge**, so adjacent barrels touch at mid-height with visible gaps at the heads; and they stack on end in columns, chime to chime. A stack built as if casks were cylinders will read wrong immediately.

## 8. Redline framing

Renders exist so the art gate can be run without opening Blender. Each frame is specified to sit beside its source.

1. **Cooperage three-quarter** — standing eye height (1.60 m), 35 mm equivalent, cask at 2.5 m, with a 1.70 m neutral figure block in frame. Matches the Lakes Creek photograph, so scale can be judged directly against it.
2. **Plan from above** — orthographic, straight down. Stave count and joint closure are checkable here and nowhere else.
3. **Stack test** — a three-high side-stacked pyramid plus one column stood on end, same camera as (1). This is the frame that catches a cask modelled as a cylinder.
4. **Chime detail** — 100 mm crop at the upper chime showing head seat, croze, hoop standoff and stave ends together.

Frames 1 and 3 are the art gate. Frames 2 and 4 are the engineering gate.

## 9. Acceptance checklist

### Graph integrity
- Every socket resolved by `identifier`; no numeric index access.
- No node named in §6's absent list appears anywhere in the graph.
- Re-running the build script produces an identical graph (WPN-001 §3 contract).

### Evaluated geometry
- Instance count equals `Stave Count` exactly.
- Stave joint angles sum to 360° ± 0.01°. **This is the closure invariant; failing it means the vessel does not close.**
- Every hoop's radius equals `r(its height fraction) + Hoop Proud` ± 0.5 mm. Hoops must not float or sink.
- `Head Radius < Bilge Radius`, and head plane sits inside the croze, not flush with the chime.
- `Croze Depth < Stave Thickness`; `Chime Height > 0`.
- **Volume assertion (v0.3):** for the storage-cask class, the realized mesh's enclosed volume must match the declared `Capacity` within ±3%. This is the only check here that ties the geometry to a real-world invariant rather than to itself — if it fails, the shape ratios are wrong even though every other assertion passes.
- After `MergeByDistance`, no duplicate vertices within 0.1 mm and no non-manifold edges at the stave joints.
- Staves flat-shaded; smooth shading present only where explicitly set.

### Art gate
- Frame 1 placed beside the Lakes Creek photograph: silhouette and belly proportion judged by eye against a figure of known height.
- Frame 3: barrels cradle on the bilge, not on their heads.
- Bilge reads as a continuous curve at silhouette, with facets legible on the staves but not on the profile.

## 10. Outstanding

1. **Storage-cask dimensions — resolved by solving rather than searching (v0.3).** Capacities are legally defined, so the ladder is computed in §5 and cross-checks against the cooperage photograph once the vessel is identified as a kilderkin. A measured barrel would still be worth having as an independent confirmation, and the ratios `h/D = 1.40` and `d/D = 0.86` remain conventional rather than measured — those two numbers are now the weakest link in the whole spec, since everything else is solved from them.
2. Bung and tap-hole placement has no evidence yet; currently out of scope, and damage-side anyway.
3. Regional variation (French *barrique* versus English barrel stave counts and profiles) is unaddressed. Treat the current parameters as English working cooperage.

## Clear handoff

Build order: `SINC_GN_CaskStave` → `SINC_GN_CaskHoop` → `SINC_GN_CaskAssembly` (staves and hoops only, no heads) → assert the closure invariant → `SINC_GN_CaskHead` → the §5 derivations. Stop at the closure assertion and report before continuing; if the joints do not close, nothing downstream is worth building.
