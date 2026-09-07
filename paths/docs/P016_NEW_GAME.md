# P016: New game

Status: automated checks passed; offscreen screens reviewed; uncommitted.
Baseline: `6a9f1c8`, the movement setup and content organization commit.

## Player flow

For a started set/practice type, Choose questions offers Resume and New game.
Resume continues the original session. New game opens a draft seeded from that
selected game's current movement, speed, seed and colour-assignment setting.
Movement and speed are editable with the existing validated controls.

The screen identifies the set and practice type and states that starting
replaces its current progress and answer history. Other games are kept.
Cancel, or Esc outside an active widget/popup, discards the draft and preserves
the original run. Start new game is the explicit replacement action; the menu's
ordinary Enter-to-Play shortcut is inactive in this setup.

Starting reloads the selected pack and starts at its first deck question.
Targets respawn, counters and attempts start empty, and the existing seed is
retained. It does not promise a different answer permutation for the same seed.
The old session is released only after the fresh session has been prepared.
There is no retained history of replaced games or disk persistence in this
checkpoint. A successfully started replacement promotes its settings to the
ordinary next-game draft; Cancel does not change those defaults.

If the file is missing, malformed or lacks the selected deck, the old run
remains paused and intact. The setup remains editable. The user can fix the
file and retry Start new game, or Cancel and Resume the original frozen
content. Pack/type selection, workshop navigation, repeated New game and
ordinary Play/direct launch cannot bypass an open replacement setup.

## Ownership and files

- `src/ui/GalleryMenu.hpp`: three new semantic actions, replacement draft and
  its observable setup state.
- `src/ui/GalleryMenu.cpp`: begin/cancel/confirm transitions and shared `start()`
  preparation used by initial launch and replacement. Loading, deck resolution
  and construction complete before the selected slot changes.
- `app/gallery_main.cpp`: New game and its setup controls, script parsing and
  report state. Actions remain queued and applied between native frames.
- `tests/gallery_menu_tests.cpp` and `tests/gallery_new_game.script`: focused
  model and native scenarios.
- `README.md`, `docs/ARCHITECTURE.md`, `docs/WORKSTREAMS.md` and this record:
  usage and checkpoint documentation.

The prior unconditional reuse of a started slot now has an explicit confirmed
replacement route through the same construction boundary. No alternate loader,
question judgment, route calculation or renderer path was added.

Production C++: **+75/-29 lines, net +46**, three existing files and zero new
production files. C++ tests: **+90/-1 lines, net +89**. Content and the underlying
runtime/scene/platform owners are unchanged. No unresolved contract decisions.

## Verification

The current CMake graph selected the pure menu test and native gallery build:

```sh
cmake --build build/question-content --target paths_gallery_menu_tests -j 4
ctest --test-dir build/question-content -R '^paths_gallery_menu_tests$' --output-on-failure
cmake --build build/gallery-port --target paths_gallery -j 4
```

The focused tests passed. New coverage exercises cancelled edits, unchanged
default settings after Cancel, resumed partial collection and wrong attempts,
invalid draft settings, blocked navigation/confirmation bypasses, fresh target
spawning and judged shots, other pack/type isolation, and resetting a later
deck question with completed/archived history back to the first question.
Missing, malformed and incompatible pack failures preserve an unfinished pop,
its clock and both correct/wrong attempts. Repair and retry load fresh content.

The native script uses `new_game`, `cancel_new_game` and `start_new_game` through
the same dispatcher as the widgets:

```sh
./build/gallery-port/paths_gallery --offscreen --seed 19 --motion stationary \
  --script tests/gallery_new_game.script \
  --capture build/new-game-evidence/restart.png \
  --report build/new-game-evidence/restart.json
```

Six native scenarios passed: setup at 1440×900 and 800×600, the small Resume
menu, cancellation back to the original game, confirmed replacement while
another game's wrong-answer record survives, and the previous three-pack
progression scenario. Reports distinguish the replacement draft from retained
session settings using `new_game_setup` and `settings_locked`. JSON assertions
passed; the small setup, small Resume and fresh answer board were reviewed.
Controls and replacement notice fit at 800×600.

Evidence is under `build/new-game-evidence/`, including `verification.json`,
scripts, logs, reports and captures. No visible window was launched. Interactive
pointer feel and swapchain acceptance remain separate from offscreen results.
