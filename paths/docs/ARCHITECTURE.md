# Paths architecture

The Library uses the **same `drawTextbook` screen as `math_lab`**. This replaces
its separate browser, focus pages, imported document reader and adjacent
question/textbook table. `CorpusTextbook` adapts corpus entries and questions
into stable `BookSection` data; it does not own page geometry. `Textbook` owns
Contents/Index navigation, Reading/Exercise mode, numbered-block references,
disclosures, text size and reading bookmarks. `TextbookUi` owns the page layout,
72-character reading measure, typography, native figure regions and footer.

`CorpusPracticeUi` supplies only question controls inside the native Exercise
area. `CorpusPractice` still selects and saves questions; `LayeredQuestionSession`
still grades them. Explicit Next retains that route and moves to the following
question section when the current section's questions are exhausted. Document
figures use existing registered models and `planLessonFigureRegions`; the
3D worker's geometry and controls remain authoritative.

`TextbookUi` owns the shared right-click text-copy menu and uses the existing
ImGui/SDL system clipboard connection. Structured passages supply their original
strings; mixed readings reuse `NativeMath::Document` source ranges and placements
to select whole paragraphs or equations. Copying preserves LaTeX, Unicode and
internal line endings without reading back rendered pixels. `CorpusPracticeUi`
formats question copies from the current session's `supportView`,
`visibleWorking` and redacted review. It includes the current choices and feedback,
without answer keys, future steps or archived working. Open reading disclosures
can be copied individually; closed disclosures never supply a copy target.
Copying has no question command, attempt, bookmark or persistence mutation.

The adapter owns copied strings and lesson blocks for the lifetime of the
borrowed `BookSection` spans. A Markdown preview rebuild remaps reading by
stable section ID and closes old disclosures. Native bookmarks keep their
version-1 generation contract. Imported reading bookmarks use version 2 with
an explicit saved row count, allowing catalogue additions/reordering while
rejecting truncation and duplicate rows atomically. Neither format contains
question attempts. The Library uses `library-textbook-v1.txt` beside the existing
question progress file; preview and `--no-progress` never touch it.

`LayeredQuestionSession::ReadReference` records reference guidance through the
existing guarded command route. It uses separate exposure bits 5–8, while the
existing question-help bits 1–4 retain their meaning. Reading a related example
does not claim that the active question's answer was shown, select its Help
tab, create working, or grade a response. `CorpusPractice` journals the added
`reference` action; all earlier action names/ordinals and question stamps remain
unchanged. Reading-only Markdown edits retain matching attempts. The pure
reading-state regression initializes neither ImGui nor fonts.

Work allocation follows [AGENTS.md](../AGENTS.md#current-work-allocation): the
textbook worker owns the full textbook/application learning experience, including
content, UI, solving, persistence, the content pipeline and figure integration.
The separate worker builds 3D assets/models. This division assigns implementation
responsibility; the canonical mathematical and state owners below remain intact.

`export_learning.py draft` reuses the package capture, compiler-selected include
closure and staged directory writer to create a fresh editable preview. Its
origin record preserves attribution and original file hashes; old audit and
publication receipts are excluded. Existing destinations and release/store
locations are rejected. Draft creation does not load an active store, and its
printed launch command uses the existing session-only `LearningDocumentPreview`
route. No runtime owner, parser, checker or persistence route was added.

Per-choice correction text is optional immutable question content:
`LayeredQuestionOptionContent::wrongFeedback`, decoded from `wrong_feedback`.
The same question validator rejects corrections on accepted choices and bounds
their text. `LearningDocuments` maps `@feedback ID | prose` to this field and
keeps diagnostics at the original directive. Omitted fields do not change old
question JSON or stamps. Supported choices receive their correction only after
the exact checker rejects the response; prepared attempts expose it through
`QuestionReviewAttempt::feedback`, with the existing step correction as fallback.
`CorpusPracticeUi` consumes these projections and wraps prompts/feedback. No new
answer policy or save record was introduced; journal replay reproduces feedback
from the frozen question content. The new linear teaching sequence uses the
existing linear and prepared-choice templates, plus one textbook overview.

The batch producer selects matrix, linear or finite-probability authoring through
a small family table. `teaching_documents()` assembles the same chapter wrapper,
family lesson blocks, practice links and question templates; `choices_text()`
owns the shared choice/answer directive formatting. One compiler/model/export
path verifies and publishes every family. Linear authoring reuses
`question_workflow.py` for bounded instance construction and independent arithmetic
checks, and one Markdown template for prose. That recipe's equation formatter now
serves both typeset and plain output; its duplicate nested plain formatter was
removed. The original 25-card runtime pack still reproduces byte for byte.
The pure batch gate reads canonical reached working displays for either family
instead of parsing matrix-only test projections. No runtime C++ changed.

`author_question_family.py` owns the reusable recipe entry point for new writer
folders. `init` supplies four editable data/prose files; `check` selects a
registered mathematical provider and assembles its groups into one ordinary
chapter. The first provider is the existing bounded linear teaching family.
Its standalone packaging/receipt route was removed. It retains original-case
construction, independent exact certificates and calculated teaching fields.
The shared runner owns metadata, stable namespace IDs, numbered titles, choice
placement, practice links, compiler/model/disclosure gates and a compact handoff
with immutable evidence. It reuses the batch template/certificate functions and
exporter file/provenance contracts. Author inputs cannot select Python paths.
No runtime grammar, judge, renderer or publication route is added; accepted
candidates reach publication through the existing exporter and coordinator.

The [matrix reference](QUESTION_PRACTICE_FORMAT.md#editable-matrix-reference)
connects optional document `@hint` to the existing question-step `hint` field.
`LayeredQuestionSession::supportView` owns its disclosure separately from worked
teaching; older steps without a hint receive a general direction. Learn combines
the current definitions and teaching. The UI displays the catalogue title and
uses its existing math-document renderer for the projected reading. No question
schema, save schema, checker or renderer was added. Omitting `@hint` emits the
same question data as before. The user approved the reference for batching;
the standalone reference remains unpublished.

Batch format 2 reads one Markdown source template for the worked explanations
and supplies exact number/matrix display fields. The output is ordinary
`matrix.v1`/`lesson.v2` content, checked and published by the existing owners.
The package retains format-1 documents byte for byte and adds new worked-card
identities; it never changes the content behind a saved question. Package version
and teaching-format version are separate. No runtime source changed for this batch.

`LearningDocumentPreview`, inside the existing document module, captures bounded
source bytes and sends settled snapshots through `LearningDocuments`. An explicit
`--watch-documents` launch uses in-memory attempts only. `CorpusPractice` retains
exact ID/stamp revisions and rejects replacement of persistence-backed sessions;
no save migration or alternate grading route is introduced. Successful reloads
remap navigation by stable IDs and invalidate lesson/figure bindings and active
input buffers. The UI adapter retains reading position where possible, consumes
compiler errors and keeps its existing source-keyed math renderer. Invalid edits
do not replace the last accepted catalogue. Published generations are not watched.

[Checked question batches](QUESTION_BATCHES.md) use one Python authoring tool,
the existing C++ question owners and the existing learning publisher. Each
bounded family supplies deterministic documents and an independent
arithmetic audit; a pure model gate must replay every generated question before
export/publication. Matrix validation returns static reasons with step/option
locations, which content decoding preserves and the document compiler maps back
to directive lines. No parallel mathematical validator or library store is added
to the runtime. The authoring Practice projection now matches the app's choices
and optional blank without changing any published linear question stamp.

Finite probability emits `choices.v1`. Its producer counts by integer division;
an independent exact oracle enumerates the original finite sample space and
sums equal outcome weights. Before export, compiled givens, working and choice
keys must match that certificate. This is an authoring check, not a new runtime
probability solver. Prepared questions replay one multiple-choice route; the
existing linear/matrix families replay five supported input routes. All use
the same `CorpusPractice` save/history owner. The batch records source digests
and refuses to publish if an authoring input changes during verification.

The six exercise roles are an authoring contract in `build_question_batch.py`.
`reasoning_documents(root, checkers, values, filename)` now supplies one manifest,
substitution and chapter-assembly route to probability and matrix sequences.
Matrix reasoning is a separate additive package attached to the existing
`worked_matrix_practice` chapter. Its exact-rational certificates check original
solutions by Cramer's rule/substitution, row-addition inverses, elimination goals,
the first omitted constant operation, and candidate-pair residuals. The shared
certificate wrapper identifies the card/role on arithmetic failure. No runtime
question model, renderer, save schema or 3D owner changes for this sequence.
`validate_role_sequence()` checks the ordered role manifest and prerequisites;
`verify_role_content()` compares compiled prepared questions with a family's
independent mathematical certificates. `chapter_text()` supplies the common
chapter wrapper for generated repetitions and authored role questions alike.
The probability role sequence keeps its wording in ordinary Markdown and its
original cases in `sequence.json`; the certificates enumerate finite outcomes,
sum exact weights and verify set relationships. Role names do not create runtime
solvers, mastery scores or new save records. Format 2 appends six questions and
one reading while retaining format-1 probability content verbatim. The builder
contract is documented in [EXERCISE_ROLES.md](EXERCISE_ROLES.md).

Practice now consumes the same symbolic choices and `SupportAction::Choose`
route as Learn. `LayeredQuestionSession` still gates responses by level, anchor
and revision and checks the exact mathematics. Its projection keeps Practice
teaching closed until requested. The existing submission action/level and guarded
journal distinguish tile input from optional `SubmitBlank` input without new
question stamps or a save schema. `CorpusPracticeUi` only renders the choices
and an optional typing disclosure; an existing draft opens that disclosure on
first display. Correct Practice tiles clear the obsolete step draft, wrong ones
retain it, and written levels still cannot consume prepared choices.

The [learning publisher](LEARNING_EXPORTS.md) adds a structured inspection report
and verified published generations. `LearningDocuments` reports its consumed
file bytes, catalogue declarations and canonical `CorpusStarter` stamp digests.
Its store reader verifies inventory and imports the captured bytes through the
same compiler before progress can load. OpenSSL Crypto supplies SHA-256 without
introducing a renderer dependency. `tools/export_learning.py` owns transport,
immutable package versions, publication locking and atomic activation; it asks
the app to validate the full proposed library and preserves every existing
question stamp. `tools/prepare_source_lesson.py` is now a producer of this same
contract: it uses the existing source parser's public lossless audit, explicit
hash-pinned source selections, current prepared routes and reviewed templates.
It does not infer answers or grade prose. Source and draft rejections happen
before output installation; publication remains owned by the existing exporter.

Imported `lesson.v2` uses the native textbook's owned `BookBlock` passages,
`bookLessonView` redaction and `drawBookBlock` rendering. Both applications link
the same `paths_textbook_ui` implementation. `BookLesson.hpp` contains the shared
reading data without pulling geometry or exercise owners into corpus parsing.
Public reading references omit closed help; complete reading fingerprints include
it. Question stamps and saved attempts remain owned by the existing question
session and persistence routes. There is no second textbook renderer or judge.

The [learning-document pipeline](LEARNING_DOCUMENTS.md) adds bounded `.paths.md`
startup import. `LearningDocuments` compiles versioned templates through the
existing corpus and question validators and appends the complete folder
atomically. ToC records, includes and stable lesson/question references remain
content data. `DocumentLessonUi` consumes the compiled reading and named figure
configuration; the existing `MathObjects` and `MathObjectScene` own diagram
behavior and geometry. The same `LayeredQuestionSession` and `CorpusPractice`
own all imported question judgments and saved evidence. No document code runs,
and neither the textbook nor native rendering ownership changes.

`matrix.v1` extends that pipeline to the existing exact two-row matrix kernel.
`QuestionSupportContent::model` selects the typed family; the same support nodes
hold `MathWorkingValue` for linear or matrix working. The shared support action
route keeps drafts, help exposure, branch history and save replay. The matrix
kernel checks row-operation responses, exact solution-set equivalence for
written matrices, and final substitution; the UI consumes its goal and input
instructions. No separate matrix practice session or persistence format is added.

The [four-level question contract](QUESTION_PRACTICE_FORMAT.md) and
[authoring workflow](QUESTION_AUTHORING_WORKFLOW.md) now have one implemented
linear family with 25 questions. Support is separate from mathematical complexity,
taxonomy level and interaction mode. `LayeredQuestionSession` owns its typed
responses, working, exposure and bounded native-input projection. The existing
exact linear kernel verifies each equation's solution set and original
substitution. `CorpusPractice` replays guarded semantic actions and draft edits
from version-2 saves, while still reading version-1 prepared saves. UI adapters
only collect input and display the owner projection. The authoring generator
publishes the optional typed support content explicitly with `--publish`.
No second answer owner or bulk catalogue loader was added. Interactive 3D
follows this question foundation.

[Matrix reasoning](MATRIX_REASONING.md) adds eight follow-up questions to the
same `CorpusPractice` bank. `LayeredQuestionSession` remains the sole owner of
accepted answers, prepared working and attempt history. Optional `reading_refs`
identify existing textbook definitions and propositions; `CorpusPracticeUi`
projects their neutral bodies without constructing a textbook or exercise
session. Focus and Method change only presentation. Reading metadata is outside
the mathematical save stamp. The regular question and textbook/lab exercise
owners remain distinct; any future figure bridge must consume the question's
canonical matrix and explicit right-hand side, not create a second judge.

[Motion lessons](MOTION_LESSONS.md) adds `paths_motion` for five authored
kinematics chapters. `MotionLesson` owns the analytic journey, goal checks,
clock and attempt records; `MotionScene` uses those coordinates in the existing
SceneFrame contract. `MotionLessonUi` sends semantic actions from graph handles,
sliders and symbolic tiles. `MotionProgressFile` validates and saves this
independent state. Contents opens the module without changing existing question
or practice-save owners, the native host, or the separate math lab. Further
Motion development is shelved at the user's request.

[Corpus starters](CORPUS_STARTERS.md) connects 278 authored questions to the
six-subject Library hierarchy. `CorpusPractice` selects independently frozen
`LayeredQuestionSession` instances and persists their canonical command journals;
the existing question owner alone judges answers and publishes working/history.
`CorpusPracticeUi` displays native symbolic choices in the Library workspace.
This collection consumes no sorter slots and leaves the old catalogue and
practice-save owner unchanged. The explicit authoring map and generator prove
subject/chapter/subcategory coverage without modifying pinned source notes.

`FiniteQuotient.hpp/.cpp` own group-map preservation, fibers/kernels,
representative-independent partition operations, subgroup quotients and finite-ring
ideals. A partition retains all possible product classes and marks ambiguity
explicitly; nonnormal subgroups never receive an invented quotient multiplication.
The Quotients and Homomorphisms Lab animates collapse without changing these
facts. It shares `SnapshotBuilder::operationCell` numeric mesh cells with Finite
Algebra, plus existing inspector, picker and playback contracts. One-class
quotients and proper images are supported. See [P068](P068_QUOTIENTS_AND_HOMOMORPHISMS.md).

`FiniteAlgebra.hpp/.cpp` own closed operation tables on two to six elements,
identity/inverse and law witnesses, generated subgroups, left/right cosets and
standard modular-ring analysis. The Finite Algebra Lab uses those results with
existing selected-row controls and node picking. Its operation cells and numeric
glyphs share one bounded triangle mesh. Subgroups require a verified group;
the ring layer explicitly uses Z/nZ independently of the stored editable table.
[P067](P067_FINITE_ALGEBRA.md) records conventions and reuse boundaries.

`FiniteGraph.hpp/.cpp` own a six-node relation and its BFS, property witnesses,
equivalence classes, and three-by-three matching analysis. The Relations and
Graphs Lab constructs arrows, a binary adjacency mesh, property counterexamples,
class grouping and alternating-path animation from that data. The adjacency mesh
avoids consuming a separate primitive per matrix cell. Matching uses the existing
relation's 0..2 -> 3..5 block and preserves the other stored edges. Semantic input
stays in `MathObjects`; the existing selected-row inspector and node picker are
reused. [P066](P066_RELATIONS_AND_GRAPHS.md) defines the bounded interfaces.

`FiniteMapsInput` / `analyzeFiniteMaps` supply the Sets and Maps Lab with
bounded finite-map truth: fibers, composition, classifications and probability
pushforwards. The model constructs trays, element beads, arrows, grouping and
route animation from that analysis. One binding table owns active input counts
and destination bounds. `MathObjects::parameterMaximum` serves both action
validation and inspector ranges; shared dropdowns omit out-of-range choices.
The model explicitly distinguishes undefined probability from a zero mass.
See [P065](P065_SETS_AND_MAPS.md) for its four layers and reuse boundaries.

`SnapshotBuilder::linkedPlot` assembles a sampled series, its scrubbing
parameter and an explicit current-value marker in one call. Forty constructions
across 17 asset families use it. The existing bounded sampler still generates
129 points; additional comparison series use the same `curve` method. The model
supplies the marker from its mathematical state, so the marker need not lie on
the first series (for example, a Fourier target/sum comparison). Read-only plots
pass `MathParameter::Count`. Sampling domains, parameter availability, undefined-
case handling, colours and series ordering retain their existing owners.
Discrete traces and parametric loops continue to use their existing builders.
No renderer or input route changes are needed.

`SnapshotBuilder::labelledArrow` pairs a vector with its explicit label position
and colour. `vectorResidual` pairs a vector with the segment from its endpoint
to a target. Fourteen labelled-vector sites and three residual constructions
share these helpers across Linear, Tensor, Quadratic, PSD, Patch, Rigid and QR.
Callers own mathematical endpoints, display scales, label text, semantic roles
and residual widths. Rank, regularity and perpendicularity remain model decisions;
the builders do not infer them. Zero vectors keep their labels while the existing
primitive threshold omits zero-length arrows and segments. Part and label order,
IDs and picker bindings are preserved.

`MathParameterSpec::control` owns static inspector metadata for all 39 assets
and 544 parameters: required group, optional short label, coordinate row and
component labels, and selected-point or mode-slot binding. The inspector consumes
those definitions directly; its separate metadata, tuple and selected-range
registries are removed. New controls declare their bindings beside their limits
and defaults without another inspector registration. A tuple is declared on its
first parameter; all selected components name the selector and selected value.
The registry checks reject missing groups, invalid rows and cross-owner bindings.

Playback bindings for every asset also live beside their parameter definitions.
A constexpr owner/layer lookup selects the animation parameter; the existing
semantic dispatcher still owns play, pause, advance and parameter mutation.
Layer masks use bits 0–3 (1, 2, 4, 8). Numerical predicates such as QR rank,
Polar iteration availability and Curve profile validity stay in `MathObjects`.
No calculation, geometry, preset, renderer or learning-state behavior changes.

[P064](P064_QR_LEAST_SQUARES.md) adds `qr`. `QrLeastSquares` owns a bounded
real 3x2 column-pivoted Gram-Schmidt factorization, projection, minimum-norm
coefficients and null directions. `MathObjects` projects the state into linked
vectors, rectangular matrices, error graphs and a coefficient-space surface.
Seventeen appended controls reuse the compact selected-vector row, endpoint
picker and construction playback route. No renderer or learning-state owner
changed.

[P063](P063_POLAR_DECOMPOSITION.md) adds `polar`. `PolarDecomposition` owns
a bounded one-sided 3x3 SVD, orthogonal/PSD factors, numerical rank, null-space
extensions and the Higham iteration trace. `MathObjects` projects that state
into marked solids, comparative panels, matrices and convergence graphs.
Fourteen appended controls reuse the inspector and playback routes. Geometry
handles reflections and rank loss without changing the renderer or learning
state owners.

[P062](P062_DISTANCE_GEOMETRY.md) adds `distance`. `DistanceGeometry` owns
four-point squared-distance matrices, normalized Gram analysis, reconstruction,
rank and zero-sum impossibility certificates. `MathObjects` projects valid
embeddings or explicitly separate requested-length bars. The cone layer mixes
squared-distance matrices through the same kernel. Twelve appended controls
reuse the existing compact rows and playback route; no renderer or learning-state
ownership changes.

[P061](P061_PROBABILITY_SIMPLEX.md) adds `simplex`. `Simplex` owns normalized
three-outcome mixtures, moments, entropy and support-aware relative entropy.
`MathObjects` projects those values into an affine triangle, sampled height
surfaces, convexity chords and supporting-plane plots. Coupled probability
controls validate before mutation and reuse the compact inspector. Finite KL
faces remain explicit at boundary references. No renderer, content or
learner-state ownership changes.

[P060](P060_BRIDGE_TRUSS.md) adds the `truss` provider. `Truss` owns planar
joint geometry, member/support equilibrium, complete-pivot factorization and
load sampling. Full-path force-cap maxima are evaluated at path vertices.
`MathObjects` projects the same state into solid members, joint markers, force
polygons, reaction plots and rank diagnostics. Twenty-six appended parameters
use shared compact controls; the existing picker selects one of six joint-offset
tuples. Sweep playback represents quasistatic loads. Content and learner-state
owners, renderer and GPU capacities remain unchanged.

[P059](P059_RIGID_BODY_ROTATION.md) adds the `rigid` provider. `RigidBody`
owns component mass properties, principal moments, body-to-world orientation
and bounded cached RK4 free rotation. `MathObjects` projects the same assembly
and state into existing primitives, axis trails, inertia tensors and invariant
readouts. Fifteen appended controls and four complete presets use the shared
compact inspector and half-speed playback. Cache replay makes scrubbing
independent of playback history. Content and learner-state owners are unchanged.

[P058](P058_VIBRATING_MEMBRANE.md) adds the `membrane` provider. `Membrane`
owns bounded closed-form modal evolution, spatial derivatives, coherent duplicate
mode grouping and integrated energy. `MembraneGeometry` publishes the existing
indexed surface and its analytic normals. `MathObjects` supplies 28 controls,
four atomic presets, retained layers, half-speed playback and linked readouts.
Compact mode selection exposes only the selected slot. The figure registry,
renderer, content pipeline and learner state retain their existing owners.

[P057](P057_PARAMETRIC_PATCHES.md) adds the `patch` provider. `BezierPatch`
owns bicubic evaluation, analytic derivatives, regularity, curvature and bounded
area estimates; `PatchGeometry` produces an open indexed surface. `MathObjects`
owns 53 named controls, four atomic presets, four layers and existing model
checks. The shared control-point snapshot now holds up to 16 points; curves and
lathes retain explicit counts. Parameter IDs widen to 16 bits without renumbering
existing entries, and preset capacity becomes 64 to hold all patch coordinates.
No serialized numeric parameter format is introduced. Compact rows show only the
selected XYZ triple; selection does not mark an example Custom. The renderer,
content pipeline and learner state keep their existing owners.

[P056](P056_COMPACT_MODEL_WORKSPACE.md) separates pure `MathObjectLayout`
rectangle planning and control metadata from native `MathControlUi` widgets.
The lab owns panel sizes, search, selected label mode and session-only collapse
memory per object/layer. Its widgets queue semantic actions before the next frame;
all linked representations consume one model revision. The textbook reuses only
the scalar row widget and retains its authored control whitelist and separate
reading/practice route. `MathObjects::ResetParameters` restores a mask atomically,
validates coupled profile heights before mutation, and invalidates affected walk
history through the same parameter-touch rules as ordinary edits.

[P055](P055_BOOLEAN_SOLIDS.md) adds `boolean` with pure defining-field and
indexed-contour kernels. Primitive evaluation, Boolean composition, gradient
regularity, probe intersections and midpoint volume share one prepared solid.
A bounded surface builder publishes through the existing indexed mesh contract.
`MathObjects` owns all controls, presets, linked tables/plots and challenges;
the scene, renderer, question/content and document owners remain unchanged.

[P054](P054_LATHE_LAB.md) adds `lathe` through pure profile/integration and
geometry kernels. `MathObjects` retains the same semantic ownership and adds a
bounded indexed surface alongside the existing grid. `MathObjectScene` validates
and publishes that mesh through the existing shader/host. Profile selection now
carries its explicit point count and action parameter; playback button text is
object metadata. Presets and chapter/document providers use the same registry.

[P053](P053_CURVES_AND_SWEEPS.md) adds the `curve` provider. A pure fixed-storage
Bezier kernel owns interpolation, derivatives, arc-length bounds/inversion and
frame transport. `MathObjects` owns its four layers and emits the same surface,
primitive, plot and table snapshots. Presets now hold up to 24 atomic parameter
updates. `MathCurveView` supplies control-point selection to the lab UI, which
sends existing parameter actions. Playback rates are declarative per object;
existing objects retain their original rates. Geometry and native ownership
remain in the existing scene and host.

[P052](P052_CONVEX_GEOMETRY.md) extends the existing `MathObjects` owner with
PSD-cone and norm-ball models, four layers each. The same fixed snapshot carries
all numerical values, plots, 2x2 matrices, surfaces and primitive placements.
Object presets are declarative data applied by one atomic semantic action.
The lab matrix adapter respects the declared dimensions. `MathObjectScene`,
the renderer, textbook lesson owners and source cards remain unchanged.

[P051](P051_DETERMINANT_VOLUME.md) adds the determinant lesson through a reusable
`ObjectLessonSpec` and two retained MathObjects instances. Explicit exercise
bindings separate source boards, systems and objects. The section registry drives
navigation, storage and CLI bounds; immutable bookmark generations preserve
complete older catalogues. The common figure UI consumes the active owner
snapshot, controls and readout selections without recomputing mathematics.

[P050](P050_SYSTEM_SOLUTION_SETS.md) reuses the spread and equation-plane provider
for explicit Ax=b systems. `SystemLesson` owns authored givens and practice
judgments; two retained `MatrixBoard` instances own exploration and practice row
history. The shared affine-space analysis publishes numerical rank, consistency
and solution families. Prediction results stay redacted until Submit or Reveal.
Existing reading bookmarks migrate by stable section IDs.

[P049](P049_LIVE_TEXTBOOK_FIGURES.md) adds the reusable `LessonSpread` layout
and topic figure bindings. `RowPlaneFigure` projects the existing MatrixBoard
into equation planes and a numerical null space; its exploration settings never
edit the board. `TextbookFigureUi` consumes that projection through the existing
MathObjectScene/NativeMath adapters. The active pane supplies one SceneFrame
packet to the unchanged native host after the UI callback selects its scene.

[P048](P048_TEXTBOOK_SECTION_FORMAT.md) adds variable-length typed lesson blocks
and stable internal references. `Textbook` owns independent help disclosures
and publishes only opened passages; reading cannot rewrite exercise evidence.
`TextbookUi` renders section 1.2 through the existing `NativeMath` adapter, whose
implementation and Library ownership remain unchanged.

[P047](P047_INTERACTIVE_TEXTBOOK.md) adds `paths_textbook` for chapter content,
reading navigation and bounded bookmarks. It retains one existing MatrixBoard
owner per chapter exercise. `TextbookUi` embeds the reusable grid/workspace;
reading position and exercise evidence remain separate. `LabPage` selects the
textbook, objects or standalone board in the native math lab.

[P046](P046_MATRIX_BOARD.md) adds `paths_matrix_board`, a pure owner for bounded
complex matrices, authored exercise instances, row operations, pivot traces and
numerical checks. `MatrixBoardUi` presents its disclosure-aware snapshots inside
`math_lab`; the twenty-two object models and native host retain their owners.

The [native equation panel](NATIVE_EQUATION_PANEL.md) adds `NativeMath`, a
MicroTeX-to-ImGui adapter with bundled fonts and bounded layout caching.
Library owns its four-sample panel and presentation state; the existing
Vulkan host owns all GPU resources. Question and corpus mathematics retain
their current owners. [Library typesetting](LIBRARY_TYPESETTING.md) extends
that same adapter with source spans, prose wrapping, inline baselines and
display equations. The reader owns Raw source / Typeset and restores that
choice through its existing reading trail. Unsupported notation remains source.

The [linked-note checkpoint](LINKED_CORPUS_NOTES.md) adds explicit related-entry
IDs to the corpus. `MathCorpus` resolves and validates them; `MathCorpusUi`
owns the bounded reading trail and restores reader position without changing
filters, question state or persistence. The existing Symbols content is unchanged.

The [matrix corpus review](RANK_CORPUS_REVIEW.md) provides four separately reviewed
teaching adaptations to the Library and reuses their definitions in all thirteen
matrix questions. Original source records remain unreviewed and immutable;
the existing question and save owners are unchanged.

The [corpus table of contents](MATH_CORPUS_TOC.md) adds `paths_math_corpus` for
bounded source-reading records and filtering, and `MathCorpusUi` within the
existing sorter Contents. The pinned corpus is explicitly unreviewed. Its
930 reading entries do not occupy sorter slots or alter question/save evidence.

The reusable [notation feature](MATH_NOTATION_DESIGN.md) adds a pure authored
definition/token model and `paths_notation_ui`, shared by the sorter's prepared
and mathematical-move workspaces. Question packs explicitly link lessons;
the component never infers symbol meaning or judges a player move. Existing
solver, reference-example, save and 3D-object owners remain in place.

Authority: the user's request to isolate the math game in its own `paths/`
folder and give it a title screen, game variations, objectives, statistics,
and scoring experiments. This supersedes plans to extend First Move inside
the parent engine. Historical First Move documents are reference material.

## Product and project boundaries

Paths is its own CMake project and executable, physically under
`/Users/kogaryu/iggy3d/paths/` for now. It must configure, build, and run after
this folder alone is copied elsewhere. It does not invoke the parent CMake,
link the parent `iggy3d` target, load a Creative document, or require parent
shaders/assets. System SDL3 and Vulkan remain declared external dependencies.
The user separately connected this folder's own Git repository to
`git@github.com:KOGA-ryu/paths.git`. Local commit `2308995` records the
foundation through P013. Movement setup and content organization are recorded
together in the subsequent local checkpoint commit.

P001 carries the existing two playable modes into that boundary. Guided
Questions teaches one equation in layers. Quick Hunt classifies a row of
equations and banks correct rows. They are choices under Paths, not two
separate applications. New mode ideas become later playable additions.

The first title screen says `PATHS` and `One problem. More than one way
through.` It offers those two working modes, `SESSION STATS`, text size,
and `QUIT`. It does not display a gallery of disabled future features.

## Owners and dependency direction

```text
                    native input / pointer / script
                                |
                     semantic app dispatcher
                       /                 \
              title/navigation       active mode model
                                           |
                                  immutable run evidence
                                    /             \
                              statistics       scoring rule
                                    \             /
                                      presentation
                                           |
                                  native Vulkan host
```

| Owner | Decisions |
| --- | --- |
| Content catalog | source identity/revision, givens, roles, prompts, option keys, explanations |
| Mode model | legal actions, checked attempts, help exposure, completion, game-specific facts |
| App dispatcher | title/stats/mode navigation, active input context, forwarding to the canonical model |
| Objective evaluator | whether a frozen run objective has been met |
| Statistics projection | counts and denominators from actual evidence |
| Score evaluator | a named, versioned game's points from evidence and frozen rules |
| UI | layout, focus, presentation text, animation; no answer keys or point calculations |
| Native host | SDL events, Vulkan resources, ImGui frame lifecycle, capture and presentation |

P001 retains the already working Hunt score in `HuntSession`; its stat view
reads that value. P002 establishes the separate score owner and removes the
old point-calculation route in the same bounded change. Do not have two live
point authorities during the migration or claim that P001 already implements
the future objective/scoring interfaces.

The copied model namespace may remain `iggy3d::first_move` during P001. It is
a source namespace, not a link to the parent project. New app/host types use
`paths`. Rename old namespaces only when it materially improves ownership;
do not turn migration into a cosmetic rewrite.

## Folder and target map

```text
paths/
  app/main.cpp                     First Move startup, CLI, scripts and reports
  app/gallery_main.cpp             gallery menu, gameplay/workshop UI, CLI, scripts and reports
  src/runtime/first_move/           existing pure models
  src/ui/FirstMoveUi.*              shared app dispatcher, title/stats/mode presentation
  src/ui/LayeredQuestionUi.*        guided question view
  src/ui/GalleryMenu.*             gallery selection, pending setup and session lifetime; no SDL/Vulkan
  src/platform/NativeVulkanHost.*   standalone SDL/Vulkan/ImGui host
  src/scene/GalleryScene.*          pure scene actions, geometry, camera, and picking
  src/scene/TargetMotion.*          route preparation, common tick advancement and time preview
  src/runtime/gallery/GallerySession.*  target/question composition and challenge transactions
  src/content/QuestionContentIO.*  playable JSON decoding and shared validation
  vendor/iggy3d/src/                attributed math, camera, motion, allocator/capture helpers
  shaders/                         copied first-room vertex-colour shaders
  third_party/imgui/                exact pinned SDL3/Vulkan ImGui subset plus license
  content/cards/                   playable question JSON in the runtime schema
  content/packs/                   question file lists and ordered practice decks
  content/authoring/               retained 002/013 authoring JSON; separate format
  content/source_snapshots/        read-only source copies and provenance
  content/reference_cases/         planning/reference specifications
  tests/                           copied baseline plus migration-boundary tests
  docs/                            active packets and source provenance
```

The [content folder guide](../content/README.md) identifies the live editing
path. The gallery loads files explicitly listed by a pack; it does not discover
all JSON files under `content/`. Authoring cards retain their original bytes
and resolve their source snapshot fields from the `content/` root through the
planning validator. They are not inputs to the gallery JSON loader.

`paths_model` contains pure C++ game models. `paths_ui`
depends on `paths_model` and privately on SDL/ImGui. `paths_native` contains
the host and copied helper implementation, with private SDK/ImGui dependencies.
`paths` composes those targets. `paths_imgui` is the local pinned dependency.
No target uses `../src`, `../apps`, `../build`, an absolute iggy3d source path,
or an absolute visualization path for a build input.

The ImGui Vulkan backend embeds its default UI shaders. The existing math
startup remains UI-only. The user-authorized P005 engine port adds a separate
`gallery` startup with a 3D Gallery Workshop. `paths_scene` owns its pure
model and copied math/camera/waypoint kernels. `GalleryScene::dispatch` is the
sole mutation route for pointer, keyboard, developer UI, and script commands;
it does not add another active-mode variable to First Move.

`NativeVulkanHost` remains the sole native/GPU owner. Its optional scene path
ports the parent's primitive triangle pipeline to the host's existing render
pass, adds a D32 depth attachment, and draws indexed vertex-colour geometry
before ImGui. The host transposes the parent's row-major CPU matrix for GLSL.
The copied shaders are compiled with glslangValidator and embedded into the
executable at build time. Paths requires no parent shader directory at runtime.
P005 ports the bounded gallery foundation, not the entire Creative editor.
See [P005_ENGINE_PORT.md](P005_ENGINE_PORT.md) for controls, scope, and evidence.

P006 adds `paths_gallery_model`, linking `paths_scene` and `paths_model` without
SDL/Vulkan. Its `GallerySession` owns one scene and one shared question session.
Typed Tick, Pause, Viewport and Shoot commands enter through its dispatcher.
Shoot carries the exact presented frame and challenge identities. The scene's
read-only `hitTestPresentedFrame` is the one geometry query used by gameplay
and workshop selection. Question correctness remains in the question model's
single `judgeOption` route, reached by Guided CheckAnswer or arcade SubmitOption
under a frozen interaction policy.

The shared question model now owns immutable content, variable steps/choices,
stable evidence IDs and accepted/collected sets. The five foundation questions
load from the P010 prepared JSON pack. P011 and P012 add separate packs for
cards 002 and 013 using that same contract and runtime owners.
The original Guided startup retains its select/check/recovery behavior.

`TargetMotion` owns all route evaluation and seeking; every scene object goes
through its fixed-tick advance operation. Waypoints delegate to the local
ported kernel. A pause checkpoint bounds preview to the following 60 seconds,
including dwell and reversal. `GalleryScene` owns the shared sphere mesh and
Spawning/Active/Popping/Retired phases. Target reset, new question state and
the complete seeded colour assignment are prepared together before commit.
The UI processes input and simulation before drawing the current board, so a
transition cannot display an old answer mapping beside new targets.

The gallery header shows correct targets, wrong answers, aim misses and
completed questions only while stopped. It does not create a competing point rule;
P002 remains the owner of future score/bonus projections. See
[P006_TARGET_FOUNDATION.md](P006_TARGET_FOUNDATION.md) for current scope/evidence.

P019 establishes the gallery's continuous arcade loop. A wrong answer retains
the current prompt, targets and collected set while motion and input continue.
The existing question owner still advances after its required correct answer
or answer set; targets pop and the next step/question starts automatically.
There is no mistake-triggered pause, review, retry round or game-over route.

`GallerySession::submitAnswer()` forwards button choices and target-hit options
to the question owner and consumes its recorded verdict in one place. It sets
a typed `GalleryFeedback` and an expiry 30 scene ticks later. `view()` exposes
it for half a second of active time;
the indicator does not gate input or advance a question. A later accepted shot
replaces it and a new challenge clears it. Misses remain distinct from wrong
mathematical answers. The UI renders the short signal without interpreting
answer labels. The old persistent coaching strings and their active-time
display/unused per-challenge clock fields are removed.

The dispatcher retains input-specific guards, hit validation and aiming misses.
The shared answer route advances resolved button choices and finite practice
immediately; arcade targets retain their pop interval. It updates target colour
history only for an accepted correct target hit. See the
[prepared-answer cleanup](CLEANUP_SORTER_INPUT.md#follow-up-prepared-answer-feedback).

Stop/Esc uses the existing `GalleryPause` route. While paused, the UI displays
the existing totals and offers Resume, optional Answer review, question
selection and Quit. Resume hides totals and continues the same game. This
does not create a completed-run record or new scoring policy. The current mode
is endless; a whole-run countdown is a later capability. These rules supersede
the proposed automatic missed-question retry direction after P018. See
[P019_CONTINUOUS_PLAY.md](P019_CONTINUOUS_PLAY.md).

`GallerySession::view()` assembles shared identity, working, pause state,
completion and accumulated attempt totals once, then fills the mode-specific
fields. `GalleryScene` still owns pause state and `LayeredQuestionSession`
still owns question evidence. Mathematical Submit events and prepared answer
attempts retain their existing counting rules; Undo and rejected input do not
become additional answers. See the [summary cleanup](CLEANUP_SORTER_INPUT.md#follow-up-shared-game-summary).

The [P020 Equation Sorter](P020_EQUATION_SORTER_SPEC.md) is a separate native
`sorter` startup sharing the host and ImGui. `EquationSorterSession` owns
inspection, assignments, reserved inventory slots and transaction Undo.
`EquationSorterContentIO` decodes its independent 100-card JSON and calls the
model's structural validator before native startup. `EquationSorterUi` sends
one semantic activation per ordinary click/fresh key press, applying queued
input after host events and before taking the next rendering snapshot.
Focus loss discards queued activation; Escape has a sorter-owned shortcut route.
Its freely classified cards do not use the judged-question schema or change
gallery behavior. The first pack is generated and independently checked algebra.
[P021](P021_MIXED_MATH_PACK.md) adds a separate prepared mixed pack: 80 retained
algebra cards and five examples each of trig, calculus, linear algebra and
discrete maths. The loader and session do not interpret mathematical expressions.
Bounded test-only interpretations check the displayed expressions; no maths
evaluator enters the runtime. The UI reserves vertical scrollbar
space from the first frame to keep card widths stable when an inventory opens.
Broader subject coverage, richer notation and gallery integration remain future work.

[P022 sorting assistance](P022_SORTER_ASSISTANCE.md) extends that same owner.
Prepared optional `subject` and `hint` metadata enter through the existing
loader and structural validator. A shared subject table maps the five subjects
to A–E; Dump stays manual. `AutoSort` collects only unassigned, classified cards
in home order and calls the existing atomic transaction method. Manual moves,
the current view and reserved inventory slots survive; one Undo reverses the
whole batch. Repeated empty batches do not create history, and stale automatic
actions use the same revision guard as a card commit.

`view()` supplies readiness, next-action guidance and prepared hint text.
`ShowHint`/`CloseHint` change help visibility without assigning or inspecting a
card. The UI presents the projection in a scrollable panel with a fixed Close
control and routes Escape to help before an underlying confirmation or inspection.
It does not classify equations or advance a mathematical solution. After all
cards are assigned, selecting any group opens its inventory in one action.

[P023 prepared solving](P023_PREPARED_SOLVING.md) connects one linked algebra
card to the existing question/gallery owners inside `sorter`.
`EquationSorterSession` owns the selected solving session and return/resume;
`LayeredQuestionSession` owns accepted decisions, the current prepared working,
and separate hint/next-move/applied-step evidence. `GallerySession` forwards
operation buttons and real sphere hits to that same question dispatcher.
Its finite prepared flow advances immediately, while the standalone continuous
gallery retains its original lifecycle. `GalleryScene::FrameTargets` uses the
existing camera-fit kernel to keep the prepared arithmetic targets visible.

The optional sorter `solve_pack` is resolved and validated by the content loader
before native startup. Optional step help and bounded working-state highlights
are frozen question content. The UI presents those spans without parsing maths.
Solving does not mutate groups, home slots or grouping Undo, and opening another
subject's unprepared card does not silently choose the algebra question. Only
the linked algebra example is a solving question in this checkpoint; the other
mixed-pack statements remain sorting content.

[P024 bracket recipes](P024_BRACKET_RECIPES.md) extends the existing authoring
tool to expand bounded `a(x + b) = c` recipes into the P023 question format.
This is the sole producer for the six prepared bracket questions, including
version 2 of the original P023 card. Exact arithmetic, distinct choices and
source/version checks happen before publication. Neither runtime loader nor UI
derives mathematical answers. The native content-copy target includes all
prepared sorter packs and their linked question files.

`EquationSorterSession` now lazily retains one `GallerySession` per opened
content home slot, bounded by the existing 100-card contract. Its one active
slot selects the session; the others remain paused. Opening, returning and
replaying retain the existing dispatchers and grouping Undo. The UI consumes
queued game input when switching context and resets the displayed challenge so
two questions with equal local challenge numbers cannot share stale controls.
There is no second progression, help or answer-history owner.

[P025](P025_FIXED_SOLVING_WORKSPACE.md) replaces the step-dependent solve UI
placement with one presentation layout derived only from window dimensions.
An ImGui root with fixed child panels keeps keyboard navigation continuous;
operation buttons, bound sphere labels and completion controls share the scene
viewport. The original equation and current working remain separately visible.
History reads the question owner's existing review projection.

`EquationSorterSession::NextSolve` requires completion and a current revision,
then uses the same preparation route as OpenSolve for the next prepared home
slot. It preserves prior per-card sessions and refuses to wrap at the end.
No chapter-selection queue is introduced. UI context changes discard queued game
input; a shot's held press is consumed until release so it cannot activate a
new operation appearing in the same space. Existing judging and scene owners
remain unchanged.

[P026](P026_COMPACT_SYMBOL_CHOICES.md) keeps that ownership while centring a
compact problem/working/choice group and bounding the activity height. The
existing recipe generator emits operation symbols and a shared both-sides
instruction into ordinary option labels and prompts. No runtime schema or
English-to-symbol UI conversion is added. Generated questions and decks use
content version 4 with generator version 2; the existing font supports all
delivered symbols. Accepted IDs, mathematical working and help are unchanged.

[P027](P027_RESPONSIVE_SOLVING_WORKSPACE.md) supersedes P026's fixed activity
cap. The same UI layout computes a scale and fitted workspace from window
dimensions; problem text, working and choices grow together. The activity
rectangle remains shared with the existing scene viewport, and resizing uses
the existing gallery framing route. No content or runtime ownership changes.

[P028](P028_STUDY_SELECTION.md) adds a table of contents using optional authored
`study.chapter`, `study.type` and `study.form` fields on sorter records. The
loader and shared sorter validator check those titles; no equation parser or
UI classifier infers them. Only linked, validated solving questions enter the
session's immutable catalogue.

`EquationSorterSession` owns subject/chapter/type inclusion, all/random/specific
selection, stable random previews and a frozen study queue. UI chapter browsing
is presentation state. Start, Resume and Next share existing solving preparation;
selected-set Next traverses the frozen queue, while individual-card Next retains
home order. Draft edits cannot change a running set.

Starting a new set prepares copies of all selected sessions before replacing
retained owners. The existing gallery replay route forwards an explicit
`archiveUnfinished` option to the question owner's `RestartQuestion` command.
Only this new-set use opts in; ordinary replay still requires completion.
The original attempt is archived with its actual completion state and evidence.
No second history, judging or help owner is added.

[P029](P029_COORDINATE_BOARD.md) adds prepared straight-line graphs to that
catalogue. Optional `LineGraph` parameters and each working state's `GraphStage`
are validated by `LayeredQuestionSession`; its read-only `coordinateGraph(x)`
projection owns intercept, run/rise geometry, clipped line endpoints and probe
coordinates. Equation strings are never parsed during play. `GraphChoice`
steps enter the existing submission route, generalized from `ChooseOperation`
to `ChooseAnswer`; there is no parallel graph judge or attempt store.

`EquationSorterUi` draws the coordinate board with native lines, circles,
triangles and text. It owns screen fitting, short reveal animations, replaying
motion, and the transient requested probe position. Probe movement reads the
question projection without modifying evidence. The graph fills the same fixed
activity region through solving, help, completion and explicit Next. The native
sorter omits the sphere render snapshot for graph questions. Existing sphere
questions retain their viewport, camera and input route. No image assets or
screenshots are involved.

[P030](P030_SIMULTANEOUS_EQUATIONS.md) extends `LineGraph` with an optional second
line and a four-decision system reveal chain. The same question owner validates
both lines, clips their endpoints, and projects two points at one shared x.
Its single system-solution helper classifies intersecting, parallel and
coincident lines and calculates any unique intersection; validation also uses
that helper to reject a crossing outside the axes. The projection makes the
guide available after both lines are revealed, exposes the relation after the
solution-count decision, and marks a unique intersection only after the final
decision. The guide never submits an answer or changes evidence.

The existing UI renderer draws a solid teal first line, a dashed violet second
line, one vertical guide and two coordinate readouts. A completed unique point
or coincident solution line is mint. The infinity choice uses native Bezier
strokes independent of font coverage. Layout reserves space for the paired
original equations without moving the problem or board between steps. Existing
selection, `ChooseAnswer`, help, replay and explicit Next routes remain the
only routes for those actions. The recipe publisher computes exact rational
answers and expands four systems into the existing default study pack; no
runtime equation parser or new production code file is added.

[P031](P031_LINKED_VALUES.md) adds an optional three-row value table to the
existing `CoordinateGraphView`. `LayeredQuestionSession::coordinateGraph(x)`
uses its existing point evaluator to sample the shared visible x range at its
ends and midpoint. The table is absent until the existing probe-availability
gate opens. Its rows are independent of the current guide input; no question
content, answer rule or reveal chain changes.

The existing graph renderer reserves a compact table beside the plot and
numerical substitution strips above it. Three sample buttons, plot dragging
and the slider all change the same transient `graphProbeX`; their inputs are
handled before the shared projection is refreshed and drawn. The amber live
row, coloured graph points and numerical substitutions therefore use one
projection in the same frame. The UI formats provided parameters and values,
with two-decimal approximations, without evaluating the equations itself.
These strips replace the prior floating probe-coordinate labels. The original
problem, graded working, help, attempts and explicit Next keep their owners.

[P032](P032_MATHEMATICAL_MOVES.md) adds the `MathMoves` interaction to the same
question owner. Bracket content explicitly opts in with
`working_model: "linear_moves"`; the sorter selects this interaction through
its existing GallerySession. Its `MathematicalMove` command forwards the frozen
question/version/run/revision and the player's operation, number and equation.
The pure `LinearEquation` kernel parses bounded affine expressions with exact
rational arithmetic and checks the selected transformation separately on each
side. A matching solution alone cannot pass an unrelated operation. The UI
never parses or checks mathematics.

`LayeredQuestionRunRecord::math` owns append-only working nodes and events. A
correct submit appends a node with its parent; a wrong submit appends an event
without changing working. Undo records a return to the parent and keeps both
branches. Completion is exact substitution of the isolated numerical answer
into the original equation; the existing completion flag gates Next. Review,
summary, restart and per-question retention read this same evidence. Prepared
commands and answer targets are unavailable in this interaction; the prepared
interaction remains for its existing arcade and other authored consumers.

`EquationSorterUi` presents a fixed original/active/input area and a scrollable
blueprint using native text and strokes. Inspection is read-only. Input is
queued before the next view, guarded against focus loss and stale identities.
The native app omits the scene for mathematical-move questions. There is no
second progression or attempt store. The six generated bracket cards advance
to version 5; graph content and graph judging retain their existing format.
The P032 packet specifies parser, numeric and history limits. Session retention
continues to be in memory; durable persistence is not implemented.

[P033](P033_VISUAL_MATH_MOVES.md) replaces the equation/number fields and Check
button with concrete operation buttons and four result tiles. The shared
question owner's `mathMoveChoices()` projects the current equation through
`availableMathMoves()`. Generation and submission call the same
transformation rules; only submission records an attempt or advances working.
The bounded palette includes expansion, coefficient division, reciprocal
multiplication, and signed constant moves. Result choices contain one valid
transformation and distinct mistakes, including missed distribution and
one-sided operations. Their order is deterministic per equation and operation.

The UI caches this read-only projection for the current question/run/revision
and retains only a selected move index. Clicking a result forwards its operation,
operand and equation through the existing `MathematicalMove` route. A successful
move or Undo clears selection; an incorrect result preserves the offered order.
The displaced text buffers and public operation-menu API are removed. Content,
correctness, attempt history, graph controls and explicit Next retain their owners.

[P034](P034_MATRIX_ROW_MOVES.md) extends that same interaction to a two-row,
three-column augmented matrix. `MathWorkingModel` selects scalar linear working
or row reduction from declared content. `prepareMathWorking()` owns initial
validation and construction. Each existing working node contains a typed
`MathWorkingValue`; `LayeredQuestionSession` visits that value to request
choices, check a move and verify completion. There is one attempt/history
route, with no matrix-specific session or progression state.

The existing pure mathematical kernel shares exact rational arithmetic between
both forms. Matrix generation and judging use the same row transformation;
all six cells must match. Identity-form answers are substituted into both
original rows before completion. The UI uses the same operation/result/history
panels, with extra height for two-line matrices. The new authored card is linked
through the existing content publisher and study catalogue. No parent project,
native renderer, build graph or additional production file is involved.

[P035](P035_ROW_REFERENCES.md) adds an optional shared reference library to the
existing question-pack loader. Cards resolve `concept_ids` into immutable
`MathReference` copies, including versions, before the existing constructor
validation. A single declared row-operation table supplies file keys, operation
families and move-palette concept IDs. It replaces the former private
declaration; transformation, example projection and decoding use that table.

`LayeredQuestionSession::mathReference()` exposes only linked reference content.
`mathReferenceExample()` uses the existing exact row transformation to project
the authored example's before/after cells and per-column arithmetic. Neither
question-answer choices nor the player's working are used as example inputs.
The UI owns the open reference ID and a bounded column cursor, just as it owns
working inspection. These presentation actions create no attempt, completion,
hint or answer-reveal records. Correct result submissions still use the sole
`MathematicalMove` dispatcher; no reference-specific solver or progression
route is added.

The reference replaces inspection in the right support pane at wide sizes,
or occupies the existing lower support area at narrow sizes. The problem,
current working and answer tiles keep their rectangles. Its content scrolls
inside fixed Close/previous/next controls. Example stepping follows the game's
pause state, and Escape closes a reference before returning to Contents.
Question/run changes clear the reference; same-question return/resume preserves
it. CMake deploys the shared library with both native content consumers.

[P036](P036_SAVED_PRACTICE.md) adds device-local practice persistence to the
sorter. Finite `GallerySession` instances opt into the question owner's accepted
command journal. `LayeredQuestionSession::dispatch()` records only accepted
changes before publishing their evidence; unchanged help requests and rejected
or stale input do not enter the journal. Other gameplay modes do not allocate
this journal. It covers current and archived runs, including mathematical Undo.

`EquationSorterSession::studyProgress()` publishes stable card IDs, selected
titles, the exact random/specific draft, the frozen queue and its position,
plus each started question's ID/version and command journal.
`restoreStudyProgress()` stages a complete replacement and constructs every
saved finite game through the existing question checker. It publishes only if
all titles, IDs, versions, selections and journals validate. Startup returns to
contents with each game paused; Resume/Next keep their existing action routes.
The gallery regenerates scene bindings from checked collection facts, including
partly collected answer sets. It does not restore a physics clock or aim misses.

`StudyProgressFile` is the filesystem/JSON adapter. Its versioned format contains
inputs and a content stamp; no saved result, correctness flag or completion flag
is trusted. Reopening replays inputs through the same domain dispatch and exact
math kernel. The content stamp catches edited judging material even without a
version bump. Restoring an incompatible file leaves the live session and original
file untouched, pauses saving for that launch, and reports the problem. Writes
use a complete temporary file in the same directory followed by replacement.
Before writing, the adapter checks that the destination still matches its last
read version. The revision check excludes idle frame updates, and identical
snapshots do not rewrite the file.

The native sorter loads before creating its host and saves after applying frame
commands. It chooses SDL's user-data folder unless the user supplies a path.
Checks/scripts/bounded runs have no implicit personal save. The UI presents a
green save status, a blue Resume button and amber failure details; it owns no
persistence or recovery policy. Transient graph probes, reference-page cursors,
camera state and grouping history remain outside this practice capability.

[P037](P037_GROWING_MATRIX_PRACTICE.md) extends the catalogue with twelve generated
matrix problems while preserving P036 saves. The existing publisher composes
the new chapter with the default 100-card catalogue; its standalone catalogue
uses the same cards and shared row references. The live mathematical palette,
question judge and UI retain their existing owners.

Practice format 2 records the pool at the last selection edit and the exact
selected order. `EquationSorterSession` now owns an ordered selected-home vector;
the UI's selection mask is its read-only projection. Restoration maps saved
card IDs to current homes and validates All/Random counts against the saved pool.
The current title pool can grow while the saved draft and active queue remain
frozen. Deliberate selection edits refresh the pool; Start set freezes the chosen
order, and Resume/Next continue to use the existing queue.

`StudyProgressFile` checks teaching stamps by stable card identity for the saved
draft, queue and retained runs. Catalogue order and unrelated additions do not
affect compatibility. Version 1's full stamp supplies the old selection pool;
version 2 saves it explicitly so repeated reloads remain stable after growth.
No saved command is reinterpreted against changed teaching content. An
incompatible dependency identifies its card, preserves the file and live state,
and pauses saving for that launch. Existing atomic writes and external-change
checks remain the filesystem adapter's responsibility.

[P038](P038_CONTENTS_PROGRESS.md) adds current-attempt progress to Contents.
`LayeredQuestionSession::progress()` derives NotStarted/InProgress/Completed from
current completion, move events and prepared answer/help evidence. Navigation,
reference browsing and archived runs do not start a fresh attempt. Undo and
Replay therefore change the projection without deleting earlier evidence.
`EquationSorterSession::view()` projects these states per question and aggregates
completed/total across every type in each chapter, independently of selection.
The UI draws small circles/ticks and counts; it stores no progress policy.
Save restoration rebuilds the same evidence through the existing checker, so
these marks require no new persisted fields, revisions or migration.

## Modes and navigation

Use `PathsScreen { Title, Playing, Stats }` and retain the existing
`FirstMoveMode { GuidedQuestion, QuickHunt }` as the mode identity in P001.
The existing shared app dispatcher owns the screen and selected/resumable mode
in `FirstMoveUiState`, while each pure mode model continues to own its run.
Do not introduce a second active-mode variable in a new shell class.
Mode descriptors form a declarative table
with ID, title, description, and bindings. Title cards and script mode names
read that table; there is no growing chain of business-condition branches.

Title → OpenMode validates the descriptor and opens/resumes that mode. Playing
→ ReturnToTitle preserves both runs, selections, help state, and archives.
Title → Stats reads both sessions; Stats → Back returns to Title. A new run is
an explicit in-mode action. Enter on Title opens the focused mode, never starts
a new run behind the learner's back. F1 returns to Title from a mode. Escape
continues to do the existing local back/close action within each mode.

An open Hunt review may be paused by returning to Title. Its modal state must
remain intact. Until that review is closed or released, opening Guided is
rejected with `Finish or close the Quick Hunt explanation first.` Opening Hunt
resumes the review. This preserves the current model boundary while allowing
the title screen to remain reachable.

All pointer, keyboard, and scripted navigation uses the carried app semantic
dispatcher, extended with title/stats actions in the copied FirstMove UI
files. It either changes Paths navigation or forwards one model command.
Do not wrap it in a second dispatcher that independently checks mode policy. Ignore
state-dependent input queued for a previous screen/context. Neither drawing a
title card nor generating a report may advance a game.

## Truth, experiment rules, and statistics

An experiment changes the experience or interpretation of work. It must not
rewrite what the learner did. The authoritative facts include question and
content version, run identity, checked choices in order, first response,
retries, reveals, completion, and prior exposure.

P002's run configuration is a value copied at run start:

```text
RunConfig {
  modeId, modeVersion,
  contentPackId, contentPackVersion,
  objectiveId, objectiveVersion,
  scoreRuleId, scoreRuleVersion,
  parameters
}
```

Changing an experiment setting affects the next run. Resuming an unfinished
run retains its original configuration. A comparison score may be computed
from a finished run under another scoring rule, but it is labelled a
comparison and does not overwrite that run's original score/rule identity.

Start with three precisely named score rules in P002's design: `practice_none`
(no points), `hunt_bank_v1` (the existing 100/240/420/520/620/720 bank schedule),
and `guided_completion_v1` (one point per completed layer, equally whether
answered or shown). The last measures progress through practice, not
correctness. A different accuracy/retry incentive gets a new explicit rule.

Keep objective and score separate. `finish_question` completes after the final
Continue, regardless of assistance. `finish_hunt_pack` completes when each row
is cleared or reviewed/released. Neither equates to a threshold on points.

Statistics expose their denominator and scope. Guided counts distinguish first
try, after retry, and shown answers. Hunt counts distinguish checked rows,
initially correct rows, and explained/released rows. Never combine those two
units into a single accuracy percentage. Time measures and durable profiles
are future capabilities; do not invent a timer or persistence schema in P001.

## Content progression

P010 loads the five gallery examples from `content/cards/` through the pack at
`content/packs/gallery_foundation.json`. `QuestionContentIO` owns file reading,
JSON decoding, accepted-option-ID conversion and question-ID/version deck
resolution. It produces the existing `LayeredQuestionContent`, calls shared
structural validation, and reports errors with source paths and JSON pointers.
The [schema contract](QUESTION_CONTENT_FORMAT.md) versions the file shape
separately from each question's content.

P013 makes the question menu the default `gallery` startup.
`GalleryMenu` owns selection, navigation and the lifetime of started gallery
sessions. It offers three named bundled packs; available practice types come
from the validated pack decks. Its `launch()` is the shared preparation boundary
for the Play action and direct CLI gameplay startup. The former construction
block in `gallery_main.cpp` is removed. `paths_gallery_menu` depends on the pure
gallery model and content loader and can be tested without SDL/Vulkan.

Selecting a different set does not modify an existing game. Returning to the
menu pauses it; Play resumes the session for that pack/practice type or prepares
a new one. The last practice type is remembered per pack. Started sessions and
the workshop scene live until process exit; no saved profiles are introduced.
Load failures leave the menu usable and preserve existing sessions. Direct CLI
content errors still stop before graphics startup. An explicitly supplied pack
outside the bundled paths gets a Custom questions entry for later resume.

`gallery_main.cpp` draws the menu and forwards pointer, keyboard and script
actions. Navigation changes are applied between native frames so the host's
scene pointer remains valid and matches that frame's answer board. Gameplay
input still uses the existing GallerySession dispatch route. See
[P013_QUESTION_MENU.md](P013_QUESTION_MENU.md) for the controls and evidence.

P014 adds pending movement setup to `GalleryMenu`. `SelectMotion` and `SetPace`
use the same dispatcher from widgets and native scripts. `selectedConfig()`
returns the selected session's frozen config when it exists, otherwise the
next-game draft with the selected practice type. Editing a started selection
is rejected by the dispatcher as well as disabled in the UI. Visiting an old
game does not overwrite the draft for an unstarted set/type.

`validateGalleryConfig()` in the existing GallerySession module serves both
menu edits and game construction. It checks the practice type and gameplay
preset requirement, then delegates route validity to TargetMotion’s `prepareRoute()`
using the same compact route definition used to create targets. Numeric limits
remain in TargetMotion; custom waypoints remain in Workshop. The UI's slider
range is an editing convenience and does not narrow the existing CLI contract
of finite positive speeds through 100. Question content, judging, assignments,
progression, renderer and movement calculations are unchanged. See
[P014_MOVEMENT_SETUP.md](P014_MOVEMENT_SETUP.md).

P016 adds an explicit replacement draft to `GalleryMenu`. `NewGame` copies
the selected session's frozen config; `SelectMotion` and `SetPace` edit only
that draft. Selection/navigation and ordinary launch are rejected until
`CancelNewGame` or `StartNewGame`. Cancelling discards the draft and restores
the saved setup without changing the run or the ordinary next-game defaults.

The private `start()` boundary prepares initial games and replacements through
the same loader and GallerySession constructor. Replacement reloads the current
pack, resolves the selected deck and constructs the fresh session before
releasing the old one. Only that pack/practice slot is replaced. On success,
the draft becomes the next-game defaults and is cleared; on failure, both
the old paused session and editable draft remain. Run history is not archived
by the menu: the explicit Start new game control replaces it, as stated in
the UI. New-game input is applied between native frames, preserving the scene
pointer lifetime. See [P016_NEW_GAME.md](P016_NEW_GAME.md).

P017 adds a read-only answer-review projection to `LayeredQuestionSession`.
`review(runIndex)` selects the current run at index zero or a retained completed
run, then resolves its question ID/version against the session's frozen catalog.
It exposes only reached steps, recorded attempts in order, existing outcomes,
collection counts and the working shown before that step. Text views borrow
the catalog for the session's lifetime. It does not rejudge answers, expose
unsubmitted option lists or reveal later working states.

`GalleryMenu` owns the Review screen, selected run and expanded row through
`OpenReview`, `CloseReview`, `SelectReviewRun` and `ToggleReviewStep`. Opening
pauses the existing game and selects its current question. Closing preserves
that pause. Other navigation and launch actions cannot bypass the open review.
The startup draws the projection and routes widgets/scripts through those
actions between frames. It retains the scene for rendering while disabling
gameplay input and keeping motion/pop time frozen. The question session remains
the sole owner of attempts, progression and completed history. New game clears
history by replacing that one session through P016's existing route. See
[P017_ANSWER_REVIEW.md](P017_ANSWER_REVIEW.md).

P018 extends that projection with an explanation view. The question owner
returns the frozen step's existing `explanation` only when
`layeredQuestionStepResolved(record)` is true. The shared predicate already
covers player resolution and a Guided answer shown; partial AllAccepted sets
remain unresolved. No second completion rule is introduced. Text stays empty
for unfinished steps or empty authored explanations. The UI renders a nonempty
value below the attempts in an expanded row; it never looks up answer keys or
checks collection masks. The native report exposes `explanation_available`
from the same projection. Catalog, loader and schema remain unchanged. See
[P018_REVIEW_EXPLANATIONS.md](P018_REVIEW_EXPLANATIONS.md).

`GallerySession(config, catalog, resolvedDeck)` freezes the selected deck;
`LayeredQuestionSession` validates and freezes the catalog. Only the question
session judges answers, records attempts and advances prepared working states.
The runtime owners have no filesystem or JSON dependency. The former compiled
gallery catalog and fixed mode indices are removed. The original Guided
starter question remains compiled in its existing question owner.

Content files contain prepared steps; loading never derives a solution or
interprets display strings. General mathematical verification and automatic
content preparation remain separate capabilities.

P011 provides `content/cards/source_002_quadratic_three_points.json` and the
single-card `content/packs/source_002.json` deck for `equation_chain`. It maps
the retained 002 authoring artifact into 13 prepared decisions, with fixed
numeric identities and before/after state references. The original file and
source snapshot remain unchanged. See [the adaptation record](P011_CARD_002_ADAPTATION.md)
for the source hashes, identity map, answer-exposure review and native proof.
No runtime C++ or schema changes are needed for this content checkpoint.

P012 adds `content/cards/source_013_closest_point_line.json` and the separate
`content/packs/source_013.json` deck using the same startup. Its 14 prepared
decisions retain the supplied target formula, Euclidean conditions, residual
argument and uniqueness justification. The question owner still judges all
choices and keeps the attempts. Completion records guided derivation practice;
it does not establish independent proof writing. See
[the 013 adaptation record](P012_CARD_013_ADAPTATION.md) for its explicit ID
map, preserved working, mathematical argument and complete native run.

`content/authoring/002_guided.json` has 13 authored layers and
`content/authoring/013_guided.json` has 14. Their source pages, hashes, choices,
misconceptions, working lines, explanations, and mathematical checks are
already present. See the content architecture for exact future catalog and
evidence IDs. In that document, old FM003 means the source-card slice, now
P003; its source paths are relative to this standalone project when implemented.

P001 ships the carried starter question and Hunt pack. P003 adds the completed
002 adaptation to a multi-card Guided grid, with the 013 derivation following
there using the same variable-length catalog. Both cards already play as
separate gallery packs. Guided grid navigation remains its own capability.

## Milestones and acceptance

The migration's proof is a successful build and offscreen run of a copy of
`paths/` located outside the parent checkout. A build from the subdirectory
that silently reaches back into the parent is not isolated. Record build,
model/input tests, offscreen capture, interactive status, and user acceptance
separately. The user has requested a title screen to implement, not a visible
window to launch during this workstream.

The planner works in docs/content; Sol builds the scoped source changes and
pushes a completion brief. No repeated worker observation is needed. Each
brief advances the active packet in `WORKSTREAMS.md` and identifies the next
capability rather than growing an unbounded refactor.

## Native mathematical objects (P039)

`MathObjects` owns the mathematical parameters, derived primitive placements,
readouts, graph adjacency and route, and one challenge per object. Its semantic
dispatch validates changes before rebuilding a bounded, immutable snapshot.
It has no SDL, Vulkan, ImGui, content loading, or study-save dependency.

`MathObjectScene` tessellates the snapshot into the existing `SceneFrame` format:
position / colour / UV vertices and 16-bit indices, within the existing fixed
GPU capacities. Unit geometry is cached and frame buffers are reserved. Camera
motion uses the existing viewport-navigation kernel; both scene producers use
`publishSceneCamera` for the same Vulkan projection convention. The native host
continues to own buffers, shaders, synchronization and command recording.

`app/math_lab_main.cpp` is the live UI/CLI consumer. It forwards parameter, route
and Check actions to the model and camera input to the scene adapter. It does
not calculate mathematical answers, alter study attempts or write personal
saves. This object library and exploration startup are a separate capability
from scored question integration and persistent lessons.

## Linked mathematical layers (P040)

`MathObjects::dispatch` also owns level selection, matrix presets, integral-bound
reversal, contour point moves and gradient-descent steps. Parameter availability
comes from the model; lowering a level restores hidden higher-level parameters
to defaults. The original shear/scale controls are entries in the same 3x3 A
used by the matrix editor, composition, projection and SVD.

Snapshots contain bounded plot samples, matrix views, one surface patch and
contour segments. Function values, derivatives, antiderivatives, surface
partials and Hessian entries are analytic. Contours interpolate the sampled
surface; Jacobi diagonalisation of A-transpose-A supplies the SVD. Numerical
formulas and challenge tolerances live in the mathematical owner. UI code
maps the supplied coordinates to screen positions and forwards semantic input.
Diagram edits are applied before drawing the following frame so its geometry,
readouts and diagrams share one model revision.

`MathObjectScene` appends the optional patch to the same reserved SceneFrame
buffers and checks the existing GPU capacities. It never creates GPU resources.
Changing a subject or level reframes its camera. SVD stages carry labelled,
coloured basis directions so rotations of the sphere remain inspectable.

`math_lab --validate` applies the same CLI actions, publishes CPU geometry and
prints measurements before constructing `NativeVulkanHost`. It rejects capture
requests and creates no native host, window or images. The numerical tests
build with `PATHS_BUILD_NATIVE=OFF`. P040 extends the existing production files;
no parent source or third-party implementation was imported.


## Symmetry, harmonics and motion (P041)

The same `MathObjects` owner now handles exact cube rotations and bounded turn
history, harmonic synthesis, oscillator initial data and preview time. All new
input enters its exhaustive semantic dispatcher. Symmetry enumerates the 24
proper rotations using signed-permutation matrices; the snapshot publishes the
vertex permutation and orbit. Plot flags provide spectrum stems and equal-axis
Lissajous presentation without adding another mathematical owner.

Harmonic coefficients and Parseval RMS are analytic; a bounded midpoint sum
independently estimates a selected coefficient. Motion uses deterministic RK4
from initial data to the selected time and a fixed 129-point trace. Work and
loss use the same integration stages. The linear spring also exposes the
causal impulse response of 1/(s^2+c*s+k) and a Simpson convolution, with their
agreement against the ODE in the snapshot. The nonlinear pendulum does not use
that linear response decomposition. Preview time does not enter scored-game,
question-attempt or study-save state.

The app forwards time ticks before presenting any view. Play stops at the
bounded horizon; scrubbing and checking pause it. Diagram edits still apply
before the following frame. The existing scene adapter and native host are
unchanged. The renderer retains its original buffer capacities; P041 tests and
CLI verification require no window, image or native host construction.


## Modular numbers, Gaussian integers and vector fields (P042)

`MathObjects` remains the sole owner of parameters, semantic actions, exact
number operations, path evaluation and challenges. Modular walks retain a
bounded step count and a visited-residue mask; changing their setup clears the
evidence. Integer kernels handle positive remainders, GCD, inverses and general
CRT consistency. Gaussian kernels distinguish nearest-integer Euclidean
division from the floor-based half-open-cell representatives used for ideals
and quotient rings. Undefined division by zero has an explicit readout.

Vector fields and paths evaluate in double precision. Analytic positions and
velocities feed Simpson line integrals and independent exact reference values.
Path reversal retains the probe position and reverses its tangent. The shared
`playbackParameter()` supplies the time parameter to both model actions and UI.
All preview state stays outside scored attempts and saved study practice.

Snapshots add one bounded 32-row, four-value-column table; label capacity rises
from 16 to 32 for two residue drums. UI renders the supplied table and forwards
step/reverse actions. Tables and plots share tabs. Primitive, plot and GPU
capacities are unchanged; the existing scene adapter and renderer require no
changes. Tests use CPU geometry and `--validate`, with no native host, window,
rendering or images. See the P042 packet for mathematical conventions and bounds.


## Flux shells, tensors and probability (P043)

The existing MathObjects owner gains three families and twelve lessons. Flux
uses weighted midpoint quadrature on spheres, box faces and disk annuli, with
separate analytic references. Stokes uses Simpson boundary integration and
independent curl flux. Tensor snapshots derive rank-two/rank-three components,
Euclidean contractions and orthonormal coordinate changes from three vectors.
Markov snapshots use row-stochastic matrices and bounded distribution traces;
sampled walks keep a separate 65-state history and explicit xorshift32 seed.
Changing walk setup clears that evidence before rebuilding the snapshot.

The existing surface patch handles sphere and disk shells. Box faces and tensor
cells use existing primitives. No renderer or GPU-capacity changes are needed.
Probability spheres below 1e-9 are replaced by fixed grey markers to avoid
singular transforms; numeric probabilities remain unchanged. The UI adds two
walk actions, table/matrix/plot tabs and a compact selector below 720 pixels in
height. Distribution steps use the existing parameter action; boundary playback
uses the existing semantic time owner. All verification remains text-only.


## Probability and statistics objects (P044)

The existing MathObjects owner adds binomial, Bayesian and covariance families.
Binomial mass uses bounded Bernoulli convolution; a separate seeded trial path
records at most twelve outcomes through semantic actions. Normal approximation
uses the Gaussian CDF and continuity correction, with zero variance explicitly
excluded. Bayesian updates normalize two joint masses only for positive evidence
probability; repeated evidence uses conditional independence and retains both
posterior components directly to avoid cancellation of small probabilities.

The covariance object derives population moments from eight actual points.
The existing SVD kernel supplies ordered principal directions and variances.
Whitening centers the cloud, rotates to principal coordinates and rescales each
axis; the inverse is unavailable for singular covariance. Snapshot matrices and
coordinate tables share the same points used by geometry. The fixed primitive,
plot, table, label and GPU capacities are unchanged. Tiny probability geometry
uses documented baseline/omission thresholds while readouts retain numeric mass.
The common xorshift32 helper preserves previous probability-walk replay. No
renderer, source-note, scored-attempt or study-save behavior changed.


## Spherical harmonics, quadratic forms and roots (P045)

MathObjects owns three additional families and twelve lessons. Spherical modes
use a bounded associated-Legendre recurrence through degree four, with explicit
real-basis and phase conventions. A separate 8-by-16 spherical quadrature checks
orthogonality and energy; analytic spectral decay supplies heat flow. Finite
differences at a fixed interior direction check Laplace-Beltrami eigenvalues.

Quadratic forms use Q D Q^T and preserve their exact chosen principal values,
including zero and negative values. A section height map changes to a unit
Rayleigh sphere at the fourth layer. Marching-squares zero contours share the
existing bounded contour view. MoveSurfacePoint now resolves its two parameters
through an exhaustive owner switch, validating both coordinates before either
write. Table-bearing snapshots can also expose the contour tab.

Roots use exact integer exponent arithmetic, bounded monic polynomial division
and the unit group modulo n (3-12). Invalid automorphism candidates remain valid
power-map examples with an explicit unavailable automorphism state. Snapshot
geometry is generated from the same maps as the tables. No host, scene-adapter,
GPU-capacity, scored-attempt, source-note or study-save change is required.
