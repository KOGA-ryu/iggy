# Save Snapshot Contract v1

## Objective

Define how product save slots get a visual snapshot without making menus,
save files, or runtime gameplay own renderer behavior.

Save snapshots are presentation sidecars for save selection. They must make
the starter save browser useful, but a missing or corrupt snapshot must never
make the save unloadable.

This is a plan only. It does not authorize source edits.

## User Decision

When a save is written, capture a screenshot from the active gameplay camera.

Rules:

- capture the gameplay or tactical camera view, not the pause menu;
- if saving from pause, capture the camera view behind the pause UI;
- store the snapshot as a sidecar image next to the save file;
- keep the save file authoritative even if the snapshot is missing;
- selector rows display title, snapshot image, and date/time.

Example file pair:

```text
save_0007.iggy3d.save
save_0007.snapshot.png
```

## Proposed Durable Files

Later implementation should prefer a small product/save bridge boundary:

```text
src/app/iggy3d/ProductSaveSnapshot.hpp
src/app/iggy3d/ProductSaveSnapshot.cpp
src/app/iggy3d/ProductSaveBridge.hpp
src/app/iggy3d/ProductSaveBridge.cpp
src/app/frontend/SaveSlotModel.hpp
src/app/frontend/SaveSlotModel.cpp
```

Existing save ownership stays in:

```text
src/runtime/save/SaveFileStore.*
src/runtime/save/SaveEnvelope.*
src/runtime/save/SaveCodec.*
```

If image encoding requires a helper later, it should be app/product-owned or
utility-owned. Runtime save files must not include renderer headers.

## Ownership

| Domain | Owns | Must not own |
| --- | --- | --- |
| Runtime save store | `.iggy3d.save` contents and load/save truth | screenshot capture, UI fallback policy |
| Product save bridge | save-slot orchestration, sidecar path calculation, snapshot request | gameplay mutation, renderer backend details |
| Product snapshot helper | capture request/result, sidecar write status, image metadata | save codec schema |
| Frontend save model | row title, date/time, snapshot availability, fallback flag | save parsing or capture |
| Selector view | display row data and fallback image state | load/delete semantics |

## Capture Semantics

Capture source is the currently active gameplay camera mode:

- first-person gameplay: capture the first-person camera view;
- tactical gameplay: capture the tactical camera view;
- editor gameplay: capture the editor camera view only when the save action is
  explicitly editor-owned;
- starter menu: no camera capture; use last known snapshot or fallback.

Pause menu is not captured. When saving from pause:

1. pause menu requests save;
2. product save bridge asks for a gameplay camera snapshot behind the pause UI;
3. save file is written;
4. snapshot sidecar is written if capture succeeds;
5. receipt reports save and snapshot independently.

Snapshot failure is non-fatal for save write unless the packet explicitly adds
a strict visual-capture lane later.

## Sidecar Path Semantics

Given:

```text
<save-root>/save_0007.iggy3d.save
```

Snapshot path is:

```text
<save-root>/save_0007.snapshot.png
```

The sidecar name should be derived deterministically from the save filename.
Do not store an absolute sidecar path inside runtime save truth.

Save summaries may cache or report the resolved sidecar path for UI display.

## Fallback Semantics

If the snapshot is missing, unreadable, corrupt, or not yet generated:

- the save remains visible;
- the save remains loadable;
- selector row uses a stable fallback thumbnail state;
- receipt reports snapshot unavailable and the reason.

Fallback must be deterministic and must not trigger a save rewrite.

## Selector Display Contract

Save selector entries must display:

```text
title
snapshot image or fallback
date/time
```

Optional later fields:

```text
location
chapter
playtime
compatibility
```

The selector does not generate snapshots. It only receives summary fields.

## Receipt Fields

Recommended save-side fields:

```text
save_written=true|false
save_path=<path-or-empty>
save_snapshot_requested=true|false
save_snapshot_written=true|false
save_snapshot_path=<path-or-empty>
save_snapshot_source=gameplay_camera|tactical_camera|editor_camera|none
save_snapshot_ui_hidden=true|false
save_snapshot_status=written|missing|corrupt|unavailable|skipped
```

Recommended selector-side fields:

```text
save_browser_snapshot_available=true|false
save_browser_snapshot_path=<path-or-empty>
save_browser_snapshot_fallback=true|false
save_browser_selected_timestamp=<timestamp-or-none>
```

Receipts remain deterministic key-value text. Do not introduce JSON.

## Test Strategy

Unit tests should prove:

- sidecar path derivation from save path;
- missing snapshot does not make a save summary invalid;
- corrupt snapshot reports fallback without rejecting the save;
- pause-save requests snapshot with UI hidden;
- selector row model carries title, snapshot/fallback, and date/time.

Product no-window smokes should prove receipt behavior with a fake or stubbed
snapshot result. Real image capture/window proof requires a later explicit
visual packet.

## Stop Rules

Stop before implementation if the plan requires:

- changing the runtime save schema only to store image bytes;
- making snapshots required for load;
- capturing pause/settings/dev UI;
- adding renderer/Vulkan dependencies to runtime save code;
- launching a window by default;
- adding JSON or a new machine-contract format.

## Long-Term Fit

Sidecar snapshots let save selection become useful while preserving the durable
truth split: save files own gameplay state, product app owns presentation
capture, and frontend selectors show summaries. This survives future chapters,
tactical views, editor saves, and richer save metadata without making runtime
save/load depend on the renderer.
