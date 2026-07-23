#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void invalidateProfilePreview(CreativeTerrainProfileState& state) noexcept {
  const std::uint64_t buildCount = state.preview.buildCount;
  state.preview = {};
  state.preview.buildCount = buildCount;
}

void setProfileFeedback(CreativeEditorState& editor, bool accepted) noexcept {
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

[[nodiscard]] std::uint16_t resolvedBaseHeight(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 center) noexcept {
  const CreativeTerrainProfileState& state = editor.terrain.profile;
  return state.baseLocked
             ? state.lockedBaseHeightCells
             : terrainHeightAt(document, center, editor.terrain.heightCells);
}

[[nodiscard]] cr::CreativeTerrainProfileRecipe profileRecipe(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 center) noexcept {
  cr::CreativeTerrainProfileRecipe recipe;
  recipe.center = center;
  recipe.baseHeightCells = resolvedBaseHeight(document, editor, center);
  recipe.profile = editor.toolSettings.terrainProfileKind;
  recipe.blend = editor.toolSettings.terrainProfileBlend;
  recipe.rodPolicy = editor.toolSettings.terrainProfileRodPolicy;
  recipe.direction = editor.toolSettings.terrainProfileDirection;
  recipe.radiusCells = editor.toolSettings.terrainProfileRadiusCells;
  recipe.amplitudeCells = editor.toolSettings.terrainProfileAmplitudeCells;
  recipe.spacingCells = editor.toolSettings.terrainProfileSpacingCells;
  recipe.frequency = editor.toolSettings.terrainProfileFrequencyCycles;
  recipe.seed = editor.toolSettings.terrainProfileSeed;
  return recipe;
}

[[nodiscard]] cr::CreativeTerrainOperationMutationRequest operationRequest(
    const CreativeTerrainProfileState& state,
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainProfileRecipe& recipe) {
  cr::CreativeTerrainOperationMutationRequest request;
  request.kind = state.editingOperationId ==
                         cr::kInvalidCreativeTerrainOperationId
                     ? cr::CreativeTerrainOperationMutationKind::Add
                     : cr::CreativeTerrainOperationMutationKind::Update;
  request.operationId = state.editingOperationId;
  request.operationKind = cr::CreativeTerrainOperationKind::Profile;
  request.profile = recipe;
  const cr::CreativeTerrainOperation* existing =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       state.editingOperationId);
  request.enabled = existing == nullptr ? true : existing->enabled;
  request.owner = existing == nullptr
                      ? cr::CreativeTerrainOperationOwner::Manual
                      : existing->owner;
  request.sourceKey = existing == nullptr ? std::string{} : existing->sourceKey;
  return request;
}

[[nodiscard]] const cr::CreativeTerrainOperation* profileAtCenter(
    const cr::CreativeDocument& document,
    cr::CreativeTerrainCoord2 center) noexcept {
  const auto& operations = document.terrainOperationStack().operations;
  for (auto it = operations.rbegin(); it != operations.rend(); ++it) {
    if (it->kind == cr::CreativeTerrainOperationKind::Profile &&
        it->owner == cr::CreativeTerrainOperationOwner::Manual &&
        it->profile.center == center) {
      return &*it;
    }
  }
  return nullptr;
}

void loadProfileOperation(const cr::CreativeTerrainOperation& operation,
                          CreativeEditorState& editor) noexcept {
  CreativeTerrainProfileState& state = editor.terrain.profile;
  const cr::CreativeTerrainProfileRecipe& recipe = operation.profile;
  state.editingOperationId = operation.id;
  state.baseLocked = true;
  state.lockedBaseHeightCells = recipe.baseHeightCells;
  state.resolvedBaseHeightCells = recipe.baseHeightCells;
  state.selectedControl = CreativeTerrainProfileControl::Amplitude;
  editor.toolSettings.terrainProfileKind = recipe.profile;
  editor.toolSettings.terrainProfileBlend = recipe.blend;
  editor.toolSettings.terrainProfileRodPolicy = recipe.rodPolicy;
  editor.toolSettings.terrainProfileDirection = recipe.direction;
  editor.toolSettings.terrainProfileRadiusCells = recipe.radiusCells;
  editor.toolSettings.terrainProfileAmplitudeCells = recipe.amplitudeCells;
  editor.toolSettings.terrainProfileSpacingCells = recipe.spacingCells;
  editor.toolSettings.terrainProfileFrequencyCycles = recipe.frequency;
  editor.toolSettings.terrainProfileSeed = recipe.seed;
  invalidateProfilePreview(state);
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainProfilePreviewCache& cache,
    const cr::CreativeDocument& document,
    const cr::CreativeTerrainProfileRecipe& recipe) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.documentRevision == document.revision() &&
         cache.recipe == recipe;
}

[[nodiscard]] std::uint16_t adjustedUnsigned(std::uint16_t value,
                                             int direction,
                                             std::uint16_t minimum,
                                             std::uint16_t maximum) noexcept {
  const int next = std::clamp(static_cast<int>(value) + direction,
                              static_cast<int>(minimum),
                              static_cast<int>(maximum));
  return static_cast<std::uint16_t>(next);
}

[[nodiscard]] std::uint8_t adjustedUnsigned(std::uint8_t value,
                                            int direction,
                                            std::uint8_t minimum,
                                            std::uint8_t maximum) noexcept {
  const int next = std::clamp(static_cast<int>(value) + direction,
                              static_cast<int>(minimum),
                              static_cast<int>(maximum));
  return static_cast<std::uint8_t>(next);
}

[[nodiscard]] std::string_view controlName(
    CreativeTerrainProfileControl control) noexcept {
  switch (control) {
    case CreativeTerrainProfileControl::BaseHeight: return "BASE";
    case CreativeTerrainProfileControl::Radius: return "RADIUS";
    case CreativeTerrainProfileControl::Amplitude: return "AMPLITUDE";
    case CreativeTerrainProfileControl::Direction: return "DIRECTION";
    case CreativeTerrainProfileControl::Frequency: return "FREQUENCY";
    case CreativeTerrainProfileControl::Spacing: return "SPACING";
    case CreativeTerrainProfileControl::Seed: return "SEED";
    case CreativeTerrainProfileControl::Count: break;
  }
  return "PROFILE";
}

}  // namespace

cr::CreativeTerrainOperationMutationPlan planCreativeEditorTerrainProfile(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) {
  const cr::CreativeTerrainProfileRecipe recipe =
      profileRecipe(document, editor, target);
  return cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      operationRequest(editor.terrain.profile, document, recipe));
}

CreativeEditorTerrainProfileReceipt
selectCreativeEditorTerrainProfileOperation(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor,
    cr::CreativeTerrainOperationId operationId) noexcept {
  CreativeEditorTerrainProfileReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainProfileAction::SelectOperation;
  const cr::CreativeTerrainOperation* operation =
      cr::findCreativeTerrainOperation(document.terrainOperationStack(),
                                       operationId);
  if (operation == nullptr ||
      operation->kind != cr::CreativeTerrainOperationKind::Profile ||
      operation->owner != cr::CreativeTerrainOperationOwner::Manual) {
    receipt.reasonCode = "creative_editor_terrain_profile_operation_invalid";
    setProfileFeedback(editor, false);
    return receipt;
  }
  receipt.targetCoord = operation->profile.center;
  receipt.changed = editor.terrain.profile.editingOperationId != operationId;
  loadProfileOperation(*operation, editor);
  receipt.accepted = true;
  receipt.reasonCode = "creative_editor_terrain_profile_operation_selected";
  setProfileFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainProfileReceipt
applyCreativeEditorTerrainProfileWithHistory(cr::CreativeAppState& appState,
                                             CreativeEditorState& editor,
                                             std::string_view source) {
  CreativeEditorTerrainProfileReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainProfileAction::Apply;
  if (!resolveCreativeEditorTerrainPointerCoord(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_profile_target_invalid";
    setProfileFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeDocument& document = appState.facade.document();
  const cr::CreativeTerrainProfileRecipe recipe =
      profileRecipe(document, editor, receipt.targetCoord);
  receipt.plan = cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      operationRequest(editor.terrain.profile, document, recipe));
  if (!receipt.plan.receipt.accepted) {
    receipt.reasonCode = receipt.plan.receipt.reasonCode;
    setProfileFeedback(editor, false);
    return receipt;
  }

  StandaloneEditTransaction transaction =
      beginEditTransaction(appState.facade, source);
  receipt.operation = appState.facade.applyTerrainOperationMutation(
      operationRequest(editor.terrain.profile, document, recipe));
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
                           : "creative_editor_terrain_profile_applied";
  if (receipt.operation.accepted) {
    editor.terrain.profile.editingOperationId = receipt.operation.operationId;
    editor.terrain.profile.resolvedBaseHeightCells = recipe.baseHeightCells;
    invalidateProfilePreview(editor.terrain.profile);
  }
  setProfileFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainProfileReceipt lockCreativeEditorTerrainProfileBase(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainProfileReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainProfileAction::LockBase;
  if (!resolveCreativeEditorTerrainPointerCoord(editor, receipt.targetCoord)) {
    receipt.reasonCode =
        "creative_editor_terrain_profile_base_target_invalid";
    setProfileFeedback(editor, false);
    return receipt;
  }
  if (editor.terrain.profile.editingOperationId ==
      cr::kInvalidCreativeTerrainOperationId) {
    const cr::CreativeTerrainOperation* operation =
        profileAtCenter(document, receipt.targetCoord);
    if (operation != nullptr) {
      loadProfileOperation(*operation, editor);
      receipt.action = CreativeEditorTerrainProfileAction::SelectOperation;
      receipt.accepted = true;
      receipt.changed = true;
      receipt.reasonCode = "creative_editor_terrain_profile_reopened";
      setProfileFeedback(editor, true);
      return receipt;
    }
  }

  const std::uint16_t base = terrainHeightAt(
      document, receipt.targetCoord, editor.terrain.heightCells);
  CreativeTerrainProfileState& state = editor.terrain.profile;
  receipt.accepted = true;
  receipt.changed = !state.baseLocked || state.lockedBaseHeightCells != base;
  state.baseLocked = true;
  state.lockedBaseHeightCells = base;
  state.resolvedBaseHeightCells = base;
  state.selectedControl = CreativeTerrainProfileControl::BaseHeight;
  receipt.reasonCode = "creative_editor_terrain_profile_base_locked";
  invalidateProfilePreview(state);
  setProfileFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainProfileReceipt unlockCreativeEditorTerrainProfileBase(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainProfileReceipt receipt;
  receipt.requested = true;
  receipt.accepted = true;
  receipt.action = CreativeEditorTerrainProfileAction::UnlockBase;
  receipt.changed = editor.terrain.profile.baseLocked;
  editor.terrain.profile.baseLocked = false;
  receipt.reasonCode = receipt.changed
                           ? "creative_editor_terrain_profile_base_auto"
                           : "creative_editor_terrain_profile_base_already_auto";
  invalidateProfilePreview(editor.terrain.profile);
  clearCreativeEditorPlacementFeedback(editor.interaction);
  return receipt;
}

bool processCreativeEditorTerrainProfileQuickEdit(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  CreativeTerrainProfileState& state = editor.terrain.profile;
  if (action == cr::CreativeInputActionId::QuickEditPrevious ||
      action == cr::CreativeInputActionId::QuickEditNext) {
    const std::uint8_t count =
        static_cast<std::uint8_t>(CreativeTerrainProfileControl::Count);
    const std::uint8_t before = static_cast<std::uint8_t>(state.selectedControl);
    const std::uint8_t after =
        action == cr::CreativeInputActionId::QuickEditPrevious
            ? static_cast<std::uint8_t>((before + count - 1U) % count)
            : static_cast<std::uint8_t>((before + 1U) % count);
    state.selectedControl = static_cast<CreativeTerrainProfileControl>(after);
    return after != before;
  }
  if (action != cr::CreativeInputActionId::QuickEditDecrease &&
      action != cr::CreativeInputActionId::QuickEditIncrease) {
    return false;
  }

  const int direction =
      action == cr::CreativeInputActionId::QuickEditIncrease ? 1 : -1;
  const cr::CreativeToolSettings before = editor.toolSettings;
  const bool baseBefore = state.baseLocked;
  const std::uint16_t lockedBaseBefore = state.lockedBaseHeightCells;
  switch (state.selectedControl) {
    case CreativeTerrainProfileControl::BaseHeight:
      state.baseLocked = true;
      state.lockedBaseHeightCells = adjustedUnsigned(
          baseBefore ? state.lockedBaseHeightCells
                     : state.resolvedBaseHeightCells,
          direction, cr::kCreativeTerrainMinimumHeightCells,
          cr::kCreativeTerrainMaximumHeightCells);
      state.resolvedBaseHeightCells = state.lockedBaseHeightCells;
      break;
    case CreativeTerrainProfileControl::Radius:
      editor.toolSettings.terrainProfileRadiusCells = adjustedUnsigned(
          editor.toolSettings.terrainProfileRadiusCells, direction, 1U,
          cr::kCreativeTerrainProfileMaximumRadiusCells);
      break;
    case CreativeTerrainProfileControl::Amplitude:
      editor.toolSettings.terrainProfileAmplitudeCells = adjustedUnsigned(
          editor.toolSettings.terrainProfileAmplitudeCells, direction, 1U,
          cr::kCreativeTerrainMaximumHeightCells);
      break;
    case CreativeTerrainProfileControl::Direction: {
      const int count =
          static_cast<int>(cr::CreativeTerrainProfileDirection::Count);
      const int current =
          static_cast<int>(editor.toolSettings.terrainProfileDirection);
      editor.toolSettings.terrainProfileDirection =
          static_cast<cr::CreativeTerrainProfileDirection>(
              (current + (direction > 0 ? 1 : count - 1)) % count);
      break;
    }
    case CreativeTerrainProfileControl::Frequency:
      editor.toolSettings.terrainProfileFrequencyCycles = adjustedUnsigned(
          editor.toolSettings.terrainProfileFrequencyCycles, direction,
          std::uint8_t{1U}, cr::kCreativeTerrainProfileMaximumFrequency);
      break;
    case CreativeTerrainProfileControl::Spacing:
      editor.toolSettings.terrainProfileSpacingCells = adjustedUnsigned(
          editor.toolSettings.terrainProfileSpacingCells, direction, 1U,
          cr::kCreativeTerrainProfileMaximumSpacingCells);
      break;
    case CreativeTerrainProfileControl::Seed:
      if (direction > 0 &&
          editor.toolSettings.terrainProfileSeed !=
              std::numeric_limits<std::uint64_t>::max()) {
        ++editor.toolSettings.terrainProfileSeed;
      } else if (direction < 0 &&
                 editor.toolSettings.terrainProfileSeed > 0U) {
        --editor.toolSettings.terrainProfileSeed;
      }
      break;
    case CreativeTerrainProfileControl::Count: return false;
  }
  const bool changed = before != editor.toolSettings ||
                       baseBefore != state.baseLocked ||
                       lockedBaseBefore != state.lockedBaseHeightCells;
  if (changed) {
    invalidateProfilePreview(state);
  }
  return changed;
}

std::string creativeEditorTerrainProfileQuickEditLabel(
    const CreativeEditorState& editor) {
  const CreativeTerrainProfileState& state = editor.terrain.profile;
  char value[96]{};
  switch (state.selectedControl) {
    case CreativeTerrainProfileControl::BaseHeight:
      std::snprintf(value, sizeof(value), "%u%s",
                    state.resolvedBaseHeightCells,
                    state.baseLocked ? " LOCKED" : " AUTO");
      break;
    case CreativeTerrainProfileControl::Radius:
      std::snprintf(value, sizeof(value), "%u CELLS",
                    editor.toolSettings.terrainProfileRadiusCells);
      break;
    case CreativeTerrainProfileControl::Amplitude:
      std::snprintf(value, sizeof(value), "%u CELLS",
                    editor.toolSettings.terrainProfileAmplitudeCells);
      break;
    case CreativeTerrainProfileControl::Direction:
      std::snprintf(
          value, sizeof(value), "%s",
          std::string(cr::toString(
              editor.toolSettings.terrainProfileDirection)).c_str());
      break;
    case CreativeTerrainProfileControl::Frequency:
      std::snprintf(value, sizeof(value), "%u CYCLES",
                    editor.toolSettings.terrainProfileFrequencyCycles);
      break;
    case CreativeTerrainProfileControl::Spacing:
      std::snprintf(value, sizeof(value), "%u CELLS",
                    editor.toolSettings.terrainProfileSpacingCells);
      break;
    case CreativeTerrainProfileControl::Seed:
      std::snprintf(value, sizeof(value), "%llu",
                    static_cast<unsigned long long>(
                        editor.toolSettings.terrainProfileSeed));
      break;
    case CreativeTerrainProfileControl::Count:
      std::snprintf(value, sizeof(value), "INVALID");
      break;
  }
  std::string label{controlName(state.selectedControl)};
  label.push_back(' ');
  label.append(value);
  if (state.editingOperationId ==
      cr::kInvalidCreativeTerrainOperationId) {
    label.append(" | NEW");
  } else {
    label.append(" | EDIT #");
    label.append(std::to_string(state.editingOperationId));
  }
  return label;
}

bool refreshCreativeEditorTerrainProfilePreview(
    CreativeEditorTerrainState& state,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  cr::CreativeTerrainCoord2 center{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, center)) {
    invalidateProfilePreview(state.profile);
    return false;
  }
  const cr::CreativeTerrainProfileRecipe recipe =
      profileRecipe(document, editor, center);
  state.profile.resolvedBaseHeightCells = recipe.baseHeightCells;
  CreativeTerrainProfilePreviewCache& cache = state.profile.preview;
  if (previewKeyMatches(cache, document, recipe)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.buildCount = nextBuildCount;
  cache.recipe = recipe;
  cache.operationPreview = cr::planCreativeTerrainOperationMutation(
      document.terrainField(), document.terrainHeightField(),
      document.terrainMaterialField(), document.terrainOperationStack(),
      operationRequest(state.profile, document, recipe));
  if (!cache.operationPreview.receipt.accepted) {
    return true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainHeightRenderPlan(
          cache.operationPreview.heightField, grid.origin,
          grid.cellSizeMeters);
  if (!render.accepted) {
    return true;
  }
  cache.patches.reserve(render.patches.size());
  for (const cr::CreativeTerrainSurfacePatch& patch : render.patches) {
    if (cr::creativeTerrainInsideRadius(recipe.center, patch.coord,
                                        recipe.radiusCells)) {
      cache.patches.push_back(patch);
    }
  }
  cache.renderAccepted = true;
  return true;
}

void appendCreativeEditorTerrainProfileOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  constexpr iggy3d::RenderLineColor stored{0.20F, 0.58F, 0.72F, 0.72F};
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  for (const cr::CreativeTerrainOperation& operation :
       document.terrainOperationStack().operations) {
    if (operation.kind != cr::CreativeTerrainOperationKind::Profile ||
        operation.id == editor.terrain.profile.editingOperationId) {
      continue;
    }
    appendCreativeEditorTerrainControlGuide(
        wireLines, document.gridSettings(),
        {operation.profile.center, operation.profile.baseHeightCells, 0U},
        stored, wireThickness * 0.9F);
  }

  const CreativeTerrainProfilePreviewCache& preview =
      editor.terrain.profile.preview;
  if (!preview.valid) {
    return;
  }
  const bool accepted = preview.operationPreview.receipt.accepted &&
                        preview.renderAccepted;
  const iggy3d::RenderLineColor color = accepted ? admitted : rejected;
  appendCreativeEditorTerrainFootprintOutline(
      wireLines, document.gridSettings(),
      {preview.recipe.center, preview.recipe.baseHeightCells,
       preview.recipe.radiusCells},
      color, wireThickness * 1.25F);
  appendCreativeEditorTerrainControlGuide(
      wireLines, document.gridSettings(),
      {preview.recipe.center, preview.recipe.baseHeightCells, 0U}, color,
      wireThickness * 1.4F);
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.75F);
  }
}

}  // namespace iggy3d_creative_app
