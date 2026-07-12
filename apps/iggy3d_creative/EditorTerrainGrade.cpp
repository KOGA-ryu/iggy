#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void setGradeFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  editor.interaction.placementFeedback = {};
  editor.interaction.placementFeedback.status =
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected;
  editor.interaction.placementFeedback.frameIndex = editor.frameIndex;
}

}  // namespace

CreativeEditorTerrainGradeReceipt beginCreativeEditorTerrainGrade(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainGradeReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainGradeAction::Anchor;
  if (!resolveCreativeEditorTerrainGradeTarget(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_grade_anchor_missing";
    setGradeFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeTerrainControlPoint* control =
      appState.facade.document().terrainField().controlAt(receipt.targetCoord);
  if (control == nullptr) {
    receipt.reasonCode = "creative_editor_terrain_grade_anchor_not_control";
    setGradeFeedback(editor, false);
    return receipt;
  }

  CreativeTerrainGradeState& grade = editor.terrain.grade;
  receipt.accepted = true;
  receipt.changed = !grade.anchorValid ||
                    grade.anchorCoord != control->coord ||
                    grade.anchorHeightCells != control->heightCells ||
                    grade.targetHeightCells != control->heightCells;
  grade.anchorValid = true;
  grade.anchorCoord = control->coord;
  grade.anchorHeightCells = control->heightCells;
  grade.targetHeightCells = control->heightCells;
  receipt.reasonCode = "creative_editor_terrain_grade_anchored";
  setGradeFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainGradeReceipt applyCreativeEditorTerrainGradeWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorTerrainGradeReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainGradeAction::Apply;
  if (!editor.terrain.grade.anchorValid ||
      !resolveCreativeEditorTerrainGradeTarget(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_grade_incomplete";
    setGradeFeedback(editor, false);
    return receipt;
  }

  receipt.plan = planCreativeEditorTerrainGrade(editor, receipt.targetCoord);
  if (!receipt.plan.accepted) {
    receipt.reasonCode = receipt.plan.reasonCode;
    setGradeFeedback(editor, false);
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
    editor.terrain.grade.anchorValid = false;
  }
  setGradeFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainGradeReceipt cancelCreativeEditorTerrainGrade(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainGradeReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainGradeAction::Cancel;
  receipt.accepted = editor.terrain.grade.anchorValid;
  receipt.changed = receipt.accepted;
  editor.terrain.grade.anchorValid = false;
  receipt.reasonCode = receipt.accepted
                           ? "creative_editor_terrain_grade_cancelled"
                           : "creative_editor_terrain_grade_not_active";
  editor.interaction.placementFeedback = {};
  return receipt;
}

bool processCreativeEditorTerrainGradeQuickEdit(
    CreativeTerrainGradeState& state,
    cr::CreativeInputActionId action) noexcept {
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      if (!state.anchorValid) {
        return false;
      }
      const int direction =
          action == cr::CreativeInputActionId::QuickEditPrevious ? 1 : -1;
      const std::uint16_t before = state.targetHeightCells;
      state.targetHeightCells = static_cast<std::uint16_t>(std::clamp(
          static_cast<int>(state.targetHeightCells) + direction,
          static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
          static_cast<int>(cr::kCreativeTerrainMaximumHeightCells)));
      return state.targetHeightCells != before;
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

std::string creativeEditorTerrainGradeQuickEditLabel(
    const CreativeTerrainGradeState& state) {
  return "END HEIGHT " + std::to_string(state.targetHeightCells) +
         " | WIDTH " + std::to_string(state.radiusCells * 2U + 1U);
}

}  // namespace iggy3d_creative_app
