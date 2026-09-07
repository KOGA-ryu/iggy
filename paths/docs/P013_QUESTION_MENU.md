# P013: Choose questions

Status: automated checks passed; native offscreen screens reviewed; uncommitted.

Run `./build/gallery-port/paths_gallery` to open the new menu. Choose one of
the three bundled sets and press Play:

- **Starter questions**: choose among the available Equality Sweep, Question
  Relay, Equation Chain and Substitution Chain decks.
- **Three points, one formula**: the 13-decision quadratic adaptation.
- **The closest point on a line**: the 14-decision guided derivation.

Click a title to select it, or use Up/Down and Enter. While playing, Pause / Esc
reveals Choose questions. Returning pauses that game. Play becomes Resume for
a started set/practice type, preserving its working, collected answers, attempts,
completed runs and any unfinished pop. Each pack remembers its last selected
practice type. Progress lasts until the process closes; there is no disk save.

Developer workshop opens the existing editor. Returning to the menu preserves
its objects and routes. The explicit `--start-mode workshop` and existing four
gameplay startup arguments remain available. An explicit custom `--content-pack`
continues to work and gets an additional Custom questions entry when its path
differs from the bundled files. The menu offers known sets; folder browsing or
automatic pack discovery is outside this checkpoint.

## Ownership and failure behavior

`src/ui/GalleryMenu.hpp/.cpp` owns app navigation and the lifetime of started
sessions. The descriptor list provides menu names and bundled filenames; actual
practice availability is read from the validated pack's decks. The original
question files and their schema are unchanged.

`GalleryMenu::launch()` is used by both direct CLI startup and the Play action.
A new game is fully loaded and prepared before it is installed as active. A
started game is resumed without replacing its frozen content. Sessions are
retained by pack and practice type, while their existing GallerySession and
LayeredQuestionSession own every gameplay decision and record. No answer key,
progression rule or scoring calculation is introduced in the menu.

Missing or malformed content stays on the menu with its source/field diagnostic
and a Try loading again control. Repairing the file allows another attempt.
Other sets and the workshop remain available; a failed load does not replace
the previous game. Direct CLI gameplay retains its original pre-graphics
startup failure behavior for invalid packs or missing decks.

The startup/UI owns a pending navigation action. It applies pointer/keyboard
navigation between native frames; scripted navigation uses the same dispatcher
at that boundary. The scene pointer supplied to NativeVulkanHost remains stable
through the entire frame. The old unconditional game/workshop startup choice
and standalone game-construction block have been replaced. The host, question
model and target motion code are unchanged.

The new pure `paths_gallery_menu` target depends on `paths_gallery_model` and
the existing content loader. It has no SDL, ImGui or Vulkan dependency. Rendering
and input widgets remain in `app/gallery_main.cpp`.

## Verification

The affected targets came from the current CMake graph:

```sh
cmake -S . -B build/question-content -DPATHS_BUILD_NATIVE=OFF
cmake --build build/question-content --target paths_gallery_menu_tests paths_gallery_tests -j 4
ctest --test-dir build/question-content -R '^paths_gallery(_menu)?_tests$' --output-on-failure
cmake --build build/gallery-port --target paths_gallery -j 4
ctest --test-dir build/gallery-port -R '^paths_content_startup_tests$' --output-on-failure
```

All passed. The menu tests cover named selections, actual deck availability,
separate starter modes, partial collection, wrong answers, paused clocks,
session identity on resume, failed-load preservation, repaired-file retry,
frozen content, custom packs and the workshop without content. The existing
gallery target covers the source-card sequences and target/working boundary.
The startup target retains all seven original content-error cases.

The native scenario can be repeated with:

```sh
./build/gallery-port/paths_gallery --seed 19 --motion stationary --offscreen \
  --script tests/gallery_menu.script --capture build/menu-evidence/switch-resume.png \
  --report build/menu-evidence/switch-resume.json
```

Menu script commands are `choose_pack N`, `choose_mode N`, `play`, `questions`
and `workshop`. Indices are zero-based menu rows, not question or answer IDs.
Existing gameplay/workshop commands keep their semantics. Older workshop
scripts should now specify `--start-mode workshop` because default startup is
the question menu. Reports retain the existing active-game record and add a
menu snapshot with summaries of all retained sessions.

Ten bounded native scenarios passed. The full switching scenario completes the
starter with two correct and one wrong, then leaves both source cards at step
2 with one correct and one wrong each. Only the visible game is unpaused.
The partial starter collection and the quadratic's in-flight pop survive menu
switches. A newly added workshop sphere survives leaving and reopening it.
The previous direct card-013 script still finishes with 14 correct, 14 wrong,
zero misses and a clean endless restart. Existing workshop actions also pass.

Menu captures were reviewed at 1440×900 and 800×600, along with Resume, the
resumed answer board, the workshop return and a missing-content error screen.
All menu choices and controls fit at both sizes. Evidence is under
`build/menu-evidence/`, including `verification.json`, scripts, reports and
screenshots. MoltenVK used the established unsandboxed offscreen route. No
visible window was opened; human pointer feel and interactive swapchain
acceptance remain separate.

## Changed files and checkpoint

- New `src/ui/GalleryMenu.hpp` and `src/ui/GalleryMenu.cpp`.
- Updated `app/gallery_main.cpp` and `CMakeLists.txt`.
- New `tests/gallery_menu_tests.cpp` and `tests/gallery_menu.script`.
- Updated `README.md`, `docs/ARCHITECTURE.md`, `docs/WORKSTREAMS.md`, and this new record.

Production C++: **+292/-40 lines, net +252**, two new files and one changed
startup. Tests add 126 C++ lines. The content loader, authored packs, gameplay
owners and renderer remain unchanged. No contract decisions remain unresolved.
Changes are uncommitted; unrelated work is preserved. The next candidate is
movement-pattern and speed controls in the menu, using the existing route and
configuration owners. Persistence and scoring remain separate workstreams.
