#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include "EditorFrame.hpp"
#include "StandaloneCaptureScript.hpp"
#include "StandaloneGizmo.hpp"
#include "StandalonePicking.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorGizmoFrame {
  iggy3d::Vec3 center{0.0F, 0.0F, 0.0F};
  std::array<GizmoAxisShaft, 3> shafts{};
  ScreenPoint centerScreen;
  std::array<ScreenPoint, 3> tipScreens{};
  std::vector<PathPointHandleHit> pathPointHandleHits;
  iggy3d::creative::CreativeObjectId selectedPathHandleObjectId =
      iggy3d::creative::kInvalidObjectId;
  bool selectedIsPathForHandles = false;
  iggy3d::Vec3 anchorS{0.0F, 0.0F, 0.0F};
};

[[nodiscard]] CreativeEditorGizmoFrame buildCreativeEditorGizmoFrame(
    const CreativeEditorSelectionFrame& selection,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float axisLengthMeters);

void logCreativeEditorPathHandleCaptureFrame(
    StandaloneCaptureScript& captureScript,
    bool captureMode,
    const CreativeEditorGizmoFrame& gizmoFrame);

}  // namespace iggy3d_creative_app
