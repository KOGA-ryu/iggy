#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include "EditorWorldLayout.hpp"

namespace iggy3d_creative_app::detail {

inline bool hasVisibleWorldLayoutName(std::string_view name) noexcept {
  for (const char value : name) {
    if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
      return true;
    }
  }
  return false;
}

inline std::string mintWorldLayoutStableKey(
    CreativeEditorWorldLayoutState& state, std::string_view prefix) {
  return cr::mintCreativeWorldLayoutStableKey(
      state.source, state.nextStableOrdinal, prefix);
}

inline bool finiteWorldLayoutPoint(
    CreativeEditorWorldLayoutPoint point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.z);
}

inline CreativeEditorWorldLayoutRectHandle worldLayoutRectHandleAt(
    cr::CreativeWorldLayoutRect footprint,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!finiteWorldLayoutPoint(point) || !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0 ||
      point.x < footprint.minimum.x - toleranceCells ||
      point.x > footprint.maximum.x + toleranceCells ||
      point.z < footprint.minimum.z - toleranceCells ||
      point.z > footprint.maximum.z + toleranceCells) {
    return CreativeEditorWorldLayoutRectHandle::None;
  }
  const bool north =
      std::fabs(point.z - footprint.minimum.z) <= toleranceCells;
  const bool east =
      std::fabs(point.x - footprint.maximum.x) <= toleranceCells;
  const bool south =
      std::fabs(point.z - footprint.maximum.z) <= toleranceCells;
  const bool west =
      std::fabs(point.x - footprint.minimum.x) <= toleranceCells;
  if (north && west) {
    return CreativeEditorWorldLayoutRectHandle::NorthWest;
  }
  if (north && east) {
    return CreativeEditorWorldLayoutRectHandle::NorthEast;
  }
  if (south && east) {
    return CreativeEditorWorldLayoutRectHandle::SouthEast;
  }
  if (south && west) {
    return CreativeEditorWorldLayoutRectHandle::SouthWest;
  }
  if (north) {
    return CreativeEditorWorldLayoutRectHandle::North;
  }
  if (east) {
    return CreativeEditorWorldLayoutRectHandle::East;
  }
  if (south) {
    return CreativeEditorWorldLayoutRectHandle::South;
  }
  if (west) {
    return CreativeEditorWorldLayoutRectHandle::West;
  }
  if (point.x >= footprint.minimum.x && point.x <= footprint.maximum.x &&
      point.z >= footprint.minimum.z && point.z <= footprint.maximum.z) {
    return CreativeEditorWorldLayoutRectHandle::Move;
  }
  return CreativeEditorWorldLayoutRectHandle::None;
}

inline bool offsetWorldLayoutCoordinate(std::int32_t value,
                                        std::int64_t delta,
                                        std::int32_t& output) noexcept {
  const std::int64_t minimumDelta =
      static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) -
      value;
  const std::int64_t maximumDelta =
      static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) -
      value;
  if (delta < minimumDelta || delta > maximumDelta) {
    return false;
  }
  output = static_cast<std::int32_t>(
      static_cast<std::int64_t>(value) + delta);
  return true;
}

inline bool snappedWorldLayoutPointerDelta(double current, double start,
                                           std::int64_t& output) noexcept {
  constexpr double kMaximumUsefulDelta = 4294967295.0;
  const double delta = std::round(current - start);
  if (!std::isfinite(delta) || delta < -kMaximumUsefulDelta ||
      delta > kMaximumUsefulDelta) {
    return false;
  }
  output = static_cast<std::int64_t>(delta);
  return true;
}

inline bool worldLayoutRectHandleMovesNorth(
    CreativeEditorWorldLayoutRectHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutRectHandle::North ||
         handle == CreativeEditorWorldLayoutRectHandle::NorthWest ||
         handle == CreativeEditorWorldLayoutRectHandle::NorthEast;
}

inline bool worldLayoutRectHandleMovesEast(
    CreativeEditorWorldLayoutRectHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutRectHandle::East ||
         handle == CreativeEditorWorldLayoutRectHandle::NorthEast ||
         handle == CreativeEditorWorldLayoutRectHandle::SouthEast;
}

inline bool worldLayoutRectHandleMovesSouth(
    CreativeEditorWorldLayoutRectHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutRectHandle::South ||
         handle == CreativeEditorWorldLayoutRectHandle::SouthEast ||
         handle == CreativeEditorWorldLayoutRectHandle::SouthWest;
}

inline bool worldLayoutRectHandleMovesWest(
    CreativeEditorWorldLayoutRectHandle handle) noexcept {
  return handle == CreativeEditorWorldLayoutRectHandle::West ||
         handle == CreativeEditorWorldLayoutRectHandle::NorthWest ||
         handle == CreativeEditorWorldLayoutRectHandle::SouthWest;
}

template <typename Manipulation>
bool worldLayoutManipulatedRect(const Manipulation& manipulation,
                                CreativeEditorWorldLayoutPoint point,
                                cr::CreativeWorldLayoutRect& output) noexcept {
  output = manipulation.originalFootprint;
  std::int64_t deltaX = 0;
  std::int64_t deltaZ = 0;
  if (!snappedWorldLayoutPointerDelta(point.x, manipulation.startPoint.x,
                                      deltaX) ||
      !snappedWorldLayoutPointerDelta(point.z, manipulation.startPoint.z,
                                      deltaZ)) {
    return false;
  }
  const CreativeEditorWorldLayoutRectHandle handle =
      manipulation.target.handle;
  if (handle == CreativeEditorWorldLayoutRectHandle::Move) {
    return offsetWorldLayoutCoordinate(manipulation.originalFootprint.minimum.x,
                                       deltaX, output.minimum.x) &&
           offsetWorldLayoutCoordinate(manipulation.originalFootprint.maximum.x,
                                       deltaX, output.maximum.x) &&
           offsetWorldLayoutCoordinate(manipulation.originalFootprint.minimum.z,
                                       deltaZ, output.minimum.z) &&
           offsetWorldLayoutCoordinate(manipulation.originalFootprint.maximum.z,
                                       deltaZ, output.maximum.z);
  }
  if (worldLayoutRectHandleMovesWest(handle) &&
      !offsetWorldLayoutCoordinate(manipulation.originalFootprint.minimum.x,
                                   deltaX, output.minimum.x)) {
    return false;
  }
  if (worldLayoutRectHandleMovesEast(handle) &&
      !offsetWorldLayoutCoordinate(manipulation.originalFootprint.maximum.x,
                                   deltaX, output.maximum.x)) {
    return false;
  }
  if (worldLayoutRectHandleMovesNorth(handle) &&
      !offsetWorldLayoutCoordinate(manipulation.originalFootprint.minimum.z,
                                   deltaZ, output.minimum.z)) {
    return false;
  }
  return !worldLayoutRectHandleMovesSouth(handle) ||
         offsetWorldLayoutCoordinate(manipulation.originalFootprint.maximum.z,
                                     deltaZ, output.maximum.z);
}

inline void invalidateWorldLayoutPreview(
    CreativeEditorWorldLayoutState& state) {
  state.previewVisible = false;
  state.previewLayoutRevision = 0U;
  state.preview = {};
}

inline std::uint64_t nextWorldLayoutSourceEpoch(
    std::uint64_t current) noexcept {
  return current == std::numeric_limits<std::uint64_t>::max()
             ? 1U
             : current + 1U;
}

inline void clearWorldLayoutInteraction(
    CreativeEditorWorldLayoutState& state) {
  state.roomManipulation = {};
  state.verticalConnectorManipulation = {};
  state.boxManipulation = {};
  state.wallManipulation = {};
  state.buildingManipulation = {};
  state.buildingTransform = {};
  state.buildingTemplatePlacement = {};
  state.openingManipulation = {};
  state.elevationManipulation = {};
  state.roomSettingsDraft = {};
  state.verticalConnectorSettingsDraft = {};
  state.boxSettingsDraft = {};
  state.wallSettingsDraft = {};
  state.openingSettingsDraft = {};
  state.levelSettingsDraft = {};
  state.terrainProfileSettingsDraft = {};
  state.terrainPathSettingsDraft = {};
  state.objectSettingsDraft = {};
  state.objectManipulation = {};
}

inline CreativeEditorWorldLayoutSourceHistoryEntry
captureWorldLayoutSourceHistoryEntry(
    const CreativeEditorWorldLayoutState& state) {
  return {{state.source, state.revision, state.savedRevision,
           state.generatedRevision, state.nextStableOrdinal},
          state.selection,
          state.activeLevelIndex,
          {}};
}

inline void resetWorldLayoutSourceHistory(
    CreativeEditorWorldLayoutState& state) {
  const std::size_t maxDepth = state.sourceHistory.maxDepth;
  state.sourceHistory = {};
  state.sourceHistory.maxDepth = maxDepth;
  state.sourceHistory.current =
      captureWorldLayoutSourceHistoryEntry(state);
}

inline void appendWorldLayoutSourceHistoryEntry(
    std::vector<CreativeEditorWorldLayoutSourceHistoryEntry>& entries,
    CreativeEditorWorldLayoutSourceHistoryEntry entry,
    std::size_t maxDepth) {
  if (maxDepth == 0U) {
    return;
  }
  if (entries.size() >= maxDepth) {
    entries.erase(entries.begin());
  }
  entries.push_back(std::move(entry));
}

inline void noteWorldLayoutSourceChange(CreativeEditorWorldLayoutState& state,
                                        std::string reason) {
  state.deferredSourceHistory = {};
  CreativeEditorWorldLayoutSourceHistoryEntry previous =
      std::move(state.sourceHistory.current);
  previous.selection = state.selection;
  previous.activeLevelIndex = state.activeLevelIndex;
  previous.source = reason;
  appendWorldLayoutSourceHistoryEntry(
      state.sourceHistory.undoEntries, std::move(previous),
      state.sourceHistory.maxDepth);
  state.sourceHistory.redoEntries.clear();
  if (state.revision != std::numeric_limits<std::uint64_t>::max()) {
    ++state.revision;
  }
  clearWorldLayoutInteraction(state);
  invalidateWorldLayoutPreview(state);
  state.statusMessage = std::move(reason);
  state.sourceHistory.current =
      captureWorldLayoutSourceHistoryEntry(state);
}

}  // namespace iggy3d_creative_app::detail
