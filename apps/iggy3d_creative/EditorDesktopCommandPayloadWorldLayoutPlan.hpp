#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "EditorWorldLayoutState.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopWorldLayoutLevelOperationPayload {
  CreativeEditorWorldLayoutLevelOperation operation =
      CreativeEditorWorldLayoutLevelOperation::Select;
  std::size_t buildingIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutLevelSettingsPayload {
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutLevelSettings settings;
};

struct CreativeDesktopWorldLayoutLevelDatumPayload {
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutLevelEditScope scope =
      CreativeEditorWorldLayoutLevelEditScope::Selected;
  double floorTopLayer = 0.0;
};

struct CreativeDesktopWorldLayoutRoofApertureCreatePayload {
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeStructuralRoofApertureKind kind =
      iggy3d::creative::CreativeStructuralRoofApertureKind::Skylight;
};

struct CreativeDesktopWorldLayoutRoofApertureManipulationPayload {
  CreativeEditorWorldLayoutRoofApertureManipulationPhase phase =
      CreativeEditorWorldLayoutRoofApertureManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutRoofManipulationPayload {
  CreativeEditorWorldLayoutRoofManipulationPhase phase =
      CreativeEditorWorldLayoutRoofManipulationPhase::Begin;
  CreativeEditorWorldLayoutRoofTarget target;
  double coordinateCells = 0.0;
};

struct CreativeDesktopGeneratedLevelSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t levelIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutLevelSettings settings;
};

struct CreativeDesktopWorldLayoutObjectSettingsPayload {
  std::size_t objectIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutObjectSettings settings;
};

struct CreativeDesktopGeneratedRoomSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  std::size_t roomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeEditorWorldLayoutRoomSettings settings;
};

struct CreativeDesktopWorldLayoutRoomSplitPayload {
  std::size_t roomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  iggy3d::creative::CreativeWorldLayoutRoomSplitAxis axis =
      iggy3d::creative::CreativeWorldLayoutRoomSplitAxis::Count;
  std::int32_t coordinate = 0;
};

struct CreativeDesktopWorldLayoutRoomMergePayload {
  std::size_t primaryRoomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryRoomIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutWallSplitPayload {
  std::size_t topologyEdgeIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::uint32_t offsetCells = 0U;
};

struct CreativeDesktopWorldLayoutWallMergePayload {
  std::size_t primaryTopologyEdgeIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::size_t secondaryTopologyEdgeIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
};

struct CreativeDesktopWorldLayoutRoomManipulationPayload {
  CreativeEditorWorldLayoutRoomManipulationPhase phase =
      CreativeEditorWorldLayoutRoomManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutRoomBoundaryManipulationPayload {
  CreativeEditorWorldLayoutRoomBoundaryManipulationPhase phase =
      CreativeEditorWorldLayoutRoomBoundaryManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutRoomCornerManipulationPayload {
  CreativeEditorWorldLayoutRoomCornerManipulationPhase phase =
      CreativeEditorWorldLayoutRoomCornerManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopGeneratedVerticalConnectorSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutVerticalConnectorSettings settings;
};

struct CreativeDesktopWorldLayoutVerticalConnectorManipulationPayload {
  CreativeEditorWorldLayoutVerticalConnectorManipulationPhase phase =
      CreativeEditorWorldLayoutVerticalConnectorManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
  CreativeEditorWorldLayoutVerticalConnectorTarget target;
};

struct CreativeDesktopWorldLayoutBoxSettingsPayload {
  std::size_t boxIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutBoxSettings settings;
};

struct CreativeDesktopWorldLayoutBoxManipulationPayload {
  CreativeEditorWorldLayoutBoxManipulationPhase phase =
      CreativeEditorWorldLayoutBoxManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

struct CreativeDesktopWorldLayoutWallSettingsPayload {
  std::size_t wallIndex =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  CreativeEditorWorldLayoutWallSettings settings;
};

struct CreativeDesktopGeneratedWallSettingsPayload {
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  CreativeEditorWorldLayoutWallSettings settings;
};

struct CreativeDesktopWorldLayoutWallManipulationPayload {
  CreativeEditorWorldLayoutWallManipulationPhase phase =
      CreativeEditorWorldLayoutWallManipulationPhase::Begin;
  CreativeEditorWorldLayoutPoint point;
  double toleranceCells = 0.25;
};

enum class CreativeDesktopWorldLayoutPropertyEditPhase : std::uint8_t {
  Preview,
  Commit,
  Cancel,
  Count,
};

using CreativeDesktopWorldLayoutPropertySettings =
    CreativeEditorWorldLayoutPropertySettings;

template <typename Settings>
[[nodiscard]] constexpr iggy3d::creative::CreativeWorldLayoutTable
creativeDesktopWorldLayoutPropertyTable() noexcept {
  if constexpr (std::is_same_v<Settings,
                               CreativeEditorWorldLayoutLevelSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Level;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutRoomMetadata>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Room;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutRoomSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Room;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutTopologyEdgeSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::TopologyEdge;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutVerticalConnectorSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::VerticalConnector;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutBoxSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Box;
  } else if constexpr (std::is_same_v<Settings,
                                      CreativeEditorWorldLayoutWallSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Wall;
  } else if constexpr (
      std::is_same_v<Settings, CreativeEditorWorldLayoutOpeningSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::Opening;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutRoofApertureSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::RoofAperture;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutTerrainProfileSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::TerrainProfile;
  } else if constexpr (
      std::is_same_v<Settings,
                     CreativeEditorWorldLayoutTerrainPathSettings>) {
    return iggy3d::creative::CreativeWorldLayoutTable::TerrainPath;
  } else {
    static_assert(
        std::is_same_v<Settings, CreativeEditorWorldLayoutObjectSettings>);
    return iggy3d::creative::CreativeWorldLayoutTable::Object;
  }
}

struct CreativeDesktopWorldLayoutPropertyEditPayload {
  CreativeDesktopWorldLayoutPropertyEditPhase phase =
      CreativeDesktopWorldLayoutPropertyEditPhase::Preview;
  iggy3d::creative::CreativeWorldLayoutTable table =
      iggy3d::creative::CreativeWorldLayoutTable::None;
  std::size_t index =
      iggy3d::creative::kInvalidCreativeWorldLayoutIndex;
  std::string stableKey;
  CreativeDesktopWorldLayoutPropertySettings settings;
};

}  // namespace iggy3d_creative_app
