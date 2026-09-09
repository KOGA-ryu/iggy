# Standalone calculator and connected tutoring

Status: **Deferred project / TODO. Specification only; implementation has not
started.** Recorded 2026-09-09 at the user's request for work down the road.
This does not start a build, create another task, schedule a reminder, or replace
the current question work. Resume one milestone only when requested.

## Product goal

Build a calculator that works independently and that Paths can teach someone to
use. A lesson presents a problem, explains an operation, highlights the relevant
calculator control, observes the user's action and responds to the resulting
mathematics. The experience should feel like a typing tutor for mathematical
tools, with buttons and symbolic choices instead of mandatory equation typing.

Start as a separate executable in the Paths CMake project, sharing components
with the existing apps. A separate repository, independent installer and broad
computer-algebra product are later decisions. Working name: **Paths Calculator**;
the target and product names are not established yet.

## Existing foundations and gaps

These are source-level observations, not acceptance of a finished calculator:

| Foundation | Reuse boundary |
| --- | --- |
| [CMake targets](../CMakeLists.txt) | `sorter` and `math_lab` already share `paths_textbook_ui`, `paths_native_math` and the native host. Add a bounded consumer; do not copy an app wholesale. |
| [MatrixBoard](../src/runtime/matrix_board/MatrixBoard.hpp) | Numerical complex matrices, row operations, accepted/rejected commands and history; currently bounded to 32 rows/columns. Reuse where its calculation contract fits. |
| [Question session](../src/runtime/first_move/LayeredQuestionSession.hpp) and [CorpusPractice](../src/content/CorpusPractice.hpp) | Canonical graded answers, exact supported mathematics, help exposure, working, Undo and saved attempts. Numerical board checks do not replace these. |
| [Math controls](../src/ui/MathControlUi.hpp), [matrix UI](../src/ui/MatrixBoardUi.hpp), [NativeMath](../src/ui/NativeMath.hpp) | Reusable presentation and input adapters. These do not yet provide a cross-app control registry. |
| [Textbook blocks](../src/runtime/textbook/BookLesson.hpp) | Existing researched reading structure, definitions and disclosure-aware presentation. |
| [Document compiler](LEARNING_DOCUMENTS.md) and [publisher](LEARNING_EXPORTS.md) | Existing authoring, validation, provenance and publication route; extend with a versioned binding when needed. |

New work includes an independent calculator workspace/save, backend adapters,
stable action/control bindings, connected-session protocol, guidance rendering
and declarative lesson bindings. Recheck these boundaries when starting; other
workers may have added reusable components since this specification.

## Ownership and command route

| Responsibility | Canonical owner |
| --- | --- |
| Lesson goal, next prompt, allowed assistance and graded completion | Paths question/session owner; textbook supplies referenced teaching. |
| Free calculation inputs, working, results and history | Calculator workspace using shared mathematical owners/backends. |
| A connected graded attempt | Existing Paths question owner. Calculator displays its accepted state and collects proposed responses. |
| Numerical calculation | Selected shared backend adapter; UI and transport never implement independent arithmetic policy. |
| Button placement, highlighting and input focus | Calculator UI, bound to named semantic commands. |
| Delivery, connection state and resynchronization | Local connection adapter; it never grades or invents an accepted operation. |
| 3D geometry, animation and model behavior | Existing asset/model worker. Textbook worker owns lesson integration and the rest of this project. |

Use one dispatcher for each owner. A calculator control invokes a semantic
command whether clicked normally or used during tutoring. In free calculation,
that command reaches the calculator workspace. In connected practice, a proposed
response reaches the question owner through its existing validation/journal
route; the returned projection becomes the visible checked working. An invalid
response can record an attempt but cannot mutate accepted working. Keep input
drafts distinct from committed mathematical state.

Do not have the numerical board accept an operation and then independently
declare the graded question passed. Navigation, guidance delivery, successful
transport and a backend's computed answer are not learner completion events.
If a task requires a new response kind, implement it in the canonical question
model before connecting a control to it.

## Learner experience

- Keep the problem beside the active response and current working. A connected
  calculator repeats the given so the learner need not scan between windows.
  Use the established compact controls and textbook formatting.
- Highlight the requested control in **gold**, show working in **cyan**, help
  in **purple**, and accepted completion in **green**, with text/symbol labels.
  Highlighting is rendered from internal control IDs; no screenshots, screen
  recognition, coordinate-based clicks or simulated learner input.
- Make teaching available beside the action. If a requested control is in a
  collapsed group, reveal that group without stealing OS focus or running it.
  Unsupported/unavailable controls produce an explanation, not a guessed click.
- Provide four guidance amounts: explained/highlighted steps; short cues with
  optional hints; checkpoints without a suggested operation; independent work
  with checking on request. All can use buttons/palettes. Keep input method
  separate from assistance. These are proposed calculator presentations, not a
  silent change to the existing Learn/Practice/Solve/Write modes or save meaning.
- Distinguish **learn this tool** exercises, which may require a specific
  command sequence, from **solve this problem** exercises, which should accept
  alternative valid routes within the checker’s documented capability.
- Wrong choices retain working and receive specific feedback. Preserve Undo,
  branches, assistance records and saved place. Completion waits for **Next**.
- A free calculation or demonstration is distinct from a graded attempt.
  Revealing a computed solution during practice is an explicit, recorded help
  action governed by the question owner. Hints, previews and reconnect snapshots
  must obey the same disclosure rules.

## Mathematics and library policy

Eigen is the proposed first numerical backend for the linear-algebra calculator:
matrix operations, appropriate decompositions and linear solves, with later
QR/SVD/eigenvalue features added by capability. See the official
[Eigen linear-algebra documentation](https://libeigen.gitlab.io/eigen/docs-nightly/group__TutorialLinearAlgebra.html).
The documentation link is a reference, not a dependency version pin.

- Pin and attribute a reviewed supported Eigen release at implementation time;
  record its license, compiler/platform compatibility and reproducible build
  source. This TODO adds no dependency or download.
- Preserve exact integer/fraction input for supported teaching problems. Crossing
  into floating-point computation must be explicit; approximate output must not
  become exact grading evidence by rounding it or converting it back to a fraction.
- Give each operation a typed contract: number domain, dimensions, assumptions,
  resource limits, algorithm and precision policy. Results distinguish exact,
  approximate, unsupported and failed cases, with useful diagnostics and a
  residual/convergence report when applicable. A small residual alone is not a
  guarantee of a small solution error for an ill-conditioned system.
- Verify numerical solutions against original inputs. Cover singular and badly
  conditioned systems, non-finite input, dimension mismatch and zero right-hand
  sides in the selected operation's policy. Do not silently substitute a
  least-squares answer for an exact solve request.
- Keep numerical computation and pedagogical derivation distinct. Reuse or author
  checked steps and definitions; a library result does not itself supply a lesson.
- Consider symbolic, arbitrary-precision, sparse or specialised numerical
  backends only for a named later capability. Do not build a plugin framework or
  add overlapping libraries before there is a live operation requiring them.

## Local app connection: proposed contract

Default to a local socket adapter on the current platform. Both apps should
continue to work independently when disconnected. No network service, account,
cloud dependency or automation of unrelated third-party calculators is required.
Use a user-local endpoint, bounded messages and an explicit session handshake;
only registered operations and typed data are accepted, never executable code.

The following message names are design placeholders, not implemented APIs:

| Message | Purpose |
| --- | --- |
| `hello` / `capabilities` | Negotiate protocol/schema version, supported operations, domains and limits. |
| `bind_task` / `snapshot` | Bind a question identity/version/stamp to a session and receive the authorized current projection. |
| `guide` / `clear_guide` | Request a named control and approved cue for a particular step/state; report visible, unavailable or unsupported. |
| `propose_action` | Submit a user's semantic action and typed operands against an expected owner revision. |
| `action_result` | Report accepted/rejected/unsupported status, reason, event identity and resulting owner revision/projection. |
| `detach` / `resync` | End tutoring or recover the owner's current state without duplicating an action. |

Carry protocol version, session/question identity, request ID and expected state
revision on mutations. Separate successful delivery from mathematical acceptance.
An owner validates freshness, dispatches once and correlates its result. Reject
stale requests; duplicate requests must not repeat operations or attempts.
Reconnect from the owner's snapshot. Prove durable receipt/journal correlation
before claiming recovery of an ambiguous in-flight request; never blindly replay
a mutation after a lost connection. Guidance must target the current step and
clear when that revision is superseded or the session detaches.

Free calculation may continue offline in its own workspace. A connected graded
workspace pauses submissions while its authoritative Paths session is unavailable;
retain the visible working and draft. Keep calculator saves separate from Paths
progress. A reconnect cannot overwrite a different/newer attempt, change the
question stamp or expose hidden answers. Reuse the current journal/save protection
where applicable, with explicit versioning if new persisted data is required.

## Authoring contract to add later

Extend the existing `.paths.md` compiler/publisher with a versioned calculator
binding. Do not add a second parser or execute commands embedded in documents.
Every binding needs:

- Stable lesson/question/step identity and an existing mathematical template.
- Typed starting values, domain and required calculator capabilities.
- A named operation/control, operand choices or allowed input, a cue, relevant
  definition references and specific feedback for mistakes.
- The applicable guidance/disclosure level and either an explicit tool-sequence
  goal or a supported mathematical completion condition.
- Expected checked results verified by the existing question owner, plus source
  provenance and a fallback when the connected calculator is unavailable.

Validate unknown actions, wrong operand types, unsupported domains and changed
references before publication. Diagnostics must identify the source file, line,
directive and reason, including included fragments. Publish through the current
atomic library route and preserve previous question identities/saves. Freeze the
actual directive syntax after the first complete connected exercise proves the
contract; this document introduces no usable authoring commands.

## Milestones and TODO checklist

All items are deferred and unchecked. Each milestone is a separate checkpoint;
finishing one does not automatically start the next.

### 1. Confirm reuse and specify the first operation

- [ ] Recheck live owners, concurrent asset work and the current build graph.
- [ ] Choose the smallest existing UI/model interfaces needed by the calculator;
      document any required ownership change before extraction.
- [ ] Define typed matrix input, action IDs, result/status format, precision and
      dimension limits; select/pin Eigen and record dependency evidence.
- [ ] Define Calculate versus connected Practice routing and the minimum versioned
      local protocol, including receipts, revision checks and disclosure.

Exit: one reviewed operation contract and one owner per mutation/assessment.

### 2. Standalone linear-algebra calculator

- [ ] Add the separate executable using the existing host, matrix presentation,
      symbolic controls and mathematical typesetting.
- [ ] Support a bounded matrix/right-hand-side grid, row operations, Undo, Reset
      and an explicitly requested numerical `Ax=b` solve through Eigen.
- [ ] Display assumptions, approximate/exact status and useful error diagnostics.
- [ ] Save/reopen the calculator workspace independently of Paths progress.

Exit: usable without Paths; valid inputs compute, invalid inputs explain why,
and reopening restores work. No claim of general CAS support.

### 3. One connected button-tutor exercise

- [ ] Bind one supported `matrix.v1` problem to the calculator, with no automatic
      solution reveal and no change to existing question stamps.
- [ ] Implement stable control bindings and revision-bound gold guidance.
- [ ] Route actual button/operand choices through the question's canonical action
      owner; return checked state, feedback and assistance to both surfaces.
- [ ] Prove wrong choices, Undo, duplicate/stale messages, disconnect/reconnect,
      save/reopen and explicit Next through the same route.

Pilot: reuse the example in the [matrix document contract](LEARNING_DOCUMENTS.md#matrix-documents):

```text
Given:        [1, 1 | 3] [2, -1 | 0]
R2 += -2 R1:  [1, 1 | 3] [0, -3 | -6]
R2 /= -3:    [1, 1 | 3] [0,  1 |  2]
R1 += -1 R2: [1, 0 | 1] [0,  1 |  2]
Result:      x = 1, y = 2
```

Each step uses supported numeric-operand row operations. Highlight the operation,
offer symbolic operand choices and expand the current definition/reason in place.
Independent substitution verifies `1+2=3` and `2(1)-2=0`. The numerical Calculate
action is a separate demonstration/check, never a substitute for the learner's
accepted responses. Pause here for the user's visual/interaction check.

### 4. Reusable document bindings and guidance levels

- [ ] Finalize the versioned binding syntax using the pilot's proven contract.
- [ ] Add compiler diagnostics, capability checks and atomic publication coverage.
- [ ] Author a second question through documents alone, without a bespoke UI or
      hardcoded lesson branch, to prove reuse.
- [ ] Implement the four guidance amounts with button-based input, explicit
      exposure records and no reinterpretation of existing mode/save semantics.
- [ ] Prove tool-sequence and mathematical-goal lessons assess the declared goal;
      report unsupported alternative routes without a false wrong-math verdict.

Exit: an author can add a supported connected exercise through the write/publish
pipeline with actionable rejection reasons and no calculator source changes.

### 5. Expand specialist capabilities only after pilot acceptance

- [ ] Select one graduate-level operation at a time: possible candidates are
      QR/least squares, SVD/rank, or eigenvalues/eigenvectors.
- [ ] Specify its mathematics, numerical limits, controls, teaching template,
      independent verification and connected exercise before implementation.
- [ ] Add another backend only where that operation requires it.
- [ ] Assess standalone distribution and other-platform transports when requested.

Deferred beyond this plan: arbitrary symbolic proofs, unrestricted CAS input,
remote collaboration, third-party app control and new 3D assets. Reuse an existing
figure only when its disclosure and state binding are explicitly specified.

## Verification and return evidence

At implementation time derive affected targets from the live build graph. Use
targeted pure-model/protocol checks, a headless two-process connection test with
isolated saves, and the affected native builds. Required observable cases are:

1. Known results plus independent substitution; invalid/unsupported input and
   numerical failure preserve working with a precise reason.
2. Only real accepted learner actions advance practice; wrong/replayed/stale
   actions and mere guidance delivery cannot complete a step.
3. Disconnect, lost acknowledgement and restart retain one canonical attempt
   without duplicate operations, overwritten work or premature answer disclosure.
4. Document rejection identifies its source; valid publication and old saves
   remain compatible under the current identity rules.

Do not launch windows, take/view screenshots, capture images or run font/image
probes. The user confirms appearance and actual interaction after each relevant
build. Give short manual instructions naming **gold control/given, cyan working,
purple help and green completion**, including close/reopen and Next behavior.
Report automated evidence and manual acceptance separately, preserve unrelated
work and leave changes uncommitted unless requested.

For each checkpoint return: capability completed, canonical owners, changed files
and production LOC, relevant build/test evidence, limitations, manual check and
the next candidate. Stop after that checkpoint.
