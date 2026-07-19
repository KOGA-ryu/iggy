#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "render/debug/DebugHudText.hpp"
#include "render/vulkan/ProjectileOverlayProjection.hpp"
#include "render/vulkan/RenderLoop.hpp"

namespace iggy3d::vulkan {

struct RenderLoopProxySceneFacts {
  bool targetMarkerVisible = false;
  bool objectiveMarkerVisible = false;
  bool keyMarkerVisible = false;
  bool dummyMarkerVisible = false;
  std::uint32_t markerCount = 0;
};

struct RenderLoopFramePlan {
  DebugHudLayoutResult debugHud;
  ProjectileOverlayLayout projectileOverlay;
  std::vector<OverlayRect> uiOverlayRects;
  std::array<CreativePreviewDrawInfo, kRenderCreativePreviewCapacity>
      creativePreviewDraws{};
  std::size_t creativePreviewDrawCount = 0;
  bool drawPackageRoom = false;
  bool drawUiFrame = false;
  bool drawProxyPrimitives = false;
  bool drawFirstRoom = false;
  RenderLoopProxySceneFacts proxyFacts;
};

FirstRoomPushConstants renderLoopPushConstantsFromMat4(const Mat4& matrix);

RenderLoopFramePlan buildRenderLoopFramePlan(
    const RenderLoopCreateInfo& createInfo, const FrameInput& frame,
    bool drawPackageRoom, bool firstRoomReady);

std::uint32_t renderLoopProxyDrawCount(
    const RenderLoopProxySceneFacts& facts) noexcept;

}  // namespace iggy3d::vulkan
