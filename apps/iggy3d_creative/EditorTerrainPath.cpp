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
#include <limits>
#include <optional>
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

[[nodiscard]] std::uint16_t terrainHeightAt(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 coord,
    std::uint16_t fallback) noexcept {
  const std::optional<std::uint16_t> authored =
      document.terrainHeightField().heightAt(coord);
  if (authored.has_value() &&
      *authored >= cr::kCreativeTerrainMinimumHeightCells) {
    return *authored;
  }
  const cr::CreativeTerrainHeightSample legacy =
      cr::sampleCreativeTerrainHeight(document.terrainField(), coord);
  return legacy.present ? legacy.heightCells : fallback;
}

[[nodiscard]] bool resolvePathPoint(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainPathSourcePoint& point) noexcept {
  if (!resolveCreativeEditorTerrainPointerCoord(editor, point.coord)) {
    return false;
  }
  point.heightCells = terrainHeightAt(
      document, point.coord, editor.terrain.heightCells);
  point.halfWidthCells = cr::creativeTerrainPathHalfWidthCells(
      editor.toolSettings.terrainPathWidth);
  point.amplitudeCells = cr::creativeTerrainPathAmplitudeCells(
      editor.toolSettings.terrainPathAmplitude);
  return true;
}

[[nodiscard]] cr::CreativeTerrainPathCrossSection pathCrossSection(
    cr::CreativeTerrainPathKind kind) noexcept {
  switch (kind) {
    case cr::CreativeTerrainPathKind::Road:
      return cr::CreativeTerrainPathCrossSection::Flat;
    case cr::CreativeTerrainPathKind::River:
      return cr::CreativeTerrainPathCrossSection::Channel;
    case cr::CreativeTerrainPathKind::Ridge:
      return cr::CreativeTerrainPathCrossSection::Berm;
    case cr::CreativeTerrainPathKind::Trench:
      return cr::CreativeTerrainPathCrossSection::Cut;
    case cr::CreativeTerrainPathKind::Count:
      return cr::CreativeTerrainPathCrossSection::Count;
  }
  return cr::CreativeTerrainPathCrossSection::Count;
}

[[nodiscard]] cr::CreativeTerrainMaterial pathMaterial(
    cr::CreativeTerrainPathKind kind) noexcept {
  switch (kind) {
    case cr::CreativeTerrainPathKind::Road:
      return cr::CreativeTerrainMaterial::Dirt;
    case cr::CreativeTerrainPathKind::River:
      return cr::CreativeTerrainMaterial::Sand;
    case cr::CreativeTerrainPathKind::Ridge:
    case cr::CreativeTerrainPathKind::Trench:
      return cr::CreativeTerrainMaterial::Stone;
    case cr::CreativeTerrainPathKind::Count:
      return cr::CreativeTerrainMaterial::Count;
  }
  return cr::CreativeTerrainMaterial::Count;
}

[[nodiscard]] std::size_t pointIndexById(
    const CreativeTerrainPathState& state,
    cr::CreativeTerrainPathSourcePointId pointId) noexcept {
  for (std::size_t index = 0U; index < state.pointCount; ++index) {
    if (state.points[index].id == pointId) {
      return index;
    }
  }
  return state.pointCount;
}

[[nodiscard]] cr::CreativeTerrainPathSourceRecipe effectivePathRecipe(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  const CreativeTerrainPathState& path = editor.terrain.path;
  cr::CreativeTerrainPathSourceRecipe recipe;
  recipe.kind = editor.toolSettings.terrainPathKind;
  recipe.elevation = editor.toolSettings.terrainPathElevation;
  recipe.curve = path.curve;
  recipe.crossSection = path.crossSection;
  recipe.startJoin = path.startJoin;
  recipe.endJoin = path.endJoin;
  recipe.falloffCells = path.falloffCells;
  recipe.paintSurface = path.paintSurface;
  recipe.material = path.material;
  recipe.nextPointId = path.nextPointId;
  if (path.pointCount > path.points.size() ||
      path.nextPointId == cr::kInvalidCreativeTerrainPathSourcePointId) {
    recipe.nextPointId = cr::kInvalidCreativeTerrainPathSourcePointId;
    return recipe;
  }
  recipe.points.assign(path.points.begin(),
                       path.points.begin() + path.pointCount);

  cr::CreativeTerrainPathSourcePoint live{};
  if (!resolvePathPoint(document, editor, live)) {
    return recipe;
  }
  const std::size_t selected = pointIndexById(path, path.selectedPointId);
  if (path.editingOperationId != cr::kInvalidCreativeTerrainOperationId &&
      path.selectedPointFollowsPointer && selected < recipe.points.size()) {
    recipe.points[selected].coord = live.coord;
    recipe.points[selected].heightCells = live.heightCells;
    return recipe;
  }
  if (path.editingOperationId != cr::kInvalidCreativeTerrainOperationId) {
    return recipe;
  }
  if ((recipe.points.empty() || live.coord != recipe.points.back().coord) &&
      recipe.points.size() < cr::kCreativeTerrainPathPointCapacity &&
      recipe.nextPointId <
          std::numeric_limits<cr::CreativeTerrainPathSourcePointId>::max()) {
    live.id = recipe.nextPointId++;
    recipe.points.push_back(live);
  }
  return recipe;
}

[[nodiscard]] cr::CreativeTerrainOperationMutationRequest pathRequest(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainPathSourceRecipe recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = editor.terrain.path.editingOperationId ==
                         cr::kInvalidCreativeTerrainOperationId
                     ? cr::CreativeTerrainOperationMutationKind::Add
                     : cr::CreativeTerrainOperationMutationKind::Update;
  request.operationId = editor.terrain.path.editingOperationId;
  request.operationKind = cr::CreativeTerrainOperationKind::Path;
  request.path = std::move(recipe);
  const cr::CreativeTerrainOperation* existing =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       request.operationId);
  request.enabled = existing == nullptr ? true : existing->enabled;
  return request;
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainPathPreviewCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainPathSourceRecipe& recipe) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.documentRevision == document.revision() &&
         cache.recipe == recipe;
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
    cr::CreativeTerrainPathSourcePoint point) noexcept {
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
    cr::CreativeTerrainPathSourcePoint from,
    cr::CreativeTerrainPathSourcePoint to,
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

struct PathHandleMatch {
  const cr::CreativeTerrainOperation* operation = nullptr;
  cr::CreativeTerrainPathSourcePointId pointId =
      cr::kInvalidCreativeTerrainPathSourcePointId;
};

[[nodiscard]] PathHandleMatch findPathHandle(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 coord) noexcept {
  const std::vector<cr::CreativeTerrainOperation>& operations =
      document.terrainOperationStack().operations;
  for (auto operation = operations.rbegin(); operation != operations.rend();
       ++operation) {
    if (operation->owner != cr::CreativeTerrainOperationOwner::Manual ||
        operation->kind != cr::CreativeTerrainOperationKind::Path) {
      continue;
    }
    const auto point = std::find_if(
        operation->path.points.begin(), operation->path.points.end(),
        [coord](const cr::CreativeTerrainPathSourcePoint& candidate) {
          return candidate.coord == coord;
        });
    if (point != operation->path.points.end()) {
      return {&*operation, point->id};
    }
  }
  return {};
}

void clearPathDraft(CreativeTerrainPathState& state) noexcept {
  state.pointCount = 0U;
  state.nextPointId = 1U;
  state.selectedPointId = cr::kInvalidCreativeTerrainPathSourcePointId;
  state.selectedPointFollowsPointer = false;
  state.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
  state.curve = cr::CreativeTerrainPathCurvePolicy::Linear;
  state.crossSection = cr::CreativeTerrainPathCrossSection::Flat;
  state.startJoin = cr::CreativeTerrainPathEndpointJoin::Open;
  state.endJoin = cr::CreativeTerrainPathEndpointJoin::Open;
  state.falloffCells = 2U;
  state.paintSurface = true;
  state.material = cr::CreativeTerrainMaterial::Dirt;
  invalidatePathPreview(state);
}

[[nodiscard]] cr::CreativeTerrainPathAmplitude amplitudeSetting(
    std::uint16_t amplitudeCells) noexcept {
  constexpr std::array<std::uint16_t, 4U> values{{1U, 2U, 4U, 8U}};
  std::size_t best = 0U;
  for (std::size_t index = 1U; index < values.size(); ++index) {
    if (std::abs(static_cast<int>(values[index]) - amplitudeCells) <
        std::abs(static_cast<int>(values[best]) - amplitudeCells)) {
      best = index;
    }
  }
  return static_cast<cr::CreativeTerrainPathAmplitude>(best);
}

void loadPathOperation(const cr::CreativeTerrainOperation& operation,
                       cr::CreativeTerrainPathSourcePointId selectedPointId,
                       CreativeEditorState& editor) {
  CreativeTerrainPathState& state = editor.terrain.path;
  state.pointCount = static_cast<std::uint8_t>(operation.path.points.size());
  std::copy(operation.path.points.begin(), operation.path.points.end(),
            state.points.begin());
  state.nextPointId = operation.path.nextPointId;
  state.selectedPointId = selectedPointId;
  state.selectedPointFollowsPointer = true;
  state.editingOperationId = operation.id;
  state.curve = operation.path.curve;
  state.crossSection = operation.path.crossSection;
  state.startJoin = operation.path.startJoin;
  state.endJoin = operation.path.endJoin;
  state.falloffCells = operation.path.falloffCells;
  state.paintSurface = operation.path.paintSurface;
  state.material = operation.path.material;
  editor.toolSettings.terrainPathKind = operation.path.kind;
  editor.toolSettings.terrainPathElevation = operation.path.elevation;
  const std::size_t selectedIndex = pointIndexById(state, selectedPointId);
  if (selectedIndex < state.pointCount) {
    editor.toolSettings.terrainPathWidth =
        static_cast<cr::CreativeTerrainPathWidth>(std::min<std::uint16_t>(
            state.points[selectedIndex].halfWidthCells, 3U));
    editor.toolSettings.terrainPathAmplitude =
        amplitudeSetting(state.points[selectedIndex].amplitudeCells);
  }
  invalidatePathPreview(state);
}

}  // namespace

cr::CreativeTerrainOperationMutationPlan planCreativeEditorTerrainPath(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  cr::CreativeTerrainPathSourceRecipe recipe =
      effectivePathRecipe(document, editor);
  if (recipe.points.size() < 2U) {
    return {};
  }
  return cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      pathRequest(document, editor, std::move(recipe)));
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
  const PathHandleMatch match =
      findPathHandle(document, receipt.targetPoint.coord);
  if (match.operation != nullptr) {
    loadPathOperation(*match.operation, match.pointId, editor);
    receipt.targetPoint =
        editor.terrain.path.points[pointIndexById(editor.terrain.path,
                                                 match.pointId)];
    receipt.accepted = true;
    receipt.changed = true;
    receipt.reasonCode = "creative_editor_terrain_path_reopened";
    setPathFeedback(editor, true);
    return receipt;
  }
  if (path.pointCount == 0U) {
    path.crossSection = pathCrossSection(editor.toolSettings.terrainPathKind);
    path.material = pathMaterial(editor.toolSettings.terrainPathKind);
  }
  if (path.pointCount > 0U &&
      path.points[path.pointCount - 1U].coord == receipt.targetPoint.coord) {
    receipt.accepted = true;
    receipt.reasonCode = "creative_editor_terrain_path_point_already_locked";
    setPathFeedback(editor, true);
    return receipt;
  }
  if (path.pointCount >= path.points.size() ||
      path.nextPointId ==
          std::numeric_limits<cr::CreativeTerrainPathSourcePointId>::max()) {
    receipt.reasonCode = "creative_editor_terrain_path_point_capacity_exceeded";
    setPathFeedback(editor, false);
    return receipt;
  }
  std::size_t insertIndex = path.pointCount;
  if (path.editingOperationId != cr::kInvalidCreativeTerrainOperationId) {
    const std::size_t selected = pointIndexById(path, path.selectedPointId);
    if (selected >= path.pointCount) {
      receipt.reasonCode = "creative_editor_terrain_path_point_not_selected";
      setPathFeedback(editor, false);
      return receipt;
    }
    insertIndex = selected + 1U;
    std::move_backward(path.points.begin() + insertIndex,
                       path.points.begin() + path.pointCount,
                       path.points.begin() + path.pointCount + 1U);
  }
  receipt.targetPoint.id = path.nextPointId++;
  path.points[insertIndex] = receipt.targetPoint;
  ++path.pointCount;
  path.selectedPointId = receipt.targetPoint.id;
  path.selectedPointFollowsPointer =
      path.editingOperationId != cr::kInvalidCreativeTerrainOperationId;
  receipt.accepted = true;
  receipt.changed = true;
  receipt.reasonCode =
      path.editingOperationId == cr::kInvalidCreativeTerrainOperationId
          ? "creative_editor_terrain_path_point_added"
          : "creative_editor_terrain_path_point_inserted";
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
  std::size_t removeIndex = path.pointCount - 1U;
  if (path.editingOperationId != cr::kInvalidCreativeTerrainOperationId) {
    removeIndex = pointIndexById(path, path.selectedPointId);
    if (removeIndex >= path.pointCount) {
      receipt.accepted = false;
      receipt.reasonCode = "creative_editor_terrain_path_point_not_selected";
      setPathFeedback(editor, false);
      return receipt;
    }
    if (path.pointCount <= 2U) {
      clearPathDraft(path);
      receipt.changed = true;
      receipt.reasonCode = "creative_editor_terrain_path_edit_cancelled";
      clearCreativeEditorPlacementFeedback(editor.interaction);
      return receipt;
    }
  }
  receipt.targetPoint = path.points[removeIndex];
  std::move(path.points.begin() + removeIndex + 1U,
            path.points.begin() + path.pointCount,
            path.points.begin() + removeIndex);
  --path.pointCount;
  path.selectedPointId = path.pointCount == 0U
                             ? cr::kInvalidCreativeTerrainPathSourcePointId
                             : path.points[std::min(removeIndex,
                                                    static_cast<std::size_t>(
                                                        path.pointCount - 1U))]
                                   .id;
  path.selectedPointFollowsPointer = false;
  receipt.changed = true;
  receipt.reasonCode = path.pointCount == 0U
                           ? "creative_editor_terrain_path_cancelled"
                           : "creative_editor_terrain_path_point_removed";
  invalidatePathPreview(path);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return receipt;
}

CreativeEditorTerrainPathReceipt reorderCreativeEditorTerrainPathPoint(
    CreativeEditorState& editor, int direction) noexcept {
  CreativeEditorTerrainPathReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainPathAction::ReorderPoint;
  CreativeTerrainPathState& path = editor.terrain.path;
  if (path.editingOperationId == cr::kInvalidCreativeTerrainOperationId ||
      path.pointCount > path.points.size() || direction == 0) {
    receipt.reasonCode = "creative_editor_terrain_path_reorder_invalid";
    setPathFeedback(editor, false);
    return receipt;
  }
  const std::size_t selected = pointIndexById(path, path.selectedPointId);
  if (selected >= path.pointCount) {
    receipt.reasonCode = "creative_editor_terrain_path_point_not_selected";
    setPathFeedback(editor, false);
    return receipt;
  }
  const std::size_t target = direction < 0
                                 ? selected == 0U ? selected : selected - 1U
                                 : selected + 1U >= path.pointCount
                                       ? selected
                                       : selected + 1U;
  receipt.targetPoint = path.points[selected];
  receipt.accepted = true;
  if (target == selected) {
    receipt.reasonCode = "creative_editor_terrain_path_reorder_no_change";
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return receipt;
  }
  std::swap(path.points[selected], path.points[target]);
  path.selectedPointFollowsPointer = false;
  receipt.changed = true;
  receipt.reasonCode = "creative_editor_terrain_path_point_reordered";
  invalidatePathPreview(path);
  setPathFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainPathReceipt cancelCreativeEditorTerrainPath(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainPathReceipt receipt;
  receipt.requested = true;
  receipt.accepted = true;
  receipt.action = CreativeEditorTerrainPathAction::Cancel;
  CreativeTerrainPathState& path = editor.terrain.path;
  receipt.changed = path.pointCount > 0U ||
                    path.editingOperationId !=
                        cr::kInvalidCreativeTerrainOperationId;
  clearPathDraft(path);
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
  if (!receipt.plan.receipt.requested) {
    receipt.reasonCode = "creative_editor_terrain_path_needs_endpoint";
    setPathFeedback(editor, false);
    return receipt;
  }
  receipt.accepted = receipt.plan.receipt.accepted;
  receipt.reasonCode = receipt.plan.receipt.reasonCode;
  if (!receipt.plan.receipt.accepted) {
    setPathFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeTerrainOperationMutationRequest request =
      pathRequest(document, editor, receipt.plan.stack.operations[
          receipt.plan.receipt.operationIndexAfter].path);
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.operation = appState.facade.applyTerrainOperationMutation(request);
  receipt.history = completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.operation.accepted && receipt.operation.changed,
      receipt.operation.reasonCode);
  receipt.changed = receipt.operation.changed;
  receipt.accepted = receipt.operation.accepted &&
                     (!receipt.changed || receipt.history.accepted);
  receipt.reasonCode = !receipt.operation.accepted
                           ? receipt.operation.reasonCode
                       : receipt.changed && !receipt.history.accepted
                           ? receipt.history.reasonCode
                           : "creative_editor_terrain_path_applied";
  if (receipt.accepted) {
    clearPathDraft(editor.terrain.path);
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
    CreativeTerrainPathState& path = editor.terrain.path;
    std::size_t selected = pointIndexById(path, path.selectedPointId);
    if (selected >= path.pointCount && path.pointCount > 0U) {
      selected = path.pointCount - 1U;
    }
    if (selected < path.pointCount) {
      path.points[selected].halfWidthCells =
          cr::creativeTerrainPathHalfWidthCells(
              editor.toolSettings.terrainPathWidth);
      path.points[selected].amplitudeCells =
          cr::creativeTerrainPathAmplitudeCells(
              editor.toolSettings.terrainPathAmplitude);
    }
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
  cr::CreativeTerrainPathSourceRecipe recipe =
      effectivePathRecipe(document, editor);
  if (recipe.points.empty()) {
    invalidatePathPreview(state.path);
    return false;
  }
  CreativeTerrainPathPreviewCache& cache = state.path.preview;
  if (previewKeyMatches(cache, document, recipe)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  const bool preserveSourceCache =
      cache.sourceCache.valid && cache.documentId == document.id() &&
      cache.documentRevision == document.revision();
  cr::CreativeTerrainPathSourceCache sourceCache =
      preserveSourceCache ? std::move(cache.sourceCache)
                          : cr::CreativeTerrainPathSourceCache{};
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.buildCount = nextBuildCount;
  cache.lockedPointCount = state.path.pointCount;
  cache.recipe = std::move(recipe);
  cache.sourceCache = std::move(sourceCache);
  if (cache.sourceCache.valid) {
    cache.dirtySegments = cr::diffCreativeTerrainPathSourceSegments(
        cache.sourceCache.recipe, cache.recipe);
  } else {
    cache.dirtySegments.changed = true;
    cache.dirtySegments.allSegments = true;
    cache.dirtySegments.segmentCount =
        cache.recipe.points.empty() ? 0U : cache.recipe.points.size() - 1U;
  }
  if (cache.recipe.points.size() < 2U) {
    return true;
  }
  cache.operationPreview = cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      pathRequest(document, editor, cache.recipe), &cache.sourceCache);
  if (!cache.operationPreview.receipt.accepted) {
    return true;
  }
  cache.dirtySegments = cache.sourceCache.dirtySegments;
  cache.generatedControlCount = cache.sourceCache.generatedControlCount;
  cache.rebuiltSegmentCount = cache.sourceCache.rebuiltSegmentCount;
  cache.reusedSegmentCount = cache.sourceCache.reusedSegmentCount;

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeComposedTerrainSurfacePlan(
          document.terrainField(), cache.operationPreview.heightField,
          cache.operationPreview.hardEdges);
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainRenderPlan(
          surface, cache.operationPreview.materialField, grid.origin,
          grid.cellSizeMeters);
  if (!render.accepted) {
    return true;
  }
  std::int64_t minimumX = std::numeric_limits<std::int64_t>::max();
  std::int64_t minimumZ = std::numeric_limits<std::int64_t>::max();
  std::int64_t maximumX = std::numeric_limits<std::int64_t>::min();
  std::int64_t maximumZ = std::numeric_limits<std::int64_t>::min();
  for (const cr::CreativeTerrainPathSourcePoint& point : cache.recipe.points) {
    const std::int64_t expansion =
        static_cast<std::int64_t>(point.halfWidthCells) +
        cache.recipe.falloffCells;
    minimumX = std::min(minimumX,
                        static_cast<std::int64_t>(point.coord.x) - expansion);
    minimumZ = std::min(minimumZ,
                        static_cast<std::int64_t>(point.coord.z) - expansion);
    maximumX = std::max(maximumX,
                        static_cast<std::int64_t>(point.coord.x) + expansion);
    maximumZ = std::max(maximumZ,
                        static_cast<std::int64_t>(point.coord.z) + expansion);
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : render.patches) {
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
  if (!preview.valid || preview.recipe.points.empty()) {
    return;
  }
  constexpr iggy3d::RenderLineColor locked{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor live{0.98F, 0.88F, 0.16F, 1.0F};
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  const bool complete = preview.recipe.points.size() >= 2U;
  const bool accepted =
      complete && preview.operationPreview.receipt.accepted &&
      preview.renderAccepted;
  const iggy3d::RenderLineColor routeColor =
      !complete ? live : accepted ? admitted : rejected;
  const cr::CreativeGridSettings grid = document.gridSettings();

  for (std::size_t index = 0U; index < preview.recipe.points.size(); ++index) {
    const cr::CreativeTerrainPathSourcePoint& point =
        preview.recipe.points[index];
    const bool isLive =
        index >= preview.lockedPointCount ||
        (editor.terrain.path.editingOperationId !=
             cr::kInvalidCreativeTerrainOperationId &&
         editor.terrain.path.selectedPointFollowsPointer &&
         point.id == editor.terrain.path.selectedPointId);
    appendCreativeEditorTerrainControlGuide(
        wireLines, grid,
        {point.coord, point.heightCells,
         static_cast<std::uint16_t>(point.halfWidthCells + 1U)},
        isLive ? live : locked, wireThickness * 1.5F);
    if (index > 0U) {
      appendPathLine(wireLines, grid, preview.recipe.points[index - 1U],
                     point, routeColor,
                     wireThickness * 1.4F);
    }
  }
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.7F);
  }
}

}  // namespace iggy3d_creative_app
