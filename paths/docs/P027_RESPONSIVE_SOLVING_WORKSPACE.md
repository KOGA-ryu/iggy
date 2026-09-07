# P027: fit the solving workspace to the window

Status: implemented; Release build and targeted input checks passed;
uncommitted. User visual confirmation is pending. No screenshots or native
windows were used for verification.

The user confirmed that P026 made the question easier to see, but estimated
that its content occupied about 1000×360 inside a 1440×860 window, leaving
roughly 470 pixels of empty space below. The fixed 200-pixel activity cap and
fixed text/button sizes caused that mismatch.

## Delivered behaviour and owner

`EquationSorterUi::solveLayout` remains the sole presentation-layout owner.
It now fits and vertically centres the main workspace within the available
window, growing the problem, working, prompt, symbols and activity together.
The former activity-height cap and fixed compact width are removed. The two
rows of operation choices use the activity height, and the scene receives that
same viewport through the existing `GalleryViewport` route. Narrow windows
retain readable text and place support below the activity.

The original problem remains gold, current working has a mint heading, and
operation choices retain teal borders. Controls remain above the maths;
support aligns with the main workspace. Positions depend on window dimensions,
so steps, hints, completion and Next cannot rearrange the workspace.

Question content, judging, help, history, scene framing and input ownership are
unchanged. No new production file or competing action route is introduced.

## Verification

The current CMake graph supplies the affected `paths_sorter_ui` library,
`sorter` executable and `paths_sorter_input_tests`. The Release build in `b`
succeeded, and the targeted input suite passed. No broader gate was needed.

Actual ImGui geometry and input checks verify live resizing through 1440×860,
800×600, 360×480 and 1920×1080, then back to the original size. All four choices
remain inside the activity; resizing preserves the current question and makes
no attempt. The existing full operation/sphere/help/replay/Next/resume checks
also pass, now including the user's estimated 1440×860 size for sphere clicks.
Proximity is checked relative to the working size as the content grows.

At 1440×860, the measured main workspace starts at y=102 and is 693 pixels
high. The gold board is 110 pixels high, the activity is 410 pixels high, and
the last operation choice ends near y=781, leaving about 79 pixels below it.
These are automated layout measurements, not a visual acceptance claim.

Production delta: one existing UI file **+32/-26 (net +6)**; no new production
files. The existing input test changes by **+35/-4 (net +31)**. Source and
binary hashes, preserved-file checks, scoped diffs and the passing test log
are under `build/responsive-workspace-evidence/`.

## User visual check and next candidate

Relaunch Sorter with the bracket practice pack. The gold question board, mint
working and teal choices should grow together and occupy much more of the
window height. Resize once to check that they remain readable and grouped;
advance to the coloured spheres to check their size and placement.

The remaining risk is visual proportion and spacing, which the user reviews.
Subject/chapter selection remains the next capability once this formatting
feedback is resolved.
