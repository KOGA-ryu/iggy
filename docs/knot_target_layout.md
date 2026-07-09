# Knot Lane — Target Layout (the planner-owned post-state)

Companion to the KNOT LANE in `refactor_targets.md`. This is the answer to "what should it actually look
like when we're done" — the target folders/files, what each owns, and where the arrow points the OTHER way
(2–10 fragments → 1 file). Measured at HEAD 2026-07-09. **The builder lands INTO this layout; deviations are
STOP-and-report, not improvisation.**

---

## 0. THE FILE BAND LAW (both diseases, one rule)

A `.cpp` **earns existence** only by one of:
1. Owning one *nameable concern* of roughly **150–700 lines**;
2. Being a **lane/department boundary** (crossing it changes owner);
3. Being a **pure kernel that needs direct test access**;
4. **Isolating a heavy dependency** (e.g. Vulkan headers) from everyone else.

A file **loses existence** when: under ~100 lines AND single includer AND no direct tests → fold it into the
concern file that calls it. Headers: public header only for externally-consumed API; internal helpers get
**one cluster header per folder-concern**, never one header per micro-TU.

Bands (reported by K4's dep tool at every milestone; outside-band = review, not auto-action):
**cpp 150–700 lines · folder 5–15 files.** Current census: 53 cpp under 80 lines (fragmentation tail),
24 over 800 (knots). Both ends converge on the band.

---

## K1 — `window/` target layout (InputFrame split, done RIGHT-sized)

Today: `InputFrame.cpp` 1,636 (34 includes) beside `FramePresenter.cpp` 1,173 and small siblings.

**Target (window/ = 8–9 cpp files):**

| file | owns | budget |
|---|---|---|
| `InputFrame.cpp` | the per-frame ORCHESTRATOR: phase order, context plumbing — nothing else | ~200–300 |
| `InputGameplayActions.cpp` | gameplay action routing, controller/gamepad chords, jump/move dispatch | 300–500 |
| `InputCreativeModes.cpp` | map-maker + creative-fly + navigate lane + rebuild triggers (the creative input surface) | 300–500 |
| `InputMenuRouting.cpp` | menu select tracking (mouse+gamepad), mouse dispatch, save-slot browser keys; **absorbs `MouseCapturePolicy.cpp` (72+35 — below band, single concern-neighbor)** | 300–500 |
| `FramePresenter.cpp` | present orchestration (its own halving is refactor #13 — same law when cut) | — |
| `Loop.cpp`, `RendererLifecycle.cpp` | unchanged (in band) | — |

ONE internal cluster header (`InputFrameStages.hpp`) for the three stage TUs — not three public headers.
**Exact section→file assignment happens at card-cut** via a section survey of the 1,636 lines (the sections
are visibly banded today); the survey maps every existing block to one of the three concerns or the
orchestrator, and anything that fits none is a STOP.

## K2 — `apps/iggy3d_creative/` target layout (THE CONDENSATION CASE)

Today: **48 files / 5,510 lines** (~115 avg), 20 cpp under 100 lines, TWO naming families (`Standalone*` old,
`CreativeEditor*` new stage-extraction). This is over-extraction — reverse it.

**Target (~13 cpp + ~6 headers, ONE naming family: `Editor*`):**

| target file | absorbs (today's fragments) | ~size |
|---|---|---|
| `main.cpp` | orchestrator only: args → bootstrap → seed → frame loop | ~150 |
| `EditorState.hpp` | the state struct (exists, 79) | — |
| `EditorFrame.cpp` | **CreativeEditorFrameInput + CommandInput + Aim + ClickSelection + Selection + PlacementInput + PickFrame** (7 cpp + 7 hpp, ≈530 content lines) | ~550 |
| `EditorGizmo.cpp` | CreativeEditorGizmoFrame + StandaloneGizmo + move/drag glue | ~300 |
| `EditorEdits.cpp` | StandaloneDelete (76) + StandaloneUndo (90 hpp) + edit-verb glue | ~200 |
| `EditorPicking.cpp` | StandalonePicking + **StandaloneFrustumCull (79)** | ~500 |
| `EditorPreviewProxies.cpp` | StandalonePreviewProxies + **StandaloneWireframeBoxEdges (34)** | ~420 |
| `EditorBrushPalette.cpp` | StandaloneBrushPalette | ~200 |
| `EditorPlacement.cpp` | StandalonePlacement | ~150 |
| `EditorPathEditing.cpp` | StandalonePathEditing (shrinks further when debt-lane #1 routes it through Facade commands) | ~150 |
| `EditorPersistence.cpp` | StandalonePersistenceProof + StandaloneRoomBakePreview | ~260 |
| `EditorCapture.cpp` | StandaloneCaptureScenario + StandaloneCaptureScript.hpp + CreativeEditorCaptureScenario | ~700 |
| `EditorBootstrap.cpp` | CreativeRendererBootstrap | ~100 |

Net: **48 → ~19 files**, every cpp in band, one family, one cluster header where TUs share internals.
Behavior-preserving (Mode P): suite + capture path green, golden untouched. **Rule for the in-flight E266
follow-ons: new stages land INTO `EditorFrame.cpp`, not as new micro-TUs** — the extraction continues, the
fragmentation stops.

## K5 — `gameplay/` Controller tail condensation (small, Mode P)

The split was right; its tail is below band. **15 cpp → ~9:**
- `ControllerJumpDash.cpp` ← JumpActions (325) + DashActions (73) + JumpDashState (80)
- `ControllerTargeting.cpp` ← TargetActions (82) + TargetOutcomeProof (233)
- `ControllerProof.cpp` ← MovementProof (244) + TraversalProof (91) *(proof emission = one seam)*
- InputIntent (31) + PlayerAccess (36) → fold into `ControllerActionPhases.cpp` / `ControllerCommandExecution.cpp` (their only callers)
- WallQueries/WallRunEvaluation/GroundQueries/Kinematics/MoveActions/ResetFall: **keep** (in band, testable kernels).
Headers collapse with their TUs (public surface unchanged — `Controller.hpp` still exposes one entry).

## Repo-wide: the 53-file fragmentation tail

Not a dedicated card. K4's dep tool lists sub-band files per folder at every milestone; the **reviewer**
applies the band law during milestone spec refresh (merge candidates named in the report → boy-scout merges,
or a small batch card when a folder has 3+). The band is a ratchet DIRECTION, not a purge — a file below
band with a band-law justification (kernel/test/boundary/heavy-dep) simply records the justification in its
folder's AGENTS.md.

## The 800+ club (context — all already on the books)

`ObjectDescriptor.cpp` 2,038 (mostly the descriptor DATA table — data-as-code is exempt from the band; split
only if the table gains a generator) · `SaveCodec.cpp` 1,971 (refactor #8 enum-tables shrinks it) ·
`Automation.cpp` 1,889 (refactors #9/#10) · `CreativeUiFields.cpp` 1,666 (receipt lane) · `InputFrame.cpp`
1,636 (K1) · `BufferImageResources.cpp` 1,557 (render lane — heavy-dep isolation, exempt) · `Session.cpp`
1,512 (spine; P3 shrinks it slightly; K3's audit decides more).
