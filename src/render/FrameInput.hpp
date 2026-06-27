#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

#include "core/math/Mat4.hpp"
#include "core/math/Vec3.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {

enum class RenderCameraMode : std::uint8_t {
  FirstPerson,
  ThirdPerson,
  TacticalOverhead,
};

struct RenderViewport {
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  float aspectRatio = 1.0F;
};

struct RenderFrameClock {
  std::uint64_t sourceTick = 0;
  std::uint64_t frameIndex = 0;
  float interpolationAlpha = 0.0F;
  float presentationDeltaSeconds = 0.0F;
};

struct RenderCameraFrame {
  RenderCameraMode mode = RenderCameraMode::ThirdPerson;
  Vec3 worldEye;
  Vec3 worldForward;
  Vec3 worldUp;
  Mat4 viewFromWorld = identityMat4();
  Mat4 clipFromView = identityMat4();
  Mat4 clipFromWorld = identityMat4();
  float nearPlane = 0.1F;
  float farPlane = 200.0F;
};

struct RenderSceneFrame {
  const SceneProjectionResult* scene = nullptr;
  const DebugProjectionResult* debug = nullptr;
};

struct RenderUiRect {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  float r = 1.0F;
  float g = 1.0F;
  float b = 1.0F;
  float a = 1.0F;
};

struct RenderUiFrame {
  bool visible = false;
  const RenderUiRect* rects = nullptr;
  std::size_t rectCount = 0;
  const DebugHudGlyphQuad* textGlyphQuads = nullptr;
  std::size_t textGlyphQuadCount = 0;
  std::size_t textGlyphCount = 0;
  std::size_t primitiveCount = 0;
};

struct FrameInput {
  RenderViewport viewport;
  RenderFrameClock clock;
  RenderCameraFrame camera;
  RenderSceneFrame projections;
  RenderUiFrame ui;
};

enum class FrameInputStatus : std::uint8_t {
  Valid,
  NotDrawable,
  MissingSceneProjection,
  InvalidAspectRatio,
  InvalidClock,
  InvalidCameraMode,
  InvalidCameraBasis,
  InvalidCameraMatrix,
  InvalidClipPlanes,
};

FrameInputStatus validateFrameInput(const FrameInput& frame);
std::string_view frameInputReasonCode(FrameInputStatus status);
std::string_view renderCameraModeName(RenderCameraMode mode);

}  // namespace iggy3d
