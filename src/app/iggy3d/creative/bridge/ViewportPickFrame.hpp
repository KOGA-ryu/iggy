#pragma once

#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"
#include "app/iggy3d/creative/spatial/ViewportPick.hpp"
#include "app/input/MouseInput.hpp"

#include <cstdint>
#include <cstddef>
#include <string>

namespace iggy3d {

struct ProductAppWindowState;
namespace creative {
struct CreativeAppState;
}  // namespace creative

struct ProductCreativeViewportPickFrameRequest {
  ProductAppWindowState* window = nullptr;
  creative::CreativeAppState* creative = nullptr;
  MouseClick click;
  bool downstreamClickSuppressed = false;
  creative::CreativeViewportPickViewport viewport;
  creative::CreativeSpatialProjectionRequest projectionRequest;
  std::int32_t z = 0;
  creative::CreativeViewportPickDepthMode depthMode =
      creative::CreativeViewportPickDepthMode::FixedZ;
};

struct ProductCreativeViewportPickFrameReceipt {
  bool requested = false;
  bool active = false;
  bool clickPresent = false;
  bool downstreamClickSuppressed = false;
  bool facadeAvailable = false;
  bool sourceAvailable = false;
  bool projected = false;
  bool picked = false;
  std::uint64_t objectCount = 0;
  std::uint64_t projectionCellCount = 0;
  creative::CreativeViewportPickStatus pickStatus =
      creative::CreativeViewportPickStatus::Unknown;
  creative::CreativeGridCoord3 coord;
  creative::CreativeGridIndex gridIndex = 0;
  creative::CreativeObjectId objectId = creative::kInvalidObjectId;
  creative::CreativeObjectKind objectKind =
      creative::CreativeObjectKind::Unknown;
  creative::CreativeSpatialOccupancyKind occupancyKind =
      creative::CreativeSpatialOccupancyKind::Unknown;
  creative::TargetRef target;
  std::size_t cellIndex = 0;
  std::string status = "product_creative_viewport_pick_not_requested";
  std::string reasonCode = "product_creative_viewport_pick_not_requested";
  std::string pickMessage;
};

[[nodiscard]] bool productCreativeViewportPickActiveForWindow(
    const ProductAppWindowState& window) noexcept;
[[nodiscard]] ProductCreativeViewportPickFrameReceipt
routeProductCreativeViewportPickFrame(
    const ProductCreativeViewportPickFrameRequest& request);

}  // namespace iggy3d
