#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
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

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

struct CreativeEditorSelectionFrame;
struct CreativeEditorSelectionTransformState;
struct CreativeMovingPlatformPathEditState;

enum class GizmoAxis : std::uint8_t { None, X, Y, Z };

struct GizmoAxisShaft {
  GizmoAxis axis = GizmoAxis::None;
  iggy3d::Vec3 tip{0.0F, 0.0F, 0.0F};
  iggy3d::RenderLineColor color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct GizmoAxisScreenHandle {
  GizmoAxis axis = GizmoAxis::None;
  float startX = 0.0F;
  float startY = 0.0F;
  float endX = 0.0F;
  float endY = 0.0F;
  bool valid = false;
};

struct GizmoAxisPickResult {
  GizmoAxis axis = GizmoAxis::None;
  float distanceSquared = 0.0F;
  bool hit = false;
};

inline constexpr float kCreativeEditorGizmoHandleHitPaddingPixels = 12.0F;

[[nodiscard]] GizmoAxisPickResult pickCreativeEditorGizmoAxisAtPixel(
    std::span<const GizmoAxisScreenHandle> handles,
    float pixelX,
    float pixelY,
    float paddingPixels =
        kCreativeEditorGizmoHandleHitPaddingPixels) noexcept;

[[nodiscard]] cr::CreativeToolMoveHeldAxis heldAxisForGrabbedAxis(
    GizmoAxis grabbed);

void logUndoMovePlacement(const char* phase,
                          cr::CreativeObjectId objectId,
                          const cr::CreativeObject* object);
void logMoveDispatch(const char* phase,
                     const cr::CreativeFacadeToolDispatchReceipt& receipt);

[[nodiscard]] cr::CreativeFacadeToolDispatchReceipt dispatchMoveReleaseWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    const cr::CreativeToolInputPacket& release,
    cr::CreativeObjectId objectId,
    std::string_view source);

struct CreativeEditorGizmoFrame {
  iggy3d::Vec3 center{0.0F, 0.0F, 0.0F};
  std::array<GizmoAxisShaft, 3> shafts{};
  std::array<GizmoAxisScreenHandle, 3> axisHandles{};
  std::vector<PathPointHandleHit> pathPointHandleHits;
  iggy3d::creative::CreativeObjectId selectedPathHandleObjectId =
      iggy3d::creative::kInvalidObjectId;
  bool selectedIsPathForHandles = false;
};

[[nodiscard]] CreativeEditorGizmoFrame buildCreativeEditorGizmoFrame(
    const CreativeEditorSelectionFrame& selection,
    const CreativeMovingPlatformPathEditState& pathEdit,
    const iggy3d::RenderCameraFrame& camera,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    float axisLengthMeters,
    iggy3d::RenderContentViewport contentViewport = {},
    const CreativeEditorSelectionTransformState* transform = nullptr);

void logCreativeEditorPathHandleCaptureFrame(
    StandaloneCaptureScript& captureScript,
    bool captureMode,
    const CreativeEditorGizmoFrame& gizmoFrame);

}  // namespace iggy3d_creative_app
