# Legacy Renderer Reference

The old renderer is historical reference, not build input. It may be read to understand prior decisions, but `iggy3d` must not import, move, link, include, or depend on it.

## Old Files Inspected

- `/Users/kogaryu/iggy/engine/apps/native_play/NativeVulkanRenderer.hpp`
- `/Users/kogaryu/iggy/engine/apps/native_play/NativeVulkanRenderer.cpp`
- `/Users/kogaryu/iggy/engine/apps/native_play/NativeSceneDrawList.hpp`
- `/Users/kogaryu/iggy/engine/apps/native_play/IggyNativePlay.cpp`

## Useful Concepts To Copy Manually If Justified

- Frame input boundary: the old renderer accepted per-frame camera/projection data rather than owning all gameplay state.
- Instance/device/surface setup: useful checklist for required WSI setup, queue family selection, device scoring, and portability extension awareness.
- Swapchain recreate flow: useful lifecycle shape for resize and out-of-date handling.
- Command buffer flow: useful reminder of per-frame acquire, record, submit, present sequencing.
- Frames in flight: useful starting point for bounded frame resources.
- Basic model slots: useful concept for mapping projected draw items to renderer-owned mesh/model resources.
- Depth format and first 3D-room needs: useful reminder that the first room is not just a flat triangle path.

## Concepts Not To Copy Blindly

- Old path names, app names, and ownership names.
- Old app coupling and direct assumptions about the previous runtime.
- Old render-pass baseline unless the new Vulkan baseline decision explicitly selects that compatibility path.
- Lack of a validation/debug messenger baseline.
- Manual memory allocation if Vulkan Memory Allocator is chosen.
- Any dependency on old build files, old source tree layout, old scene types, or old draw-list types.
- Any renderer-side read of runtime truth that bypasses `iggy3d` projection/frame input ownership.

## Translation Rule

Translate ideas, not code.

If a concept survives review, rewrite it against `iggy3d` names, `iggy3d` ownership, and the file surface in [file_surface.md](file_surface.md). The old renderer is not a source file dependency, not a template to paste from, and not an architecture authority.
