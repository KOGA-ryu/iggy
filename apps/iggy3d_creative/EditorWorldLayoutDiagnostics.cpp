#include "EditorWorldLayoutDiagnostics.hpp"

#include "EditorWorldLayout.hpp"
#include "EditorWorldLayoutInternal.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace iggy3d_creative_app {
namespace {

namespace cr = iggy3d::creative;

std::string diagnosticSubject(const cr::CreativeWorldLayoutReceipt& receipt) {
  if (receipt.failedTable == cr::CreativeWorldLayoutTable::None ||
      receipt.failedIndex == cr::kInvalidCreativeWorldLayoutIndex) {
    return "Layout";
  }
  return std::string(cr::toString(receipt.failedTable)) + " " +
         std::to_string(receipt.failedIndex + 1U);
}

std::string diagnosticMessage(const cr::CreativeWorldLayoutReceipt& receipt) {
  const std::string subject = diagnosticSubject(receipt);
  switch (receipt.status) {
    case cr::CreativeWorldLayoutStatus::InvalidDocument:
      return "Document is not ready for layout generation";
    case cr::CreativeWorldLayoutStatus::InvalidSchema:
      return "Layout header or ownership policy is invalid";
    case cr::CreativeWorldLayoutStatus::Empty:
      return "Add at least one layout symbol";
    case cr::CreativeWorldLayoutStatus::DuplicateStableKey:
      return subject + " duplicates an existing stable key";
    case cr::CreativeWorldLayoutStatus::InvalidSymbol:
      return subject + " has invalid settings or ownership";
    case cr::CreativeWorldLayoutStatus::KernelRejected:
      return subject + " failed geometry validation";
    case cr::CreativeWorldLayoutStatus::CapacityExceeded:
      return "Generated layout exceeds engine capacity";
    case cr::CreativeWorldLayoutStatus::MutationRejected:
      return subject + " could not stage its terrain changes";
    case cr::CreativeWorldLayoutStatus::ObjectRejected:
      return subject + " could not materialize its object recipe";
    case cr::CreativeWorldLayoutStatus::InstallRejected:
      return "Generated layout could not be installed";
    case cr::CreativeWorldLayoutStatus::StalePlan:
      return "Layout source changed during generation";
    case cr::CreativeWorldLayoutStatus::NotRequested:
      return "Layout preflight was not requested";
    case cr::CreativeWorldLayoutStatus::NoChange:
    case cr::CreativeWorldLayoutStatus::Ready:
    case cr::CreativeWorldLayoutStatus::Applied:
      break;
  }
  return subject + " is not ready";
}

struct DiagnosticTarget {
  bool valid = false;
  CreativeEditorWorldLayoutSelection selection;
  std::size_t activeLevelIndex = cr::kInvalidCreativeWorldLayoutIndex;
  std::size_t buildingIndex = cr::kInvalidCreativeWorldLayoutIndex;
  bool hasCenter = false;
  CreativeEditorWorldLayoutPoint center;
};

CreativeEditorWorldLayoutPoint rectCenter(cr::CreativeWorldLayoutRect value) {
  return {(static_cast<double>(value.minimum.x) + value.maximum.x) * 0.5,
          (static_cast<double>(value.minimum.z) + value.maximum.z) * 0.5};
}

bool resolveBuildingCenter(const CreativeEditorWorldLayoutState& state,
                           std::size_t buildingIndex,
                           CreativeEditorWorldLayoutPoint& center) noexcept {
  cr::CreativeWorldLayoutBuildingBounds bounds;
  if (!cr::measureCreativeWorldLayoutBuildingBounds(state.source,
                                                     buildingIndex, bounds)) {
    return false;
  }
  center = {(static_cast<double>(bounds.minimum.x) + bounds.maximum.x) * 0.5,
            (static_cast<double>(bounds.minimum.z) + bounds.maximum.z) * 0.5};
  return true;
}

DiagnosticTarget resolveDiagnosticTarget(
    const CreativeEditorWorldLayoutState& state,
    cr::CreativeWorldLayoutTable table, std::size_t index) {
  DiagnosticTarget target;
  const cr::CreativeWorldLayout& source = state.source;
  switch (table) {
    case cr::CreativeWorldLayoutTable::Building:
      if (index >= source.buildings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                          index};
      target.buildingIndex = index;
      target.hasCenter = resolveBuildingCenter(state, index, target.center);
      break;
    case cr::CreativeWorldLayoutTable::Level:
      if (index >= source.levels.size()) return target;
      target.activeLevelIndex = index;
      target.buildingIndex = source.levels[index].buildingIndex;
      if (target.buildingIndex >= source.buildings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Building,
                          target.buildingIndex};
      target.hasCenter =
          resolveBuildingCenter(state, target.buildingIndex, target.center);
      break;
    case cr::CreativeWorldLayoutTable::Room:
      if (index >= source.rooms.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Room, index};
      target.activeLevelIndex = source.rooms[index].levelIndex;
      target.buildingIndex = source.rooms[index].buildingIndex;
      target.center = rectCenter(source.rooms[index].footprint);
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::VerticalConnector:
      if (index >= source.verticalConnectors.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::VerticalConnector, index};
      target.buildingIndex = source.verticalConnectors[index].buildingIndex;
      target.center = rectCenter(source.verticalConnectors[index].footprint);
      target.hasCenter = true;
      if (source.verticalConnectors[index].lowerRoomIndex < source.rooms.size()) {
        target.activeLevelIndex =
            source.rooms[source.verticalConnectors[index].lowerRoomIndex]
                .levelIndex;
      }
      break;
    case cr::CreativeWorldLayoutTable::Box:
      if (index >= source.boxes.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Box, index};
      target.buildingIndex = source.boxes[index].buildingIndex;
      target.center = rectCenter(source.boxes[index].footprint);
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::Wall:
      if (index >= source.walls.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Wall, index};
      target.buildingIndex = source.walls[index].buildingIndex;
      target.center = {
          (static_cast<double>(source.walls[index].start.x) +
           source.walls[index].end.x) *
              0.5,
          (static_cast<double>(source.walls[index].start.z) +
           source.walls[index].end.z) *
              0.5};
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::Opening: {
      if (index >= source.openings.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Opening,
                          index};
      const cr::CreativeWorldLayoutOpening& opening = source.openings[index];
      const CreativeEditorWorldLayoutOpeningHost host =
          resolveCreativeEditorWorldLayoutOpeningHost(state, index);
      if (host.valid && host.lengthCells > 0.0) {
        const double t = std::clamp(opening.centerOffsetCells /
                                        host.lengthCells,
                                    0.0, 1.0);
        target.center = {host.start.x + (host.end.x - host.start.x) * t,
                         host.start.z + (host.end.z - host.start.z) * t};
        target.hasCenter = true;
      }
      if (opening.hostKind == cr::CreativeWorldLayoutOpeningHostKind::Wall &&
          opening.wallIndex < source.walls.size()) {
        target.buildingIndex =
            source.walls[opening.wallIndex].buildingIndex;
      } else if (opening.hostKind ==
                     cr::CreativeWorldLayoutOpeningHostKind::RoomEdge &&
                 opening.roomIndex < source.rooms.size()) {
        target.buildingIndex =
            source.rooms[opening.roomIndex].buildingIndex;
        target.activeLevelIndex =
            source.rooms[opening.roomIndex].levelIndex;
      }
      break;
    }
    case cr::CreativeWorldLayoutTable::Object: {
      if (index >= source.objects.size()) return target;
      target.selection = {CreativeEditorWorldLayoutSelectionKind::Object,
                          index};
      const cr::CreativeWorldLayoutObject& object = source.objects[index];
      target.center =
          object.mode == cr::CreativeObjectLibraryPlacementMode::Bounds
              ? CreativeEditorWorldLayoutPoint{
                    (object.boundsCells.min.x + object.boundsCells.max.x) * 0.5,
                    (object.boundsCells.min.z + object.boundsCells.max.z) * 0.5}
              : CreativeEditorWorldLayoutPoint{object.pointCells.x,
                                               object.pointCells.z};
      target.hasCenter = std::isfinite(target.center.x) &&
                         std::isfinite(target.center.z);
      break;
    }
    case cr::CreativeWorldLayoutTable::TerrainProfile:
      if (index >= source.terrainProfiles.size()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainProfile, index};
      target.center = {
          static_cast<double>(source.terrainProfiles[index].center.x),
          static_cast<double>(source.terrainProfiles[index].center.z)};
      target.hasCenter = true;
      break;
    case cr::CreativeWorldLayoutTable::TerrainPath: {
      if (index >= source.terrainPaths.size()) return target;
      const cr::CreativeWorldLayoutTerrainPath& path =
          source.terrainPaths[index];
      if (path.pointCount == 0U ||
          path.firstPointIndex > source.terrainPathPoints.size() ||
          path.pointCount >
              source.terrainPathPoints.size() - path.firstPointIndex) {
        return target;
      }
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainPath, index};
      double sumX = 0.0;
      double sumZ = 0.0;
      for (std::size_t pointIndex = 0U; pointIndex < path.pointCount;
           ++pointIndex) {
        const cr::CreativeTerrainPathPoint& point =
            source.terrainPathPoints[path.firstPointIndex + pointIndex];
        sumX += point.coord.x;
        sumZ += point.coord.z;
      }
      const double pointCount = static_cast<double>(path.pointCount);
      target.center = {sumX / pointCount, sumZ / pointCount};
      target.hasCenter = true;
      break;
    }
    case cr::CreativeWorldLayoutTable::TerrainPathPoint: {
      if (index >= source.terrainPathPoints.size()) return target;
      const auto owner = std::find_if(
          source.terrainPaths.begin(), source.terrainPaths.end(),
          [index](const cr::CreativeWorldLayoutTerrainPath& path) {
            return index >= path.firstPointIndex &&
                   index - path.firstPointIndex < path.pointCount;
          });
      if (owner == source.terrainPaths.end()) return target;
      target.selection = {
          CreativeEditorWorldLayoutSelectionKind::TerrainPath,
          static_cast<std::size_t>(owner - source.terrainPaths.begin())};
      target.center = {
          static_cast<double>(source.terrainPathPoints[index].coord.x),
          static_cast<double>(source.terrainPathPoints[index].coord.z)};
      target.hasCenter = true;
      break;
    }
    case cr::CreativeWorldLayoutTable::None:
      return target;
  }
  target.valid = true;
  return target;
}

}  // namespace

CreativeEditorWorldLayoutDiagnosticReport
buildCreativeEditorWorldLayoutDiagnosticReport(
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout) {
  CreativeEditorWorldLayoutDiagnosticReport report;
  const cr::CreativeWorldLayoutCompileResult compiled =
      cr::buildCreativeWorldLayoutPlan(document, layout);
  report.compileReceipt = compiled.receipt;
  report.ready = compiled.receipt.accepted;
  report.hasChanges = compiled.receipt.accepted &&
                      compiled.receipt.status ==
                          cr::CreativeWorldLayoutStatus::Ready;
  if (!report.ready) {
    CreativeEditorWorldLayoutDiagnostic& issue = report.issues[0];
    issue.status = compiled.receipt.status;
    issue.table = compiled.receipt.failedTable;
    issue.index = compiled.receipt.failedIndex;
    issue.message = diagnosticMessage(compiled.receipt);
    issue.reasonCode = compiled.receipt.reasonCode;
    issue.kernelReasonCode = compiled.receipt.kernelReasonCode;
    report.issueCount = 1U;
  }
  return report;
}

const CreativeEditorWorldLayoutDiagnosticReport&
refreshCreativeEditorWorldLayoutDiagnostics(
    CreativeEditorWorldLayoutDiagnosticCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeWorldLayout& layout, std::uint64_t layoutRevision) {
  const std::uint64_t terrainRevision = document.terrainField().revision();
  const std::uint64_t materialRevision =
      document.terrainMaterialField().revision();
  if (cache.valid && cache.layoutRevision == layoutRevision &&
      cache.documentId == document.id() &&
      cache.documentRevision == document.revision() &&
      cache.terrainRevision == terrainRevision &&
      cache.materialRevision == materialRevision) {
    return cache.report;
  }
  cache.layoutRevision = layoutRevision;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.terrainRevision = terrainRevision;
  cache.materialRevision = materialRevision;
  cache.report = buildCreativeEditorWorldLayoutDiagnosticReport(document,
                                                                 layout);
  cache.valid = true;
  ++cache.buildCount;
  return cache.report;
}

CreativeEditorWorldLayoutEditReceipt
focusCreativeEditorWorldLayoutDiagnostic(
    CreativeEditorWorldLayoutState& state, cr::CreativeWorldLayoutTable table,
    std::size_t index) {
  const DiagnosticTarget target = resolveDiagnosticTarget(state, table, index);
  if (!target.valid) {
    state.statusMessage = "validation issue has no selectable symbol";
    return {false, false,
            "creative_editor_world_layout_diagnostic_target_invalid"};
  }

  const bool changed = state.tool != CreativeEditorWorldLayoutTool::Select ||
                       state.selection.kind != target.selection.kind ||
                       state.selection.index != target.selection.index ||
                       (target.activeLevelIndex !=
                            cr::kInvalidCreativeWorldLayoutIndex &&
                        state.activeLevelIndex != target.activeLevelIndex);
  detail::clearWorldLayoutInteraction(state);
  state.anchorActive = false;
  state.tool = CreativeEditorWorldLayoutTool::Select;
  state.selection = target.selection;
  if (target.activeLevelIndex < state.source.levels.size()) {
    state.activeLevelIndex = target.activeLevelIndex;
  } else if (target.buildingIndex < state.source.buildings.size()) {
    repairCreativeEditorWorldLayoutActiveLevel(state, target.buildingIndex);
  }
  if (target.hasCenter) {
    state.canvasPanX =
        -static_cast<float>(target.center.x) * state.canvasPixelsPerCell;
    state.canvasPanZ =
        -static_cast<float>(target.center.z) * state.canvasPixelsPerCell;
    const double horizontal =
        state.elevationAxis == CreativeEditorWorldLayoutElevationAxis::X
            ? target.center.x
            : target.center.z;
    state.elevationPanHorizontal =
        -static_cast<float>(horizontal) * state.elevationPixelsPerCell;
  }
  state.statusMessage = "validation issue selected";
  return {true, changed,
          "creative_editor_world_layout_diagnostic_target_focused"};
}

}  // namespace iggy3d_creative_app
