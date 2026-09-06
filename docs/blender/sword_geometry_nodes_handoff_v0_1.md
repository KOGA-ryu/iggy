# Sword Geometry Nodes handoff — WPN-001 (applying the GH-007 research to weapons)

Companion to the GH-007 door node research. Same Blender 5.1.1 target (build `b70da489d7f4`), same `SINC_` library, same Python/idempotency contract. Target asset: **WPN-001, one-handed cruciform arming sword** — the most reusable European archetype (blade, cross guard, wrapped grip, pommel). Katana-family weapons are explicitly out of scope for v1 (different construction, different wrap topology; see §5).

The central conclusion inverts the door's. The door failed where hand-hewn character was the point: adzes and broken fibers are irregular by nature, and Geometry Nodes noise reads as melted wood. A sword blade is the opposite object — it is **ground** geometry. Its ideal form *is* analytic: planes, tapers, and grooves produced by deliberate stock removal. Geometry Nodes can therefore deliver the entire clean "armory grade" sword — blade with correct cross-sections and crisp grind lines, fuller, tang, guard, grip, wound wrap, pommel, assembly, masks, and export. What they still cannot deliver is **damage and ornament**: nicked and rolled edges, bent blades, engraving and inlay, ornate finials, basket hilts, mushroomed peens. Those remain direct geometry or authored cutters, exactly as GH-007 concluded for adze planes.

Two consequences follow:

1. The escape-hatch budget shrinks. A door needed direct geometry to look *right*; a sword needs direct geometry only to look *used*.
2. The discipline flips sign in one place: timber wants controlled bow and twist; a blade wants **dead straightness**. Any bow, warp, or seed-driven bend on a blade reads as a defect. Straightness is a functional invariant, not a variation axis.

## Conventions

Component-local axes, consistent with GH-007 stock:

- Blade, guard bar, grip, pommel: **X = length** (the long axis), Y = width, Z = thickness.
- Blade datum: **x = 0 at the shoulder** (blade/tang transition where the guard seats). Blade occupies `[0, Blade Length]`, tang occupies `[−Tang Length, 0]`.
- Assembly frame = blade frame. +X points at the tip, hilt components stack down −X. Unlike the door (boards rotated into door space), no component rotation is needed for the blade — the assembly's spine is the blade's own axis.

Connection notation and socket-identifier rules are identical to GH-007:

```text
NodeType["Output"] → NodeType["Input"]
```

Sockets resolve by runtime `identifier`, never numeric index.

## 1. Recommended reusable groups

Build order rationale is in §5. Shader mask contracts are deliberately shared with GH-007 so existing shader groups drive sword hardware unmodified.

### `SINC_GN_SwordBlade`

The first node. Its accepted v1 produces controlled ground stock: tapers, cross-section, fuller, ricasso, tang. No damage.

Interface:

| Socket | Type | Meaning |
|---|---|---|
| `Blade Length` | `NodeSocketFloat`, distance | Shoulder to tip |
| `Tang Length` | `NodeSocketFloat`, distance | Shoulder to tang end |
| `Base Width` | `NodeSocketFloat`, distance | Full width at shoulder |
| `Base Thickness` | `NodeSocketFloat`, distance | Full thickness at ridge/spine, at shoulder |
| `Profile Taper` | `NodeSocketFloat`, factor | Width at tip zone start ÷ base width |
| `Distal Taper` | `NodeSocketFloat`, factor | Thickness at tip ÷ base thickness |
| `Tip Length` | `NodeSocketFloat`, distance | Convergence zone at the point |
| `Section` | menu via `NodeSocketMenu` | `Diamond` / `Hexagonal` / `Lenticular` |
| `Bevel Shoulder` | `NodeSocketFloat`, factor | Hexagonal only: fraction of half-width where the edge grind starts |
| `Edge Land` | `NodeSocketFloat`, distance | Residual edge thickness (0.2–0.5 mm); never zero |
| `Ricasso Length` | `NodeSocketFloat`, distance | Unsharpened rectangular zone above the shoulder (0 = none) |
| `Fuller Enable` | `NodeSocketBool` | |
| `Fuller Width` | `NodeSocketFloat`, distance | Full groove width |
| `Fuller Depth` | `NodeSocketFloat`, factor | Fraction of **local** half-thickness, clamped ≤ 0.8 |
| `Fuller Start`, `Fuller Length`, `Fuller Runout` | `NodeSocketFloat`, distance | Groove extent and smooth exit |
| `Length Segments` | `NodeSocketInt` | X resolution (64–128) |
| `Tang Width Factor`, `Tang Thickness Factor` | `NodeSocketFloat`, factor | Tang stock relative to base dims |
| `Nick Count` | `NodeSocketInt` | Sparse edge nicks (default 0) |
| `Nick Size` | `NodeSocketVector` | Cutter dimensions |
| `Nick Depth` | `NodeSocketFloat`, distance | Max cut depth into the edge |
| `Damage Cutter Collection` | `NodeSocketCollection` | Authored notch/roll cutters |
| `Use Damage Cutters` | `NodeSocketBool` | |
| `Seed` | `NodeSocketInt` | Cosmetic only |
| `Steel Material` | `NodeSocketMaterial` | |
| `Geometry` | output | Blade + tang mesh |

Core topology:

```text
GeometryNodeMeshCube["Mesh"]                 # blade stock
    → GeometryNodeSetPosition["Geometry"]    # feature-aligned ring remap (Y)
    → GeometryNodeSetPosition["Geometry"]    # analytic section/taper/tip field
    → GeometryNodeStoreNamedAttribute ×4     # edge / fuller / tang(=0) / damage masks
GeometryNodeMeshCube["Mesh"]                 # tang stock
    → GeometryNodeSetPosition["Geometry"]    # tang taper
    → GeometryNodeStoreNamedAttribute        # tang mask = 1
    → GeometryNodeJoinGeometry               # with blade; join, do not union
    → GeometryNodeMergeByDistance            # tip vertices only, by selection
    → [optional nick Boolean branch]
    → UV branch → GeometryNodeSetMaterial → NodeGroupOutput["Geometry"]
```

Blade and tang are separate joined shells so the shoulder step stays exact — the GH-007 rule about not chasing sharp functional steps through one deformed grid, applied here.

**Analytic field.** All shaping is one reconstruction of `(x, y·s_y, z·s_z)`, no noise anywhere:

```text
t = MapRange(x, 0 → Blade Length, 0 → 1)
w(t)  = (Base Width / 2)      · mix(1, Profile Taper, t)   # half-width
h0(t) = (Base Thickness / 2)  · mix(1, Distal Taper, t)    # half-thickness at ridge
tip zone: over the last Tip Length, w and h0 multiply by a
          smoothstep envelope converging toward Edge Land / 2
y_n = |y| / w(t)
```

Half-thickness `H(y, t)` per section family, selected by `GeometryNodeMenuSwitch`:

```text
Diamond:     H = max(h0 · (1 − y_n), Edge Land / 2)
Hexagonal:   H = min(h0, s · (w − |y|) + Edge Land / 2)
             s = h0 / ((1 − Bevel Shoulder) · w)
Lenticular:  H = max(h0 · sqrt(1 − y_n²), Edge Land / 2)
Fuller:      H −= (Fuller Depth · h0) · bump(|y|, Fuller Width)
                 · runout(x)          # MapRange, Smooth Step interpolation
Ricasso:     H = mix(h0, H, smoothstep over Ricasso Length)   # slab → section
```

Final position: `z' = z_norm · H` where `z_norm = z / (Base Thickness / 2)`, and `y' = y_ring(t)` (below). Because the field acts on *half*-thickness and `z` scales symmetrically, both flats and both fullers are mirror-exact by construction — section symmetry is not a thing that can drift.

Two structural points that make or break the art gate:

- **Crisp grind lines come from `min()`/`max()` of analytic planes, not from a bevel node.** GH-007 correctly flagged that no GN bevel/chamfer exists. For ground steel none is needed: the hexagonal section's grind line is the intersection of the flat plane and the edge plane, and `ShaderNodeMath MINIMUM` produces exactly that crease. The same trick chamfers the pommel (§ pommel).
- **Feature-aligned rings.** A crease is only crisp if a vertex ring lies exactly on it. Do not use the cube's uniform Y distribution. Fix `Vertices Y` to a small odd count (11–13) and remap each ring by index to a semantic station computed from the live parameters — centerline/ridge, fuller floor, fuller wall, flat mid, bevel shoulder, edge land — mirrored. Implementation: `ring = round(y_uniform_n · (N−1))` → `GeometryNodeIndexSwitch` selecting the station position (catalog-confirmed, §6). This is the blade equivalent of hand-modeled edge loops, and it is why the grind line survives grazing light. A uniform grid softens every crease across one cell width and fails the gate.

**Tip.** The envelope converges width and thickness toward `Edge Land / 2`; the residual micro-cluster at the point is welded by `GeometryNodeMergeByDistance` with the selection restricted to `x > Blade Length − ε`. Never merge globally — the edge land is legitimate sub-millimetre geometry.

**Nick branch** (disabled by default, mirrors GH-007 chips):

```text
GeometryNodeMeshLine        # stations along the edge, count = Nick Count
    → GeometryNodeSetID
    → GeometryNodeSetPosition   # bounded jitter < spacing/2 → non-overlap by construction
    → GeometryNodeInstanceOnPoints  ← cutter cube via GeometryNodeGeometryToInstance
    → GeometryNodeRealizeInstances
    → GeometryNodeMeshBoolean DIFFERENCE["Mesh 2"]
```

`FunctionNodeRandomValue` (ID = station ID, Seed = group seed) controls only: station jitter, which edge, cutter scale/rotation inside tight intervals, depth within `[0, Nick Depth]`. A `GeometryNodeProximity` result against the realized cutters is stored as `sinc_steel_damage_mask` **before** subtraction. Warning carried over from GH-007 and amplified: this is the library's highest-risk Boolean — thin edge geometry meeting small cutters. v1 ships with nicks off and a shader-only nick comparison; mesh nicks are accepted per-cut or not at all.

Named attributes (point domain unless noted):

- `sinc_steel_edge_mask` — from `y_n` via MapRange; 1 at the edge land, falloff inward
- `sinc_steel_fuller_mask` — the fuller bump field itself
- `sinc_steel_tang_mask` — 1 on tang shell, 0 on blade
- `sinc_steel_damage_mask` — proximity, only when nicks/cutters run
- `UVMap` — 2D vector, Face Corner domain

UV: mark seams analytically (edge rings where `y_n ≈ 1`, plus the shoulder), then `GeometryNodeUVUnwrap` → `GeometryNodeUVPackIslands` → store as `UVMap`, same as GH-007. Two flat islands, edge-land strip, tang island.

Seed-controlled: nick placement fields (when enabled), nothing else. **Not** seed-controlled: every dimension, taper, section parameter, fuller geometry, tang, straightness. There is no "character" randomness on a blade; character is the shader's job and the damage tier's job.

### `SINC_GN_SwordGuard`

Straight cruciform cross guard: bar + central block (écusson) + exact tang slot.

Interface: `Span`, `Bar Width`, `Bar Thickness`, `End Taper` (factor), `Arc Sagitta` (distance; 0 = straight bar, small values curve the quillons toward the blade), `Block Width/Height/Depth`, `Tang Slot Width`, `Tang Slot Thickness`, `Fit Epsilon`, `Seed`, `Iron Material`, `Geometry` out.

```text
GeometryNodeMeshCube["Mesh"]            # bar, local X = span
    → GeometryNodeSetPosition           # end taper + arc: z += Sagitta · (1 − (2t−1)²)
GeometryNodeMeshCube                    # block
    → GeometryNodeJoinGeometry
    → GeometryNodeMeshBoolean DIFFERENCE["Mesh 1"]   # ONE cutter: tang slot prism
    → masks → UV → GeometryNodeSetMaterial → out
```

The slot cutter is `Tang Slot ± Fit Epsilon` in Y/Z and oversized in X so no cutter face is coplanar with the block — the GH-007 Boolean discipline verbatim. Slot position is the origin. Functional, never seeded.

Masks: store `sinc_iron_edge_mask` (distance to bar edges) and `sinc_iron_hammer_mask` (low-frequency 4D noise, W from seed). **These are the GH-007 iron contract — the existing `SINC_SH_ForgedIron` group must drive the guard with zero modification.** That reuse is the point of sharing the library.

Curved/S-quillons with finials: v2 via a `GeometryNodeCurvePrimitiveBezierSegment` spine + `GeometryNodeCurveToMesh` sweep (nodes confirmed, §6). Hero finials (beast heads, knots): direct mesh, always.

### `SINC_GN_GripCore`

Interface: `Length`, `Radius`, `Oval Ratio` (Y/Z asymmetry), `Waist` (factor, mid-grip narrowing), `Swell` (factor, end swelling), `Wood Material`, `Geometry` out.

```text
GeometryNodeMeshCylinder                # local X = length, Side Segments ≥ 24
    → GeometryNodeSetPosition           # radial field r(t) = Radius · profile(Waist, Swell, t); y·Oval
    → masks → GeometryNodeSetMaterial → out
```

Bare wooden grips store the `sinc_wood_*` masks from GH-007 so `SINC_SH_WornWood` drives them unmodified. No tang hole — the hilt stack hides it; drilling an invisible hole is a Boolean spent on nothing.

### `SINC_GN_GripWrap`

The rivet-grade Geometry Nodes win of this asset: helical wire/cord wrap by instancing, exact where exactness is wanted.

Interface: `Length`, `Base Radius`, `Pitch`, `Strand Count`, `Strand Radius`, `Strand Shape` (menu: `Round Wire` / `Flat Lace`), `Handedness` (bool), `Riser Count`, `Riser Radius`, `Ferrule Enable`, `Ferrule Length/Radius`, `Seed`, `Wrap Material`, `Iron Material`, `Geometry` out.

```text
GeometryNodeCurveSpiral                  # Rotations = Length / Pitch,
                                         # Start/End Radius = Base Radius + Strand Radius
    → GeometryNodeResampleCurve
    → GeometryNodeCurveToMesh ← profile: GeometryNodeCurvePrimitiveCircle (wire)
                               or flattened GeometryNodeCurvePrimitiveQuadrilateral (lace)
per-strand: duplicate the spiral, rotate k·2π/Strand Count about the axis   # multi-start helix
risers:  GeometryNodeCurvePrimitiveCircle → GeometryNodeCurveToMesh (circle profile)
         → instanced at stations          # torus substitute — no GeometryNodeMeshTorus exists (§6)
ferrules: GeometryNodeMeshCylinder ×2, iron masks
    → GeometryNodeJoinGeometry → masks → materials → out
```

The spiral primitive runs along Z; transform once onto local X. Store `UVMap` from the spline factor (captured with `GeometryNodeCaptureAttribute` before `Curve to Mesh`) × profile parameter — curve sweeps do not produce UVs by themselves.

Seed-controlled: nothing in v1. A real wrap is wound under tension: constant pitch, both strands tight. Randomized pitch or strand wobble reads as sloppy work, not age. If micro-variation is ever wanted, it enters as one clamped sub-millimetre phase term, cosmetic review required.

Katana tsukamaki (crossing diamond wrap) is **not** this group — it is a woven pattern, not a helix, and needs its own research if the katana family opens.

### `SINC_GN_SwordPommel`

Revolved-profile families via the radial-field pattern, not a lathe node (none exists).

Interface: `Family` (menu: `Disc` / `Wheel` / `Scent-Stopper`), `Radius`, `Thickness`, `Chamfer`, `Neck Radius`, `Neck Length`, `Peen Block` (bool), `Seed`, `Iron Material`, `Geometry` out.

```text
GeometryNodeMeshCylinder                 # local X = axis, Side Segments dense
    → GeometryNodeSetPosition            # radial field: r_target(x) / r_cylinder scales y,z
    → optional peen block (small cylinder, joined)
    → masks → GeometryNodeSetMaterial → out
```

`r(x)` per family is piecewise analytic. Chamfers and the wheel's rim/hub step use the same `min()`/`max()`-of-planes trick as the blade's grind line, in `(r, x)` space — crisp revolve creases with zero Booleans, provided rings are feature-aligned on X exactly as the blade aligns them on Y. Iron mask contract as per guard. Ornate pommels (brazil nut, fishtail, zoomorphic): direct mesh. A mushroomed hand-peened tang end: direct mesh; the optional `Peen Block` is the clean machined form only.

### `SINC_GN_SwordAssembly`

Composition only — every component already accepted on its own.

Interface: pass-throughs for the four components, `Enable Guard/Wrap/Pommel`, `Master Seed`, `Steel/Iron/Wood/Wrap Material`, `Realize For Export`, `Geometry` out.

The stack is a **datum ledger, all functional, none seeded** (x = 0 at the shoulder):

```text
blade:   [0, Blade Length], tang to −Tang Length
guard:   seats against the shoulder, occupies [−Block Depth, 0]
grip:    [−(Block Depth + Grip Length), −Block Depth]
pommel:  centered at −(Block Depth + Grip Length + Neck Length + Thickness/2)
invariant: Tang Length ≥ Block Depth + Grip Length + pommel seat depth
```

That invariant — the tang must reach into the pommel — is checked by the validation script (§7), not trusted.

```text
4 × GeometryNodeGroup(components)
    → GeometryNodeTransform each to its datum
    → GeometryNodeJoinGeometry
    ├─→ procedural output
    └─→ GeometryNodeRealizeInstances
        → GeometryNodeSeparateComponents["Mesh"]
        → GeometryNodeTriangulate
        → GeometryNodeSwitch(data_type=GEOMETRY, switch=Realize For Export)
        → NodeGroupOutput
```

Child seeds: `FunctionNodeHashValue` over `(Master Seed, role constant, index)` — blade 1, guard 2, grip 3, wrap 4, pommel 5, fasteners 6 — so enabling the wrap never reseeds the guard. The GH-007 For Each Geometry Element zone is **not needed** here (each component exists once); it becomes relevant again for armory racks and battlefield scatter, where per-instance seed variation of whole swords is exactly what the zone is for.

Grip pins/rivets through the tang: reuse `SINC_GN_ForgedFasteners` from GH-007 unchanged, points supplied at functional stations.

### `SINC_SH_ForgedSteel`

`ShaderNodeTree` group; the material owns `ShaderNodeOutputMaterial`.

Interface: `Vector`, `Seed`, `Steel Color`, `Patina Color`, `Fresh Metal Color`, `Grind Scale`, `Grind Strength`, `Polish Amount`, `Patina Amount`, `Pitting Scale`, `Pitting Amount`, `Anisotropy`, `Roughness Min`, `Roughness Max`, `Bump Strength`, `Bump Distance`, `BSDF` out.

```text
Vector → ShaderNodeMapping
    ├─→ ShaderNodeTexWave      # BANDS along X, stretched hard via Mapping scale = grind striations
    ├─→ ShaderNodeTexNoise 4D  # broad temper/patina variation, W = Seed
    ├─→ ShaderNodeTexNoise 4D  # fine roughness breakup, W = Seed + offset
    └─→ ShaderNodeTexVoronoi   # sparse pitting
masks: ShaderNodeAttribute ×4  # sinc_steel_edge/fuller/tang/damage_mask
edge mask   → roughness ↓, polish ↑            # freshly honed edge band
fuller mask → slight roughness/striation shift # ground groove reads distinct
tang mask   → forge-black, rough, no polish    # honest hidden-surface finish
damage mask → Fresh Metal Color, low roughness # bright cuts in dark steel
Metallic = 1.0;  Anisotropy → ShaderNodeBsdfPrincipled["Anisotropic"]
    with ShaderNodeTangent along X             # striations polarize highlights lengthwise
fine height → ShaderNodeBump → Principled["Normal"] → Group Output["BSDF"]
```

Hamon is a mask-driven `ShaderNodeValToRGB` treatment and stays **shader-only, deferred** with the katana family. Seed controls both `W` values and grind phase — nothing else. Same Unreal warning as GH-007, plus one sharper: standard base-color/normal/roughness/metallic bakes **do not carry anisotropy**; either rebuild the anisotropic response in the Unreal material or accept isotropic highlights (decide at the art gate, not at export time).

### Shader reuse (no new work)

- `SINC_SH_ForgedIron` — guard, pommel, ferrules, fasteners, as-is via the `sinc_iron_*` contract.
- `SINC_SH_WornWood` — bare wooden grips, later scabbard bodies, as-is via `sinc_wood_*`.
- New leather for wrapped grips: `SINC_SH_WrappedLeather` (`Vector`, `Seed`, `Leather Color`, `Wear Color`, `Pore Scale`, `Crease Strength` driven by the wrap's UV V-coordinate so creases follow the winding, `Roughness Min/Max`, `Bump`, `BSDF`). Restrained; folds and overlap geometry are direct-mesh hero work, not shader tricks.

## 2. Geometry Nodes versus direct geometry

| Operation | Verdict |
|---|---|
| Profile / distal taper | Clean: analytic `mix` over `t` |
| Diamond / hexagonal / lenticular section | Clean: half-thickness field |
| Crisp grind lines | Clean: `min()` of planes + feature-aligned rings — no bevel node needed |
| Fuller with run-out | Clean: subtractive analytic groove; **never** a Boolean |
| Ricasso transition | Clean: smoothstep blend slab → section |
| Tip convergence | Acceptable: envelope + selection-restricted Merge by Distance |
| Flamberge (wavy blade) | Clean v2: periodic analytic width modulation — controlled, not noise |
| Single-edge sections (falchion) | Acceptable v2: asymmetric field in signed y |
| Curved saber blade | v2: analytic arc bend or curve spine; v1 stays straight |
| Clip point / tanto tips | Mix: piecewise envelopes acceptable; hero tips direct |
| Edge nicks | Cautious: sparse cutters, per-cut acceptance; shader nicks first; thin-edge Booleans are the library's top failure risk |
| Rolled edge, torn metal, bent blade | Direct mesh (damage tier) |
| Engraving, inlay, maker's marks | Shader/decal; deep engraving direct |
| Hamon | Shader only, ever |
| Wire/cord grip wrap | Excellent: spiral + curve-to-mesh instancing |
| Katana crossing wrap (tsukamaki) | Direct / own research — a weave, not a helix |
| Leather wrap folds and overlaps | Shader for creases; direct for hero folds |
| Riser rings | Clean: curve-circle sweep (no torus primitive exists — verified §6) |
| Guard bar + exact tang slot | Clean with exactly one Boolean |
| Curved / S quillons | Acceptable v2 via bezier sweep; finials direct |
| Basket / swept hilts | Direct mesh; curves assist scaffolding only |
| Pommel revolve families | Clean: cylinder + radial field, `min()` chamfers |
| Ornate / zoomorphic pommels | Direct mesh |
| Peen block | Clean; mushroomed hand peen direct |
| Hilt assembly stacking | Excellent: functional datum ledger |
| Material masks | Excellent: named attributes, shared contracts |
| Export realization | Clean: realize → separate mesh → triangulate |

## 3. Python and idempotency contract

Identical to GH-007 — that document is canonical for: `interface.new_socket` creation, the direction-checking `connect()` wrapper, copy/build/validate/remap staged updates, never `user_clear()`, semantic `sinc_key` identity, digest-gated rebuilds, asset marking and catalog UUIDs from `blender_assets.cats.txt`, and evaluated-mesh extraction via `evaluated_depsgraph_get()` / `new_from_object`. Do not fork those patterns for weapons. Sword-specific additions:

- **Menu sockets are interface contract.** `Section`, `Strand Shape`, and `Family` menus: item names, order, and identifiers enter the graph digest, and modifier-value migration maps menu values by item name, never by index. A reordered menu that silently changes every stored blade from Diamond to Hexagonal is an interface break, not a cosmetic edit.
- **`GeometryNodeIndexSwitch` item counts** (ring-station tables) are likewise digest-covered — a changed station count re-rigs the blade's crease topology.
- **`ShaderNodeFloatCurve` is widget-authored, not socket-drivable.** Blade and pommel profiles therefore stay math-driven so sockets keep control. If a FloatCurve is ever used for a baked-in family profile, its curve points must be serialized into the digest or the "same recipe, same digest" guarantee silently dies.
- **Validation script duties** (with the §7 checklist): `GeometryNodeAttributeStatistic` / Python-side equivalents assert width and thickness monotonicity along stations (no hourglass), fuller web ≥ minimum everywhere against **local** `h0(t)`, centerline straightness, and the tang-reach invariant via `GeometryNodeBoundBox` per component before join.

## 4. Fragile approaches to reject

All GH-007 rejections stand (old interface API, `bpy.ops.node.*`, numeric socket indexing, deprecated Euler nodes, `ShaderNodeMixRGB`, `user_clear()`, Boolean chains, coplanar cutters, render-completion-as-approval, procedural-shader-through-FBX, unrealized instance export). Sword-specific additions:

- Any noise on the blade silhouette, flats, or grind planes. Melted steel reads as fake faster than melted wood.
- Seed-driven blade bend, warp, or bow. A bent blade is authored damage-tier content.
- Boolean fullers. The groove is analytic; a Boolean groove buys debris and soft walls with the library's scarcest currency.
- Uniform Y rings when any crease family (grind line, fuller wall) is enabled — creases land between rings and soften.
- Bevels faked via Subdivision Surface — it destroys exactly the creases the blade exists to show.
- Randomized wrap pitch or strand wobble.
- Seeded functional fit: slot dimensions, tang dimensions, seat positions, fastener stations.
- Nick cutters allowed to overlap each other, the fuller, or the tip in one unordered Boolean.
- A zero-thickness edge (`Edge Land = 0`) — degenerate side faces, shading artifacts, and Boolean failures on contact.

## 5. Smallest useful implementation order

1. **`SINC_GN_SwordBlade` v1** — straight double-edged arming blade; all three sections; fuller; ricasso; tang; masks; UV; material. No nicks, no damage. Stop and judge one blade in front, profile, grazing, and point-on views, plus flat clay. If the grind line is soft or the flats ripple, the node is rejected regardless of test status.
2. **`SINC_SH_ForgedSteel`** — restrained; verify striations and the honed-edge band support the grinds rather than disguise them.
3. **`SINC_GN_SwordGuard`** — the one Boolean; prove `SINC_SH_ForgedIron` drives it unmodified.
4. **`SINC_GN_GripCore` + `SINC_GN_GripWrap`** — the high-reuse pair: the wrap group also serves axes, maces, spears, and tool handles.
5. **`SINC_GN_SwordPommel`** — revolve families, `min()` chamfers.
6. **`SINC_GN_SwordAssembly`** — datum ledger, hash-seeding, realize-for-export, validation script with the invariants from §3.
7. **Nick branch** — enabled only behind per-cut visual acceptance, compared against the shader-only alternative first.
8. **Direct-geometry damage/ornament tier** — rolled edges, hero notches, engraved fullers, ornate finials — only after a clean armory sword is approved.

Then variants in rough order of leverage: flamberge modulation, falchion single-edge section, saber arc, and — as its own research doc, not a parameter — the katana family (different blade construction, hamon, tsuba, woven wrap).

Review loop note: acceptance renders go out as contact sheets in the four canonical views (matching the existing `*-stages` / contact-sheet practice). Redlines happen on renders; nothing in this pipeline ever requires the reviewing artist to open Blender.

## 6. Exact locally available node support

Verified 2026-07-28 by name-grep against [catalog.json](/Users/kogaryu/Documents/Codex/2026-07-25/sinc/work/blender-capabilities/catalog.json) (Blender 5.1.1, build `b70da489d7f4`, 540 node types). Everything on the GH-007 confirmed list carries over. Additionally confirmed present, and load-bearing for this proposal:

```text
GeometryNodeCurveToMesh
GeometryNodeCurvePrimitiveCircle
GeometryNodeCurvePrimitiveLine
GeometryNodeCurvePrimitiveQuadrilateral
GeometryNodeCurvePrimitiveBezierSegment
GeometryNodeCurveSpiral
GeometryNodeResampleCurve
GeometryNodeSetCurveRadius
GeometryNodeSetCurveTilt
GeometryNodeSampleCurve
GeometryNodeTrimCurve
GeometryNodeFillCurve
GeometryNodeFilletCurve
GeometryNodeSplineParameter
GeometryNodeMeshToCurve
GeometryNodeMergeByDistance
GeometryNodeExtrudeMesh
GeometryNodeScaleElements
GeometryNodeSubdivideMesh
GeometryNodeMeshCircle
GeometryNodeMeshGrid
GeometryNodeFlipFaces
GeometryNodeDuplicateElements
GeometryNodeSplitEdges
GeometryNodeCaptureAttribute
GeometryNodeAttributeStatistic
GeometryNodeBoundBox
GeometryNodeMenuSwitch
GeometryNodeIndexSwitch
GeometryNodeSampleIndex
ShaderNodeFloatCurve      # widget-authored; see §3 caveat
ShaderNodeClamp
ShaderNodeTangent
```

Confirmed **absent**, with consequences already folded into the design:

- `GeometryNodeMeshTorus` — no torus primitive; riser rings and any annular part are curve-circle sweeps.
- Still no bevel/chamfer, lathe, knife/bisect, inset, loop-cut, bridge, or solidify node — unchanged from GH-007. The blade sidesteps bevel and lathe analytically (`min()` planes; radial fields); nothing here may claim them as nodes.
- `ShaderNodeBsdfAnisotropic` exists but is not used: Principled's anisotropy inputs keep one BSDF path and one bake story.

## 7. Acceptance checklist

### Graph integrity

All GH-007 items apply verbatim (confirmed identifiers only, no deprecated nodes, no `bpy.ops.node.*`, no numeric sockets, idempotent rebuilds, digest stability, user/value survival, recoverable previous group), plus:

- [ ] Menu-socket item sets and identifiers are digest-covered; menu values migrate by name.
- [ ] IndexSwitch station tables are digest-covered.
- [ ] No FloatCurve outside digest coverage.

### Evaluated geometry

- [ ] Dimensions match the recipe within tolerance.
- [ ] Width and thickness are monotone non-increasing shoulder→tip outside the ricasso (no hourglass), asserted by station sampling.
- [ ] Centerline straightness: zero deviation within float tolerance. Any bend is a build failure, not character.
- [ ] Cross-section mirror-symmetric within tolerance for double-edged families.
- [ ] Edge land continuous shoulder→tip, thickness within `[Edge Land min, max]`; never zero.
- [ ] Grind-line crease lies on a vertex ring (dihedral localized to one ring, not smeared).
- [ ] Fuller web thickness ≥ minimum **everywhere**, computed against local `h0(t)` under distal taper.
- [ ] Tip welded: no degenerate faces or zero-area fans after Merge by Distance.
- [ ] Tang reaches the pommel seat; hilt stack seats gapless within epsilon (BoundBox per component).
- [ ] Closed, manifold, positive-volume shells; no Boolean debris (nick builds inspected per cut).
- [ ] Seed changes affect only approved cosmetic fields; same seed reproduces the same evaluated mesh.
- [ ] Masks survive realization; UV0 bounded with intentional seams; wrap UVs present from spline capture.
- [ ] Export branch is realized, mesh-only, deliberately triangulated; material slot order stable.

### Unreal handoff

GH-007 baking rules apply (bake or rebuild all shaders; base color/normal/roughness/metallic; masks baked or exported as intentional vertex colors; UV1 if lightmaps; collision authored separately — a box/capsule pair suffices for a sword; normals/tangents checked after import), plus:

- [ ] Anisotropic grind response either rebuilt in the Unreal material or consciously dropped — decided at the art gate, recorded in the asset notes.
- [ ] Pivot/axis convention recorded: origin at the shoulder datum, +X to the tip; engine grip-socket offset documented per weapon.

### Art gate

- [ ] Silhouette reads as a credible sword typology — believable length/width/taper proportions, acute centered point.
- [ ] Grazing light: flats dead-flat; grind line one crisp straight crease; fuller walls crisp, floor smooth, run-out clean.
- [ ] Edge highlight runs continuous and straight shoulder→tip.
- [ ] Hilt reads as assembled components with real seats — visible part boundaries, not one extruded blob.
- [ ] Wrap reads wound under tension: constant pitch, correct and consistent handedness on both faces.
- [ ] Guard and pommel read forged (mask-driven variation), the blade reads ground — two different metal histories on one object.
- [ ] The sword still works in flat clay with procedural materials disabled.
- [ ] A human explicitly approves the evaluated shape from the four-view contact sheet.

## Clear handoff

Build `SINC_GN_SwordBlade` first, with only analytic ground-stock shaping: tapers, one of three sections, fuller with run-out, ricasso, tang, masks, UV. Feature-aligned rings and `min()`-plane creases are the two techniques that make it work; noise on the silhouette and seeded bends are the two ways to fail it instantly. One blade must pass grazing-light review before any shading, guard, wrap, pommel, or assembly work begins. The clean armory sword is fully within Geometry Nodes' reach — closer than the door ever was, because a blade's ideal form is analytic. Damage and ornament are not: nicked, rolled, bent, engraved, and ornate remain direct geometry, authored and accepted piece by piece. A green graph that produces a soft grind line is a failed sword.
