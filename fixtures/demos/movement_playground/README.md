# Movement Playground

Launch:

```sh
./build/iggy3d_visual_demo --renderer vulkan --window --interactive --input auto --package fixtures/demos/movement_playground/package.iggy3d.toml --print-render-receipt
```

Purpose:

- flat 48 ft by 48 ft authored room with grid strips on floor and walls;
- player spawn near the south side, looking into the arena;
- jump pads and a marked gap lane;
- clamber block and stepped ledges;
- vault rail with posts;
- dash lane with start/end strips;
- elevated wire-walk rail with supports;
- spell target blocks with projectile blocker surfaces;
- wall-run/clamber wall;
- walkable/blocker/projectile spatial surfaces for later movement mechanics.

Current runtime truth:

- this fixture is a playable/renderable test arena;
- first-person kinematic walking works against the shifted room surface set;
- jump, clamber, vault, dash, spell projectile, and wire-walk mechanics are authored as test zones, not implemented ability modes yet.
