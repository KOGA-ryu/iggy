#pragma once

#include "app/iggy3d/creative/world/WorldLayoutPlanProjection.hpp"

#include <cstddef>

struct ImDrawList;

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutCanvasTransform;
struct CreativeEditorWorldLayoutPlanViewCache;
struct CreativeEditorWorldLayoutState;

struct CreativeEditorWorldLayoutPlanDrawReceipt {
  std::size_t basePrimitiveCount = 0U;
  std::size_t selectedPrimitiveCount = 0U;
  std::size_t suppressedPrimitiveCount = 0U;
};

// O(P) on idle frames with no editor-side temporary geometry or sorting.
// Geometry and stable paint order rebuild only when the semantic plan-view
// cache invalidates; ImGui retains ownership of its draw-buffer capacity.
[[nodiscard]] CreativeEditorWorldLayoutPlanDrawReceipt
drawCreativeEditorWorldLayoutPlan(
    ImDrawList& drawList,
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    const CreativeEditorWorldLayoutState& state,
    const iggy3d::creative::CreativeWorldLayout& layout,
    const CreativeEditorWorldLayoutPlanViewCache& planView);

}  // namespace iggy3d_creative_app
