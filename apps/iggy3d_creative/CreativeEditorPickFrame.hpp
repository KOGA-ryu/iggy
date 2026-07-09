#pragma once

#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include "CreativeEditorState.hpp"
#include "StandaloneCaptureScript.hpp"
#include "StandalonePicking.hpp"

namespace iggy3d_creative_app {

struct CreativeEditorPickFrame {
  std::vector<ObjectVisualPickBounds> objectPickCandidates;
  bool haveFloorBounds = false;
  iggy3d::Vec3 floorBoxMin{};
  iggy3d::Vec3 floorBoxMax{};
};

[[nodiscard]] CreativeEditorPickFrame buildCreativeEditorPickFrame(
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    iggy3d::creative::CreativeObjectId floorObjectId,
    StandaloneCaptureScript& captureScript,
    bool captureMode);

void logCreativeEditorWorldPickProofFrame(
    const iggy3d::creative::Facade& facade,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    const CreativeEditorPickFrame& pickFrame,
    iggy3d::creative::CreativeObjectId floorObjectId,
    CreativeEditorState& editor,
    bool captureMode);

}  // namespace iggy3d_creative_app
