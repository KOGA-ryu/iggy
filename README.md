# iggy3d

Standalone 3D runtime/gameplay repo for the `iggy3d` build.

Current status: playable runtime demo in progress. The movement playground is
the current first-person test arena for movement, traversal, spells, debug
telemetry, room editing, and procedural bean model proxies.

Startup script:

```sh
#!/usr/bin/env sh
set -eu

cd ~/iggy3d
cmake --build build --target iggy3d_visual_demo -j 8

./build/iggy3d_visual_demo \
  --renderer vulkan \
  --window \
  --interactive \
  --input auto \
  --package fixtures/demos/movement_playground/package.iggy3d.toml \
  --print-render-receipt
```

If Vulkan is not available on the machine, replace `--renderer vulkan` with
`--renderer null` for receipt-only validation.

Start here:

- `docs/architecture.md`
- `docs/ownership.md`
- `docs/acceptance_demo.md`
- `docs/roadmap.md`
- `docs/file_plans/INDEX.md`
- `docs/file_plans/PRIORITY.md`
- `docs/file_plans/COMPLETE_BUILD_SURFACE.md`

Hard rule: this repo must not include, link, or build against old
`/Users/kogaryu/iggy` code.
