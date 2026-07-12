#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void invalidatePathPreview(CreativeTerrainPathState& path) noexcept {
  path.preview.valid = false;
  path.preview.renderAccepted = false;
  path.preview.patches.clear();
}

void setPathFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

[[nodiscard]] bool resolvePathPoint(const cr::CreativeDocument& document,
                                    const CreativeEditorState& editor,
                                    cr::CreativeTerrainPathPoint& point) noexcept {
  if (!resolveCreativeEditorTerrainPointerCoord(editor, point.coord)) {
    return false;
  }
  const cr::CreativeTerrainHeightSample sample =
      cr::sampleCreativeTerrainHeight(document.terrainField(), point.coord);
  point.heightCells =
      sample.present ? sample.heightCells : editor.terrain.heightCells;
  return true;
}

[[nodiscard]] std::size_t effectivePathPoints(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    std::array<cr::CreativeTerrainPathPoint,
               cr::kCreativeTerrainPathPointCapacity>& points) noexcept {
  const CreativeTerrainPathState& path = editor.terrain.path;
  if (path.pointCount > path.points.size()) {
    return 0U;
  }
  std::copy_n(path.points.begin(), path.pointCount, points.begin());
  std::size_t count = path.pointCount;
  cr::CreativeTerrainPathPoint endpoint{};
  if (!resolvePathPoint(document, editor, endpoint)) {
    return count;
  }
  if (count == 0U) {
    points[count++] = endpoint;
    return count;
  }
  if (endpoint.coord != points[count - 1U].coord && count < points.size()) {
    points[count++] = endpoint;
  }
  return count;
}

[[nodiscard]] cr::CreativeTerrainPathRequest pathRequest(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    std::span<const cr::CreativeTerrainPathPoint> points) noexcept {
  cr::CreativeTerrainPathRequest request;
  request.field = &document.terrainField();
  request.points = points;
  request.kind = editor.toolSettings.terrainPathKind;
  request.elevation = editor.toolSettings.terrainPathElevation;
  request.halfWidthCells = cr::creativeTerrainPathHalfWidthCells(
      editor.toolSettings.terrainPathWidth);
  request.amplitudeCells = cr::creativeTerrainPathAmplitudeCells(
      editor.toolSettings.terrainPathAmplitude);
  return request;
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainPathPreviewCache& cache,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    std::span<const cr::CreativeTerrainPathPoint> points) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.terrainRevision == document.terrainField().revision() &&
         cache.pointCount == points.size() &&
         cache.lockedPointCount == editor.terrain.path.pointCount &&
         std::equal(points.begin(), points.end(), cache.points.begin()) &&
         cache.kind == editor.toolSettings.terrainPathKind &&
         cache.elevation == editor.toolSettings.terrainPathElevation &&
         cache.halfWidthCells == cr::creativeTerrainPathHalfWidthCells(
                                     editor.toolSettings.terrainPathWidth) &&
         cache.amplitudeCells == cr::creativeTerrainPathAmplitudeCells(
                                    editor.toolSettings.terrainPathAmplitude);
}

template <typename Enum>
[[nodiscard]] bool stepClampedEnum(Enum& value,
                                   Enum count,
                                   int direction) noexcept {
  const int before = static_cast<int>(value);
  const int last = static_cast<int>(count) - 1;
  const int after = std::clamp(before + direction, 0, last);
  value = static_cast<Enum>(after);
  return after != before;
}

[[nodiscard]] cr::CreativeVec3 pathPointTop(
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainPathPoint point) noexcept {
  return {grid.origin.x +
              (static_cast<double>(point.coord.x) + 0.5) *
                  grid.cellSizeMeters,
          grid.origin.y + point.heightCells * grid.cellSizeMeters,
          grid.origin.z +
              (static_cast<double>(point.coord.z) + 0.5) *
                  grid.cellSizeMeters};
}

void appendPathLine(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainPathPoint from,
    cr::CreativeTerrainPathPoint to,
    iggy3d::RenderLineColor color,
    float thickness) {
  const cr::CreativeCoreVec3Conversion start =
      cr::creativeVec3ToCoreChecked(pathPointTop(grid, from));
  const cr::CreativeCoreVec3Conversion end =
      cr::creativeVec3ToCoreChecked(pathPointTop(grid, to));
  if (!start.converted || !end.converted) {
    return;
  }
  iggy3d::RenderCreativeWireframeDebugLine line;
  line.start = start.value;
  line.end = end.value;
  line.color = color;
  line.thickness = thickness;
  lines.push_back(line);
}

}  // namespace

cr::CreativeTerrainPathPlan planCreativeEditorTerrainPath(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) noexcept {
  std::array<cr::CreativeTerrainPathPoint,
             cr::kCreativeTerrainPathPointCapacity>
      points{};
  const std::size_t count = effectivePathPoints(document, editor, points);
  if (count < 2U) {
    return {};
  }
  return cr::buildCreativeTerrainPathPlan(
      pathRequest(document, editor, std::span{points.data(), count}));
}

CreativeEditorTerrainPathReceipt addCreativeEditorTerrainPathPoint(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainPathReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainPathAction::AddPoint;
  if (!resolvePathPoint(document, editor, receipt.targetPoint)) {
    receipt.reasonCode = "creative_editor_terrain_path_target_invalid";
    setPathFeedback(editor, false);
    return receipt;
  }
  CreativeTerrainPathState& path = editor.terrain.path;
  if (path.pointCount > path.points.size()) {
    receipt.reasonCode = "creative_editor_terrain_path_state_invalid";
    setPathFeedback(editor, false);
    return receipt;
  }
  if (path.pointCount > 0U &&
      path.points[path.pointCount - 1U].coord == receipt.targetPoint.coord) {
    receipt.accepted = true;
    receipt.reasonCode = "creative_editor_terrain_path_point_already_locked";
    setPathFeedback(editor, true);
    return receipt;
  }
  if (path.pointCount >= path.points.size()) {
    receipt.reasonCode = "creative_editor_terrain_path_point_capacity_exceeded";
    setPathFeedback(editor, false);
    return receipt;
  }
  path.points[path.pointCount++] = receipt.targetPoint;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.reasonCode = "creative_editor_terrain_path_point_added";
  invalidatePathPreview(path);
  setPathFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainPathReceipt removeCreativeEditorTerrainPathPoint(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainPathReceipt receipt;
  receipt.requested = true;
  receipt.accepted = true;
  receipt.action = CreativeEditorTerrainPathAction::RemovePoint;
  CreativeTerrainPathState& path = editor.terrain.path;
  if (path.pointCount > path.points.size()) {
    receipt.accepted = false;
    receipt.reasonCode = "creative_editor_terrain_path_state_invalid";
    setPathFeedback(editor, false);
    return receipt;
  }
  if (path.pointCount == 0U) {
    receipt.reasonCode = "creative_editor_terrain_path_already_empty";
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return receipt;
  }
  receipt.targetPoint = path.points[path.pointCount - 1U];
  --path.pointCount;
  receipt.changed = true;
  receipt.reasonCode = path.pointCount == 0U
                           ? "creative_editor_terrain_path_cancelled"
                           : "creative_editor_terrain_path_point_removed";
  invalidatePathPreview(path);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return receipt;
}

CreativeEditorTerrainPathReceipt cancelCreativeEditorTerrainPath(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainPathReceipt receipt;
  receipt.requested = true;
  receipt.accepted = true;
  receipt.action = CreativeEditorTerrainPathAction::Cancel;
  CreativeTerrainPathState& path = editor.terrain.path;
  receipt.changed = path.pointCount > 0U;
  path.pointCount = 0U;
  receipt.reasonCode = receipt.changed
                           ? "creative_editor_terrain_path_cancelled"
                           : "creative_editor_terrain_path_already_empty";
  invalidatePathPreview(path);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return receipt;
}

CreativeEditorTerrainPathReceipt applyCreativeEditorTerrainPathWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorTerrainPathReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainPathAction::Apply;
  receipt.plan =
      planCreativeEditorTerrainPath(appState.facade.document(), editor);
  if (!receipt.plan.requested) {
    receipt.reasonCode = "creative_editor_terrain_path_needs_endpoint";
    setPathFeedback(editor, false);
    return receipt;
  }
  receipt.accepted = receipt.plan.accepted;
  receipt.reasonCode = receipt.plan.reasonCode;
  if (!receipt.plan.accepted) {
    setPathFeedback(editor, false);
    return receipt;
  }
  if (receipt.plan.items().empty()) {
    static_cast<void>(cancelCreativeEditorTerrainPath(editor));
    setPathFeedback(editor, true);
    return receipt;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.mutation =
      appState.facade.applyTerrainControlEdits(receipt.plan.items());
  editor.terrain.lastMutation = receipt.mutation;
  receipt.accepted = receipt.mutation.accepted;
  receipt.changed = receipt.mutation.changed;
  receipt.reasonCode = receipt.mutation.reasonCode;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.mutation.accepted && receipt.mutation.changed,
      receipt.mutation.reasonCode));
  if (receipt.accepted) {
    static_cast<void>(cancelCreativeEditorTerrainPath(editor));
  }
  setPathFeedback(editor, receipt.accepted);
  return receipt;
}

bool processCreativeEditorTerrainPathQuickEdit(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  bool changed = false;
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
      changed = stepClampedEnum(editor.toolSettings.terrainPathAmplitude,
                                cr::CreativeTerrainPathAmplitude::Count, 1);
      break;
    case cr::CreativeInputActionId::QuickEditNext:
      changed = stepClampedEnum(editor.toolSettings.terrainPathAmplitude,
                                cr::CreativeTerrainPathAmplitude::Count, -1);
      break;
    case cr::CreativeInputActionId::QuickEditDecrease:
      changed = stepClampedEnum(editor.toolSettings.terrainPathWidth,
                                cr::CreativeTerrainPathWidth::Count, -1);
      break;
    case cr::CreativeInputActionId::QuickEditIncrease:
      changed = stepClampedEnum(editor.toolSettings.terrainPathWidth,
                                cr::CreativeTerrainPathWidth::Count, 1);
      break;
    default:
      break;
  }
  if (changed) {
    invalidatePathPreview(editor.terrain.path);
  }
  return changed;
}

std::string creativeEditorTerrainPathQuickEditLabel(
    const CreativeEditorState& editor) {
  std::string label("WIDTH ");
  label.append(std::to_string(cr::creativeTerrainPathWidthCells(
      editor.toolSettings.terrainPathWidth)));
  label.append(cr::creativeTerrainPathUsesDepth(
                   editor.toolSettings.terrainPathKind)
                   ? " | DEPTH "
                   : " | RISE ");
  label.append(std::to_string(cr::creativeTerrainPathAmplitudeCells(
      editor.toolSettings.terrainPathAmplitude)));
  return label;
}

bool refreshCreativeEditorTerrainPathPreview(
    CreativeEditorTerrainState& state,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  std::array<cr::CreativeTerrainPathPoint,
             cr::kCreativeTerrainPathPointCapacity>
      points{};
  const std::size_t pointCount = effectivePathPoints(document, editor, points);
  if (pointCount == 0U) {
    invalidatePathPreview(state.path);
    return false;
  }
  const std::span<const cr::CreativeTerrainPathPoint> pathPoints{
      points.data(), pointCount};
  CreativeTerrainPathPreviewCache& cache = state.path.preview;
  if (previewKeyMatches(cache, document, editor, pathPoints)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.terrainRevision = document.terrainField().revision();
  cache.buildCount = nextBuildCount;
  cache.pointCount = static_cast<std::uint8_t>(pointCount);
  cache.lockedPointCount = state.path.pointCount;
  std::copy(pathPoints.begin(), pathPoints.end(), cache.points.begin());
  cache.kind = editor.toolSettings.terrainPathKind;
  cache.elevation = editor.toolSettings.terrainPathElevation;
  cache.halfWidthCells = cr::creativeTerrainPathHalfWidthCells(
      editor.toolSettings.terrainPathWidth);
  cache.amplitudeCells = cr::creativeTerrainPathAmplitudeCells(
      editor.toolSettings.terrainPathAmplitude);
  if (pointCount < 2U) {
    return true;
  }
  cache.plan = cr::buildCreativeTerrainPathPlan(
      pathRequest(document, editor, pathPoints));
  if (!cache.plan.accepted) {
    return true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainMutationPreviewReceipt preview =
      cr::buildCreativeTerrainMutationPreview(
          document.terrainField(), cache.plan.items(), grid.origin,
          grid.cellSizeMeters);
  if (!preview.accepted) {
    return true;
  }
  std::uint16_t expansion = 1U;
  for (const cr::CreativeTerrainControlPoint& control :
       cache.plan.finalControls()) {
    expansion = std::max(expansion, control.radiusCells);
  }
  const std::int64_t minimumX =
      static_cast<std::int64_t>(cache.plan.minimumCoord.x) - expansion;
  const std::int64_t minimumZ =
      static_cast<std::int64_t>(cache.plan.minimumCoord.z) - expansion;
  const std::int64_t maximumX =
      static_cast<std::int64_t>(cache.plan.maximumCoord.x) + expansion;
  const std::int64_t maximumZ =
      static_cast<std::int64_t>(cache.plan.maximumCoord.z) + expansion;
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.render.patches) {
    if (patch.coord.x >= minimumX && patch.coord.x <= maximumX &&
        patch.coord.z >= minimumZ && patch.coord.z <= maximumZ) {
      cache.patches.push_back(patch);
    }
  }
  cache.renderAccepted = true;
  return true;
}

void appendCreativeEditorTerrainPathOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeTerrainPathPreviewCache& preview = editor.terrain.path.preview;
  if (!preview.valid || preview.pointCount == 0U) {
    return;
  }
  constexpr iggy3d::RenderLineColor locked{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor live{0.98F, 0.88F, 0.16F, 1.0F};
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  const bool complete = preview.pointCount >= 2U;
  const bool accepted =
      complete && preview.plan.accepted && preview.renderAccepted;
  const iggy3d::RenderLineColor routeColor =
      !complete ? live : accepted ? admitted : rejected;
  const cr::CreativeGridSettings grid = document.gridSettings();

  for (std::size_t index = 0U; index < preview.pointCount; ++index) {
    const bool isLive = index >= preview.lockedPointCount;
    appendCreativeEditorTerrainControlGuide(
        wireLines, grid,
        {preview.points[index].coord, preview.points[index].heightCells, 1U},
        isLive ? live : locked, wireThickness * 1.5F);
    if (index > 0U) {
      appendPathLine(wireLines, grid, preview.points[index - 1U],
                     preview.points[index], routeColor,
                     wireThickness * 1.4F);
    }
  }
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainControlPoint& control :
       preview.plan.finalControls()) {
    appendCreativeEditorTerrainControlGuide(
        wireLines, grid, control, admitted, wireThickness);
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.7F);
  }
}

}  // namespace iggy3d_creative_app
