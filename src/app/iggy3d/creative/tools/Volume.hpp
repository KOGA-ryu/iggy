#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/ShapeBrush.hpp"

namespace iggy3d::creative {

struct CreativeToolSettings;

inline constexpr std::string_view kCreativeVolumeCellTag =
    "iggy3d.volume_cell.v1";
inline constexpr std::uint64_t kDefaultCreativeVolumeOperationLimit = 16'384;

enum class CreativeVolumeSelectionPhase : std::uint8_t {
  Empty,
  FirstCorner,
  Complete,
};

enum class CreativeVolumeCorner : std::uint8_t {
  First,
  Second,
};

struct CreativeVolumeSelection {
  CreativeVolumeSelectionPhase phase = CreativeVolumeSelectionPhase::Empty;
  CreativeGridCoord3 firstCell{};
  CreativeGridCoord3 secondCell{};
  CreativeVec3 origin{};
  double cellSize = 1.0;
};

enum class CreativeVolumeOperationKind : std::uint8_t {
  Fill,
  Hollow,
  Replace,
  Erase,
  Clone,
  Count,
};

enum class CreativeVolumeOperationStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidSelection,
  InvalidRequest,
  UnsupportedObjectKind,
  OperationLimitExceeded,
  RelationshipRejected,
  CreateRejected,
  RemoveRejected,
  CopyRejected,
  PasteRejected,
  VoxelMutationRejected,
  NoChange,
  Applied,
};

struct CreativeVolumeOperationRequest {
  CreativeVolumeOperationKind operation = CreativeVolumeOperationKind::Fill;
  CreativeVolumeSelection selection{};
  CreativeObjectKind objectKind = CreativeObjectKind::Wall;
  CreativeShapeBrushKind shapeKind = CreativeShapeBrushKind::Box;
  CreativeShapeBrushAxis shapeAxis = CreativeShapeBrushAxis::Y;
  bool hasReplaceKindFilter = false;
  CreativeObjectKind replaceKindFilter = CreativeObjectKind::Unknown;
  bool hasCloneOffset = false;
  CreativeVec3 cloneOffset{};
  std::uint64_t maxAffectedObjects =
      kDefaultCreativeVolumeOperationLimit;
};

struct CreativeVolumeOperationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeVolumeOperationKind operation = CreativeVolumeOperationKind::Fill;
  CreativeShapeBrushKind shapeKind = CreativeShapeBrushKind::Box;
  CreativeShapeBrushAxis shapeAxis = CreativeShapeBrushAxis::Y;
  CreativeVolumeOperationStatus status =
      CreativeVolumeOperationStatus::NotRequested;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::uint64_t volumeCellCount = 0;
  std::uint64_t shapeCandidateCellCount = 0;
  std::uint64_t plannedCellCount = 0;
  std::uint64_t skippedOccupiedCellCount = 0;
  std::uint64_t matchedObjectCount = 0;
  std::uint64_t matchedVoxelCellCount = 0;
  std::uint64_t createdObjectCount = 0;
  std::uint64_t removedObjectCount = 0;
  std::uint64_t createdVoxelCellCount = 0;
  std::uint64_t removedVoxelCellCount = 0;
  std::uint64_t replacedVoxelCellCount = 0;
  std::uint64_t dirtyVoxelChunkCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::vector<CreativeObjectId> createdObjectIds;
  std::vector<CreativeObjectId> removedObjectIds;
  std::string reasonCode = "creative_volume_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeVolumeSelectionPhase phase) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeVolumeOperationKind operation) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeVolumeOperationStatus status) noexcept;

[[nodiscard]] bool creativeVolumeSelectionHasFirstCorner(
    const CreativeVolumeSelection& selection) noexcept;
[[nodiscard]] bool creativeVolumeSelectionComplete(
    const CreativeVolumeSelection& selection) noexcept;
[[nodiscard]] bool creativeVolumeSelectionValid(
    const CreativeVolumeSelection& selection) noexcept;
void clearCreativeVolumeSelection(CreativeVolumeSelection& selection) noexcept;

// A third corner press starts a fresh selection. This makes repeated box
// authoring a two-click loop without a separate reset command.
[[nodiscard]] CreativeVolumeSelectionPhase advanceCreativeVolumeSelection(
    CreativeVolumeSelection& selection,
    CreativeGridCoord3 cell) noexcept;
[[nodiscard]] CreativeVolumeSelectionPhase setCreativeVolumeSelectionCorner(
    CreativeVolumeSelection& selection,
    CreativeVolumeCorner corner,
    CreativeGridCoord3 cell) noexcept;
[[nodiscard]] bool expandCreativeVolumeSelectionToCell(
    CreativeVolumeSelection& selection,
    CreativeGridCoord3 cell) noexcept;
[[nodiscard]] CreativeVolumeSelection previewCreativeVolumeSelection(
    const CreativeVolumeSelection& selection,
    CreativeGridCoord3 cursorCell) noexcept;
[[nodiscard]] bool resizeCreativeVolumeSelectionHeight(
    CreativeVolumeSelection& selection,
    std::int32_t deltaCells) noexcept;

[[nodiscard]] CreativeGridCoord3 creativeVolumeCellFromWorld(
    CreativeVec3 worldPosition,
    double cellSize,
    CreativeVec3 origin = {}) noexcept;
[[nodiscard]] bool tryCreativeVolumeCellFromWorld(
    CreativeVec3 worldPosition,
    double cellSize,
    CreativeVec3 origin,
    CreativeGridCoord3& outCell) noexcept;
[[nodiscard]] CreativeBounds creativeVolumeCellBounds(
    CreativeGridCoord3 cell,
    double cellSize,
    CreativeVec3 origin = {}) noexcept;
[[nodiscard]] CreativeGridBounds3 creativeVolumeGridBounds(
    const CreativeVolumeSelection& selection) noexcept;
[[nodiscard]] CreativeBounds creativeVolumeWorldBounds(
    const CreativeVolumeSelection& selection) noexcept;
[[nodiscard]] std::uint64_t creativeVolumeCellCount(
    const CreativeVolumeSelection& selection) noexcept;

[[nodiscard]] CreativeVolumeOperationKind nextCreativeVolumeOperation(
    CreativeVolumeOperationKind operation) noexcept;
[[nodiscard]] bool creativeVolumeBrushSupported(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] CreativeObjectKind firstCreativeVolumeBrush(
    std::span<const CreativeObjectKind> palette) noexcept;
[[nodiscard]] CreativeObjectKind nextCreativeVolumeBrush(
    std::span<const CreativeObjectKind> palette,
    CreativeObjectKind current) noexcept;

// O(1). Maps committed contextual settings into the existing volume request
// contract without executing an edit. Invalid settings, operation, or clone
// scale leave request byte-for-byte unchanged and return false.
[[nodiscard]] bool applyCreativeToolSettingsToVolumeRequest(
    CreativeVolumeOperationRequest& request,
    const CreativeToolSettings& settings) noexcept;

// Fill/hollow write the chunked voxel field and also recognize retired tagged
// cell objects so old documents remain editable. Replace targets voxel cells
// and retired tagged cell objects. Erase and clone affect both voxels and every
// ordinary object wholly contained by the selected world bounds. Each command
// stages one document and commits only after every object and voxel edit passes.
[[nodiscard]] CreativeVolumeOperationReceipt executeCreativeVolumeOperation(
    CreativeDocument& document,
    const CreativeVolumeOperationRequest& request);

}  // namespace iggy3d::creative
