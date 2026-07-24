#include "EditorTerrain.hpp"

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/input/WorldActionIntent.hpp"

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
          editor.toolSettings.terrainSculptTargetHeightCells,
          editor.toolSettings.terrainSculptFalloff,
          editor.toolSettings.terrainSculptMask};
}

void setSculptFeedback(CreativeEditorState& editor, bool accepted) noexcept {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

void invalidateSculptPreview(CreativeTerrainSculptState& sculpt) noexcept {
  sculpt.preview.valid = false;
  sculpt.preview.renderAccepted = false;
  sculpt.preview.patches.clear();
  sculpt.preview.contours = {};
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
         cache.falloff == editor.toolSettings.terrainSculptFalloff &&
         cache.mask == editor.toolSettings.terrainSculptMask &&
         cache.radiusCells == cr::creativeTerrainSculptRadiusCells(
                                  editor.toolSettings.terrainSculptRadius) &&
         cache.strengthCells == cr::creativeTerrainSculptStrengthCells(
                                    editor.toolSettings.terrainSculptStrength) &&
         cache.contourIntervalCells == editor.terrain.contours.intervalCells &&
         cache.contourMajorEvery == editor.terrain.contours.majorEvery &&
         (!cr::creativeTerrainSculptUsesTargetHeight(cache.mode) ||
          cache.targetHeightCells ==
              editor.toolSettings.terrainSculptTargetHeightCells);
}

[[nodiscard]] cr::CreativeTerrainContourPlan buildSculptContourPlan(
    const cr::CreativeTerrainField& source,
    const cr::CreativeTerrainSculptPlan& sculpt,
    std::uint16_t intervalCells,
    std::uint16_t majorEvery) {
  if (!sculpt.dirtyRegion.valid || sculpt.items().empty()) {
    return {};
  }
  cr::CreativeTerrainField preview = source;
  const cr::CreativeTerrainMutationReceipt mutation = preview.apply(
      sculpt.items());
  if (!mutation.accepted) {
    return {};
  }

  cr::CreativeTerrainSurfacePlan surface;
  surface.requested = true;
  surface.sourceRevision = preview.revision();
  const cr::CreativeTerrainCoord2 minimum = sculpt.dirtyRegion.minimum;
  const cr::CreativeTerrainCoord2 maximum = sculpt.dirtyRegion.maximum;
  surface.columns.reserve(static_cast<std::size_t>(
      sculpt.dirtyRegion.candidatePatchCount));
  for (std::int64_t z = minimum.z; z <= maximum.z; ++z) {
    for (std::int64_t x = minimum.x; x <= maximum.x; ++x) {
      const cr::CreativeTerrainHeightSample sample =
          cr::sampleCreativeTerrainHeight(
              preview, {static_cast<std::int32_t>(x),
                        static_cast<std::int32_t>(z)});
      if (!sample.present) {
        continue;
      }
      surface.contributionCount += sample.contributingControlCount;
      surface.columns.push_back({sample.coord, sample.heightCells});
    }
  }
  surface.accepted = true;
  surface.status = surface.columns.empty()
                       ? cr::CreativeTerrainSurfacePlanStatus::Empty
                       : cr::CreativeTerrainSurfacePlanStatus::Ready;
  surface.reasonCode = surface.columns.empty()
                           ? "creative_terrain_sculpt_contours_empty"
                           : "creative_terrain_sculpt_contours_ready";
  return cr::buildCreativeTerrainContourPlan(
      surface, {intervalCells, majorEvery,
                cr::kCreativeTerrainContourSegmentCapacity});
}

void appendSculptSquareOutline(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& lines,
    cr::CreativeGridSettings grid,
    cr::CreativeTerrainCoord2 center,
    std::uint16_t radiusCells,
    iggy3d::RenderLineColor color,
    float thickness) {
  const double minimumX =
      grid.origin.x +
      static_cast<double>(static_cast<std::int64_t>(center.x) - radiusCells) *
          grid.cellSizeMeters;
  const double minimumZ =
      grid.origin.z +
      static_cast<double>(static_cast<std::int64_t>(center.z) - radiusCells) *
          grid.cellSizeMeters;
  const double edgeCells = static_cast<double>(radiusCells * 2U + 1U);
  const double maximumX = minimumX + edgeCells * grid.cellSizeMeters;
  const double maximumZ = minimumZ + edgeCells * grid.cellSizeMeters;
  const double y = grid.origin.y + grid.cellSizeMeters * 0.12;
  const std::array<cr::CreativeVec3, 4U> corners{{
      {minimumX, y, minimumZ},
      {maximumX, y, minimumZ},
      {maximumX, y, maximumZ},
      {minimumX, y, maximumZ},
  }};
  for (std::size_t index = 0U; index < corners.size(); ++index) {
    const cr::CreativeCoreVec3Conversion start =
        cr::creativeVec3ToCoreChecked(corners[index]);
    const cr::CreativeCoreVec3Conversion end =
        cr::creativeVec3ToCoreChecked(corners[(index + 1U) % corners.size()]);
    if (!start.converted || !end.converted) {
      continue;
    }
    iggy3d::RenderCreativeWireframeDebugLine line;
    line.start = start.value;
    line.end = end.value;
    line.color = color;
    line.thickness = thickness;
    lines.push_back(line);
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
      editor.toolSettings.terrainSculptTargetHeightCells !=
      sample.heightCells;
  editor.toolSettings.terrainSculptTargetHeightCells = sample.heightCells;
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
  clearCreativeEditorPlacementFeedback(editor.interaction);
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
  label.append(" | FALLOFF ");
  label.append(cr::toString(editor.toolSettings.terrainSculptFalloff));
  label.append(" | MASK ");
  label.append(cr::toString(editor.toolSettings.terrainSculptMask));
  if (cr::creativeTerrainSculptUsesTargetHeight(
          editor.toolSettings.terrainSculptMode)) {
    label.append(" | TARGET ");
    label.append(std::to_string(
        editor.toolSettings.terrainSculptTargetHeightCells));
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

  const cr::CreativeWorldIntentFrame intents = cr::resolveCreativeWorldIntents(
      actions, cr::CreativeWorldIntentPolicy::Placement);
  const bool cancelPressed = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Negative);
  if (cancelPressed) {
    finalizeCreativeTerrainSculptStroke(
        appState, editor, "creative_terrain_sculpt_cancelled");
    static_cast<void>(cancelCreativeEditorTerrainSculpt(editor));
    return;
  }

  const cr::CreativeMaterialRepeatRequest request =
      cr::makeCreativeWorldStrokeRepeatRequest(actions,
                                               monotonicTimeNanoseconds);

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
  cache.falloff = editor.toolSettings.terrainSculptFalloff;
  cache.mask = editor.toolSettings.terrainSculptMask;
  cache.radiusCells = cr::creativeTerrainSculptRadiusCells(
      editor.toolSettings.terrainSculptRadius);
  cache.strengthCells = cr::creativeTerrainSculptStrengthCells(
      editor.toolSettings.terrainSculptStrength);
  cache.targetHeightCells = editor.toolSettings.terrainSculptTargetHeightCells;
  cache.contourIntervalCells = editor.terrain.contours.intervalCells;
  cache.contourMajorEvery = editor.terrain.contours.majorEvery;
  cache.plan = planCreativeEditorTerrainSculpt(document, editor, center);
  if (!cache.plan.accepted || !cache.plan.dirtyRegion.valid ||
      cache.plan.items().empty()) {
    return true;
  }

  const cr::CreativeGridSettings grid = document.gridSettings();
  const cr::CreativeTerrainMutationPreviewReceipt preview =
      cr::buildCreativeTerrainMutationPreview(
          document.terrainField(), cache.plan.items(),
          {cache.plan.dirtyRegion.minimum, cache.plan.dirtyRegion.maximum},
          grid.origin, grid.cellSizeMeters);
  if (!preview.accepted) {
    return true;
  }
  cache.sampledColumnCoordinateCount =
      preview.sampledColumnCoordinateCount;
  cache.candidatePatchCoordinateCount =
      preview.candidatePatchCoordinateCount;
  cache.patches = preview.render.patches;
  cache.contours = buildSculptContourPlan(
      document.terrainField(), cache.plan, cache.contourIntervalCells,
      cache.contourMajorEvery);
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
  if (preview.mask == cr::CreativeTerrainSculptMask::Square) {
    appendSculptSquareOutline(wireLines, document.gridSettings(),
                              preview.center, preview.radiusCells,
                              outlineColor, wireThickness * 1.25F);
  } else {
    appendCreativeEditorTerrainFootprintOutline(
        wireLines, document.gridSettings(),
        {preview.center, preview.targetHeightCells, preview.radiusCells},
        outlineColor, wireThickness * 1.25F);
  }
  if (!preview.renderAccepted) {
    return;
  }
  for (const cr::CreativeTerrainSurfacePatch& patch : preview.patches) {
    appendCreativeEditorTerrainPatchSlopeTriangles(
        wireLines, patch, wireThickness * 0.75F);
  }
  static_cast<void>(appendCreativeEditorTerrainContourPlan(
      document, preview.contours, wireThickness * 0.85F, wireLines));
}

}  // namespace iggy3d_creative_app
