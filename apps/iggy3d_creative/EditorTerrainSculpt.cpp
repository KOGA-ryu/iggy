#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "runtime/movement/MovementPolicy.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

[[nodiscard]] cr::CreativeTerrainSculptRequest sculptRequest(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) noexcept {
  return {document.terrainField().controls(),
          target,
          editor.toolSettings.terrainSculptMode,
          cr::creativeTerrainSculptRadiusCells(
              editor.toolSettings.terrainSculptRadius),
          cr::creativeTerrainSculptStrengthCells(
              editor.toolSettings.terrainSculptStrength),
          editor.terrain.sculpt.targetHeightCells};
}

void setSculptFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  editor.interaction.placementFeedback = {};
  editor.interaction.placementFeedback.status =
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected;
  editor.interaction.placementFeedback.frameIndex = editor.frameIndex;
}

void invalidateSculptPreview(CreativeTerrainSculptState& sculpt) noexcept {
  sculpt.preview.valid = false;
  sculpt.preview.renderAccepted = false;
  sculpt.preview.patches.clear();
}

[[nodiscard]] bool previewKeyMatches(
    const CreativeTerrainSculptPreviewCache& cache,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 center) noexcept {
  return cache.valid && cache.documentId == document.id() &&
         cache.documentRevision == document.revision() &&
         cache.center == center &&
         cache.mode == editor.toolSettings.terrainSculptMode &&
         cache.radiusCells == cr::creativeTerrainSculptRadiusCells(
                                  editor.toolSettings.terrainSculptRadius) &&
         cache.strengthCells == cr::creativeTerrainSculptStrengthCells(
                                    editor.toolSettings.terrainSculptStrength) &&
         (!cr::creativeTerrainSculptUsesTargetHeight(cache.mode) ||
          cache.targetHeightCells ==
              editor.terrain.sculpt.targetHeightCells);
}

[[nodiscard]] bool insideBrush(cr::CreativeTerrainCoord2 center,
                               cr::CreativeTerrainCoord2 coord,
                               std::uint16_t radiusCells) noexcept {
  const std::int64_t dx = static_cast<std::int64_t>(center.x) - coord.x;
  const std::int64_t dz = static_cast<std::int64_t>(center.z) - coord.z;
  const std::int64_t radius = radiusCells;
  if (dx < -radius || dx > radius || dz < -radius || dz > radius) {
    return false;
  }
  return dx * dx + dz * dz <= radius * radius;
}

void appendLine(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
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

[[nodiscard]] iggy3d::RenderLineColor slopeColor(
    iggy3d::Vec3 first,
    iggy3d::Vec3 second,
    iggy3d::Vec3 third) noexcept {
  constexpr iggy3d::RenderLineColor walkable{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor careful{1.0F, 0.82F, 0.16F, 1.0F};
  constexpr iggy3d::RenderLineColor blocked{1.0F, 0.20F, 0.18F, 1.0F};
  iggy3d::Vec3 normal;
  if (!iggy3d::tryNormalize(iggy3d::cross(second - first, third - first),
                            normal)) {
    return blocked;
  }
  if (normal.y < 0.0F) {
    normal = normal * -1.0F;
  }
  const iggy3d::SlopeSample slope = iggy3d::sampleSlope(normal);
  if (!slope.valid || !slope.walkable) {
    return blocked;
  }
  return slope.carefulFooting ? careful : walkable;
}

void appendPatchSlopeTriangles(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    const cr::CreativeTerrainSurfacePatch& patch,
    float thickness) {
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
    const iggy3d::RenderLineColor color =
        slopeColor(center.value, first.value, second.value);
    appendLine(lines, center.value, first.value, color, thickness);
    appendLine(lines, first.value, second.value, color, thickness);
    appendLine(lines, second.value, center.value, color, thickness);
  }
}

[[nodiscard]] cr::CreativeDocumentHistoryTransaction& ensureSculptTransaction(
    cr::CreativeAppState& appState,
    CreativeTerrainSculptStrokeState& stroke) {
  if (!stroke.transaction.active) {
    stroke.transaction =
        beginEditTransaction(appState.facade, "creative_terrain_sculpt_stroke");
  }
  return stroke.transaction;
}

void applySculptStrokeMutation(cr::CreativeAppState& appState,
                               CreativeEditorState& editor) {
  cr::CreativeTerrainCoord2 target{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, target)) {
    setSculptFeedback(editor, false);
    return;
  }
  const cr::CreativeTerrainSculptPlan plan =
      planCreativeEditorTerrainSculpt(appState.facade.document(), editor, target);
  if (!plan.accepted || plan.items().empty()) {
    setSculptFeedback(editor, plan.accepted);
    return;
  }

  CreativeTerrainSculptStrokeState& stroke = editor.terrain.sculpt.stroke;
  static_cast<void>(ensureSculptTransaction(appState, stroke));
  editor.terrain.lastMutation =
      appState.facade.applyTerrainControlEdits(plan.items());
  if (editor.terrain.lastMutation.accepted &&
      editor.terrain.lastMutation.changed) {
    ++stroke.acceptedMutationCount;
    invalidateSculptPreview(editor.terrain.sculpt);
  }
  setSculptFeedback(editor, editor.terrain.lastMutation.accepted);
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

cr::CreativeTerrainSculptPlan planCreativeEditorTerrainSculpt(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainCoord2 target) noexcept {
  return cr::buildCreativeTerrainSculptPlan(
      sculptRequest(document, editor, target));
}

CreativeEditorTerrainSculptReceipt applyCreativeEditorTerrainSculptWithHistory(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    std::string_view source) {
  CreativeEditorTerrainSculptReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainSculptAction::Apply;
  if (!resolveCreativeEditorTerrainPointerCoord(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_sculpt_target_invalid";
    setSculptFeedback(editor, false);
    return receipt;
  }
  receipt.plan = planCreativeEditorTerrainSculpt(
      appState.facade.document(), editor, receipt.targetCoord);
  receipt.accepted = receipt.plan.accepted;
  receipt.reasonCode = receipt.plan.reasonCode;
  if (!receipt.plan.accepted || receipt.plan.items().empty()) {
    setSculptFeedback(editor, receipt.plan.accepted);
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
    invalidateSculptPreview(editor.terrain.sculpt);
  }
  setSculptFeedback(editor, receipt.accepted);
  return receipt;
}

CreativeEditorTerrainSculptReceipt sampleCreativeEditorTerrainSculptHeight(
    const cr::CreativeDocument& document,
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainSculptReceipt receipt;
  receipt.requested = true;
  receipt.action = CreativeEditorTerrainSculptAction::SampleHeight;
  if (!resolveCreativeEditorTerrainPointerCoord(editor, receipt.targetCoord)) {
    receipt.reasonCode = "creative_editor_terrain_sculpt_sample_target_invalid";
    setSculptFeedback(editor, false);
    return receipt;
  }
  const cr::CreativeTerrainHeightSample sample =
      cr::sampleCreativeTerrainHeight(document.terrainField(),
                                      receipt.targetCoord);
  if (!sample.present) {
    receipt.reasonCode = "creative_editor_terrain_sculpt_sample_missing";
    setSculptFeedback(editor, false);
    return receipt;
  }
  receipt.accepted = true;
  receipt.changed =
      editor.terrain.sculpt.targetHeightCells != sample.heightCells;
  editor.terrain.sculpt.targetHeightCells = sample.heightCells;
  receipt.reasonCode = "creative_editor_terrain_sculpt_height_sampled";
  invalidateSculptPreview(editor.terrain.sculpt);
  setSculptFeedback(editor, true);
  return receipt;
}

CreativeEditorTerrainSculptReceipt cancelCreativeEditorTerrainSculpt(
    CreativeEditorState& editor) noexcept {
  CreativeEditorTerrainSculptReceipt receipt;
  receipt.requested = true;
  receipt.accepted = true;
  receipt.action = CreativeEditorTerrainSculptAction::Cancel;
  receipt.changed = editor.terrain.sculpt.preview.valid;
  receipt.reasonCode = "creative_editor_terrain_sculpt_cancelled";
  invalidateSculptPreview(editor.terrain.sculpt);
  editor.interaction.placementFeedback = {};
  return receipt;
}

bool processCreativeEditorTerrainSculptQuickEdit(
    CreativeEditorState& editor,
    cr::CreativeInputActionId action) noexcept {
  bool changed = false;
  switch (action) {
    case cr::CreativeInputActionId::QuickEditPrevious:
      changed = stepClampedEnum(
          editor.toolSettings.terrainSculptStrength,
          cr::CreativeTerrainSculptStrength::Count, 1);
      break;
    case cr::CreativeInputActionId::QuickEditNext:
      changed = stepClampedEnum(
          editor.toolSettings.terrainSculptStrength,
          cr::CreativeTerrainSculptStrength::Count, -1);
      break;
    case cr::CreativeInputActionId::QuickEditDecrease:
      changed = stepClampedEnum(editor.toolSettings.terrainSculptRadius,
                                cr::CreativeTerrainSculptRadius::Count, -1);
      break;
    case cr::CreativeInputActionId::QuickEditIncrease:
      changed = stepClampedEnum(editor.toolSettings.terrainSculptRadius,
                                cr::CreativeTerrainSculptRadius::Count, 1);
      break;
    default:
      return false;
  }
  if (changed) {
    invalidateSculptPreview(editor.terrain.sculpt);
  }
  return changed;
}

std::string creativeEditorTerrainSculptQuickEditLabel(
    const CreativeEditorState& editor) {
  std::string label(cr::toString(editor.toolSettings.terrainSculptMode));
  label.append(" | RADIUS ");
  label.append(std::to_string(cr::creativeTerrainSculptRadiusCells(
      editor.toolSettings.terrainSculptRadius)));
  label.append(" | STRENGTH ");
  label.append(std::to_string(cr::creativeTerrainSculptStrengthCells(
      editor.toolSettings.terrainSculptStrength)));
  if (cr::creativeTerrainSculptUsesTargetHeight(
          editor.toolSettings.terrainSculptMode)) {
    label.append(" | TARGET ");
    label.append(std::to_string(editor.terrain.sculpt.targetHeightCells));
  }
  return label;
}

void finalizeCreativeTerrainSculptStroke(cr::CreativeAppState& appState,
                                         CreativeEditorState& editor,
                                         std::string_view reasonCode) {
  CreativeTerrainSculptStrokeState& stroke = editor.terrain.sculpt.stroke;
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

void processCreativeTerrainSculptStrokeFrame(
    cr::CreativeAppState& appState,
    CreativeEditorState& editor,
    const cr::CreativeWorldActionFrame& actions,
    std::uint64_t monotonicTimeNanoseconds) {
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (held.kind != cr::CreativeHeldItemKind::TerrainSculpt) {
    finalizeCreativeTerrainSculptStroke(
        appState, editor, "creative_terrain_sculpt_non_sculpt_tool");
    return;
  }

  const bool cancelPressed =
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Primary) ||
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Reject);
  if (cancelPressed) {
    finalizeCreativeTerrainSculptStroke(
        appState, editor, "creative_terrain_sculpt_cancelled");
    static_cast<void>(cancelCreativeEditorTerrainSculpt(editor));
    return;
  }

  cr::CreativeMaterialRepeatRequest request;
  request.nowNanoseconds = monotonicTimeNanoseconds;
  request.secondaryPressed =
      cr::creativeWorldActionPressed(actions,
                                     cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionPressed(actions, cr::CreativeWorldActionId::Accept);
  request.secondaryDown =
      cr::creativeWorldActionDown(actions,
                                  cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionDown(actions, cr::CreativeWorldActionId::Accept);
  request.secondaryReleased =
      cr::creativeWorldActionReleased(actions,
                                      cr::CreativeWorldActionId::Secondary) ||
      cr::creativeWorldActionReleased(actions,
                                      cr::CreativeWorldActionId::Accept);

  CreativeTerrainSculptStrokeState& stroke = editor.terrain.sculpt.stroke;
  const cr::CreativeMaterialRepeatResult repeat =
      cr::stepCreativeMaterialRepeat(stroke.repeat, request);
  stroke.repeat = repeat.next;
  if (repeat.finalized) {
    finalizeCreativeTerrainSculptStroke(
        appState, editor, "creative_terrain_sculpt_released");
    return;
  }
  if (repeat.mutationDue &&
      repeat.dueKind == cr::CreativeMaterialStrokeKind::Place) {
    applySculptStrokeMutation(appState, editor);
  }
}

bool refreshCreativeEditorTerrainSculptPreview(
    CreativeEditorTerrainState& state,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) {
  cr::CreativeTerrainCoord2 center{};
  if (!resolveCreativeEditorTerrainPointerCoord(editor, center)) {
    invalidateSculptPreview(state.sculpt);
    return false;
  }
  CreativeTerrainSculptPreviewCache& cache = state.sculpt.preview;
  if (previewKeyMatches(cache, document, editor, center)) {
    return false;
  }

  const std::uint64_t nextBuildCount = cache.buildCount + 1U;
  cache = {};
  cache.valid = true;
  cache.documentId = document.id();
  cache.documentRevision = document.revision();
  cache.buildCount = nextBuildCount;
  cache.center = center;
  cache.mode = editor.toolSettings.terrainSculptMode;
  cache.radiusCells = cr::creativeTerrainSculptRadiusCells(
      editor.toolSettings.terrainSculptRadius);
  cache.strengthCells = cr::creativeTerrainSculptStrengthCells(
      editor.toolSettings.terrainSculptStrength);
  cache.targetHeightCells = state.sculpt.targetHeightCells;
  cache.plan = planCreativeEditorTerrainSculpt(document, editor, center);
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
    if (insideBrush(center, patch.coord, cache.radiusCells)) {
      cache.patches.push_back(patch);
    }
  }
  cache.renderAccepted = true;
  return true;
}

void appendCreativeEditorTerrainSculptOverlay(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines) {
  const CreativeTerrainSculptPreviewCache& preview =
      editor.terrain.sculpt.preview;
  if (!preview.valid) {
    return;
  }
  constexpr iggy3d::RenderLineColor admitted{0.20F, 1.0F, 0.35F, 1.0F};
  constexpr iggy3d::RenderLineColor rejected{1.0F, 0.20F, 0.18F, 1.0F};
  const iggy3d::RenderLineColor outlineColor =
      preview.plan.accepted && preview.renderAccepted ? admitted : rejected;
  appendCreativeEditorTerrainFootprintOutline(
      wireLines, document.gridSettings(),
      {preview.center, preview.targetHeightCells, preview.radiusCells},
      outlineColor, wireThickness * 1.25F);
  if (!preview.renderAccepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendPatchSlopeTriangles(wireLines, patch, wireThickness * 0.75F);
  }
}

}  // namespace iggy3d_creative_app
