# P020: Equation Sorter v1 — Paths specification

Status: implemented after the user's subsequent build authorization; automated
checks passed, native offscreen views inspected, changes uncommitted. Human
pointer/swapchain acceptance remains open. The gallery checkpoint remains P019.

Source: [astra-equation-sorter-handoff.md](/Users/kogaryu/Documents/ChatGPT/astra-equation-sorter-handoff.md).
Source SHA-256: `26caf34c8927b68e3ffe2e5c19bcf34640d57d281b76537fa72d61b14a331835`.
The source is unchanged. The initial task adapted its specification; the user
subsequently asked to start building, authorizing this standalone prototype.

## Delivery in Paths

The separate native executable, `sorter`, builds inside the existing Paths
CMake project. It reuses C++20, Dear ImGui,
SDL3 and `NativeVulkanHost`. Set `NativeLaunchConfig::enableScene=false` and
render the UI with no gallery scene. Use the current native SDK/build setup;
there is no browser page, web framework or additional dependency to install.

The sorter is a separate startup, following the project's original practice of
developing game variations independently. It does not become a required stage
before shooting, add a menu entry to `gallery`, launch another mode on
completion, or modify the gallery's question model, scoring or run history.
The handoff calls the other game Brake Shot; no assumption is made that this
name identifies a particular Paths executable. Leave any such separate project
untouched as well.

The player freely groups 100 equations into A, B, C, D or Dump. All assignments
are accepted. There are no family labels, correctness judgments, lessons,
timers, automatic grouping, drag-and-drop, backend, accounts, telemetry, saved
profiles or integration with the shooting game. State and Undo last until the
sorter closes. Redo is outside v1.

## Screen and spatial contract

Use a low-glare dark surface and readable off-white equations. A persistent
toolbar contains A–D and Dump with counts, Unsorted count and Undo. The selected
destination has both a persistent outline and the word **Active**. Start with
no active bucket and all 100 equations in the original grid.

The equation area scrolls vertically below the toolbar. Use responsive columns
with comfortable cells, initially around 220 pixels minimum width and 96 pixels
height. At narrow widths, wrap the toolbar and reduce the number of columns;
do not shrink the equations to fit all 100 on screen. Keep button widths and
status/confirmation space stable as counts and selection change.

Every equation has an immutable home index. The original grid always renders
all 100 home slots in that order, including placeholders. An assigned card
leaves a muted, noninteractive placeholder labelled with its owning bucket.
Returning it restores the same cell. No compaction, refilling, animated
reordering or movement of hit rectangles occurs on assignment or return.

Inspection changes outline and background inside
the existing cell. It does not move the cell, enlarge its hit area, obscure the
equation or open a dialog. Pointer hover, keyboard focus and inspection are
distinct states; focus alone never changes membership.

Inventory replaces the grid area. Its header shows the bucket name, count,
Back to grid and Empty bucket. It never squeezes the board into a side panel.
Reserve space for the inline empty confirmation within the inventory header.
Keep counts, inspection, commits and Undo from moving the scroll position.
Preserve grid scroll on return and use stable ImGui child identities for the
grid and each inventory. Clamp only when dimensions/content bounds require it.
User scrolling and explicit keyboard navigation can bring controls into view;
there is no automatic scrolling to a moved or restored equation.

## Interaction transitions

All rows below use one semantic dispatcher. An activation is one ordinary
mouse click or one fresh keyboard activation, not an OS double-click gesture.

| Current state and action | Result |
| --- | --- |
| Grid: select an inactive bucket | Make it active, clear inspection, stay in the grid |
| Grid: select the already active bucket | Open its inventory and clear inspection |
| Inventory: select another bucket | Make it active and open its inventory directly |
| Inventory: select its current bucket again | Leave the inventory and frozen slots open; do not compact or reset scroll |
| Back to grid | Return to the original grid, preserve active bucket, clear inspection |
| Activate a visible equation different from the inspected one | Inspect it; do not move the previously inspected equation |
| Activate the same inspected grid equation with a destination | Assign it to that bucket; clear inspection and keep the destination active |
| Activate a grid equation without a destination | It may be inspected, but cannot move; show a quiet Select a bucket hint |
| Select a bucket after inspecting without a destination | Clear inspection; the next equation activation only inspects |
| Activate the same inspected inventory equation | Return it to Unsorted at its original home; never transfer it into another bucket |
| Click background or a placeholder, or press Escape | Clear inspection without a move; Escape also cancels a pending empty confirmation |
| Focus or hover an equation | No inspection or assignment |

Switching buckets or views cancels a pending empty confirmation. Dump follows
the exact same rules as A–D; it means holding for later, not deletion or failure.

When Unsorted reaches zero, show a quiet **All 100 assigned** state. Keep
inventories, returns, emptying and Undo available. This is derived from counts,
not a terminal state. Undo or return can make equations Unsorted again.

### Frozen inventory slots and Undo

On deliberately opening or switching an inventory, capture its current members
in original home order. Freeze that list of equation IDs for as long as this
inventory stays open. A returned equation leaves a placeholder reserved for
that ID. Never reuse its slot for a different equation.

Undo that restores a removed member fills its reserved slot. A subtler case
occurs when Undo restores a member that was absent when this inventory opened:
append that ID at the end, without reusing holes or shifting existing slots.
Append several newly restored IDs in home order. A later deliberate reopen
rebuilds a compact home-ordered inventory. There are at most 100 unique reserved
IDs in an open inventory, regardless of how often the player uses Undo.

Required example: put e1 and e2 into A; return e1; go to the grid and add e3;
reopen A with slots `[e2, e3]`. Undo the e3 assignment, then Undo the earlier e1
return. The open inventory becomes `[e2, placeholder-for-e3, e1]`. Existing
cards have not moved, e1 belongs to A again, and the next deliberate reopen
orders members by home index.

### Empty bucket and Undo

Empty bucket is available only in an inventory and disabled at count zero.
Requesting it clears inspection and displays **Empty N equations back to grid?**
with Confirm and Cancel in the reserved inline space. Nothing moves yet.

Confirm operates on the canonical membership of that bucket, returning every
member to its original home in one transaction. Open inventory slots become
placeholders. Cancel changes no membership or history. Navigation and Undo
cancel the prompt before performing their action. Activating a card instead
cancels the prompt and inspects that card; it cannot also commit a move because
requesting Empty already cleared inspection. Confirmation is valid only for the
same still-open inventory and the unchanged membership it presented.

Undo reverses the latest committed assignment, inventory return or bulk empty,
even after changing views or buckets. One Undo reverses the entire bulk empty.
Repeated Undo continues until the history is exhausted, then the button is
disabled. Inspection, navigation, cancellation, rejected commands and no-ops
never enter history. Undo preserves the current view, active bucket and scroll,
clears stale inspection/confirmation, and restores exact membership and homes.

## Content boundary

Keep this unscored catalog separate from `LayeredQuestionContent`. The existing
question format requires answer choices and accepted IDs, and its catalog
capacity is 64. Do not fabricate correct answers or change those limits to fit
100 sortable expressions. This capability needs display content and ownership,
not a judging model.

Runtime file: `content/sorter/equations_v1.json`.

```json
{
  "schema_version": 1,
  "equations": [
    { "id": 1001, "home_index": 17, "text": "x + 3 = 7" }
  ]
}
```

This illustrates the shape; a valid file must contain exactly 100 records.
IDs are unique positive 32-bit values. Home indices form exactly the permutation
0–99 and are independent of IDs and file order. Text is a distinct, nonempty,
single-line equation, up to 96 bytes for this fixture. The v1 generator uses
ASCII signs and parentheses supported by the current font. A 1 MiB input limit
is sufficient and follows the existing content-loader convention.

Use a small deterministic authoring script, `tools/generate_sorter_fixture.py`,
to produce the committed JSON once. It is not a runtime generator or solver.
Prepare 25 equations each in the forms `x + b = c`, `ax = c`, `ax + b = c`,
and `a(x + b) = c`, including subtraction and negative-coefficient variants.
Choose known integer solutions and nonzero coefficients, compute the matching
right side, reject duplicate display strings, and interleave the forms using a
fixed documented ordering. Format `- 3`, not `+ -3`; avoid redundant `1x`.
Equivalent equations or repeated solutions are allowed: the player chooses
their own grouping criteria.

Keep known solution witnesses in `tests/fixtures/equation_sorter_v1.solutions.json`,
keyed by equation ID. They are verification data, not inputs to the application.
Independently parse the four rendered forms in a test-only checker and evaluate
both sides at the witnesses using bounded integer arithmetic. A nonzero linear
coefficient establishes a unique solution. Check reproducible generation as
well as the mathematical validity of the rendered strings; do not merely reuse
the generator's calculation as its own proof.

`loadSorterContent(path)` decodes JSON and calls the model's shared structural
validator. It reports the source path and offending field on failure, before
creating the native host. There is no compiled fallback. Load once and freeze
content for the session. Default to the bundled file beside the executable;
an explicit `--content FILE` override supports edited fixtures and failure tests.

## C++ state and methods

`EquationSorterSession` is the canonical owner of assignment and interaction
state. It has no SDL, ImGui, JSON, filesystem, gallery or scoring dependency.

```cpp
using SorterEquationId = std::uint32_t;
enum class SorterBucket { A, B, C, D, Dump };
using SorterOwner = std::optional<SorterBucket>; // nullopt is Unsorted
// SorterView::inventory distinguishes grid and inventory presentation.

struct SorterEquation {
  SorterEquationId id;
  std::size_t homeIndex;
  std::string text;
};
struct OwnerChange {
  SorterEquationId equation;
  SorterOwner from, to;
};
struct SorterTransaction { std::vector<OwnerChange> changes; };
```

Freeze the 100 content records. Maintain exactly one owner entry per home index.
Resolve stable IDs through the frozen catalog; never interpret an ID as a slot
number. Keep active bucket, view kind, inspected ID, pending empty confirmation,
inventory slot IDs and Undo history as separate state. Slot IDs are presentation
reservations, not independent ownership lists. Derive membership and all six
counts from the owner array.

Use a closed `SorterActionKind` with SelectBucket, BackToGrid, ActivateEquation,
ClearInspection, RequestEmpty, ConfirmEmpty, CancelEmpty and Undo. The public
route is `dispatch(action)`; return accepted/changed/reason. Useful private
methods are `activateEquation`, `openInventory`, `commitTransaction`, `undo`
and `reconcileInventorySlots`. A shared `validateSorterContent` validates the
constructor and loaded content; `view()` returns a read-only frame snapshot.
Do not add a generic reducer framework, event bus or command registry service.

Stamp activation/confirmation commands with the presented interaction revision.
Increment it when interaction state changes. A stale command, missing ID,
invalid bucket, placeholder or equation unavailable in that view is rejected
without changing ownership, inspection or history. Tests and scripts obtain a
fresh revision for each intentional activation. Views do not mutate the model.

Commit validation checks every change's ID, current `from` owner, distinct ID
and legal `to` owner before applying any change. Prepare the transaction and
any allocation before the owner array changes. Bulk empty is a vector of
changes inside one history entry. Undo validates and applies the inverse of
the last entry, then removes that entry. Failure cannot leave a partial move
or a partially restored group. Do not copy the entire growing Undo history on
each action or frame.

The main view work is bounded by 100 equations. Counts and membership are
recomputed with one small scan. Stable-ID lookups and reserved-slot checks scan
at most 100 immutable records/slots; a bulk transaction has at most 100 changes.
There is no history-length scan or copy on each action. History grows with committed changes, never with
inspection, navigation or rendered frames; do not silently truncate Undo.

Required invariants after every accepted or rejected command:

- Exactly 100 unique equations and immutable home indices remain present.
- Each has exactly one owner; Unsorted plus the five bucket counts equals 100.
- Inventory view always has an active bucket; slot IDs are valid and unique.
- Every current inventory member has a slot, and a hole never adopts another ID.
- An inspected ID is available in the current view.
- Undo records only completed ownership transactions.

## Input and rendering route

The native host forwards SDL input to the existing ImGui backend. The sorter
UI translates widgets into `SorterAction` values; mouse, keyboard and semantic
scripts all reach `EquationSorterSession::dispatch`. Gameplay code does not
receive sorter commands.

Use one normal ImGui button activation per card, with its ID based on the
equation and view rather than its changing label. Do not also dispatch from raw
mouse-down, mouse-up, double-click or a second keyboard handler. Keep
`ImGuiItemFlags_ButtonRepeat` off. The pinned button implementation supports
fresh navigation activation without its optional repeat path. Enable keyboard
navigation; Tab/arrows move focus, and Enter/Space provide the same two ordinary
activations as the mouse. Holding either key must not inspect and then commit.
Escape clears/cancels within the sorter and never invokes gallery pause.

Draw from one snapshot and queue widget commands for the next frame boundary.
Take a fresh snapshot after dispatch so counts, cards, placeholders and inspected
state agree. On focus loss discard queued activations and clear inspection.
Ownership changes must not auto-focus and activate a neighbouring equation.
Use the same stable slot rectangle when the card becomes a placeholder.

The UI owns layout, scroll positions and keyboard focus only. It must not
mutate owners, build its own Undo records, judge equations or decide whether a
second activation can commit. UI-only background handling sends ClearInspection
once when the click was not consumed by a card or control.

## Files and build targets

These paths are implemented in this checkpoint.

| File | Responsibility |
| --- | --- |
| `src/runtime/sorter/EquationSorterSession.hpp` and `.cpp` | Content types/structural validation, canonical state, dispatcher, transactions, stable slots and view |
| `src/content/EquationSorterContentIO.hpp` and `.cpp` | JSON decode/file errors into validated sorter content; no UI or ownership mutation |
| `src/ui/EquationSorterUi.hpp` and `.cpp` | Shared native rendering/input adapter used by the executable and actual ImGui input tests |
| `app/sorter_main.cpp` | Small standalone launch, content selection, host lifecycle, frame-boundary dispatch and optional script/report/capture plumbing |
| `content/sorter/equations_v1.json` | Frozen 100-equation fixture |
| `tools/generate_sorter_fixture.py` and test witness JSON | Deterministic content authoring and independent verification inputs |
| `tests/equation_sorter_tests.cpp` | State, Undo, spatial-slot and ownership regressions |
| `tests/equation_sorter_content_tests.cpp` | Actual loader errors, fixture identity and independent rendered-equation checks |
| `tests/equation_sorter_input_tests.cpp` | Actual ImGui mouse/keyboard/focus/scroll behavior using the production UI |
| `tests/equation_sorter.script` | Repeatable native interaction/capture scenario |
| `CMakeLists.txt` | Additive target wiring and sorter-only content deployment |

Seven new C++ production files and one offline authoring script serve this
standalone capability. Existing gallery production files are unchanged by P020.

Target direction: `paths_sorter_model` is pure;
`paths_sorter_content` links it and uses the existing `paths_json` privately;
`paths_sorter_ui` links the model and the pinned ImGui; `sorter` composes
content/UI with `paths_native` and the existing SDL setup. Build model/content
tests with `PATHS_BUILD_NATIVE=OFF`; input/native targets remain under the
current native option. Add a sorter content deployment target copying only its
fixture. Do not widen the gallery content target or change its default startup.

## Acceptance coverage

The current graph provides `paths_sorter_tests`, `paths_sorter_content_tests`,
`paths_sorter_input_tests` and `sorter`. The following contract is covered
by the model, loader, actual ImGui input tests and native evidence described
below; it does not claim a human playtest.

| Boundary | Required observable result |
| --- | --- |
| Initial state | 100 original cells, no active bucket, zero bucket counts, Undo disabled |
| Two activations | First only inspects; second moves exactly one equation; a different first activation only changes inspection |
| No destination | Inspection works; repeated activations do not assign; choosing a bucket clears old inspection |
| Bucket/view navigation | Exact table behavior, no assignment or Undo entry, no unexpected compaction on reselecting an open inventory |
| Pointer safety | Rapid repeated clicks on a vacated cell cannot move a neighbour in either view |
| Keyboard/focus | Same two-stage behavior, visible focus, no move on focus alone, no held-key repeat commit, queued input discarded on focus loss |
| Reverse movement | Inventory second activation restores the immutable home, never another bucket |
| Cross-view Undo | Exact membership restored without navigating or changing the active destination |
| Frozen slots | Undo fills the old slot; restoration absent at inventory-open appends without shifting other cards |
| Bulk empty | Cancel changes nothing; Confirm returns the exact membership; one Undo restores it all |
| Dump/empty/exhausted | Dump behaves identically, empty inventories stay usable, no-op actions add no history |
| All assigned | Quiet state, no launch/modal/game over; return/Undo remain usable |
| Invariants | Mixed assignment/return/empty/Undo/navigation/stale-action sequences preserve every invariant after every step |
| Fixture | Exactly 100 distinct equations; deterministic generation; correct rendered signs and independently checked integer solutions |
| Loader | Missing/malformed/wrong-sized content and duplicate IDs/homes fail with source/field diagnostics before native startup |
| Layout | Toolbar stays visible, moves/Undo preserve scroll and hit rectangles, readable at 1440×900 and 800×600 plus a narrow one-column layout |

Use a headless ImGui context with real mouse/key event sequences and the actual
sorter UI, not only direct model commands. Assert rectangles and scroll offsets
before/after moves. Flush queued actions and render their resulting snapshot
before comparing reports with captures. Optional script commands should be a
small table forwarding `bucket`, `activate`, `grid`, `clear`, `empty`,
`confirm_empty`, `cancel_empty`, `undo` and `frame` to the same owners.

Use the existing host's offscreen capture support for screenshots. An explicit
local `--report` may write owner maps, counts, view, inspected ID, inventory slot
IDs, pending confirmation and Undo depth for verification; it is not analytics
and contains no solution witnesses. The browser-testing requirement in the
original handoff becomes native ImGui input tests and native captures here.
Do not launch a visible window unless the user asks. Report model tests, actual
input tests, visual inspection and interactive acceptance separately.

Completion means the player can select a bucket, inspect and store an equation,
inspect and return it, and recover individual or bulk moves while the board
keeps its spatial layout. A later gallery connection requires its own request
and specification; it is not part of this delivery.

## Build checkpoint: 2026-09-07

The executable is `build/gallery-port/paths_sorter`. It loads the deployed
`content/sorter/equations_v1.json` beside itself, including when launched from
another directory. `--content FILE` selects edited content without recompiling;
`--check-content` validates it and exits before creating the native host.

Verified:

- `paths_sorter_tests` and `paths_sorter_content_tests` in the native-disabled
  `build/sorter-model` configuration: passed. Includes 5,000 deterministic mixed
  actions checked against independent ownership snapshots, full 100-card empty
  and Undo, the absent-at-open restoration case, malformed/invalid files, and an
  independent interpretation of all 100 displayed equations at known solutions.
- `python3 tools/generate_sorter_fixture.py --check`: byte-for-byte reproducible
  authored pack and solution witnesses.
- `paths_sorter` and `paths_sorter_input_tests`: native build passed. Actual
  ImGui mouse, Enter/Space holds, Tab/arrows, Escape, focus-loss queue rejection,
  wheel scrolling, fixed rectangles and Undo passed at 1440x900, 800x600,
  360x640 and 360x480. Escape routing preserves card focus; focus is visibly
  distinct from inspection and the active destination.
- Native offscreen grid captures at 1440x900, 800x600 and 360x480, plus the
  scripted return/bulk-empty/Undo inventory: inspected. Reports agree with
  canonical counts, membership and reserved slots. The sandbox could not
  initialize Vulkan; the offscreen runs succeeded with approved host access.
- Startup from another working directory and missing/malformed/wrong-sized
  content failures before host creation: passed.

Evidence lives in `build/sorter-evidence/`: grid/small/narrow/inventory PNG and
JSON pairs, capture metadata, and `startup-and-reports.json`.

Production delta: seven new C++ files, +709 physical lines; one offline authoring
script, +55 lines; existing CMake wiring, +30 lines. No gallery production route
was replaced or changed. Nineteen prior modified/untracked files outside the
four intentionally updated documentation files were preserved byte-for-byte.

The runtime loader preserves subject-independent printable ASCII display text.
Tests exercise trig, calculus, linear algebra and discrete-maths notation, but
only the algebra pack has been authored and independently checked as content.
There is no runtime solver, grading of groups, saved-session format or rich
equation typesetter. Closing the app ends this session and its Undo history.
Human interaction feel is still untested; no visible window was launched.

Next candidate: player feedback on sorting, followed by a small checked set of
new subject examples and a decision on the notation each subject requires.

## Resumed checkpoint: 2026-09-07 — keyboard navigation and recovery

Resuming the existing prototype exposed a gap in the original input checks:
Tab could remain on a card after it became a placeholder or on Cancel after
the empty prompt closed. Focus inside the inventory also could not cross back
to its header controls with Tab. These failures were reproduced through actual
ImGui input before the repair; the earlier checks only established that some
navigation ID remained, not that Tab could leave it.

`EquationSorterUi` remains the sole input/presentation adapter. Its stable
grid/inventory child now shares the parent's navigation scope. If the focused
widget disappears or becomes disabled, focus returns to the persistent active
bucket (A before a destination is selected). The pinned ImGui navigation API
updates that focus directly, without activation or a scroll request. This also
keeps an exhausted Undo button from stranding navigation. Session state,
transactions, loading and equation content are unchanged.

Verification for this repair:

- `paths_sorter_input_tests` and `paths_sorter` rebuilt successfully. The
  targeted input test passed, retaining the original four viewport sizes and
  adding Tab recovery after a moved card and exhausted Undo, held Enter/Space
  after a commit, and navigation from a live inventory card to its controls.
- The actual UI sequence at 1440x900, 800x600 and 360x480 now covers returning
  to a scrolled grid, Empty/Cancel, Escape cancellation, Confirm and one-click
  bulk Undo. Membership, frozen rectangles and scroll offsets are asserted.
- The native 800x600 offscreen inventory capture was inspected. Its report
  matches the original scripted ownership, inspection, history and slot order:
  98 Unsorted, two in A, with the returned equation's reserved hole intact.
  As before, Vulkan required approved host access after sandbox initialization
  failed. No visible window was opened.
- The existing passing model/content evidence is retained; those unchanged
  owners were not retested for this UI-only repair. Pre-resume hashes verify
  that all other production files, content and earlier gallery work survived.

Production delta against the resumed working state: **+19/-1 lines, net +18**
in one existing C++ file; zero production files added. The existing input test
adds **+75/-4 lines, net +71**. Evidence, before/after input logs, the capture
and preservation receipt are in `build/sorter-evidence/resumed/`.

P020 remains uncommitted and human pointer/swapchain acceptance remains open.
The next step is player feedback on the existing sorter prototype.
