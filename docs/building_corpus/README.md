# Fantasy Homes Grammar Census — Phase 1

Governed by [PHASE1_BRIEF_v1.md](PHASE1_BRIEF_v1.md) (blessed 2026-07-31). All tables are
emitted by [`census_v1.py`](census_v1.py), which fails closed on dangling references,
duplicate IDs, or incomplete slots. **This README is maintained by hand and is subordinate
to the CSVs** — if a number here disagrees with a table, the table is right.

## State after cycle 1 (2026-07-31)

Ace's opening order: *start Phase 1 with Mitchell and Ellis.* Both are loaded, trap-checked,
and content-verified at full resolution.

| source | leaves | offset rule | fraction trap | content verified |
|---|--:|---|---|---|
| `mitchell1898` (`buildingconstruc00mitc`) | 336 | printed = leaf − 13, confirmed at 2 points | **destroys fractions** | n206 (printed 193): dimensioned ledged/braced door details — the construction of the Sinc door fixture |
| `mitchell1894` (`buildingconstru01mitcgoog`) | 293 | printed = leaf − 14, confirmed at 2 points | **destroys fractions** | not yet inspected; Google scan, worse OCR — prefer 1898 where both cover a topic |
| `ellis1902` (`india.history.resource.100246`) | 421 | **OFFSET DRIFTS** (−17 at n141, −32 at n300); `imagecount` absent, trusted-branch test unevaluable — read the printed number per citation | **destroys fractions** | n141 (printed 124): sash-frame construction — pulley stile, pockets, oak sill sinkings; dimensioned isometrics **and** quantified prose rules on one page |

**The structural finding of cycle 1:** dimension density in the *prose* of all three books is
anomalously low because **the numbers live on the figures**. For the joinery shelf the
extraction route is plate-first (dimline/contour at full resolution), with prose supplying
rules and part names — the inverse of the Addy workflow. All fraction-bearing values must be
read off the page image; the OCR layer destroys every ½ ¼ ¾ across 1,050 leaves.

Two new traps were found and documented in `iabook.py` where future cycles will see them:
the **derivative-prefix trap** (Ellis's hOCR files are `100246_*`, not identifier-prefixed)
and the **absent-imagecount addendum** to the offset rule.

Content clusters located: Ellis windows/sashes leaves ~129–145 within 102 sash-bearing
leaves; Mitchell 1898 sash/casement cluster n192–n323; Mitchell 1894 n179–n284.

## State after cycle 2 (2026-07-31) — BOM consolidation

**[`building_bom_v1.csv`](building_bom_v1.csv): 166 rows**, every PASS candidate covered,
every row carrying its verbatim quote (fail-closed enforces *no quote, no row*). Exclusive
disposition of the 149-row vernacular manifest: **121 assigned, 23 rules-deferred (cycle 3),
5 reserve, 5 out-of-scope (Pompeii), 0 unexplained.** Ingested alongside: 3 Great Chalfield
rows from the halls corpus, 29 Viollet rows (Coucy 8 with the datum conflict carried
verbatim; Carcassonne 5 flagged `provenance`; town-fabric 16), and 13 hand-consolidated
orphan rows with generator-computed metres.

**Both PENDING candidates resolved to PASS:**

- **Berwick St Leonard barn** — found at `someaccountofdom02park` n228, exactly where the
  cycle-48 citation said. The volume identity was the missing fact, and hunting it produced
  the **wrong-Berwick trap** (the obvious volumes return Berwick-upon-Tweed).
- **The Garner cellar is Lavenham Guildhall** — identified from the page itself at full
  resolution (the two-column OCR had jumbled the name). Bonus rows: main hall ~30 × 17 ft,
  18 in jetty, and *"doors were ledged and boarded, hung on massive hand hinges."*

Also swept in: **Beetham Hall** (the 39 × 26 ft XIVc hall *"now used as a barn"*) and the
Cholsey barn as `RESERVE-*` rows — real buildings with real quotes but no census slot;
preserved, never ranked. Bolsterstone gained its cycle-49 findings as BOM rows (bay ~15 ft,
loft at 6+ ft, both from the full-res-verified page).

Evidence across the BOM: 113 measured, 33 documentary-survey, 9 author-estimate,
5 documented-reconstruction, 3 secondary, 2 author-rule, 1 ocr-uncertain;
41 rows verified at full resolution.

One more generator lesson, caught by fail-closed on its first BOM run: **raw tab-joined
TSVs must be read `QUOTE_NONE`** — default csv quoting silently strips the literal quotes
in names like `"Coit" at Rushy Lee`, which is precisely the silent-mutation class the gate
exists for.

## State after cycle 3 (2026-07-31) — the rules lane

**[`rules_v1.csv`](rules_v1.csv): 78 rows** across 9 groups — the census's parametric
holdings, each rule with its verbatim quote, conflicts carried **unmerged** under
`conflict_group`:

| group | rows | contents |
|---|--:|---|
| stair | 27 | Gwilt: Newland pairing table, Palladio staircase, headway, riser count, widths — incl. the same-page contradiction (`gwilt-stair-pitch`, 12 rows: constant-product 66 vs the table that violates it by up to 47%) |
| bay_module | 13 | the 16-ft bay with its Roman ancestry (Vitruvius, Palladius, Columella) and its documentary tests (Felsa = 5 bays, berchary = 10) |
| fortification | 10 | **the complete XIIIc crenellation formula** (merlon 2.00 × 1.70–3.30 × 0.45 m; crenel sill 1.00 m, width 0.70 m) plus the design principle: *"les dimensions des crénelages étant données par la taille de l'homme"* |
| unit_system | 7 | perch, rod/rood, and the Welsh yokes — `long-yoke` conflict (16 / 16½ / 15½ ft) never averaged |
| window | 6 | Viollet's parametric mullion progression (2 m → 1 mullion; 4 m → 1+2; 8 m → 1+2+4) and the 1 m max-clear-glass rule |
| joinery_weathering | 5 | **Ellis's sash rules, fractions read off the page image** (throat ≥ ¼ in; water bar 1 × 3/16 in; sill bench allowance ⅛ in; pulley-stile sinking ¼ in) — the first rules extracted from the cycle-1 books |
| bridge | 4 | timber pile spacing 12 m; Villard's 50-ft span from 20-ft timbers |
| building_code | 3 | Fitz-Alwyne's Assize 1189 (the BC-08A crossover content) |
| material_table | 3 | pointers to the masonry-strength and roof-scantling tables and the parametric Paley profiles |

The two rules-crossover candidates now have their content in place: `BC-07A-crenel-set` →
the fortification group; `BC-08A-fitzalwyne` → the building_code group.

**[`negative_facts_v1.csv`](negative_facts_v1.csv): 8 constraints, never ranked** — the
mill absence, breadth ≠ span for aisled buildings (VERIFIED), the two do-not-reconcile
conflicts, Cholsey's as-printed flag, the Coucy datum conflict, the Carcassonne provenance
constraint, and the Kensworth plan-reliability warning.

**[`derived_v1.csv`](derived_v1.csv): 6 roof pitches** with their safety flags carried
(1 SAFE — Gunthwaite, author-guaranteed single span; 1 UNSAFE — Walton, refuted span
assumption; 4 UNAISLED), each linked to its candidate and its formula stated.

## State after cycle 4 (2026-07-31) — the provisional element tally

**[`element_tally_v1.csv`](element_tally_v1.csv) + [`element_matrix_v1.csv`](element_matrix_v1.csv):
all 166 BOM rows mapped into 25 canonical elements, 0 unmapped** (fail-closed asserts it).
Every tally row carries the banner: **PROVISIONAL — a coverage instrument, not a production
queue.** No reuse priors, no production authority; the furniture-v1 ranking overreach is not
repeated.

Coverage leaders (weighted, per the documented provisional weights — first-hand numbers 1.0,
estimates/reconstructions/secondary 0.5, ocr-uncertain 0.0):

| element | rows | candidates | W |
|---|--:|--:|--:|
| height | 29 | 15 | 13.0 |
| internal_length | 20 | 14 | 12.5 |
| internal_width | 19 | 13 | 12.0 |
| width | 11 | 9 | 8.5 |
| **opening_width** | 15 | 7 | 7.0 |
| internal_height | 12 | 6 | 5.5 |
| **opening_height** | 9 | 5 | 5.0 |
| wall_thickness | 6 | 5 | 4.5 |

Envelope dimensions dominate as expected — and **openings are the strongest non-envelope
class**, which independently supports the section-first thesis: window and door sections are
the element layer every slot shares.

**The dry run's finding was a dialect fact:** 21 of the 29 initially-unmapped rows were Addy
saying *breadth* where the vocab (built on Viollet and the halls corpus) says *width*.
Handled by a census-side **overlay** — `element_vocab.py` itself is untouched; its freeze is
a review decision. Recorded proposals:

- **new classes**: `hearth_screen` (the brief's own proposal — Great Hatfield's speer rows,
  which the bare vocab buried under generic length/height) and `plan_dimension` (axis
  labelling that doesn't follow the length/width convention — Padley's "order as printed"
  pair, Hornsea's compass-labelled pair; assigning these to length/width would invent an
  axis the source does not state)
- **pattern extensions** to existing classes: breadth→width family, arcade-pillar
  sections→member_section, ambry→opening_width, escarpment→level_difference,
  ditch→gap, capacity→count

## State after cycle 5 (2026-07-31) — the section inventory; Phase 1 complete

**[`section_inventory_v1.csv`](section_inventory_v1.csv): 289 figure entries after the
audit's filters** (originally 384 — see AUDIT_PHASE1.md). The Phase-2 decomposition
worklist: 247 from Ellis (real captions: *"Part Vertical Section of Frame"*, *"Section of
Bevelled Meeting Rails"*, *"Method of Fixing Pulley Stile in Sill"*), 42 from Mitchell 1898
(labels plus prose references — Mitchell cites both ways).
**218 priority-1 (sash/casement/skylight/shutter) and 71 priority-2 (door).**

Status discipline holds to the end: **382 MAPPED, 2 VERIFIED-LEAF** (only the figures on the
two leaves actually inspected at full resolution). `dimensioned` is UNKNOWN throughout —
the books carry their dimensions on the figures and the OCR destroys fractions, so only
full-resolution inspection can answer it, and that is precisely Phase 2's job. The scope
cut is visible, not silent: 74 non-window/door leaves holding 177 figures were excluded and
counted.

Extraction runs as a separate script ([`section_inventory_extract.py`](section_inventory_extract.py))
against the cached OCR, committing [`section_inventory_raw.tsv`](section_inventory_raw.tsv);
the census generator ingests the committed TSV and never touches the network.

**Phase 1 is complete.** Every deliverable in the blessed brief exists, emitted by one
fail-closed generator. Per the brief's stop conditions: no family authoring, no donor
comparisons, no asset selection, no mill sourcing, no Blender. Next: Codex audits the
corpus; Phase 2 (family authoring + section decomposition, shared method with GOK-001)
begins on Ace's word.

## Files

| file | rows | contents |
|---|--:|---|
| [`sources_v1.csv`](sources_v1.csv) | 11 | source registry; + Gwilt (the drift-rule book, stair source) |
| [`candidates_v1.csv`](candidates_v1.csv) | 25 | building zero + 12 slots × 2: **22 PASS**, **3 NONE** (forge candidate B unsourced; both mill rows — the documented negative) |
| [`building_bom_v1.csv`](building_bom_v1.csv) | 166 | row-level element instances: stable `B###` IDs, value + unit + computed value_m, datum, evidence class, uncertainty type, verbatim quote, leaf ref |
| [`rules_v1.csv`](rules_v1.csv) | 78 | the parametric rules lane: `RL###` IDs, 9 groups, conflicts unmerged, quotes mandatory (table pointers exempt but target-checked) |
| [`negative_facts_v1.csv`](negative_facts_v1.csv) | 8 | constraints and documented absences — never ranked |
| [`derived_v1.csv`](derived_v1.csv) | 6 | derived roof pitches: formula, safety flag, span assumption, candidate link |
| [`element_tally_v1.csv`](element_tally_v1.csv) | 25 | PROVISIONAL coverage tally: mapping layer, rows, weighted evidence |
| [`element_matrix_v1.csv`](element_matrix_v1.csv) | 25 | elements × candidates; cells 1 / 0.5 / u / blank |
| [`section_inventory_v1.csv`](section_inventory_v1.csv) | 289 | Phase-2 worklist: figure, caption, topics, priority, status (post-audit filters) |
| [`section_inventory_raw.tsv`](section_inventory_raw.tsv) | 289 | committed raw extraction (input to the generator) |
| [`AUDIT_PHASE1.md`](AUDIT_PHASE1.md) | — | the Phase-1 audit record |

`B###` IDs are positional over the frozen ingestion order (vernacular manifest order →
halls → Viollet files → orphans); new rows append only. The two rules-crossover candidates
(`BC-07A-crenel-set`, `BC-08A-fitzalwyne`) intentionally carry zero BOM rows — their content
is `rules_v1`.

## Phase 2 opened — cycle 6 (2026-07-31): plate 451A decomposition

First decomposition target per Ace's call: **plate 451A** (Ellis n153 = printed 136, read
off the page). Full record in [sections/451A/](sections/451A/README.md). The plate's eight
details enumerated and VERIFIED; **calibration from its own printed scale bar: 70.401
px/inch, RMS 0.024 in**; No.4 (meeting stiles + oak slip) measured into a 13-quantity
schedule — headline: **stile depth 1.911 in ≈ the classic 2-inch casement stock**; joint
anatomy read (bevelled, slip-feathered, weatherproof by geometry). Two method verdicts,
both proven on the plate: scanline-median edge measurement works on hatched engravings;
blob tracing fails at wood-contact merges exactly as the Paley doctrine predicts — curved
profiles get authored parametrically and overlay-verified, not pixel-traced.

**Blind spot found and quantified during the decomposition:** engraved-caption full plates
are invisible to the OCR-derived section inventory — n153 itself has zero inventory rows.
~63 low-text leaves in Ellis and ~62 in Mitchell 1898 form the same class. The Phase-2 queue
therefore gets a **plate sweep** (contact-sheet triage of low-text leaves) alongside the
caption worklist.

**Cycle 7:** the No.4 profiles are authored — [sections/451A/profiles_451A_no4_v1.json](sections/451A/profiles_451A_no4_v1.json),
the census's **first geometry artifact**: the left casement stile (bay side fully measured,
meeting bevel provisional-visual) and the oak slip's double-lobed weather moulding
(empirical crown polyline), overlay-verified in two iterations — the first iteration
refuted the provisional meeting face exactly as the loop is designed to do.

## Phase-1 audit (2026-07-31)

Codex being busy, the audit ran here — mechanical and adversarial by design (recomputation
+ seed-42 image spot-checks, not re-reading of claims). **PASS with two fixed defects.**
Full record in [AUDIT_PHASE1.md](AUDIT_PHASE1.md): 0 metre-recomputation mismatches, 0 true
witness failures, sums close; B163's witness-form drift and an emit-format wart fixed;
section inventory calibrated at ~25% sample noise (n=8, wide CI), mechanically-caused noise
filtered (384 → 289 rows, exclusions counted), residual topic noise documented. Bonus
findings: Ellis fig 475 is visibly dimensioned, and plate 451A (printed 136) carries eight
weatherproof sash/casement sections **with a printed inch scale** — the top of the Phase-2
queue.

## Phase-1 close-out

All brief deliverables emitted; stop conditions honoured. Outstanding facts a reviewer
should challenge first: the 3 NONE candidate rows (forge B, both mill rows), the Ellis
printed-page column (drifting offset — every citation needs its page read), the 29
initially-unmapped elements now handled by the census overlay (proposals, not vocabulary
freezes), and the 2-of-289 verification rate on the section inventory (by design — MAPPED
is a worklist status, not a claim).
