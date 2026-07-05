#pragma once

#include "app/iggy3d/creative/Document.hpp"
#include "app/iggy3d/creative/Object.hpp"

#include <cstdint>

namespace iggy3d::creative {

// Placement is the pure creation-placement policy (contract TD-5): successive
// creates must not stack invisibly at the descriptor-default origin. The
// helper offsets the descriptor-default placement along +X by
// (existing object count) * document snap stepX.
//
// Consistency law (post-D8 coherence): for kinds without a transform the
// offset moves the default bounds; for transform kinds BOTH the transform
// position and the bounds are translated by the same offset so they never
// drift apart.
struct CreativePlacedCreateRequest {
  CreativeDocumentCreateRequest createRequest;
  std::uint64_t existingObjectCount = 0;
  double stepX = 0.0;
  double offsetX = 0.0;
  bool offsetApplied = false;
  bool transformOffsetApplied = false;
  bool boundsOffsetApplied = false;
};

[[nodiscard]] CreativePlacedCreateRequest buildPlacedCreateRequest(
    const CreativeDocument& document, CreativeObjectKind kind);

}  // namespace iggy3d::creative
