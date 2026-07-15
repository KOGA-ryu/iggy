#pragma once

#include <array>
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

// Sub-rectangle of the swapchain-sized viewport that the 3D scene occupies,
// in drawable-pixel space. The all-zero default is the full-frame sentinel
// (content rect == viewport), so every existing producer is unaffected and
// --capture stays byte-identical. Distinct from RenderViewport, which remains
// swapchain-truth and also drives swapchain recreation; only camera aspect and
// picking/overlay math read the content rect. See
// docs/creative_desktop_ui_plan.md DD-5.
struct RenderContentViewport {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
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

struct RenderLineColor {
  float r = 1.0F;
  float g = 1.0F;
  float b = 1.0F;
  float a = 1.0F;
};

struct RenderCreativeWireframeDebugLine {
  Vec3 start;
  Vec3 end;
  RenderLineColor color;
  std::uint64_t objectId = 0;
  std::uint32_t objectKind = 0;
  std::uint32_t style = 0;
  std::uint32_t segmentKind = 0;
  float thickness = 1.0F;
};

struct RenderCreativeWireframeDebugFrame {
  bool visible = false;
  bool available = false;
  const RenderCreativeWireframeDebugLine* lines = nullptr;
  std::size_t lineCount = 0;
};

enum class RenderCreativePreviewRole : std::uint8_t {
  Held,
  PlacementValid,
  PlacementInvalid,
  Count,
};

inline constexpr std::size_t kRenderCreativePreviewRoleCount =
    static_cast<std::size_t>(RenderCreativePreviewRole::Count);
inline constexpr std::size_t kRenderCreativePreviewCapacity = 2U;
inline constexpr std::size_t kRenderCreativePreviewAssetIdCapacity = 128U;

struct RenderCreativePreviewItem {
  RenderCreativePreviewRole role = RenderCreativePreviewRole::Held;
  Mat4 clipFromModel = identityMat4();
  bool includePathWireframe = false;
  std::array<char, kRenderCreativePreviewAssetIdCapacity + 1U> assetId{};
};

struct RenderCreativePreviewFrame {
  std::array<RenderCreativePreviewItem, kRenderCreativePreviewCapacity> items{};
  std::uint8_t itemCount = 0;
};

struct FrameInput {
  RenderViewport viewport;
  RenderContentViewport contentViewport;
  RenderFrameClock clock;
  RenderCameraFrame camera;
  RenderSceneFrame projections;
  RenderUiFrame ui;
  RenderCreativeWireframeDebugFrame creativeWireframeDebug;
  RenderCreativePreviewFrame creativePreview;
};

// True for the all-zero sentinel that means "content rect == full viewport".
[[nodiscard]] bool isFullFrameContentViewport(
    const RenderContentViewport& rect) noexcept;
// Resolves the sentinel to the full viewport rect; otherwise returns the
// explicit content rect. Only call on a frame that passed validateFrameInput.
[[nodiscard]] RenderContentViewport effectiveContentViewport(
    const FrameInput& frame) noexcept;

[[nodiscard]] std::string_view renderCreativePreviewAssetId(
    const RenderCreativePreviewItem& item) noexcept;
[[nodiscard]] bool setRenderCreativePreviewAssetId(
    RenderCreativePreviewItem& item,
    std::string_view assetId) noexcept;

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
  InvalidCreativeWireframeDebugLines,
  InvalidCreativePreviewItems,
  InvalidContentViewport,
};

FrameInputStatus validateFrameInput(const FrameInput& frame);
std::string_view frameInputReasonCode(FrameInputStatus status);
std::string_view renderCameraModeName(RenderCameraMode mode);

}  // namespace iggy3d
