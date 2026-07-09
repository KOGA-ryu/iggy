#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Facade.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

#include "EditorCapture.hpp"
#include "EditorPicking.hpp"
#include "EditorEdits.hpp"

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

struct CreativeEditorState;
struct CreativeEditorSelectionFrame;

enum class GizmoAxis { None, X, Y, Z };

struct GizmoAxisShaft {
  GizmoAxis axis = GizmoAxis::None;
  iggy3d::Vec3 tip{0.0F, 0.0F, 0.0F};
  iggy3d::RenderLineColor color{1.0F, 1.0F, 1.0F, 1.0F};
};

[[nodiscard]] cr::CreativeToolMoveHeldAxis heldAxisForGrabbedAxis(
    GizmoAxis grabbed);
[[nodiscard]] const char* gizmoAxisName(GizmoAxis axis);

void logObjectPlacement(const char* phase, const cr::CreativeObject* object);
void logUndoMovePlacement(const char* phase,
                          cr::CreativeObjectId objectId,
                          const cr::CreativeObject* object);
void logMoveDispatch(const char* phase,
                     const cr::CreativeFacadeToolDispatchReceipt& receipt);

[[nodiscard]] cr::CreativeFacadeToolDispatchReceipt dispatchMoveReleaseWithUndo(
    cr::CreativeAppState& appState,
    StandaloneUndoStack& undoStack,
    const cr::CreativeToolInputPacket& release,
    cr::CreativeObjectId objectId,
    std::string_view source);

[[nodiscard]] GizmoAxis pickGizmoAxisFromProjectedShafts(
    const std::array<GizmoAxisShaft, 3>& shafts,
    ScreenPoint centerScreen,
    const std::array<ScreenPoint, 3>& tipScreens,
    float px,
    float py,
    float thresholdPx);

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

struct CreativeEditorMoveFrameRequest {
  iggy3d::SdlWindow& window;
  cr::CreativeAppState& appState;
  CreativeEditorState& editor;
  const CreativeEditorSelectionFrame& selection;
  const CreativeEditorGizmoFrame& gizmoFrame;
  const iggy3d::RenderCameraFrame& camera;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
  float axisLengthMeters = 0.0F;
  float handleThresholdPx = 0.0F;
  bool captureMode = false;
};

void processCreativeEditorMoveFrame(
    const CreativeEditorMoveFrameRequest& request);

}  // namespace iggy3d_creative_app
