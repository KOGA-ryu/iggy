#pragma once

#include <cstdint>

namespace iggy3d {

// Owned creative-undo mirror state (product window view of the creative document's undo depth) --
// extracted from the ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). Lives
// app-side (not creative/, which is a separate lane). Behavior-identical.
struct ProductCreativeUndoState {
  bool available = false;
  std::uint64_t depth = 0;
};

}  // namespace iggy3d
