#pragma once

#include <cstdint>

namespace iggy3d {

class SdlWindow;

namespace creative {

struct CreativeAppState;
struct CreativeInputRouteResult;

}  // namespace creative

}  // namespace iggy3d

namespace iggy3d_creative_app {

namespace cr = iggy3d::creative;

struct CreativeEditorState;

struct CreativeEditorTransformFrameRequest {
  iggy3d::SdlWindow& window;
  cr::CreativeAppState& appState;
  CreativeEditorState& editor;
  const cr::CreativeInputRouteResult& routedInput;
  float directionX = 0.0F;
  float directionY = 0.0F;
  std::int32_t nudgeWheelSteps = 0;
  bool fineNudge = false;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeEditorTransformFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
};

[[nodiscard]] CreativeEditorTransformFrameResult
processCreativeEditorTransformFrame(
    const CreativeEditorTransformFrameRequest& request);

}  // namespace iggy3d_creative_app
