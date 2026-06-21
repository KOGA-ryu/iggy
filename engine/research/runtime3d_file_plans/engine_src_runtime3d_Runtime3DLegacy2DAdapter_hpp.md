# `engine/src/runtime3d/Runtime3DLegacy2DAdapter.hpp`

Purpose: declare the temporary bridge from existing 2D/product-loop state into
runtime3d state.

Must contain:

- `#pragma once`.
- Namespace `iggy::runtime3d`.
- Adapter status enum.
- Adapter input struct.
- Adapter result struct.
- `class Runtime3DLegacy2DAdapter`.

Construction rules:

- This is the only runtime3d file family allowed to include old 2D runtime
  headers.
- First skeleton may expose placeholder status if 2D source dependencies are
  not wired.
- Adapter is one-way: old 2D state to runtime3d state.

Completion:

- Adapter tests can prove placeholder status or mapped demo entities.

