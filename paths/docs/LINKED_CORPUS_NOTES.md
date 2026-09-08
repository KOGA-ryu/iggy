# Linked reviewed notes — resting checkpoint

The four reviewed Library notes now form a small connected reading path.
Compact cyan **Related notes** buttons at the end of an entry open another
reviewed explanation in the same reader. **Back** and Escape retrace the path,
restoring the previous reading position and Original/Reviewed choice. Escape
at the start of the trail returns to Contents as before.

Search, subject and topic filters stay in place while following links. A
linked target may be outside those results; filtered Previous/Next controls
are then disabled. A new search, filter choice, list selection or filtered
Previous/Next starts a fresh browsing path. Practice returns to the existing
selection and unfinished question. The Library retains its current reading
state while it stays loaded; reading history is not personal save data.

## Authored connections

| Reviewed note | Related notes, in display order |
| --- | --- |
| Gaussian Elimination | Reduced Row Echelon Form |
| Reduced Row Echelon Form | Gaussian Elimination; Rank |
| Rank | Reduced Row Echelon Form; Nullity |
| Nullity | Rank; Reduced Row Echelon Form |

These are explicit reading connections, not a new assertion that every
relationship is a prerequisite. No automatic title matching or links to
unreviewed material are introduced. All four teaching adaptations, citations,
examples and versions remain unchanged. The corpus still contains 930 original
entries, with four reviewed adaptations and 926 awaiting review.

## Owners and bounds

`content/authoring/matrix_corpus_review.json` owns the ordered `reading_links`
map, keyed by permanent corpus entry IDs. `generate_corpus_toc.py` publishes
optional `related` IDs beside each entry's `review`; navigation metadata does
not change the mathematical content version or the Symbols projection.

`MathCorpus` resolves IDs to entries after loading the complete catalogue.
Both publication and runtime reject missing, unreviewed, duplicate and self
targets, unreviewed origins, malformed lists and more than four links per note.
Catalogues without links remain readable. Reordering entries preserves targets.

`MathCorpusUi` owns the reading trail and uses the existing reader, controls
and input path. It retains the sixteen most recent return points. Restoring
scroll waits for the destination content to be laid out so a short note cannot
truncate the return position of a longer note. There are no new runtime files,
gameplay routes, equation solvers or persistence fields.

Original/Reviewed now retains a stable control identity when its label changes.
Linked jumps and returns focus the reader controls, keeping Tab usable after a
button from the previous note disappears. The new input regression exposed and
verified this small focus correction within the linked-reading route.

## Verification and stopping point

The current build graph selects the Release sorter, `paths_corpus_tests` and
`paths_sorter_input_tests`. The targeted gates are `paths_corpus_tests`,
`paths_corpus_toc_tests` and `paths_sorter_input_tests`, followed by the packaged
content-only startup check from outside the repository.

Checks cover stable target identity after reordering, older unlinked data,
invalid links, deterministic publication, pointer and keyboard navigation,
focus loss, multiple returns, a twenty-jump cycle and bounded history. At
1440×860, 800×600 and 360×480, numerical layout/input checks cover link access,
Back control placement, search preservation, Original/Reviewed restoration,
scroll restoration from short to long notes and exact unfinished-question
Resume. No windows, screenshots, captures or image files are used.

The evidence is `build/linked-notes-evidence/verification.json`, with build,
test and startup logs and a diff against the captured pre-change state.
Question cards, source snapshots, mathematical definitions, Symbols lessons
and question-pack metadata are preserved. Changes remain uncommitted, and
human visual acceptance remains deferred.

This is a stopping point. The corpus can be browsed, four reviewed notes can
be read and followed, and their shared definitions are already available in
all thirteen matrix questions. Broad corpus review, new playable singular
systems and the separately owned 3D-object work remain outside this checkpoint.
No further work is scheduled.
