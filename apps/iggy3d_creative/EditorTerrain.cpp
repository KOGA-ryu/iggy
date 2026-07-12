#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorPreviewProxies.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] cr::CreativeTerrainCoord2 aimedTerrainCoord(
    const CreativeEditorState& editor) noexcept {
  const cr::CreativeGridCoord3 cell =
      editor.interaction.target.grid.targetCell;
  return {cell.x, cell.z};
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

void setFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  editor.interaction.placementFeedback = {};
  editor.interaction.placementFeedback.status =
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected;
  editor.interaction.placementFeedback.frameIndex = editor.frameIndex;
}

}  // namespace

CreativeEditorTerrainEditReceipt applyCreativeEditorTerrainEditWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    CreativeEditorTerrainEditKind kind,
    std::string_view source) {
  CreativeEditorTerrainEditReceipt receipt;
  receipt.requested = true;
  receipt.kind = kind;
  if (!editor.interaction.target.grid.valid) {
    receipt.reasonCode = "creative_editor_terrain_target_invalid";
    setFeedback(editor, false);
    return receipt;
  }

  receipt.coord = aimedTerrainCoord(editor);
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
                      editor.terrain.radiusCells != existing->radiusCells;
    editor.terrain.heightCells = existing->heightCells;
    editor.terrain.radiusCells = existing->radiusCells;
    receipt.reasonCode = "creative_editor_terrain_sampled";
    setFeedback(editor, true);
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
  setFeedback(editor, receipt.accepted);
  return receipt;
}

bool processCreativeEditorTerrainQuickEdit(
    CreativeEditorTerrainState& state,
    cr::CreativeInputActionId action) noexcept {
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      const CreativeEditorTerrainSetting before = state.selectedSetting;
      state.selectedSetting =
          before == CreativeEditorTerrainSetting::Height
              ? CreativeEditorTerrainSetting::Radius
              : CreativeEditorTerrainSetting::Height;
      return state.selectedSetting != before;
    }
    case cr::CreativeInputActionId::QuickEditDecrease:
    case cr::CreativeInputActionId::QuickEditIncrease: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditDecrease ? -1 : 1;
      if (state.selectedSetting == CreativeEditorTerrainSetting::Height) {
        const std::uint16_t before = state.heightCells;
        state.heightCells = static_cast<std::uint16_t>(std::clamp(
            static_cast<int>(state.heightCells) + direction,
            static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
            static_cast<int>(cr::kCreativeTerrainMaximumHeightCells)));
        return state.heightCells != before;
      }
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
  if (state.selectedSetting == CreativeEditorTerrainSetting::Height) {
    return "HEIGHT " + std::to_string(state.heightCells);
  }
  return "RADIUS " + std::to_string(state.radiusCells);
}

void appendCreativeEditorTerrainOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::TerrainControl ||
      editor.catalog.model.open || editor.catalog.toolWheel.open ||
      editor.toolOptions.open || editor.transform.active) {
    return;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const float thickness = std::max(0.02F, wireThickness * 0.75F);
  constexpr iggy3d::RenderLineColor existingColor{0.18F, 0.82F, 0.92F, 1.0F};
  constexpr iggy3d::RenderLineColor influenceColor{0.16F, 0.45F, 0.52F, 0.9F};
  constexpr iggy3d::RenderLineColor previewColor{0.98F, 0.88F, 0.16F, 1.0F};
  for (const cr::CreativeTerrainControlPoint& control :
       document.terrainField().controls()) {
    appendBounds(wireLines, terrainRodBounds(grid, control, 0.18),
                 existingColor, thickness);
    cr::CreativeBounds influence = terrainRodBounds(
        grid, control, static_cast<double>(control.radiusCells * 2U + 1U));
    influence.max.y = influence.min.y + grid.cellSizeMeters * 0.08;
    appendBounds(wireLines, influence, influenceColor, thickness * 0.65F);
  }

  if (editor.interaction.target.grid.valid) {
    const cr::CreativeTerrainControlPoint preview{
        aimedTerrainCoord(editor), editor.terrain.heightCells,
        editor.terrain.radiusCells};
    appendBounds(wireLines, terrainRodBounds(grid, preview, 0.24), previewColor,
                 thickness * 1.2F);
    cr::CreativeBounds influence = terrainRodBounds(
        grid, preview, static_cast<double>(preview.radiusCells * 2U + 1U));
    influence.max.y = influence.min.y + grid.cellSizeMeters * 0.12;
    appendBounds(wireLines, influence, previewColor, thickness);
  }
}

}  // namespace iggy3d_creative_app
