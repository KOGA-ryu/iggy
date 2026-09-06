# SINC_GeometryProof_Profile — Build Specification v1

One reliable geometry-proof system for 2D profiles and sections. Proven on four
profiles, then stopped. This document is the reviewed design; code may not
silently diverge from it.

## 0. Boundary (frozen)

Included: 2D profiles/sections, reference overlay, relative proportions, lines +
circular arcs + cubic Bézier segments, solid/void orientation, hard and smooth
joins, one small proof extrusion.

Excluded: furniture assemblies, automatic image interpretation, materials, wear,
LODs, Unreal, validation *programs* (beyond this system's own tests), generic
CAD replacement. `SINC_GeometryProof_Joint` and `_Assembly` are explicitly
deferred until Phase 7 completes.

## 1. Tooling decision record

- **Pure-Python spec + compiler, headless bpy for the scene.** No Geometry
  Nodes dependency, no add-ons (CAD Sketcher, MeasureIt, fSpy imports are all
  rejected for the production path). Verified on this machine: Blender 5.1.1,
  python3 available, no numpy/PIL — the compiler uses stdlib only.
- Rationale: the repo's existing proof culture is deterministic Python +
  headless builds + reopen-the-.blend tests (`forged_fasteners_v1` pattern).
  Editable geometry survives because the *spec is the editable input*, embedded
  in the .blend as a text datablock with a bundled rebuild script.
- fSpy stays out of scope as tooling; perspective-rectification *eligibility*
  is a Phase 7 documentation deliverable only.

## 2. Package layout

```
assets/creative/geometry_proof/
  BUILD_SPEC.md            this document
  INTAKE.md                research->harness handoff contract
  profile_spec.py          schema + validation (pure Python, stdlib only)
  profile_compiler.py      spec -> CompiledProfile (THE single geometry source)
  profile_intake.py        handoff JSON -> draft spec (frames, arc fits, tolerance)
  proof_scene.py           bpy builder: CompiledProfile -> .blend + renders
  profiles/
    TEMPLATE.json          annotated authoring template (Phase 7)
    paley_pl1_fig6_v1.json cavetto pilot (Phase 4 donor)
    ovolo_v1.json          Phase 5.1
    ogee_brandon_pl9_v1.json  Phase 5.2
    stepped_paley_pl1_fig7_v1.json  Phase 5.3
  output/<profile_id>/
    <profile_id>_proof.blend
    <profile_id>_manifest.json
    renders/front.png opposite.png three_quarter.png wire.png
tests/unit/geometry_proof_compiler_tests.py   (pure python, always runs)
tests/unit/geometry_proof_blend_tests.py      (skipUnless Blender installed)
```

## 3. The single-source invariant

`profile_compiler.compile_profile(spec)` is the only producer of geometry.
The proof annotations, the master curve, the solid fill, the extrusion, and any
future production consumer all read the same `CompiledProfile`. There is no
second authoring path: the .blend contains no independently drawn curve, and
the Bézier-approximation trap is explicitly rejected — the master curve is the
compiler's sampled polyline, not a hand-tuned approximation of it.

Acceptance test for the invariant: rebuild with `direction` flipped on one arc
and every derived object (proof arc, arrow, fill, extrusion) must change
together. That is Phase 4's `arc_direction` flip gate.

## 4. Profile specification format

One JSON document per profile. Versioned, diff-reviewable, no hidden state.
Everything geometrically ambiguous is an explicit property.

```json
{
  "format": "SINC_GeometryProof_Profile/1",
  "id": "paley_pl1_fig6_v1",
  "reference": {
    "source": "Paley, Manual of Gothic Mouldings 1845, Plate I fig 6",
    "image": "docs/blender/paley_pl1_fig6_overlay.png",
    "scale_px_per_unit": 22.09,
    "source_frame": "plate pixels, y-down, 1930x2951 scan"
  },
  "units": "in",
  "datum": "stock_width",
  "tolerance": { "distance": 0.1, "angle_deg": 2.0 },
  "construction_plane": {
    "plane": "XZ",
    "profile_x": "+X (wall-line direction)",
    "profile_y": "+Z (up, away from wall-line)",
    "extrude_axis": "+Y",
    "scale_m_per_unit": 0.0254
  },
  "stations": {
    "wall_start": {
      "at": [0.0, 0.0],
      "source_px": [1004, 772],
      "evidence": "MEASURED",
      "join": "HARD"
    }
  },
  "segments": [
    {
      "name": "wall_line",
      "kind": "LINE",
      "from": "wall_start",
      "to": "wall_end",
      "role": "BOUNDING",
      "evidence": "MEASURED"
    },
    {
      "name": "bowtell",
      "kind": "ARC",
      "from": "arris_cusp",
      "to": "bead_exit",
      "centre": [8.44, 2.29],
      "direction": "CLOCKWISE",
      "role": "MOULDED",
      "evidence": "FITTED"
    }
  ],
  "solid_side": "LEFT",
  "parameters": {
    "stock_width": { "value": 10.91, "claim": { "kind": "free" } },
    "bowtell_radius": { "value": 2.29,
                        "claim": { "kind": "radius_of", "segment": "bowtell" } }
  },
  "proof": { "extrusion_depth": 2.0 },
  "notes": ["free-text provenance, corrections, open questions"]
}
```

### 4.1 Frames

- **Profile frame**: 2D, y-UP, source units (`units`). Origin and axis meaning
  declared in `construction_plane` prose fields. For Paley material the frame
  is his own: wall-line horizontal, soffit vertical (cycle-28 correction is
  the law here).
- **Source frame**: plate pixels, y-down. Each station may carry `source_px`.
  Validation: `source_px` mapped through `scale_px_per_unit` + the declared
  frame must land within `tolerance.distance` of `at`, using the first station
  as the registration origin. Catches transcription errors and axis swaps —
  the cycle-22/28 error classes.
- **Reference image placement**: the proof scene places the reference plane
  from two optional `reference` fields — `crop_px_at_origin` (the pixel of
  the *reference image file* that coincides with profile origin, y-down from
  the image's top-left) and `crop_px_per_unit` (image pixels per profile
  unit). These are distinct from `scale_px_per_unit`, which describes the
  *original plate scan* and drives only `source_px` registration — a cropped
  or magnified overlay image has its own scale. If an `image` is declared
  without both crop fields, no plane is placed and the manifest says so.
- **Tolerance is also the feature-size floor**: any non-ARC segment whose
  endpoints are within `tolerance.distance` is rejected as degenerate, so no
  representable feature may be smaller than the declared tolerance. Choose
  `tolerance.distance` below the smallest real member.
- **Normalized stations** are *derived*, never authored: `at / parameters[datum].value`.
  The compiler emits them; the manifest records them. Authored-normalized
  would create a second source of truth.
- **Construction plane**: profile (x,y) → world (X,Z), extrusion along +Y,
  front camera looks along +Y. `scale_m_per_unit` converts to metres (factory
  authors in metres).

### 4.2 Stations

Named 2D points. `at` in profile units. `evidence` from §4.5. `join` declares
the transition where the two adjacent outline segments meet at this station:

- `HARD`: corner is intentional. Never smoothed, never bevelled. If tangents
  are actually continuous within `tolerance.angle_deg` the compiler emits a
  WARNING (suspicious authoring), not an error.
- `SMOOTH`: tangent-continuous is a *claim*. The compiler computes both
  tangents and REJECTS the spec if they disagree beyond `tolerance.angle_deg`.
  A smooth join is proven, not drawn.

### 4.3 Segments and the closed outline

`segments` is an **ordered closed loop**: each segment's `to` is the next
segment's `from`, and the last closes to the first. This is the cycle-27
resolution adopted as law: *a moulding section is a closed outline, not an
open profile chain.* The moulded face remains distinguishable via `role`:

- `MOULDED` — the carved edge. Production consumers that want "the profile"
  read exactly the MOULDED subchain.
- `BOUNDING` — wall-line, soffit, upper face, stock closure runs. Drawn
  differently in the proof (dashed, muted), included in the solid fill and
  extrusion.

Segment kinds:

- `LINE`: from → to.
- `ARC`: `centre` [x,y] + `direction` ∈ {`CLOCKWISE`, `COUNTERCLOCKWISE`},
  as seen in the y-up profile frame (= exactly what the front camera shows).
  Radius is DERIVED: |centre−from| must equal |centre−to| within
  `tolerance.distance`, else rejection. Sweep is derived from the endpoints in
  the declared direction; must be in [0.5°, 360°) — sub-half-degree sliver
  arcs are rejected outright, because a near-zero sweep between
  almost-coincident stations is almost always a direction flip, not a member.
  **No accumulated headings, no sign conventions, no handle coordinates** —
  the cycle-24 error class is unrepresentable.
- `BEZIER`: cubic, `handle_from` + `handle_to` as absolute points. Present to
  honour the frozen boundary; the four pilot profiles use only LINE/ARC.

### 4.4 Solid and void

`solid_side` ∈ {`LEFT`, `RIGHT`} of the traversal direction. The compiler
closes the outline and verifies it is simple: proper crossings, improper
touches (a vertex on another edge's interior), colinear overlaps, and
adjacent-edge doubling-back are all rejected, as is an outline whose enclosed
area is at or below `tolerance.distance²` (winding is undefined there). It
then computes the signed area and REJECTS the spec if the declared side
contradicts the winding.
The proof scene fills the interior (solid) and stamps a void marker outside the
moulded edge. Wrong-side-of-the-line is thereby a build failure, not a render
surprise.

### 4.5 Evidence classes

Per station and per segment, aligned with the dossier vocabulary:

| class | meaning | proof colour |
|---|---|---|
| `PRINTED` | dimension printed on the plate | green |
| `MEASURED` | pixel-measured on the scan | green (dark) |
| `FITTED` | least-squares fitted to a traced contour | blue |
| `DERIVED` | computed from other measured members | cyan |
| `ESTIMATED` | read by eye, plausible proportion | orange |
| `AUTHORED` | authored translation / invention, labelled | magenta |

Never silently promote a class. The proof render makes weak evidence visible.

### 4.6 Parameters (relative parameters as validated claims)

`parameters` are named values with machine-checkable claims, not a macro
language. Claim kinds: `free` (documentation only), `radius_of` (segment),
`length_of` (segment), `angle_of` (LINE, absolute vs +x), `distance`
(station pair), `extent` (`width`/`depth` of the full outline). The compiler
verifies every claim against the geometry within tolerance. Printed dimensions
become falsifiable instead of decorative. Editing geometry without updating a
claimed parameter is a rejection, by design.

## 5. Compiler output (`CompiledProfile`)

- sampled closed outline (list of (x,y), default 24 samples per quarter turn,
  arc endpoints and stations always exact vertices — hard corners are exact);
- the MOULDED chains: open polylines with distinct endpoints; a fully-MOULDED
  profile yields one chain equal to the closed outline (first point not
  repeated);
- per-station: position, incoming/outgoing tangents, join kind, evidence;
- per-ARC: centre, radius, sweep_deg, direction, midpoint (for arrows);
- normalized stations (÷ datum);
- signed area, winding, extents;
- validation report: errors (reject) + warnings (manifest);
- deterministic: same spec → identical floats; no time, no randomness.

## 6. Proof scene (`proof_scene.py`, headless bpy)

Collections and objects, all `SINC_`-prefixed:

- `SINC_Reference` — image plane with `reference.image`, in the construction
  plane behind the profile, placed and scaled via `crop_px_at_origin` +
  `crop_px_per_unit` (see §4.1; `scale_px_per_unit` is the plate-scan scale
  and does not place the image).
- `SINC_Master` — ONE curve object: closed poly spline from the compiler
  samples, 2D fill, extruded `proof.extrusion_depth` → this single object is
  simultaneously the editable master, the solid fill, and the proof extrusion.
- `SINC_Annotations` — source-station markers (spheres, evidence-coloured),
  arc centres (small cross meshes) + radius guide edges to both endpoints,
  arc-direction arrows at arc midpoints, tangent arrows at SMOOTH stations,
  hard-corner diamonds at HARD stations, void marker.
- `SINC_Cameras` — `SINC_Cam_Front` (ortho, along +Y), `SINC_Cam_Opposite`
  (ortho, along −Y), `SINC_Cam_ThreeQuarter` (persp), `SINC_Cam_Wire`
  (three-quarter framing; wire render shows edge/curve objects only).
- Text datablocks: `profile_spec.json` (THE editable input),
  `profile_spec.py`, `profile_compiler.py`, `rebuild_proof.py` (run inside
  Blender → revalidate + regenerate everything from the JSON text). The .blend
  is self-contained and reopens editable with zero add-ons.
- Renders: EEVEE with emission-only materials (flat diagram look, no
  lighting dependence), deterministic; four PNGs.
- Manifest: spec sha256, blender version, object inventory, extents,
  normalized stations, validation warnings, render list. Generated, never
  hand-edited.

## 7. Acceptance gates

**Phase 4 (cavetto pilot, first donor):**

1. Arc orientation matches the reference overlay (visual, front render).
2. Solid and void unmistakable (fill + void marker, front render).
3. Hard shoulders hard (corner markers; sampled vertices exact at stations).
4. Flipping `direction` on the bowtell arc changes proof AND extrusion in one
   rebuild (headless A/B compare of sampled geometry + re-render).
5. No duplicate independently authored curve (object census in blend test).
6. Saved .blend reopens with spec text + rebuild script + master curve intact.
7. Front / opposite / three-quarter / wire agree (all four rendered from the
   same master object — agreement is structural, verified visually).
8. Migration check: MOULDED subchain within 0.1 in of the old
   `moulding_profile.build()` polyline for fig 6 (shape parity with the
   corrected cavetto), while additionally expressing closure + solid side the
   old format could not.

**Phase 5 (vocabulary):** ovolo (opposite curvature), ogee (two arcs, an
inflection, SMOOTH join between them), stepped (straight-only, HARD joins,
zero arcs). Each goes through the identical pipeline with zero bespoke code;
any bespoke requirement = stop and repair the system first.

**Phase 7 (stop):** template + rectification-eligibility list + measured
old-vs-new comparison, then the side quest ends.

## 8. Error classes this design kills (traceability)

| historical failure | mechanism here |
|---|---|
| cycle 22: figure number read as dimension | parameters carry claims checked against geometry |
| cycle 22/28: axes swapped | source_px registration validation vs declared frame |
| cycle 24: arc on wrong side (sign convention) | explicit centre + named direction; no headings |
| cycle 23/24: member identity wrong (hollow vs chamfer) | per-segment kind + evidence + overlay markers |
| cycle 27: open chain can't say what's solid | closed outline + declared, winding-checked solid_side |
| proof drawing ≠ asset geometry | single CompiledProfile feeds both; flip gate proves it |

## 9a. Research intake (added after the 451A measurement thread)

Measurement threads hand off ONE document (`SINC_GeometryProof_Handoff/2`,
contract in INTAKE.md): calibration with explicit control-point uncertainty,
source projection model and any authored model assumption; working-crop frames
as declared transforms; named 2D landmarks with evidence; scalar extents with
statuses; optional arc traces. The retained `/1` records remain readable.
`profile_intake.py` resolves frames, fits circles (Kasa least-squares,
residuals reported), derives `tolerance.distance` = 2 x control-point
uncertainty, and emits a DRAFT spec that
fails compilation until an author declares segments, joins, roles, arc
directions, and solid side. Intake never interprets pixels - the Phase 0
boundary stands. Proof: the fig 6 handoff example round-trips into the
hand-authored pilot's stations (tested).

`profile_proposer.py` automates the mechanical share of that authoring:
stations chained in declared traversal order, ARCs attached only where a
fitted trace circle passes through both endpoints, direction chosen by
where the trace samples lie, SMOOTH claimed only where computed tangents
already agree, solid_side resolved by compile-and-flip. Every proposal is
compiler-refereed; roles, evidence, datum and the visual gate stay human.

## 9. Out-of-scope temptations (rejected now, on the record)

Expression language in parameters; automatic tracing; Geometry Nodes mirror of
the compiler; per-profile bespoke builders; Bézier approximation of arcs as the
master; committing generated output without review. Keep changes uncommitted
until the user asks.
