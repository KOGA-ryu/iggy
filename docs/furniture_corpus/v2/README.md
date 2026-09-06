# Furniture Grammar Census — v2 refined taxonomy

_Broad object-part labels refined into construction-compatible recipe families._  
_v1 is preserved unchanged in the parent directory; nothing here overwrites it._

## What changed, and the one result to read first

**Zero families qualify as `exact_donor`. Thirty-six v1 rows claimed it.**

Under the strict test — compatible silhouette, section, topology, **joints**, **interfaces** and editability — exact donor reuse requires an existing donor to be compatible *with*. The asset library contains exactly **one geometry asset**, `rough_hewn_timber_beam_v1`. `structural_oak_joinery_v1`, `structural_oak_door_v1` and `forged_iron_v1` are **material** assets: they donate surface, not construction. Naming forged-iron as a donor for a hinge, or oak-door as a donor for a frame-and-panel rail, is precisely the name-resemblance error the brief forbids. **Every donor field reads `AUDIT_REQUIRED`.**

| reuse class | v1 rows | v2 families |
|---|--:|--:|
| exact_donor | 36 | **0** |
| parameterized_recipe | 77 | 22 |
| shared_profile | 25 | 5 |
| assembly_grammar | 16 | 7 |
| bespoke | 3 | **7** |

**77 row-level reclassifications** in `exact_reuse_corrections_v2.csv`, each with its reason.

## The splits that matter most

Three are genuine construction *opposites*, not size changes — mixing them would produce unbuildable geometry:

1. **`foot.integral_stile_termination` vs `foot.turned_separate`.** Four objects have no foot part at all: the stile runs past the lowest rail to the floor. Donating a turned foot there means cutting the stile and changing its joint schedule.
2. **`post.panel_muntin` vs `post.case_corner_structural`.** A corner post is grooved and mortised on *adjacent* faces; a muntin on *opposite* faces. Different joint schedule.
3. **`frame.stile_rail_muntin_grid` vs `frame.applied_moulded_surround`.** One is structural joinery holding a floating panel; the other is applied trim on a flat board. Confusing them yields a frame that cannot hold a panel.

A fourth family exists only to prevent an error: **`frame.assembled_from_members`** records three objects where what reads as a frame is the assembly of post + post + rail + crest. There is no frame part to author.

## Evidence weighting

`VISIBLE` 1.0 · `PARTIALLY VISIBLE` 0.5 · `INFERRED FROM TYPE` **0.0** · `NOT DETERMINABLE` **0.0**.

Inferred rows are preserved as research notes — the matrix marks them `i` — and contribute no production priority. The clearest case is **`brace.corner_glue_block`**: seven members, but **four are inferred from type** and only three are evidenced. Under v1's unweighted count it looked like a strong recipe; weighted, it is flagged CRITICAL in the unresolved report.

## Top ten by evidence-weighted priority

| # | recipe family | parent | weighted evidence | reuse class |
|--:|---|---|--:|---|
| 1 | `moulding.profile_library` | moulding run | 9.5 | shared_profile |
| 2 | `inlay.linear_stringing_path` | inlay path | 7.0 | parameterized_recipe |
| 3 | `ornament.instanced_relief_master` | repeated ornament master | 8.0 | assembly_grammar |
| 4 | `foot.turned_separate` | foot | 6.0 | parameterized_recipe |
| 5 | `panel.floating_board_in_groove` | floating panel | 5.5 | parameterized_recipe |
| 6 | `cornice.stacked_fillet_moulding` | crest/cornice | 6.0 | shared_profile |
| 7 | `ornament.marquetry_motif_set` | repeated ornament master | 4.0 | parameterized_recipe |
| 8 | `frame.applied_moulded_surround` | panel frame | 4.0 | parameterized_recipe |
| 9 | `crest.shaped_sawn_board` | crest/cornice | 4.0 | shared_profile |
| 10 | `stile.separate_rear_leg` | rear leg/stile | 3.0 | parameterized_recipe |

`moulding.profile_library` tops it on 11 of 18 objects, and the records say why: several objects state explicitly that **one section serves several roles at different scales** — 69.146.2's single profile does drawer surrounds, side-panel frames and the plinth.

## Files

- `taxonomy_v2.py` — the authored family definitions, with reasoning in the docstring
- `refined_component_taxonomy_v2.csv` — 41 families × the 14 required fields
- `refined_component_matrix_v2.csv` — families × candidates; cells 1 / 0.5 / i / blank
- `evidence_weighted_tally_v2.csv` — weighted frequency per family
- `corrected_recipe_ranking_v2.csv` — ranking with the score decomposition
- `exact_reuse_corrections_v2.csv` — 77 reclassifications with reasons
- `unresolved_family_splits_v2.csv` — 11 families flagged for audit

## For the reviewing agents

- **Construction-compatibility auditor:** the three opposites above are the load-bearing claims. `post.arm_support` is the weakest — it is defined by ROLE plus a shape selector because one object (30.120.59) carries two different arm-post shapes at once.
- **Sinc-donor auditor:** every field is `AUDIT_REQUIRED` and only `rough_hewn_timber_beam_v1` is a geometry asset. Three families name it as a *candidate* for continuous-stock or square-section logic; silhouette, section, joints and editability have **not** been compared.
- **Evidence-weighting auditor:** `brace.corner_glue_block` and `rail.seat_frame` are where inferred rows most affect the outcome. Eleven single-member or bespoke-multi-member families are flagged as possible over-splits.

**Stopped here.** No production asset selected, no Blender code written.
