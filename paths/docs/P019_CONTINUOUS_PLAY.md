# P019: continuous arcade play

Status: automated checks passed; offscreen screens reviewed; uncommitted.
Baseline: the completed P018 working state, saved under
`build/continuous-play-evidence/baseline/` with `baseline.json` hashes.

## The loop

The user clarified that the game should support continuous clicking, with a
small indicator for mistakes and results only after stopping or a future timer
expires. They explicitly chose to keep the current question active after a
wrong click. The proposed missed-question retry round after P018 is superseded.

1. Read the prompt and match an answer's colour to a target.
2. Click. A right answer pops; a wrong answer briefly displays **Wrong**.
3. Keep shooting. Wrong answers leave the question, remaining targets and
   collected answers intact. Targets keep moving and input stays available.
4. Collect the required correct answer or answers. The existing pop/spawn
   mechanics automatically move to the next step or question.
5. Continue indefinitely. No mistake opens a dialog, requires acknowledgement,
   triggers review or ends the run.
6. Choose **Stop / Esc** to pause and see the accumulated totals. **Resume**
   continues the same game and hides the totals again.

Correct and aiming-miss signals are also brief. Only the most recent accepted
shot's signal is shown, for up to half a second of active play. A new challenge
clears it. A miss remains separate from a wrong mathematical choice.

Detailed Answer review and its explanations remain optional while stopped.
They are never part of the automatic loop. Correct/wrong/miss/question totals
still come from existing evidence; this checkpoint does not invent a point
formula. The current mode is explicitly **Endless**. A whole-run countdown has
not been implemented; future time expiry may stop the run, mistakes may not.

Stop is the existing pause operation, not a destructive reset or a new terminal
run state. New game retains its explicit replacement behavior. The UI's Quit
control is offered while stopped, after the player can see the totals.

## Ownership and code

- `src/runtime/gallery/GallerySession.hpp` adds `GalleryFeedback` with None,
  Correct, Incorrect and Miss, replacing persistent prose in the gallery view.
- `GallerySession.cpp::submitHit()` derives the signal from the existing
  question verdict or geometry miss, records its expiry and returns normally.
  Wrong clicks do not create a transition or change pause state. The expiry
  uses the scene's existing 60 Hz clock; `view()` is read-only. Stop freezes
  that clock and Resume continues it. No timer blocks the next shot.
- `app/gallery_main.cpp` maps the signal to a small coloured label in the
  play-area header. Totals render only while paused. Stop/Resume forwards the
  existing `GalleryPause` action; no second navigation or scoring owner exists.
  Native reports retain internal totals and add the short feedback value.

Persistent coaching strings, the always-visible counters and per-question
active-time display are removed. Its now-unused `elapsedTicks` and
`challengeStartTick_` fields are removed with it. Question content, collection,
working transitions, colour assignment and attempt records retain their owners.

Production C++: **+38/-20 lines, net +18**, three existing files and zero new
production files against the saved baseline. Tests add 37 lines in the existing
gallery test and `tests/gallery_continuous_play.script`. Changes remain
uncommitted; P016–P018 and unrelated changes are preserved.

## Verification

The current CMake graph selected the affected gallery model test and native
executable:

```sh
cmake --build build/question-content --target paths_gallery_tests -j 4
ctest --test-dir build/question-content -R '^paths_gallery_tests$' --output-on-failure
cmake --build build/gallery-port --target paths_gallery -j 4
```

The focused test passed. It fires repeated wrong answers into moving targets,
checks the same question remains active, immediately follows with a correct
shot, and observes automatic progression without a correction phase. It also
checks half-second expiry, Stop/Resume preserving records and clock, aiming
misses remaining distinct, and new shots replacing the previous signal. The
existing collection, stale-input, source-card and endless-progression tests
remain green in the same target.

Five native offscreen cases exited zero: a visible wrong signal during play,
the same question after that signal expires, two wrong clicks followed by two
correct hits and Stop, Resume with identical evidence, and stopped results at
800×600. The active 1440×900 screen has no live counters or correction message;
the 800×600 stopped screen shows the totals. Both were visually reviewed.

Evidence is in `build/continuous-play-evidence/`: binary hash, scripts, reports,
captures and logs. `verify.py` checks saved results and writes
`verification.json`. It verifies feedback expiry without changing attempts and
identical targets, clock and question records across Stop/Resume. No visible
window was opened; human pointer feel and interactive swapchain acceptance
remain separate.

## Immediate priority: a small, sustained loop

The user's next direction is to keep the build as simple as possible: answer
questions quickly, keep clicking for long sessions, then adapt different levels
of maths to this same system and begin player testing. The earlier suggestion
to add a countdown next is deferred.

The immediate check is repeated-input responsiveness, automatic question
changes and how accumulated attempts/history affect long sessions. The bounded
P019 tests above do not establish long-session performance. Improvements should
address observed friction in this loop before adding another gameplay system.

After that, prepare small maths packs through the existing prompt, choices,
accepted answer IDs and working-state format. Start with arithmetic and order
of operations, then multi-step algebra, using the existing more advanced cards
as examples of what the same format can express. The user will playtest this
loop with those packs. New visuals are not a prerequisite for that test.

## Later idea: make the chosen operation visible

The player could choose the appropriate next operation, then see its effect on
the equation. For example, `3 + 4 * 2` becomes `3 + 8` after choosing the
multiplication, then `11` after choosing the addition. The selected subexpression
could highlight and collapse into its result. Algebra could similarly show an
operation applied to both sides of an equation.

As the player progresses through a problem, the equation display and effects
could become richer, ending with a completion effect. These visuals should make
the accepted mathematical change easy to see and preserve readable prompts and
targets. An effect must not block input, pause play or delay the next question.

This is a deferred presentation idea, not a new solver or mandatory puzzle
stage. A future implementation should observe the question owner's committed
step and prepared before/after working states; it must not independently judge
the operation or force a single universal solving order. No effect system,
animation metadata or additional gameplay code is introduced by this note.
