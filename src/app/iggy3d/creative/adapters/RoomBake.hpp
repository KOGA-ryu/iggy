#pragma once

#include "app/iggy3d/creative/document/Document.hpp"
#include "content/assets/RoomAsset.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeRoomBakeStatus : std::uint8_t {
  Unknown,
  MissingDocument,
  InvalidDocument,
  NoRenderableObjects,
  Baked,
};

enum class CreativeRoomBakeReachabilityStatus : std::uint8_t {
  Unknown,
  NotRequested,
  NotChecked,
  InvalidCellSize,
  NoWalkableCells,
  GridTooLarge,
  NoUsableSeeds,
  Reachable,
  IslandsFound,
};

struct CreativeRoomBakeRequest {
  const CreativeDocument* document = nullptr;
  std::string roomId;
  std::string sourceName;
  std::string sourceSubset = "creative_document";
  bool includeHidden = false;
  bool validateReachability = true;
  float reachabilityCellSizeMeters = 1.0F;
};

struct CreativeRoomBakeReceipt {
  bool requested = false;
  bool accepted = false;
  std::uint64_t objectCount = 0;
  std::uint64_t consideredObjectCount = 0;
  std::uint64_t bakedStaticMeshCount = 0;
  std::uint64_t bakedAnchorCount = 0;
  std::uint64_t bakedSpatialSurfaceCount = 0;
  std::uint64_t skippedHiddenCount = 0;
  std::uint64_t skippedEditorOnlyCount = 0;
  std::uint64_t skippedNoBoundsCount = 0;
  std::uint64_t skippedUnsupportedAnchorCount = 0;
  std::uint64_t skippedUnsupportedShapeCount = 0;
  std::uint64_t skippedRoomMetadataCount = 0;
  CreativeRoomBakeStatus status = CreativeRoomBakeStatus::Unknown;
  std::string reasonCode = "creative_room_bake_not_requested";
  std::string message = "creative_room_bake_not_requested";
};

struct CreativeRoomBakeReachabilityReceipt {
  bool requested = false;
  bool checked = false;
  bool hasIslands = false;
  std::uint32_t walkableCellCount = 0;
  std::uint32_t reachedCellCount = 0;
  std::uint32_t strandedCellCount = 0;
  std::uint32_t seedAnchorCount = 0;
  std::uint32_t usableSeedCount = 0;
  std::uint32_t blockedSeedCount = 0;
  float cellSizeMeters = 1.0F;
  CreativeRoomBakeReachabilityStatus status =
      CreativeRoomBakeReachabilityStatus::Unknown;
  std::string reasonCode = "creative_room_bake_reachability_not_requested";
  std::string message = "creative_room_bake_reachability_not_requested";
};

struct CreativeRoomBakeStaticMeshSource {
  // Greedy/static bake passes may merge multiple CreativeObjects into one
  // RoomStaticMeshAsset. In that case multiple records share staticMeshId.
  CreativeObjectId objectId{kInvalidObjectId};
  std::string staticMeshId;
};

struct CreativeRoomBakeAnchorSource {
  CreativeObjectId objectId{kInvalidObjectId};
  std::string anchorId;
};

struct CreativeRoomBakeSpatialSurfaceSource {
  CreativeObjectId objectId{kInvalidObjectId};
  std::string surfaceId;
  std::string sourceStaticMeshId;
};

struct CreativeRoomBakeResult {
  RoomAsset room;
  CreativeRoomBakeReceipt receipt;
  CreativeRoomBakeReachabilityReceipt reachability;
  std::vector<CreativeRoomBakeStaticMeshSource> staticMeshSources;
  std::vector<CreativeRoomBakeAnchorSource> anchorSources;
  std::vector<CreativeRoomBakeSpatialSurfaceSource> spatialSurfaceSources;
};

[[nodiscard]] std::string_view toString(CreativeRoomBakeStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeRoomBakeReachabilityStatus status) noexcept;

[[nodiscard]] bool creativeRoomBakeBoundsAreValid(
    CreativeBounds bounds) noexcept;

[[nodiscard]] CreativeRoomBakeResult buildRoomAssetFromCreativeDocument(
    const CreativeRoomBakeRequest& request);

}  // namespace iggy3d::creative
