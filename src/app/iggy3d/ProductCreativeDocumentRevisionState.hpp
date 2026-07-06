#pragma once

#include <cstdint>

namespace iggy3d {

// Owned creative-document revision-observation state (product window view) -- extracted from the
// ProductAppWindowState god-struct (docs/appkernel_build_map_v0_1.md, L2). App-side (not creative/,
// a separate lane). Behavior-identical.
struct ProductCreativeDocumentRevisionState {
  bool observed = false;
  std::uint64_t documentId = 0;
  std::uint64_t beforeFrame = 0;
  std::uint64_t afterFrame = 0;
};

}  // namespace iggy3d
