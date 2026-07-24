#pragma once

#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorWorldLayoutCanvasScreenPoint {
  float x = 0.0F;
  float y = 0.0F;
};

struct CreativeEditorWorldLayoutCanvasTransform {
  CreativeEditorWorldLayoutCanvasScreenPoint origin;
  float pixelsPerCell = 28.0F;
};

[[nodiscard]] CreativeEditorWorldLayoutCanvasScreenPoint
planCreativeEditorWorldLayoutCanvasScreenPoint(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    double x, double z) noexcept;

[[nodiscard]] CreativeEditorWorldLayoutPoint
planCreativeEditorWorldLayoutCanvasWorldPoint(
    const CreativeEditorWorldLayoutCanvasTransform& transform,
    CreativeEditorWorldLayoutCanvasScreenPoint screen) noexcept;

}  // namespace iggy3d_creative_app
