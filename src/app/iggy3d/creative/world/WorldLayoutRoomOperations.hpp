#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutRoomSplitAxis : std::uint8_t {
  X,
  Z,
  Count,
};

enum class CreativeWorldLayoutRoomOperationStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidSourceTopology,
  DuplicateStableKey,
  CutDoesNotBisectRoom,
  DisconnectedResult,
  RoomsNotAdjacent,
  OpeningConflict,
  ConnectorConflict,
  BoundaryCapacityExceeded,
  ResultingTopologyInvalid,
  NoChange,
  Ready,
};

struct CreativeWorldLayoutRoomSplitRequest {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomSplitAxis axis =
      CreativeWorldLayoutRoomSplitAxis::Count;
  std::int32_t coordinate = 0;
  std::string newRoomStableKey;
  std::string newRoomName;
};

struct CreativeWorldLayoutRoomMergeRequest {
  std::size_t primaryRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryRoomIndex = kInvalidCreativeWorldLayoutIndex;
};

struct CreativeWorldLayoutRoomBoundaryMoveRequest {
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  // Horizontal edges move to this Z coordinate; vertical edges move to X.
  std::int32_t coordinate = 0;
};

struct CreativeWorldLayoutRoomCornerMoveRequest {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t topologyVertexIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeTerrainCoord2 position{};
};

enum class CreativeWorldLayoutRoomEdgeAnchor : std::uint8_t {
  Start,
  End,
  Count,
};

struct CreativeWorldLayoutRoomEdgeSettingsRequest {
  std::size_t topologyEdgeIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRoomEdgeAnchor fixedEndpoint =
      CreativeWorldLayoutRoomEdgeAnchor::Start;
  std::uint32_t lengthCells = 0U;
  double wallThicknessCells =
      kDefaultCreativeWorldLayoutWallThicknessCells;
  std::uint16_t wallHeightCells = 0U;
  CreativeWorldLayoutWallProfile profile =
      CreativeWorldLayoutWallProfile::Automatic;
  CreativeStructuralMaterial material =
      CreativeStructuralMaterial::Blockout;
  CreativeWorldLayoutWallJoinStyle joinStyle =
      CreativeWorldLayoutWallJoinStyle::Square;
};

// Remap vectors are indexed by the source layout. Removed geometry maps to the
// invalid sentinel. A split publishes its appended room separately because one
// source room intentionally produces two edited rooms.
struct CreativeWorldLayoutRoomOperationResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutRoomOperationStatus status =
      CreativeWorldLayoutRoomOperationStatus::NotRequested;
  std::size_t primaryRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedConnectorIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayout edited;
  std::vector<std::size_t> sourceToEditedRoomIndices;
  std::vector<std::size_t> sourceToEditedVertexIndices;
  std::vector<std::size_t> sourceToEditedEdgeIndices;
  std::string reasonCode =
      "creative_world_layout_room_operation_not_requested";
};

// The minimum-coordinate half retains the source room identity. The
// maximum-coordinate half is appended with the supplied stable key/name.
[[nodiscard]] CreativeWorldLayoutRoomOperationResult
splitCreativeWorldLayoutRoom(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomSplitRequest& request);

// The primary room retains its identity and metadata. The secondary room is
// removed; both source room indices remap to the surviving room.
[[nodiscard]] CreativeWorldLayoutRoomOperationResult
mergeCreativeWorldLayoutRooms(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomMergeRequest& request);

// Moves one canonical edge parallel to itself. Every room sharing the edge and
// every incident polygon vertex moves in the same atomic operation. Hosted
// openings retain world placement where possible; connectors must remain
// contained by their owning rooms.
[[nodiscard]] CreativeWorldLayoutRoomOperationResult
moveCreativeWorldLayoutRoomBoundary(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomBoundaryMoveRequest& request);

// Moves the selected room corner by composing its horizontal and vertical
// canonical boundary moves into one atomic result. Both coordinates snap to
// integer grid lines before this kernel is called. All incident identities and
// hosted dependencies follow the same rules as a direct boundary move.
[[nodiscard]] CreativeWorldLayoutRoomOperationResult
moveCreativeWorldLayoutRoomCorner(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomCornerMoveRequest& request);

// Sets exact integer-grid length and wall thickness for one canonical edge.
// One endpoint remains fixed; the opposite endpoint moves through the same
// corner/boundary kernel used by direct manipulation. The complete geometry,
// hosted-opening, connector, and thickness change is atomic.
[[nodiscard]] CreativeWorldLayoutRoomOperationResult
setCreativeWorldLayoutRoomEdgeSettings(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomEdgeSettingsRequest& request);

}  // namespace iggy3d::creative
