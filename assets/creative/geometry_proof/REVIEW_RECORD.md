# Review record — SINC_GeometryProof_Profile

The inspectable record behind the README's hardening claims. Two multi-agent
reviews were run on 2026-07-31; this file lists what they found so acceptance
does not rest on an uninspectable headline.

## 1. Adversarial review (41 agents: 5 reviewer lenses + refutation verifiers)

36 findings raised; each independently verified by an adversarial agent
instructed to REFUTE it with live reproduction. 15 confirmed and repaired
same-day; 21 refuted.

### Confirmed and repaired

| finding | repair |
|---|---|
| Simple-polygon check blind to improper touches, colinear overlaps, zero-area outlines | full intersection test incl. colinear/on-segment cases; doubling-back check; zero-area rejection |
| Sliver arcs: near-coincident stations with a tiny sweep compile and the member silently vanishes | sweep gate: < 0.5 deg rejected |
| Required enum fields accept explicit `null` (`join: null` bypasses the SMOOTH proof) | presence-based guards |
| Blend tests never bind the artifact to the committed spec (stale-green) | manifest sha256 vs current spec file asserted |
| Blend flip-gate passes on a wrongly rebuilt artifact | master curve bound to freshly recompiled geometry |
| Parameter-claim rejection coverage was one-of-six claim kinds | rejection tests for every claim kind |
| BEZIER tangents never asserted (sign flip survives the suite) | bezier smooth-join accept/reject tests |
| Moulded-chain wrap branch zero coverage | wrap and all-MOULDED tests |
| All-MOULDED chain returned duplicated seam vertex | outline-convention dedupe + documented |
| Reference plane spec/code divergence (crop fields undocumented) | BUILD_SPEC 4.1/6 document the crop fields; manifest reference_note |
| Undocumented minimum feature size (= tolerance.distance) | documented in BUILD_SPEC 4.1 |
| Ortho camera crops labels on tall profiles (shipped stepped render) | aspect-aware ortho span |
| mm-scale profiles render blank (clip_start) | clip planes scaled to scene |
| Label stagger collides for same-parity adjacent stations (shipped fig 6 render) | collision-resolution pass |
| Targeted `--flip` builds could write into canonical output/ | guard added |

Refuted (21): included claims about determinism, single-anchor source_px,
extent-vs-sampling semantics, wire-proxy staleness, repo_root derivation,
render-engine gating and others - each found to be documented, intended
behavior or an unreachable scenario. Full verdicts with reproduction notes
live in the session workflow journals.

## 2. Acceptance-evidence audit (9 agents: 8 per-profile + completeness critic)

All eight profiles: evidence fresh (manifest sha256 = current spec, coherent
build timestamps, four non-trivial renders each), badges consistent with
README. Per-profile disclosures are curated onto the acceptance review board;
the highlights a reviewer must not miss:

- **paley_pl1_fig6_v1**: overlay crop placement is eyeball-grade (~±10 px);
  upper-face routing deliberately unasserted; hollow_end is an authored
  resolution of a 0.021 in measured inconsistency; only stock_width is
  print-anchored.
- **gok001_cavetto_fillet_b_v1**: cross-check only - does NOT discharge
  GOK001-CFB-P04; oblique source ineligible for overlay; land ratios
  REFERENCE_INFERRED.
- **ovolo / ogee / stepped / mm_rail**: zero measured evidence BY DESIGN -
  vocabulary/scale proofs, not donors. Ogee is adapted (60 deg, unequal
  radii), not a Brandon reproduction. Stepped carries a benign
  HARD-but-continuous warning at riser3_end (role boundary, not a corner).
- **brandon_pl9_tiebeam_v1**: only the 20x14 scantling is PRINTED; members
  are proportional-era ESTIMATED; the whole right arris is an AUTHORED
  mirror; casement sweep 90->45 is an authored correction overriding the
  record; awaits leaf n79 raster before donor status.
- **receding_orders_paley_pl1_fig7_v1**: PROPORTIONAL ONLY, no overlay; the
  5.05 extent "validation" is partly circular (0.85 has no independent
  source); bbox-ratio match is ~0.9%, larger than strict tolerance; closure
  is authored to the drawn crop.

### Critic reconciliations and their resolutions

| item | resolution |
|---|---|
| README stale counts (42 vs 66 tests; six vs eight profiles; envelope over "six") | FIXED 2026-07-31 |
| Envelope overstated cubics (no retained profile uses BEZIER) | FIXED - envelope now says so |
| Stepped spec notes falsified by the faithful fig 7 | FIXED + rebuilt (new sha) |
| No inspectable review record | this file |
| No per-gate acceptance record; user's review is the first recorded sign-off | correct - the acceptance board IS that record |
| "Four accepted profile examples" reading (reference-grade vs through-pipeline) | user decision on the board |
| Package uncommitted - sha bindings anchor to disk, not a revision | user decision: commit on acceptance |
| Rectification list is family-level; Met photos deferred to per-piece | disclosed; per-piece pass is future work |
| TEMPLATE.json has no schema-sync guard | disclosed; accepted risk for now |
| Proposer lane has not yet produced an inventory profile (tests + scratch e2e only) | disclosed; first live consumer is 451A No.4 |
