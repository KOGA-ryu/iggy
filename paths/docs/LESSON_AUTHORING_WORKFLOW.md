# Reusable textbook lesson workflow

The production unit is one complete lesson in the accepted **Read + figure /
Reading only / Figure only / Exercise** shell. One builder finishes that unit
before starting another. This workflow is for Paths at
`/Users/kogaryu/iggy3d/paths`, not the parent Creative project or Sinc.

The [current work allocation](../AGENTS.md#current-work-allocation) puts the full
textbook, teaching content, equation solving, formatting, content pipeline and
figure integration under the textbook worker. The separate worker builds 3D
assets/models. Existing authored lessons and researched formatting are the
baseline this textbook worker maintains and extends.

Use the [task template](templates/LESSON_TASK.md) for assignments.
[P051](P051_DETERMINANT_VOLUME.md) implements the determinant pilot.
[Batch 01](lesson_tasks/BATCH_01.md) supplies three ready implementation briefs
and a start instruction. A single lesson is the default unit; an explicit batch
request authorizes sequential continuation through its bounded queue.

## Non-negotiable working constraints

- Do not take, generate, render, open, inspect or return images. Do not use
  screenshots, image tools, browser/native previews, offscreen captures, or
  font-rasterization probes. The user performs visual checks by launching the
  finished executable. Reading source text and inspecting CPU geometry is allowed.
- Never run `paths_native_math_tests`. Do not run an unfiltered CTest suite or
  assume a test is image-free because it is named headless. Inspect newly selected
  tests and CLI paths before running them. `--validate` must exit before native
  host creation, typesetter initialization and bookmark I/O.
- Work directly, with no subagents or new tasks unless the user authorizes them.
  Leave changes uncommitted. Respect the actual sandbox and approval mechanism;
  do not bypass it. Complete authorized implementation without extra confirmation.
- Source problem pages and pinned notes are read-only. Preserve unrelated dirty
  files, the Library, regular-question practice, and shelved motion work.
- A mathematical owner computes and checks; a figure projects its snapshot; the
  UI dispatches actions and presents results. Reading and camera changes do not
  create attempts. Never silently reinterpret A's final column as b.

## What repeats, and what must be decided for each lesson

| Work | Builder repeats | Assignment must supply |
| --- | --- | --- |
| Orient | Read the named example and current ownership; record pre-edit hashes | Exact topic, reference files and writable files |
| Author reading | Introduction, definitions, worked examples, figure caption, practice, summary | Learning outcome, prerequisites, conventions and source provenance |
| Choose cases | Ordinary case, contrast, degenerate case | Exact inputs, expected results and what each case teaches |
| Connect model | Bind the existing owner and consume its snapshot | Correct representation, owner, dimensions and numerical limits |
| Connect controls | Labels, ranges, reset behavior and semantic dispatch | What each control changes and what it must preserve |
| Add practice | Prompt, response/check, reveal behavior and retained history | Existing judge or explicit new owner; pre-answer disclosure rules |
| Register section | Stable IDs, contents, index, navigation, CLI and bookmarks | Intended position and compatibility requirements |
| Verify | Compile, run named pure checks and text CLI cases, inspect diff | Independent mathematical oracle and exact success criteria |
| Deliver | Install/build in the normal location and give a launch command | Human visual actions and expected visible behavior |

The reusable shell, camera, native host and typesetter should normally receive
no changes. Reuse content structure; do not clone entire UI implementations.

## Match the assignment to the work

**Existing binding:** new prose, presets, captions or exercises within an
established mathematical representation. Terra Medium is a reasonable starting
point after a pilot has established the pattern; High is also suitable for the
first multi-lesson batch.

**New binding to an existing model:** for example, attach the existing linear
cube model to the textbook. Specify the adapter and routing boundaries first;
use Terra High for the first implementation. P051 established the first object
binding; Batch 01 reuses it.

**New numerical method or representation:** settle mathematical definitions,
conditioning, exactness and independent checks before assigning production.
A stronger reasoning pass can produce that specification. Terra can then build
against it. Do not use a generic template as permission to invent missing math.

These are workflow recommendations, not measured Terra performance claims.
OpenAI describes Terra as an everyday-work model with strong reasoning and tool
use, and recommends adjusting reasoning effort to the task. Start with one
pilot and judge correctness, repair work and elapsed time before lowering effort.
[Official model guidance](https://learn.chatgpt.com/docs/models#choosing-sol-terra-and-luna)

## Assignment specification

Keep a filled task compact and decisive. It must identify:

1. One observable learning outcome and explicit scope.
2. The closest accepted lesson and exact reusable model/figure code.
3. What every object, axis, colour, equation and number means.
4. Bounded controls, ordinary and degenerate cases, and expected results.
5. Which practice facts are visible before and after a response or Reveal.
6. File ownership, shared integration edits, and a mathematical check independent
   of the code being checked.
7. Build/test commands, the launch command, and the user's short visual test.

There should be no unresolved question that changes the mathematics when the
builder begins. The builder may resolve routine wording, C++ organization and
local layout choices within the accepted shell without asking again.

## Execution sequence

### 1. Inspect only the relevant context

Read `AGENTS.md`, current architecture/workstream entries, this workflow, the
filled task, and its named reference implementation. Use `rg` to locate symbols.
Inspect Git status and capture the starting contents/hashes of files to edit.
Do not reread the whole source corpus, entire repository, or old task transcripts.
Do not initialize Git or discard existing uncommitted work for isolation.

If the task's file or API description has drifted, adapt to the current code
while preserving its intent. Identify an actual incompatible ownership or
mathematical change before requesting clarification; do useful independent work
while that question is pending.

### 2. Establish the mathematical owner and examples

Use an existing owner wherever its meaning fits. Check the task's fixed examples
before UI work. For new behavior, use an independent certificate: exact values,
known vectors, integer minors, an analytic identity, or a separately justified
calculation. Calling the production helper twice is not independent evidence.

Specify zero, singular, negative, invalid and boundary behavior where relevant.
Do not copy the systems lesson's tolerance or cancellation rule into another
algorithm without a mathematical reason.

### 3. Build reading, model controls and practice together

Use typed `BookBlock` passages and stable IDs. Write the caption and symbol
conventions alongside the figure binding. All views consume the same current
mathematical state for that example. Independent practice may have its own
clearly identified example state, as P050 does.

Send mutations through semantic actions. Make reset versus undo behavior explicit.
Expose answers, reduced forms and solution geometry only at the disclosure stage
specified by the task. Authored teaching figures may intentionally show solutions;
fresh prediction exercises may not. Reveal does not count as a submitted answer,
and restart must not erase known assistance from retained attempt history.

### 4. Integrate the complete section

Add its contents/index links, exercise route and CLI selection. Keep stable
section/block IDs even if printed numbers change. Check every shared call site
that assumes a matrix board, figure provider or fixed section count.

Shared integration points after P051:

| File | Integration responsibility |
| --- | --- |
| `src/runtime/textbook/Textbook.hpp` | Section/figure/exercise types and retained owners |
| `src/runtime/textbook/MatrixChapter.cpp` | Section factory registration, order and stable bookmark generations |
| `src/runtime/textbook/Textbook.cpp` | Registry-sized reading state, explicit routing and bookmark migration |
| `src/ui/TextbookUi.*` | Four-view navigation, reading, exercise and scene selection |
| `src/ui/TextbookFigureUi.*` | Shared object controls/readout selections and geometry publication |
| `app/math_lab_main.cpp` | Registry-derived CLI bounds and early text-only validation |
| `CMakeLists.txt` | Only required source/target/test registrations |
| `tests/textbook_tests.cpp` | Navigation, bindings, references and bookmark compatibility |

`BookExerciseKind` explicitly routes MatrixBoard, Systems and Object practice.
For an object lesson, supply an `ObjectLessonSpec`, a section factory,
`BookFigureKind::Object`, `BookExerciseKind::Object`, and the next immutable
`bookmarkGeneration`. Register the factory once in `MatrixChapter.cpp`. Section
counts, reading/help storage, navigation and CLI bounds derive from that registry.
Do not invent a source card or call `book.board()` for an object exercise.

`ObjectLesson` sends controls, presets and Check to existing MathObjects owners.
Each lesson retains independent exploration and practice. Preset zero initializes
practice and must be unsolved. Optional metric/matrix selectors choose existing
readouts by name. The common UI requires no topic-specific copy for these
bindings. New interaction types still need an explicit integration specification.

Keep existing section IDs and bookmark generations unchanged. New sections get
a later generation. The reader accepts only complete older generations, rejects
unknown or duplicate records, and initializes new reading positions to zero.
CMake source/test registration remains an explicit small edit.

### 5. Verify and repair

Run the named targets in the filled task. Baseline pure targets available today:

```sh
cmake -S . -B b-lesson-headless -DPATHS_BUILD_NATIVE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build b-lesson-headless --target paths_textbook_tests paths_textbook_figure_tests paths_system_lesson_tests paths_matrix_board_tests paths_determinant_lesson_tests paths_math_object_tests -j 6
ctest --test-dir b-lesson-headless -R '^(paths_textbook_tests|paths_textbook_figure_tests|paths_system_lesson_tests|paths_matrix_board_tests|paths_determinant_lesson_tests|paths_math_object_tests)$' --output-on-failure
```

Add the task's meaningful numerical/behavioral target and affected existing model
tests. For simple prose or caption changes, reuse existing content checks; do not
write tests that merely reproduce strings or the implementation.

Compile the native executable, then run only the reviewed `--validate` cases:

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b --target math_lab -j 6
```

Check invalid-action atomicity, intended state sharing, hidden practice answers,
geometry bounds and finite values where the change affects them. Reuse the
existing 1,008 layout cases; do not add another layout test matrix when the shell
has not changed. A native compile is not a visual or pointer test.

Diagnose a failing check and repair its cause. Do not weaken the check or relabel
a numerical failure as success. Once the named checks pass, finish delivery;
broaden testing only for a new change, failure or unresolved concern.

### 6. Install without overwriting concurrent work

If work used a temporary staging copy, compare live hashes with the starting
versions before installing only the task's owned files. On drift, merge the
small intended change into the newest live file and rerun affected checks.
Never replace a live shared `CMakeLists.txt` with an older complete copy.

Build the installed `paths/b/math_lab`, verify its named text-only launch cases,
and check the scoped diff. No commit, push or visible application launch is part
of this workflow. Ask for permission only when the actual environment requires
it, explaining the concrete blocked action and reason.

### 7. Return a usable result

The final response needs the lesson title, absolute launch command, 2–3 visual
actions with expected outcomes, checks passed, and a short material limitation.
State that no images were used and changes are uncommitted. Automated completion
and user visual acceptance are separate facts. Update the existing workstream
entry and one lesson document; do not create extra diaries or duplicate ledgers.

## Calibrate Terra with a pilot

Use one complete lesson per task. Record a compact line in its delivery note:
model/effort, completed scope, passed checks, material repairs and user verdict.
Use actual available timings/usage only; do not invent a speed or savings ratio.
If the pilot works, reuse the packet structure for the next lesson, with Medium
for routine extensions and High for new integrations. Keep mathematical design
review focused on new mathematical assumptions rather than repeating every check.

After two or three successful lessons, consider automating the mechanical parts:
section registration, stable-ID collision checks, CLI bounds and test wiring.
Keep prose, mathematical conventions and independent expected results in the
filled task. No lesson generator or unattended batch runner exists as part of
this workflow; stabilize the pattern before building one.

P051 completes the pilot implementation and prepares the three-item queue.
Launching a worker or changing model settings is separate from writing these
files. Use the bounded start instruction in Batch 01 when the user requests
that run; preserve the no-image constraint throughout.
