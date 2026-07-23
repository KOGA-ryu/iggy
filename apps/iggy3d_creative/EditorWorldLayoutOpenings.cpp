#include "EditorWorldLayout.hpp"

#include "EditorWorldLayoutInternal.hpp"
#include "EditorWorldLayoutOpeningInternal.hpp"

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace iggy3d_creative_app {

using detail::clearWorldLayoutInteraction;
using detail::mintWorldLayoutStableKey;
using detail::noteWorldLayoutSourceChange;
using opening_detail::commitOpeningCandidate;
using opening_detail::kOpeningEndClearanceCells;
using opening_detail::kOpeningGeometryEpsilon;
using opening_detail::kOpeningHitToleranceCells;
using opening_detail::kOpeningSnapCells;
using opening_detail::nearestOpeningHost;
using opening_detail::OpeningHostProjection;
using opening_detail::openingHost;
using opening_detail::openingHostOffset;
using opening_detail::openingHostPoint;
using opening_detail::openingSettings;
using opening_detail::openingTargetAt;
using opening_detail::openingWithSettings;
using opening_detail::OpeningValidation;
using opening_detail::snappedOpeningDelta;
using opening_detail::validateOpeningCandidate;
using opening_detail::validOpeningKind;

CreativeEditorWorldLayoutOpeningPlacementPlan
planCreativeEditorWorldLayoutOpeningPlacement(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point,
    cr::CreativeBuildingOpeningKind kind) {
  CreativeEditorWorldLayoutOpeningPlacementRequest request;
  request.point = point;
  request.kind = kind;
  return planCreativeEditorWorldLayoutOpeningPlacement(state, request);
}

CreativeEditorWorldLayoutOpeningPlacementPlan
planCreativeEditorWorldLayoutOpeningPlacement(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningPlacementRequest& request) {
  CreativeEditorWorldLayoutOpeningPlacementPlan result;
  const cr::CreativeBoundsMetrics assetSource =
      cr::measureCreativeBounds(request.insertAssetSourceBoundsMeters);
  const bool validAsset =
      request.hasInsertAssetSourceBounds
          ? !request.insertAssetId.empty() && request.useExplicitDimensions &&
                assetSource.valid && cr::isPositiveCreativeVec3(assetSource.size)
          : request.insertAssetId.empty();
  const bool validExplicitDimensions =
      !request.useExplicitDimensions ||
      (std::isfinite(request.widthCells) && request.widthCells > 0.0 &&
       std::isfinite(request.cutoutBottomCells) &&
       request.cutoutBottomCells >= 0.0 &&
       std::isfinite(request.cutoutHeightCells) &&
       request.cutoutHeightCells > 0.0 &&
       std::isfinite(request.insertThicknessCells) &&
       request.insertThicknessCells > 0.0);
  if (!detail::finiteWorldLayoutPoint(request.point) ||
      !validOpeningKind(request.kind) ||
      !validAsset || !validExplicitDimensions) {
    result.message = "Opening target is invalid";
    result.reasonCode =
        "creative_editor_world_layout_opening_placement_invalid";
    return result;
  }

  const OpeningHostProjection projection =
      nearestOpeningHost(state.source, request.point, kOpeningHitToleranceCells,
                         state.activeLevelIndex);
  if (!projection.hit) {
    result.message = "Place the opening on a room edge or partition";
    result.reasonCode = "creative_editor_world_layout_wall_not_found";
    return result;
  }

  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = projection.hostKind;
  opening.wallIndex = projection.wallIndex;
  opening.roomIndex = projection.roomIndex;
  opening.roomEdge = projection.roomEdge;
  opening.roomTopologyEdgeIndex = projection.topologyEdgeIndex;
  opening.kind = request.kind;
  opening.includeInsert = true;
  switch (request.kind) {
    case cr::CreativeBuildingOpeningKind::Door:
      opening.widthCells = 1.0;
      opening.cutoutBottomCells = 0.0;
      opening.cutoutHeightCells = 2.1;
      opening.insertBottomCells = 0.0;
      opening.insertHeightCells = 2.1;
      opening.insertWidthCells = 1.0;
      opening.insertThicknessCells = 0.15;
      break;
    case cr::CreativeBuildingOpeningKind::Window:
      opening.widthCells = 1.5;
      opening.cutoutBottomCells = 1.0;
      opening.cutoutHeightCells = 1.2;
      opening.insertBottomCells = 1.0;
      opening.insertHeightCells = 1.2;
      opening.insertWidthCells = 1.5;
      opening.insertThicknessCells = 0.10;
      break;
  }
  if (request.useExplicitDimensions) {
    opening.widthCells = request.widthCells;
    opening.cutoutBottomCells = request.cutoutBottomCells;
    opening.cutoutHeightCells = request.cutoutHeightCells;
    opening.insertBottomCells = request.cutoutBottomCells;
    opening.insertHeightCells = request.cutoutHeightCells;
    opening.insertWidthCells = request.widthCells;
    opening.insertThicknessCells = request.insertThicknessCells;
  }

  const double halfWidth = opening.widthCells * 0.5;
  const double minimumCenter = halfWidth + kOpeningEndClearanceCells;
  const double maximumCenter =
      projection.lengthCells - halfWidth - kOpeningEndClearanceCells;
  if (minimumCenter > maximumCenter + kOpeningGeometryEpsilon) {
    result.message = "Wall is too short for this opening";
    result.reasonCode = "creative_editor_world_layout_wall_too_short";
    return result;
  }
  opening.centerOffsetCells =
      std::clamp(std::round(projection.centerOffsetCells / kOpeningSnapCells) *
                     kOpeningSnapCells,
                 minimumCenter, maximumCenter);
  const cr::CreativeWorldLayoutOpeningValidationResult validation =
      cr::validateCreativeWorldLayoutOpening(
          {&state.source, &opening, cr::kInvalidCreativeWorldLayoutIndex,
           kOpeningEndClearanceCells,
           opening_detail::kOpeningMinimumWidthCells});
  if (!validation.accepted) {
    switch (validation.status) {
      case cr::CreativeWorldLayoutOpeningValidationStatus::InteriorWindow:
        result.message = "Place windows on an exterior room edge";
        result.reasonCode =
            "creative_editor_world_layout_window_requires_exterior";
        break;
      case cr::CreativeWorldLayoutOpeningValidationStatus::Overlap:
        result.message = "Opening overlaps an existing opening";
        result.reasonCode = "creative_editor_world_layout_opening_overlap";
        break;
      case cr::CreativeWorldLayoutOpeningValidationStatus::
          EndClearanceInvalid:
        result.message = "Wall is too short for this opening";
        result.reasonCode = "creative_editor_world_layout_wall_too_short";
        break;
      case cr::CreativeWorldLayoutOpeningValidationStatus::WallHeightExceeded:
        result.message = "Opening exceeds the wall height";
        result.reasonCode =
            "creative_editor_world_layout_opening_height_invalid";
        break;
      case cr::CreativeWorldLayoutOpeningValidationStatus::MissingHost:
        result.message = "Opening host is unavailable";
        result.reasonCode =
            "creative_editor_world_layout_opening_host_invalid";
        break;
      case cr::CreativeWorldLayoutOpeningValidationStatus::
          UnsupportedHostOrientation:
        result.message = "Openings require a cardinal wall";
        result.reasonCode =
            "creative_editor_world_layout_opening_host_orientation_unsupported";
        break;
      default:
        result.message = "Opening target is invalid";
        result.reasonCode =
            "creative_editor_world_layout_opening_placement_invalid";
        break;
    }
    return result;
  }
  result.host = openingHost(state.source, opening);

  result.opening = std::move(opening);
  result.centerPoint =
      openingHostPoint(result.host, result.opening.centerOffsetCells);
  result.startPoint = openingHostPoint(
      result.host,
      result.opening.centerOffsetCells - result.opening.widthCells * 0.5);
  result.endPoint = openingHostPoint(
      result.host,
      result.opening.centerOffsetCells + result.opening.widthCells * 0.5);
  result.pointerDistanceCells = projection.distanceCells;
  result.message = request.kind == cr::CreativeBuildingOpeningKind::Door
                       ? "Door target ready"
                       : "Window target ready";
  result.reasonCode =
      "creative_editor_world_layout_opening_placement_ready";
  result.accepted = true;
  return result;
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningPlacement(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningPlacementRequest& request) {
  CreativeEditorWorldLayoutOpeningPlacementPlan plan =
      planCreativeEditorWorldLayoutOpeningPlacement(state, request);
  if (!plan.accepted) {
    state.statusMessage = plan.message;
    return {false, false, std::string(plan.reasonCode)};
  }

  cr::CreativeWorldLayoutOpening opening = std::move(plan.opening);
  opening.stableKey = mintWorldLayoutStableKey(
      state, request.kind == cr::CreativeBuildingOpeningKind::Door ? "door"
                                                                   : "window");
  opening.name = request.label.empty()
                     ? (request.kind == cr::CreativeBuildingOpeningKind::Door
                            ? "Door "
                            : "Window ") +
                           std::to_string(state.source.openings.size() + 1U)
                     : std::string(request.label);
  if (request.hasInsertAssetSourceBounds) {
    opening.insertAssetId = std::string(request.insertAssetId);
    opening.insertAssetSourceBoundsMeters =
        request.insertAssetSourceBoundsMeters;
    opening.hasInsertAssetSourceBounds = true;
  }
  state.source.openings.push_back(std::move(opening));
  state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                     state.source.openings.size() - 1U};
  noteWorldLayoutSourceChange(
      state, request.kind == cr::CreativeBuildingOpeningKind::Door
                 ? "door added"
                 : "window added");
  return {true, true, "creative_editor_world_layout_opening_added"};
}


bool readCreativeEditorWorldLayoutOpeningSettings(
    const CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings& output) noexcept {
  if (openingIndex >= state.source.openings.size()) {
    return false;
  }
  output = openingSettings(state.source.openings[openingIndex]);
  return true;
}

CreativeEditorWorldLayoutOpeningHost
resolveCreativeEditorWorldLayoutOpeningHost(
    const CreativeEditorWorldLayoutState& state,
    std::size_t openingIndex) noexcept {
  const cr::CreativeWorldLayout &source =
      creativeEditorWorldLayoutDisplaySource(state);
  if (openingIndex >= source.openings.size()) {
    return {};
  }
  const cr::CreativeWorldLayoutOpening& opening = source.openings[openingIndex];
  CreativeEditorWorldLayoutOpeningHost host = openingHost(source, opening);
  if (state.wallManipulation.active &&
      opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex == state.wallManipulation.target.wallIndex) {
    host.start = {
        static_cast<double>(state.wallManipulation.previewStart.x),
        static_cast<double>(state.wallManipulation.previewStart.z),
    };
    host.end = {
        static_cast<double>(state.wallManipulation.previewEnd.x),
        static_cast<double>(state.wallManipulation.previewEnd.z),
    };
  }
  std::size_t hostBuildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
      opening.wallIndex < source.walls.size()) {
    hostBuildingIndex = source.walls[opening.wallIndex].buildingIndex;
  } else if (opening.hostKind ==
                 cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
             opening.roomIndex < source.rooms.size()) {
    hostBuildingIndex = source.rooms[opening.roomIndex].buildingIndex;
  }
  if (state.buildingManipulation.active &&
      hostBuildingIndex == state.buildingManipulation.buildingIndex) {
    const double deltaX = static_cast<double>(
        state.buildingManipulation.previewDeltaXCells);
    const double deltaZ = static_cast<double>(
        state.buildingManipulation.previewDeltaZCells);
    host.start.x += deltaX;
    host.start.z += deltaZ;
    host.end.x += deltaX;
    host.end.z += deltaZ;
  }
  host.lengthCells =
      std::hypot(host.end.x - host.start.x, host.end.z - host.start.z);
  host.valid = std::isfinite(host.lengthCells) && host.lengthCells > 0.0;
  return host;
}

CreativeEditorWorldLayoutOpeningTarget
findCreativeEditorWorldLayoutOpeningTarget(
    const CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) noexcept {
  if (!detail::finiteWorldLayoutPoint(point) ||
      !std::isfinite(toleranceCells) ||
      toleranceCells <= 0.0) {
    return {};
  }
  if (state.selection.kind ==
          CreativeEditorWorldLayoutSelectionKind::Opening &&
      state.selection.index < state.source.openings.size()) {
    const CreativeEditorWorldLayoutOpeningTarget selectedTarget =
        openingTargetAt(state, state.selection.index, point, toleranceCells,
                        true);
    if (selectedTarget.handle !=
        CreativeEditorWorldLayoutOpeningHandle::None) {
      return selectedTarget;
    }
  }
  for (std::size_t index = state.source.openings.size(); index > 0U; --index) {
    const CreativeEditorWorldLayoutOpeningTarget target = openingTargetAt(
        state, index - 1U, point, toleranceCells, false);
    if (target.handle != CreativeEditorWorldLayoutOpeningHandle::None) {
      return target;
    }
  }
  return {};
}

CreativeEditorWorldLayoutEditReceipt
setCreativeEditorWorldLayoutOpeningSettings(
    CreativeEditorWorldLayoutState& state, std::size_t openingIndex,
    CreativeEditorWorldLayoutOpeningSettings settings) {
  if (openingIndex >= state.source.openings.size()) {
    state.statusMessage = "opening selection is stale";
    return {false, false,
            "creative_editor_world_layout_opening_index_invalid"};
  }
  const cr::CreativeWorldLayoutOpening candidate = openingWithSettings(
      state.source.openings[openingIndex], settings);
  return commitOpeningCandidate(state, openingIndex, candidate,
                                "opening settings updated");
}

CreativeEditorWorldLayoutOpeningAssetGeometryPlan
planCreativeEditorWorldLayoutOpeningAssetGeometry(
    const CreativeEditorWorldLayoutOpeningAssetGeometryRequest& request)
    noexcept {
  CreativeEditorWorldLayoutOpeningAssetGeometryPlan plan;
  const cr::CreativeBoundsMetrics source =
      cr::measureCreativeBounds(request.sourceBoundsMeters);
  if (!validOpeningKind(request.kind) || !source.valid ||
      !cr::isPositiveCreativeVec3(source.size) ||
      !cr::isFiniteCreativeVec3(request.scale) ||
      !cr::isPositiveCreativeVec3(request.scale) ||
      !std::isfinite(request.gridCellSizeMeters) ||
      request.gridCellSizeMeters <= 0.0) {
    plan.reasonCode =
        "creative_editor_world_layout_opening_asset_geometry_invalid";
    return plan;
  }
  const bool localXIsPrimary = source.size.x >= source.size.z;
  const double widthMeters =
      localXIsPrimary ? source.size.x * request.scale.x
                      : source.size.z * request.scale.z;
  const double thicknessMeters =
      localXIsPrimary ? source.size.z * request.scale.z
                      : source.size.x * request.scale.x;
  plan.accepted = true;
  plan.widthCells = widthMeters / request.gridCellSizeMeters;
  plan.cutoutBottomCells =
      request.kind == cr::CreativeBuildingOpeningKind::Window ? 1.0 : 0.0;
  plan.cutoutHeightCells =
      source.size.y * request.scale.y / request.gridCellSizeMeters;
  plan.insertThicknessCells =
      thicknessMeters / request.gridCellSizeMeters;
  plan.reasonCode =
      "creative_editor_world_layout_opening_asset_geometry_ready";
  return plan;
}

CreativeEditorWorldLayoutOpeningInsertPlan
planCreativeEditorWorldLayoutOpeningInsert(
    const CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningInsertRequest& request) {
  CreativeEditorWorldLayoutOpeningInsertPlan plan;
  if (request.openingIndex >= state.source.openings.size() ||
      request.operation >=
          CreativeEditorWorldLayoutOpeningInsertOperation::Count) {
    plan.message = "Opening insert request is invalid";
    plan.reasonCode =
        "creative_editor_world_layout_opening_insert_request_invalid";
    return plan;
  }

  cr::CreativeWorldLayoutOpening candidate =
      state.source.openings[request.openingIndex];
  if (request.operation ==
      CreativeEditorWorldLayoutOpeningInsertOperation::UseProceduralInsert) {
    candidate.includeInsert = true;
    candidate.insertAssetId.clear();
    candidate.insertAssetSourceBoundsMeters = {};
    candidate.hasInsertAssetSourceBounds = false;
  } else {
    const cr::CreativeBoundsMetrics source =
        cr::measureCreativeBounds(request.assetSourceBoundsMeters);
    if (request.assetKind != candidate.kind || request.assetId.empty() ||
        !source.valid || !cr::isPositiveCreativeVec3(source.size)) {
      plan.message = "Choose a compatible door or window asset";
      plan.reasonCode =
          "creative_editor_world_layout_opening_insert_asset_invalid";
      return plan;
    }
    candidate.includeInsert = true;
    candidate.insertAssetId = std::string(request.assetId);
    candidate.insertAssetSourceBoundsMeters =
        request.assetSourceBoundsMeters;
    candidate.hasInsertAssetSourceBounds = true;

    if (request.operation ==
        CreativeEditorWorldLayoutOpeningInsertOperation::
            ResizeOpeningToAsset) {
      const CreativeEditorWorldLayoutOpeningAssetGeometryPlan geometry =
          planCreativeEditorWorldLayoutOpeningAssetGeometry(
              {request.assetKind, request.assetSourceBoundsMeters,
               request.assetScale, request.gridCellSizeMeters});
      if (!geometry.accepted) {
        plan.message = "Opening resize settings are invalid";
        plan.reasonCode =
            "creative_editor_world_layout_opening_insert_resize_invalid";
        return plan;
      }
      candidate.widthCells = geometry.widthCells;
      candidate.cutoutHeightCells = geometry.cutoutHeightCells;
      candidate.insertBottomCells = candidate.cutoutBottomCells;
      candidate.insertHeightCells = candidate.cutoutHeightCells;
      candidate.insertWidthCells = candidate.widthCells;
      candidate.insertThicknessCells = geometry.insertThicknessCells;
    }
  }

  const OpeningValidation validation =
      validateOpeningCandidate(state, request.openingIndex, candidate);
  if (!validation.accepted) {
    plan.message = validation.message;
    plan.reasonCode = validation.reasonCode;
    return plan;
  }
  plan.accepted = true;
  plan.candidate = std::move(candidate);
  plan.message =
      request.operation ==
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  UseProceduralInsert
          ? "Procedural opening insert ready"
          : "Catalog opening insert ready";
  plan.reasonCode =
      "creative_editor_world_layout_opening_insert_ready";
  return plan;
}

CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningInsert(
    CreativeEditorWorldLayoutState& state,
    const CreativeEditorWorldLayoutOpeningInsertRequest& request) {
  CreativeEditorWorldLayoutOpeningInsertPlan plan =
      planCreativeEditorWorldLayoutOpeningInsert(state, request);
  if (!plan.accepted) {
    state.statusMessage = plan.message;
    return {false, false, std::move(plan.reasonCode)};
  }
  return commitOpeningCandidate(
      state, request.openingIndex, plan.candidate,
      request.operation ==
              CreativeEditorWorldLayoutOpeningInsertOperation::
                  UseProceduralInsert
          ? "procedural opening insert selected"
          : "opening insert replaced");
}


CreativeEditorWorldLayoutEditReceipt
applyCreativeEditorWorldLayoutOpeningManipulation(
    CreativeEditorWorldLayoutState& state,
    CreativeEditorWorldLayoutOpeningManipulationPhase phase,
    CreativeEditorWorldLayoutPoint point, double toleranceCells) {
  if (phase >= CreativeEditorWorldLayoutOpeningManipulationPhase::Count) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_phase_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Cancel) {
    const bool changed = state.openingManipulation.active;
    state.openingManipulation = {};
    state.statusMessage = "opening manipulation cancelled";
    return {true, changed,
            "creative_editor_world_layout_opening_manipulation_cancelled"};
  }
  if (state.tool != CreativeEditorWorldLayoutTool::Select) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_tool_invalid"};
  }
  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Begin) {
    const CreativeEditorWorldLayoutOpeningTarget target =
        findCreativeEditorWorldLayoutOpeningTarget(state, point,
                                                   toleranceCells);
    if (target.handle == CreativeEditorWorldLayoutOpeningHandle::None ||
        target.openingIndex >= state.source.openings.size()) {
      return detail::selectWorldLayoutAtPoint(state, point);
    }
    const cr::CreativeWorldLayoutOpening& opening =
        state.source.openings[target.openingIndex];
    const CreativeEditorWorldLayoutOpeningHost host =
        openingHost(state.source, opening);
    const double pointerOffset = openingHostOffset(host, point);
    if (!host.valid || !std::isfinite(pointerOffset)) {
      return {false, false,
              "creative_editor_world_layout_opening_host_invalid"};
    }
    clearWorldLayoutInteraction(state);
    state.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                       target.openingIndex};
    state.anchorActive = false;
    state.openingManipulation = {
        true,
        state.revision,
        target,
        pointerOffset,
        opening.centerOffsetCells,
        opening.widthCells,
        opening.centerOffsetCells,
        opening.widthCells,
        true,
        "creative_editor_world_layout_opening_manipulation_ready",
    };
    state.statusMessage =
        target.handle == CreativeEditorWorldLayoutOpeningHandle::Move
            ? "drag opening along its wall"
            : "drag to resize opening";
    return {true, true,
            "creative_editor_world_layout_opening_manipulation_started"};
  }

  if (!state.openingManipulation.active ||
      state.openingManipulation.target.openingIndex >=
          state.source.openings.size()) {
    return {
        false, false,
        "creative_editor_world_layout_opening_manipulation_not_active"};
  }
  const std::size_t openingIndex =
      state.openingManipulation.target.openingIndex;
  const cr::CreativeWorldLayoutOpening& opening =
      state.source.openings[openingIndex];
  if (state.revision != state.openingManipulation.sourceRevision ||
      opening.centerOffsetCells !=
          state.openingManipulation.originalCenterOffsetCells ||
      opening.widthCells != state.openingManipulation.originalWidthCells) {
    state.openingManipulation = {};
    state.statusMessage = "opening changed while drag was active";
    return {false, false,
            "creative_editor_world_layout_opening_manipulation_stale"};
  }

  if (phase == CreativeEditorWorldLayoutOpeningManipulationPhase::Update) {
    const CreativeEditorWorldLayoutOpeningHost host =
        openingHost(state.source, opening);
    const double pointerOffset = openingHostOffset(host, point);
    double delta = 0.0;
    if (!host.valid || !std::isfinite(pointerOffset) ||
        !snappedOpeningDelta(pointerOffset,
                             state.openingManipulation.startPointerOffsetCells,
                             delta)) {
      return {false, false,
              "creative_editor_world_layout_opening_pointer_invalid"};
    }

    double center = state.openingManipulation.originalCenterOffsetCells;
    double width = state.openingManipulation.originalWidthCells;
    const double originalMinimum = center - width * 0.5;
    const double originalMaximum = center + width * 0.5;
    switch (state.openingManipulation.target.handle) {
      case CreativeEditorWorldLayoutOpeningHandle::Move:
        center += delta;
        break;
      case CreativeEditorWorldLayoutOpeningHandle::Start: {
        const double minimum = originalMinimum + delta;
        center = (minimum + originalMaximum) * 0.5;
        width = originalMaximum - minimum;
        break;
      }
      case CreativeEditorWorldLayoutOpeningHandle::End: {
        const double maximum = originalMaximum + delta;
        center = (originalMinimum + maximum) * 0.5;
        width = maximum - originalMinimum;
        break;
      }
      case CreativeEditorWorldLayoutOpeningHandle::None:
      case CreativeEditorWorldLayoutOpeningHandle::Count:
        return {
            false, false,
            "creative_editor_world_layout_opening_manipulation_handle_invalid"};
    }
    if (center == state.openingManipulation.previewCenterOffsetCells &&
        width == state.openingManipulation.previewWidthCells) {
      return {true, false, state.openingManipulation.reasonCode};
    }

    CreativeEditorWorldLayoutOpeningSettings settings =
        openingSettings(opening);
    settings.centerOffsetCells = center;
    settings.widthCells = width;
    const cr::CreativeWorldLayoutOpening candidate =
        openingWithSettings(opening, settings);
    const OpeningValidation validation =
        validateOpeningCandidate(state, openingIndex, candidate);
    state.openingManipulation.previewCenterOffsetCells = center;
    state.openingManipulation.previewWidthCells = width;
    state.openingManipulation.previewValid = validation.accepted;
    state.openingManipulation.reasonCode = validation.reasonCode;
    state.statusMessage = validation.accepted ? "opening drag preview"
                                              : validation.message;
    return {true, true, validation.reasonCode};
  }

  const CreativeEditorWorldLayoutEditReceipt updated =
      applyCreativeEditorWorldLayoutOpeningManipulation(
          state, CreativeEditorWorldLayoutOpeningManipulationPhase::Update,
          point, toleranceCells);
  if (!updated.accepted) {
    return updated;
  }
  if (!state.openingManipulation.previewValid) {
    const std::string reasonCode = state.openingManipulation.reasonCode;
    state.openingManipulation = {};
    return {false, false, reasonCode};
  }
  CreativeEditorWorldLayoutOpeningSettings settings = openingSettings(opening);
  settings.centerOffsetCells =
      state.openingManipulation.previewCenterOffsetCells;
  settings.widthCells = state.openingManipulation.previewWidthCells;
  const cr::CreativeWorldLayoutOpening candidate =
      openingWithSettings(opening, settings);
  state.openingManipulation = {};
  return commitOpeningCandidate(state, openingIndex, candidate,
                                "opening updated");
}

}  // namespace iggy3d_creative_app
