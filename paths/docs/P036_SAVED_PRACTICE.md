# P036: saved practice

Authority: the user authorized choosing and building another capability after
asking for an assessment of their separate math-notes project. This checkpoint
adds automatic practice saving across app restarts. It does not import or edit
that project. The accepted visual controls remain the solving format.

## Player behavior

The rebuilt sorter automatically saves the selected titles, all/random/specific
draft, frozen practice queue, current question position, and started question
runs. Reopening shows the contents page and a blue **Resume set** button. Resume
returns to the same checked working. Finished questions stay finished until the
player chooses Next or Play again.

Wrong answers, hint/next-move exposure, shown prepared steps, mathematical moves,
Undo branches and archived runs survive. Editing a future selection does not
replace the frozen active set. An unstarted random draft reopens with exactly
the same selected questions. The original gold problem and cyan working keep
the established layout; save state uses the existing compact contents heading.

Green means the practice is saved or ready to resume. An amber status reports
a saving problem; hovering exposes details. Solver Contents/Groups text also
turns amber on failure. Ordinary play remains available when saving fails.

## Canonical owners and storage

- `LayeredQuestionSession` retains accepted changing commands when a finite
  gallery opts into journaling. It remains the sole judge, progression owner,
  exact mathematical checker and evidence owner.
- `GallerySession` rebuilds a saved single-question game by dispatching the
  journal through that question owner. It regenerates active target bindings
  and respects already collected answers. Endless and Guided gameplay do not
  record this additional journal.
- `EquationSorterSession` owns the selection and queue snapshot. Restoration
  stages every question and validates stable IDs, titles, versions, selections
  and queue references before replacing any live state. Restored games pause
  behind the existing contents/Resume route.
- `StudyProgressFile` is the JSON/filesystem adapter, consumed by native startup
  and the headless persistence test. It contains no answer or progression rules.
  No conflicting correctness route was added or removed.

The native loop saves after applying an action and checks once more on normal
exit. Idle frames do not rewrite the file. A complete temporary file is closed
in the destination directory before atomic replacement. Temporary creation is
exclusive; failed writes preserve the last saved destination. The adapter checks
for an external change before writing and retains that changed file on failure.

The default path comes from `SDL_GetPrefPath("KOGA ryu", "Paths")`, using
`practice-<catalogue stem>.json`. The default catalogue therefore saves to
`practice-study_practice_v1.json` inside the user's Paths application-data folder.
The path is independent of the launch directory and the build directory.
`--progress FILE` selects an explicit file; `--no-progress` disables persistence.
Content checks return before locating or opening progress. Scripts and bounded
runs never use personal progress implicitly. Output/input path collisions are
rejected before native startup.

Format `paths_practice`, version 1, stores command inputs and a teaching-content
stamp. It does not restore supplied working, correctness flags or completion
flags. The existing domain checker rebuilds those facts. Changed question IDs,
versions, answer keys, working chains or graph definitions reject the load.
A malformed, unsupported or incompatible file remains untouched and disables
saving for that launch. Restore failure leaves the complete prior live session
untouched. The format limits a file to 32 MiB and each question to 200,000
commands; exceeding a limit reports failure without replacing the previous file.

## Verification and scope

All three native targets (`sorter`, `gallery`, `paths`) rebuilt. Eight focused
CTest entries passed: persistence failure cases, a separate-process writer,
a separate-process reader, mathematical moves, prepared solving, sorter input,
gallery integration and the Guided question owner.

The writer and reader reconstruct a three-question set across actual process
exit: completed scalar working with an abandoned branch, a completed graph with
help and a wrong answer, and an unfinished matrix with exact fractions and an
archived run. The reader finishes the matrix, reopens its completed working,
replays it and verifies that archives are neither lost nor duplicated.

Additional checks cover random drafts, partially collected sphere answers,
stale inputs, unchanged idle writes, invalid IDs/versions/revisions/queues,
truncated JSON, changed answer keys without version bumps, external edits and
failed-write recovery. The ImGui harness clicks the restored Resume button and
finishes through symbol/result tiles at 1440x860, 800x600 and 360x480. It checks
save-status geometry and explicit Next. These are headless input/model checks;
no window, screenshot or capture was used.

Native `--check-content` succeeded from `/private/tmp`. The pure persistence
executable links only C++ and system libraries, with no SDL/Vulkan dependency.
All 65 existing source content files are unchanged; all 53 bundled JSON files
match their source bytes.

Production C++ relative to the saved P035 state: **+398/-22 lines, net +376**,
nine existing files and two new files for the live persistence adapter. CMake:
**+9/-1**. Tests: **+233/-1**, including one new persistence test file. This is
an authorized capability addition, not an ownership-repair or cleanup packet.
Prior uncommitted changes are preserved; no commit or push was made.

Evidence is in `build/practice-save-evidence/`: the per-turn baseline and hashes,
scoped diff/counts, build logs, test logs and `verification.json`. The initial
new-target build needed Make's regenerated graph; a later partial-collection
fixture needed its explicit AllAccepted rule. Both are resolved. Final evidence
uses `build-final.log`, `boundary-build.log`, `verification-tests.log`,
`persistence-boundaries.log` for its two successful process phases, and
`collection-tests.log` for the extended persistence cases.

## Remaining boundary and next candidate

This is one local practice save per catalogue filename, with no profile switcher,
cloud sync or automatic migration when stamped teaching material changes.
Grouping arrangements, camera/physics timing, aim misses, graph probe positions
and reference example cursors are presentation or separate-mode state and are
not persisted. The file is not an anti-cheating mechanism or a power-loss backup.
The user's visual acceptance remains pending. P031 and P035 retain their own
earlier pending visual checks.

Stop after this capability. The next candidate is compact started/finished marks
beside problems in the contents page, derived from saved question evidence.

## User visual check

Launch the rebuilt sorter, start a set and make a few moves. Return to contents
and look for the green save message. Close and reopen the game, then click the
blue Resume set button. Confirm the same gold problem, cyan working and green
checked history return. Finish one problem and reopen again: its final working
should remain until Next is chosen.
