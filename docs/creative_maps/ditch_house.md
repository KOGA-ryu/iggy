# Ditch House

## Open

```sh
i3dc --map ditch_house
```

The first run creates `~/.iggy3d/creative_standalone/ditch_house.iggy3d.save`.
Later runs reopen that save. To replace it with the canonical authored map
without opening a window:

```sh
i3dc --generate-map ditch_house
```

An arbitrary existing slot can be opened with `i3dc --load <save-id>`.

## Authored Content

- Dry, shallow sand channel between dirt banks; the surrounding terrain is
  raised to three cells.
- Footbridge, approach, porch, and player arrival marker.
- Four-room house: living room, kitchen, workshop, and bedroom.
- Segmented exterior/interior walls, seven windows, and five door panels stored
  in open poses so movement routes are not sealed.
- Generic furniture, workshop props, one NPC spawn, and four dressing rocks.

## Capability Gaps

These are current representation limits exposed while authoring this map.

- **Water:** `WaterVolume` is only a volume descriptor. There is no terrain
  water surface, shallow-water material, flow, or water physics, so the channel
  is intentionally dry.
- **Excavation below the grid origin:** terrain controls accept heights 1-64.
  The ditch therefore uses a one-cell channel inside a three-cell raised field;
  it cannot cut below Y=0.
- **Doors:** there is no open/close state or door interaction system. Door
  panels are authored already open.
- **Windows:** window objects are box proxies with no transparent glass or
  frame-specific mesh.
- **Furniture:** `Furniture`, `Crate`, and `Barrel` render as generic box
  proxies. There is no reusable authored-object or prefab mesh library yet.
- **Roofing:** the house uses four flat roof slabs. There is no pitched-roof
  generator or roof join logic.
- **Map selection UI:** built-in maps and save slots have command-line loading,
  but no in-editor browser or thumbnail picker.
- **Room presentation:** the four Room objects are metadata containers and are
  intentionally skipped by the runtime geometry bake; room names are not shown
  in the viewport.
