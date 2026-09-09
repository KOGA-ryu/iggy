# Drive the velocity graph

**Status: shelved at the user's request.** The user tried the prototype and saw
the direction, but wants time to consider what it needs. Further Motion work
is deferred while regular symbolic questions continue. The implementation and
saves remain available; the original build evidence below is retained without
claiming full visual acceptance. No manual review is requested now.

Open **Contents → Motion** in `sorter`. This checkpoint supplies one playable
question for each of the first five chapters. All five are available directly;
**Next** advances one chapter only after the current problem is complete.

| Chapter | Problem | Example working plan |
| --- | --- | --- |
| 1. Where am I? | Move from position 2 m to position 8 m | Set finish position to 8; displacement is 6 m |
| 2. Moving at a steady pace | Start at 0 m and reach 8 m in 4 s | Constant velocity 2 m/s |
| 3. Building a journey | Start at 0 m; pass 2 m at 2 s; reach 8 m at 4 s | 1 m/s for 2 s, then 3 m/s for 2 s |
| 4. Going out and coming back | Start at 0 m; reach 6 m at 2 s; return at 4 s; travel 12 m | 3 m/s for 2 s, then -3 m/s for 2 s |
| 5. Accelerating and stopping | Travel 12 m in 4 s, starting and finishing at rest | Velocity 0 → 6 → 0 m/s, with the peak at 2 s |

The last question also accepts a peak at 1, 1.5, 2.5 or 3 seconds. All those
triangles have area 12 m. The acceleration and braking slopes change. The game
judges the stated physical constraints rather than matching a secret plan.

## Playing in the persistent workspace

The gold question remains above the cart, graph and controls. Drag the cyan
graph handles or adjust the compact sliders. For the last question, dragging
the peak horizontally changes its time and dragging vertically changes its
velocity. Values snap to the chapter's half-unit steps. Equation typing is
disabled. **Choose a symbolic plan**, in the Working area, supplies four
typeset tiles that set exactly the same parameters.

**Run** records a complete journey. **Pause / Resume**, **|<** (rewind), the
timeline slider and playback speeds 0.25x, 0.5x, 1x and 2x share one clock.
Scrubbing is a preview and cancels the in-flight attempt; use Run to record a
new complete journey. Replaying a completed or inspected run adds no duplicate
evidence. The violet cart and curve compare the previous different plan; both
are sampled at the same time as the cyan cart.

The Working area shows typeset relationships, signed areas, displacement,
distance and acceleration. Orange checks identify missed goals; green checks
mark success. **Symbols and meaning** explains notation in place. **Run history**
opens earlier working without replacing the current plan; **My plan** returns
to it. A completed result stays visible until an explicit chapter selection,
Next or Again. Again retains completed runs. Undo coalesces an entire drag into
one change and retains all recorded attempts.

At 1440×860 and 800×600, working occupies the right side. At 360×480, it scrolls
in the space below the graph controls. The question, cart, graph and transport
remain in the same workspace. The native cart is actual SceneFrame geometry;
the graph and typeset working use the existing ImGui/Vulkan presentation path.

## Mathematical and persistence ownership

`MotionLesson` owns the five authored specifications, allowed parameters,
analytic piecewise motion, signed displacement, absolute distance, goal checks,
playback and immutable completed-attempt records. `MotionScene` places geometry
at those sampled coordinates. `MotionLessonUi` collects semantic actions and
presents the same facts; choosing a tile does not introduce another checker.

This checkpoint models **kinematics**. Chapters 2–4 have idealised instantaneous
velocity changes; chapter 5 has finite linear velocity ramps. Acceleration is
reported as undefined at a velocity jump or a corner with unequal slopes.
The first chapter uses a one-second position animation only to illustrate a
change of coordinate. Observation ending in chapters 2–4 does not imply the
cart's final velocity became zero. Forces, mass, friction and collisions are
outside these first five chapters.

The model accepts at most two segments, two parameters, 256 recorded attempts
and 64 recent Undo changes per chapter. Sampling is constant time; scene
preparation and UI work have fixed or explicitly bounded loops. Reaching the
attempt limit stops further recording without discarding history. Nonfinite
inputs and malformed restored state are rejected before mutation.

`MotionProgressFile` stores `motion-lessons-v1.json` next to the normal practice
save. Its five versioned chapter IDs survive row reordering. A chapter whose
mathematics changes must receive a new versioned ID. Restoration checks every
plan and recomputes every result through `MotionLesson`; it does not trust a
saved correctness flag. In-flight runs reopen paused and Resume completes the
same attempt. Malformed, incompatible or externally changed saves retain the
original bytes. This is a bounded local single-writer save, not cloud sync.
`--no-progress`, scripts and bounded runs retain their no-implicit-save behavior.

The existing practice catalogue, 278 Library starters, source corpus, question
owner and save formats are unchanged. No changes are made to the separate
textbook or mathematical-object workstream. The native host and renderer retain
their existing ownership. There was no prior motion-lesson route to remove.

## Build and verification

```sh
cd /Users/kogaryu/iggy3d/paths
cmake -S . -B b -DCMAKE_BUILD_TYPE=Release
cmake --build b --target sorter -j 6
./b/sorter
```

Release sorter builds. The targeted gate consists of `paths_motion_tests`,
`paths_motion_ui_tests` and the existing `paths_sorter_input_tests`. It checks:

- Six complete model routes, including a second valid stopping plan.
- Independent integration of all 1,021 allowed plans, both distance and displacement.
- Twelve combinations of playback speed and frame rate, plus pause and scrub boundaries.
- In-flight save/resume, completion, failed attempts, Undo, row reordering,
  malformed data and preservation of externally edited saves.
- 135 native scene samples with finite geometry, bounded buffers and the cart
  at the model's coordinate inside its viewport.
- Fifteen actual graph-input routes and five symbolic-tile routes in headless
  ImGui frames, plus explicit Next, replay, live resizing and Library return.
- The surviving practice input, saved Resume, notation, reference and solving routes.

`sorter --check-content` still validates 100 sortable cards, 930 Library entries
and 278 starters before creating a native host. The evidence record is
`build/motion-lessons-evidence/verification.json`.

No windows, screenshots, screen captures or rendered image artifacts were used.
Headless UI verification acknowledges font atlases in memory and inspects draw
data. **Human visual acceptance remains pending.** Changes remain uncommitted.

For a short manual check: choose cyan **Motion** in Contents. Keep the gold
question in view while dragging the cyan plan and running the cart. A second
different run should show a violet comparison. In chapter 4, use 3 and -3 m/s
and look for displacement 0 m but distance 12 m. In chapter 5, use peak velocity
6 m/s and peak time 2 s: the green result should remain until Next/Again or a
chapter selection. Pause a run, close, reopen Motion and Resume once.

If the user returns to this design, a possible chapter 6 would connect curved
velocity profiles, numerical area approximations and the same cart playback.
It is not an active implementation commitment.
