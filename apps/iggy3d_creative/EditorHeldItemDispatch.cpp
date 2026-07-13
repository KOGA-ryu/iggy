#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include <array>

#include "EditorState.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainPaint.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/input/HeldItemRegistry.hpp"

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;
namespace {

void refreshHeldItemPreview(
    const CreativeEditorWorldInteractionFrameRequest& request,
    cr::CreativeHeldItemKind kind);

[[nodiscard]] bool processExclusiveHeldItemFrame(
    const CreativeEditorWorldInteractionFrameRequest& request,
    const cr::CreativeHotbarEntry& held) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  const bool assetScatter =
      creativeEditorUsesAssetScatter(held, editor.toolSettings);
  if (!assetScatter) {
    finalizeCreativeAssetScatterStroke(
        request.appState, editor,
        "creative_asset_scatter_non_scatter_tool");
  }
  if (definition.frameMode != cr::CreativeHeldItemFrameMode::TerrainPaint) {
    finalizeCreativeEditorTerrainPaintStroke(
        request.appState, editor, "creative_terrain_paint_non_paint_tool");
  }
  switch (definition.frameMode) {
    case cr::CreativeHeldItemFrameMode::MaterialStroke: {
      finalizeCreativeTerrainStroke(request.appState, editor,
                                    "creative_terrain_stroke_material_tool");
      if (assetScatter) {
        finalizeCreativeMaterialStroke(
            request.appState, editor,
            "creative_material_stroke_asset_scatter");
        processCreativeAssetScatterFrame(
            request.appState, editor, request.actions,
            request.monotonicTimeNanoseconds);
      } else {
        processCreativeMaterialStrokeFrame(
            request.appState, editor, request.actions,
            request.monotonicTimeNanoseconds);
      }
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTargetMaterial,
            request, held);
      }
      return true;
    }
    case cr::CreativeHeldItemFrameMode::TerrainControlStroke: {
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_non_material_tool");
      finalizeCreativeTerrainSculptStroke(
          request.appState, editor, "creative_terrain_sculpt_non_sculpt_tool");
      processCreativeTerrainStrokeFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (!editor.terrain.stroke.repeat.active &&
          cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTerrainControl,
            request, held);
      }
      return true;
    }
    case cr::CreativeHeldItemFrameMode::TerrainPaint:
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_terrain_paint_tool");
      finalizeCreativeTerrainStroke(
          request.appState, editor,
          "creative_terrain_stroke_terrain_paint_tool");
      finalizeCreativeTerrainSculptStroke(
          request.appState, editor,
          "creative_terrain_sculpt_terrain_paint_tool");
      processCreativeEditorTerrainPaintFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (!editor.terrainPaint.repeat.active &&
          cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        static_cast<void>(sampleCreativeEditorTerrainMaterial(
            request.appState.facade.document(), editor));
      }
      return true;
    case cr::CreativeHeldItemFrameMode::TerrainSculpt:
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_non_material_tool");
      finalizeCreativeTerrainStroke(
          request.appState, editor,
          "creative_terrain_stroke_non_terrain_tool");
      processCreativeTerrainSculptStrokeFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds);
      if (!editor.terrain.sculpt.stroke.repeat.active &&
          cr::creativeTerrainSculptUsesTargetHeight(
              editor.toolSettings.terrainSculptMode) &&
          cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        static_cast<void>(sampleCreativeEditorTerrainSculptHeight(
            request.appState.facade.document(), editor));
      }
      static_cast<void>(refreshCreativeEditorTerrainSculptPreview(
          editor.terrain, request.appState.facade.document(), editor));
      return true;
    case cr::CreativeHeldItemFrameMode::ObjectMove: {
      finalizeCreativeMaterialStroke(
          request.appState, editor,
          "creative_material_stroke_non_material_tool");
      finalizeCreativeTerrainStroke(
          request.appState, editor,
          "creative_terrain_stroke_non_terrain_tool");
      finalizeCreativeTerrainSculptStroke(
          request.appState, editor,
          "creative_terrain_sculpt_non_sculpt_tool");
      processCreativeEditorMoveInteraction(request);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
          dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTargetMaterial,
            request, held);
      }
      return true;
    }
    case cr::CreativeHeldItemFrameMode::Standard:
    case cr::CreativeHeldItemFrameMode::Count:
      break;
  }
  finalizeCreativeMaterialStroke(
      request.appState, editor, "creative_material_stroke_non_material_tool");
  finalizeCreativeTerrainStroke(
      request.appState, editor, "creative_terrain_stroke_non_terrain_tool");
  finalizeCreativeTerrainSculptStroke(
      request.appState, editor, "creative_terrain_sculpt_non_sculpt_tool");
  refreshHeldItemPreview(request, held.kind);
  return false;
}

void refreshHeldItemPreview(
    const CreativeEditorWorldInteractionFrameRequest& request,
    cr::CreativeHeldItemKind kind) {
  switch (cr::describeCreativeHeldItem(kind).previewMode) {
    case cr::CreativeHeldItemPreviewMode::TerrainProfile:
      static_cast<void>(refreshCreativeEditorTerrainProfilePreview(
          request.editor.terrain, request.appState.facade.document(),
          request.editor));
      return;
    case cr::CreativeHeldItemPreviewMode::TerrainPath:
      static_cast<void>(refreshCreativeEditorTerrainPathPreview(
          request.editor.terrain, request.appState.facade.document(),
          request.editor));
      return;
    case cr::CreativeHeldItemPreviewMode::TerrainRegion:
      if (request.editor.terrain.region.stamp.active) {
        static_cast<void>(refreshCreativeEditorTerrainStampPreview(
            request.editor.terrain, request.appState.facade.document(),
            request.editor, request.appState.terrainStamp));
      } else {
        static_cast<void>(refreshCreativeEditorTerrainRegionPreview(
            request.editor.terrain, request.appState.facade.document(),
            request.editor));
      }
      return;
    case cr::CreativeHeldItemPreviewMode::None:
    case cr::CreativeHeldItemPreviewMode::Count:
      return;
  }
}

}  // namespace

void processCreativeEditorHeldItemFrame(
    const CreativeEditorWorldInteractionFrameRequest& request) {
  CreativeEditorState& editor = request.editor;
  const cr::CreativeHotbarEntry& held =
      cr::selectedCreativeHotbarEntry(editor.interaction.hotbar);
  if (processExclusiveHeldItemFrame(request, held)) {
    return;
  }

  const cr::CreativeHeldItemDefinition& definition =
      cr::describeCreativeHeldItem(held.kind);
  constexpr std::array actions{
      cr::CreativeWorldActionId::Primary,
      cr::CreativeWorldActionId::Secondary,
      cr::CreativeWorldActionId::Pick,
  };
  const bool rejectPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Reject);
  const bool acceptPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Accept);
  if (rejectPressed) {
    dispatchCreativeEditorHeldItemWorldOperation(
        definition.rejectOperation, request, held);
  } else if (acceptPressed) {
    dispatchCreativeEditorHeldItemWorldOperation(
        definition.acceptOperation, request, held);
  }
  const bool primaryPressed = cr::creativeWorldActionPressed(
      request.actions, cr::CreativeWorldActionId::Primary);
  for (std::size_t index = 0; index < actions.size(); ++index) {
    if (definition.primaryWinsSimultaneous && index == 1U && primaryPressed) {
      continue;
    }
    if (cr::creativeWorldActionPressed(request.actions, actions[index])) {
      dispatchCreativeEditorHeldItemWorldOperation(
          definition.worldOperations[index], request, held);
    }
  }
}

}  // namespace iggy3d_creative_app
