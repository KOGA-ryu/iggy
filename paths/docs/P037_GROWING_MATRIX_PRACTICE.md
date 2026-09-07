# P037: matrix practice that preserves saved work

Status: Automated Green; user visual confirmation pending; uncommitted.

Authority: the user specified a frozen P036 working copy, one worker building a
twelve-question matrix chapter, and the current session owning save compatibility
and final integration. This is an authorized capability addition, not a cleanup
or ownership-repair packet. Changes remain uncommitted.

## Player behavior

**Linear algebra → Matrix practice** contains four Integer foundations, four
Swaps and negatives, and four Exact fractions questions. New card IDs are
6101–6112, each at question version 1. All use the existing two-equation matrix
controls and exact checker. The accepted example, ID 6001, remains under
**Matrices and systems → Row reduction**. The [result sheet](P037_MATRIX_PRACTICE_RESULTS.md)
lists each question's answer and tested solving routes.

The default catalogue and separately loadable matrix practice catalogue each
contain 100 records: 27 playable questions and 73 sorting fillers. All fifteen
earlier playable questions keep their identity, content and order. Adding the
chapter removes twelve sorting-only tail fillers.

Reopening an unchanged saved practice restores the same problem, working,
attempts, Undo branches, archived runs, selection order and active queue position
after questions are added or reordered. New questions appear in Contents without
entering the saved draft or active queue. A deliberate title/selection edit
refreshes the future draft; clicking All again includes new problems within the
chosen titles. Only Start set replaces the active queue. Completion still waits
for Next.

## Canonical owners and compatibility

`EquationSorterSession` remains the selection and queue owner. Its selected-home
vector stores order; the former mutable selection mask is replaced by a read-only
mask in the UI projection. A saved selection pool preserves what All or an
underfilled Random selection meant when that draft was made. Restore maps stable
IDs to current homes, validates selected membership and counts against that pool,
and stages all question replay before replacing live state.

`StudyProgressFile` continues to adapt JSON and atomic filesystem replacement.
It compares question stamps by card identity for every saved selected question,
queue entry and retained run. It no longer compares the entire catalogue array
or treats current catalogue position as saved order. Unrelated additions or
changes to questions outside those saved dependencies do not reinterpret work.
Changed mathematics, answer keys or versions of a dependency identify the card
in the incompatibility message. The original file and live session remain
untouched, and saving pauses for that launch.

Format 2 explicitly stores `selection_pool`; it retains the existing command
journals and stamps. P036 format-1 files load directly: the old full catalogue
stamp and saved titles recover their pool. The next state change writes format 2;
loading or idle frames alone do not rewrite the file. The old P036 executable
will reject format 2 safely. The format is bounded by the existing 100-card,
32-MiB-file and 200,000-commands-per-question limits.

`LayeredQuestionSession` and its exact mathematical kernel still own legal moves,
choices, judging, completion, history and replay. The chapter introduces no new
runtime solver, parser, UI control or reference owner. The existing publisher
composes reproducible chapter output into the default catalogue after validating
every generated question version.

## Verification

The starting copy contains current tracked and untracked P036 source/content,
with a SHA-256 manifest. The worker has a separate copy and pure build folder.
The original frozen executable wrote the retained
`tests/fixtures/practice_p036_v1.json` before persistence changes. This fixture
contains completed scalar and graph work plus unfinished fractional matrix work,
wrong answers, help exposure, Undo branches and an archived run.

The persistence regression loads that actual file into an expanded/reordered
catalogue, compares every retained command and exact queue/selection order,
finishes the matrix through the checker, and reopens/replays it again. Additional
cases cover version-1 All and underfilled Random drafts, version-2 Specific
drafts, repeat reloads, deliberate future expansion, changed answer keys with no
version bump, malformed files, duplicate IDs, missing dependencies, external
file changes and failed writes. The existing separate writer and reader phases
prove process-exit persistence.

Release builds of `sorter`, `gallery` and `paths` pass. Nine targeted CTest entries
pass: `paths_practice_save_tests`, the separate write/read phases,
`paths_math_moves_tests`, `paths_matrix_practice_tests`,
`paths_matrix_practice_recipe_tests`, `paths_bracket_recipe_tests`,
`paths_sorter_solve_tests`, and `paths_sorter_input_tests`. The integrated chapter
gate checks 24 routes with 92 accepted moves and 92 deliberate wrong-tile retries.
Its seven Python tests independently check arithmetic, coverage and publication.

The ImGui input harness checks actual chapter selection and solves fractional
card 6112 at 1440×860, 800×600 and 360×480. Its two-row result labels fit their
buttons with padding; original, working, result and history panels stay fixed.
No screenshot, capture or visible window is used. Visual acceptance remains
separate from these headless checks.

Both publishers pass `--check`. The executable validates both bundled catalogues
from `/private/tmp` before graphics or personal progress startup. All 79 bundled
JSON files match their source bytes. All existing question JSON files are
unchanged; only the default catalogue JSON changes among earlier content files.
The pure persistence and chapter executables link only C++/system libraries.

P037 production C++ changes three existing files by **+66/-22 lines, net +44**,
with zero new production C++ files. The authoring tools change by **+261/-2,
net +259**, including one new generator. The chapter adds 26 content JSON files
(recipes, twelve cards, twelve packs and the standalone catalogue). This task
also adds the actual P036 save fixture, two test files and two documentation
files. Separate input-cleanup edits in the shared checkout are preserved and
excluded from these counts; the passing integrated input gate includes them.

Evidence is in `build/matrix-practice-evidence/`: starting hashes, the real P036
writer/build logs, worker transfer receipt and first-example gate, final build
logs, `integrated-tests.log`, `integration-fixture-tests.log`, content integrity,
scoped source changes and `verification.json`. Two integration-test assumptions
needed adjustment: the old four-type Contents count, and a capacity-refusal
fixture whose base now already contained the twelve new cards. Only those two
checks were rerun after correction; both pass. Earlier persistence fixture
development also changed the stamp-tampering case to target a saved dependency.

## User visual check

Launch `b/sorter`. Use the blue Resume set button to confirm that an existing
save brings back the same gold problem, cyan working and green checked branches.
Return to Contents, choose Matrix practice and start a new set. Try one problem
from each type: gold stays fixed, a wrong tile leaves cyan working intact, Undo
retains earlier branches, and a completed green result remains until Next. Close
and reopen once to check that the same colours and working return.

Remaining boundary: changed saved questions are preserved as incompatible files;
there is no automatic migration of mathematical work onto changed problems or
profile/history browser for those files. Existing presentation state outside
practice remains outside persistence. User visual acceptance is pending.

Next candidate, after this checkpoint: started/finished marks beside problems in
Contents, projected from the saved question evidence.
