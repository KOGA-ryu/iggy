# Paths

## Interactive maths objects and linked diagrams

The native `math_lab` now contains twenty-two objects. The original expansion cube,
unit circle, integration disks and binary graph remain available. The matrix
object now has four working learning layers, and the new function and surface
laboratories connect their geometry to interactive diagrams. Symmetry, harmonics
and motion add three further families. Modular drums, the Gaussian lattice and
the vector-field chamber add another three. Flux shells, tensor blocks and a
probability network extend the collection. The new probability/statistics batch
adds a binomial board, Bayesian cube and covariance cloud. Spherical harmonics,
quadratic forms and roots of unity now add twelve more working layers.

| Object | Learning layers |
| --- | --- |
| Functions | Inputs/roots; secants and derivative limits; signed accumulation and bound reversal; Taylor approximation |
| Matrices | Volume/rank; full 3x3 matrices, composition and eigenvector probes; projection/least squares; four SVD stages |
| Surfaces | Height/contours; sections, partial derivatives and tangent planes; gradient descent/Hessian; stationary points on a circle |
| Symmetry | Labelled cube rotations; composition/Undo; cyclic subgroups; orbits, stabilisers and permutations |
| Harmonics | Complex rotor; Lissajous curves; Fourier sums and signed spectra; coefficient extraction |
| Motion | Spring/pendulum initial states; phase/energy; damping; forcing and linear-spring convolution |
| Modular drums | Residues; cycles and GCD; modular inverses; Chinese remainder systems |
| Gaussian lattice | Complex arithmetic; norm and Euclidean division; principal ideals; quotient rings and zero divisors |
| Vector-field chamber | Vectors and dot products; parameterized paths; line integrals; potentials and path dependence |
| Flux shells | Surface normals; flux integrals; divergence theorem; Stokes and oriented boundaries |
| Tensor blocks | Outer products; three-index tensors; contraction; orthonormal coordinate changes |
| Probability network | Sampled walks; transition matrices; distribution evolution; stationarity and convergence |
| Binomial board | Independent trials; binomial probabilities; mean/variance/tails; normal approximation |
| Bayesian cube | Joint probability; conditioning; Bayesian odds; repeated evidence |
| Covariance cloud | Mean; covariance/correlation; PCA; whitening |
| Harmonic sphere | Spherical coordinates; 25 real harmonic modes; superposition/orthogonality; spherical heat diffusion |
| Quadratic forms | Evaluation/sections; orthogonal diagonalization; inertia/zero sets; Rayleigh quotient extrema |
| Roots of unity | Root constellation; complex power maps; cyclic subgroups; cyclotomic polynomials/Galois action |

The level selector changes the available operations and challenge. Drag a
plot to move its input, drag entries in the matrix diagram to edit A, or click
the contour map to move the surface point. Narrow windows put linked plots
and matrices in tabs. Left-drag the 3D viewport to orbit, right-drag to pan,
and scroll to zoom. Symmetry has turn and Undo controls. Harmonics and motion
can play, pause, advance, restart or scrub time; playback stops at the time limit.
Modular drums have forward/backward steps and an observed-visit table. Gaussian
integers have plane/norm-height views and exact quotient-class colouring. The
field chamber adds path reversal and playback with linked work plots. Values
and plots occupy separate tabs when both are available. The newest objects
also expose matrices in their own tab. Short windows use a compact subject
selector. The probability network has separate controls for sampled walks
and advancing complete distributions; the Stokes boundary can play or scrub.
The binomial board steps through a reproducible trial path. The Bayesian cube
compares joint and conditioned probabilities. The covariance cloud projects
points onto principal axes and transforms full-rank data into whitened coordinates.
The harmonic sphere links angular probes to meridian/latitude plots and a heat
energy curve. Quadratic zero-set diagrams can move the shared probe. The root
constellation shows power maps, rising subgroup paths and exact polynomial coefficients.

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b -t math_lab -j4
./b/math_lab
```

For a text-only run that creates no native host, window, rendering or images:

```sh
./b/math_lab --validate --object function --level 3 --set function=2 --set at=0.5 --set degree=5 --check
./b/math_lab --validate --object linear --level 3 --preset 2 --check
./b/math_lab --validate --object surface --level 3 --set surface=1 --set constraint=1 --set circle_angle=0 --check
./b/math_lab --validate --object symmetry --level 1 --turn x --turn y --check
./b/math_lab --validate --object harmonics --level 3 --set waveform=0 --set probe_frequency=2 --check
./b/math_lab --validate --object oscillator --level 3 --advance 8 --check
./b/math_lab --validate --object modular --level 3 --set modulus=3 --set integer=2 --set modulus2=5 --set residue2=3 --set crt=8 --check
./b/math_lab --validate --object gaussian --level 3 --set quotient=0 --check
./b/math_lab --validate --object field --level 2 --set field=2 --set path=4 --reverse-path --set path_time=0 --advance 1 --check
./b/math_lab --validate --object flux --level 3 --set flux_field=2 --set flux_orientation=1 --set boundary_time=0 --advance 1 --check
./b/math_lab --validate --object tensor --level 3 --set basis_angle=45 --check
./b/math_lab --validate --object probability --level 3 --set chain=1 --set chain_steps=12 --check
./b/math_lab --validate --object binomial --level 3 --set trials=12 --check
./b/math_lab --validate --object bayes --level 3 --check
./b/math_lab --validate --object covariance --level 3 --check
./b/math_lab --validate --object spherical --level 3 --set heat_time=0.5 --check
./b/math_lab --validate --object quadratic --level 3 --set form_yaw=0 --set form_pitch=0 --set form_x=0 --set form_y=0 --set form_z=1 --check
./b/math_lab --validate --object roots --level 3 --set root_n=8 --set root_auto=5 --check
```

Arguments apply in order. `--help` lists every parameter and its range. The
reusable model and scene adapter remain in `src/runtime/math_objects/` and
`src/scene/MathObjectScene.*`; they use the existing SceneFrame/Vulkan pipeline.
Exploration state is separate from scored question attempts and study saves.
See [P039](docs/P039_MATH_OBJECTS.md) for the original five objects and
[P040](docs/P040_LINKED_MATH_LAYERS.md) for function/matrix/surface layers, and
[P041](docs/P041_SYMMETRY_HARMONICS_MOTION.md) for symmetry, harmonics and motion.
[P042](docs/P042_NUMBERS_AND_FIELDS.md) documents modular, Gaussian and field
objects. [P043](docs/P043_SHELLS_TENSORS_PROBABILITY.md) documents flux shells,
tensors and probability. [P044](docs/P044_PROBABILITY_STATISTICS.md) adds binomial,
Bayesian and covariance objects from the expanded TOC.
[P045](docs/P045_SPECTRAL_FORMS_ROOTS.md) adds spherical harmonics, quadratic forms
and cyclotomic root constellations. Agent verification
remains text-only under the user's no-images constraint. The user reported
successful tries of P041 and P043; P044 and P045 visual review remain for the user.

## Existing game modes

A continuous arcade math game. Read the prompt, click the matching coloured
targets, and keep shooting. Wrong clicks give a brief signal while the same
question stays active. Clearing its required answers automatically starts the
next step or question. Endless play continues until you choose to stop.

**Stop / Esc** pauses the run and shows your totals. **Resume** keeps going from
the same point. Detailed answer review is optional while stopped. Mistakes never
open a lesson, require an acknowledgement or send you into a correction round.
See [the continuous-play loop](docs/P019_CONTINUOUS_PLAY.md).

The separate `sorter` app now opens a [table of contents](docs/P028_STUDY_SELECTION.md):
**Algebra → Linear equations → Bracket equations**, plus
**Algebra → Straight lines → Slope and intercept** and
**Algebra → Simultaneous equations → Two straight lines**, and
**Linear algebra → Matrices and systems → Row reduction**. The default study pack
has six bracket problems, four single-line graphs, four simultaneous equations
and thirteen matrix row-reduction questions. The new **Linear algebra → Matrix
practice** chapter adds four **Integer foundations**, four **Swaps and negatives**,
and four **Exact fractions** problems. The [problem and answer sheet](docs/P037_MATRIX_PRACTICE_RESULTS.md)
lists their intended skills and tested solving routes.
Choose whole chapters, a random
number, or individual questions, then **Start set**. **Next**
follows only your selection. **Back to contents** pauses the question, and
**Resume set** keeps your place even if you edit the next selection. Starting
another set saves earlier attempts and begins fresh ones. Text and selection
buttons remain compact; more subjects and problem types will join this catalogue
as their solving content is prepared.

[Contents progress](docs/P038_CONTENTS_PROGRESS.md) uses a small empty grey circle
for an unstarted attempt, a **cyan dot** for work in progress and a **green tick**
for a completed current attempt. Chapters show completed/total, such as **5/12**,
across all their questions. Undo, Replay and starting a fresh set update the marks;
earlier runs remain saved. The marks return with saved practice.

[Practice progress](docs/P036_SAVED_PRACTICE.md) now saves automatically in the
rebuilt sorter. Close the game and reopen it: the contents page shows a **green**
save message and a **blue Resume set** button. Your question order, current
working, wrong attempts, help use, Undo branches and earlier runs are retained.
A completed problem still waits for **Next**. Selection edits for a future set
stay separate from the active practice queue.

[Catalogue growth](docs/P037_GROWING_MATRIX_PRACTICE.md) preserves existing saved
practice when questions are added or reordered. Resume keeps the same problem,
working, history and question order. New problems appear in Contents for your
next selection; selecting **All** again includes newly available problems in the
chosen titles. **Start set** explicitly replaces the active queue. P036 saves
open directly and migrate after the next change. If a question needed by your
saved draft or working has changed, the amber message identifies its card and
the original save stays untouched.

The file lives in SDL's user application-data folder for **Paths**, named
`practice-study_practice_v1.json` for the default catalogue. Each catalogue
filename has its own save. Use `--progress /absolute/path/practice.json` to
choose a file, or `--no-progress` for a temporary session. Content checks,
scripts and bounded runs do not use personal saves unless a progress file is
explicitly supplied. An unreadable or incompatible file remains untouched;
an amber status explains why saving is paused. This saves practice on this
device; group arrangements and presentation settings are not part of the save.

The [coordinate board](docs/P029_COORDINATE_BOARD.md) builds a line in four
decisions: intercept, run, rise, and second point. The **gold** problem stays
fixed, the **teal** slope triangle grows with accepted answers, and the final
line is **mint**. Numerical choices sit below the same graph. **Replay** repeats
the latest drawing movement. After completion, drag on the graph or adjust
the **x** slider to move its teal point and read its coordinates. Keyboard:
Tab to the slider, Space to adjust with arrows, Space again to finish; Enter
allows a typed value. This exploration does not change recorded answers.
**Play again** starts a fresh attempt; **Next problem** advances only on request.
The four examples cover positive, negative, fractional, and zero slopes.

[Simultaneous equations](docs/P030_SIMULTANEOUS_EQUATIONS.md) use that same board
for two fixed original equations, a **teal** first line and a dashed **violet**
second line. After finding both slopes, move the shared **x** guide to compare
the two y-values, choose the number of solutions, and check the result. The
four examples include integer and fractional intersections, parallel lines,
and two equations describing the same line. The final intersection is marked
**mint**; coincident lines receive a mint stroke along their shared solutions.
The choices use numbers, coordinate pairs and a drawn **∞** symbol. Help,
Replay, Resume and explicit Next retain the existing controls.

Both graph chapters now include a [linked value table](docs/P031_LINKED_VALUES.md).
Three compact sample rows sit beside the graph; click one, or Tab to it and
press Enter, to move the existing x guide. The **amber** bottom row follows
the current input. The numerical substitution above the graph highlights the
same x in amber, with **teal** y-values for the first line and **violet** for
the second. Readouts use two decimal places and `~` for approximation; the
graph retains full calculation precision. Values appear when the guide unlocks:
after completing a single line, or after revealing both lines in a system.
The original problem stays fixed, and exploring values records no answer.

**Groups** opens the existing [Equation Sorter](docs/P020_EQUATION_SORTER_SPEC.md), which loads 100
prepared algebra equations from JSON. Choose A–E or Dump, click a card once to
inspect it, and again to store it. Click the active bucket to open its inventory;
the same two clicks return a card to its original grid cell. Undo reverses an
individual move or a confirmed Empty bucket action. All grouping is freely
chosen by the player. **Auto sort** groups remaining cards by their prepared
subjects in one undoable move: A Algebra, B Trig, C Calculus, D Linear algebra,
E Discrete maths. Cards you already placed stay in their groups. **Hint / Next**
explains the current action and shows the inspected card's subject and prepared
hint, or an available example if no card is inspected. After all cards are
grouped, one group click opens its inventory. See
[sorting assistance](docs/P022_SORTER_ASSISTANCE.md). The shooting game remains
a separate startup.

The six bracket questions now use [visual mathematical choices](docs/P033_VISUAL_MATH_MOVES.md).
Click a concrete symbol move such as **÷ 3**, **× 1/3**, **- 2**, or **a(b+c)**,
then click the resulting equation from four compact tiles. Both the number and
the equation are selected visually; there are no solving text fields or separate
Check button. For example, `3(x + 2) = 21` accepts division first (`x+2=7`,
then `x=5`) or expansion first (`3x+6=21`, `3x=15`, then `x=5`). The same
exact checker judges every selected result. Wrong choices keep the active
working and tile order in place so you can retry. Fractions appear directly
in the choices.

The [linear algebra question](docs/P034_MATRIX_ROW_MOVES.md) uses the same two-click
format with augmented matrices. It solves `2x + y = 7` and `x - y = -1` by
swapping, dividing and combining rows. Click a row operation, then its resulting
matrix. Both a swap-first route and a fraction route reach `x = 2, y = 3`.
In Contents, uncheck Bracket equations and select Linear algebra to practise
only the matrix question. Its original matrix stays gold, current working cyan
and checked steps green. No equation or matrix typing is needed.

A **violet ?** beside the selected row move opens its [reference](docs/P035_ROW_REFERENCES.md)
in the existing support area. Read the definition and rule, then use **< / >**
to follow a separate example through its three columns. Cyan links the current
column and calculation; green marks completed example working. **Close** or
**Esc** returns to inspection. Reference browsing preserves the original
problem, selected choices, attempts and working. On short windows, the reference
body scrolls beneath pinned controls. Example stepping follows Pause / Resume.

The **gold** original problem stays above the **cyan** active equation and its
compact controls. Each checked move adds a connected **green** block with a
check mark in the blueprint below. Select a block to inspect its working and
justification. **Undo move** returns to its parent while retaining attempts
and the old branch. **Pause / Resume**, return to contents, and per-question
resume preserve your working while the app remains open. **Play again** and
starting a fresh set archive earlier attempts. Progress is currently held in
memory, so closing the app ends the session.

Completion checks your numerical answer in the original equation and waits for
**Next problem**. In a study set, Next follows only the frozen selection and
stops at its last question. The six examples include positive and negative
coefficients, subtraction, a zero answer and fractions. They are available in
the default study pack and this smaller pack:

```sh
./b/sorter --content content/sorter/bracket_practice_v1.json
```

The [recipes](content/authoring/linear_bracket_recipes.json) generate the
versioned questions and their `linear_moves` opt-in. Their prepared chain is
retained for prepared arcade consumers; the sorter checks your selected moves
through the shared question owner. Graph chapters keep their numerical choices,
linked visuals and optional help. The original [prepared solving](docs/P023_PREPARED_SOLVING.md)
and [workspace](docs/P025_FIXED_SOLVING_WORKSPACE.md) records document earlier checkpoints.

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release # First build only
cmake --build b -t sorter -j4
./b/sorter
# Read edited content directly; no rebuild is needed for text changes:
./b/sorter --content content/sorter/equations_v1.json
# Try the mixed-subject sample pack:
./b/sorter --content content/sorter/mixed_foundations_v1.json
```

Tab/arrows move focus; fresh Enter/Space presses activate. Escape closes an
open hint first; otherwise it clears inspection or cancels an empty confirmation.
Cards leave fixed placeholders,
and assignments have no animation delay. State and Undo last until closing.
The original algebra pack covers four algebra forms. The separate
[mixed-subject pack](docs/P021_MIXED_MATH_PACK.md) retains 80 of those cards and
adds five each for trig, calculus, linear algebra and discrete maths. These are
small prepared samples with independently checked results and plain-text
notation. The loader reads declared subjects without interpreting the maths. Rich mathematical
typesetting and solving sequences for the other subjects remain later work.

This folder is the standalone project root. It contains a pinned seed of First
Move's models and UI, the native helper code it needs, Dear ImGui, and reviewed
source-card plans. The independent executable/build is being implemented under
[P001](docs/P001_BUILD_PACKET.md); copying these files alone is not a completed
native migration.

The [gallery target foundation](docs/P006_TARGET_FOUNDATION.md) extends the
separate `gallery` startup with coloured spheres, stationary answer
rows, immediate judging, collection, and automatic question changes. Four
startup presets exercise the shared target/question boundary using reviewed
foundation questions. Separate packs below add the two authored source cards.
Points/bonus rules and saved profiles remain later workstreams.

```sh
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release # First build only
cmake --build b -t gallery -j4
./b/gallery
./b/gallery --start-mode workshop
./b/gallery --start-mode equality_sweep
./b/gallery --start-mode question_relay
./b/gallery --start-mode equation_chain
./b/gallery --start-mode substitution_chain
```

Launching `gallery` now opens **Choose questions**. Select Starter
questions, Three points, one formula, or The closest point on a line, then
press **Play**. Starter questions also offers its four practice types.
Up/Down selects a set and Enter starts it. During a game, use **Stop / Esc**
then **Choose questions** to return. **Resume** continues a previously started
set; each set and practice type keeps its own progress until the app closes.
See [the menu checkpoint](docs/P013_QUESTION_MENU.md) for scope and verification.

Before **Play**, choose **Movement** and **Speed**. Movement offers Stationary
and twelve patrol patterns; the speed slider covers 0.01–100 m/s. Stationary
targets stay still. Each started game keeps its settings: **Resume** shows
them with the controls locked. An unstarted set or practice type uses the
latest pending choices. See [movement setup](docs/P014_MOVEMENT_SETUP.md).

To change a started set's setup, choose **New game** beside Resume. It opens
with that game's current movement and speed. **Cancel** (or Esc) keeps the
existing game. **Start new game** replaces that set/practice type's progress
and answer history, beginning at the first question with the chosen settings.
Other games stay intact. If loading fails, the old game remains available;
repair the content and retry, or Cancel and Resume. See
[the new-game checkpoint](docs/P016_NEW_GAME.md).

After **Stop / Esc**, optionally choose **Answer review** to inspect the current question.
Each reached step shows its result, the working you saw and your submitted
answers in order, with written Correct/Incorrect labels. Expand a row for
details, or choose a completed question from the selector. Partial answer sets
show how many answers you have collected; future steps and explanations for
unfinished steps stay hidden. **Back to game** returns to the paused game; use **Resume**
when ready. History lasts for that game in this app session; New game replaces
the selected game's history. See [answer review](docs/P017_ANSWER_REVIEW.md).

Expand a resolved step to read its **Explanation** below your attempts. A
choose-one step unlocks it after one valid answer; a choose-all step unlocks
it after the full set is collected. Earlier explanations remain available
while you solve later steps and in completed-question history. The text comes
from each card's existing `explanation` field; an empty string omits the
section. See [review explanations](docs/P018_REVIEW_EXPLANATIONS.md).

**Developer workshop** opens the [Gallery Workshop](docs/P005_ENGINE_PORT.md).
The explicit `--start-mode workshop` launch still works. The workshop provides
camera controls, editable boxes/ramps/frames/spheres, and twelve
motion patterns plus stationary and custom waypoints. Pause to preview the
next 60 seconds of a route. Changes last for the session. Game presets use a
fixed camera and a visible cursor; click a ball to submit its matching answer.
Use `--seed 19 --motion circle --pace 0.7` to select a repeatable setup.

Substitution Chain is a file-loaded playtest of the integral
`6x(x^2 + 1)^2 dx`: choose the substitution, differentiate it, find the
multiplier, integrate, substitute back, and verify by differentiation. Its
working changes only when the question session advances. All five gallery
examples now live in [question files](content/cards/), selected by the ordered
decks in [the starter pack](content/packs/gallery_foundation.json).
The [content folder guide](content/README.md) explains which files to edit
for playable questions and where the retained authoring material belongs.

The build copies `content/` next to the executable. That bundled starter pack
is the default regardless of the folder you launch from. To play directly
from your edited source files, select the pack explicitly:

```sh
./b/gallery --start-mode substitution_chain \
  --content-pack content/packs/gallery_foundation.json
```

Add or edit cards and change the pack's deck, then relaunch the same executable;
no C++ rebuild is needed. An active run keeps its original content. Pack paths
resolve from the launch folder; card paths resolve from the pack's folder.
Invalid content reports its source and field. Direct gameplay launches stop
before graphics startup; the menu stays open and offers **Try loading again**.
Existing games retain their original content when resumed. See
[the question-file contract](docs/QUESTION_CONTENT_FORMAT.md) for fields,
stable answer IDs, versioning, and limits. Workshop does not accept a content
pack because it has no questions.

[Three points, one formula](docs/P011_CARD_002_ADAPTATION.md) is now a separate
13-decision source-card pack. It covers unknowns and inputs, building a linear
system, solving for the coefficients, and checking the recovered function:

```sh
./b/gallery --start-mode equation_chain \
  --content-pack content/packs/source_002.json
```

The original authored card is retained alongside the prepared game file, with
its source attribution and an explicit stable-ID mapping in the adaptation
record. This pack plays through the existing gallery executable.

[The closest point on a line](docs/P012_CARD_013_ADAPTATION.md) adds a separate
14-decision pack: parameters, projection conditions, coordinates, and why the
point is uniquely closest in Euclidean distance. The target formula stays
visible because this is guided derivation practice.

```sh
./b/gallery --start-mode equation_chain \
  --content-pack content/packs/source_013.json
```

Both source packs retain their authored working and per-decision attempts,
then restart in endless mode using the existing target and question owners.

Building native targets also requires `glslangValidator` (or `glslang`); the
compiled scene shaders are embedded into the executable. The pure scene and
learning models can be built with `-DPATHS_BUILD_NATIVE=OFF`.

- [Architecture](docs/ARCHITECTURE.md) — ownership, modes, evidence, and build boundary.
- [Workstreams](docs/WORKSTREAMS.md) — current build and following capabilities.
- [Source-card architecture](content/SOURCE_CARD_ARCHITECTURE.md) — two complete authored adaptations.
- [002: Three points, one formula](docs/P011_CARD_002_ADAPTATION.md) — 13 playable gallery decisions and retained authoring source.
- [013: Closest point on a line](docs/P012_CARD_013_ADAPTATION.md) — 14 playable gallery decisions and retained derivation.
- [Planning validation](content/planning_validation.json) — exact arithmetic and provenance checks, separate from game tests.
- [Migration seed](docs/MIGRATION_SEED_MANIFEST.json) — where each borrowed file came from.

The original `paths` startup builds with `cmake --build b -t paths -j4`.
SDL3 and Vulkan are external SDK dependencies; the parent iggy3d checkout and
build are not runtime dependencies.
