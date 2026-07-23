#include "EditorWorldLayoutOpeningInternal.hpp"

#include "EditorWorldLayoutInternal.hpp"

#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace iggy3d_creative_app::opening_detail {

using detail::noteWorldLayoutSourceChange;

CreativeEditorWorldLayoutOpeningHost openingHost(
    const cr::CreativeWorldLayout& layout,
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  const cr::CreativeWorldLayoutOpeningHostFrame resolved =
      cr::resolveCreativeWorldLayoutOpeningHost(layout, opening);
  if (!resolved.accepted) {
    return {};
  }
  return {
      true,
      {static_cast<double>(resolved.start.x),
       static_cast<double>(resolved.start.z)},
      {static_cast<double>(resolved.end.x),
       static_cast<double>(resolved.end.z)},
      resolved.lengthCells,
      resolved.baseLayer,
      resolved.wallHeightCells,
  };
}

double openingHostOffset(CreativeEditorWorldLayoutOpeningHost host,
                         CreativeEditorWorldLayoutPoint point) noexcept {
  if (!host.valid || !detail::finiteWorldLayoutPoint(point)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double dx = host.end.x - host.start.x;
  const double dz = host.end.z - host.start.z;
  return ((point.x - host.start.x) * dx + (point.z - host.start.z) * dz) /
         host.lengthCells;
}

CreativeEditorWorldLayoutPoint openingHostPoint(
    CreativeEditorWorldLayoutOpeningHost host, double offsetCells) noexcept {
  if (!host.valid || !std::isfinite(offsetCells)) {
    return {};
  }
  const double inverseLength = 1.0 / host.lengthCells;
  return {host.start.x + (host.end.x - host.start.x) * inverseLength *
                             offsetCells,
          host.start.z + (host.end.z - host.start.z) * inverseLength *
                             offsetCells};
}

double pointDistance(CreativeEditorWorldLayoutPoint lhs,
                     CreativeEditorWorldLayoutPoint rhs) noexcept {
  return std::hypot(lhs.x - rhs.x, lhs.z - rhs.z);
}

OpeningHostProjection nearestOpeningHost(
    const cr::CreativeWorldLayout& layout,
    CreativeEditorWorldLayoutPoint point, double tolerance,
    std::size_t activeLevelIndex) {
  const cr::CreativeWorldLayoutOpeningHostHit hit =
      cr::findNearestCreativeWorldLayoutOpeningHost(
          layout, {point.x, point.z}, tolerance, activeLevelIndex);
  return {hit.hit,
          hit.hostKind,
          hit.wallIndex,
          hit.roomIndex,
          hit.topologyEdgeIndex,
          hit.roomEdge,
          hit.centerOffsetCells,
          hit.host.lengthCells,
          hit.distanceCells};
}

bool sameHost(const cr::CreativeWorldLayoutOpening& opening,
              const OpeningHostProjection& projection) noexcept {
  if (opening.hostKind != projection.hostKind) {
    return false;
  }
  return opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall
             ? opening.wallIndex == projection.wallIndex
             : opening.roomIndex == projection.roomIndex &&
                   (projection.topologyEdgeIndex !=
                            cr::kInvalidCreativeWorldLayoutIndex
                        ? opening.roomTopologyEdgeIndex ==
                              projection.topologyEdgeIndex
                        : opening.roomEdge == projection.roomEdge);
}


bool nearlyEqual(double lhs, double rhs) noexcept {
  return std::fabs(lhs - rhs) <= kOpeningGeometryEpsilon;
}

bool validOpeningKind(cr::CreativeBuildingOpeningKind kind) noexcept {
  return kind == cr::CreativeBuildingOpeningKind::Door ||
         kind == cr::CreativeBuildingOpeningKind::Window;
}

bool sameVec3(cr::CreativeVec3 lhs, cr::CreativeVec3 rhs) noexcept {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool sameBounds(cr::CreativeBounds lhs, cr::CreativeBounds rhs) noexcept {
  return sameVec3(lhs.min, rhs.min) && sameVec3(lhs.max, rhs.max);
}

CreativeEditorWorldLayoutOpeningSettings openingSettings(
    const cr::CreativeWorldLayoutOpening& opening) noexcept {
  return {opening.centerOffsetCells,
          opening.widthCells,
          opening.cutoutBottomCells,
          opening.cutoutHeightCells,
          opening.door,
          opening.window,
          opening.facing,
          opening.includeInsert};
}

cr::CreativeWorldLayoutOpening openingWithSettings(
    const cr::CreativeWorldLayoutOpening& existing,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  cr::CreativeWorldLayoutOpening candidate = existing;
  const bool insertTracksBottom =
      nearlyEqual(existing.insertBottomCells, existing.cutoutBottomCells);
  const bool insertTracksHeight =
      existing.insertHeightCells > 0.0 &&
      nearlyEqual(existing.insertHeightCells, existing.cutoutHeightCells);
  const bool insertTracksWidth =
      existing.insertWidthCells > 0.0 &&
      nearlyEqual(existing.insertWidthCells, existing.widthCells);
  candidate.centerOffsetCells = settings.centerOffsetCells;
  candidate.widthCells = settings.widthCells;
  candidate.cutoutBottomCells = settings.sillHeightCells;
  candidate.cutoutHeightCells = settings.heightCells;
  candidate.door = settings.door;
  candidate.window = settings.window;
  candidate.facing = settings.facing;
  candidate.includeInsert = settings.includeInsert;
  if (insertTracksBottom) {
    candidate.insertBottomCells = settings.sillHeightCells;
  }
  if (insertTracksHeight) {
    candidate.insertHeightCells = settings.heightCells;
  }
  if (insertTracksWidth) {
    candidate.insertWidthCells = settings.widthCells;
  }
  return candidate;
}

bool sameEditableOpening(const cr::CreativeWorldLayoutOpening& lhs,
                         const cr::CreativeWorldLayoutOpening& rhs) noexcept {
  return lhs.centerOffsetCells == rhs.centerOffsetCells &&
         lhs.widthCells == rhs.widthCells &&
         lhs.cutoutBottomCells == rhs.cutoutBottomCells &&
         lhs.cutoutHeightCells == rhs.cutoutHeightCells &&
         lhs.door == rhs.door && lhs.window == rhs.window &&
         lhs.facing == rhs.facing &&
         lhs.includeInsert == rhs.includeInsert &&
         lhs.insertBottomCells == rhs.insertBottomCells &&
         lhs.insertHeightCells == rhs.insertHeightCells &&
         lhs.insertWidthCells == rhs.insertWidthCells &&
         lhs.insertThicknessCells == rhs.insertThicknessCells &&
         lhs.insertAssetId == rhs.insertAssetId &&
         lhs.hasInsertAssetSourceBounds ==
             rhs.hasInsertAssetSourceBounds &&
         sameBounds(lhs.insertAssetSourceBoundsMeters,
                    rhs.insertAssetSourceBoundsMeters);
}

OpeningValidation validateOpeningCandidate(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate) {
  if (openingIndex >= state.source.openings.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutOpeningValidationResult validation =
      cr::validateCreativeWorldLayoutOpening(
          {&state.source, &candidate, openingIndex,
           kOpeningEndClearanceCells, kOpeningMinimumWidthCells});
  if (validation.accepted) {
    return {true, "creative_editor_world_layout_opening_settings_ready",
            "opening settings ready"};
  }
  switch (validation.status) {
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidInsert:
      return {false, std::string(validation.reasonCode),
              "opening insert dimensions or asset metadata is invalid"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::InsertDoesNotFit:
      return {false, std::string(validation.reasonCode),
              "opening insert no longer fits its cutout"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::DoorSillInvalid:
      return {false, std::string(validation.reasonCode),
              "door openings must begin at floor height"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidDoorSettings:
      return {false, std::string(validation.reasonCode),
              "door leaf, hinge, swing, lock, or timing settings are invalid"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidWindowSettings:
      return {false, std::string(validation.reasonCode),
              "window treatment is invalid"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::DoorSwingObstructed:
      return {false, std::string(validation.reasonCode),
              "door swing intersects a wall, door, or vertical route"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::MissingHost:
    case cr::CreativeWorldLayoutOpeningValidationStatus::
        UnsupportedHostOrientation:
      return {false, std::string(validation.reasonCode),
              "opening host is unavailable"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::EndClearanceInvalid:
      return {false, std::string(validation.reasonCode),
              "opening needs a quarter-cell wall pier at each end"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::WallHeightExceeded:
      return {false, std::string(validation.reasonCode),
              "opening exceeds the host wall height"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::InteriorWindow:
      return {false, std::string(validation.reasonCode),
              "windows must remain on exterior room edges"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::Overlap:
      return {false, std::string(validation.reasonCode),
              "opening overlaps another opening on this wall"};
    case cr::CreativeWorldLayoutOpeningValidationStatus::NotRequested:
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidRequest:
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidKind:
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidFacing:
    case cr::CreativeWorldLayoutOpeningValidationStatus::InvalidCutout:
    case cr::CreativeWorldLayoutOpeningValidationStatus::Ready:
      break;
  }
  return {false, std::string(validation.reasonCode),
          "opening settings are invalid"};
}

CreativeEditorWorldLayoutEditReceipt commitOpeningCandidate(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    const cr::CreativeWorldLayoutOpening& candidate,
    std::string statusMessage) {
  const OpeningValidation validation =
      validateOpeningCandidate(state, openingIndex, candidate);
  if (!validation.accepted) {
    state.statusMessage = validation.message;
    return {false, false, validation.reasonCode};
  }
  if (sameEditableOpening(state.source.openings[openingIndex], candidate)) {
    return {true, false,
            "creative_editor_world_layout_opening_settings_no_change"};
  }
  state.source.openings[openingIndex] = candidate;
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     openingIndex};
  noteWorldLayoutSourceChange(state, std::move(statusMessage));
  return {true, true,
          "creative_editor_world_layout_opening_settings_updated"};
}


bool snappedOpeningDelta(double current, double start,
                         double& output) noexcept {
  const double delta = current - start;
  if (!std::isfinite(delta)) {
    return false;
  }
  output = std::round(delta / kOpeningSnapCells) * kOpeningSnapCells;
  return std::isfinite(output);
}

CreativeEditorWorldLayoutOpeningTarget openingTargetAt(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutPoint point, double toleranceCells,
    bool includeResizeHandles) noexcept {
  if (!detail::worldLayoutOpeningOnActiveLevel(state, state.source,
                                               openingIndex) ||
      !detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) || toleranceCells <= 0.0) {
    return {};
  }
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  const CreativeEditorWorldLayoutOpeningHost host =
      openingHost(state.source, opening);
  if (!host.valid) {
    return {};
  }
  const double halfWidth = opening.widthCells * 0.5;
  const CreativeEditorWorldLayoutPoint center =
      openingHostPoint(host, opening.centerOffsetCells);
  const CreativeEditorWorldLayoutPoint start =
      openingHostPoint(host, opening.centerOffsetCells - halfWidth);
  const CreativeEditorWorldLayoutPoint end =
      openingHostPoint(host, opening.centerOffsetCells + halfWidth);
  if (includeResizeHandles) {
    const double centerDistance = pointDistance(point, center);
    const double startDistance = pointDistance(point, start);
    const double endDistance = pointDistance(point, end);
    double nearestDistance = centerDistance;
    CreativeEditorWorldLayoutOpeningHandle nearestHandle =
        CreativeEditorWorldLayoutOpeningHandle::Move;
    if (startDistance < nearestDistance) {
      nearestDistance = startDistance;
      nearestHandle = CreativeEditorWorldLayoutOpeningHandle::Start;
    }
    if (endDistance < nearestDistance) {
      nearestDistance = endDistance;
      nearestHandle = CreativeEditorWorldLayoutOpeningHandle::End;
    }
    if (nearestDistance <= toleranceCells) {
      return {openingIndex, nearestHandle};
    }
  }
  const double pointOffset = openingHostOffset(host, point);
  if (!std::isfinite(pointOffset)) {
    return {};
  }
  const double boundedOffset =
      std::clamp(pointOffset, opening.centerOffsetCells - halfWidth,
                 opening.centerOffsetCells + halfWidth);
  if (pointDistance(point, openingHostPoint(host, boundedOffset)) <=
      toleranceCells) {
    return {openingIndex, CreativeEditorWorldLayoutOpeningHandle::Move};
  }
  return {};
}

}  // namespace iggy3d_creative_app::opening_detail
