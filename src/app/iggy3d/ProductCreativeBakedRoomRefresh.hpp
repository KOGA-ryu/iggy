#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "app/iggy3d/creative/adapters/RoomBake.hpp"

namespace iggy3d {

class Session;

using ProductCreativeBakedRoomActivationHook = std::function<void(
    Session&,
    const RoomAsset&,
    const creative::CreativeDocument&)>;

struct ProductCreativeBakedActiveRoomRefreshRequest {
  std::string roomId = "iggy3d_creative_baked_room";
  std::string sourceName = "iggy3d.creative";
  std::string sourceSubset = "creative_document_bake";
  bool includeHidden = false;
  bool clearOnNoRenderable = false;
  ProductCreativeBakedRoomActivationHook activationHook;
};

struct ProductCreativeBakedActiveRoomRefreshResult {
  bool accepted = false;
  bool clearedActiveRoom = false;
  std::string status = "product_creative_baked_room_not_requested";
  std::string reasonCode = "product_creative_baked_room_not_requested";
  creative::CreativeDocumentId documentId = creative::kInvalidDocumentId;
  std::uint64_t objectCount = 0;
  bool bakeMeasured = false;
  std::uint64_t bakeElapsedMicroseconds = 0;
  std::uint64_t bakedDocumentRevision = 0;
  creative::CreativeRoomBakeReceipt bakeReceipt;
  std::uint64_t staticMeshCount = 0;
  std::uint64_t anchorCount = 0;
  std::uint64_t spatialSurfaceCount = 0;
  std::uint64_t staticMeshSourceCount = 0;
  std::uint64_t anchorSourceCount = 0;
  std::uint64_t spatialSurfaceSourceCount = 0;
  bool activeRoomLoaded = false;
  std::string activeRoomStatus = "not_loaded";
  bool collisionReady = false;
  std::uint64_t collisionQuerySurfaceCount = 0;
};

}  // namespace iggy3d
