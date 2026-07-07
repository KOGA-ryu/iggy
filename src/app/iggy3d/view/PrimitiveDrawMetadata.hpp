#pragma once

#include "app/iggy3d/view/PrimitiveDrawList.hpp"

#include <array>
#include <span>

namespace iggy3d {

struct ProductPrimitiveDrawKindMetadata {
  ProductPrimitiveDrawKind kind = ProductPrimitiveDrawKind::DebugMarker;
  ProductPrimitiveColor baseColor;
  float markerSize = 14.0F;
};

inline constexpr std::array<ProductPrimitiveDrawKindMetadata, 22U>
    kProductPrimitiveDrawKindMetadata = {{
        {ProductPrimitiveDrawKind::PlayerMarker, {80, 170, 236}, 26.0F},
        {ProductPrimitiveDrawKind::NpcMarker, {210, 78, 76}, 28.0F},
        {ProductPrimitiveDrawKind::PickupMarker, {229, 196, 72}, 20.0F},
        {ProductPrimitiveDrawKind::InteractableMarker, {198, 142, 222}, 22.0F},
        {ProductPrimitiveDrawKind::ObjectiveMarker, {126, 201, 176}, 18.0F},
        {ProductPrimitiveDrawKind::TacticalMarker, {126, 201, 176}, 18.0F},
        {ProductPrimitiveDrawKind::DebugMarker, {112, 118, 120}, 14.0F},
        {ProductPrimitiveDrawKind::PlayerFocusIndicator, {226, 230, 211}, 36.0F},
        {ProductPrimitiveDrawKind::DoorMarker, {220, 178, 86}, 24.0F},
        {ProductPrimitiveDrawKind::FloorTile, {54, 78, 68}, 58.0F},
        {ProductPrimitiveDrawKind::ElevatedFloorTile, {92, 126, 102}, 58.0F},
        {ProductPrimitiveDrawKind::RampTile, {82, 139, 156}, 58.0F},
        {ProductPrimitiveDrawKind::BlockedSlopeTile, {184, 82, 74}, 58.0F},
        {ProductPrimitiveDrawKind::WallTile, {76, 86, 92}, 62.0F},
        {ProductPrimitiveDrawKind::PropTile, {151, 102, 58}, 42.0F},
        {ProductPrimitiveDrawKind::RoomEditorCursor, {245, 214, 96}, 30.0F},
        {ProductPrimitiveDrawKind::RoomEditorPlacementPreview, {105, 205, 228}, 52.0F},
        {ProductPrimitiveDrawKind::PhysicsAabbDebug, {105, 205, 228}, 34.0F},
        {ProductPrimitiveDrawKind::PhysicsContactNormalDebug, {236, 118, 86}, 18.0F},
        {ProductPrimitiveDrawKind::PhysicsBroadphasePairDebug, {166, 184, 177}, 14.0F},
        {ProductPrimitiveDrawKind::MapMakerGridDot, {86, 130, 172}, 8.0F},
        {ProductPrimitiveDrawKind::MapMakerCubePreview, {126, 221, 186}, 54.0F},
    }};

[[nodiscard]] inline std::span<const ProductPrimitiveDrawKindMetadata>
productPrimitiveDrawKindMetadataCatalog() noexcept {
  return kProductPrimitiveDrawKindMetadata;
}

[[nodiscard]] inline const ProductPrimitiveDrawKindMetadata*
findProductPrimitiveDrawKindMetadata(ProductPrimitiveDrawKind kind) noexcept {
  for (const ProductPrimitiveDrawKindMetadata& metadata :
       kProductPrimitiveDrawKindMetadata) {
    if (metadata.kind == kind) {
      return &metadata;
    }
  }
  return nullptr;
}

[[nodiscard]] inline ProductPrimitiveColor baseColorForProductPrimitiveDrawKind(
    ProductPrimitiveDrawKind kind) noexcept {
  const ProductPrimitiveDrawKindMetadata* metadata =
      findProductPrimitiveDrawKindMetadata(kind);
  return metadata != nullptr ? metadata->baseColor : ProductPrimitiveColor{112, 118, 120};
}

[[nodiscard]] inline float markerSizeForProductPrimitiveDrawKind(
    ProductPrimitiveDrawKind kind) noexcept {
  const ProductPrimitiveDrawKindMetadata* metadata =
      findProductPrimitiveDrawKindMetadata(kind);
  return metadata != nullptr ? metadata->markerSize : 14.0F;
}

}  // namespace iggy3d
