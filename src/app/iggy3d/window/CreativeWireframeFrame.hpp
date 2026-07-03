#pragma once

#include "app/iggy3d/creative/DocumentWireframe.hpp"
#include "app/iggy3d/creative/SpatialProjection.hpp"

#include <cstdint>
#include <string>

namespace iggy3d {

struct ProductAppWindowState;
namespace creative {
class Facade;
}  // namespace creative

struct ProductCreativeWireframeFrameRequest {
  ProductAppWindowState* window = nullptr;
  creative::Facade* facade = nullptr;
  creative::CreativeSpatialProjectionRequest projectionRequest;
};

struct ProductCreativeWireframeFrameReceipt {
  bool requested = false;
  bool active = false;
  bool facadeAvailable = false;
  bool documentAvailable = false;
  bool sourceAvailable = false;
  std::uint64_t objectCount = 0;
  std::uint64_t visibleObjectCount = 0;
  std::uint64_t itemCount = 0;
  std::uint64_t segmentCount = 0;
  std::uint64_t boxItemCount = 0;
  std::uint64_t lineItemCount = 0;
  std::uint64_t pointItemCount = 0;
  std::uint64_t skippedDegenerateCount = 0;
  creative::CreativeDocumentWireframeStatus wireframeStatus =
      creative::CreativeDocumentWireframeStatus::Unknown;
  std::string wireframeReasonCode = "none";
  creative::CreativeDocumentWireframeSegmentStatus segmentStatus =
      creative::CreativeDocumentWireframeSegmentStatus::Unknown;
  std::string segmentReasonCode = "none";
  std::string status = "creative_wireframe_frame_not_requested";
  std::string reasonCode = "creative_wireframe_frame_not_requested";
};

[[nodiscard]] bool productCreativeWireframeFrameActiveForWindow(
    const ProductAppWindowState& window) noexcept;
[[nodiscard]] ProductCreativeWireframeFrameReceipt
routeProductCreativeWireframeFrame(
    const ProductCreativeWireframeFrameRequest& request);

}  // namespace iggy3d
