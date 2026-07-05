# iggy3d_creative — agent rules

The standalone, self-contained creative level editor (build → edit → persist).
**Full handoff: `docs/creative_mode/standalone_app_handoff.md` — read it first.**

## The rules that keep this maintainable
1. **Edit ONLY `main.cpp` here.** No product source (`src/app/iggy3d/**`), no
   `CMakeLists.txt`. The app is a *pure consumer* of the `iggy3d` library.
   - The one exception is a deliberate change to a kernel *system* (e.g. the
     view-agnostic Move). If you must, keep the default backward-compatible and
     run `cmake --build build && ctest` — all 236 must stay green.
2. **Generic systems, never per-kind code.** Select/Inspect/Move/Gizmo/Place key
   off the *selection* + `document().objects()`, never a hardcoded id or a
   `if (kind == ...)`. The only kind-specific code allowed is pure data
   (`renderRoleForKind` color, `brushFootprintFor` size). A new kind = a
   descriptor row in the kernel + those two data entries; it inherits the toolkit.
3. **Capture-verify every visual change.** Build, then:
   `./build/iggy3d_creative --capture /tmp/shot.png --frames 12` and LOOK at the
   PNG (it renders headless under MoltenVK — no display needed). Add a synthesized
   `--capture` script for any new feature. Trust the submit *reason*
   (`package_room_meshes_presented`) not `coverage`.

## Fast gotchas (details in the handoff)
- Debug-line `thickness` is WORLD METRES (use ~0.03, not 3.0).
- Renderer draws only AXIS-ALIGNED debug lines.
- `makeProductVulkanFrame` leaves `frame.ui`/`.creativeWireframeDebug` empty — the
  app fills them (vectors must outlive `submitFrame`).
- Move/place anchor = `transform.position` (the facade's anchor), not `bounds.min`.
- Grid config extents must be > 0.

Build: `cmake --build build --target iggy3d_creative -j10`  (ccache-backed).
