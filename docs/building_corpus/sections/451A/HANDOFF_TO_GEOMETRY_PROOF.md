# Handoff: 451A No.4 → SINC_GeometryProof_Profile

A profile-authoring harness now exists at `assets/creative/geometry_proof/`
(see its `INTAKE.md`). It replaces the `moulding_profile.py` loop this
cycle's README points at — same parametric-from-landmarks philosophy, plus
validation that loop could not do: explicit arc direction, closed-outline
solid/void checked against winding, tangency claims verified, parameter
claims (your measured schedule) checked against the built geometry, and a
proof scene rendered from the same compiled result the asset consumes.

`handoff_451A_no4_DRAFT.json` (this directory) already carries the No.4
calibration, frames, and full measured schedule in the harness's intake
format. What it still needs from this thread:

1. **Landmark assembly** — named 2D points (`[x, y]` in a declared frame)
   for the profile corners: the scanline bands in the CSV notes pair into
   these once the `no4_grid` crop parentage is confirmed.
2. **Oak-slip head trace** — 5+ sampled points along the curved head, any
   declared frame; intake least-squares fits the circle and reports
   residuals (the fig 6 bowtell pattern).
3. **M04/M05 resolution** against the Ellis text (or leave them
   INTERPRETATION-OPEN; intake maps that honestly to ESTIMATED).

Then:

```
python3 assets/creative/geometry_proof/profile_intake.py \
    docs/building_corpus/sections/451A/handoff_451A_no4_DRAFT.json --out /tmp/451a_draft.json
```

Running it BEFORE the landmarks exist is also useful: the validation errors
are exactly the remaining checklist. After intake, segment/join/solid-side
authoring happens in the draft spec, and `proof_scene.py` renders the
overlay-verified proof.
