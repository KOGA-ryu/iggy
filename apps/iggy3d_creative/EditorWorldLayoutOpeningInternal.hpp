#pragma once

#include "EditorWorldLayout.hpp"

#include <cstddef>
#include <limits>
#include <string>

namespace iggy3d_creative_app::opening_detail {

inline constexpr double kOpeningHitToleranceCells = 0.75;
inline constexpr double kOpeningSnapCells = 0.25;
inline constexpr double kOpeningEndClearanceCells = 0.25;
inline constexpr double kOpeningMinimumWidthCells = 0.25;
inline constexpr double kOpeningGeometryEpsilon = 1.0e-9;

struct OpeningHostProjection {
  bool hit = false;
  cr::CreativeWorldLayoutOpeningHostKind hostKind =
      cr::CreativeWorldLayoutOpeningHostKind::Wall;
  std::size_t wallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t roomIndex = cr::kInvalidCreativeWorldLayoutIndex;
  cr::CreativeWorldLayoutRoomEdge roomEdge =
      cr::CreativeWorldLayoutRoomEdge::North;
  double centerOffsetCells = 0.0;
  double lengthCells = 0.0;
  double distanceCells = std::numeric_limits<double>::infinity();
};

struct OpeningValidation {
  bool accepted = false;
  std::string reasonCode =
      "creative_editor_world_layout_opening_settings_invalid";
  std::string message = "opening settings are invalid";
};

[[nodiscard]] CreativeEditorWorldLayoutOpeningHost openingHost(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutOpening& opening) noexcept;
[[nodiscard]] double openingHostOffset(
    CreativeEditorWorldLayoutOpeningHost host,
    CreativeEditorWorldLayoutPoint point) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutPoint openingHostPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) noexcept;
[[nodiscard]] bool openingIntervalsOverlap(
    const cr::CreativeWorldLayoutOpening& candidate,
    CreativeEditorWorldLayoutOpeningHost candidateHost,
    const cr::CreativeWorldLayoutOpening& existing,
    CreativeEditorWorldLayoutOpeningHost existingHost) noexcept;
[[nodiscard]] OpeningHostProjection nearestOpeningHost(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point, double tolerance,
    std::size_t activeLevelIndex);
[[nodiscard]] bool sameHost(
    const cr::CreativeWorldLayoutOpening& opening,
    const OpeningHostProjection& projection) noexcept;

[[nodiscard]] bool validOpeningKind(
    cr::CreativeBuildingOpeningKind kind) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutOpeningSettings openingSettings(
    const cr::CreativeWorldLayoutOpening& opening) noexcept;
[[nodiscard]] cr::CreativeWorldLayoutOpening openingWithSettings(
    const cr::CreativeWorldLayoutOpening& existing,
    CreativeEditorWorldLayoutOpeningSettings settings);
[[nodiscard]] OpeningValidation validateOpeningCandidate(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate);
[[nodiscard]] CreativeEditorWorldLayoutEditReceipt commitOpeningCandidate(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate,
    std::string statusMessage);

[[nodiscard]] bool snappedOpeningDelta(
    double current, double start, double& output) noexcept;
[[nodiscard]] CreativeEditorWorldLayoutOpeningTarget openingTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool includeResizeHandles) noexcept;

}  // namespace iggy3d_creative_app::opening_detail
