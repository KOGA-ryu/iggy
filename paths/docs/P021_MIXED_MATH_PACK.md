# P021: mixed maths sample pack

Status: **Automated Green; offscreen reviewed; uncommitted; Manual Test Needed**.

The separate Equation Sorter can now load a prepared 100-card pack containing
80 existing algebra equations and 20 new examples across four subjects. The
player uses the existing inspect, assign, return and Undo interactions. This
checkpoint provides a small sample for testing notation and free grouping,
not comprehensive subject coverage or a sequence of judged solving steps.

## Content and launch

The new file is
[`content/sorter/mixed_foundations_v1.json`](../content/sorter/mixed_foundations_v1.json).
It retains the exact IDs and text of the first 80 home slots from
`equations_v1.json`. Four algebra cards precede each new card; the new subjects
cycle in the order below. Home indices are authored for this separate pack
and remain immutable during its session. The original 100-card algebra file
and its solution witnesses are unchanged.

| Subject | New IDs | Prepared examples |
| --- | --- | --- |
| Trig | 2001–2005 | Sine, cosine and tangent with explicit degree or radian units |
| Calculus | 2101–2105 | Three monomial derivatives and two definite integrals |
| Linear algebra | 2201–2205 | Dot product, vector addition/scaling, determinant and matrix-vector product |
| Discrete maths | 2301–2305 | Set union/intersection, combinations and Boolean operations |

All records use the existing schema version 1: `id`, `home_index`, `text`.
Text is printable ASCII within the existing 96-byte limit. Subject labels
are not injected into the cards, and the runtime does not interpret ID ranges.
The [content guide](../content/README.md#equation-sorter-packs) defines the notation.

From the Paths project root:

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t sorter -j4
./b/sorter --content content/sorter/mixed_foundations_v1.json
```

Both packs are copied beside the executable by its existing content target.
Launching without `--content` still uses algebra. An explicit source path reads
edited JSON on the next launch without rebuilding C++; an open session keeps
its loaded content.

## Ownership and bounded UI correction

`loadSorterContent()` remains the sole file-loading route and calls the
existing structural validator. `EquationSorterSession` remains the canonical
owner of assignment, inspection, reserved slots and transaction Undo.
`EquationSorterUi` presents that snapshot and queues semantic actions.
No competing route was introduced, so no conflicting route needed deletion.

The mixed input regression exposed a first-frame layout fault when a 20-card
inventory began overflowing: at 800×600 the first card shrank from about 246.67
to 242 pixels after the vertical scrollbar appeared. Reserving scrollbar width
with `ImGuiWindowFlags_AlwaysVerticalScrollbar` stabilizes the grid and inventory
from the first frame. This is the only production C++ edit: **one line replaced
(+1/−1, net 0 LOC), zero new production C++ files**. Content deployment and test
registration use the existing CMake file. No dependency or runtime solver was added.

## Mathematical and interaction checks

[`equation_sorter_mixed_tests.cpp`](../tests/equation_sorter_mixed_tests.cpp)
loads the real JSON and interprets its displayed strings independently of
authoring. Its checks are deliberately bounded to this fixture's notation:

- Trig evaluates the stated function and angle unit using standard-library
  functions with absolute tolerance `1e-12`; this is a numerical check.
- Derivatives compare integer coefficients and powers. Definite integrals
  use exact integer cross multiplication of the monomial antiderivative.
- Vector/matrix operations use exact integer arithmetic. Sets and Boolean
  operations are evaluated directly; combinations are checked by enumerating
  subsets rather than repeating the authoring formula.
- Each of the 20 new strings also receives a deliberately wrong result that
  its checker must reject. The test verifies 80 retained algebra identities/text
  and five new cards per subject.
- All 20 new cards pass through the unchanged sorter session, including an
  individual return/Undo and a confirmed Empty/Undo with slot restoration.

The expanded `paths_sorter_input_tests` uses the real loader, ImGui font and
mouse events at 1440×900, 800×600, 360×640 and 360×480. It measures every mixed
expression against the rendered card, then opens the 20-card inventory,
returns a card and restores it through the actual Undo button. Card bounds
stay fixed. Existing pointer, keyboard, focus-loss and recovery tests also pass.

These checks run outside the game. Unsupported notation requires another
independent authoring check; structural loading alone does not establish
mathematical correctness.

## Verification and evidence

The current CMake graph built `paths_sorter_mixed_tests` in `build/sorter-model`
and `paths_sorter` plus `paths_sorter_input_tests` in `build/gallery-port`.
The final targeted gate passed:

```sh
ctest --test-dir build/sorter-model -R '^paths_sorter_mixed_tests$' --output-on-failure
ctest --test-dir build/gallery-port -R '^paths_sorter_input_tests$' --output-on-failure
python3 tools/generate_sorter_fixture.py --check
./build/gallery-port/paths_sorter --content content/sorter/mixed_foundations_v1.json --check-content
```

Native offscreen runs produced images and session reports under
`build/mixed-sorter-evidence/`. All four final images were visually inspected:

| Image/report stem | Size | Observed result |
| --- | --- | --- |
| `grid` | 1440×900 | Mixed opening grid, 100 unsorted cards |
| `subjects` | 1440×900 | All 20 new expressions visible in A, 80 unsorted; return/Empty/Undo restored membership and slots |
| `subjects-small` | 800×600 | Same state in three readable, scrollable columns |
| `matrices-narrow` | 360×640 | Both matrix cards readable in B, 98 unsorted, intact row brackets |

The reusable native scenario is
[`equation_sorter_mixed.script`](../tests/equation_sorter_mixed.script).
`verification.json` beside the captures records the checked report states,
source/deployed content equality and SHA-256 hashes of evidence and the binary.
These artifacts establish automated and offscreen behavior; no interactive
window or human playtest was performed in this checkpoint.

The next candidate is a short user playtest of mixed notation and grouping.
Use that feedback before authoring a larger pack or choosing richer typesetting.
Connecting the sorter to the shooting gallery remains a separate capability.
