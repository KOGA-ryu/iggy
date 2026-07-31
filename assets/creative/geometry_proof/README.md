# SINC_GeometryProof_Profile

One reliable geometry-proof system for 2D profiles and sections. A profile is
authored as an explicit, validated specification; the proof scene and the
production geometry both derive from the same compiled result, so a proof
that passes the eyeball gate *is* the asset geometry.

Status: **production candidate** - pending user acceptance review. Hardened
by a 41-agent adversarial review (36 findings raised, 15 confirmed by
refutation-resistant reproduction and repaired, 21 refuted - the inspectable
record is `REVIEW_RECORD.md`); 66 automated tests green (38 compiler + 18
intake + 6 proposer + 4 blend). See `BUILD_SPEC.md` for the reviewed design.

Profile inventory, badged honestly:

- `paley_pl1_fig6_v1` - the **reference-verified pilot**: plate-measured,
  overlay-registered, migration-checked against the old workflow. Flip A/B
  evidence retained in `output/paley_pl1_fig6_v1/flip_ab/`.
- `gok001_cavetto_fillet_b_v1` - **cross-check of the approved GOK-001
  CavettoFillet_B ledger geometry** (five stations E00-E04, normalized R=1,
  unequal lands, inverted quarter circle), built through the system with
  zero code changes. It does NOT discharge GOK001-CFB-P04: that ledger's
  no-advance rule requires its own v3 builder to run, reopen, and pass its
  six disproof views.
- `ovolo_v1`, `ogee_brandon_pl9_v1`, `stepped_paley_pl1_fig7_v1` -
  **vocabulary proofs**, not donors: authored stress exercises (evidence
  AUTHORED/ESTIMATED, `reference_placed: false` by design) that exist to
  prove inverted curvature, compound curves with both arc directions,
  multiple radii, smooth-join verification, straight-only outlines, and the
  opposite winding each pass the identical pipeline.
- `mm_rail_scale_check_v1` - **scale proof**, not a donor: a 40 x 18 mm rail
  retained so the millimetre capability (units `mm`, `scale_m_per_unit`
  0.001) has an artifact on disk, not just a fixed review finding.
- `brandon_pl9_tiebeam_v1` - **record-reconstructed section** (timber): the
  full 20 x 14 in tie-beam, the first profile with BOTH extents claimed
  against PRINTED dimensions, and the first to exercise mirroring+reversal
  (the right arris flips the left's arc-direction pattern). Left arris
  DERIVED/ESTIMATED from the recorded members with corrections on the
  record; right arris an AUTHORED mirror. Awaits a leaf n79 raster harvest
  for overlay verification before donor status.
- `receding_orders_paley_pl1_fig7_v1` - **the faithful fig 7**: three
  receding orders with the quarter coves struck in every re-entrant angle
  (the stepped variant deliberately omitted them). Five compiler-verified
  SMOOTH joins - the heaviest tangency load yet; the cove construction
  reproduces the recorded extent width 5.05 exactly, while the old builder
  yields 4.2 x 3.35 against its own record's 5.05 x 5.9.

Eight profile specs total; the flipped-bowtell build under `flip_ab/` is
retained A/B *evidence*, not a ninth profile.

## What exists

| file | owns |
|---|---|
| `BUILD_SPEC.md` | the reviewed design; code may not silently diverge |
| `INTAKE.md` | the research->harness bridge contract |
| `profile_spec.py` | spec schema + structural validation (stdlib only) |
| `profile_compiler.py` | spec -> geometry, THE single source; geometric validation |
| `profile_intake.py` | measurement handoffs -> draft specs (frames, arc fits, tolerance) |
| `profile_proposer.py` | handoff -> compiler-verified CANDIDATE spec (automated authoring lane) |
| `proof_scene.py` | headless bpy builder: .blend + 4 proof renders + manifest |
| `profiles/*.json` | one explicit spec per profile; `TEMPLATE.json` to author more |
| `handoffs/` | worked handoff example (also consumed by the intake tests) |
| `output/<id>/` | generated .blend, manifest, renders (generated - never hand-edited) |

Tests: `tests/unit/geometry_proof_compiler_tests.py` (pure python, 38 tests),
`tests/unit/geometry_proof_intake_tests.py` (18 tests - the fig 6 handoff
must round-trip into the hand-authored pilot's stations),
`tests/unit/geometry_proof_proposer_tests.py` (6 tests - direction follows
trace evidence, reversal flips it, solid side auto-resolves, unmatched
traces report loudly),
`tests/unit/geometry_proof_blend_tests.py` (reopens the built .blend in
Blender, 4 tests - including binding the artifact to the committed spec's
sha256 and binding the saved master curve to freshly recompiled geometry, so
neither a stale artifact nor a divergent rebuild can stay green).

Research intake: measurement threads deliver a `SINC_GeometryProof_Handoff/1`
JSON (calibration, crop frames, named landmarks, extents, arc traces);
`profile_intake.py` emits a draft spec with stations placed, claims filled,
and tolerance derived from the calibration residual - see `INTAKE.md`. First
live consumer: Ellis Plate 451A No.4
(`docs/building_corpus/sections/451A/handoff_451A_no4_DRAFT.json`).

## Reproduction

```
python3 tests/unit/geometry_proof_compiler_tests.py
/Applications/Blender.app/Contents/MacOS/Blender --background \
    --python assets/creative/geometry_proof/proof_scene.py -- \
    --spec assets/creative/geometry_proof/profiles/paley_pl1_fig6_v1.json \
    --out  assets/creative/geometry_proof/output/paley_pl1_fig6_v1
python3 tests/unit/geometry_proof_blend_tests.py
```

(`proof_scene.py` resolves module imports and the repo root from its own
location; run it with any working directory. `--flip SEGMENT` builds an
A/B variant with one arc reversed; `--skip-render` skips the PNGs.)

The saved `.blend` is self-contained: it embeds the spec JSON, the compiler,
the schema, this builder, and `rebuild_proof.py`. Inside Blender: edit the
`profile_spec.json` text datablock, run `rebuild_proof.py`, and the master
curve, fills, annotations and extrusion regenerate from the edited spec.

## Production consumption

Asset scripts consume accepted profile donors through
`profile_compiler.moulded_chain_points(spec)`. They do not recreate profile
geometry. The proof extrusion and the master curve in the .blend are the same
compiled outline.

## Measured comparison against the old cavetto workflow

The old workflow (`docs/blender/reference_manifests/moulding_profile.py`)
chained members by accumulated heading with curvature hidden in sign
conventions. Its fig 6 record needed cycles 22-29 - six recorded correction
cycles - and still carried defects the new system's validation exposed on
first compile:

| old-workflow defect | magnitude | new-system mechanism |
|---|---|---|
| hollow stopped short of the measured chamfer start (sweep under-derived) | 0.24 in | arcs end at named stations; centre/radius mismatch rejected |
| final chamfer displaced, poking THROUGH the soffit plane | 0.23 in past the stock edge | closed outline + claims; migration test pins the corner to the plane |
| unclosed break at the arris between chamfer and fitted arc | 0.056 in | segments chain by shared station names - gaps are unrepresentable |
| roll exit-heading convention disagreed with its own sampled arc direction by 180 deg; the hollow was chained off the wrong heading and compensated invisibly | silent | direction is an explicit reviewed property; tangents are computed from it and joins are validated |
| open chain could not state solid vs void (cycle-27 open question) | structural | closed outline, declared solid_side, winding check, SOLID/VOID in every render |
| measured hollow/chamfer triple mutually inconsistent - invisible to the old system | 0.021 in | simple-polygon check caught the crossing; resolved explicitly and recorded in the spec |

Build cost now: one spec compile + full proof scene + 4 renders in ~2.5 s;
the flip gate (reverse one arc, everything regenerates) is one CLI flag or
one text edit inside the .blend.

## Honest remaining risks

- The reference overlay placement (`crop_px_at_origin`/`crop_px_per_unit`)
  for the pilot is eyeball-estimated (~±10 px); good enough that misreads are
  visible, not measurement-grade. A measured crop is the known upgrade.
- The fig 6 upper-face routing (plate y~722, cycles 26-27) is deliberately
  NOT asserted; closure follows the uncut stock face and the spec notes the
  open question. Solid area in the upper-left bounding region may change if
  that question is ever settled; the moulded edge cannot.
- Workbench-vs-EEVEE: renders use EEVEE emission materials (flat diagram
  look). Nothing here claims engine parity for any consumer.
- BEZIER segments are schema-complete and unit-tested but no accepted profile
  uses one yet.
- Proven envelope, stated precisely: closed simple single-outline sections of
  lines and circular arcs in the XZ construction plane, at the scales and
  curvatures of the eight retained profiles (largest: the 20 x 14 in
  tie-beam; smallest: the 40 x 18 mm rail; arcs from 45 to 267 degrees,
  both directions, mirrored and reversed). Cubics are schema-complete and
  unit-tested but appear in NO retained profile - profile-level cubic
  evidence does not exist yet. Validation runs on the sampled polygon at
  the configured density; extreme curvature, extreme aspect ratios,
  near-degenerate tolerances, and multi-outline sections (holes, islands)
  are outside what has been demonstrated.

## Boundary

Included: 2D profiles/sections, reference overlay, explicit arcs, solid/void,
hard/smooth joins, one proof extrusion, eight profile specs through one
unchanged pipeline (badging above - one reference-verified, one
approved-ledger cross-check, two record-reconstructed sections, four
vocabulary/scale proofs).
Excluded (deliberately, see BUILD_SPEC.md section 0): assemblies, joints,
automatic image interpretation, materials/wear/LODs/Unreal, generic CAD.
`SINC_GeometryProof_Joint` and `_Assembly` remain unstarted until the user
closes this side quest.
