#include "EditorTerrainPaint.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] bool terrainPaintTarget(const CreativeEditorState& editor,
                                      cr::CreativeTerrainCoord2& target) {
  if (!editor.interaction.target.terrainHit) {
    return false;
  }
  target = editor.interaction.target.terrainCell;
  return true;
}

[[nodiscard]] cr::CreativeTerrainPaintPlan terrainPaintPlan(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainMaterial material) {
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainSurfacePlan(document.terrainField());
  cr::CreativeTerrainCoord2 center{};
  if (!surface.accepted || !terrainPaintTarget(editor, center)) {
    return {};
  }
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = surface.columns;
  request.materialField = &document.terrainMaterialField();
  request.center = center;
  request.material = material;
  request.radiusCells = cr::creativeTerrainPaintRadiusCells(
      editor.toolSettings.terrainPaintRadius);
  return cr::buildCreativeTerrainPaintPlan(request);
}

void setFeedback(CreativeEditorState& editor, bool accepted) {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

void applyTerrainPaint(cr::CreativeAppState& appState,
                       CreativeEditorState& editor,
                       cr::CreativeMaterialStrokeKind kind) {
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  const cr::CreativeTerrainMaterial material =
      kind == cr::CreativeMaterialStrokeKind::Remove
          ? cr::CreativeTerrainMaterial::Grass
          : editor.toolSettings.terrainPaintMaterial;
  state.lastPlan = terrainPaintPlan(appState.facade.document(), editor, material);
  if (!state.lastPlan.accepted) {
    setFeedback(editor, false);
    return;
  }
  if (state.lastPlan.status == cr::CreativeTerrainPaintPlanStatus::NoChange) {
    setFeedback(editor, true);
    return;
  }
  if (!state.transaction.active) {
    state.transaction = beginEditTransaction(
        appState.facade, "creative_terrain_paint_stroke");
  }
  if (!state.transaction.active) {
    setFeedback(editor, false);
    return;
  }
  state.lastMutation =
      appState.facade.applyTerrainMaterialEdits(state.lastPlan.items());
  if (state.lastMutation.accepted && state.lastMutation.changed) {
    ++state.acceptedMutationCount;
  }
  setFeedback(editor, state.lastMutation.accepted);
}

void appendLine(std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
                iggy3d::Vec3 start,
                iggy3d::Vec3 end,
                iggy3d::RenderLineColor color,
                float thickness) {
  iggy3d::RenderCreativeWireframeDebugLine line;
  line.start = start;
  line.end = end;
  line.color = color;
  line.thickness = thickness;
  lines.push_back(line);
}

}  // namespace

void finalizeCreativeEditorTerrainPaintStroke(cr::CreativeAppState& appState,
                                              CreativeEditorState& editor,
                                              std::string_view reasonCode) {
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  if (!state.repeat.active && !state.transaction.active) {
    state = {};
    return;
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      std::move(state.transaction);
  const bool changed = state.acceptedMutationCount > 0U;
  state = {};
  if (transaction.active) {
    static_cast<void>(completeEditTransaction(
        appState.history, std::move(transaction), appState.facade, changed,
        reasonCode));
  }
}

void processCreativeEditorTerrainPaintFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds) {
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  const cr::CreativeMaterialRepeatResult repeat =
      cr::stepCreativeMaterialRepeat(
          state.repeat,
          cr::makeCreativeWorldStrokeRepeatRequest(
              actions, monotonicTimeNanoseconds));
  state.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeEditorTerrainPaintStroke(
        appState, editor, "creative_terrain_paint_stroke_released");
    return;
  }
  if (repeat.mutationDue) {
    applyTerrainPaint(appState, editor, repeat.dueKind);
  }
}

bool sampleCreativeEditorTerrainMaterial(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  cr::CreativeTerrainCoord2 target{};
  if (!terrainPaintTarget(editor, target)) {
    setFeedback(editor, false);
    return false;
  }
  const cr::CreativeTerrainMaterial material =
      document.terrainMaterialField().materialAt(target);
  const bool changed = editor.toolSettings.terrainPaintMaterial != material;
  editor.toolSettings.terrainPaintMaterial = material;
  setFeedback(editor, true);
  return changed;
}

void appendCreativeEditorTerrainPaintOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  cr::CreativeTerrainCoord2 center{};
  if (!terrainPaintTarget(editor, center)) {
    return;
  }
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainSurfacePlan(document.terrainField());
  if (!surface.accepted) {
    return;
  }
  const std::int64_t radius = cr::creativeTerrainPaintRadiusCells(
      editor.toolSettings.terrainPaintRadius);
  const std::int64_t radiusSquared = radius * radius;
  const cr::CreativeGridSettings grid = document.gridSettings();
  constexpr std::array colors{
      iggy3d::RenderLineColor{0.20F, 0.95F, 0.25F, 1.0F},
      iggy3d::RenderLineColor{0.52F, 0.30F, 0.12F, 1.0F},
      iggy3d::RenderLineColor{0.68F, 0.70F, 0.72F, 1.0F},
      iggy3d::RenderLineColor{0.98F, 0.84F, 0.40F, 1.0F},
  };
  const std::size_t colorIndex =
      static_cast<std::size_t>(editor.toolSettings.terrainPaintMaterial);
  const iggy3d::RenderLineColor color =
      colorIndex < colors.size() ? colors[colorIndex] : colors.front();
  for (const cr::CreativeTerrainColumn& column : surface.columns) {
    const std::int64_t dx =
        static_cast<std::int64_t>(column.coord.x) - center.x;
    const std::int64_t dz =
        static_cast<std::int64_t>(column.coord.z) - center.z;
    if (dx * dx + dz * dz > radiusSquared) {
      continue;
    }
    const float minX = static_cast<float>(
        grid.origin.x + column.coord.x * grid.cellSizeMeters);
    const float minZ = static_cast<float>(
        grid.origin.z + column.coord.z * grid.cellSizeMeters);
    const float maxX = minX + static_cast<float>(grid.cellSizeMeters);
    const float maxZ = minZ + static_cast<float>(grid.cellSizeMeters);
    const float y = static_cast<float>(
        grid.origin.y + column.heightCells * grid.cellSizeMeters + 0.03);
    appendLine(lines, {minX, y, minZ}, {maxX, y, minZ}, color, thickness);
    appendLine(lines, {maxX, y, minZ}, {maxX, y, maxZ}, color, thickness);
    appendLine(lines, {maxX, y, maxZ}, {minX, y, maxZ}, color, thickness);
    appendLine(lines, {minX, y, maxZ}, {minX, y, minZ}, color, thickness);
  }
}

}  // namespace iggy3d_creative_app
