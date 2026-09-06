# Fantasy Homes Grammar Census — Phase-1 Brief (v1, DRAFT for review)

Replicate the Furniture Grammar Census for buildings, with every v3 lesson baked in from
birth. Phase 1 is the **census**: candidates, row-level BOM, rules, negative facts, and a
provisional tally. Phase 1 does **not** author recipe families (that is Phase 2, the analog
of furniture v2/v3), does not compare donors, does not select a production asset, and does
not write Blender code.

Workflow: Claude authors; Codex audits; Blender stays paused. The architecture mining lane
**stays stood down** — Phase 1 is consolidation of already-mined material; the only new
reading permitted is from books already cached on disk.

---

## 0. Building zero (the baseline)

**Gunthwaite Hall barn**, near Penistone — the best-evidenced building in the corpus and the
only one that is simultaneously *extant*, *measured*, and *self-checking*:

> 165 × 43 × 30 ft; 15 ft to the tie-beams and 15 above; **11 bays of 15 ft** (11 × 15 = 165 ✓);
> two arcades of wooden pillars 14 × 9 in on stone pedestals; tie-beams 23 ft pillar to
> pillar; roof in a single span across the whole breadth (author-guaranteed); timber frame
> filled with stonework to 8 ft 9 in; six barn doors.

Every candidate records its grammar divergence from building zero, exactly as every chair
recorded divergence from CMA 1982.5. Gunthwaite also carries the corpus's founding
construction warning: it is **aisled**, and the Bolsterstone section (inspected at full
resolution, VERIFIED) proves that for this building class **breadth ≠ roof span** and no
single pitch describes the section.

## 1. Slots — 12, two candidates each

"Best" means lowest ambiguity and highest reusable-part yield, not the prettiest building.
Seed candidates below are starting points from the existing manifests; confirming or beating
them is Phase-1 work. Every candidate needs both an English/documentary and a measured
pairing where the corpus allows it.

| slot | type | seed candidate A | seed candidate B | known gaps |
|---|---|---|---|---|
| 01_hall_house | manor hall / great hall | Kensworth hall 35×30×22 (documentary-survey, XIIc) | a measured Tudor hall from the 351-row halls corpus (e.g. Great Chalfield 40×20×20) | Kensworth plan is only ~approximately to scale — topology reference only |
| 02_cot_booth | single-cell dwelling | Great Hatfield mud house (measured: walls 1 ft 7 in, eaves 6 ft 2 in) | Bishop Hatfield booth 20×18 ft, c. 1350 (documentary-survey) | — |
| 03_barn | large agricultural | Walton great barn 168×53×33½ (documentary, self-checking perch arithmetic) | Berwick St Leonard, XVc, 90×25, 50 across the transept | Berwick is **not yet in any manifest** (cached, Turner/Parker v4 n228) — consolidate in |
| 04_byre_shippon | animal + mixed dwelling | Rushy Lee "coit" 44×37 (measured, aisled) | Bolsterstone ox-house (VERIFIED section; 8 oxen, 4/bay, ~15 ft bays) | Felsa cow-house 80 ft = 5 bays as documentary corroboration |
| 05_forge_workshop | craft building | village forge 20×18 ft, c. 1350 (documentary — the only smithy footprint in the corpus) | **pending evidence** | second candidate needs a source |
| 06_keep_tower | fortified tower | Peak (Peveril) keep (measured: 22×19 room, 17/27 ft pair, sentry aperture) | a Viollet donjon (documented-reconstruction; carry the Coucy **datum conflict** as recorded) | tower storey height still DERIVED-only |
| 07_gate_fortification | gate, curtain, crenellation | Viollet crenel/merlon parametric set (§44 formula) | Carcassonne/Pierrefonds rows | **restoration-flagged**: Viollet's own sites; the re-class of older rows is still open queue work |
| 08_town_house | urban party-wall | Fitz-Alwyne's Assize 1189 (author-rule: walls 3 ft thick × 16 ft high; joists at 8 ft) | Viollet town-fabric rows | **frontage width is missing from the whole corpus** — record as the slot's open number |
| 09_chapel_first_floor | chapel-over-hall | Padley Hall (measured, hall 32×17×12 + chapel over) | Charney Bassett (measured, late XIIIc) | — |
| 10_cellar_crypt | undercroft | Hornsea crypt (measured, incl. fireplace 6 ft 2 in × 3 ft 2 in) | Garner & Stratton cellar 31′9″×16′10″×7′3″ | Garner cellar **not yet in any manifest** (cached, n125) — consolidate in |
| 11_mill | mill | **NONE — documented negative** | **NONE** | no mill dimension exists in Viollet or Addy; slot exists to force the sourcing decision, which is Ace's call, not Phase 1's |
| 12_stone_cell | corbelled drystone | Gallarus Oratory (measured, full openings schedule) | Teampull Beannachadh (measured, walls 2′5″–2′11″) | — |

## 2. Admission gates (mechanical, pass/fail)

1. **At least one dimensioned pair** (plan L × W) plus one height, OR a self-checking
   dimension set (stated in one unit and verifiable in another, Walton-style).
2. **Verbatim quote for every dimension** — *no quote, no row*. The quote is the witness;
   cycle 46 proved this is the defence that works.
3. **Source pinned to leaf/page**, with the book's offset rule applied and recorded
   (`len(pageindex) == imagecount` branch or measured drift).
4. **Evidence class assigned** per row: `measured` / `measured-fabric` /
   `documentary-survey` / `author-rule` / `secondary` / `derived`. Never conflated;
   documentary-survey stays separate because its error mode is scribal, not metrological.
5. **Units and datum stated**; metric values are **computed by the generator from the
   printed unit, never hand-carried** (the rule that already governs
   `english_vernacular_dimensions_v1.py`). Historical units carry their conversion
   (pied de roi 0.3248, perch 16 ft, toise) inline.
6. **Rights recorded per source** even though all are pre-1930 scans — IA rights fields
   verbatim, explicit class column, no prose sniffing.
7. **Fantasy fit is a recorded field, not a gate** (`fantasy_role`: what the building does
   in the game world). Gates stay mechanical.

## 3. Measurement discipline

- **PUBLISHED** (printed in the source) / **DERIVED** (computed; formula + assumption
  stated; safety flag) / **INFERRED** — never conflated.
- Every DERIVED row carries the safety vocabulary already proven in
  `roof_pitch_derived_v1.tsv`: **SAFE / UNSAFE / UNAISLED** (or slot-appropriate analog),
  with the refuting evidence cited when a row is demoted (Walton's 24.4° stays in the
  corpus, flagged UNSAFE, as the worked example).
- **Datum is a column** (ground level, ditch bottom, tie-beam, wall-head/"square",
  ridge-tree). The Coucy 64-vs-65 m conflict is the standing reason. Datum conflicts flag
  the *value*, they do not penalize anything else.
- OCR-destroyed fractions: any fractional value is either read off the page image
  (`verified_full_res=yes`) or shipped `ocr-uncertain`. Never reconstructed from context.

## 4. Element vocabulary

Use the existing 41-class canonical vocabulary in `element_vocab.py` as the broad layer
(the analog of furniture's 46 components). Phase 1 may propose additions where the
vernacular material demands them (candidates: `speer/hearth-screen`, `outshut`,
`cruck blade`, `padstone`) but records them as proposals in the README — the vocabulary
freeze is a review decision.

## 5. Typed uncertainty — from birth

`construction` / `quantity` / `plate_coverage` / `surface` / `provenance`. Only
construction doubt will ever penalize (Phase 2); Phase 1 just records the type per row.
Buildings-specific guidance:

- *aisling unstated* (is the roof carried on arcades?) = **construction** — the Walton lesson.
- *bay count from damaged OCR* = **quantity**.
- *plate mapped but not inspected at full resolution* = **plate_coverage** — VERIFIED only
  after inspection, same rule as `architecture_book_plates_v1.tsv`.
- *original vs restoration* (Carcassonne, Pierrefonds, the Peveril chimney rebuild) =
  **provenance**.

## 6. Rules lane — first-class from birth

Buildings differ from furniture here: the corpus's highest-value holdings are **parametric
rules**, and they get their own file from day one instead of being disguised as parts:

- the 16-ft structural bay (with the Roman ancestry chain and the Welsh yoke conflict rows
  recorded **unaveraged**);
- Fitz-Alwyne's party-wall code (1189);
- the Gwilt stair rules — including both of its two contradicting rules, unreconciled;
- the crenel/merlon formula (§44);
- the Viollet parametric window rule;
- Paley's moulding-profile constructions (already parametric in
  `moulding_profiles_v1.json`);
- masonry strength and timber scantling tables.

Rules receive no production rank in Phase 1. They are the future toolkit lane's ore.

## 7. Negative facts and constraints — never ranked

Seed set: **no mill dimension exists in any mined source**; **breadth ≠ roof span for
aisled/cruck buildings** (Bolsterstone, VERIFIED); the Welsh yoke length conflict
(16 / 16½ / 15½ ft, all recorded); Cholsey's "51 feet high" recorded-as-printed flag.

## 8. Scale contract

All Phase-1 dimensions are **real-world**. Fantasy scaling is a declared downstream
transform governed by Sinc's `docs/asset-library/CANONICAL_SCALE_CONTRACT.md` — nothing
"giant" is baked into census data.

## 9. Donor anchors (inventory only — no comparisons in Phase 1)

Recorded as pointers with status `AUDIT_REQUIRED`, nothing more: `rough_hewn_timber_beam_v1`
(in-domain, architecture); Sinc GH-001 hand-hewn beam system; GH-002 pegged mortise-tenon;
GH-011 fastener family; GOK-001 profile kernel; Sinc `families/fantasy-houses.md` and the
giant-house family. Unlike furniture, in-domain geometry donors **exist** — which is exactly
why no name may earn a status without the Phase-3 comparison.

## 10. Deliverables (all emitted by one fail-closed generator)

| file | contents |
|---|---|
| `candidates_v1.csv` | slots × candidates, gates, source/leaf, rights class, verification method, fantasy_role |
| `building_bom_v1.csv` | row-level element instances: stable `B###` IDs, element class, value + unit + computed value_m, datum, evidence class, uncertainty type, verbatim quote, leaf ref |
| `rules_v1.csv` | the parametric rules lane, each with its quote and conflict rows unmerged |
| `negative_facts_v1.csv` | constraints and documented absences |
| `derived_v1.csv` | derived values with formula, assumption, and safety flag |
| `element_matrix_v1.csv` / `element_tally_v1.csv` | elements × candidates; evidence-weighted tally — **PROVISIONAL, a coverage instrument, not a production queue** (v1-furniture's ranking overreach is not repeated) |
| `sources_v1.csv` | book registry: IA id, offset rule, per-book OCR traps (fraction destruction etc.) |
| `README.md` | hand-maintained, subordinate to the CSVs, with the ID-stability invariant stated |

Generator requirements from birth: stable positional IDs documented as frozen; quotes
mandatory (a dimension row without a quote is a build error); value_m computed only;
exclusive disposition partition asserted; fail-closed on any dangling reference.

## 11. The joinery-section library (Ace's steer, 2026-07-31)

The dimensioned **joinery section** — a window/door frame profile in section with printed
dimensions — is the highest-value drawing class in the corpus-to-be, because the section
converts to 3D geometry almost mechanically (profile + sweep path + junction rules, the same
insight that put `moulding.profile_toolkit` at the top of the furniture ranking). Three
rights-clean tiers, existence verified against the IA and LoC APIs on 2026-07-31:

| tier | source class | verified anchors | role |
|---|---|---|---|
| textbook atlases | pre-1931 joinery/construction treatises on IA | Ellis *Modern Practical Joinery* 1902 (`india.history.resource.100246`); Mitchell *Building Construction* 1894 (`buildingconstru01mitcgoog`) + 1898 (`buildingconstruc00mitc`); Hodgson 1910 (`moderncarpentryj02hodguoft`); Roubo (3 vols, 1828 ed.) | canonical sections + construction rules |
| measured surveys | **HABS/HAER at LoC — 45,882 structures**, US-gov public domain | collection count verified via API | measured-fabric sections of real buildings |
| trade catalogues | millwork/sash-and-door catalogues 1880–1930 | Foster-Munger 1895 (276 pp); Radford 1904 (`Radfordsashdoorsblindsmouldings0001`, 424 pp); Farley & Loetscher 1898 (358 pp); 28 hits in one query | standard sizes at industrial scale — the statistical backbone for parametric ranges |

Phase-1 handling: these enter `sources_v1.csv` as registry rows with their offset rules and
traps. Actual section decomposition is **Phase-2+ work** and shares its method with the
GOK-001 moulding decomposition — the existing `dimline.py` / `contour.py` /
`moulding_profile.py` chain is the tooling. Note for the fantasy setting: prefer the
historical single-glazed sections; modern manufacturer details (double-glazed IGUs) are the
right drawing *class* but the wrong period and murkier rights.

## 12. Stop conditions

Stop after the Phase-1 corpus. Do not author recipe families, do not compare donors, do not
select an asset, do not restart the mining lane, do not source the mill (flag it), and do
not write Blender code. Codex audits the corpus; Phase 2 (family authoring, the v2/v3
analog) begins only on Ace's word.

---

*Open decisions for Ace before execution:* (a) confirm Gunthwaite as building zero;
(b) confirm/edit the 12 slots — especially whether slot 11 (mill) stays as a forcing
function or is dropped until sourced; (c) whether slot 05's second candidate justifies any
new reading beyond the cached shelf (default: no); (d) sequencing against the GOK-001
moulding decomposition.
