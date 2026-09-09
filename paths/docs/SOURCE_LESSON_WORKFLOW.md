# Reviewed source lessons

The first complete source lesson is **002: Three points, one formula**, under
Linear Algebra in the regular Library. It reuses the current Claude source,
the existing 13-step Paths guided card and current row-operation definitions.
The native textbook's definitions, numbered equations, references and separate
hint/answer/solution/proof controls now also render imported `lesson.v2` blocks.
The other worker's 3D models remain independent; this reading-only pilot needs
no new model or geometry.

## Current owners and limits

| Responsibility | Existing owner used |
| --- | --- |
| Lossless source audit | `deterministic-transcript-parser` 0.8.0 public `Parser` with `github` profile |
| Reviewed selections and source freshness | `tools/prepare_source_lesson.py` plus the explicit review manifest |
| Textbook structure and redaction | `BookLesson.hpp`, `bookLessonView`, `drawBookBlock` |
| Document syntax, references and template validation | `LearningDocuments` |
| Guided decisions | `LayeredQuestionSession`, using the existing source 002 route and keys |
| Exact reduction, four support levels and progress | Existing matrix support and `CorpusPractice` |
| Portable export and active library replacement | `tools/export_learning.py` |

The source parser preserves text; it does not decide which claims are correct,
turn arbitrary LaTeX into an executable question, or supply an answer checker.
The source-card adapter handles one explicitly reviewed sequential prepared
route with one accepted option per step. A new numerical family still needs an
existing checker or an explicit implementation assignment. An unsupported proof
must not be relabelled as a checked numeric answer.

Card 002 has two deliberately distinct questions:

- **Question 1:** the complete source problem, including setup, all coefficients,
  verification and uniqueness within the polynomial model. Its 13 prepared
  decisions reuse the established card; notation has an explicit TeX map.
- **Question 2:** the reduced 2x2 system, after c=0 has been supplied. Four levels
  use the exact matrix checker. The workspace uses x=a and y=b, while t names
  the polynomial input. This practice does not grade construction of the full
  three-variable system or arbitrary written polynomial proofs.

The exact answer is a=3/2, b=1/2, c=0. Both reduced equations and all three
original observations are checked independently in the tests; the stated 3x3
determinant is -2. Source learner Attempt/setup/solve/Run fields are never
treated as evidence or copied into the learner-facing documents.

## Preparing and publishing the current lesson

From `/Users/kogaryu/iggy3d/paths`:

```sh
python3 -B tools/prepare_source_lesson.py \
  content/authoring/learning/claude_002/review.json \
  --source-root /Users/kogaryu/devil/99-red-booleans/problems/math \
  --parser-root /Users/kogaryu/Documents/Codex/2026-09-07/referenced-chatgpt-conversation-this-is-an-2/outputs/transcript-parser \
  --output build/authoring/source_002_textbook/verified

python3 -B tools/export_learning.py publish \
  build/authoring/source_002_textbook/verified \
  --target b/sorter --output build/exports/source_002_textbook/verified
```

The source and parser roots are local development inputs. Neither checkout is
modified or copied into the runtime; another machine supplies its own paths.
The parser is only needed for preparing source cards. Importing or publishing
already-authored documents does not require it. Preparation never publishes by
itself. Publication asks the actual target app to validate the complete proposed
library before switching `active.json`; unchanged existing questions keep their
IDs, stamps and saved work.

The reviewed source inputs are named individually; the adapter never scans the
source directory for new cards. Unmapped drafts and superseded sibling pages do
not enter the generated documents. Every selected input has a SHA-256 pin:
source card, current prepared card, current row rules, presentation map and
lesson template. A change to any pin fails with `source.stale`, the file and
field, expected/actual digests and an instruction to review the mapping again.
An explicit `draft` status or another parser version also fails. Even a source
edit confined to Attempt changes its full-file pin and requires review.

Preparation retains the exact original page, selected Question bytes, reviewed
mapping and full public parser audit under the output's `audit/`. Only the
generated `documents/` closure and portable provenance enter the exported pack.
Earlier prepared output is immutable: repeated identical preparation succeeds;
different output requires a new reviewed output directory. Do not bypass a
freshness failure by exporting an older prepared directory. Published packages
are deliberate immutable snapshots, not a live mirror of the source folder.

## Repeating this with another card

1. Read the complete selected source and check whether the exercise already has
   an active adaptation. Resolve duplicate or conflicting source pages before
   choosing one. Preserve all conditions and every requested part of the task.
2. Choose the existing mathematical owner and scope the playable task exactly.
   State any reduction or practice variant separately. Do not invent runnable
   evidence from a source's status fields or missing program.
3. Follow `content/authoring/learning/claude_002/review.json`. Select source byte
   ranges and their hashes; pin each current dependency. Use `status: draft`
   while drafting. Do not have a batch generator mark its own output reviewed.
4. Write `lesson.in.md` in the established textbook block format. Supply symbol
   roles, conditions, numbered mathematics, explicit help disclosures and stable
   references. Keep complete solutions out of public introductory teaching.
5. Map every current working state and option explicitly in `presentation.json`.
   Preserve the route's decisions and accepted IDs. `math` values are LaTeX;
   `text` values are prose. `{{prepared_route}}`, `{{row_addition}}` and
   `{{row_scaling}}` in this pilot include the declared existing material.
   Unknown substitution names fail. Nothing in the input is executed as code.
6. Independently solve and check the chosen example, every typed intermediate
   operation, every final condition and distractor. Only then set `reviewed` and
   capture the final input hashes. Preserve attribution and scope of reuse.
7. Prepare, inspect and run the pure model routes; export/publish only that exact
   prepared result. Once played, do not change frozen question mathematics or
   silently reuse its identity for a different problem.
8. Build the affected apps, then give the user a short manual visual check. No
   screenshots, images, windows or font-rasterization probes are part of the
   automated workflow.

## Verification commands

```sh
cmake --build b --target sorter math_lab paths_learning_document_tests paths_textbook_tests -j 4
./b/paths_learning_document_tests
./b/paths_textbook_tests
python3 -B tests/source_lesson_tests.py \
  --parser-root /Users/kogaryu/Documents/Codex/2026-09-07/referenced-chatgpt-conversation-this-is-an-2/outputs/transcript-parser
./b/paths_learning_document_tests --source-store b/learning-store
./b/sorter --inspect-documents --document-store b/learning-store
```

The source integration test uses the matching frozen source snapshot and scratch
directories. It proves deterministic parser audit/output, seven source/mapping
rejections, every source solving route, independent fractional arithmetic,
preserved saves and Undo branches, actual publisher/startup integration and
rejection of tampered exported text. The document test adds malformed block,
reference and disclosure cases and checks that unopened solutions affect reading
fingerprints without changing mathematical question stamps. Existing textbook
tests cover navigation, references, separate disclosures and reading bookmarks.

Visual acceptance is the user's check: launch `b/sorter`, open Linear Algebra →
Three points, one formula → 002: Three points, one formula. Look for the gold
title, teal numbered block headings, separate Show hint/answer/solution controls
and readable equations at increased text size. The two blue question buttons
open the full guided problem and the four-level reduced practice. Cyan working
should survive a wrong move; a green result should remain until Next.
