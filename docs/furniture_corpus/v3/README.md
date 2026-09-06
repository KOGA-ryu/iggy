# Furniture Grammar Census — v3 (repaired, v3.1 + final micro-patch)

A focused repair pass on v3, followed by one final localized correction pass (ten items from
the spot audit). The v3 structure is preserved — row-level provenance, evidence
weighting, lane separation, candidate coverage, generator-emitted CSVs, non-asset removal.
**v1 and v2 are not written to by this generator, and their files are unchanged on disk.**

No census restart, no new references, no architecture mining, no asset selected, no Blender
code.

### About this document

**The twelve CSVs are generator-emitted and reproducible** — `python3 taxonomy_v3.py` from
this directory rewrites all of them. **This README is maintained by hand.** It is checked
against the tables but it is not emitted by them; if a number here ever disagrees with a CSV,
the CSV is right.

## Corrected counts

| | v3 | v3.1 | final patch |
|---|--:|--:|--:|
| analytical subrows | 465 | 472 | **479** |
| parents given suffixed identities | 18 (claimed) | 21 | **22** |
| — of those, expanded into >1 subrow | 17 (actual) | 21 | **22** |
| — renamed to a single subrow | 1 | 0 | **0** |
| analytical subrows created by splits | — | 46 | **54** |
| families | 88 | 118 | **122** (115 rankable, **7 `AUDIT_ONLY`**) |
| assignments | 208 | 204 | **209** |
| weighted evidence | 192.0 | 190.0 | **194.0** (179 VISIBLE + 30 PARTIALLY VISIBLE) |
| quarantined rows | — | 11 | **13** |
| donor rows unresolvable | 6 | 0 | **0** |

**Split vocabulary, stated exactly** (v3 conflated these): 22 parent rows carry suffixed
identities, all 22 expanded into more than one analytical row, 54 subrows were created, and
the corpus holds 479 analytical subrows. The final patch added the three splits the audit
demanded: **`R114` into its six named masters** (MOP star, MOP disc, carved rosette/patera,
flower-in-linked-chain, abalone plaque, carved palmette — the source count field names all
six; the previous two-bucket split collapsed identities the source keeps separate),
**`R216` into its three recorded rail roles** (arched frieze / drawer-bounding /
centre-section), and **the missing `R247.d`** — the single rail above the drawers, present in
the source count but dropped by the three-way split.

## 1. Source and family corrections

**`R234` is now four masters, not one renamed aggregate** — `R234.a` anthemion capital,
`.b` husk-and-laurel swag, `.c` dentil block, `.d` drilled dot. The drilled dot is
*subtractive* where the other three are applied, so they could never have shared a recipe.
`R114` and `R338` and `R247` are split on the same principle.

**Construction membership corrected where it was not proved:**

| row | was | now |
|---|---|---|
| `R193.a` | `foot.turned_integral_with_leg`, proven | moved to `foot.turned_interface_unresolved` — "transitions into" describes a shape change, not proved continuity of stock |
| `R243` | `termination.stile_to_floor`, proven | **quarantined** — "whether it is glued-on or integral is not resolvable under the ebonizing" |
| `R156` | `termination.leg_toe_prismatic` | **quarantined** — the seam may be an original cuff or a splice, and a splice is a joint |
| `R320` | `muntin.panel_grooved`, proven | **quarantined** — neither the opposite-face grooves nor the tenons are visible |
| `R192` | proven **and** unresolved | unresolved only — the housing is not visible in any view |
| `R338` | one row, form and end joint conflated | `R338.a` form (proved, in production) / `R338.b` end joint (quarantined) |
| `R437` | construction-uncertain | **construction proved**; only the register count is open |
| `R056` | `bespoke_hero` | **`ornament.radial_petal_array`**, a recipe — one petal master arrayed round a bell |

## 2. Analysis buckets cannot enter the production queue

Seven families now carry `status = AUDIT_ONLY` and score **0 / `NOT_RANKABLE_AUDIT_ONLY`**:
`foot.turned_interface_unresolved`, `post.carcase_unresolved`, `stile.continuity_unresolved`,
`rail.multi_purpose_unresolved`, and — added by the final audit for having zero proved
construction members while ranked — `partition.interior_vertical`, `rail.table_frame`,
`rail.chest_lock_and_lower`.

A visible silhouette with an unknown interface is research, not a recipe.

## 3. Uncertainty is typed

v3 used one ambiguity penalty for every kind of doubt, so a family was punished for facts
that have nothing to do with how its part is built. Now:

- **construction** — how the part is made or meets its host → **reduces confidence, penalises**
- **quantity** · **view_coverage** · **surface** · **provenance** → recorded, **no penalty**

`R034`, `R438`, `R434`, `R437` are quantity doubts (only the front half of a band is ever
photographed). `R442`, `R443` are view-coverage doubts (one face of the capital was
photographed; four are implied). None of them is doubt about construction, and none of them
costs anything now. The two capital masks rise from 0.037 to 0.15 as a direct result.

## 4. `panel.floating_wood_in_groove` repaired — it is no longer the lane leader

v3 admitted 11 members and asserted a concealed groove and a never-glued interface for rows
where the photographs prove only placement or appearance. Each member is now classed by how
its interface is actually known (`panel_interface_class` in the assignment table):

| class | rows | admissible? |
|---|---|---|
| groove stated by source | `R363` | yes |
| frame-and-panel stated by source (with groove depth) | `R198` | yes |
| frame layout only | `R273`, `R381` | **no — quarantined by the final audit** |
| inferred from type | `R072`, `R131`, `R219`, `R317` | **no** |
| unresolved | `R165`, `R318`, `R249` | **no** |

Only `R363` and `R198` prove the exact groove/captured-panel contract — `R198`'s companion
`R197` records groove depth explicitly, while `R273`/`R381`'s companions (`R272`, `R380`)
prove frame layout, panel count and peg pattern, never the concealed capture. Weighted
evidence falls **10.0 → 4.0 → 2.0** across the three passes and the family now scores
**2.07**. `R072` is
the sharpest case: its "plain ungilded board with a visible horizontal glue joint" is, if
anything, evidence *against* a free-floating panel. `R249` is excluded because its own frame
row (`R248`) says the case-side construction cannot be settled — admitting the panel would
decide by the back door what the frame refuses to decide.

## 5. Invalid merges undone

**`ornament.instanced_relief_master` is gone.** It held 17 unrelated masters and led the
assembly lane at 22.59 on an argument amounting to "these all get placed somewhere". Placement
is not construction. It is replaced by:

- **`ornament.placement_system`** — the instancing rule alone, owning no geometry (3 members, the rows that actually record a placement rule as the reusable thing). Score **2.07**.
- **21 `relief.*` families**, one per actual master, each with its own attachment and material process. `relief.glass_cabochon` is moulded glass set into a seat; `relief.mop_disc_in_bezel` is shell captured by a metal bezel; `relief.drilled_dot` is subtractive. These were never one family.

**`ornament.carved_in_solid_face`** kept only `R364` (linenfold, genuinely cut into the host).
`R303.a` became `relief.applied_wreath_ring` — **applied, fixing method unresolved**: the
final audit removed the "glued" claim, since the source shows the applied state but never the
fixing (unlike `R356`'s garland, whose screws are photographed). `R303.b` became
`inlay.inset_swappable_cell` (a veneer cut, not geometry).

**`ornament.marquetry_motif_set`** narrowed to genuine botanical sets (`R184`, `R211`, `R269`,
`R331`). Removed: `R154` → `inlay.closed_line_outline_master` (pure closed line-figures that
retarget by *stretching*, which no botanical motif can do), `R186` → `inlay.keyhole_cartouche`
(a functional marker keyed to hardware, not a field filler), and `R114` → the final audit
split it into **all six masters its count field names**: `inlay.star_scatter_master`,
`inlay.mop_disc_master`, `inlay.abalone_plaque` (flat shell cuts) and
`relief.carved_rosette_patera`, `relief.flower_chain_run`, `relief.carved_palmette`
(host-carved) — not the earlier two-bucket collapse.

**`rail.case_divider` split eight ways** by load, interface, section and receiver schedule:
`rail.drawer_divider`, `rail.panel_frame_stock`, `rail.frieze`, `rail.plinth`,
`rail.back_secondary`, `rail.table_frame`, `rail.chest_lock_and_lower`,
`rail.shared_stock_multi_role`, plus `rail.multi_purpose_unresolved` (`AUDIT_ONLY`).

## 6. Seat frame and duplicate ownership

`rail.seat_show_face` and `rail.seat_secondary_concealed` were split on **finish, not
construction** — same section, same tenon. Merged into **`rail.seat_tenoned`** with surface
and visibility as parameters and front/side/rear as roles in the assembly schedule. 2011.3
proves the merge by carrying both states on one frame. **`R097` split out as
`rail.seat_pinned`** — visible round pegs are a different joint schedule *and* a visible
surface event. The closed seat frame remains an assembly grammar, not a rail mesh.

**`termination.stile_to_floor` deleted.** It duplicated `stile.continuous_leg_and_stile`;
continuing past the lower rail to the floor is a termination setting on the stile recipe, and
`stile.case_corner_board` already carried that axis. `R376` moved there.

## 7. Corrected lane leaders

| lane | leader | score | change |
|---|---|--:|---|
| toolkit_capability | `moulding.profile_toolkit` | **31.41** | unchanged — still first |
| component_recipe | **`inlay.linear_stringing_path`** | **9.45** | leader since v3.1; panel now 2.07 after the final panel cut |
| assembly_grammar | **`seatframe.closed_frame_assembly`** | **3.02** | leader since v3.1; the 22.59 family was dissolved |
| bespoke_hero | **seven** tied | 0.15 | v3's README said eight; the true v3 figure was five, typed uncertainty made it seven |

`moulding.profile_toolkit` survives every correction and remains the cleanest near-ready
work — 13.0 weighted evidence across 14 objects, construction confidence 0.857.

## 8. Score reproducibility

Every factor is emitted at **full precision** alongside its numerator and denominator
(`n_proven`, `n_member_subrows`, `n_construction_uncertain`), and the final audit added an
emitted **`rankability_factor`** (1 for a rankable family, 0 for `AUDIT_ONLY`) so the printed
formula covers every row — previously the `AUDIT_ONLY` zeros were overridden after the
product and only the rankable rows reproduced. Verified:

**`weighted_evidence × confidence_factor × reuse_multiplier × ambiguity_factor ×
breadth_factor × rankability_factor = final_score` — all 122 rows, 0 mismatches**, computed
from the printed columns alone.

## 9. Fail-closed generation

`validate()` runs before anything is written and raises `CorpusError` if a declared row UID
does not exist, a family cites a quarantined or non-asset row, a proven/uncertain row is not
declared as a member, a family id repeats, or a row is marked both construction-proven and
construction-uncertain.

**It caught a real defect on its first run**: `flyrail.moving_assembly` had `R147` marked both
proven and unresolved — the same incoherence as `R192`, which I had fixed by hand without
noticing the second instance. The rail and pivot *are* visible; the unresolved **stop** has no
BOM row and is now a family-level open question rather than a penalty on a well-evidenced row.

## 10. Disposition — an exclusive partition

v3 presented overlapping counts as a partition (`R446` appeared as both zero-weight and
non-asset). Assigned in priority order, the buckets now sum:

| bucket | subrows |
|---|--:|
| assigned to a family | 203 |
| non-assets | 6 |
| quarantined | 13 |
| zero-weight notes only | 13 |
| **unexplained** | **0** |
| **total** | **235** |

The generator asserts both the sum and the emptiness of the unexplained bucket.

## 11. v1 coverage history — my error, corrected

**v3 stated that v1 "promises to list the gap candidates and then doesn't". That was wrong.**
I read v1's tally section and stopped short of the section below it. v1's README, under
*"Known gaps, stated rather than hidden"*, identifies both failures plainly:

> **2 BOMs missing**: `FC-02-60_4_14` (Phyfe footstool) and `FC-06-27_57_1` (chest of
> drawers) — both agents died on API errors mid-response.

Coverage statuses are corrected accordingly: **18 analysed, 2 api failure, 3 page-only
evidence, 1 not dispatched (reason unrecorded)**. The remaining four were not analysed for
different or unrecorded reasons — v1 dispatched 20 agents for 24 candidates without recording
which four were left out.

## 12. Rights classes

v3 said the V&A object was "the only non-open-access candidate". Also wrong. Rights are now an
explicit class per candidate:

All 24 candidates are now **explicitly mapped** — no substring detection survives anywhere
in the generator.

| class | n | candidates |
|---|--:|---|
| unrestricted / open production use | 21 | Met and Cleveland CC0 |
| **conflict_or_restricted** | 1 | **PMA `1974-224-1a,b`** — the object record says Public Domain while the site-wide notice limits use and requests permissions. A conflict between two authority statements is not an attribution licence; restricted until an authoritative licence resolves it (the final audit corrected v3.1's "attribution required" here) |
| noncommercial or restricted | 1 | Brooklyn `76.63a-f` — stored `rightsType` is "Creative Commons 3D", a noncommercial family, not unrestricted reuse |
| page-visible reference only | 1 | V&A `236:1, 2-1869` — `© Victoria and Albert Museum` on all 7 assets |

The first attempt at this classified all 24 as unrestricted, because it pattern-matched the
rights prose and the V&A string contains the phrase *"NOT open access and NOT CC0"* — a
substring test for "cc0" matched the negation. Rights are now stated per candidate, never
inferred from prose.

## 13. Donors

**All six "not locatable" rows are resolved**, and the final audit replaced every
repository-root path with the **asset-specific ledger and `.blend` paths** (e.g.
`docs/asset-library/GH009_FURNITURE_COMPONENT_LEDGER.md` plus the
`outputs/blender-toolkit-pilot/gh009-furniture-*` builds). The claim that the Sinc assets are
absent is deleted. Status is recorded **per donor-to-family relationship** — one package can
be a precedent for one family and incompatible with another — and the final audit added the
missing relations: `GH-001 → stile.continuous_leg_and_stile, stile.case_corner_board` and
`GH-002 → frame.stile_rail_muntin_grid, rail.seat_tenoned`, all `RECIPE_PRECEDENT_ONLY`.

**15 entries: 7 `RECIPE_PRECEDENT_ONLY`, 3 `MATERIAL_ONLY`, 3 `INCOMPATIBLE`, 2
`AUDIT_REQUIRED`. Zero `VERIFIED_COMPATIBLE`.**

| donor | status |
|---|---|
| Sinc GH-001, GH-002, GH-011, GOK-001 | `RECIPE_PRECEDENT_ONLY` |
| Sinc GH-009 | `AUDIT_REQUIRED` |
| Sinc GH-018 | `INCOMPATIBLE` with the quadrant stay; generic moving-eye precedent at most |
| Sinc GH-019 | `AUDIT_REQUIRED`, **donor-ineligible until P39** |
| `structural_oak_joinery_v1` geometry | `RECIPE_PRECEDENT_ONLY` |
| joinery / door / forged-iron materials | `MATERIAL_ONLY` |
| **`structural_oak_door_v1` fixture** | **`INCOMPATIBLE`** with all frame and panel families |
| openwork strap hinge | `INCOMPATIBLE` with the quadrant stay; generic pivot precedent |
| forged fasteners | `RECIPE_PRECEDENT_ONLY` for generation and placement |

**The door fixture is the one worth naming.** v3 flagged it as both the most plausible
frame-and-panel precedent and the most dangerous name-resemblance trap. The trap was real: it
is an externally imported Sinc **ledged-and-braced plank door** — vertical planks on ledges
with a diagonal brace. No stiles, no rails, no muntins, no grooves, no floating panel. That is
the opposite construction to frame-and-panel, and "oak door" would have sold it.

## 14. Files

| file | rows |
|---|--:|
| [`bom_v3.csv`](bom_v3.csv) | 479 |
| [`refined_taxonomy_v3.csv`](refined_taxonomy_v3.csv) | 122 |
| [`bom_row_family_assignments_v3.csv`](bom_row_family_assignments_v3.csv) | 209 |
| [`refined_component_matrix_v3.csv`](refined_component_matrix_v3.csv) | 122 |
| [`evidence_weighted_tally_v3.csv`](evidence_weighted_tally_v3.csv) | 122 |
| [`ranking_v3.csv`](ranking_v3.csv) | 122 |
| [`donor_compatibility_matrix_v3.csv`](donor_compatibility_matrix_v3.csv) | 15 |
| [`quarantined_rows_v3.csv`](quarantined_rows_v3.csv) | 13 |
| [`candidate_coverage_report_v3.csv`](candidate_coverage_report_v3.csv) | 24 |
| [`unresolved_construction_report_v3.csv`](unresolved_construction_report_v3.csv) | 110 |
| [`non_asset_constraints_v3.csv`](non_asset_constraints_v3.csv) | 5 |
| [`v2_to_v3_correction_ledger.csv`](v2_to_v3_correction_ledger.csv) | 156 |

## 15. Narrowed claims

- **`v2_to_v3_correction_ledger.csv` is a categorised correction *summary*, not an exhaustive enumeration.** It records truncated IDs, retired/split/new families, the granularity change and the donor-premise change. It does **not** enumerate every individual reassignment, evidence override, non-asset move or scoring change. The per-row detail lives in the assignment, quarantine and taxonomy tables.
- **`R000`–`R446` are stable only while `component_bom_v1.csv` row ordering stays immutable.** The IDs are positional. If v1's BOM is ever reordered or appended to in the middle, every ID shifts and every table here breaks. **Treat v1's BOM row order as frozen.**
- **v1/v2 preservation is supported by generator write scope and current file state, not proven.** The generator writes only into `v3/`, and v1 and v2 files are unmodified on disk — but there are no baseline hashes and no committed git state, so byte-for-byte history cannot be demonstrated.

## 16. Remaining blockers

1. **`GH-009` requires an exact corpus compatibility comparison** — silhouette, section, topology, construction, joints, interfaces, editability — against the **two** proved panel members (`R363`, `R198`). `R273` and `R381` are quarantined `frame_layout_only` rows and must not serve as comparison targets.
2. **`GH-019` remains blocked by its P39 donor-admission gate.**
3. **No donor is currently `VERIFIED_COMPATIBLE`.**
4. **Single-member families remain the majority.** Per the audit these boundaries are mostly defensible — genuine unique mechanisms, host interfaces, material processes and hero carvings — and the count rose because the relief and R114 masters correctly became their own identities. Not treated as a defect to merge away.
5. **The moulding subset still needs exact profile decomposition** before any builder. The ten observed profile atoms and their mitre / stopped-end / junction rules have not been matched against `GOK-001`.
