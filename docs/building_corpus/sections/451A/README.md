# Plate 451A decomposition — cycle 6 record (Phase 2 opened)

**Source**: Ellis 1902, leaf n153 = **printed 136, read off the page** (per the drifting-offset
rule; this also pins the drift curve: offset −17 holds through n153, then grows to −22 @ n175,
−32 @ n300 as plate leaves accumulate). Plate title: *"451A. Nos. 1–8. Details of Improved
Methods of Constructing Weatherproof Sash and Casement Windows."* Source raster caps at
1901 × 2700 (probed at w=3000/4000 — the `w` trap measured, both ways).

## The eight details (VERIFIED at full resolution)

| no | subject |
|---|---|
| 1 | Section through a French casement (full vertical: head, fanlight, transom, draught check, water check, sill — all labelled) |
| 2 | Sketch of double-hung sashes (isometric: parting bead, top/bottom sash) |
| 3 | Section at striking jamb of single-light casement frame |
| 4 | **Section at meeting stiles of pair of casements, with oak slip** ← this cycle's target |
| 5 | Section at hanging jamb |
| 6 | Alternative joint for meeting stiles |
| 7 | Section of boxing of a double-hung sash frame (weights drawn) |
| 8 | Section through a cased sash frame (full vertical) |

Details 3 + 4 + 5 together form **one complete horizontal plan section across a casement
window** — jamb to jamb.

## Calibration

The plate carries its own printed 12-inch scale bar. Least-squares over its 13 major ticks:
**70.401 px/inch**, RMS residual 1.66 px = **0.024 in (0.6 mm)** at object scale, residual
signs alternating (no curvature trend). The first inch is divided into eighths; they
independently give 70.0 px/in. One pixel = 0.36 mm; 1/16 in ≈ 4.4 px — comfortably readable.

## No.4 measured schedule

See [`measurements_451A_no4_v1.csv`](measurements_451A_no4_v1.csv) — 13 quantities, each with
pixels, inches, nearest workshop size, method, and status. Headlines:

- **stile depth 1.911 in ≈ 1 15/16 — the classic finished 2-inch casement stock**
- glass plane set back 0.611 in (≈ ⅝) from the outer face; glass drawn 3/16 thick
- meeting stiles 2¼ / 2 3/16 at the face band — the asymmetry is **real at that level**
  (bevelled lap joint), not an error; nominal stock is likely equal
- oak slip: head 1½ wide × ⅜ tall, projecting 9/32 above the face, total depth 1 15/16 —
  it runs nearly the full stile depth

## Joint anatomy (read at 3×)

A **bevelled, slip-feathered meeting joint**: both stiles bevelled on a diagonal meeting
plane; the oak slip's moulded head covers the joint on the weather side; its feather/tongue
runs down between the bevelled faces; a small bead element terminates at mid-depth. This is
the weatherproofing idea the plate exists to teach — the joint has no straight-through path
for wind or water.

## Method verdicts (both proven on this plate)

1. **Scanline-median edge measurement works** on hatched engravings: hatching terminates at
   the outline, so the median outermost-ink extent over a row band is the member edge. All
   scalar measurements above came from this, cross-checked against the drawn scale.
2. **Blob tracing fails exactly where the Paley doctrine predicts**: wood-to-wood contact
   merges members into one component (the slip trace wandered into both stiles at the
   contact faces — overlay kept as proof in the working cache). Curved profiles must be
   **authored parametrically from measured landmarks and verified by overlay**, the
   moulding_profile.py loop — not pixel-traced.

## Working images (session cache, re-derivable from the recipe above)

`plate451A_upright.jpg` (rotate 90 CW), `scalebar.png` (crop 1290,680 + 120×1100),
`strip345.png` (crop 1330,500 + 350×1650), `no4_grid.png` (crop 520,30 + grid 35 + ×2),
`slip_trace_overlay.png` (the failed-merge proof, ×3).

## A worklist blind spot this plate exposed

**Leaf n153 has zero rows in `section_inventory_v1.csv`.** The plate's captions are
*engraved in the artwork*, not typeset, so the OCR-caption extractor never saw them — the
richest object found so far was invisible to the text-derived worklist and was reached only
because the Phase-1 audit's random sample landed on the leaf. Quantified: **Ellis has 63
leaves under 200 OCR characters, Mitchell 1898 has 62** — front/back matter aside, roughly
fifty plate-candidate leaves per book in the same blind class. Mitigation for the Phase-2
queue: a **plate sweep** — contact-sheet triage of the low-text leaves (the tooling exists:
`iabook.contact_sheet`), which complements the caption worklist rather than replacing it.

## Cycle 7 — the No.4 profiles, authored and overlay-verified

**[`profiles_451A_no4_v1.json`](profiles_451A_no4_v1.json)** — two parts in inches, shared
origin (outer face × bay edge), every vertex carrying a status:

- **`casement_stile_left`** (closed, 15 vertices): bay side fully measured — weathering
  splay (~23° from vertical), glass slot, proud rebate-cheek fillet down to the y-225 line
  (now identified as the cheek's bottom step), relieved inner body, rounded bottom arris.
  Meeting side: a **diagonal bevelled lap**, provisional-visual. Glass rebate root depth
  provisional. Slip seating notch in the face OPEN.
- **`oak_slip_head`** (closed, 36 vertices): the exposed weather moulding — flat fillet
  shoulders at +0.11 in, a **double-lobed crown** carried as an empirical polyline from the
  per-column top-profile (arc fitting deferred), crests at ~+0.37 in above the face.

**The overlay loop worked exactly as doctrine says.** Iteration 1 *confirmed* the entire
bay side and the crown, and *refuted* my provisional vertical meeting face — the drawn
joint is diagonal, top corner toward the meeting, bottom corner drawn back. Iteration 2
sits. One measurement approach failed honestly along the way: the joint's thin white gap
line cannot be isolated numerically from the hatch gaps (both are white slivers of similar
width), which is why the meeting bevel carries `provisional-visual` rather than `measured`.

Assembly datums emitted with the parts: glass plane +0.611/+0.788 in, cheek line +1.186 in,
inner face +1.911 in.

Overlay proofs in the working cache: `no4_authored_overlay.png` (iteration 1, the refuting
one — kept deliberately), `no4_authored_overlay_v2.png` (the sitting fit).

## Next

Author the two No.4 profiles parametrically from this schedule — the casement stile
(rectilinear: stock + glazing rebate + bevel) and the oak slip (curved head: landmark
read → arc fit → overlay verify). Then No.3 and No.5 complete the full window plan section,
and No.1/No.8 give the verticals. M04/M05 interpretations to be settled against Ellis's
own text (the weathering chapter, printed 124, already VERIFIED).
