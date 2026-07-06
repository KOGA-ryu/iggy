#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "content/assets/RoomAsset.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeRoomBakeStatus : std::uint8_t {
  Unknown,
  MissingDocument,
  InvalidDocument,
  NoRenderableObjects,
  Baked,
};

struct CreativeRoomBakeRequest {
  const CreativeDocument* document = nullptr;
  std::string roomId;
  std::string sourceName;
  std::string sourceSubset = "creative_document";
  bool includeHidden = false;
};

struct CreativeRoomBakeReceipt {
  bool requested = false;
  bool accepted = false;
  std::uint64_t objectCount = 0;
  std::uint64_t consideredObjectCount = 0;
  std::uint64_t bakedStaticMeshCount = 0;
  std::uint64_t bakedSpatialSurfaceCount = 0;
  std::uint64_t skippedHiddenCount = 0;
  std::uint64_t skippedEditorOnlyCount = 0;
  std::uint64_t skippedNoBoundsCount = 0;
  std::uint64_t skippedUnsupportedShapeCount = 0;
  std::uint64_t skippedRoomMetadataCount = 0;
  CreativeRoomBakeStatus status = CreativeRoomBakeStatus::Unknown;
  std::string reasonCode = "creative_room_bake_not_requested";
  std::string message = "creative_room_bake_not_requested";
};

struct CreativeRoomBakeResult {
  RoomAsset room;
  CreativeRoomBakeReceipt receipt;
};

[[nodiscard]] std::string_view toString(CreativeRoomBakeStatus status) noexcept;

[[nodiscard]] CreativeRoomBakeResult buildRoomAssetFromCreativeDocument(
    const CreativeRoomBakeRequest& request);

}  // namespace iggy3d::creative
