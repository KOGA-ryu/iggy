#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/tools/TerrainBrushKernel.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void invalidateProfilePreview(CreativeTerrainProfileState& profile) noexcept {
  profile.preview.valid = false;
  profile.preview.renderAccepted = false;
  profile.preview.patches.clear();
}

void setProfileFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  editor.interaction.placementFeedback = {};
  editor.interaction.placementFeedback.status =
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected;
  editor.interaction.placementFeedback.frameIndex = editor.frameIndex;
}

[[nodiscard]] std::uint16_t resolvedBaseHeight(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 center) noexcept {
  if (editor.terrain.profile.baseLocked) {
    return editor.terrain.profile.lockedBaseHeightCells;
  }
  const cr::CreativeTerrainHeightSample sample =
      cr::sampleCreativeTerrainHeight(document.terrainField(), center);
  return sample.present ? sample.heightCells : editor.terrain.heightCells;
}

[[nodiscard]] cr::CreativeTerrainProfileRequest profileRequest(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) noexcept {
  cr::CreativeTerrainProfileRequest request;
  request.field = &document.terrainField();
  request.center = target;
  request.baseHeightCells = resolvedBaseHeight(document, editor, target);
  request.profile = editor.toolSettings.terrainProfileKind;
  request.blend = editor.toolSettings.terrainProfileBlend;
  request.rodPolicy = editor.toolSettings.terrainProfileRodPolicy;
  request.direction = editor.toolSettings.terrainProfileDirection;
  request.radiusCells = cr::creativeTerrainProfileRadiusCells(
      editor.toolSettings.terrainProfileRadius);
  request.amplitudeCells = cr::creativeTerrainProfileAmplitudeCells(
      editor.toolSettings.terrainProfileAmplitude);
  request.spacingCells = cr::creativeTerrainProfileSpacingCells(
      editor.toolSettings.terrainProfileSpacing);
  request.frequency = cr::creativeTerrainProfileFrequencyCycles(
      editor.toolSettings.terrainProfileFrequency);
  return request;
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainProfilePreviewCache& cache,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 center,
    std::uint16_t baseHeight) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.terrainRevision == document.terrainField().revision() &&
         cache.center == center &&
         cache.resolvedBaseHeightCells == baseHeight &&
         cache.profile == editor.toolSettings.terrainProfileKind &&
         cache.blend == editor.toolSettings.terrainProfileBlend &&
         cache.rodPolicy == editor.toolSettings.terrainProfileRodPolicy &&
         cache.direction == editor.toolSettings.terrainProfileDirection &&
         cache.radiusCells == cr::creativeTerrainProfileRadiusCells(
                                  editor.toolSettings.terrainProfileRadius) &&
         cache.amplitudeCells == cr::creativeTerrainProfileAmplitudeCells(
                                     editor.toolSettings
                                         .terrainProfileAmplitude) &&
         cache.spacingCells == cr::creativeTerrainProfileSpacingCells(
                                   editor.toolSettings.terrainProfileSpacing) &&
         cache.frequency == cr::creativeTerrainProfileFrequencyCycles(
                                editor.toolSettings.terrainProfileFrequency);
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

}  // namespace

cr::CreativeTerrainProfilePlan planCreativeEditorTerrainProfile(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) noexcept {
  return cr::buildCreativeTerrainProfilePlan(
      profileRequest(document, editor, target));
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
  receipt.plan = planCreativeEditorTerrainProfile(
      appState.facade.document(), editor, receipt.targetCoord);
  receipt.accepted = receipt.plan.accepted;
  receipt.reasonCode = receipt.plan.reasonCode;
  if (!receipt.plan.accepted || receipt.plan.items().empty()) {
    setProfileFeedback(editor, receipt.plan.accepted);
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
  if (receipt.changed) {
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
  const cr::CreativeTerrainHeightSample sample =
      cr::sampleCreativeTerrainHeight(document.terrainField(),
                                      receipt.targetCoord);
  const std::uint16_t base =
      sample.present ? sample.heightCells : editor.terrain.heightCells;
  receipt.accepted = true;
  receipt.changed = !editor.terrain.profile.baseLocked ||
                    editor.terrain.profile.lockedBaseHeightCells != base;
  editor.terrain.profile.baseLocked = true;
  editor.terrain.profile.lockedBaseHeightCells = base;
  editor.terrain.profile.resolvedBaseHeightCells = base;
  receipt.reasonCode = "creative_editor_terrain_profile_base_locked";
  invalidateProfilePreview(editor.terrain.profile);
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
  editor.interaction.placementFeedback = {};
  return receipt;
}

bool processCreativeEditorTerrainProfileQuickEdit(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  bool changed = false;
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
      changed = stepClampedEnum(editor.toolSettings.terrainProfileAmplitude,
                                cr::CreativeTerrainProfileAmplitude::Count, 1);
      break;
    case cr::CreativeInputActionId::QuickEditNext:
      changed = stepClampedEnum(editor.toolSettings.terrainProfileAmplitude,
                                cr::CreativeTerrainProfileAmplitude::Count, -1);
      break;
    case cr::CreativeInputActionId::QuickEditDecrease:
      changed = stepClampedEnum(editor.toolSettings.terrainProfileRadius,
                                cr::CreativeTerrainProfileRadius::Count, -1);
      break;
    case cr::CreativeInputActionId::QuickEditIncrease:
      changed = stepClampedEnum(editor.toolSettings.terrainProfileRadius,
                                cr::CreativeTerrainProfileRadius::Count, 1);
      break;
    default:
      break;
  }
  if (changed) {
    invalidateProfilePreview(editor.terrain.profile);
  }
  return changed;
}

std::string creativeEditorTerrainProfileQuickEditLabel(
    const CreativeEditorState& editor) {
  const CreativeTerrainProfileState& profile = editor.terrain.profile;
  std::string label("AMP ");
  label.append(std::to_string(cr::creativeTerrainProfileAmplitudeCells(
      editor.toolSettings.terrainProfileAmplitude)));
  label.append(" | RADIUS ");
  label.append(std::to_string(cr::creativeTerrainProfileRadiusCells(
      editor.toolSettings.terrainProfileRadius)));
  label.append(profile.baseLocked ? " | BASE LOCK " : " | BASE AUTO ");
  label.append(std::to_string(profile.resolvedBaseHeightCells));
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
  const std::uint16_t baseHeight = resolvedBaseHeight(document, editor, center);
  state.profile.resolvedBaseHeightCells = baseHeight;
  CreativeTerrainProfilePreviewCache& cache = state.profile.preview;
  if (previewKeyMatches(cache, document, editor, center, baseHeight)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.terrainRevision = document.terrainField().revision();
  cache.buildCount = nextBuildCount;
  cache.center = center;
  cache.resolvedBaseHeightCells = baseHeight;
  cache.profile = editor.toolSettings.terrainProfileKind;
  cache.blend = editor.toolSettings.terrainProfileBlend;
  cache.rodPolicy = editor.toolSettings.terrainProfileRodPolicy;
  cache.direction = editor.toolSettings.terrainProfileDirection;
  cache.radiusCells = cr::creativeTerrainProfileRadiusCells(
      editor.toolSettings.terrainProfileRadius);
  cache.amplitudeCells = cr::creativeTerrainProfileAmplitudeCells(
      editor.toolSettings.terrainProfileAmplitude);
  cache.spacingCells = cr::creativeTerrainProfileSpacingCells(
      editor.toolSettings.terrainProfileSpacing);
  cache.frequency = cr::creativeTerrainProfileFrequencyCycles(
      editor.toolSettings.terrainProfileFrequency);
  cache.plan = planCreativeEditorTerrainProfile(document, editor, center);
  if (!cache.plan.accepted) {
    return true;
  }

  cr::CreativeTerrainField previewField = document.terrainField();
  if (!cache.plan.items().empty()) {
    const cr::CreativeTerrainMutationReceipt mutation =
        previewField.apply(cache.plan.items());
    if (!mutation.accepted) {
      return true;
    }
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainRenderPlan render =
      cr::buildCreativeTerrainRenderPlan(previewField, grid.origin,
                                         grid.cellSizeMeters);
  if (!render.accepted) {
    return true;
  }
  cache.patches.reserve(render.patches.size());
  for (const cr::CreativeTerrainSurfacePatch& patch : render.patches) {
    if (cr::creativeTerrainInsideRadius(center, patch.coord,
                                        cache.radiusCells)) {
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
  const CreativeTerrainProfilePreviewCache& preview =
      editor.terrain.profile.preview;
  if (!preview.valid) {
    return;
  }
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  const bool accepted = preview.plan.accepted && preview.renderAccepted;
  const iggy3d::RenderLineColor color = accepted ? admitted : rejected;
  appendCreativeEditorTerrainFootprintOutline(
      wireLines, document.gridSettings(),
      {preview.center, preview.resolvedBaseHeightCells, preview.radiusCells},
      color, wireThickness * 1.25F);
  if (!accepted) {
    return;
  }
  for (const cr::CreativeTerrainControlEdit& edit : preview.plan.items()) {
    appendCreativeEditorTerrainControlGuide(
        wireLines, document.gridSettings(), edit.control, admitted,
        wireThickness * 1.2F);
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.75F);
  }
}

}  // namespace iggy3d_creative_app
