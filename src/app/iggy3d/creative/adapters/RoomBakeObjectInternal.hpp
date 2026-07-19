#pragma once

#include "app/iggy3d/creative/adapters/RoomBakeAssetSurfaces.hpp"
#include "app/iggy3d/creative/adapters/RoomBakeInternal.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative::room_bake_internal {

enum class RoomBakeObjectDecision {
  SkipHidden,
  SkipEditorOnly,
  SkipRoomMetadata,
  SkipUnsupportedAnchor,
  BakeAnchor,
  SkipUnsupportedShape,
  SkipNoBounds,
  BakeStaticMesh,
};

struct RoomBakeObjectClassification {
  RoomBakeObjectDecision decision{RoomBakeObjectDecision::SkipUnsupportedShape};
  bool countedAsConsidered{true};
  BakeBounds bounds{};
  Vec3 orientedSize{};
  CreativeTransformedBounds transformedBounds{};
  std::uint16_t proceduralSegmentCount{0};
  BakedRoomRole role{BakedRoomRole::Unsupported};
  std::string_view anchorKind{};
  RoomBakeAssetSurfaceClassification assetSurfaces;
};

[[nodiscard]] RoomBakeObjectClassification skippedClassification(
    RoomBakeObjectDecision decision,
    bool countedAsConsidered) noexcept;

struct BakeStaticMeshEntry {
  const CreativeObject* object = nullptr;
  const CreativeObjectDescriptor* descriptor = nullptr;
  RoomBakeObjectClassification classification;
  std::size_t documentIndex = 0;
};

[[nodiscard]] bool validBakeBounds(
    CreativeBounds bounds,
    BakeBounds& baked) noexcept;
[[nodiscard]] bool uprightRotation(CreativeVec3 rotation) noexcept;
[[nodiscard]] bool identityRotation(CreativeVec3 rotation) noexcept;
[[nodiscard]] std::string_view roleName(BakedRoomRole role) noexcept;
[[nodiscard]] std::string_view meshIdForRole(BakedRoomRole role) noexcept;
[[nodiscard]] std::string_view materialIdForRole(BakedRoomRole role) noexcept;
[[nodiscard]] std::string stableObjectId(
    CreativeObjectId objectId,
    std::string_view suffix = {});
[[nodiscard]] std::string stableObjectId(
    const CreativeObject& object,
    std::string_view suffix = {});
[[nodiscard]] Vec3 blockerNormalForRole(
    BakeBounds bounds,
    BakedRoomRole role) noexcept;
[[nodiscard]] RoomSpatialSurface walkableSurfaceForObject(
    CreativeObjectId objectId,
    BakeBounds bounds,
    std::string sourceStaticMeshId);
void appendSpatialSurfaceSource(
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    CreativeObjectId objectId,
    const RoomSpatialSurface& surface);
void appendSpatialSurfaces(
    RoomAsset& room,
    std::vector<CreativeRoomBakeSpatialSurfaceSource>& sources,
    CreativeRoomBakeReceipt& receipt,
    const CreativeObject& object,
    const CreativeObjectDescriptor& descriptor,
    const RoomBakeObjectClassification& classification);

}  // namespace iggy3d::creative::room_bake_internal
