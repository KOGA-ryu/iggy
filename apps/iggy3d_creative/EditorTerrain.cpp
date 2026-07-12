#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "runtime/movement/MovementPolicy.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] cr::CreativeTerrainCoord2 aimedTerrainCoord(
    const CreativeEditorState& editor) noexcept {
  if (editor.terrain.selectionValid) {
    return editor.terrain.selectedCoord;
  }
  if (editor.terrain.hoverValid) {
    return editor.terrain.hoverCoord;
  }
  if (editor.interaction.target.terrainHit) {
    return editor.interaction.target.terrainCell;
  }
  const cr::CreativeGridCoord3 cell =
      editor.interaction.target.grid.targetCell;
  return {cell.x, cell.z};
}

[[nodiscard]] bool terrainPointerCoord(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2& coord) noexcept {
  if (editor.terrain.hoverValid) {
    coord = editor.terrain.hoverCoord;
    return true;
  }
  if (editor.interaction.target.terrainHit) {
    coord = editor.interaction.target.terrainCell;
    return true;
  }
  if (!editor.interaction.target.grid.valid) {
    return false;
  }
  const cr::CreativeGridCoord3 cell =
      editor.interaction.target.grid.targetCell;
  coord = {cell.x, cell.z};
  return true;
}

[[nodiscard]] cr::CreativeTerrainGradePlan terrainGradePlan(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) noexcept {
  const CreativeTerrainGradeState& grade = editor.terrain.grade;
  if (!grade.anchorValid) {
    return {};
  }
  return cr::buildCreativeTerrainGradePlan(
      {grade.anchorCoord, target, grade.anchorHeightCells,
       grade.targetHeightCells, grade.radiusCells});
}

[[nodiscard]] bool terrainEditCoord(
    const CreativeEditorState& editor,
    CreativeEditorTerrainEditKind kind,
    cr::CreativeTerrainCoord2& coord) noexcept {
  if (kind != CreativeEditorTerrainEditKind::Sample &&
      editor.terrain.selectionValid) {
    coord = editor.terrain.selectedCoord;
    return true;
  }
  if (editor.terrain.hoverValid) {
    coord = editor.terrain.hoverCoord;
    return true;
  }
  if (editor.interaction.target.terrainHit) {
    coord = editor.interaction.target.terrainCell;
    return true;
  }
  if (!editor.interaction.target.grid.valid) {
    return false;
  }
  const cr::CreativeGridCoord3 cell =
      editor.interaction.target.grid.targetCell;
  coord = {cell.x, cell.z};
  return true;
}

[[nodiscard]] cr::CreativeBounds terrainRodBounds(
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    double widthCells) noexcept {
  const double halfWidth = widthCells * grid.cellSizeMeters * 0.5;
  const double centerX = grid.origin.x +
                         (static_cast<double>(control.coord.x) + 0.5) *
                             grid.cellSizeMeters;
  const double centerZ = grid.origin.z +
                         (static_cast<double>(control.coord.z) + 0.5) *
                             grid.cellSizeMeters;
  return {{centerX - halfWidth, grid.origin.y, centerZ - halfWidth},
          {centerX + halfWidth,
           grid.origin.y + control.heightCells * grid.cellSizeMeters,
           centerZ + halfWidth}};
}

void appendBounds(std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
                  cr::CreativeBounds bounds,
                  iggy3d::RenderLineColor color,
                  float thickness) {
  const cr::CreativeCoreVec3Conversion minimum =
      cr::creativeVec3ToCoreChecked(bounds.min);
  const cr::CreativeCoreVec3Conversion maximum =
      cr::creativeVec3ToCoreChecked(bounds.max);
  if (minimum.converted && maximum.converted) {
    appendStandaloneWireframeBoxEdges(lines, minimum.value, maximum.value,
                                      color, thickness);
  }
}

void appendTerrainFootprintOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness) {
  const std::int32_t radius = control.radiusCells;
  const auto insideDisk = [radius](std::int32_t dx, std::int32_t dz) {
    const std::int64_t x = dx;
    const std::int64_t z = dz;
    return x * x + z * z <=
           static_cast<std::int64_t>(radius) * radius;
  };
  const auto appendEdge = [&](cr::CreativeVec3 start, cr::CreativeVec3 end) {
    const cr::CreativeCoreVec3Conversion coreStart =
        cr::creativeVec3ToCoreChecked(start);
    const cr::CreativeCoreVec3Conversion coreEnd =
        cr::creativeVec3ToCoreChecked(end);
    if (!coreStart.converted || !coreEnd.converted) {
      return;
    }
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = coreStart.value;
    line.end = coreEnd.value;
    line.color = color;
    line.thickness = thickness;
    lines.push_back(line);
  };

  const double y = grid.origin.y + grid.cellSizeMeters * 0.12;
  for (std::int32_t dz = -radius; dz <= radius; ++dz) {
    for (std::int32_t dx = -radius; dx <= radius; ++dx) {
      if (!insideDisk(dx, dz)) {
        continue;
      }
      const double minimumX =
          grid.origin.x + (control.coord.x + dx) * grid.cellSizeMeters;
      const double maximumX = minimumX + grid.cellSizeMeters;
      const double minimumZ =
          grid.origin.z + (control.coord.z + dz) * grid.cellSizeMeters;
      const double maximumZ = minimumZ + grid.cellSizeMeters;
      if (!insideDisk(dx - 1, dz)) {
        appendEdge({minimumX, y, minimumZ}, {minimumX, y, maximumZ});
      }
      if (!insideDisk(dx + 1, dz)) {
        appendEdge({maximumX, y, minimumZ}, {maximumX, y, maximumZ});
      }
      if (!insideDisk(dx, dz - 1)) {
        appendEdge({minimumX, y, minimumZ}, {maximumX, y, minimumZ});
      }
      if (!insideDisk(dx, dz + 1)) {
        appendEdge({minimumX, y, maximumZ}, {maximumX, y, maximumZ});
      }
    }
  }
}

[[nodiscard]] cr::CreativeVec3 terrainRodTopCenter(
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control) noexcept {
  return {grid.origin.x +
              (static_cast<double>(control.coord.x) + 0.5) *
                  grid.cellSizeMeters,
          grid.origin.y + control.heightCells * grid.cellSizeMeters,
          grid.origin.z +
              (static_cast<double>(control.coord.z) + 0.5) *
                  grid.cellSizeMeters};
}

void appendTerrainLine(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeVec3 start,
    cr::CreativeVec3 end,
    iggy3d::RenderLineColor color,
    float thickness) {
  const cr::CreativeCoreVec3Conversion coreStart =
      cr::creativeVec3ToCoreChecked(start);
  const cr::CreativeCoreVec3Conversion coreEnd =
      cr::creativeVec3ToCoreChecked(end);
  if (!coreStart.converted || !coreEnd.converted) {
    return;
  }
  iggy3d::RenderCreativeWireframeDebugLine line;
  line.start = coreStart.value;
  line.end = coreEnd.value;
  line.color = color;
  line.thickness = thickness;
  lines.push_back(line);
}

void appendTerrainGradePreview(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float thickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines) {
  constexpr iggy3d::RenderLineColor anchorColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor gradeColor{0.30F, 1.0F, 0.38F, 1.0F};
  constexpr iggy3d::RenderLineColor invalidColor{1.0F, 0.20F, 0.16F, 1.0F};
  const cr::CreativeGridSettings grid = document.gridSettings();
  cr::CreativeTerrainCoord2 target{};
  if (!editor.terrain.grade.anchorValid) {
    if (!terrainPointerCoord(editor, target)) {
      return;
    }
    const cr::CreativeTerrainControlPoint* control =
        document.terrainField().controlAt(target);
    if (control != nullptr) {
      appendBounds(lines, terrainRodBounds(grid, *control, 0.24), anchorColor,
                   thickness * 1.4F);
    }
    return;
  }

  const CreativeTerrainGradeState& grade = editor.terrain.grade;
  const cr::CreativeTerrainControlPoint anchor{
      grade.anchorCoord, grade.anchorHeightCells, grade.radiusCells};
  if (!terrainPointerCoord(editor, target)) {
    appendBounds(lines, terrainRodBounds(grid, anchor, 0.24), anchorColor,
                 thickness * 1.4F);
    return;
  }
  const cr::CreativeTerrainGradePlan plan = terrainGradePlan(editor, target);
  if (!plan.accepted) {
    const cr::CreativeTerrainControlPoint rejected{
        target, grade.targetHeightCells, grade.radiusCells};
    appendBounds(lines, terrainRodBounds(grid, anchor, 0.24), invalidColor,
                 thickness * 1.4F);
    appendBounds(lines, terrainRodBounds(grid, rejected, 0.24), invalidColor,
                 thickness * 1.4F);
    appendTerrainLine(lines, terrainRodTopCenter(grid, anchor),
                      terrainRodTopCenter(grid, rejected), invalidColor,
                      thickness * 1.2F);
    return;
  }

  cr::CreativeVec3 previousTop{};
  bool hasPrevious = false;
  for (const cr::CreativeTerrainControlEdit& edit : plan.items()) {
    appendBounds(lines, terrainRodBounds(grid, edit.control, 0.20), gradeColor,
                 thickness);
    const cr::CreativeVec3 top = terrainRodTopCenter(grid, edit.control);
    if (hasPrevious) {
      appendTerrainLine(lines, previousTop, top, gradeColor, thickness * 1.2F);
    }
    previousTop = top;
    hasPrevious = true;
  }
  if (!plan.items().empty()) {
    appendTerrainFootprintOutline(lines, grid, plan.items().front().control,
                                  anchorColor, thickness);
    if (plan.items().size() > 1U) {
      appendTerrainFootprintOutline(lines, grid, plan.items().back().control,
                                    gradeColor, thickness);
    }
  }
}

void setFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

void clearTerrainGradeAnchor(CreativeTerrainGradeState& grade) noexcept {
  grade.anchorValid = false;
}

[[nodiscard]] bool terrainStrokeVisited(
    const CreativeTerrainStrokeState& stroke,
    cr::CreativeTerrainCoord2 coord) noexcept {
  return std::find(stroke.visited.begin(),
                   stroke.visited.begin() + stroke.visitedCount,
                   coord) != stroke.visited.begin() + stroke.visitedCount;
}

[[nodiscard]] bool rememberTerrainStrokeTarget(
    CreativeTerrainStrokeState& stroke,
    cr::CreativeTerrainCoord2 coord) noexcept {
  if (stroke.visitedCount >= stroke.visited.size()) {
    stroke.capacityReached = true;
    return false;
  }
  stroke.visited[stroke.visitedCount++] = coord;
  return true;
}

[[nodiscard]] bool ensureTerrainStrokeTransaction(
    cr::CreativeAppState& appState,
    CreativeTerrainStrokeState& stroke,
    cr::CreativeMaterialStrokeKind kind,
    cr::CreativeTerrainRodStampMode stampMode) {
  if (stroke.transaction.active) {
    return true;
  }
  const bool removing = kind == cr::CreativeMaterialStrokeKind::Remove;
  const std::string_view source =
      stampMode == cr::CreativeTerrainRodStampMode::Seed
          ? removing ? "creative_terrain_clear_stroke"
                     : "creative_terrain_seed_stroke"
          : removing ? "creative_terrain_erase_stroke"
                     : "creative_terrain_paint_stroke";
  stroke.transaction = beginEditTransaction(appState.facade, source);
  return stroke.transaction.active;
}

void rejectTerrainStroke(CreativeEditorState& editor) noexcept {
  setFeedback(editor, false);
}

[[nodiscard]] cr::CreativeTerrainSeedPlan terrainSeedPlan(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 center,
    cr::CreativeMaterialStrokeKind kind) noexcept {
  return cr::buildCreativeTerrainSeedPlan(
      {&document.terrainField(), center,
       kind == cr::CreativeMaterialStrokeKind::Remove
           ? cr::CreativeTerrainSeedOperation::Clear
           : cr::CreativeTerrainSeedOperation::SeedMissing,
       cr::creativeTerrainSeedRadiusCells(editor.toolSettings.terrainSeedRadius),
       cr::creativeTerrainSeedSpacingCells(
           editor.toolSettings.terrainSeedSpacing),
       editor.terrain.heightCells, editor.terrain.radiusCells});
}

void applyTerrainSeedStrokeMutation(cr::CreativeAppState& appState,
                                    CreativeEditorState& editor,
                                    cr::CreativeMaterialStrokeKind kind) {
  CreativeEditorTerrainState& terrain = editor.terrain;
  CreativeTerrainStrokeState& stroke = terrain.stroke;
  cr::CreativeTerrainCoord2 center{};
  if (!terrainPointerCoord(editor, center)) {
    rejectTerrainStroke(editor);
    return;
  }
  if (terrainStrokeVisited(stroke, center)) {
    return;
  }
  const cr::CreativeTerrainSeedPlan plan =
      terrainSeedPlan(appState.facade.document(), editor, center, kind);
  if (!plan.accepted) {
    if (plan.status == cr::CreativeTerrainSeedPlanStatus::CapacityExceeded) {
      stroke.capacityReached = true;
    }
    static_cast<void>(rememberTerrainStrokeTarget(stroke, center));
    rejectTerrainStroke(editor);
    return;
  }
  if (plan.items().empty()) {
    static_cast<void>(rememberTerrainStrokeTarget(stroke, center));
    setFeedback(editor, true);
    return;
  }
  if (!ensureTerrainStrokeTransaction(
          appState, stroke, kind, cr::CreativeTerrainRodStampMode::Seed) ||
      !rememberTerrainStrokeTarget(stroke, center)) {
    rejectTerrainStroke(editor);
    return;
  }

  terrain.lastMutation =
      appState.facade.applyTerrainControlEdits(plan.items());
  if (terrain.lastMutation.status ==
      cr::CreativeTerrainMutationStatus::CapacityExceeded) {
    stroke.capacityReached = true;
  }
  if (terrain.lastMutation.accepted && terrain.lastMutation.changed) {
    ++stroke.acceptedMutationCount;
    terrain.hoverValid = false;
    terrain.selectionValid = false;
  }
  setFeedback(editor, terrain.lastMutation.accepted);
}

void applyTerrainStrokeMutation(cr::CreativeAppState& appState,
                                CreativeEditorState& editor,
                                cr::CreativeMaterialStrokeKind kind) {
  CreativeEditorTerrainState& terrain = editor.terrain;
  CreativeTerrainStrokeState& stroke = terrain.stroke;
  if (stroke.capacityReached) {
    rejectTerrainStroke(editor);
    return;
  }
  if (editor.toolSettings.terrainRodStampMode ==
      cr::CreativeTerrainRodStampMode::Seed) {
    applyTerrainSeedStrokeMutation(appState, editor, kind);
    return;
  }

  const CreativeEditorTerrainEditKind editKind =
      kind == cr::CreativeMaterialStrokeKind::Remove
          ? CreativeEditorTerrainEditKind::Remove
          : CreativeEditorTerrainEditKind::Upsert;
  cr::CreativeTerrainCoord2 coord{};
  if (!terrainEditCoord(editor, editKind, coord)) {
    rejectTerrainStroke(editor);
    return;
  }
  if (terrainStrokeVisited(stroke, coord)) {
    return;
  }
  if (stroke.visitedCount >= stroke.visited.size()) {
    stroke.capacityReached = true;
    rejectTerrainStroke(editor);
    return;
  }

  const cr::CreativeTerrainControlPoint* existing =
      appState.facade.document().terrainField().controlAt(coord);
  if (editKind == CreativeEditorTerrainEditKind::Remove && existing == nullptr) {
    static_cast<void>(rememberTerrainStrokeTarget(stroke, coord));
    rejectTerrainStroke(editor);
    return;
  }
  const cr::CreativeTerrainControlPoint requested{
      coord, terrain.heightCells, terrain.radiusCells};
  if (editKind == CreativeEditorTerrainEditKind::Upsert &&
      existing != nullptr && *existing == requested) {
    static_cast<void>(rememberTerrainStrokeTarget(stroke, coord));
    terrain.selectionValid = false;
    setFeedback(editor, true);
    return;
  }
  if (editKind == CreativeEditorTerrainEditKind::Upsert && existing == nullptr &&
      appState.facade.document().terrainField().controlCount() >=
          cr::kCreativeTerrainControlCapacity) {
    stroke.capacityReached = true;
    rejectTerrainStroke(editor);
    return;
  }
  if (!ensureTerrainStrokeTransaction(
          appState, stroke, kind, cr::CreativeTerrainRodStampMode::Single) ||
      !rememberTerrainStrokeTarget(stroke, coord)) {
    rejectTerrainStroke(editor);
    return;
  }

  const cr::CreativeTerrainControlEdit edit{
      editKind == CreativeEditorTerrainEditKind::Remove
          ? cr::CreativeTerrainEditKind::Remove
          : cr::CreativeTerrainEditKind::Upsert,
      requested};
  terrain.lastMutation =
      appState.facade.applyTerrainControlEdits(std::span{&edit, 1U});
  if (terrain.lastMutation.status ==
      cr::CreativeTerrainMutationStatus::CapacityExceeded) {
    stroke.capacityReached = true;
  }
  const bool changed = terrain.lastMutation.accepted &&
                       terrain.lastMutation.changed;
  if (changed) {
    ++stroke.acceptedMutationCount;
    if (editKind == CreativeEditorTerrainEditKind::Remove) {
      terrain.hoverValid = false;
    } else {
      terrain.selectionValid = false;
    }
  }
  setFeedback(editor, terrain.lastMutation.accepted);
}

}  // namespace

bool resolveCreativeEditorTerrainPointerCoord(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2& target) noexcept {
  return terrainPointerCoord(editor, target);
}

void appendCreativeEditorTerrainFootprintOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness) {
  appendTerrainFootprintOutline(lines, grid, control, color, thickness);
}

void appendCreativeEditorTerrainControlGuide(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainControlPoint control,
    iggy3d::RenderLineColor color,
    float thickness) {
  appendBounds(lines, terrainRodBounds(grid, control, 0.18), color, thickness);
}

void appendCreativeEditorTerrainPatchSlopeTriangles(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const cr::CreativeTerrainSurfacePatch& patch,
    float thickness) {
  constexpr iggy3d::RenderLineColor walkable{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor careful{1.0F, 0.82F, 0.16F, 1.0F};
  constexpr iggy3d::RenderLineColor blocked{1.0F, 0.20F, 0.18F, 1.0F};
  const cr::CreativeCoreVec3Conversion center =
      cr::creativeVec3ToCoreChecked(patch.center);
  if (!center.converted) {
    return;
  }
  for (std::size_t index = 0U; index < patch.corners.size(); ++index) {
    const cr::CreativeCoreVec3Conversion first =
        cr::creativeVec3ToCoreChecked(patch.corners[index]);
    const cr::CreativeCoreVec3Conversion second = cr::creativeVec3ToCoreChecked(
        patch.corners[(index + 1U) % patch.corners.size()]);
    if (!first.converted || !second.converted) {
      continue;
    }
    iggy3d::RenderLineColor color = blocked;
    iggy3d::Vec3 normal;
    if (iggy3d::tryNormalize(
            iggy3d::cross(first.value - center.value,
                          second.value - center.value),
            normal)) {
      if (normal.y < 0.0F) {
        normal = normal * -1.0F;
      }
      const iggy3d::SlopeSample slope = iggy3d::sampleSlope(normal);
      if (slope.valid && slope.walkable) {
        color = slope.carefulFooting ? careful : walkable;
      }
    }
    const auto append = [&lines, color, thickness](iggy3d::Vec3 start,
                                                    iggy3d::Vec3 end) {
      iggy3d::RenderCreativeWireframeDebugLine line;
      line.start = start;
      line.end = end;
      line.color = color;
      line.thickness = thickness;
      lines.push_back(line);
    };
    append(center.value, first.value);
    append(first.value, second.value);
    append(second.value, center.value);
  }
}

bool resolveCreativeEditorTerrainGradeTarget(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2& target) noexcept {
  return terrainPointerCoord(editor, target);
}

cr::CreativeTerrainGradePlan planCreativeEditorTerrainGrade(
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) noexcept {
  return terrainGradePlan(editor, target);
}

CreativeEditorTerrainEditReceipt applyCreativeEditorTerrainEditWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorTerrainEditKind kind,
    std::string_view source) {
  CreativeEditorTerrainEditReceipt receipt;
  receipt.requested = true;
  receipt.kind = kind;
  if (!terrainEditCoord(editor, kind, receipt.coord)) {
    receipt.reasonCode = "creative_editor_terrain_target_invalid";
    setFeedback(editor, false);
    return receipt;
  }

  const cr::CreativeTerrainControlPoint* existing =
      appState.facade.document().terrainField().controlAt(receipt.coord);
  if (kind == CreativeEditorTerrainEditKind::Sample) {
    if (existing == nullptr) {
      receipt.reasonCode = "creative_editor_terrain_sample_missing";
      setFeedback(editor, false);
      return receipt;
    }
    receipt.accepted = true;
    receipt.changed = editor.terrain.heightCells != existing->heightCells ||
                      editor.terrain.radiusCells != existing->radiusCells ||
                      !editor.terrain.selectionValid ||
                      editor.terrain.selectedCoord != existing->coord;
    editor.terrain.heightCells = existing->heightCells;
    editor.terrain.radiusCells = existing->radiusCells;
    editor.terrain.selectionValid = true;
    editor.terrain.selectedCoord = existing->coord;
    editor.terrain.selectedOriginal = *existing;
    receipt.reasonCode = "creative_editor_terrain_selected";
    setFeedback(editor, true);
    return receipt;
  }

  if (kind == CreativeEditorTerrainEditKind::Remove &&
      editor.terrain.selectionValid) {
    receipt.accepted = true;
    receipt.changed =
        editor.terrain.heightCells != editor.terrain.selectedOriginal.heightCells ||
        editor.terrain.radiusCells != editor.terrain.selectedOriginal.radiusCells;
    editor.terrain.heightCells = editor.terrain.selectedOriginal.heightCells;
    editor.terrain.radiusCells = editor.terrain.selectedOriginal.radiusCells;
    editor.terrain.selectionValid = false;
    receipt.reasonCode = "creative_editor_terrain_edit_cancelled";
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return receipt;
  }

  cr::CreativeTerrainControlEdit edit;
  edit.kind = kind == CreativeEditorTerrainEditKind::Remove
                  ? cr::CreativeTerrainEditKind::Remove
                  : cr::CreativeTerrainEditKind::Upsert;
  edit.control = {receipt.coord, editor.terrain.heightCells,
                  editor.terrain.radiusCells};
  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.mutation =
      appState.facade.applyTerrainControlEdits(std::span{&edit, 1U});
  editor.terrain.lastMutation = receipt.mutation;
  receipt.accepted =
      receipt.mutation.accepted &&
      (kind == CreativeEditorTerrainEditKind::Upsert ||
       receipt.mutation.changed);
  receipt.changed = receipt.mutation.changed;
  receipt.reasonCode = receipt.mutation.reasonCode;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade,
      receipt.mutation.accepted && receipt.mutation.changed,
      receipt.mutation.reasonCode));
  if (kind == CreativeEditorTerrainEditKind::Upsert &&
      editor.terrain.selectionValid && receipt.mutation.accepted) {
    editor.terrain.selectionValid = false;
  }
  if (kind == CreativeEditorTerrainEditKind::Remove &&
      receipt.mutation.changed) {
    editor.terrain.hoverValid = false;
  }
  setFeedback(editor, receipt.accepted);
  return receipt;
}

bool processCreativeEditorTerrainQuickEdit(
    CreativeEditorTerrainState& state,
    cr::CreativeInputActionId action) noexcept {
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditPrevious ? 1 : -1;
      const std::uint16_t before = state.heightCells;
      state.heightCells = static_cast<std::uint16_t>(std::clamp(
          static_cast<int>(state.heightCells) + direction,
          static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
          static_cast<int>(cr::kCreativeTerrainMaximumHeightCells)));
      return state.heightCells != before;
    }
    case cr::CreativeInputActionId::QuickEditDecrease:
    case cr::CreativeInputActionId::QuickEditIncrease: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditDecrease ? -1 : 1;
      const std::uint16_t before = state.radiusCells;
      state.radiusCells = static_cast<std::uint16_t>(std::clamp(
          static_cast<int>(state.radiusCells) + direction,
          static_cast<int>(cr::kCreativeTerrainMinimumRadiusCells),
          static_cast<int>(cr::kCreativeTerrainMaximumRadiusCells)));
      return state.radiusCells != before;
    }
    default:
      return false;
  }
}

std::string creativeEditorTerrainQuickEditLabel(
    const CreativeEditorTerrainState& state) {
  return "HEIGHT " + std::to_string(state.heightCells) + " | RADIUS " +
         std::to_string(state.radiusCells);
}

void clearCreativeEditorTerrainInteraction(
    CreativeEditorTerrainState& state,
    std::uint64_t documentId) noexcept {
  if (state.selectionValid) {
    state.heightCells = state.selectedOriginal.heightCells;
    state.radiusCells = state.selectedOriginal.radiusCells;
  }
  state.documentId = documentId;
  state.hoverValid = false;
  state.selectionValid = false;
  clearTerrainGradeAnchor(state.grade);
  state.sculpt.preview.valid = false;
  state.sculpt.preview.renderAccepted = false;
  state.sculpt.preview.patches.clear();
  state.profile.baseLocked = false;
  state.profile.resolvedBaseHeightCells = state.heightCells;
  state.profile.preview.valid = false;
  state.profile.preview.renderAccepted = false;
  state.profile.preview.patches.clear();
  state.path.pointCount = 0U;
  state.path.preview.valid = false;
  state.path.preview.renderAccepted = false;
  state.path.preview.patches.clear();
}

void updateCreativeEditorTerrainAim(
    CreativeEditorTerrainState& state,
    const cr::CreativeDocument& document,
    WorldRay ray,
    float occluderDistanceMeters) noexcept {
  if (state.documentId != document.id()) {
    clearCreativeEditorTerrainInteraction(state, document.id());
  }
  state.hoverValid = false;
  if (state.selectionValid &&
      document.terrainField().controlAt(state.selectedCoord) == nullptr) {
    state.heightCells = state.selectedOriginal.heightCells;
    state.radiusCells = state.selectedOriginal.radiusCells;
    state.selectionValid = false;
  }
  if (!ray.valid) {
    return;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const float tolerance = static_cast<float>(grid.cellSizeMeters * 0.6);
  const float maximumDistance =
      std::isfinite(occluderDistanceMeters) && occluderDistanceMeters >= 0.0F
          ? occluderDistanceMeters + tolerance
          : std::numeric_limits<float>::max();
  float nearestDistance = maximumDistance;
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    const cr::CreativeBounds bounds = terrainRodBounds(grid, control, 0.5);
    const cr::CreativeCoreVec3Conversion minimum =
        cr::creativeVec3ToCoreChecked(bounds.min);
    const cr::CreativeCoreVec3Conversion maximum =
        cr::creativeVec3ToCoreChecked(bounds.max);
    if (!minimum.converted || !maximum.converted) {
      continue;
    }
    float entryDistance = 0.0F;
    if (!rayEntryDistanceForAabb(
            ray, VisualBounds{minimum.value, maximum.value}, entryDistance) ||
        entryDistance > nearestDistance ||
        (state.hoverValid && entryDistance == nearestDistance)) {
      continue;
    }
    nearestDistance = entryDistance;
    state.hoverValid = true;
    state.hoverCoord = control.coord;
  }
}

void finalizeCreativeTerrainStroke(cr::CreativeAppState& appState,
                                   CreativeEditorState& editor,
                                   std::string_view reasonCode) {
  CreativeTerrainStrokeState& stroke = editor.terrain.stroke;
  if (!stroke.repeat.active && !stroke.transaction.active) {
    return;
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      std::move(stroke.transaction);
  const bool changed = stroke.acceptedMutationCount > 0U;
  stroke = {};
  if (transaction.active) {
    static_cast<void>(completeEditTransaction(
        appState.history, std::move(transaction), appState.facade, changed,
        reasonCode));
  }
}

void processCreativeTerrainStrokeFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::TerrainControl) {
    finalizeCreativeTerrainStroke(appState, editor,
                                  "creative_terrain_stroke_non_terrain_tool");
    return;
  }

  CreativeTerrainStrokeState& stroke = editor.terrain.stroke;
  const cr::CreativeMaterialRepeatRequest repeatRequest =
      cr::makeCreativeWorldStrokeRepeatRequest(actions,
                                               monotonicTimeNanoseconds);

  const cr::CreativeMaterialRepeatResult repeat =
      cr::stepCreativeMaterialRepeat(stroke.repeat, repeatRequest);
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeTerrainStroke(appState, editor,
                                  "creative_terrain_stroke_released");
    return;
  }
  if (repeat.began) {
    stroke.cancelOnly =
        repeat.dueKind == cr::CreativeMaterialStrokeKind::Remove &&
        editor.toolSettings.terrainRodStampMode ==
            cr::CreativeTerrainRodStampMode::Single &&
        editor.terrain.selectionValid;
    if (stroke.cancelOnly) {
      static_cast<void>(applyCreativeEditorTerrainEditWithHistory(
          appState, editor, CreativeEditorTerrainEditKind::Remove,
          "creative_terrain_stroke_cancel_draft"));
      return;
    }
  }
  if (!repeat.mutationDue || stroke.cancelOnly) {
    return;
  }
  applyTerrainStrokeMutation(appState, editor, repeat.dueKind);
}

void appendCreativeEditorTerrainOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines,
    bool captureMode) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  const bool terrainControl =
      held.kind == cr::CreativeHeldItemKind::TerrainControl;
  const bool terrainGrade = held.kind == cr::CreativeHeldItemKind::TerrainGrade;
  const bool terrainSculpt =
      held.kind == cr::CreativeHeldItemKind::TerrainSculpt;
  const bool terrainProfile =
      held.kind == cr::CreativeHeldItemKind::TerrainProfile;
  const bool terrainPath = held.kind == cr::CreativeHeldItemKind::TerrainPath;
  if (captureMode ||
      (!terrainControl && !terrainGrade && !terrainSculpt && !terrainProfile &&
       !terrainPath) ||
      editor.catalog.model.open || editor.catalog.toolWheel.open ||
      editor.toolOptions.open || editor.controls.open || editor.transform.active ||
      editor.transform.controlsOpen) {
    return;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const float thickness = std::max(0.02F, wireThickness * 0.75F);
  constexpr iggy3d::RenderLineColor existingColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor influenceColor{0.16F, 0.45F, 0.52F, 0.9F};
  constexpr iggy3d::RenderLineColor hoverColor{0.30F, 1.0F, 0.38F, 1.0F};
  constexpr iggy3d::RenderLineColor selectedColor{1.0F, 0.42F, 0.82F, 1.0F};
  constexpr iggy3d::RenderLineColor previewColor{0.98F, 0.88F, 0.16F, 1.0F};
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    const bool selected = editor.terrain.selectionValid &&
                          editor.terrain.selectedCoord == control.coord;
    const bool hovered = editor.terrain.hoverValid &&
                         editor.terrain.hoverCoord == control.coord;
    appendBounds(wireLines, terrainRodBounds(grid, control, 0.18),
                 selected ? selectedColor : hovered ? hoverColor : existingColor,
                 (selected || hovered) ? thickness * 1.5F : thickness);
    cr::CreativeBounds influence = terrainRodBounds(
        grid, control, static_cast<double>(control.radiusCells * 2U + 1U));
    influence.max.y = influence.min.y + grid.cellSizeMeters * 0.08;
    appendBounds(wireLines, influence, influenceColor, thickness * 0.65F);
  }

  if (terrainSculpt) {
    appendCreativeEditorTerrainSculptOverlay(document, editor, thickness,
                                             wireLines);
    return;
  }

  if (terrainProfile) {
    appendCreativeEditorTerrainProfileOverlay(document, editor, thickness,
                                              wireLines);
    return;
  }

  if (terrainPath) {
    appendCreativeEditorTerrainPathOverlay(document, editor, thickness,
                                           wireLines);
    return;
  }

  if (terrainGrade) {
    appendTerrainGradePreview(document, editor, thickness, wireLines);
    return;
  }

  if (terrainControl &&
      editor.toolSettings.terrainRodStampMode ==
          cr::CreativeTerrainRodStampMode::Seed) {
    cr::CreativeTerrainCoord2 center{};
    if (!terrainPointerCoord(editor, center)) {
      return;
    }
    const cr::CreativeMaterialStrokeKind kind =
        editor.terrain.stroke.repeat.active
            ? editor.terrain.stroke.repeat.kind
            : cr::CreativeMaterialStrokeKind::Place;
    const cr::CreativeTerrainSeedPlan plan =
        terrainSeedPlan(document, editor, center, kind);
    constexpr iggy3d::RenderLineColor seedColor{0.30F, 1.0F, 0.38F, 1.0F};
    constexpr iggy3d::RenderLineColor clearColor{1.0F, 0.20F, 0.18F, 1.0F};
    const iggy3d::RenderLineColor color =
        !plan.accepted || kind == cr::CreativeMaterialStrokeKind::Remove
            ? clearColor
            : seedColor;
    appendTerrainFootprintOutline(
        wireLines, grid,
        {center, editor.terrain.heightCells,
         cr::creativeTerrainSeedRadiusCells(
             editor.toolSettings.terrainSeedRadius)},
        color, thickness * 1.2F);
    for (const cr::CreativeTerrainControlEdit& edit : plan.items()) {
      appendBounds(wireLines, terrainRodBounds(grid, edit.control, 0.22), color,
                   thickness * 1.15F);
    }
    return;
  }

  if (editor.terrain.selectionValid || editor.terrain.hoverValid ||
      editor.interaction.target.grid.valid) {
    const cr::CreativeTerrainControlPoint preview{
        aimedTerrainCoord(editor), editor.terrain.heightCells,
        editor.terrain.radiusCells};
    appendBounds(wireLines, terrainRodBounds(grid, preview, 0.24), previewColor,
                 thickness * 1.2F);
    appendTerrainFootprintOutline(wireLines, grid, preview, previewColor,
                                  thickness);
  }
}

}  // namespace iggy3d_creative_app
