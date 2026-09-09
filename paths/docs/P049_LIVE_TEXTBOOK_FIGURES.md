# P049 — reusable textbook spread with a live 3D figure

Section **1.2, Row operations and RREF**, now opens with reading on the left
and an adjustable native 3D figure on the right. The divider resizes the panes.
**Reading only** and **Figure only** give either activity the full workspace;
**Read + figure** restores the spread. The figure follows the existing matrix
board's current state and history. This is the first provider for a shared shell.

```sh
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 2
/Users/kogaryu/iggy3d/paths/b/math_lab --book --section 2 --book-view figure
```

The user accepted P048's reading format before this extension and subsequently
confirmed that P049's layout and interaction work. P050 reuses that accepted shell. No window, screenshot,
image, render, preview or font-rasterization probe is used for verification.

## What the RREF figure means

The three columns of card 004 part (a) become coefficients of x, y and z in the
explicitly homogeneous system **Ax = 0**. Each nonzero row describes a plane
through the origin. Coloured open grids show finite portions of those infinite
planes; gold shows their common solution set. A movable probe lies in that set.
All printed columns still belong to A. No final column is reinterpreted as b.

Part (a) gives two planes with a common line. Part (d) gives three independent
equations with the origin as their only solution. The other printed shapes
remain available in Exercise. The provider explicitly declines matrices with
other column counts, more than three rows, complex entries or nonfinite values.
A zero row imposes no constraint and is not drawn as a plane. The model also
supports nullities two and three for future examples.

**Next pivot**, **Undo**, **Reset example**, and the manual row controls use the
same MatrixBoard instance as the section's exercise. A nonzero row scaling
changes the equation's coefficients while leaving its plane in place; row
addition can change a plane while retaining the common intersection. Selecting
a printed part restarts that card's work, as the existing exercise selector does.

The figure offers a solution probe, row highlighting through the equation list,
original-plane comparison, normal vectors, labels, plane extent and grid density.
The rows' equations use the same colours as their planes and update after each
operation. Coefficients are displayed to five significant digits. The full matrix
workspace remains reachable through **Exercise**. Camera navigation uses the
existing orbit, pan and zoom controls, with **Reset camera** for framing.

## Reusing the format

The page structure lives in one implementation. A section supplies a figure
binding beside its prose, references and exercise metadata:

```cpp
BookFigureSpec{
  BookFigureKind::RowPlanes,
  "rref.planes",
  "Equation planes / Ax = 0",
  "Figure 1.2.7. Each coloured grid is one equation ..."
}
```

To adapt a new topic:

1. Copy the section's content structure and give the figure a stable ID, title
   and caption. Reuse an existing provider where its mathematical interpretation
   fits; otherwise add a provider for the new representation.
2. Have that provider consume its mathematical owner's snapshot and publish
   bounded geometry, measurements and controls. It must not duplicate judging
   or rewrite another exercise's work.
3. Keep the shared pane, camera, navigation, caption and responsive-layout code.
   Only the topic's content, projection and controls should vary.

`LessonSpread` owns the reusable rectangles for the header, contents rail,
reading pane, divider, figure and footer. Below the minimum split width it uses
a single pane; the header buttons select reading or figure explicitly. In a
wide expanded figure, controls sit alongside the viewport. In smaller figure
panels, controls scroll below it while the viewport remains fixed.

`TextbookFigureUi` connects the provider with those regions and the existing
`NativeMath` and `MathObjectScene` adapters. Its first provider is `RowPlaneFigure`.
The math lab supplies one stable SceneFrame packet to the native host, selected
after the active UI pane is drawn. Each frame starts empty, so leaving a figure
or selecting an unsupported shape cannot leave an old scene behind. The native
host, renderer, object collection, Library and source problem pages are unchanged.

## Mathematical and state boundaries

`RowPlaneFigure` is a read-only projection of the board plus bounded exploration
settings. It normalizes nonzero rows, constructs an orthonormal row-space basis
with reorthogonalization, and obtains the orthogonal complement in three
dimensions. Rank uses a relative tolerance of 1e-10. Original and current null
spaces are compared through their orthogonal projectors, with tolerance 1e-8.
When they agree, a stable original basis prevents a solution probe from jumping
merely because a row was swapped. A disagreement is reported explicitly.

The geometry illustrates the given matrix immediately, including its solution
space. This is authored teaching disclosure; it does not produce an RREF answer,
mark a board as worked, run its Check action, open source help or grade a proof.
Each user-requested row operation goes through the existing board dispatcher.
Figure settings and camera movement are session state. The reading bookmark
continues to contain only the section, scroll positions and text size.

## Verification

The new pure CPU suite checks the known null direction for printed part (a),
row addition/scaling/swaps, guided reduction and Undo, all four nullities, zero
rows, very small and large row scales, unsupported dimensions and complex
coefficients, stale-geometry removal, parameter rejection, cached publication,
probe membership, retained checked results, camera navigation and pane bounds.

The suite inspects **153 CPU mesh states** and **1,008 layouts**. Maximum geometry
is **3,402 vertices and 10,656 indices**, below the existing 8,192 / 65,536 limits.
Three new CLI cases cover split, expanded small-window and reading-only modes.
Together with the previous textbook, board and object checks, the targeted run
contains **34 text-only tests**. All eight changed/new C++ translation units
also pass warnings-enabled syntax checks. The native target is compiled and
linked without executing its windowed rendering path.

```sh
ctest --test-dir /Users/kogaryu/iggy3d/paths/b \
  -R '^(paths_textbook_figure_tests|paths_textbook_tests|paths_matrix_board_tests|paths_math_.*_cli)$' \
  --output-on-failure
```

`--validate` remains before preferences, bookmark I/O, NativeMath construction
and native host creation. The font-rasterizing native-math suite is excluded.
Changes remain uncommitted. This checkpoint implements the reusable spread and
one 3D topic provider; additional providers are future section work.
