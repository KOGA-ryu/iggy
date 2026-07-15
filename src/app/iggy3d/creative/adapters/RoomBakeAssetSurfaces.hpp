#pragma once

#include "app/iggy3d/creative/adapters/RoomBakeInternal.hpp"

namespace iggy3d::creative::room_bake_internal {

enum class RoomBakeAssetSurfacePolicy {
  Descriptor,
  Bounds,
  CompoundBounds,
  None,
  MissingMetadata,
  UnsupportedCollision,
  InvalidMetadata,
};

struct RoomBakeAssetSurfaceClassification {
  RoomBakeAssetSurfacePolicy policy{RoomBakeAssetSurfacePolicy::Descriptor};
  bool walkable = false;
  bool walkableTransformUnsupported = false;
  const StaticMeshAssetCatalogEntry* catalogEntry = nullptr;
};

[[nodiscard]] RoomBakeAssetSurfaceClassification
classifyRoomBakeAssetSurfaces(
    const CreativeObject& object,
    const StaticMeshAssetCatalog* catalog) noexcept;

// Returns true when the object is asset-backed and generic descriptor surface
// emission must stop, including explicit no-collision and failure cases.
[[nodiscard]] bool appendRoomBakeAssetSpatialSurfaces(
    RoomAsset& room,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    CreativeRoomBakeReceipt& receipt,
    const CreativeObject& object,
    BakeBounds bounds,
    BakedRoomRole role,
    const RoomBakeAssetSurfaceClassification& classification);

}  // namespace iggy3d::creative::room_bake_internal
