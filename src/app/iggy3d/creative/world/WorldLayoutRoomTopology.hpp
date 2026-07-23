#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace iggy3d::creative {

inline constexpr double
    kCreativeWorldLayoutRoomOpeningEndClearanceCells = 0.25;

enum class CreativeWorldLayoutRoomFootprintEditStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  InvalidSourceTopology,
  InvalidFootprint,
  SharedRoomMoveUnsupported,
  OpeningDoesNotFit,
  OpeningOverlap,
  InteriorWindow,
  VerticalConnectorDoesNotFit,
  ResultingTopologyInvalid,
  NoChange,
  Ready,
};

struct CreativeWorldLayoutRoomFootprintEditRequest {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRect targetFootprint;
  double minimumOpeningEndClearanceCells =
      kCreativeWorldLayoutRoomOpeningEndClearanceCells;
};

struct CreativeWorldLayoutRoomFootprintChange {
  std::size_t roomIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutRect originalFootprint;
  CreativeWorldLayoutRect editedFootprint;
};

struct CreativeWorldLayoutRoomFootprintEditResult {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeWorldLayoutRoomFootprintEditStatus status =
      CreativeWorldLayoutRoomFootprintEditStatus::NotRequested;
  std::size_t sourceRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedRoomIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedOpeningIndex = kInvalidCreativeWorldLayoutIndex;
  std::size_t failedConnectorIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayout edited;
  std::vector<CreativeWorldLayoutRoomFootprintChange> roomChanges;
  std::size_t adjustedOpeningCount = 0U;
  std::string reasonCode =
      "creative_world_layout_room_footprint_edit_not_requested";
};

// Recomputes a room-based building's horizontal semantic extent from its
// rooms. Legacy CreateRoom roots remain authored geometry and are not replaced.
// The return value reports whether the resulting root extent is valid.
[[nodiscard]] bool refreshCreativeWorldLayoutBuildingRoomFootprint(
    CreativeWorldLayout& layout, std::size_t buildingIndex) noexcept;

// Control-path edit. Shared edges are discovered from the canonical room
// compiler, so moving one side updates every room that owns the opposite side.
// O(R^2 + O^2 + V) for R rooms, O openings, and V vertical connectors;
// output order follows source rows.
[[nodiscard]] CreativeWorldLayoutRoomFootprintEditResult
editCreativeWorldLayoutRoomFootprint(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutRoomFootprintEditRequest& request);

}  // namespace iggy3d::creative
