# Playtest and Simulation Testing

## Automated Gate

```bash
cmake --build build --target \
  creative_play_preparation_tests \
  creative_playtest_launch_tests \
  playtest_lifecycle_tests \
  playtest_process_owner_tests \
  i3dp_headless_smoke
ctest --test-dir build \
  -R '^(creative_play_preparation_tests|creative_playtest_launch_tests|playtest_lifecycle_tests|playtest_process_owner_tests|i3dp_headless_smoke)$' \
  --output-on-failure
```

## Manual Acceptance

The current playtest command and canonical save contract need one dedicated
product brief before a manual case can be considered reliable.

| Test ID | Work ID | Scenario | Command | Status | Last Verified |
| --- | --- | --- | --- | --- | --- |
| PLY-MAN-001 | PLY-001 | Launch the canonical authored map in the separate playtest executable and return without changing the editor document | Command contract pending | Blocked | Not yet |
| FM-MAN-001 | FM-001 | Complete untimed Hunt with keyboard and pointer, review an error, finish, and restart at ordinary and enlarged text sizes | `./build/first_move` | Pending | Not yet |

### Native First Move Hunt

Build `first_move`, `first_move_hunt_tests`, and `first_move_input_tests`.
The model and input tests cover initial judgments, banked scoring, retained
mistakes, review/release, repeat suppression, focus loss, and restart history.
`first_move_capture_smoke` exercises the actual native Vulkan/ImGui path;
the Rendering and Preview department owns its capture boundary.

Launch from the repository with `./build/first_move`. Arrow keys move, Space
marks, Enter commits, C clears ready rows, E opens error review, Escape closes
review, X releases an explained row, and R restarts after completion. Pointer
controls dispatch the same game commands.

This slice contains one fixed 24-example linearity pack and in-process run
history. An explicit `--report PATH` exports that history; automatic durable
study history, corpus integration, Cascade, Lab, and a Creative menu launcher
remain later capabilities. Repeated runs are identified as practice on
previously exposed material.

FM-001 automated checkpoint (2026-09-06 UTC): all nine saved gameplay
scenarios are green. The final independent gate rebuilt Hunt and its two test
targets, passed both model/input tests, and passed the bounded empty-frame,
Creative-capture, and First-Move-capture gate with Metal/MoltenVK access.

FM001-R1 repaired the enlarged-text keyboard access failure reproduced during
review. Its in-memory ImGui regression drives the real SDL input queue and UI
render function at 1024x768 / 150% text. Up, Down, Page Up, Page Down, Home,
and End reach complete content-sized explanation cards without nested
scrolling; focus loss, repeat suppression, close/reopen, viewport changes, and
immutable first-attempt evidence remain covered.

Review and bounded repair packet:

- [Independent review](/Users/kogaryu/.codex/visualizations/2026/09/06/01a074b3-1ae9-7a40-8c9a-71f86d8fecb0/first-move/FM001_REVIEW.md)
- [FM001-R1 packet](/Users/kogaryu/.codex/visualizations/2026/09/06/01a074b3-1ae9-7a40-8c9a-71f86d8fecb0/first-move/FM001_R1_REVIEW_ACCESS_PACKET.md)
- [Final automated review](/Users/kogaryu/.codex/visualizations/2026/09/06/01a074ae-914f-7b23-bbc4-abaf7d14d533/first-move/FM001_FINAL_REVIEW.md)

The input test privately creates an ImGui context to verify complete
explanation visibility through the real render function. No window or GPU is
required for that regression. FM-MAN-001 remains pending; capture inspection
is not a human play session or user acceptance.
