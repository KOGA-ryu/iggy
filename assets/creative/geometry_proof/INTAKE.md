# Research → harness intake

The bridge between measurement threads (plate decompositions, measured
schedules, arc traces) and the proof harness. One contract, one tool, one
honest boundary.

## The pipeline

```
research thread                          harness (this package)
--------------                           ----------------------
measure plates, calibrate scale,
name landmarks, trace curves     ──►     handoff JSON  (SINC_GeometryProof_Handoff/2)
                                              │
                                              ▼
                                         profile_intake.py
                                           frame resolution, arc fits,
                                           tolerance from control-point uncertainty
                                              │
                                              ├──► DRAFT spec (hand-author segments...)
                                              │
                                              ▼
                                         profile_proposer.py  (the automated lane)
                                           chains stations in traversal order,
                                           attaches fitted arcs, picks direction
                                           from trace evidence, proves SMOOTH
                                           joins, compile-flips solid_side
                                              │
                                              ▼
                                         CANDIDATE spec  (compiler-verified)
                                              │   human review: roles, evidence,
                                              │   datum, extrusion
                                              ▼
                                         profile spec ──► proof_scene.py ──► visual gate
                                                                        ──► accepted donor
```

The proposer never invents geometry: stations connect in declared order,
ARCs appear only where a traced circle passes through both endpoints within
tolerance, direction follows where the trace samples actually lie, SMOOTH is
claimed only where computed tangents already agree, and the compiler referees
every proposal. Traces matching no station pair are reported as owed
landmarks, never dropped. Final acceptance stays with the visual gate.

## What the research thread delivers (the handoff)

See `handoffs/EXAMPLE_paley_pl1_fig6_handoff.json` for the retained `/1`
worked example (it is also consumed by the intake tests, so it cannot rot).
The intake remains backward-compatible with `/1`; new workbench exports use
`SINC_GeometryProof_Handoff/2`. Fields:

- **calibration** — px per unit, `control_point_uncertainty_px`, and the
  `source_projection_model`. The uncertainty becomes the spec's
  `tolerance.distance` (2 × uncertainty), so measurement quality directly
  sets how strict the geometry validation is. If the projection depends on
  an authored reading such as `fronto_parallel_subject`, `/2` carries that
  `model_assumption` as structured data with its evidence class. The legacy
  `/1` field `rms_residual_px` remains readable but is never emitted by new
  workbench exports because it misnamed authored point uncertainty as a fit
  residual.
- **frames** — every working crop/magnification declared as a frame with
  `offset_px` + `scale` relative to its parent; exactly one root. Landmarks
  can then be recorded in whatever crop they were measured in, and intake
  resolves them to the root frame. This kills the "which image were those
  scanline numbers in" failure class.
- **landmarks** — named 2D points with evidence classes. These become
  stations.
- **arc_traces** — sampled points along curved members (5 minimum). Intake
  least-squares fits a circle, reports centre/radius/residuals in profile
  units, and suggests FITTED or ESTIMATED evidence depending on whether the
  fit lands inside tolerance.
- **extents** — the scalar schedule (the 451A CSV shape: measure_id, value,
  status, method). These become spec `parameters`; rows with `between`
  landmarks become verified `distance` claims.
- **open_questions** — carried into the draft's TODO list, never silently
  dropped (451A's M04/M05 class).

## What intake deliberately does NOT do

It never declares segments, joins, roles, arc directions, or solid side -
those are geometry *readings*, the authoring judgment this system makes
explicit. The emitted draft **fails compilation by design** until an author
completes it, and the report lists exactly what remains. No pixels are ever
interpreted: only declared measurements move through. Phase 0's "no automatic
image interpretation" boundary survives intact.

## Commands

```
python3 profile_intake.py handoffs/EXAMPLE_paley_pl1_fig6_handoff.json --out /tmp/draft.json
```

Run it on an incomplete handoff and the validation errors are the checklist
of what the research thread still owes - the 451A stub
(`docs/building_corpus/sections/451A/handoff_451A_no4_DRAFT.json`) is shipped
in exactly that state on purpose.

## Proof the bridge works

`tests/unit/geometry_proof_intake_tests.py` round-trips the fig 6 example:
the handoff's plate-pixel landmarks must come out as the same station
coordinates the pilot spec was hand-authored with (wall_start exact,
chamfer_top exact, hollow_end 0.021 in away - the documented consistency
nudge), the bowtell trace must re-fit to the recorded centre/radius, and the
emitted draft must fail structural validation (segments undeclared) - the
honesty property, asserted.
