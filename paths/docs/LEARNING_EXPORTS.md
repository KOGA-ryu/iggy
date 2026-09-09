# Learning exports: authoring into the app

Status: **implemented for authored `.paths.md` packages on macOS/Linux**.
`tools/export_learning.py` exports, installs and publishes through the existing
[learning-document importer](LEARNING_DOCUMENTS.md). The first package adds one
matrix chapter, one reading and one question with all four support levels.
Human visual acceptance remains separate. A prose-to-question parser adapter,
archives and general question migration are future work. Explicit source-folder
[live preview](LEARNING_DOCUMENTS.md#live-markdown-preview) is available for
authoring; published generations remain immutable and load on the next launch.

## Publish a chapter

From the Paths root, with `b/sorter` built:

```sh
python3 tools/export_learning.py publish content/authoring/learning/matrix_foundations
./b/sorter
```

Publication is headless and does not read or write personal progress. It prints
a JSON receipt with the package, store, generation and whether anything changed.
The next normal launch uses `b/learning-store/active.json`. Ordinary rebuilds
copy bundled content into `b/content/`; they do not overwrite this store.

In Library, search **Matrix foundations: two equations**, open the lesson and
use its blue question link. The new problem is `x+y=5, 2x-y=1`. Learn presents
symbolic operations; Practice fills a multiplier/divisor; Solve and Write
accept complete matrices. Given stays gold beside cyan working. The green
answer `x=2, y=3` stays until Next. Earlier questions remain available.

The first publication retains the current bundled document folder. If the app
was using a custom authoring folder, select that same folder explicitly:

```sh
python3 tools/export_learning.py publish content/authoring/learning/matrix_foundations \
  --target b/sorter --base-documents content/write --store build/my-learning-library
./b/sorter --document-store build/my-learning-library
```

On later publications, the active store supplies the current documents.
`--documents FOLDER` still launches directly from a folder and overrides default
store discovery; do not use that flag when inspecting a published chapter.

To produce a portable package without activating it, or install one elsewhere:

```sh
python3 tools/export_learning.py export content/authoring/learning/matrix_foundations \
  --output build/exports/matrix_foundations/1
python3 tools/export_learning.py install build/exports/matrix_foundations/1 \
  --target b/sorter --store build/another-learning-library
```

Export does not create or activate a store. It checks against the current store
when one is available, including references to other installed chapters. The
receiving target always checks the complete proposed library again.

## Authoring inputs

```text
content/authoring/learning/matrix_foundations/
  authoring.json
  documents/
    chapter.paths.md
    shared/row_rules.inc.md
```

`.paths.md` is the only playable interchange document. Use `lesson.v1`,
`choices.v1`, `linear.v1` or `matrix.v1`; reuse `@include` and existing diagram
providers. All `.paths.md` files under `documents/` are entry documents. Keep
unfinished alternatives outside that root. The compiler identifies the include
closure, so the publisher does not parse or rewrite document commands.

`authoring.json` supplies:

- `format: "paths_learning_authoring"` and integer `format_version: 1`.
- Permanent `package_id` and positive integer `package_version`.
- `sources`: source records with `id`, `kind` (`original`, `generated`, `adapted`),
  `title`, `uri`, `revision`, `attribution`, `reuse` and `content_ids`.

Every provided lesson/question needs source coverage. Source records describe
attribution; an author-supplied statement is not a tool-generated fact check.
Keep source snapshots, generator seeds and authoring decisions in authoring
storage. Do not include local file URIs in portable provenance. The exporter
supplies no arbitrary English evaluator, script runner or new geometry code.

Package IDs use 1–64 lowercase letters, digits or underscores. `baseline` is
reserved for the documents retained during first publication. Versions are
positive 32-bit integers. Release bytes are immutable: reusing an ID/version
for changed content is rejected. Identical publication leaves the active record
and its modification time unchanged.

## Canonical validation and structured reports

```sh
./b/sorter --document-capabilities
./b/sorter --inspect-documents --documents content/write
./b/sorter --inspect-documents --document-store b/learning-store
```

All three commands exit before creating the native host or loading progress.
Inspection emits `paths_document_report`, version 1, as JSON on stdout and exits
nonzero on rejection. The report contains:

| Field | Meaning |
| --- | --- |
| `accepted`, `message`, `diagnostics` | Result and source file/line/field diagnostics with stable code families |
| `entry_documents` | Sorted entry files discovered by the compiler |
| `files` | Exact consumed entry/include paths, byte lengths and SHA-256 digests |
| `entities` | Subject/chapter declarations, lesson/question IDs, parentage, templates, links and used figure contracts |
| `catalogue` | Complete resulting catalogue identities and all frozen question-stamp digests |
| `base_catalogue_sha256` | Fingerprint of the catalogue before this document import |

Question digests hash `CorpusStarter::stamp` inside the C++ content owner. The
exporter does not reconstruct question JSON, scrape human diagnostics for IDs,
or implement a second answer checker. The existing exact kernels check linear
and matrix transitions. `choices.v1` validates the authored answer key and
workflow; it does not establish arbitrary scientific truth.

Capabilities come from the current template and figure registries. Package
requirements describe only used templates, figures/parameters and external
reading/question references. Adding an unrelated 3D object does not change a
matrix package's contract. Receipts record the actual target executable hash.

## Package and library layouts

The exported directory is inspectable and portable:

```text
build/exports/matrix_foundations/1/
  bundle.json
  documents/chapter.paths.md
  documents/shared/row_rules.inc.md
  provenance.json
  receipt.json
```

`bundle.json` uses `format: "paths_learning_export"`, version 1. It records the
package ID/version, `entry_documents`, sorted `files`, `requires` and `provides`.
It inventories all document/provenance bytes; the manifest does not hash itself.
`receipt.json` records technical checks actually performed and leaves factual
review and visual acceptance unperformed/pending. It is outside payload identity,
so a different target receipt does not create different lesson content.

Publication stores immutable complete generations:

```text
<store>/
  generations/<SHA-256 of library.json>/
    library.json
    documents/baseline/...
    documents/<package-id>/...
    packages/<package-id>/bundle.json
    packages/<package-id>/provenance.json
  active.json
```

`library.json` uses `paths_learning_library`, version 1, with all file hashes,
current package versions, previously used release hashes and the complete frozen
question-stamp list. `active.json` uses `paths_learning_store`, version 1, and
contains only the bounded generation digest plus its format fields. A sibling
advisory lock serializes publishers; the kernel releases it if a publisher exits.

The publisher captures source bytes into private staging before checking them.
It packages those same bytes even if an author subsequently edits the source;
unchecked replacements are never copied into a validated package. It validates
the complete prospective document library, checks the package's claimed contract
against the compiler report, and compares every existing question stamp.

Files and directory entries are flushed before the generation becomes eligible
for activation. The publisher verifies readback and exercises the same C++ store
reader before atomically replacing the active record on the same filesystem.
A failure before that replacement leaves the previous active record intact.
An interruption at activation exposes an old or new complete generation. If
post-activation directory durability cannot be confirmed, the error explicitly
says the new generation is active; it does not claim a rollback.

Startup verifies manifest identity, inventory, exact file bytes and question
stamps, then imports the captured document bytes through `LearningDocuments`.
A failed active store stops before progress is loaded or written. Staged or
unactivated generations are never discovered as lessons. A running app retains
its existing content until relaunch. Progress and runtime output files must
stay outside the document/store trees.

Filesystem inputs reject symlinks, including ancestor links. Use physical
paths (for example `/private/tmp` rather than its `/tmp` alias). Document limits
remain 128 entry documents, 128 KiB per file, 2 MiB expanded text, eight nested
includes and the existing combined catalogue/step/choice limits. Packages and
generations are bounded to 16 MiB, with at most 4 MiB per metadata file.
The POSIX writer uses Python's standard library; C++ integrity checks use
OpenSSL Crypto, found by CMake. No network service is used by publication.

## Saved work and update rules

Package versions, document/template versions and question versions have separate
meanings. Renaming or reordering files does not change an explicit question ID
or its canonical stamp. Assign IDs once and keep the authoring source-to-ID map.

Every existing question ID and frozen stamp must remain available unchanged,
including questions from the retained baseline and other packages. Raising a
question version cannot make changed mathematics compatible with an old attempt.
A mathematical or frozen-teaching correction needs an explicitly authored new
question ID while the original remains available. Removal and general save
migration require later work in `CorpusPractice`.

Reading outside a question stamp can change in a new package release. Teaching
included inside a typed question is frozen with that question, even when its
source lives in a shared include. The publisher never edits personal saves.
Older generations are retained as recovery material; deletion, garbage collection
and rollback after play have no supported automatic workflow in this version.

## Existing source parser and reviewed authoring adapter

`deterministic-transcript-parser` 0.8.0 was inspected and its builder consulted.
Its public `Parser`, `ParseOptions`, `Parser.parse()` and `ParseResult.to_dict()`
can preserve source and provide a full audit without C++ indexing or libclang:

```python
from transcript_parser import Parser, ParseOptions
result = Parser(ParseOptions(profile="github")).parse(
    source_bytes.decode("utf-8"), input_format="text", source_name=source_record_id,
)
audit = result.to_dict()
assert result.output.encode("utf-8") == source_bytes
```

This package is not required to publish already-authored documents.
[`prepare_source_lesson.py`](../tools/prepare_source_lesson.py) now consumes its
audited source plus explicitly reviewed, hash-pinned authoring decisions and
emits this same `.paths.md` format. The [source lesson workflow](SOURCE_LESSON_WORKFLOW.md)
documents the implemented card 002 pilot, failure diagnostics and commands.
Taxonomy, template, notation, domains, answers and complete steps/teaching remain
explicit authoring decisions. Missing or ambiguous fields stay outside the
importable documents root; unselected source files are never swept into a pack.

The adapter's presentation target is the
[shared textbook standard for equation solving](QUESTION_PRACTICE_FORMAT.md#shared-textbook-presentation-standard).
Claude's Question, Notation, Method, Setup key and Solution sections are source
material for an explicit mapping into that standard. Preserve source and block
identity, conditions and provenance; keep answers/solutions out of pre-answer
projections. Reuse approved textbook definitions and existing figure providers
where their meaning fits. `lesson.v2` now imports the native textbook block and
disclosure format. Figures bound to solver attempts remain future integration
work; this reading-only pilot does not add a geometry or grading owner.

The parser has no Markdown AST, LaTeX parser or exercise model. Transcript cleanup
can change text even with fillers disabled. Its source-preserving profile keeps
all text, but sentence/paragraph units are not trusted math-block boundaries;
CRLF paragraphs may group differently. Use approved block selections with their
qualifiers and context, retaining original bytes and the full audit.

The audit's `report.schema_version` is 1. It carries parser/configuration/input
identity, exact unit slices, decisions and staged edits. Unit offsets are
half-open Unicode character offsets into `prepared_text`; transcript preparation
can differ from original file bytes. IDs/ranges locate one source snapshot,
not permanent questions. Bind selections to that snapshot hash and configuration;
source edits invalidate old selections without silently reassigning content IDs.
`source_name` is a label, so attribution and source revision remain explicit.
Pin the parser version and audit shape, then prove compatibility before upgrades.

The C++ packet/shorthand subsystem and its private single-file writers are not
a lesson format or a durable publication API. Reuse the public source/audit
boundary rather than copying that engine into Paths. The reviewed mapping must
also preserve qualifiers: a correct numeric answer cannot certify explanatory
prose. This pilot explicitly distinguishes the complete prepared problem from
the four-level reduced-system practice.

## Verification

The targeted gate is `paths_learning_export_tests`, alongside the existing
learning-document model and UI gates. It covers reproducible export/install,
unchanged activation, exact include closure, source edits during export,
source diagnostics, wrong mathematics, unsupported capabilities, missing
provenance, file tampering, invented manifest claims, references across packages,
collisions, combined question limits, symlinks, frozen updates/removals,
interrupted activation and concurrent publishers.

The same gate invokes the real published-library reader and `CorpusPractice`
to restore an earlier draft, help exposure and Undo branches without changing
save bytes. It also solves the published matrix question through actual headless
UI input in all four modes at 1440×860, 800×600 and 360×480. No window, screenshot,
capture or image review is involved. The evidence is under
`build/learning-export-evidence/`; manual visual acceptance belongs to the user.
