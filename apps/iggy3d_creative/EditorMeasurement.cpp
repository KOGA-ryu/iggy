#include "EditorMeasurement.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

#include "app/iggy3d/creative/document/DocumentSnap.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/tools/SelectionResolution.hpp"
#include "app/iggy3d/creative/world/WorldLayoutDimensions.hpp"
#include "app/iggy3d/creative/world/WorldLayoutOpenings.hpp"
#include "EditorState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

using Result = CreativeEditorMeasurementPointResult;

[[nodiscard]] bool finitePoint(cr::CreativeVec3 point) noexcept {
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z);
}

[[nodiscard]] cr::CreativeMeasurementPoint measurementPoint(
    cr::CreativeVec3 point,
    cr::CreativeMeasurementSnapKind snapKind,
    cr::CreativeObjectId objectId = cr::kInvalidObjectId) noexcept {
  cr::CreativeMeasurementPoint result;
  result.x = point.x;
  result.y = point.y;
  result.z = point.z;
  result.snapKind = snapKind;
  if (objectId != cr::kInvalidObjectId) {
    result.target = cr::TargetRef{static_cast<cr::Id>(objectId)};
  }
  return result;
}

void accept(Result& result,
            cr::CreativeMeasurementSnapMode appliedMode,
            cr::CreativeMeasurementPoint point,
            double sourceDistanceMeters,
            std::string_view reasonCode) noexcept {
  result.accepted = true;
  result.status = CreativeEditorMeasurementPointStatus::Ready;
  result.appliedMode = appliedMode;
  result.point = point;
  result.sourceDistanceMeters = sourceDistanceMeters;
  result.reasonCode = reasonCode;
}

void reject(Result& result,
            CreativeEditorMeasurementPointStatus status,
            std::string_view reasonCode) noexcept {
  result.status = status;
  result.reasonCode = reasonCode;
}

[[nodiscard]] bool exactTargetPoint(
    const CreativeEditorWorldTarget& target,
    cr::CreativeVec3& point) noexcept {
  if (!target.valid || !target.grid.resolved ||
      !finitePoint(target.grid.hitPoint)) {
    return false;
  }
  point = target.grid.hitPoint;
  return true;
}

[[nodiscard]] cr::CreativeSemanticSelectionResolution resolveSource(
    const cr::CreativeDocument& document,
    const CreativeEditorWorldTarget& target,
    const cr::CreativeWorldLayout* worldLayout,
    cr::CreativeVec3 hitPoint) noexcept {
  if (!target.objectHit || worldLayout == nullptr) {
    return {};
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (!std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !finitePoint(grid.origin)) {
    return cr::resolveCreativeSemanticSelection(
        document, target.objectId, worldLayout);
  }
  const cr::CreativeVec3 sourcePointCells{
      (hitPoint.x - grid.origin.x) / grid.cellSizeMeters,
      (hitPoint.y - grid.origin.y) / grid.cellSizeMeters,
      (hitPoint.z - grid.origin.z) / grid.cellSizeMeters,
  };
  return cr::resolveCreativeSemanticSelection(
      document, target.objectId, *worldLayout, sourcePointCells);
}

[[nodiscard]] bool resolveSurface(
    Result& result,
    const CreativeEditorWorldTarget& target,
    cr::CreativeVec3 hitPoint) noexcept {
  accept(result, cr::CreativeMeasurementSnapMode::Surface,
         measurementPoint(
             hitPoint, cr::CreativeMeasurementSnapKind::Surface,
             target.objectHit ? target.objectId : cr::kInvalidObjectId),
         0.0, "creative_editor_measurement_surface_ready");
  return true;
}

[[nodiscard]] bool resolveGrid(
    Result& result,
    const cr::CreativeDocument& document,
    cr::CreativeVec3 hitPoint,
    double snapStepMeters) noexcept {
  if (!std::isfinite(snapStepMeters) || snapStepMeters <= 0.0) {
    reject(result, CreativeEditorMeasurementPointStatus::InvalidRequest,
           "creative_editor_measurement_grid_step_invalid");
    return false;
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  cr::CreativeDocumentSnapSettings settings;
  settings.stepX = snapStepMeters;
  settings.stepY = snapStepMeters;
  settings.stepZ = snapStepMeters;
  settings.originX = grid.origin.x;
  settings.originY = grid.origin.y;
  settings.originZ = grid.origin.z;
  const cr::CreativeDocumentSnapReceipt snapped =
      cr::snapCreativeDocumentPoint({hitPoint.x, hitPoint.y, hitPoint.z},
                                    settings);
  if (!snapped.accepted || !snapped.snapped) {
    reject(result, CreativeEditorMeasurementPointStatus::InvalidRequest,
           "creative_editor_measurement_grid_invalid");
    return false;
  }
  const cr::CreativeVec3 point{snapped.snappedPoint.x,
                               snapped.snappedPoint.y,
                               snapped.snappedPoint.z};
  accept(result, cr::CreativeMeasurementSnapMode::Grid,
         measurementPoint(point, cr::CreativeMeasurementSnapKind::Grid),
         std::hypot(point.x - hitPoint.x, point.y - hitPoint.y,
                    point.z - hitPoint.z),
         "creative_editor_measurement_grid_ready");
  return true;
}

[[nodiscard]] bool resolveVertex(
    Result& result,
    const cr::CreativeDocument& document,
    const CreativeEditorWorldTarget& target,
    cr::CreativeVec3 hitPoint) noexcept {
  if (!target.objectHit) {
    reject(result, CreativeEditorMeasurementPointStatus::MissingObject,
           "creative_editor_measurement_vertex_object_missing");
    return false;
  }
  const cr::CreativeObject* object = document.findObject(target.objectId);
  if (object == nullptr) {
    reject(result, CreativeEditorMeasurementPointStatus::MissingObject,
           "creative_editor_measurement_vertex_object_missing");
    return false;
  }
  const cr::CreativeTransformedBounds bounds =
      cr::resolveCreativeObjectBounds(*object);
  if (!bounds.valid) {
    reject(result, CreativeEditorMeasurementPointStatus::InvalidObjectBounds,
           "creative_editor_measurement_vertex_bounds_invalid");
    return false;
  }
  std::size_t nearestIndex = 0U;
  double nearestDistance = std::numeric_limits<double>::infinity();
  for (std::size_t index = 0U; index < bounds.corners.size(); ++index) {
    const cr::CreativeVec3 corner = bounds.corners[index];
    const double distance = std::hypot(
        corner.x - hitPoint.x, corner.y - hitPoint.y,
        corner.z - hitPoint.z);
    if (distance < nearestDistance) {
      nearestDistance = distance;
      nearestIndex = index;
    }
  }
  accept(result, cr::CreativeMeasurementSnapMode::Vertex,
         measurementPoint(bounds.corners[nearestIndex],
                          cr::CreativeMeasurementSnapKind::Vertex,
                          target.objectId),
         nearestDistance, "creative_editor_measurement_vertex_ready");
  return true;
}

[[nodiscard]] bool resolveOpening(
    Result& result,
    const cr::CreativeDocument& document,
    const CreativeEditorWorldTarget& target,
    const cr::CreativeWorldLayout* worldLayout,
    cr::CreativeVec3 hitPoint) noexcept {
  const cr::CreativeSemanticSelectionResolution source =
      resolveSource(document, target, worldLayout, hitPoint);
  if (!source.accepted || !source.worldLayoutSource.owned ||
      source.worldLayoutSource.table != cr::CreativeWorldLayoutTable::Opening ||
      worldLayout == nullptr ||
      source.worldLayoutSource.index >= worldLayout->openings.size()) {
    reject(result, CreativeEditorMeasurementPointStatus::MissingOpening,
           "creative_editor_measurement_opening_missing");
    return false;
  }

  const std::size_t openingIndex = source.worldLayoutSource.index;
  const cr::CreativeWorldLayoutOpening& opening =
      worldLayout->openings[openingIndex];
  const cr::CreativeWorldLayoutOpeningHostFrame host =
      cr::resolveCreativeWorldLayoutOpeningHost(*worldLayout, opening);
  const cr::CreativeWorldLayoutOpeningDimensions dimensions =
      cr::measureCreativeWorldLayoutOpeningDimensions(
          document.gridSettings(), *worldLayout, openingIndex);
  if (!host.accepted || !dimensions.accepted) {
    reject(result,
           CreativeEditorMeasurementPointStatus::InvalidOpeningGeometry,
           "creative_editor_measurement_opening_geometry_invalid");
    return false;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const std::array offsets{
      opening.centerOffsetCells - opening.widthCells * 0.5,
      opening.centerOffsetCells,
      opening.centerOffsetCells + opening.widthCells * 0.5,
  };
  const std::array heights{
      dimensions.cutoutBottomMeters,
      dimensions.cutoutBottomMeters + dimensions.cutoutHeightMeters * 0.5,
      dimensions.cutoutTopMeters,
  };
  cr::CreativeVec3 nearest{};
  double nearestDistance = std::numeric_limits<double>::infinity();
  for (double offset : offsets) {
    const cr::CreativeWorldLayoutOpeningHostPoint hostPoint =
        cr::creativeWorldLayoutOpeningHostPoint(host, offset);
    for (double height : heights) {
      const cr::CreativeVec3 candidate{
          grid.origin.x + hostPoint.x * grid.cellSizeMeters,
          height,
          grid.origin.z + hostPoint.z * grid.cellSizeMeters,
      };
      const double distance = std::hypot(
          candidate.x - hitPoint.x, candidate.y - hitPoint.y,
          candidate.z - hitPoint.z);
      if (distance < nearestDistance) {
        nearest = candidate;
        nearestDistance = distance;
      }
    }
  }
  if (!finitePoint(nearest) || !std::isfinite(nearestDistance)) {
    reject(result,
           CreativeEditorMeasurementPointStatus::InvalidOpeningGeometry,
           "creative_editor_measurement_opening_geometry_invalid");
    return false;
  }
  result.worldLayoutSource = {cr::CreativeWorldLayoutTable::Opening,
                              openingIndex};
  accept(result, cr::CreativeMeasurementSnapMode::Opening,
         measurementPoint(nearest, cr::CreativeMeasurementSnapKind::Opening,
                          target.objectId),
         nearestDistance, "creative_editor_measurement_opening_ready");
  return true;
}

[[nodiscard]] bool resolveLevel(
    Result& result,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout* worldLayout,
    std::size_t activeLevelIndex,
    cr::CreativeVec3 hitPoint) noexcept {
  if (worldLayout == nullptr ||
      activeLevelIndex >= worldLayout->levels.size()) {
    reject(result, CreativeEditorMeasurementPointStatus::MissingLevel,
           "creative_editor_measurement_level_missing");
    return false;
  }
  const cr::CreativeWorldLayoutLevel& level =
      worldLayout->levels[activeLevelIndex];
  const cr::CreativeGridSettings grid = document.gridSettings();
  if (level.buildingIndex >= worldLayout->buildings.size() ||
      !std::isfinite(level.floorTopLayer) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      !finitePoint(grid.origin)) {
    reject(result, CreativeEditorMeasurementPointStatus::MissingLevel,
           "creative_editor_measurement_level_invalid");
    return false;
  }
  const cr::CreativeVec3 point{
      hitPoint.x,
      grid.origin.y + level.floorTopLayer * grid.cellSizeMeters,
      hitPoint.z,
  };
  result.worldLayoutSource = {cr::CreativeWorldLayoutTable::Level,
                              activeLevelIndex};
  accept(result, cr::CreativeMeasurementSnapMode::Level,
         measurementPoint(point, cr::CreativeMeasurementSnapKind::Level),
         std::abs(point.y - hitPoint.y),
         "creative_editor_measurement_level_ready");
  return true;
}

}  // namespace

CreativeEditorMeasurementPointResult resolveCreativeEditorMeasurementPoint(
    const CreativeEditorMeasurementPointRequest& request) noexcept {
  Result result;
  result.requested = true;
  result.requestedMode = request.snapMode;
  if (request.document == nullptr || request.target == nullptr ||
      !request.document->isValid() ||
      request.snapMode >= cr::CreativeMeasurementSnapMode::Count ||
      !std::isfinite(request.vertexToleranceMeters) ||
      request.vertexToleranceMeters < 0.0) {
    reject(result, CreativeEditorMeasurementPointStatus::InvalidRequest,
           "creative_editor_measurement_point_request_invalid");
    return result;
  }

  cr::CreativeVec3 hitPoint;
  if (!exactTargetPoint(*request.target, hitPoint)) {
    reject(result, CreativeEditorMeasurementPointStatus::InvalidTarget,
           "creative_editor_measurement_target_invalid");
    return result;
  }

  switch (request.snapMode) {
    case cr::CreativeMeasurementSnapMode::Grid:
      static_cast<void>(resolveGrid(result, *request.document, hitPoint,
                                    request.snapStepMeters));
      return result;
    case cr::CreativeMeasurementSnapMode::Surface:
      static_cast<void>(resolveSurface(result, *request.target, hitPoint));
      return result;
    case cr::CreativeMeasurementSnapMode::Vertex:
      static_cast<void>(resolveVertex(result, *request.document,
                                      *request.target, hitPoint));
      return result;
    case cr::CreativeMeasurementSnapMode::Opening:
      static_cast<void>(resolveOpening(result, *request.document,
                                       *request.target, request.worldLayout,
                                       hitPoint));
      return result;
    case cr::CreativeMeasurementSnapMode::Level:
      static_cast<void>(resolveLevel(result, *request.document,
                                     request.worldLayout,
                                     request.activeLevelIndex, hitPoint));
      return result;
    case cr::CreativeMeasurementSnapMode::Auto: {
      Result semantic = result;
      if (resolveOpening(semantic, *request.document, *request.target,
                         request.worldLayout, hitPoint)) {
        return semantic;
      }
      Result vertex = result;
      if (resolveVertex(vertex, *request.document, *request.target, hitPoint) &&
          vertex.sourceDistanceMeters <= request.vertexToleranceMeters) {
        return vertex;
      }
      if (request.target->objectHit || request.target->voxelHit ||
          request.target->terrainHit) {
        static_cast<void>(resolveSurface(result, *request.target, hitPoint));
      } else {
        static_cast<void>(resolveGrid(result, *request.document, hitPoint,
                                      request.snapStepMeters));
      }
      return result;
    }
    case cr::CreativeMeasurementSnapMode::Count:
      break;
  }
  reject(result, CreativeEditorMeasurementPointStatus::InvalidRequest,
         "creative_editor_measurement_snap_mode_invalid");
  return result;
}

CreativeEditorMeasurementPointResult resolveCreativeEditorMeasurementPoint(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept {
  const cr::CreativeWorldLayout* worldLayout =
      request.editor.worldLayout.generatedRevision ==
              request.editor.worldLayout.revision
          ? &request.editor.worldLayout.source
          : nullptr;
  const double snapStep =
      cr::creativeSnapIncrementMeters(request.editor.toolSettings.snapIncrement);
  const double vertexTolerance =
      std::clamp(snapStep * 0.25, 0.02, 0.25);
  return resolveCreativeEditorMeasurementPoint(
      {&request.appState.facade.document(),
       &request.editor.interaction.target,
       worldLayout,
       request.editor.worldLayout.activeLevelIndex,
       request.editor.toolSettings.measurementSnapMode,
       snapStep,
       vertexTolerance});
}

namespace {

[[nodiscard]] cr::CreativeMeasurementReceipt configureForFrame(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept {
  return request.appState.facade.configureMeasurement(
      request.editor.toolSettings.measurementMode,
      request.editor.toolSettings.measurementAxis,
      request.editor.toolSettings.measurementClosePath);
}

[[nodiscard]] CreativeEditorMeasurementActionReceipt pointAction(
    const CreativeEditorWorldInteractionFrameRequest& request,
    bool preview) noexcept {
  CreativeEditorMeasurementActionReceipt result;
  result.requested = true;
  const cr::CreativeMeasurementReceipt configured = configureForFrame(request);
  if (!configured.accepted) {
    result.measurement = configured;
    result.reasonCode = "creative_editor_measurement_configuration_rejected";
    return result;
  }
  result.point = resolveCreativeEditorMeasurementPoint(request);
  if (!result.point.accepted) {
    result.reasonCode = result.point.reasonCode;
    return result;
  }
  result.measurement =
      preview ? request.appState.facade.previewMeasurementPoint(
                    result.point.point)
              : request.appState.facade.appendMeasurementPoint(
                    result.point.point);
  result.accepted = result.measurement.accepted;
  result.changed = result.measurement.changed;
  result.reasonCode = result.measurement.message;
  return result;
}

}  // namespace

CreativeEditorMeasurementActionReceipt appendCreativeEditorMeasurementPoint(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept {
  return pointAction(request, false);
}

CreativeEditorMeasurementActionReceipt previewCreativeEditorMeasurementPoint(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept {
  return pointAction(request, true);
}

CreativeEditorMeasurementActionReceipt completeCreativeEditorMeasurement(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept {
  CreativeEditorMeasurementActionReceipt result;
  result.requested = true;
  result.measurement = configureForFrame(request);
  if (!result.measurement.accepted) {
    result.reasonCode = "creative_editor_measurement_configuration_rejected";
    return result;
  }
  result.measurement = request.appState.facade.completeMeasurement();
  result.accepted = result.measurement.accepted;
  result.changed = result.measurement.changed;
  result.reasonCode = result.measurement.message;
  return result;
}

CreativeEditorMeasurementActionReceipt cancelCreativeEditorMeasurement(
    const CreativeEditorWorldInteractionFrameRequest& request) noexcept {
  CreativeEditorMeasurementActionReceipt result;
  result.requested = true;
  result.measurement = request.appState.facade.cancelMeasurement();
  result.accepted = result.measurement.accepted;
  result.changed = result.measurement.changed;
  result.reasonCode = result.measurement.message;
  return result;
}

std::string formatCreativeEditorMeasurementReadout(
    const cr::CreativeMeasurementState& state) {
  const cr::CreativeMeasurementReadout readout =
      cr::buildCreativeMeasurementReadout(state);
  if (!readout.visible) {
    return "POINT 1";
  }
  if (!readout.valid) {
    return "POINT " + std::to_string(state.pointCount + 1U);
  }

  char buffer[192];
  switch (readout.mode) {
    case cr::CreativeMeasurementMode::AxisProjected:
      std::snprintf(buffer, sizeof(buffer), "%s %s %.3f %s",
                    cr::toString(readout.axis).data(),
                    readout.primaryLabel.data(), readout.primaryValue,
                    readout.primaryUnit.data());
      break;
    case cr::CreativeMeasurementMode::Slope:
      if (readout.hasSecondaryValue) {
        std::snprintf(buffer, sizeof(buffer), "%s %+.3f %s | %s %+.2f%s",
                      readout.primaryLabel.data(), readout.primaryValue,
                      readout.primaryUnit.data(),
                      readout.secondaryLabel.data(), readout.secondaryValue,
                      readout.secondaryUnit.data());
      } else {
        std::snprintf(buffer, sizeof(buffer), "%s %+.3f %s | GRADE VERTICAL",
                      readout.primaryLabel.data(), readout.primaryValue,
                      readout.primaryUnit.data());
      }
      break;
    case cr::CreativeMeasurementMode::Area:
      std::snprintf(buffer, sizeof(buffer), "%s %.3f %s | %s %.3f %s",
                    readout.primaryLabel.data(), readout.primaryValue,
                    readout.primaryUnit.data(),
                    readout.secondaryLabel.data(), readout.secondaryValue,
                    readout.secondaryUnit.data());
      break;
    case cr::CreativeMeasurementMode::Vertical:
      std::snprintf(buffer, sizeof(buffer), "%s %+.3f %s",
                    readout.primaryLabel.data(), readout.primaryValue,
                    readout.primaryUnit.data());
      break;
    case cr::CreativeMeasurementMode::Distance:
    case cr::CreativeMeasurementMode::Perimeter:
      std::snprintf(buffer, sizeof(buffer), "%s %.3f %s",
                    readout.primaryLabel.data(), readout.primaryValue,
                    readout.primaryUnit.data());
      break;
    case cr::CreativeMeasurementMode::Count:
      return "INVALID MEASUREMENT";
  }
  std::string result(buffer);
  result.append(" | ");
  result.append(cr::toString(readout.snapKind));
  return result;
}

cr::CreativeMeasurementGeometry
projectCreativeEditorMeasurementGeometryToGrid(
    const cr::CreativeMeasurementGeometry& worldGeometry,
    const cr::CreativeGridSettings& grid) noexcept {
  cr::CreativeMeasurementGeometry result;
  if (!worldGeometry.visible || !std::isfinite(grid.origin.x) ||
      !std::isfinite(grid.origin.y) || !std::isfinite(grid.origin.z) ||
      !std::isfinite(grid.cellSizeMeters) || grid.cellSizeMeters <= 0.0 ||
      worldGeometry.pointCount > result.points.size() ||
      worldGeometry.segmentCount > result.segments.size()) {
    return result;
  }
  const auto projectPoint = [&](cr::CreativeMeasurementPoint point) {
    point.x = (point.x - grid.origin.x) / grid.cellSizeMeters;
    point.y = (point.y - grid.origin.y) / grid.cellSizeMeters;
    point.z = (point.z - grid.origin.z) / grid.cellSizeMeters;
    return point;
  };
  result.visible = true;
  result.completed = worldGeometry.completed;
  result.closed = worldGeometry.closed;
  result.pointCount = worldGeometry.pointCount;
  result.segmentCount = worldGeometry.segmentCount;
  for (std::size_t index = 0U; index < result.pointCount; ++index) {
    result.points[index] = projectPoint(worldGeometry.points[index]);
  }
  for (std::size_t index = 0U; index < result.segmentCount; ++index) {
    result.segments[index] = {
        projectPoint(worldGeometry.segments[index].start),
        projectPoint(worldGeometry.segments[index].end),
    };
  }
  return result;
}

}  // namespace iggy3d_creative_app
