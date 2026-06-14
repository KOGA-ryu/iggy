# ASCII Dungeon Debug Projection Research

Purpose: learn how to use ASCII as a cheap dungeon/debug view without making it
authoritative game state. This is a builder-facing research note, not a production
API spec.

## Source Anchors

- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/display.c:51`: `newsym`,
  `map_*`, `show_glyph`, and `flush_screen` are separated by ownership: normal
  display update, remembered map mutation, temporary buffered effects, backend
  flush.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/display.c:916`: `newsym(x,y)`
  decides what visible cell should display from authoritative level, vision,
  monster, object, trap, engraving, and memory state.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/display.c:2249`: `flush_screen`
  walks dirty buffered glyph cells and sends them to `print_glyph`.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/src/display.c:2635`: `map_glyphinfo`
  turns glyph ids into display metadata and final terminal character.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:35`: terrain types are
  explicitly not display symbols.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/rm.h:159`: `struct rm`
  separates real terrain `typ` from remembered/displayed `glyph`.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/include/display.h:1024`: buffered
  map cells carry dirty state plus `glyph_info`.
- `/Users/kogaryu/iggy/NetHack-NetHack-5.0/win/tty/wintty.c:3857`: tty backend
  prints the resolved `ttychar`, not game state.
- `/Users/kogaryu/edi/src/drafting/DraftingAsciiMap.h:11`: ASCII cell kind is
  structural data; glyph vocabulary is swappable data.
- `/Users/kogaryu/edi/src/drafting/DraftingAsciiMap.h:57`: parser lowers text
  rows to typed cells; render round-trips the proof text.
- `/Users/kogaryu/edi/src/drafting/DraftingAsciiMap.cpp:11`: default glyphs map
  chars to neutral kinds/tags.
- `/Users/kogaryu/edi/src/drafting/DraftingAsciiMap.cpp:128`: geometry planning
  derives drafting objects from parsed cells; caller owns id minting.
- `/Users/kogaryu/edi-ui/src/recipe/RecipeOps.h:12`: one typed op stream has
  three consumers: ASCII preview, validators, and execution.
- `/Users/kogaryu/edi-ui/src/recipe/RecipeOpsAscii.h:10`: ASCII preview is a
  proof stage, not the recipe truth.
- `/Users/kogaryu/edi-ui/src/recipe/RecipeOpsAscii.h:17`: glyphs are data.
- `/Users/kogaryu/edi-ui/src/recipe/RecipeOpsAscii.cpp:126`: ASCII canvas stores
  UTF-8 glyph strings in a row-major cell buffer.
- `/Users/kogaryu/edi-ui/src/recipe/RecipeOpsAscii.cpp:442`: preview refuses
  unresolved ops it cannot honestly show.
- `/Users/kogaryu/iggy/engine/src/scene/draft/DraftDocument2D.hpp:11`: draft
  symbols are typed authored/editor facts.
- `/Users/kogaryu/iggy/engine/src/scene/level/LevelTileMap.hpp:12`: compiled
  tile map owns walkability, spawns, dimensions, and player start.
- `/Users/kogaryu/iggy/engine/src/scene/ai/AiMap2D.hpp:11`: AI map nodes own
  positions, radii, weights, tags, and links.
- `/Users/kogaryu/iggy/engine/src/scene/ai/AiMapQuery2D.hpp:21`: AI map queries
  produce derived entries, weights, and tags at a position.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorState2D.hpp:11`: NPC actor
  registry owns live actor identity, profile, faction, goal, and position.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorControlState2D.hpp:13`: NPC
  control registry owns objective, behavior, and move mode.
- `/Users/kogaryu/iggy/engine/src/scene/npc/NpcActorFrameState2D.hpp:11`: frame
  state is a projection joining actor and control registries.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcMapPlayControlFrameStep2D.hpp:22`:
  map play control step consumes hand, map query, and proposal context.
- `/Users/kogaryu/iggy/engine/src/scene/ai/NpcMapPlayControlFrameStep2D.cpp:18`:
  the step builds audit entries, proposals, apply report, and resulting control
  registry.

Note: `/Users/kogaryu/edi_u` was not present. Nearby repos inspected:
`/Users/kogaryu/edi` and `/Users/kogaryu/edi-ui`.

## Grep Commands Used

```sh
rg -n "Ascii|ASCII|Dungeon|dungeon|glyph|char grid|grid|canvas|export|import|parser|parse|document|Document|tile|Tile|symbol|Symbol" /Users/kogaryu/edi-ui/README.md /Users/kogaryu/edi-ui/docs /Users/kogaryu/edi-ui/src /Users/kogaryu/edi-ui/tests
rg -n "Ascii|ASCII|Dungeon|dungeon|glyph|char grid|grid|canvas|export|import|parser|parse|document|Document|tile|Tile|symbol|Symbol" /Users/kogaryu/edi/README.md /Users/kogaryu/edi/docs /Users/kogaryu/edi/src /Users/kogaryu/edi/tests
rg --files /Users/kogaryu/iggy/engine/src/scene/ai /Users/kogaryu/iggy/engine/src/scene/npc /Users/kogaryu/iggy/engine/src/scene/draft /Users/kogaryu/iggy/engine/src/scene/level | rg 'AiMap|NpcMapPlay|NpcActor|DraftDocument|LevelTileMap'
find /Users/kogaryu/iggy -maxdepth 2 \( -iname '*godot*' -o -iname '*devilution*' -o -iname '*master.zip*' \) -print
```

## Boundary Shape

```text
Edi authoring/export
-> ASCII rows + symbol palette
-> import validation / proof render
-> typed Iggy draft, level, AI-map, and spawn metadata
-> runtime/session orchestration later

typed Iggy state
-> ASCII debug projection
-> text rows / snapshots / legend
```

The important boundary is one-way lowering or one-way projection. ASCII may be an
input artifact and a debug artifact. It should not become the mutable runtime map.

## Authored, Intermediate, Typed, Projected

- Authored truth: Edi document state if a dungeon is authored in Edi, or
  `DraftDocument2D` when Iggy owns the imported/editor document.
- Intermediate ASCII: rows plus a symbol palette. Useful for review, copy/paste,
  tests, and import proof. Not enough to own gameplay semantics.
- Typed Iggy state: `DraftDocument2D`, `LevelTileMap`, `AiMap2D`,
  `NpcActorState2DRegistry`, and `NpcActorControlState2DRegistry`.
- Debug projection: generated ASCII frame from typed state and reports. Disposable.
- Runtime: later orchestration only. Runtime should not own ASCII unless it is
  carrying a transient debug frame/report.

NetHack's useful lesson is that display characters are the final projection of
level, memory, visibility, monsters, and backend glyph rules. Edi's useful lesson
is that glyphs can be configurable data while parser/builders switch on typed
cell kinds.

## First Slice Recommendation

Recommended first builder slice: `SceneAsciiCanvas2D`.

Why first:

- It is the shared substrate for import proof frames and AI debug frames.
- It does not require Edi import semantics yet.
- It does not depend on NPC AI reports yet.
- It gives cheap snapshot tests immediately.
- It follows the small part of NetHack worth copying: row-major display buffer
  plus deterministic rendering.

Candidate files:

- `/Users/kogaryu/iggy/engine/src/scene/ascii/SceneAsciiCanvas2D.hpp`
- `/Users/kogaryu/iggy/engine/src/scene/ascii/SceneAsciiCanvas2D.cpp`
- `/Users/kogaryu/iggy/engine/tests/scene_ascii_canvas_2d_tests.cpp`

Candidate API shape:

```text
SceneAsciiCanvas2D
- width
- height
- fill
- row-major char cells
- contains(x, y)
- get(x, y)
- set(x, y, char)
- drawPoint(x, y, char)
- drawLabel(x, y, string)
- renderRows() -> vector<string>
- renderString() -> string
```

Tests should prove:

- empty canvas renders stable rows
- set/get respects bounds
- out-of-bounds writes do not corrupt cells
- labels clip at the right edge
- multi-row render is deterministic
- source AI/map/NPC inputs are not needed and not mutated

Do not include yet:

- parser/import
- runtime ownership
- terminal backend
- colors
- UTF-8 glyph width handling
- dirty-cell incremental redraw
- AI decision logic
- collision, pathing, or LOS

## Second And Third Slices

Second slice: `SceneAsciiSymbolPalette2D`.

- Maps a symbol id to a display char, label, role, and optional neutral tag.
- Keeps glyph behavior configurable instead of hardcoded into chars.
- Supports legend generation.
- Should not turn a char into gameplay behavior directly.

Third slice: `NpcAiAsciiDebugFrame2D`.

- Consumes existing typed facts:
  `AiMap2D`, `NpcActorState2DRegistry`, `NpcActorControlState2DRegistry`,
  `NpcMapPlayControlFrameStep2DResult`, and optional path results.
- Emits rows plus legend/report notes.
- Does not decide, mutate, queue, or apply anything.

ASCII import/parser should come after canvas plus palette. Import has more
ownership questions because it must decide whether text lowers into draft symbols,
level tiles, AI-map nodes, spawn markers, or an import validation report.

## Symbol Configuration

Use explicit symbol roles, not char behavior.

```text
char '#'
symbol id "wall"
role Terrain
emits draft kind Wall or level blocker only through importer config

char 'C'
symbol id "cover"
role AiTag
emits tag "cover" only through importer config

char 'g'
symbol id "goblin_spawn"
role ActorSpawn
emits npc definition/profile id only through importer config
```

For debug projection, the palette is display-only:

- `tag:cover -> C`
- `tag:danger -> ^`
- `moveMode:routed -> r`
- `target:retreat -> R`
- `npc:goblin -> g`
- `player -> P`

The legend is part of the output so snapshots remain readable.

## NpcAiAsciiDebugFrame2D Draw Order

Start with low-detail overlays and add facts only when needed.

1. Base map terrain or empty cells, if supplied.
2. AI map nodes by tag or dominant weight.
3. Optional query/focus tile.
4. Target positions from current objective/control.
5. Optional path or next-step markers.
6. NPC actor positions from `NpcActorState2DRegistry`.
7. Control overlay from `NpcActorControlState2DRegistry`.
8. Selected `NpcMapPlayControlFrameStep2DResult` facts as legend lines, not grid
   clutter.
9. Player marker if provided by the caller.

Suggested priority if two facts share a cell:

```text
focused selection
player
npc actor
target
path / next step
AI tag
terrain
empty
```

## Compute Notes

- Full redraw is fine first: `O(width * height + overlays)`.
- Row-major char cells are tiny at test sizes.
- Rendering rows allocates `O(width * height)` string data; acceptable for
  snapshot tests and CI.
- Dirty cells are a terminal/UI optimization. NetHack needs them because it owns
  interactive terminal output; Iggy debug tests do not need them yet.
- Per-cell priority can avoid accidental overwrites, but simple ordered drawing is
  enough for the first slice.
- Do not scan every cell for every AI node. Draw nodes directly at tile positions.
- Radius/influence fills are later because they cost `O(nodes * filled area)`.
- String/id lookup in a small palette is fine. Do not build hashed indexes until
  profiles show it matters.
- Snapshot comparison should use `vector<string>` or one joined string.

## Avoid Now

- ASCII as saved runtime state.
- ASCII as collision/pathfinding authority after import.
- Hardcoded behavior in characters.
- Hidden conversion from local name strings to gameplay semantics.
- Qt/UI dependencies in the ASCII layer.
- Color, terminal control, or font-specific glyph widths.
- Runtime queues or actor movement inside the projection.
- LOS/path recomputation inside the projection.
- A parser that silently accepts unresolved symbols as gameplay facts.
- A mini-engine where ASCII owns map mutation, actor mutation, or AI decisions.

## Builder-Ready Summary

Recommended first slice:

```text
Name: SceneAsciiCanvas2D
Folder: engine/src/scene/ascii
Input: width, height, fill char, draw operations
Output: vector<string> rows and joined string
Tests: bounds, set/get, label clipping, deterministic rows
Not included: parser, AI, runtime, colors, dirty cells, UTF-8
```

Recommended follow-up:

```text
Name: SceneAsciiSymbolPalette2D
Folder: engine/src/scene/ascii
Input: symbol entries with char/id/role/label/tag
Output: lookup result and legend rows
Tests: duplicate chars, unknown chars, stable legend, no behavior inference
Not included: map import mutation
```

Recommended AI debug follow-up:

```text
Name: NpcAiAsciiDebugFrame2D
Folder: engine/src/scene/ai/debug or engine/src/scene/ascii
Input: AiMap2D, NPC actor/control registries, map play step result, optional path
Output: ASCII rows plus legend
Tests: map tags draw, NPC positions draw, controls override tags, no source mutation
Not included: AI decisions, movement execution, runtime queues
```
