# `engine/src/runtime3d/Runtime3DSessionState.hpp`

Purpose: declare top-level runtime3d session state.

Must contain:

- `#pragma once`.
- Include `<string>` and `<vector>`.
- Include `Runtime3DClock.hpp`, `Runtime3DCameraState.hpp`,
  `Runtime3DWorldState.hpp`, and `Runtime3DCommand.hpp`.
- Namespace `iggy::runtime3d`.
- `enum class Runtime3DSessionLifecycle` with `NotLoaded`, `Loading`,
  `PlayingRealtime`, `PlanningTactical`, `Paused`, `Completed`, `Failed`,
  `IncompatibleSave`.
- `struct Runtime3DSessionState`.

Session fields:

- package identity;
- scenario identity;
- lifecycle default `NotLoaded`;
- clock state;
- camera state;
- world state;
- command log vector;
- current selected entity/target placeholders if needed.

Construction rules:

- Keep state serializable by shape.
- Do not store raw input, renderer state, or recursive snapshots.

Completion:

- Session state tests can assert defaults and playable session setup.

