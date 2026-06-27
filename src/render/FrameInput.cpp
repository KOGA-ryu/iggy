#include "render/FrameInput.hpp"

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

bool hasUiContent(const RenderUiFrame& ui) {
  return ui.visible &&
         ((ui.rects != nullptr && ui.rectCount > 0U) ||
          (ui.textGlyphQuads != nullptr && ui.textGlyphQuadCount > 0U));
}

}  // namespace

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
  }
  return "frame_input_invalid";
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

  // branch-gate: BG-1080
  if (frame.projections.scene == nullptr && !hasUiContent(frame.ui)) {
    return FrameInputStatus::MissingSceneProjection;
  }

  return FrameInputStatus::Valid;
}

}  // namespace iggy3d
