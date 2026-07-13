#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeInternal.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeReachability.hpp"

#include <string>
#include <utility>

namespace iggy3d::creative {

bool creativeRoomBakeBoundsAreValid(CreativeBounds bounds) noexcept {
  const CreativeBoundsMetrics metrics = measureCreativeBounds(bounds);
  if (!metrics.valid || !isPositiveCreativeVec3(metrics.size)) {
    return false;
  }
  return creativeVec3ToCoreChecked(bounds.min).converted &&
         creativeVec3ToCoreChecked(bounds.max).converted &&
         creativeVec3ToCoreChecked(metrics.center).converted &&
         creativeVec3ToCoreChecked(metrics.size).converted;
}

namespace {

void setStatus(CreativeRoomBakeReceipt& receipt,
               CreativeRoomBakeStatus status,
               std::string reasonCode,
               bool accepted = false) {
  receipt.status = status;
  receipt.reasonCode = std::move(reasonCode);
  receipt.message = receipt.reasonCode;
  receipt.accepted = accepted;
}

}  // namespace

std::string_view toString(CreativeRoomBakeStatus status) noexcept {
  switch (status) {
    case CreativeRoomBakeStatus::Unknown:
      return "unknown";
    case CreativeRoomBakeStatus::MissingDocument:
      return "missing_document";
    case CreativeRoomBakeStatus::InvalidDocument:
      return "invalid_document";
    case CreativeRoomBakeStatus::NoRenderableObjects:
      return "no_renderable_objects";
    case CreativeRoomBakeStatus::Baked:
      return "baked";
  }
  return "unknown";
}

std::string_view toString(
    CreativeRoomBakeReachabilityStatus status) noexcept {
  switch (status) {
    case CreativeRoomBakeReachabilityStatus::Unknown:
      return "unknown";
    case CreativeRoomBakeReachabilityStatus::NotRequested:
      return "not_requested";
    case CreativeRoomBakeReachabilityStatus::NotChecked:
      return "not_checked";
    case CreativeRoomBakeReachabilityStatus::InvalidCellSize:
      return "invalid_cell_size";
    case CreativeRoomBakeReachabilityStatus::NoWalkableCells:
      return "no_walkable_cells";
    case CreativeRoomBakeReachabilityStatus::GridTooLarge:
      return "grid_too_large";
    case CreativeRoomBakeReachabilityStatus::NoUsableSeeds:
      return "no_usable_seeds";
    case CreativeRoomBakeReachabilityStatus::Reachable:
      return "reachable";
    case CreativeRoomBakeReachabilityStatus::IslandsFound:
      return "islands_found";
  }
  return "unknown";
}

CreativeRoomBakeResult buildRoomAssetFromCreativeDocument(
    const CreativeRoomBakeRequest& request) {
  CreativeRoomBakeResult result;
  result.reachability =
      initialCreativeRoomBakeReachabilityReceipt(request);
  result.receipt.requested = true;
  result.room.id = request.roomId.empty() ? "creative_room" : request.roomId;
  result.room.version = 1;
  result.room.units = "m";
  result.room.source = "iggy3d.creative_document";
  result.room.sourceFile = request.sourceName;
  result.room.sourceSubset = request.sourceSubset;

  if (request.document == nullptr) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::MissingDocument,
              "creative_room_bake_document_missing");
    return result;
  }

  const CreativeDocument& document = *request.document;
  if (!document.isValid()) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::InvalidDocument,
              "creative_room_bake_document_invalid");
    return result;
  }

  result.receipt.objectCount = document.objectCount();
  result.receipt.voxelCellCount = document.voxelField().occupiedCellCount();
  result.receipt.voxelChunkCount = document.voxelField().chunkCount();
  room_bake_internal::appendRoomBakeObjects(
      result, document, request.includeHidden);
  room_bake_internal::appendRoomBakeFields(result, request, document);

  result.receipt.bakedStaticMeshCount = result.room.staticMeshes.size();
  result.receipt.bakedAnchorCount = result.room.anchors.size();
  result.receipt.bakedSpatialSurfaceCount = result.room.spatialSurfaces.size();
  result.reachability =
      validateCreativeRoomBakeReachability(result.room, request);

  if (result.room.staticMeshes.empty() && result.room.anchors.empty()) {
    setStatus(result.receipt,
              CreativeRoomBakeStatus::NoRenderableObjects,
              "creative_room_bake_no_renderable_objects");
    return result;
  }

  setStatus(result.receipt,
            CreativeRoomBakeStatus::Baked,
            "creative_room_baked",
            true);
  return result;
}

}  // namespace iggy3d::creative
