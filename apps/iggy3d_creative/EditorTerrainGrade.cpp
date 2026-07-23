#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

struct GradeHandleMatch {
  const cr::CreativeTerrainOperation* operation = nullptr;
  CreativeTerrainGradeHandle handle = CreativeTerrainGradeHandle::Count;
};

void setGradeFeedback(CreativeEditorState& editor, bool accepted) noexcept {
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

[[nodiscard]] GradeHandleMatch findGradeHandle(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 coord) noexcept {
  const std::vector<cr::CreativeTerrainOperation>& operations =
      document.terrainOperationStack().operations;
  for (auto it = operations.rbegin(); it != operations.rend(); ++it) {
    if (it->kind != cr::CreativeTerrainOperationKind::Grade) {
      continue;
    }
    if (it->grade.start == coord) {
      return {&*it, CreativeTerrainGradeHandle::Start};
    }
    if (it->grade.end == coord) {
      return {&*it, CreativeTerrainGradeHandle::End};
    }
  }
  return {};
}

[[nodiscard]] cr::CreativeTerrainOperationMutationRequest gradeRequest(
    const CreativeTerrainGradeState& state,
    const cr::CreativeDocument& document) noexcept {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = state.editingOperationId ==
                         cr::kInvalidCreativeTerrainOperationId
                     ? cr::CreativeTerrainOperationMutationKind::Add
                     : cr::CreativeTerrainOperationMutationKind::Update;
  request.operationId = state.editingOperationId;
  request.operationKind = cr::CreativeTerrainOperationKind::Grade;
  request.grade = state.recipe;
  const cr::CreativeTerrainOperation* existing =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       state.editingOperationId);
  request.enabled = existing == nullptr ? true : existing->enabled;
  return request;
}

void clearGradePreview(CreativeTerrainGradeState& state) noexcept {
  state.operationPreview = {};
  state.sourceDocumentId = cr::kInvalidDocumentId;
  state.sourceDocumentRevision = 0U;
}

[[nodiscard]] std::uint16_t adjustedHeight(std::uint16_t value,
                                           int direction) noexcept {
  return static_cast<std::uint16_t>(std::clamp(
      static_cast<int>(value) + direction,
      static_cast<int>(cr::kCreativeTerrainMinimumHeightCells),
      static_cast<int>(cr::kCreativeTerrainMaximumHeightCells)));
}

[[nodiscard]] std::uint16_t adjustedUnsigned(std::uint16_t value,
                                             int direction,
                                             std::uint16_t maximum) noexcept {
  return static_cast<std::uint16_t>(std::clamp(
      static_cast<int>(value) + direction, 0, static_cast<int>(maximum)));
}

[[nodiscard]] std::string_view controlName(
    CreativeTerrainGradeControl control) noexcept {
  switch (control) {
    case CreativeTerrainGradeControl::StartHeight: return "START HEIGHT";
    case CreativeTerrainGradeControl::EndHeight: return "END HEIGHT";
    case CreativeTerrainGradeControl::HalfWidth: return "WIDTH";
    case CreativeTerrainGradeControl::CrossSlope: return "CROSS SLOPE";
    case CreativeTerrainGradeControl::Falloff: return "FALLOFF";
    case CreativeTerrainGradeControl::Count: break;
  }
  return "GRADE";
}

}  // namespace

CreativeEditorTerrainGradeReceipt beginCreativeEditorTerrainGrade(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor) {
  CreativeEditorTerrainGradeReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainGradeAction::Anchor;
  if (!resolveCreativeEditorTerrainGradeTarget(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_grade_handle_missing";
    setGradeFeedback(editor, false);
    return receipt;
  }

  const cr::CreativeDocument& document = appState.facade.document();
  CreativeTerrainGradeState next = editor.terrain.grade;
  const GradeHandleMatch match = findGradeHandle(document, receipt.targetCoord);
  if (match.operation != nullptr) {
    next.active = true;
    next.selectedHandle = match.handle;
    next.selectedControl = match.handle == CreativeTerrainGradeHandle::Start
                               ? CreativeTerrainGradeControl::StartHeight
                               : CreativeTerrainGradeControl::EndHeight;
    next.editingOperationId = match.operation->id;
    next.recipe = match.operation->grade;
    next.statusMessage = "Editing terrain grade";
    receipt.reasonCode = "creative_editor_terrain_grade_reopened";
  } else {
    cr::CreativeTerrainGradeRecipe recipe;
    recipe.start = receipt.targetCoord;
    recipe.end = receipt.targetCoord.x == std::numeric_limits<std::int32_t>::max()
                     ? cr::CreativeTerrainCoord2{receipt.targetCoord.x - 1,
                                                receipt.targetCoord.z}
                     : cr::CreativeTerrainCoord2{receipt.targetCoord.x + 1,
                                                receipt.targetCoord.z};
    const std::uint16_t height = terrainHeightAt(
        document, receipt.targetCoord, editor.terrain.heightCells);
    recipe.startHeightCells = height;
    recipe.endHeightCells = height;
    recipe.halfWidthCells = next.recipe.halfWidthCells;
    recipe.crossSlopePermille = next.recipe.crossSlopePermille;
    recipe.falloffCells = next.recipe.falloffCells;
    next.active = true;
    next.selectedHandle = CreativeTerrainGradeHandle::End;
    next.selectedControl = CreativeTerrainGradeControl::EndHeight;
    next.editingOperationId = cr::kInvalidCreativeTerrainOperationId;
    next.recipe = recipe;
    next.statusMessage = "Aim the grade end handle";
    receipt.reasonCode = "creative_editor_terrain_grade_started";
  }
  clearGradePreview(next);
  next.sourceDocumentId = document.id();
  next.sourceDocumentRevision = document.revision();
  receipt.accepted = true;
  receipt.changed = !(next.active == editor.terrain.grade.active &&
                      next.selectedHandle ==
                          editor.terrain.grade.selectedHandle &&
                      next.editingOperationId ==
                          editor.terrain.grade.editingOperationId &&
                      next.recipe == editor.terrain.grade.recipe);
  editor.terrain.grade = std::move(next);
  setGradeFeedback(editor, true);
  return receipt;
}

bool refreshCreativeEditorTerrainGradePreview(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) {
  CreativeTerrainGradeState& state = editor.terrain.grade;
  if (!state.active) {
    clearGradePreview(state);
    return false;
  }
  cr::CreativeTerrainCoord2 target;
  if (!resolveCreativeEditorTerrainGradeTarget(editor, target)) {
    clearGradePreview(state);
    state.statusMessage = "Grade handle target unavailable";
    return false;
  }
  if (state.selectedHandle == CreativeTerrainGradeHandle::Start) {
    state.recipe.start = target;
  } else {
    state.recipe.end = target;
  }
  state.operationPreview = cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      gradeRequest(state, document));
  state.sourceDocumentId = document.id();
  state.sourceDocumentRevision = document.revision();
  ++state.previewCount;
  state.statusMessage = state.operationPreview.receipt.accepted
                            ? "Terrain grade preview ready"
                            : "Terrain grade invalid: " +
                                  std::string(
                                      state.operationPreview.receipt.reasonCode);
  return state.operationPreview.receipt.accepted;
}

bool creativeEditorTerrainGradePreviewMatches(
    const CreativeTerrainGradeState& state,
    const cr::CreativeDocument& document) noexcept {
  return state.active && state.sourceDocumentId == document.id() &&
         state.sourceDocumentRevision == document.revision() &&
         state.operationPreview.receipt.accepted;
}

CreativeEditorTerrainGradeReceipt applyCreativeEditorTerrainGradeWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorTerrainGradeReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainGradeAction::Apply;
  if (!editor.terrain.grade.active ||
      !resolveCreativeEditorTerrainGradeTarget(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_grade_incomplete";
    setGradeFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeDocument& document = appState.facade.document();
  static_cast<void>(refreshCreativeEditorTerrainGradePreview(document, editor));
  receipt.plan = editor.terrain.grade.operationPreview;
  if (!receipt.plan.receipt.accepted) {
    receipt.reasonCode = receipt.plan.receipt.reasonCode;
    setGradeFeedback(editor, false);
    return receipt;
  }

  const cr::CreativeTerrainOperationMutationRequest request =
      gradeRequest(editor.terrain.grade, document);
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
                           : "creative_editor_terrain_grade_applied";
  if (receipt.operation.accepted) {
    CreativeTerrainGradeState& state = editor.terrain.grade;
    state.editingOperationId = receipt.operation.operationId;
    state.active = false;
    clearGradePreview(state);
    state.sourceDocumentId = appState.facade.document().id();
    state.sourceDocumentRevision = appState.facade.document().revision();
    state.statusMessage = receipt.changed ? "Terrain grade applied"
                                          : "Terrain grade unchanged";
  }
  setGradeFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainGradeReceipt cancelCreativeEditorTerrainGrade(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainGradeReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainGradeAction::Cancel;
  receipt.accepted = editor.terrain.grade.active;
  receipt.changed = receipt.accepted;
  editor.terrain.grade.active = false;
  clearGradePreview(editor.terrain.grade);
  editor.terrain.grade.statusMessage = receipt.accepted
                                          ? "Terrain grade canceled"
                                          : "Terrain grade not active";
  receipt.reasonCode = receipt.accepted
                           ? "creative_editor_terrain_grade_cancelled"
                           : "creative_editor_terrain_grade_not_active";
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return receipt;
}

bool processCreativeEditorTerrainGradeQuickEdit(
    CreativeTerrainGradeState& state,
    cr::CreativeInputActionId action) noexcept {
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
    case cr::CreativeInputActionId::QuickEditNext: {
      const std::uint8_t count =
          static_cast<std::uint8_t>(CreativeTerrainGradeControl::Count);
      const std::uint8_t before = static_cast<std::uint8_t>(state.selectedControl);
      const std::uint8_t after =
          action == cr::CreativeInputActionId::QuickEditPrevious
              ? static_cast<std::uint8_t>((before + count - 1U) % count)
              : static_cast<std::uint8_t>((before + 1U) % count);
      state.selectedControl = static_cast<CreativeTerrainGradeControl>(after);
      return after != before;
    }
    case cr::CreativeInputActionId::QuickEditDecrease:
    case cr::CreativeInputActionId::QuickEditIncrease: {
      const int direction =
          action == cr::CreativeInputActionId::QuickEditDecrease ? -1 : 1;
      const cr::CreativeTerrainGradeRecipe before = state.recipe;
      switch (state.selectedControl) {
        case CreativeTerrainGradeControl::StartHeight:
          state.recipe.startHeightCells =
              adjustedHeight(state.recipe.startHeightCells, direction);
          break;
        case CreativeTerrainGradeControl::EndHeight:
          state.recipe.endHeightCells =
              adjustedHeight(state.recipe.endHeightCells, direction);
          break;
        case CreativeTerrainGradeControl::HalfWidth:
          state.recipe.halfWidthCells = adjustedUnsigned(
              state.recipe.halfWidthCells, direction,
              cr::kCreativeTerrainGradeMaximumHalfWidthCells);
          break;
        case CreativeTerrainGradeControl::CrossSlope:
          state.recipe.crossSlopePermille = std::clamp(
              state.recipe.crossSlopePermille + direction * 10,
              -cr::kCreativeTerrainGradeMaximumCrossSlopePermille,
              cr::kCreativeTerrainGradeMaximumCrossSlopePermille);
          break;
        case CreativeTerrainGradeControl::Falloff:
          state.recipe.falloffCells = adjustedUnsigned(
              state.recipe.falloffCells, direction,
              cr::kCreativeTerrainGradeMaximumFalloffCells);
          break;
        case CreativeTerrainGradeControl::Count: return false;
      }
      return state.recipe != before;
    }
    default:
      return false;
  }
}

std::string creativeEditorTerrainGradeQuickEditLabel(
    const CreativeTerrainGradeState& state) {
  char value[96]{};
  switch (state.selectedControl) {
    case CreativeTerrainGradeControl::StartHeight:
      std::snprintf(value, sizeof(value), "%u",
                    state.recipe.startHeightCells);
      break;
    case CreativeTerrainGradeControl::EndHeight:
      std::snprintf(value, sizeof(value), "%u", state.recipe.endHeightCells);
      break;
    case CreativeTerrainGradeControl::HalfWidth:
      std::snprintf(value, sizeof(value), "%u CELLS",
                    state.recipe.halfWidthCells * 2U + 1U);
      break;
    case CreativeTerrainGradeControl::CrossSlope:
      std::snprintf(value, sizeof(value), "%.1f%%",
                    state.recipe.crossSlopePermille / 10.0);
      break;
    case CreativeTerrainGradeControl::Falloff:
      std::snprintf(value, sizeof(value), "%u CELLS",
                    state.recipe.falloffCells);
      break;
    case CreativeTerrainGradeControl::Count:
      std::snprintf(value, sizeof(value), "INVALID");
      break;
  }
  std::string label{controlName(state.selectedControl)};
  label.push_back(' ');
  label.append(value);
  if (state.operationPreview.receipt.accepted) {
    const cr::CreativeTerrainGradeReadout& readout =
        state.operationPreview.receipt.replay.lastGradeReadout;
    char readoutText[192]{};
    std::snprintf(readoutText, sizeof(readoutText),
                  " | LONG %.1f%% | CROSS %.1f%% | MAX %.1f DEG | %s %s",
                  readout.longitudinalSlopePercent,
                  readout.crossSlopePercent,
                  readout.maximumCollisionSlopeDegrees,
                  std::string(readout.movementBand).c_str(),
                  readout.walkable ? "WALKABLE" : "BLOCKED");
    label.append(readoutText);
  }
  return label;
}

}  // namespace iggy3d_creative_app
