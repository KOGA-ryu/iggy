#pragma once

#include "app/iggy3d/creative/adapters/RoomBake.hpp"

namespace iggy3d::creative::room_bake_internal {

struct BakeBounds {
  Vec3 min;
  Vec3 max;
  Vec3 center;
  Vec3 size;
};

enum class BakedRoomRole {
  Unsupported,
  Floor,
  Wall,
  Prop,
};

[[nodiscard]] bool validBakeBounds(CreativeBounds bounds,
                                   BakeBounds& baked) noexcept;
[[nodiscard]] bool occupancySupportsRuntimeRoomGeometry(
    CreativeSpatialOccupancyKind occupancy) noexcept;
[[nodiscard]] bool descriptorSupportsRuntimeRoomGeometry(
    const CreativeObjectDescriptor& descriptor) noexcept;
[[nodiscard]] BakedRoomRole roleForObject(
    const CreativeObjectDescriptor& descriptor,
    Vec3 orientedSize) noexcept;
[[nodiscard]] std::string_view roleName(BakedRoomRole role) noexcept;
[[nodiscard]] std::string_view meshIdForRole(BakedRoomRole role) noexcept;
[[nodiscard]] std::string_view materialIdForRole(BakedRoomRole role) noexcept;

void setWallSegmentFields(RoomStaticMeshAsset& mesh, BakeBounds bounds);
[[nodiscard]] Vec3 blockerNormalForRole(BakeBounds bounds,
                                        BakedRoomRole role) noexcept;
[[nodiscard]] std::vector<Vec3> boxExtentPoints(BakeBounds bounds);
[[nodiscard]] RoomSpatialSurface walkableSurfaceForStableId(
    std::string stableId,
    BakeBounds bounds);
[[nodiscard]] RoomSpatialSurface blockerSurfaceForStableId(
    std::string stableId,
    BakeBounds bounds,
    Vec3 normal,
    bool projectile);
void appendSpatialSurfaceSource(
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    CreativeObjectId objectId,
    const RoomSpatialSurface& surface);

void appendRoomBakeObjects(CreativeRoomBakeResult& result,
                           const CreativeDocument& document,
                           bool includeHidden);
void appendRoomBakeFields(CreativeRoomBakeResult& result,
                          const CreativeRoomBakeRequest& request,
                          const CreativeDocument& document);

}  // namespace iggy3d::creative::room_bake_internal
