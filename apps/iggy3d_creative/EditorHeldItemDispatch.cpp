#include "EditorInteraction.hpp"
#include "EditorInteractionInternal.hpp"

#include "EditorState.hpp"
#include "EditorMeasurement.hpp"
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
  const CreativeEditorContinuousGestureOwner gestureOwner =
      creativeEditorContinuousGestureOwner(editor);
  finalizeCreativeEditorContinuousGesturesExcept(
      request.appState, editor, gestureOwner,
      "creative_continuous_gesture_owner_changed");
  switch (gestureOwner) {
    case CreativeEditorContinuousGestureOwner::Material:
      processCreativeMaterialStrokeFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds, request.assetCatalog,
          request.placementClearanceCache);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTargetMaterial,
            request, held);
      }
      return true;
    case CreativeEditorContinuousGestureOwner::AuthoredAsset:
      processCreativeAuthoredAssetFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds,
          request.placementClearanceCache);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTargetMaterial,
            request, held);
      }
      return true;
    case CreativeEditorContinuousGestureOwner::AssetScatter:
      processCreativeAssetScatterFrame(
          request.appState, editor, request.actions,
          request.monotonicTimeNanoseconds,
          request.placementClearanceCache);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTargetMaterial,
            request, held);
      }
      return true;
    case CreativeEditorContinuousGestureOwner::TerrainControl:
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
    case CreativeEditorContinuousGestureOwner::TerrainPaint:
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
    case CreativeEditorContinuousGestureOwner::TerrainSculpt:
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
    case CreativeEditorContinuousGestureOwner::None:
    case CreativeEditorContinuousGestureOwner::Count:
      break;
  }

  switch (definition.frameMode) {
    case cr::CreativeHeldItemFrameMode::ObjectMove:
      processCreativeEditorMoveInteraction(request);
      if (cr::creativeWorldActionPressed(request.actions,
                                         cr::CreativeWorldActionId::Pick)) {
        dispatchCreativeEditorHeldItemWorldOperation(
            cr::CreativeHeldItemWorldOperation::SampleTargetMaterial,
            request, held);
      }
      return true;
    case cr::CreativeHeldItemFrameMode::Standard:
      refreshHeldItemPreview(request, held.kind);
      return false;
    case cr::CreativeHeldItemFrameMode::MaterialStroke:
    case cr::CreativeHeldItemFrameMode::TerrainControlStroke:
    case cr::CreativeHeldItemFrameMode::TerrainPaint:
    case cr::CreativeHeldItemFrameMode::TerrainSculpt:
    case cr::CreativeHeldItemFrameMode::Count:
      return true;
  }
  return true;
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
    case cr::CreativeHeldItemPreviewMode::Measurement:
      static_cast<void>(previewCreativeEditorMeasurementPoint(request));
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
  const cr::CreativeHeldItemWorldOperationList operations =
      cr::resolveCreativeHeldItemWorldOperations(definition, request.actions);
  for (cr::CreativeHeldItemWorldOperation operation : operations.items()) {
    dispatchCreativeEditorHeldItemWorldOperation(operation, request, held);
  }
}

}  // namespace iggy3d_creative_app
