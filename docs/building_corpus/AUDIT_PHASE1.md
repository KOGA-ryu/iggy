# Phase-1 audit (2026-07-31)

Auditor: Claude, taking over the Codex slot (Ace's instruction). Because the author and
auditor are the same, the audit is **mechanical and adversarial by design** — recomputation
and image spot-checks with a fixed random seed, not re-reading of claims. Findings below
include the defects found and fixed; nothing was silently repaired.

## A. Mechanical checks — PASS

- **value_m recomputation** from the printed value string: 137 parseable rows, **0
  mismatches**. (29 rows are non-ft/in formats — French/mixed — excluded from this check.)
- **Witness check** (every value digit present in its quote): 28 flags, all 28 resolved to
  witnesses in non-digit form — French number-words (*soixante-quatre*), Roman numerals
  (*xx pedum*), printed fractions ("9 1/2"), spelled English ("forty-four"). **0 true
  witness failures.**
- **Self-checking sums**: Kensworth 11+11=22, Walton 21.5+12=33.5, Gunthwaite 15+15=30 —
  all close to <5 mm.
- **Ellis rule value_m**: recomputed, all correct (two apparent failures were this audit
  script's own tolerance being tighter than the stored 5-decimal rounding).

## B. Defects found and fixed

1. **B163 witness-form drift** (found by the witness check): the Lavenham jetty value had
   been normalised to "1 ft 6 in" where the quote says "about 18 inches". Value now carries
   the quote's own form. Generator note records why.
2. **Emit-format wart** exposed by that fix: `0 ft 18 in` — the orphan value formatter
   didn't handle ft=0. Fixed.

## C. Section-inventory spot-check — the calibration

8 rows sampled with **seed 42** (6 Ellis + 2 Mitchell, proportional), each page fetched and
inspected at full resolution:

| entry | leaf | verdict |
|---|---|---|
| S057 | ellis n121 | topic ✓ (SCREEN DOORS); figure in adjacent slice — documented straddle |
| S012 | ellis n42 | **FALSE POSITIVE** — tools page; `glazing` fired on "glass tube"/"glass-paper" |
| S142 | ellis n175 | ✓ correct (LIFTING SHUTTERS; fig 497 referenced by name) |
| S127 | ellis n164 | topic ✓ (SKYLIGHTS); straddle; **bonus: fig 475 visibly dimensioned (2'-9½", 11½)** |
| S116 | ellis n153 | topic ✓; the page is plate 451A — **8 weatherproof sash/casement sections with a printed inch scale** |
| S071 | ellis n127 | topic ✓ (PREPARING A DOOR); process figures; straddle |
| S370 | mitchell n213 | ✓ correct (dimensioned sash-in-solid-frame plate, figs 426–429) |
| S329 | mitchell n58 | **FALSE POSITIVE** — masonry glossary; "fig 1" was an OCR split of "figure 116"; `door` tag from stray words |

**Measured: 2/8 noise, 4/8 correct-with-straddle, 2/8 fully correct.** n=8, so the
confidence interval is wide — treat 25% as an estimate, not a rate.

**Filters applied from the noise post-mortem** (mechanical causes only — no threshold
chasing against the sample): bare "glass" dropped from the glazing pattern; `glazing`
can no longer qualify a leaf alone; OCR-split figure numbers killed by lookahead;
window/door keywords must hit ≥2 per leaf. Inventory 384 → **289 rows** (218 priority-1,
71 priority-2); the 253 excluded figures are counted, not hidden. Residual topic noise
**persists** (n58 still qualifies via stray mentions in slice text) — MAPPED status means
what it says: verify at full resolution before use, which is Phase 2's normal operation.

## D. Incidental findings worth keeping

- **Ellis offset, third datum**: −22 at n175 (−17 @ n141, −32 @ n300) — smooth drift,
  consistent with uncounted plate leaves. Mitchell −13 confirmed a third time (n213→200).
- **First direct proof that Ellis figures carry dimensions** (fig 475) and that plate 451A
  carries a **printed scale bar** — meaning even unlabelled sections on that plate are
  measurable. 451A goes to the top of the Phase-2 queue.
- **A rules-lane candidate spotted on the page image**: panel fitting allowance — *"cut off
  to size, nett length, and ⅛ in. narrow (some joiners cut them 1/16 in. small all
  round)"* (Ellis, printed 110). Directly relevant to the furniture panel family too.
  Not added mid-audit; queued.

## Verdict

Phase 1 **passes** with the two fixed defects and the calibrated inventory. The corpus's
dimensional layer is clean under recomputation; its witness discipline held; the worklist
is honest about what it is. Remaining risks are the documented ones: Ellis's drifting
pagination, residual inventory noise, and the 3 NONE candidate slots.
