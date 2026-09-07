# P014: Movement setup before Play

Status: automated checks passed; offscreen screens reviewed; committed with P015.
Baseline: local commit `2308995`, which records the standalone foundation
through P013. No push was made.

## Controls and contract

The question menu now provides Movement and Speed before Play. The thirteen
gameplay choices use the existing route descriptors: Stationary, Horizontal
shuttle, Vertical lift, Diagonal rebound, Circle, Ellipse, Figure eight, Sine
wave, Zigzag, Box patrol, Stop and go, Breathing spiral and Seeded roam.
Custom waypoint authoring remains in Developer workshop.

The logarithmic speed slider covers 0.01–100 m/s. This is the menu's editing
range; the engine and CLI retain their existing finite, positive, at-most-100
contract, including smaller positive values. The engine interprets speed as
nominal pace on curved routes, where instantaneous speed can vary. Stationary
targets do not move, although their stored speed still must be valid.

CLI `--motion`, `--pace` and `--seed` initialize the pending setup. A new game
copies that setup when Play succeeds. A started pack/practice type retains
its original settings, movement state, attempts and progress. Selecting it
shows Resume and disables movement editing. The dispatcher rejects attempted
edits too, with an explanation. Switching to an unstarted pack/type restores
the latest next-game draft; inspecting or resuming an older game does not
overwrite it. Progress and settings last until process exit.

There is no restart/new-game history control in this checkpoint. To use another
setup, choose an unstarted set/type or relaunch the app. A deliberate New game
action is the next candidate. Scoring, persistence and saved profiles remain
separate specifications.

## Ownership

`GalleryMenu` owns pending setup, selection and session lifetime. `GallerySession`
owns each game's immutable configuration. `validateGalleryConfig()` is shared
by menu edits and game construction, with numeric/route checks delegated to
the existing TargetMotion preparation function. The former constructor-only
preset check is replaced by this shared boundary. There is no second UI speed
validator, route calculator, answer key or progression path.

`gallery_main.cpp` presents controls and forwards semantic actions between
frames. Keyboard pack shortcuts do not fire while a widget is active. Reports
add selected and retained-session movement/pace values from those owners,
plus the selected setup's locked state. The existing active-game report stays
compatible. Question content, answer assignments, judging, attempts, route
calculations and the renderer are unchanged.

## Verification

Affected targets were derived from the current CMake graph. Both pure targets
and the native executable built successfully:

```sh
cmake --build build/question-content --target paths_gallery_menu_tests paths_gallery_tests -j 4
ctest --test-dir build/question-content -R '^paths_gallery(_menu)?_tests$' --output-on-failure
cmake --build build/gallery-port --target paths_gallery -j 4
```

The menu tests cover all thirteen presets reaching actual moving scenes;
stationary targets remaining still; correct and wrong clicks on every preset;
double speed producing double displacement before a turn; unchanged seeded
answer assignment; invalid, nonfinite and boundary speeds; invalid/custom
routes; immutable Resume setup; and preserved position, clock, challenge,
partial collection and attempts. The existing gallery integration regressions
also passed. After improving the locked-edit diagnostic, the menu target was
rebuilt and passed again, and the native rejection check confirmed the text.

The repeatable native scenario is:

```sh
./build/gallery-port/paths_gallery --offscreen --seed 19 \
  --script tests/gallery_menu_motion.script \
  --capture build/menu-motion-evidence/resume.png \
  --report build/menu-motion-evidence/resume.json
```

New script commands are `choose_motion ROUTE_ID` and `set_pace NUMBER`. Route
IDs come from the existing descriptors, for example `figure_eight`, `circle`
and `stationary`. They dispatch the same menu actions as the widgets.

Eleven successful native scenarios cover both menu sizes (1440×900 and
800×600), pending selection, moving gameplay, two differently configured
games with Resume, CLI defaults, direct source-card startup, measured speed
differences and the previous three-pack progress scenario. Two additional
negative scenarios deliberately reject zero speed and editing a resumed game.
JSON assertions passed for settings, positions, answer assignments and retained
progress. Screens reviewed include the small menu, pending selection, small
Resume and the moving answer board. All standard controls fit at 800×600.

Evidence is in `build/menu-motion-evidence/verification.json`, with scripts,
reports, logs and captures beside it. No visible window was opened. Offscreen
evidence does not establish interactive pointer feel or swapchain acceptance.

## Changed files

- `app/gallery_main.cpp`
- `src/ui/GalleryMenu.hpp` and `src/ui/GalleryMenu.cpp`
- `src/runtime/gallery/GallerySession.hpp` and `GallerySession.cpp`
- `tests/gallery_menu_tests.cpp` and new `tests/gallery_menu_motion.script`
- `README.md`, `docs/ARCHITECTURE.md`, `docs/WORKSTREAMS.md` and this record

Production C++: **+89/-21 lines, net +68**, five existing files changed and
no new production files. C++ tests: **+102/-2 lines, net +100**. This is the
requested movement-controls feature, not an ownership cleanup. Unrelated
changes are preserved. No contract decisions remain unresolved for P014.
