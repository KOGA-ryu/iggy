#include "render/FrameInput.hpp"

#include <algorithm>
#include <cmath>

namespace iggy3d {
namespace {

bool isValidCameraMode(RenderCameraMode mode) {
  switch (mode) {
    case RenderCameraMode::FirstPerson:
    case RenderCameraMode::ThirdPerson:
    case RenderCameraMode::TacticalOverhead:
      return true;
  }
  return false;
}

bool isFiniteScalar(float value) {
  return std::isfinite(value);
}

bool isNonZeroFiniteVector(Vec3 value) {
  return isFinite(value) && lengthSquared(value) > 0.0F;
}

bool isValidCreativePreviewRole(RenderCreativePreviewRole role) {
  switch (role) {
    case RenderCreativePreviewRole::Held:
    case RenderCreativePreviewRole::PlacementValid:
    case RenderCreativePreviewRole::PlacementInvalid:
    case RenderCreativePreviewRole::MovingPlatformRoute:
      return true;
    case RenderCreativePreviewRole::Count:
      return false;
  }
  return false;
}

bool isValidCreativePreviewGeometry(
    const RenderCreativePreviewItem& item) noexcept {
  const bool hasAsset = !renderCreativePreviewAssetId(item).empty();
  switch (item.geometryProfile) {
    case RenderCreativePreviewGeometryProfile::Box:
      return item.proceduralSegmentCount == 0U;
    case RenderCreativePreviewGeometryProfile::RampWedge:
      return !hasAsset && item.proceduralSegmentCount == 0U;
    case RenderCreativePreviewGeometryProfile::StairSteps:
      return !hasAsset && item.proceduralSegmentCount > 0U &&
             item.proceduralSegmentCount <=
                 kRenderCreativePreviewMaximumStairSegmentCount;
    case RenderCreativePreviewGeometryProfile::Count:
      return false;
  }
  return false;
}

bool isValidCreativePreviewAssetId(
    const RenderCreativePreviewItem& item) noexcept {
  const std::string_view assetId = renderCreativePreviewAssetId(item);
  if (assetId.empty()) {
    return item.assetId.front() == '\0';
  }
  return item.assetId.back() == '\0' && assetId.front() != '/' &&
         assetId.find("..") == std::string_view::npos &&
         std::all_of(assetId.begin(), assetId.end(), [](char value) {
           const unsigned char c = static_cast<unsigned char>(value);
           return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '/';
         });
}

bool hasUiContent(const RenderUiFrame& ui) {
  return ui.visible &&
         ((ui.rects != nullptr && ui.rectCount > 0U) ||
          (ui.textGlyphQuads != nullptr && ui.textGlyphQuadCount > 0U));
}

}  // namespace

std::string_view renderCreativePreviewAssetId(
    const RenderCreativePreviewItem& item) noexcept {
  const auto terminator =
      std::find(item.assetId.begin(), item.assetId.end(), '\0');
  return {item.assetId.data(),
          static_cast<std::size_t>(terminator - item.assetId.begin())};
}

bool setRenderCreativePreviewAssetId(RenderCreativePreviewItem& item,
                                     std::string_view assetId) noexcept {
  if (assetId.empty()) {
    item.assetId.fill('\0');
    return true;
  }
  if (assetId.size() > kRenderCreativePreviewAssetIdCapacity ||
      assetId.front() == '/' || assetId.find("..") != std::string_view::npos ||
      !std::all_of(assetId.begin(), assetId.end(), [](char value) {
        const unsigned char c = static_cast<unsigned char>(value);
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '/';
      })) {
    return false;
  }
  item.assetId.fill('\0');
  std::copy(assetId.begin(), assetId.end(), item.assetId.begin());
  return true;
}

std::string_view renderCameraModeName(RenderCameraMode mode) {
  switch (mode) {
    case RenderCameraMode::FirstPerson:
      return "first_person";
    case RenderCameraMode::ThirdPerson:
      return "third_person";
    case RenderCameraMode::TacticalOverhead:
      return "tactical_overhead";
  }
  return "invalid";
}

std::string_view frameInputReasonCode(FrameInputStatus status) {
  switch (status) {
    case FrameInputStatus::Valid:
      return "frame_input_valid";
    case FrameInputStatus::NotDrawable:
      return "frame_not_drawable";
    case FrameInputStatus::MissingSceneProjection:
      return "frame_scene_missing";
    case FrameInputStatus::InvalidAspectRatio:
      return "frame_aspect_invalid";
    case FrameInputStatus::InvalidClock:
      return "frame_clock_invalid";
    case FrameInputStatus::InvalidCameraMode:
      return "frame_camera_mode_invalid";
    case FrameInputStatus::InvalidCameraBasis:
      return "frame_camera_basis_invalid";
    case FrameInputStatus::InvalidCameraMatrix:
      return "frame_camera_matrix_invalid";
    case FrameInputStatus::InvalidClipPlanes:
      return "frame_clip_planes_invalid";
    case FrameInputStatus::InvalidCreativeWireframeDebugLines:
      return "frame_creative_wireframe_debug_lines_invalid";
    case FrameInputStatus::InvalidCreativePreviewItems:
      return "frame_creative_preview_items_invalid";
    case FrameInputStatus::InvalidContentViewport:
      return "frame_content_viewport_invalid";
  }
  return "frame_input_invalid";
}

bool isFullFrameContentViewport(const RenderContentViewport& rect) noexcept {
  return rect.x == 0 && rect.y == 0 && rect.width == 0U && rect.height == 0U;
}

RenderContentViewport effectiveContentViewport(const FrameInput& frame) noexcept {
  if (isFullFrameContentViewport(frame.contentViewport)) {
    return {0, 0, frame.viewport.width, frame.viewport.height};
  }
  return frame.contentViewport;
}

FrameInputStatus validateFrameInput(const FrameInput& frame) {
  if (frame.viewport.width == 0U || frame.viewport.height == 0U) {
    return FrameInputStatus::NotDrawable;
  }
  if (!isFiniteScalar(frame.viewport.aspectRatio)) {
    return FrameInputStatus::InvalidAspectRatio;
  }
  const float expectedAspect = static_cast<float>(frame.viewport.width) /
                               static_cast<float>(frame.viewport.height);
  if (std::fabs(frame.viewport.aspectRatio - expectedAspect) > 0.001F) {
    return FrameInputStatus::InvalidAspectRatio;
  }

  // Content viewport: the all-zero sentinel is full-frame; any explicit rect
  // must be non-zero, non-negative, and fit inside the swapchain viewport.
  // (Producers must clamp a transient zero-sized dock node to full-frame
  // before it reaches here — plan DD-5.)
  if (!isFullFrameContentViewport(frame.contentViewport)) {
    const RenderContentViewport& rect = frame.contentViewport;
    if (rect.x < 0 || rect.y < 0 || rect.width == 0U || rect.height == 0U) {
      return FrameInputStatus::InvalidContentViewport;
    }
    const std::uint64_t right =
        static_cast<std::uint64_t>(rect.x) + rect.width;
    const std::uint64_t bottom =
        static_cast<std::uint64_t>(rect.y) + rect.height;
    if (right > frame.viewport.width || bottom > frame.viewport.height) {
      return FrameInputStatus::InvalidContentViewport;
    }
  }

  if (!isFiniteScalar(frame.clock.interpolationAlpha) ||
      frame.clock.interpolationAlpha < 0.0F || frame.clock.interpolationAlpha > 1.0F ||
      !isFiniteScalar(frame.clock.presentationDeltaSeconds) ||
      frame.clock.presentationDeltaSeconds < 0.0F) {
    return FrameInputStatus::InvalidClock;
  }

  if (!isValidCameraMode(frame.camera.mode)) {
    return FrameInputStatus::InvalidCameraMode;
  }

  if (!isFinite(frame.camera.worldEye) || !isNonZeroFiniteVector(frame.camera.worldForward) ||
      !isNonZeroFiniteVector(frame.camera.worldUp)) {
    return FrameInputStatus::InvalidCameraBasis;
  }

  if (!isFinite(frame.camera.viewFromWorld) || !isFinite(frame.camera.clipFromView) ||
      !isFinite(frame.camera.clipFromWorld)) {
    return FrameInputStatus::InvalidCameraMatrix;
  }

  if (!isFiniteScalar(frame.camera.nearPlane) || !isFiniteScalar(frame.camera.farPlane) ||
      frame.camera.nearPlane <= 0.0F || frame.camera.farPlane <= frame.camera.nearPlane) {
    return FrameInputStatus::InvalidClipPlanes;
  }

  if (frame.creativeWireframeDebug.lineCount > 0U &&
      frame.creativeWireframeDebug.lines == nullptr) {
    return FrameInputStatus::InvalidCreativeWireframeDebugLines;
  }

  if (frame.creativePreview.itemCount > kRenderCreativePreviewCapacity) {
    return FrameInputStatus::InvalidCreativePreviewItems;
  }
  for (std::size_t index = 0; index < frame.creativePreview.itemCount;
       ++index) {
    const RenderCreativePreviewItem& item =
        frame.creativePreview.items[index];
    if (!isValidCreativePreviewRole(item.role) ||
        !isFinite(item.clipFromModel) ||
        !isValidCreativePreviewAssetId(item) ||
        !isValidCreativePreviewGeometry(item)) {
      return FrameInputStatus::InvalidCreativePreviewItems;
    }
  }

  // branch-gate: BG-1080
  if (frame.projections.scene == nullptr && !hasUiContent(frame.ui)) {
    return FrameInputStatus::MissingSceneProjection;
  }

  return FrameInputStatus::Valid;
}

}  // namespace iggy3d
