# Native equation panel

Follow-on: [Library typesetting](LIBRARY_TYPESETTING.md) now uses this same
renderer within the 930 existing source entries. The record below describes
the preceding four-sample checkpoint.

The sorter Library now opens **Equations**, a native ImGui panel with four
typeset samples. It establishes the rendering path before importing complete
problem pages or changing the Library's source-body renderer. The work remains
uncommitted; visual acceptance belongs to the user.

| Selector | Colour | Expression |
| --- | --- | --- |
| Fraction | Cyan | `(a+b)/(c+d)`, as a stacked fraction |
| Root | Gold | `sqrt(1+sqrt(1+x^2))`, with nested radicals |
| Matrix | Green | A 3x3 matrix with a fractional entry and negative numbers |
| 095 | Violet | `Delta u = -lambda^2 (1+x/2)u` from the lopsided-drum question |

The final example preserves the equation in the Question section of
`/Users/kogaryu/devil/99-red-booleans/problems/math/095_eigenmodes_of_a_lopsided_drum.md`.
Its fraction and enclosing parentheses are typeset explicitly. This is an
expression preview, not a playable adaptation or a verified solution to 095.
No external problem page or learner state is read or written at runtime.

## Ownership and data flow

`NativeMath` owns native typesetting and drawing. A strict MicroTeX parser
builds the formula, and its drawing interface emits positioned glyph quads,
rules and polygons. ImGui's existing atlas and Vulkan backend own texture
uploads, buffers, descriptors, clipping and final drawing. No new Vulkan host,
shader or browser route is introduced. Nothing is flattened into an equation
image or captured from a window.

Math fonts use em units. The adapter reads each bundled font's ascender,
descender and units-per-em to configure ImGui's scale, which is essential for
tall roots and matrix delimiters. A 48-pixel font atlas supplies glyphs; the
panel displays mathematical em sizes from 16 to 40, initially 24. Glyph identities
remain separate, while current atlas UVs are resolved on each draw. Layouts are
cached by expression and size, with sixteen entries maximum.

The adapter is tied to its ImGui context and UI thread. Process-wide MicroTeX
font descriptors retain paths rather than ImGui pointers, allowing sequential
UI contexts to use fresh atlases. The source directory is bundled beside the
executable as `math_typesetter/`. There is no runtime network dependency.

`MathCorpusUi` owns the modal panel, sample choice, text size and source toggle.
Close and Escape return to the same reading entry and filters; modal input
cannot activate underlying Practice. The panel follows window dimensions and
long equations have horizontal scrolling. Small UI controls remain 13-pixel
text and 22-pixel button height. Gameplay, question correctness, practice saves,
reviewed-note content and the other builder's math objects retain their owners.

## Error handling and current limits

Unknown commands are rejected using MicroTeX's explicit non-partial parser.
An additional group check rejects unfinished braces, which MicroTeX otherwise
accepts even in strict mode. Escaped braces and TeX comments are accounted for.
The adapter bounds expression length, group depth, text size and output extent.
Missing glyphs or unsupported drawing operations produce an error with the
source available, rather than silently substituted glyphs or partial output.

This checkpoint exposes the four fixed samples. It does not claim support for
every LaTeX command, the entire 95-page collection, inline Markdown layout,
symbol hit testing, world-space equation boards or SDF zoom. Rounded math
frames currently report an unsupported-operation error. MicroTeX's native
adapter covers the operations exercised here; further content needs its own
compatibility check.

## Verification

Release targets: `sorter`, `paths_native_math_tests`, `paths_sorter_input_tests`.
Both targeted CTest entries passed. The native math test exercises:

- Separate numerator/denominator glyphs and an intervening fraction bar.
- Raised, smaller superscripts, aligned matrix rows/columns, and ink extents.
- All four expressions at 16, 24 and 40, plus cache reuse and error recovery.
- Mouse selectors, size/source controls, Close, Escape and Tab/Enter.
- Library preservation and blocking of input to underlying Practice.
- Layout at 1440x860, 800x600 and 360x480.
- The native backend's dynamic-atlas request contract with dummy texture IDs,
  cached glyph UV rebinding, live resize, size bounds and horizontal scrolling.

These are CPU-only ImGui frames with an in-memory font atlas. No SDL window,
Vulkan device, screenshot, framebuffer capture or exported image was used.
Actual on-screen appearance remains unverified until the user checks it.
The existing sorter input suite passed before the subsequent parser-only
guards; it does not instantiate `NativeMath`. Its passing gate was not repeated.

Evidence and bundle/source integrity results are recorded in
`build/native-equation-evidence/verification.json`.

First-party production C++ changes add 354 net lines across two new files and
three existing files. This is a new rendering capability, not a cleanup.
There are 205 pinned dependency seed files, plus the local MicroTeX CMake and
attribution files; one dependency source has the documented lifetime patch.
All 78 deployed resource files match their bundled sources, and all 103
existing content files match the pre-work hashes. Other pre-existing files
outside the eight explicitly changed app/UI/build/documentation files retain
their pre-work hashes. `sorter --check-content` also passed from `/private/tmp`.

## Manual check

Launch `b/sorter`, then Contents → Library → the blue Equations button.
Inspect the cyan fraction, gold nested root, green matrix and violet 095
expression for readable symbols and clean spacing. Try A-/A+, Show source,
and Close or Escape. A narrow window should let a large equation scroll
horizontally. The Library should retain its selected entry.

The next candidate is rendering equations within real Library entries through
this same owner, with source text preserved and unsupported expressions explicit.
