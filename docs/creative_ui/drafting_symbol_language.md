# Creative Drafting Symbol Language v1.0 (UI-DRAFTING-1)

One visual grammar for the 2D editor: procedural toolbar glyphs plus a
declarative plan-symbol style table. This document is the written half of the
language; `drafting_symbol_sheet.svg` / `.png` are the visual half.

**Authority:** the C++ tables are the source of truth —
`apps/iggy3d_creative/EditorToolGlyphs.cpp` (glyph geometry) and
`apps/iggy3d_creative/EditorDraftingStyle.cpp` (role styles). The sheets are
regenerated references, not inputs. Nothing here adds authoring semantics; the
semantic plan projection (`WorldLayoutPlanProjection`, Codex's lane) decides
which role a primitive carries, and the style table only answers how a role
draws.

## Visual laws

1. Toolbar glyphs live on a 24x24 logical grid, ~1-cell safe margin,
   1.25-cell base stroke, butt caps, miter joins, one-pixel minimum at render.
2. Glyphs are single-tint procedural vectors drawn by
   `drawCreativeEditorToolGlyph`. No SVG loading, no icon font, no atlas.
3. Canvas plan symbols are parametric and world-oriented. They are never
   scaled toolbar icons; geometry comes from the plan projection, style from
   the role table.
4. Exterior walls read heavier (Heavy, 0.18 cells) than interior partitions
   (Medium, 0.11 cells). Shared boundaries are partitions with a long dash.
5. Overhead geometry (roof outline/ridge) is lighter and dashed, and only
   participates in the overhead layer. Lower-storey context is ghosted:
   reduced alpha, hairline, painted under the active storey's fills.
6. Every state stays distinguishable in grayscale: overlays differ by stroke
   class, dash pattern, or fill pattern — never hue alone. Invalid carries a
   shape cue (diagonal hatch + short dash). Tests pin this pairwise.
7. Doors draw a wall break, a leaf, and a swing arc matching the actual
   opening pose (all four directions honored from projection data; closed
   doors draw the leaf across the break with no arc). Windows draw a wall
   break with parallel glazing lines.
8. Stairs draw tread lines and an up arrow; ramps draw boundary lines and the
   up arrow with no treads.
9. No text inside icons or plan symbols. Tooltips and surrounding UI own
   wording.
10. Proven sizes: glyphs at 16/20/24/32 px (thickness ladder pinned in
    `creative_editor_tool_glyph_tests`); plan strokes at 6/12/24/48 px/cell
    (stroke-class scaling pinned in `creative_editor_drafting_style_tests`).

## Stroke classes

| Class | Width (cells) | Notes |
|---|---|---|
| None | 0 | fill-only roles |
| Hairline | — | always the style's minimum pixel width (1 px) |
| Light | 0.07 | clamps to 1 px below ~14 px/cell |
| Medium | 0.11 | |
| Heavy | 0.18 | exterior walls only |

## Role table (mirrors `EditorDraftingStyle.cpp`)

| Role | Tint | Stroke | Dash (cells) | Fill | Order | Layers |
|---|---|---|---|---|---|---|
| exterior wall | #E8E4D8 | Heavy | — | — | 40 | normal+context |
| interior partition | #C9C4B4 | Medium | — | — | 38 | normal+context |
| shared boundary | #C9C4B4 | Medium | 1.2/0.8 | — | 39 | normal+context |
| room floor | #8B8570 | None | — | solid 0.16 | 10 | normal+context |
| door | #E8E4D8 | Medium | — | — | 42 | normal+context |
| door swing | #E8E4D8 | Hairline | 0.9/0.9 | — | 43 | normal+context |
| window | #9FC4D6 | Medium | — | — | 41 | normal+context |
| stair | #C9C4B4 | Light | — | — | 36 | normal+context |
| ramp | #C9C4B4 | Light | — | — | 36 | normal+context |
| roof outline | #A89B78 | Light | 1.5/1.0 | — | 60 | overhead |
| roof ridge | #A89B78 | Hairline | 0.8/0.8 | — | 61 | overhead |
| contour minor | #6E6753 | Hairline | — | — | 6 | normal |
| contour major | #8A8168 | Light | — | — | 7 | normal |
| plateau | #99906F | Light | — | — | 8 | normal |
| road | #A9A9B4 | Medium | — | — | 9 | normal |
| ditch | #5E6E78 | Light | — | — | 8 | normal |
| bridge | #B4A98F | Medium | — | — | 12 | normal |
| elevation band | #4A5A46 | None | — | solid 0.10 | 2 | normal |
| region mask | #E8E4D8 | Light | 2.5/2.0 | — | 80 | normal |
| object bounds | #B4A98F | Light | — | — | 30 | normal+context |
| object point | #B4A98F | Medium | — | — | 31 | normal+context |
| object nature | #7FA36B | Light | — | — | 30 | normal+context |
| object architecture | #A9A9B4 | Light | — | — | 30 | normal+context |
| object cover | #5FA8A0 | Medium | — | — | 32 | normal+context |
| object prop | #B08D6E | Light | — | — | 30 | normal+context |
| player spawn | #6FDC8C | Medium | — | — | 50 | normal |
| npc spawn | #E0B34A | Medium | — | — | 50 | normal |
| hover overlay | #FFFFFF | Light | — | solid 0.08 | 90 | normal |
| selected overlay | #4FA9FF | Medium | — | solid 0.10 | 92 | normal |
| preview valid overlay | #52F07A | Light | 1.5/1.0 | solid 0.08 | 94 | normal |
| preview invalid overlay | #F05A50 | Light | 0.8/0.8 | hatch 0.22 | 95 | normal |
| locked overlay | #9AA0A6 | Hairline | 0.6/1.2 | hatch 0.10 | 91 | normal |
| generated overlay | #7FB8FF | Light | 2.0/1.2 | — | 89 | normal |
| overhead overlay | #E8E4D8 a140 | Hairline | 1.5/1.5 | — | 88 | overhead |
| lower level ghost overlay | #8C8778 a110 | Hairline | — | — | 4 | context |

Notes:
- The rectangle and ellipse region masks share the `region mask` role: mask
  shape is projection geometry, not a style difference. Their toolbar glyphs
  remain distinct (`mask rectangle`, `mask ellipse`).
- Object categories carry distinct hues because cover is tactically
  meaningful in this game; in grayscale, cover still separates by its Medium
  weight against Light neighbors.
- Draw order is ascending: fills (2-10) under terrain lines (6-12) under
  objects (30-32) under architecture lines (36-43) under spawns (50) under
  overhead (60-61, its own layer) under the region mask (80) under the state
  overlays (88-95). The ghost underlay (4) sits beneath the active storey.

## Toolbar glyph vocabulary

The original 43 glyphs are unchanged (tool, terrain-op, mask, asset-category,
workflow, and badge families; all mapping functions stay exhaustive). This
batch adds the 12-glyph view-control family: plan view, elevation view, 3d
view, level up, level down, fit all, fit selection, roof visibility, lower
level context, contours, dimensions, snap. Total: 55. Names and bounds are
pinned headlessly; every glyph stays inside the 24-grid and spans at least
half of it on one axis.

## Catalog asset thumbnails (design only — deliberately not glyphs)

Catalog assets are never hand-drawn icons. They should eventually render from
the actual meshes: fixed orthographic three-quarter camera, consistent
lighting, cached output. This batch designs only the frame language (sheet
section 6):

- Frame: rounded 4 px tile, 1 px paper border, #22272B well.
- Missing asset: dashed frame + diagonal slash. No text, no question mark.
- Selected asset: accent (#4FA9FF) 2 px border + corner check.
- Category badge: small corner chip reusing the family glyph (nature,
  cover, props, architecture) at ~14 px.

## Reserved symbols

On the sheet but absent from every production enum (pinned by test against
`creativeEditorDraftingReservedSymbolNames()`): objective marker, asset
thumbnail frame, asset thumbnail missing, asset thumbnail selected, asset
category badge.

## Misread risks, called out

- Player vs NPC spawn: same pin silhouette; player is a solid dot, NPC a
  hollow diamond. At 16 px the dot/diamond distinction carries it; the plan
  symbols additionally separate by hue (green vs amber).
- Shared boundary vs generated overlay: both long-dashed; shared boundary is
  warm paper at Medium, generated is cool blue at Light and only ever wraps
  geometry as an outline. Grayscale separation comes from weight.
- Ramp vs stair (toolbar): both wedge-shaped; stairs always show treads, the
  ramp never does (law 8 mirrors this on canvas).
- Fit all vs fit selection: identical brackets; the inner rect is solid for
  all, dashed for selection — dash is the "selection" cue everywhere in the
  language (region mask, fit selection, lower-level context).
- Level up vs level down: chevron sits above the plate stack for up, below
  for down; the plate stack itself is identical.

## Regenerating the sheets

The sheet generator script lives in the session scratchpad
(`make_drafting_sheet.py`); it emits `drafting_symbol_sheet.svg` and the PNG
comes from QuickLook (`qlmanage -t -s 3060`). QuickLook only renders square
thumbnails, so the SVG canvas is padded square. If the C++ tables change,
regenerate or accept the sheet as stale — the tests, not the sheet, guard the
contract.
