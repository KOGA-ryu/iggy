# P056 — Compact model workspace

The 27-object lab now uses one responsive toolbar and two optional panels.
The selected object keeps its existing controls, layers, geometry and checks.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --object boolean --level 0 --object-preset 0
```

## Layout and interaction

- **Object:** searchable dropdown across object keys, names and titles. All
  whitespace-separated search words must match; matching ignores letter case.
- **Example:** the selected example name, Defaults or Custom after an edit.
  Selecting a curve/profile handle does not mark the example as customized.
- **Layer:** the existing learning layers. Collapse choices are remembered
  separately for each object and layer during the current app session.
- **Controls:** a right inspector with labels beside sliders and editable numeric
  fields. Drag a number or Ctrl-click to type it. XYZ, UV, indices and rotation
  components share rows. Longer labels wrap; hovering reveals the full label.
- **Groups:** shape, profile, transform, operation, probe, animation, sampling,
  display and advanced settings appear only when they contain available controls.
  A changed-count badge compares all group parameters with canonical defaults,
  including parameters hidden at this layer. R resets a row or group; Reset all
  restores the object's defaults. These resets do not restore the last preset.
- **View:** reset camera, label visibility, and relevant guides/section controls.
  Hover/selected labels are the initial mode; All labels and Hide labels remain
  available. Mouse orbit, pan, zoom and editable handle selection are retained.
- **Analysis:** resizable bottom drawer with Graph, Values, Math and Exercise.
  Graph keeps plot scrubbing, contour interaction and matrix-entry editing.
  Values contains every metric and the full value table. Math holds the
  relationship, explanation and convention. Exercise retains the model's check.
- **Space:** drag the divider left of Controls or above Analysis to resize.
  Their toolbar checkboxes hide them. At narrow widths Controls overlays the
  model and can be hidden; pointer input over it does not navigate the camera.
  The header uses one, two or three rows as width and scale require.

Two or three key metrics remain visible above the model. At 1440×900 and scale
1, with a 360 px inspector and 200 px drawer, the pure layout allocates a
1062×560 model viewport. Hiding both panels expands it to 1428×766. The previous
layout allocated roughly 304 px of model height with linked diagrams open.
These are computed layout dimensions, not a visual review of native widgets.
At extremely short/scaled layouts the planner hides the metrics/drawer to keep
the remaining panes inside bounds.

## Shared implementation

`MathObjectLayout` has no ImGui, native-host or font dependency. It owns control
group/tuple metadata, constrained control ranges, presentation memory and pure
rectangle planning. `MathControlUi` renders scalar/choice/tuple rows. The lab
and textbook object figures share the scalar widget; the textbook retains its
explicit author-controlled parameter list and Read + Model / Read / Model /
Exercise shell. Other equation, matrix and document-specific controls are not
converted by this pass.

Model controls queue actions for the start of the next frame so geometry,
plots, metrics and feedback consume the same revision. Object/example/layer
selection happens in the toolbar before those views are drawn. Panel, camera
and label changes do not grade an answer. Existing layer-change parameter rules
are retained; session collapse memory is presentation state, not saved progress.

`MathObjects::ResetParameters` is the single owner of partial resets. It rejects
empty/foreign masks, checks all coupled lathe height gaps before mutation, and
commits one revision. A height's individual R is unavailable if its default would
cross a neighbour; Profile R resets the coupled group together. Playback stops,
affected seeded walks/trials restart, and unrelated symmetry moves are retained.
Sampling choices now live in the canonical parameter specification instead of a
special case in the old lab sidebar.

## Validation

Pure checks and native compilation are performed without opening a window,
rasterizing fonts, capturing or viewing images. `math_lab --validate` returns
before native-host creation and bookmark I/O; object validation now also prints
its pure control/layout plan. Native widget interaction and appearance require
the user's manual visual test.

The focused layout suite checks 6,480 rectangle configurations, control coverage
for all 96 available object/layer combinations, search, collapse/example state,
and atomic/coupled reset behavior: 153,115 assertions pass. The installed Release
math_lab and sorter builds pass, together with all 81 selected tests: 17 pure CPU
suites and 64 reviewed text-only CLI cases. Narrow Boolean and wide sweep cases
verify the new layout-report route. Installed source hashes matched after the
checks. No images or native windows were used. Changes remain uncommitted.
