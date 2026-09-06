# Furniture Grammar Census v1

_Production-reference corpus for a reusable fantasy-environment asset library._  
_Generated 2026-07-30. Facts come from museum open-access APIs; agents supplied analysis only._

## Method, and why it is trustworthy

The admission gates are **factual**, so they are applied by machine, not by judgement: `museum_api.py` queries the Metropolitan and Cleveland open-access APIs and reads `dimensions`, `accessionNumber`, `additionalImages` and `isPublicDomain` directly. A view count is *counted*, not estimated. Anything a source does not supply is the literal string `NOT STATED` and is never filled in.

Every candidate returned by a research agent was then **independently re-verified** against the API in a separate pass. Result: **24 checked, 0 mismatches** on accession, published dimensions and view count. Three candidates (Philadelphia, Brooklyn, V&A) have no open API; they are marked `PAGE-VERIFIED ONLY` in `candidates_v1.csv` rather than mixed in silently.

### Object zero

| field | value |
|---|---|
| source | Cleveland Museum of Art, accession **1982.5** |
| museum title | `Chair` (the brief calls it a side chair; the museum does not) |
| maker / date | Herter Brothers (American), c. 1880 |
| medium | ebonized cherry and other woods |
| dimensions (PUBLISHED) | `Overall: 84.5 x 40.7 x 46 cm (33 1/4 x 16 x 18 1/8 in.)` |
| rights | CC0 |

## Slot coverage

**12 slots, 24 candidates, 2 per slot.** The search-priority ladder mattered: the same-maker tier filled only two slots with two candidates each (armchair, console). Eight slots had exactly one Herter candidate, and settee, chest/trunk and long table had **none** — those were filled at tiers 2 and 3, with the tier recorded per candidate.

| slot | A | views | B | views |
|---|---|--:|---|--:|
| 01_armchair | Metropolitan Museum of Art 1999.488 | 5 | Metropolitan Museum of Art 2012.216 | 6 |
| 02_stool | Metropolitan Museum of Art 67.230 | 3 | Metropolitan Museum of Art 60.4.14 | 4 |
| 03_settee | Cleveland Museum of Art 2011.3 | 11 | Metropolitan Museum of Art 30.120.59 | 9 |
| 04_side_table | Metropolitan Museum of Art 1972.47 | 11 | Cleveland Museum of Art 1969.262 | 13 |
| 05_desk | Metropolitan Museum of Art 69.146.3 | 6 | Philadelphia Museum of Art 1974-224-1a,b | 9 |
| 06_chest_drawers | Metropolitan Museum of Art 69.146.2 | 5 | Metropolitan Museum of Art 27.57.1 | 13 |
| 07_sideboard | Metropolitan Museum of Art 1999.79 | 10 | Brooklyn Museum 76.63a-f | 7 |
| 08_wardrobe | Metropolitan Museum of Art 69.140a, b | 10 | Cleveland Museum of Art 2019.59 | 18 |
| 09_bedstead | Metropolitan Museum of Art 69.146.1 | 5 | Cleveland Museum of Art 1954.151 | 22 |
| 10_chest | Cleveland Museum of Art 1971.281 | 4 | Cleveland Museum of Art 1984.161 | 3 |
| 11_long_table | Metropolitan Museum of Art 10.125.133 | 7 | Victoria and Albert Museum 236:1, 2-1869 | 7 |
| 12_console | Metropolitan Museum of Art 2002.298.1 | 8 | Metropolitan Museum of Art 2002.298.2 | 8 |

## The tally — what to build first

46 normalized components across 18 analysed objects (2 of 20 BOM agents died on API errors; their candidates are listed as gaps below).

| # | component | objects | % of corpus | dominant reuse class |
|--:|---|--:|--:|---|
| 1 | **foot** | 17 | 94% | exact accepted donor reuse |
| 2 | **post** | 17 | 94% | parameterized recipe reuse |
| 3 | **rail** | 17 | 94% | parameterized recipe reuse |
| 4 | **repeated ornament master** | 17 | 94% | exact accepted donor reuse |
| 5 | **moulding run** | 16 | 89% | shared profile or curve reuse |
| 6 | **rear leg/stile** | 13 | 72% | parameterized recipe reuse |
| 7 | **brace** | 14 | 78% | parameterized recipe reuse |
| 8 | **panel frame** | 13 | 72% | assembly-grammar reuse |
| 9 | **floating panel** | 12 | 67% | parameterized recipe reuse |
| 10 | **crest/cornice** | 12 | 67% | parameterized recipe reuse |

Score = 60·frequency + 25·mean reuse weight + 15·mean evidence quality. Bespoke components are weighted down hard (0.15), so a spectacular one-off cannot outrank a plain part that appears everywhere.

### The optimization you described, confirmed by the data

`lattice upright` — the baseline chair's most distinctive feature — appears in **2 of 18** objects and ranks near the bottom (score 44.2). `foot`, `post` and `rail` appear in **17 of 18**. So the lattice waits, and the first three recipes are the boring ones. That is the census doing its job: it disagrees with taste.

A second surprise worth acting on: `stretcher` — present on object zero — appears in only **4 of 18** objects. The baseline is not representative of the corpus on that member.

## Files

- `museum_api.py` — API harvester + mechanical admission gates (documents 3 API traps)
- `candidates_v1.csv` — the 24-candidate corpus, one row per candidate, with verification method
- `candidates_tier1_herter.csv` — all 37 Herter Brothers objects at the Met with gate results
- `component_bom_v1.csv` — 447 component rows: name, count, evidence class, reuse class, variation axis
- `component_matrix_v1.csv` — cross-reference matrix, components x candidates
- `component_tally_v1.csv` — frequency tally per normalized component
- `variation_table_v1.csv` — what changes between instances of each recurring component
- `recipe_ranking_v1.csv` — ranked authoring order with the score decomposition
- `rejected_references_v1.csv` — 152 rejections, each with the exact gate it failed
- `unresolved_evidence_v1.csv` — per-candidate evidence blockers and hidden construction

## Known gaps, stated rather than hidden

- **2 BOMs missing**: `FC-02-60_4_14` (Phyfe footstool) and `FC-06-27_57_1` (chest of drawers) — both agents died on API errors mid-response. The candidates are verified; only their component analysis is absent.
- **3 candidates page-verified only** — Philadelphia 1974-224-1a,b, Brooklyn 76.63a-f, V&A 236:1,2-1869. View counts for these were counted by eye, not returned by an API.
- **No underside or interior view** exists for several candidates; those components are marked `NOT DETERMINABLE` or `INFERRED FROM TYPE` in the BOM and must not be treated as observed.
- Slot 05 candidate B and slot 07 candidate B come from institutions without open APIs, so their rights status is as stated on the page, not machine-confirmed.
