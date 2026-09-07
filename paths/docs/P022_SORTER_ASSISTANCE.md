# P022: Auto sort, hints and next actions

Status: **Automated Green; offscreen reviewed; uncommitted; Manual Test Needed**.

The user requested automatic grouping and a way to ask for help at every step,
then chose grouping by maths subject. This checkpoint adds assistance to the
existing sorter. Its suggestions remain optional; manual grouping is not judged.

## Player behavior

**Auto sort** groups remaining classified cards in one move:

| Group | Prepared subject | Mixed-pack count from a fresh session |
| --- | --- | --- |
| A | Algebra | 80 |
| B | Trigonometry | 5 |
| C | Calculus | 5 |
| D | Linear algebra | 5 |
| E | Discrete maths | 5 |
| Dump | Manual use | 0 |

Cards already placed by the player keep their owners. The automatic move is
one transaction, so **one Undo** restores all of its previous memberships.
Auto sort leaves the current grid/inventory open, preserves home and inventory
slots, and clears the old inspection or empty confirmation when it moves cards.
It disables when no eligible cards remain. Untagged cards remain available for
manual grouping; help reports their count rather than inventing a subject.

The default algebra-only pack automatically goes entirely into A. The separate
mixed pack exercises all five groups. Launch it from the Paths root:

```sh
./b/sorter --content content/sorter/mixed_foundations_v1.json
```

**Hint / Next** is available on the initial grid, while inspecting, in an
inventory, during Empty confirmation and after all cards are grouped. The
main screen displays a short next action. The optional help panel shows the
same action, the inspected card's prepared subject/hint, or an available example
when nothing is inspected. Asking for help never assigns or silently inspects
a card and never adds Undo history. Close hint or Escape returns to the same
interaction. If Empty was pending, the first Escape closes help; a second
Escape cancels Empty.

After every card is assigned, one click on a group opens its inventory for
review. The player can return cards or Undo. This remains a free sorter; no
mathematical solving stage starts after grouping.

## Content and canonical owner

Schema version 1 gains optional `subject` and `hint` fields. Both bundled
100-card files now contain these fields for every record; their existing IDs,
equation text and home indices are unchanged. The algebra authoring script
produces the metadata too, while its solution witnesses remain unchanged.
See the [content guide](../content/README.md#equation-sorter-packs) for keys,
limits and an editable example.

`sorterSubjects` is the shared declarative subject-to-group table.
`EquationSorterContentIO` loads declared metadata; the existing validator checks
subject values and bounded printable hint text. Missing metadata preserves
manual use of older v1 packs. Runtime code does not infer subjects from IDs,
parse mathematical expressions or generate explanations.

`EquationSorterSession::dispatch(AutoSort)` builds an ordered batch of currently
unassigned tagged cards and calls `commitTransaction()`. That existing method
validates and allocates before mutation, records one history entry, and
reconciles reserved slots. Empty batches do nothing. The revision guard rejects
stale automatic requests before preparing the batch. Work is bounded to 100
cards; batch preparation is O(N), while existing ID lookups/slot reconciliation
keep total transaction processing O(N²). There is no travel animation or timer.

`view()` computes eligible/untagged counts and guidance from session state.
An ordered rule table selects the next action. `ShowHint` and `CloseHint` own
help visibility without changing memberships. The UI only queues actions and
presents that projection. Its hint panel stays inside the viewport during
live resizing, scrolls long text, and keeps Close visible.

No competing assignment or Undo route was introduced. Six existing production
C++ files change by **+151/−34 lines (net +117)**; there are **zero new production
C++ files**. Two existing JSON packs and the existing authoring tool gain
metadata; the existing tests are extended. The new files are this checkpoint
and one native scenario script. This is an authorized feature addition.

## Verification

The current CMake graph supplied `paths_sorter`, `paths_sorter_tests`,
`paths_sorter_content_tests`, `paths_sorter_mixed_tests` and
`paths_sorter_input_tests`; they built in `build/gallery-port`.

```sh
ctest --test-dir build/gallery-port -R '^paths_sorter_(tests|content_tests|mixed_tests|input_tests)$' --output-on-failure
python3 tools/generate_sorter_fixture.py --check
./build/gallery-port/paths_sorter --content content/sorter/mixed_foundations_v1.json --check-content
```

All four targeted suites passed. The input suite was rerun after the final hint
panel opacity/resize correction and passed. Coverage includes:

- Subject validation, old untagged content, prepared hints and unchanged
  independent checks of all equation results.
- Fresh mixed grouping with counts `0,80,5,5,5,5,0`; partial manual grouping;
  untagged cards; stale/repeated Auto sort; whole-batch Undo and reserved slots.
- Guidance in initial, inspection, inventory, confirmation, completion and
  recovery states; 5,000 mixed semantic actions with ownership/history checks.
- Real mouse/keyboard controls at 1440×900, 800×600, 360×640 and 360×480;
  held Enter; hint close/Escape precedence; resizing while help is open.

[`equation_sorter_assist.script`](../tests/equation_sorter_assist.script) runs
Auto sort, hints, discrete inventory, an individual return/Undo and Empty/Undo
through the native route. Evidence under `build/sorter-assist-evidence/` includes
`sorted-discrete` (800×600), `start-narrow` and `hint-narrow` (360×480), each with
a PNG and state report. The images were inspected for readable group labels,
maths/hint text and accessible controls. `verification.json` records checked
states and hashes; test logs are retained beside it.

No visible window or human playtest was performed. Next is the user's playtest
of automatic grouping and optional hints. Future solving stages should carry
their own optional prepared help and next actions through their canonical owner.
