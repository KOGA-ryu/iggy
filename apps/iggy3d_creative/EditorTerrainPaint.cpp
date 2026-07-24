#include "EditorTerrainPaint.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "EditorEdits.hpp"
#include "EditorInteraction.hpp"
#include "EditorState.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/input/WorldActionIntent.hpp"

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

void setFeedback(CreativeEditorState& editor, bool accepted) {
  setCreativeEditorPlacementFeedback(
      editor.interaction,
      accepted ? CreativeEditorPlacementFeedbackStatus::Placed
               : CreativeEditorPlacementFeedbackStatus::Rejected,
      editor.frameIndex);
}

[[nodiscard]] cr::CreativeTerrainPaintPlan terrainPaintPlan(
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor,
    cr::CreativeTerrainMaterial material) {
  const cr::CreativeTerrainSurfacePlan surface =
      cr::buildCreativeTerrainSurfacePlan(document.terrainField());
  cr::CreativeTerrainCoord2 target{};
  const bool completedRegion =
      editor.toolSettings.terrainPaintMode ==
          cr::CreativeTerrainPaintMode::Region &&
      editor.terrainPaint.regionPhase ==
          CreativeEditorTerrainPaintRegionPhase::Complete;
  if (!surface.accepted ||
      (!completedRegion && !terrainPaintTarget(editor, target))) {
    return {};
  }
  if (completedRegion) {
    target = editor.terrainPaint.secondCorner;
  }
  cr::CreativeTerrainPaintRequest request;
  request.surfaceColumns = surface.columns;
  request.materialField = &document.terrainMaterialField();
  request.mode = editor.toolSettings.terrainPaintMode;
  request.center = target;
  request.material = material;
  request.source = editor.toolSettings.terrainPaintSource;
  request.radiusCells = cr::creativeTerrainPaintRadiusCells(
      editor.toolSettings.terrainPaintRadius);
  request.hardness = editor.toolSettings.terrainPaintHardness;
  request.opacity = editor.toolSettings.terrainPaintOpacity;
  request.mask = editor.toolSettings.terrainPaintMask;
  request.blend = editor.toolSettings.terrainPaintBlend;
  request.slopeFilter = editor.toolSettings.terrainPaintSlopeFilter;
  request.heightFilter = editor.toolSettings.terrainPaintHeightFilter;
  if (request.mode == cr::CreativeTerrainPaintMode::Region) {
    const CreativeEditorTerrainPaintState& state = editor.terrainPaint;
    const cr::CreativeTerrainCoord2 first =
        state.regionPhase == CreativeEditorTerrainPaintRegionPhase::Empty
            ? target
            : state.firstCorner;
    const cr::CreativeTerrainCoord2 second =
        state.regionPhase == CreativeEditorTerrainPaintRegionPhase::Complete
            ? state.secondCorner
            : target;
    request.minimumCoord = {std::min(first.x, second.x),
                            std::min(first.z, second.z)};
    request.maximumCoord = {std::max(first.x, second.x),
                            std::max(first.z, second.z)};
  }
  return cr::buildCreativeTerrainPaintPlan(request);
}

[[nodiscard]] cr::CreativeTerrainMaterial previewMaterial(
    const CreativeEditorState& editor) noexcept {
  return editor.toolSettings.terrainPaintMode ==
                 cr::CreativeTerrainPaintMode::Brush &&
             editor.terrainPaint.repeat.active &&
             editor.terrainPaint.repeat.kind ==
                 cr::CreativeMaterialStrokeKind::Remove
         ? cr::CreativeTerrainMaterial::Grass
         : editor.toolSettings.terrainPaintMaterial;
}

[[nodiscard]] bool previewCacheMatches(
    const CreativeEditorTerrainPaintPreviewCache& cache,
    const cr::CreativeDocument& document,
    const CreativeEditorState& editor) noexcept {
  cr::CreativeTerrainCoord2 target{};
  bool targetValid = terrainPaintTarget(editor, target);
  if (editor.toolSettings.terrainPaintMode ==
          cr::CreativeTerrainPaintMode::Region &&
      editor.terrainPaint.regionPhase ==
          CreativeEditorTerrainPaintRegionPhase::Complete) {
    target = editor.terrainPaint.secondCorner;
    targetValid = true;
  }
  const cr::CreativeGridSettings grid = document.gridSettings();
  return cache.valid && cache.targetValid == targetValid &&
         cache.documentId == document.id() &&
         cache.terrainRevision == document.terrainField().revision() &&
         cache.materialRevision ==
             document.terrainMaterialField().revision() &&
         cr::creativeVec3ExactlyEqual(cache.gridOrigin, grid.origin) &&
         cache.gridCellSizeMeters == grid.cellSizeMeters &&
         cache.mode == editor.toolSettings.terrainPaintMode &&
         cache.material == previewMaterial(editor) &&
         cache.source == editor.toolSettings.terrainPaintSource &&
         cache.radius == editor.toolSettings.terrainPaintRadius &&
         cache.hardness == editor.toolSettings.terrainPaintHardness &&
         cache.opacity == editor.toolSettings.terrainPaintOpacity &&
         cache.mask == editor.toolSettings.terrainPaintMask &&
         cache.blend == editor.toolSettings.terrainPaintBlend &&
         cache.slopeFilter == editor.toolSettings.terrainPaintSlopeFilter &&
         cache.heightFilter == editor.toolSettings.terrainPaintHeightFilter &&
         cache.regionPhase == editor.terrainPaint.regionPhase &&
         cache.target == target &&
         cache.firstCorner == editor.terrainPaint.firstCorner &&
         cache.secondCorner == editor.terrainPaint.secondCorner;
}

[[nodiscard]] std::vector<CreativeEditorTerrainPaintPreviewEdge>
buildPreviewEdges(const cr::CreativeDocument& document,
                  const cr::CreativeTerrainPaintPlan& plan) {
  std::vector<CreativeEditorTerrainPaintPreviewEdge> edges;
  if (plan.previews().size() != plan.cells().size()) {
    return edges;
  }
  edges.reserve(plan.cells().size() * 4U);
  const cr::CreativeGridSettings grid = document.gridSettings();
  for (std::size_t cellIndex = 0U; cellIndex < plan.cells().size(); ++cellIndex) {
    const cr::CreativeTerrainCoord2 coord = plan.cells()[cellIndex];
    const cr::CreativeTerrainHeightSample height =
        cr::sampleCreativeTerrainHeight(document.terrainField(), coord);
    if (!height.present) {
      continue;
    }
    const float cellSize = static_cast<float>(grid.cellSizeMeters);
    const float inset = cellSize * 0.08F;
    const float minX = static_cast<float>(
                           grid.origin.x + coord.x * grid.cellSizeMeters) +
                       inset;
    const float minZ = static_cast<float>(
                           grid.origin.z + coord.z * grid.cellSizeMeters) +
                       inset;
    const float maxX = minX + cellSize - inset * 2.0F;
    const float maxZ = minZ + cellSize - inset * 2.0F;
    const float y = static_cast<float>(
        grid.origin.y + height.heightCells * grid.cellSizeMeters + 0.03);
    const std::array<iggy3d::Vec3, 4U> corners{{
        {minX, y, minZ},
        {maxX, y, minZ},
        {maxX, y, maxZ},
        {minX, y, maxZ},
    }};
    const cr::CreativeVec3 layerColor =
        cr::creativeTerrainMaterialRenderColor(
            plan.previews()[cellIndex].afterWeights);
    for (std::size_t edge = 0U; edge < corners.size(); ++edge) {
      edges.push_back({corners[edge], corners[(edge + 1U) % corners.size()],
                       {static_cast<float>(layerColor.x),
                        static_cast<float>(layerColor.y),
                        static_cast<float>(layerColor.z), 1.0F}});
    }
  }
  return edges;
}

void refreshPreviewCache(const cr::CreativeDocument& document,
                         CreativeEditorState& editor) {
  CreativeEditorTerrainPaintPreviewCache& cache = editor.terrainPaint.preview;
  if (previewCacheMatches(cache, document, editor)) {
    return;
  }
  cr::CreativeTerrainCoord2 target{};
  bool targetValid = terrainPaintTarget(editor, target);
  if (editor.toolSettings.terrainPaintMode ==
          cr::CreativeTerrainPaintMode::Region &&
      editor.terrainPaint.regionPhase ==
          CreativeEditorTerrainPaintRegionPhase::Complete) {
    target = editor.terrainPaint.secondCorner;
    targetValid = true;
  }
  cache.valid = true;
  cache.targetValid = targetValid;
  cache.documentId = document.id();
  cache.terrainRevision = document.terrainField().revision();
  cache.materialRevision = document.terrainMaterialField().revision();
  cache.gridOrigin = document.gridSettings().origin;
  cache.gridCellSizeMeters = document.gridSettings().cellSizeMeters;
  cache.mode = editor.toolSettings.terrainPaintMode;
  cache.material = previewMaterial(editor);
  cache.source = editor.toolSettings.terrainPaintSource;
  cache.radius = editor.toolSettings.terrainPaintRadius;
  cache.hardness = editor.toolSettings.terrainPaintHardness;
  cache.opacity = editor.toolSettings.terrainPaintOpacity;
  cache.mask = editor.toolSettings.terrainPaintMask;
  cache.blend = editor.toolSettings.terrainPaintBlend;
  cache.slopeFilter = editor.toolSettings.terrainPaintSlopeFilter;
  cache.heightFilter = editor.toolSettings.terrainPaintHeightFilter;
  cache.regionPhase = editor.terrainPaint.regionPhase;
  cache.target = target;
  cache.firstCorner = editor.terrainPaint.firstCorner;
  cache.secondCorner = editor.terrainPaint.secondCorner;
  cache.plan = targetValid ? terrainPaintPlan(document, editor, cache.material)
                           : cr::CreativeTerrainPaintPlan{};
  cache.edges = buildPreviewEdges(document, cache.plan);
  ++cache.buildCount;
}

void applyContinuousBrush(cr::CreativeAppState& appState,
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
        appState.facade, "creative_terrain_paint_brush_stroke");
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

[[nodiscard]] bool applyOneShotPlan(cr::CreativeAppState& appState,
                                    CreativeEditorState& editor,
                                    cr::CreativeTerrainMaterial material,
                                    std::string_view source) {
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  state.lastPlan = terrainPaintPlan(appState.facade.document(), editor, material);
  if (!state.lastPlan.accepted) {
    setFeedback(editor, false);
    return false;
  }
  if (state.lastPlan.status == cr::CreativeTerrainPaintPlanStatus::NoChange) {
    setFeedback(editor, true);
    return true;
  }
  cr::CreativeDocumentHistoryTransaction transaction =
      beginEditTransaction(appState.facade, source);
  if (!transaction.active) {
    setFeedback(editor, false);
    return false;
  }
  state.lastMutation =
      appState.facade.applyTerrainMaterialEdits(state.lastPlan.items());
  const bool changed =
      state.lastMutation.accepted && state.lastMutation.changed;
  static_cast<void>(completeEditTransaction(
      appState.history, std::move(transaction), appState.facade, changed,
      state.lastMutation.reasonCode));
  setFeedback(editor, state.lastMutation.accepted);
  return state.lastMutation.accepted;
}

void processConnectedPaint(cr::CreativeAppState& appState,
                           CreativeEditorState& editor,
                           const cr::CreativeWorldActionFrame& actions) {
  const cr::CreativeWorldIntentFrame intents = cr::resolveCreativeWorldIntents(
      actions, cr::CreativeWorldIntentPolicy::Placement);
  const bool restore = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Negative);
  const bool paint = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Positive);
  if (!restore && !paint) {
    return;
  }
  const cr::CreativeTerrainMaterial material =
      restore ? cr::CreativeTerrainMaterial::Grass
              : editor.toolSettings.terrainPaintMaterial;
  static_cast<void>(applyOneShotPlan(
      appState, editor, material,
      restore ? "creative_terrain_connected_restore"
              : "creative_terrain_connected_paint"));
}

void processRegionPaint(cr::CreativeAppState& appState,
                        CreativeEditorState& editor,
                        const cr::CreativeWorldActionFrame& actions) {
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  const cr::CreativeWorldIntentFrame intents = cr::resolveCreativeWorldIntents(
      actions, cr::CreativeWorldIntentPolicy::Placement);
  const bool back = cr::creativeWorldIntentPressed(
      intents, cr::CreativeWorldIntentId::Negative);
  if (back) {
    switch (state.regionPhase) {
      case CreativeEditorTerrainPaintRegionPhase::Empty:
        return;
      case CreativeEditorTerrainPaintRegionPhase::FirstCorner:
        state.regionPhase = CreativeEditorTerrainPaintRegionPhase::Empty;
        break;
      case CreativeEditorTerrainPaintRegionPhase::Complete:
        state.regionPhase = CreativeEditorTerrainPaintRegionPhase::FirstCorner;
        break;
    }
    clearCreativeEditorPlacementFeedback(editor.interaction);
    return;
  }
  if (!cr::creativeWorldIntentPressed(
          intents, cr::CreativeWorldIntentId::Positive)) {
    return;
  }
  cr::CreativeTerrainCoord2 target{};
  if (state.regionPhase != CreativeEditorTerrainPaintRegionPhase::Complete &&
      !terrainPaintTarget(editor, target)) {
    setFeedback(editor, false);
    return;
  }
  switch (state.regionPhase) {
    case CreativeEditorTerrainPaintRegionPhase::Empty:
      state.firstCorner = target;
      state.regionPhase = CreativeEditorTerrainPaintRegionPhase::FirstCorner;
      setFeedback(editor, true);
      return;
    case CreativeEditorTerrainPaintRegionPhase::FirstCorner:
      state.secondCorner = target;
      state.regionPhase = CreativeEditorTerrainPaintRegionPhase::Complete;
      setFeedback(editor, true);
      return;
    case CreativeEditorTerrainPaintRegionPhase::Complete:
      if (applyOneShotPlan(appState, editor,
                           editor.toolSettings.terrainPaintMaterial,
                           "creative_terrain_region_paint")) {
        state.regionPhase = CreativeEditorTerrainPaintRegionPhase::Empty;
      }
      return;
  }
}

void synchronizePaintMode(cr::CreativeAppState& appState,
                          CreativeEditorState& editor) {
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  const cr::CreativeTerrainPaintMode requested =
      editor.toolSettings.terrainPaintMode;
  if (state.modeInitialized && state.activeMode == requested) {
    return;
  }
  if (state.modeInitialized) {
    finalizeCreativeEditorTerrainPaintStroke(
        appState, editor, "creative_terrain_paint_mode_changed");
  }
  editor.terrainPaint.modeInitialized = true;
  editor.terrainPaint.activeMode = requested;
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
  synchronizePaintMode(appState, editor);
  CreativeEditorTerrainPaintState& state = editor.terrainPaint;
  switch (editor.toolSettings.terrainPaintMode) {
    case cr::CreativeTerrainPaintMode::Brush: {
      const cr::CreativeMaterialRepeatResult repeat =
          cr::stepCreativeMaterialRepeat(
              state.repeat,
              cr::makeCreativeWorldStrokeRepeatRequest(
                  actions, monotonicTimeNanoseconds));
      state.repeat = repeat.next;
      if (repeat.finalized) {
        finalizeCreativeEditorTerrainPaintStroke(
            appState, editor, "creative_terrain_paint_stroke_released");
      } else if (repeat.mutationDue) {
        applyContinuousBrush(appState, editor, repeat.dueKind);
      }
      break;
    }
    case cr::CreativeTerrainPaintMode::Connected:
      processConnectedPaint(appState, editor, actions);
      break;
    case cr::CreativeTerrainPaintMode::Region:
      processRegionPaint(appState, editor, actions);
      break;
    case cr::CreativeTerrainPaintMode::Count:
      editor.terrainPaint.preview = {};
      break;
  }
  refreshPreviewCache(appState.facade.document(), editor);
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
  const cr::CreativeTerrainMaterial material = previewMaterial(editor);
  const bool cached =
      previewCacheMatches(editor.terrainPaint.preview, document, editor);
  const cr::CreativeTerrainPaintPlan fallback =
      cached ? cr::CreativeTerrainPaintPlan{}
             : terrainPaintPlan(document, editor, material);
  const cr::CreativeTerrainPaintPlan& plan =
      cached ? editor.terrainPaint.preview.plan : fallback;
  if (!plan.accepted) {
    return;
  }
  const std::vector<CreativeEditorTerrainPaintPreviewEdge> fallbackEdges =
      cached ? std::vector<CreativeEditorTerrainPaintPreviewEdge>{}
             : buildPreviewEdges(document, plan);
  const std::vector<CreativeEditorTerrainPaintPreviewEdge>& edges =
      cached ? editor.terrainPaint.preview.edges : fallbackEdges;
  for (const CreativeEditorTerrainPaintPreviewEdge& edge : edges) {
    appendLine(lines, edge.start, edge.end, edge.color, thickness);
  }
}

}  // namespace iggy3d_creative_app
