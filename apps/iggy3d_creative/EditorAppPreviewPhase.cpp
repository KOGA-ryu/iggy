#include "EditorAppFramePhases.hpp"

#include "EditorAssetReplacement.hpp"
#include "EditorTerrain.hpp"
#include "EditorTerrainGeneration.hpp"
#include "EditorVolume.hpp"
#include "EditorWorldLayoutLifecycle.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;
namespace {

void refreshVolumeScenePreview(
    const CreativeEditorAppPreviewPhaseRequest& request,
    const creative::CreativeDocument& renderDocument) {
  const creative::CreativeHotbarEntry& held =
      creative::selectedCreativeHotbarEntry(request.editor.interaction.hotbar);
  const bool fillPreview =
      held.kind == creative::CreativeHeldItemKind::VolumeFill;
  const bool clonePreview =
      held.kind == creative::CreativeHeldItemKind::VolumeClone;
  const bool eligible =
      &renderDocument == &request.activeAppState.facade.document() &&
      (fillPreview || clonePreview) && request.editor.volume.active &&
      !request.modalBlocksWorldActions && !request.captureMode;
  if (!eligible) {
    invalidateCreativeEditorVolumeScenePreview(request.volumePreviewCache);
    return;
  }
  request.editor.volume.operation =
      fillPreview ? creative::CreativeVolumeOperationKind::Fill
                  : creative::CreativeVolumeOperationKind::Clone;
  const creative::CreativeVolumeSelection selection =
      creativeEditorVolumePreviewSelection(request.editor.volume);
  if (!creative::creativeVolumeSelectionValid(selection)) {
    invalidateCreativeEditorVolumeScenePreview(request.volumePreviewCache);
    return;
  }
  static_cast<void>(refreshCreativeEditorVolumeOperationPreview(
      request.editor.volume, renderDocument, selection,
      request.editor.placeBrush, request.editor.toolSettings));
  if (!request.editor.volume.preview.stagedDocumentValid) {
    invalidateCreativeEditorVolumeScenePreview(request.volumePreviewCache);
    return;
  }
  static_cast<void>(refreshCreativeEditorVolumeScenePreview(
      request.volumePreviewCache,
      request.editor.volume.preview.stagedDocument,
      request.editor.volume.preview.refreshCount, &request.assetCatalog));
}

void refreshTerrainOperationScenePreview(
    const CreativeEditorAppPreviewPhaseRequest& request,
    const creative::CreativeDocument& renderDocument) {
  const creative::CreativeTerrainOperationMutationPlan* operationPreview =
      nullptr;
  const bool targetsRenderedDocument =
      &renderDocument == &request.activeAppState.facade.document();
  if (targetsRenderedDocument &&
      creativeEditorTerrainGenerationPreviewMatches(
          request.editor.terrainGeneration, renderDocument)) {
    operationPreview =
        &request.editor.terrainGeneration.operationPreview;
  }

  const creative::CreativeHotbarEntry& held =
      creative::selectedCreativeHotbarEntry(request.editor.interaction.hotbar);
  const bool gradeEligible =
      targetsRenderedDocument &&
      held.kind == creative::CreativeHeldItemKind::TerrainGrade &&
      request.editor.terrain.grade.active &&
      !request.modalBlocksWorldActions && !request.captureMode;
  if (gradeEligible) {
    static_cast<void>(refreshCreativeEditorTerrainGradePreview(
        renderDocument, request.editor));
    if (creativeEditorTerrainGradePreviewMatches(
            request.editor.terrain.grade, renderDocument)) {
      operationPreview = &request.editor.terrain.grade.operationPreview;
    } else {
      operationPreview = nullptr;
    }
  }

  if (operationPreview == nullptr) {
    invalidateCreativeEditorGeneratedTerrainPreview(
        request.terrainPreviewCache);
    return;
  }
  static_cast<void>(refreshCreativeEditorGeneratedTerrainPreview(
      request.terrainPreviewCache, request.sceneCache, renderDocument,
      operationPreview->heightField,
      operationPreview->receipt.replay.heightHash,
      operationPreview->materialField,
      operationPreview->receipt.replay.materialHash,
      &request.assetCatalog));
}

StandaloneRoomBakePreviewScene* resolveSelectedPreview(
    const CreativeEditorAppPreviewPhaseRequest& request,
    StandaloneRoomBakePreviewScene* selectedPreview) {
  selectedPreview = &request.sceneCache.preview;
  if (request.terrainPreviewCache.valid) {
    selectedPreview = &request.terrainPreviewCache.preview;
  }
  if (request.volumePreviewCache.valid) {
    selectedPreview = &request.volumePreviewCache.scene.preview;
  }
  return selectedPreview;
}

void resolvePreviewSelection(
    const CreativeEditorAppPreviewPhaseRequest& request,
    CreativeEditorAppPreviewPhaseReceipt& receipt) {
  receipt.selectedPreview =
      resolveSelectedPreview(request, receipt.selectedPreview);
  receipt.terrainContourSurface =
      &request.sceneCache.composedTerrainSurface;
  receipt.terrainContourSurfaceKey =
      request.sceneCache.terrainSurfaceBuildCount;
  if (request.terrainPreviewCache.valid &&
      receipt.selectedPreview == &request.terrainPreviewCache.preview) {
    receipt.terrainContourSurface =
        &request.terrainPreviewCache.composedSurface;
    receipt.terrainContourSurfaceKey =
        request.terrainPreviewCache.heightHash;
  }
}

}  // namespace

CreativeEditorAppPreviewPhaseReceipt prepareCreativeEditorAppPreviewPhase(
    const CreativeEditorAppPreviewPhaseRequest& request) {
  CreativeEditorAppPreviewPhaseReceipt receipt;
  const creative::CreativeDocument& layoutRenderDocument =
      creativeEditorWorldLayoutRenderDocument(
          request.editor.worldLayout,
          request.activeAppState.facade.document());
  receipt.renderDocument =
      &creativeEditorAssetReplacementRenderDocument(
          request.editor.assetReplacement, layoutRenderDocument);
  static_cast<void>(refreshCreativeEditorSceneCache(
      request.sceneCache, *receipt.renderDocument, &request.assetCatalog));
  refreshVolumeScenePreview(request, *receipt.renderDocument);
  refreshTerrainOperationScenePreview(request, *receipt.renderDocument);
  resolvePreviewSelection(request, receipt);
  return receipt;
}

void refreshCreativeEditorAppPreviewAfterInteraction(
    const CreativeEditorAppPreviewPhaseRequest& request,
    CreativeEditorAppPreviewPhaseReceipt& receipt,
    iggy3d::FrameInput& frame) {
  static_cast<void>(refreshCreativeEditorSceneCache(
      request.sceneCache, *receipt.renderDocument, &request.assetCatalog));
  refreshVolumeScenePreview(request, *receipt.renderDocument);
  refreshTerrainOperationScenePreview(request, *receipt.renderDocument);
  receipt.selectedPreview =
      resolveSelectedPreview(request, receipt.selectedPreview);
  frame.projections.scene = &receipt.selectedPreview->scene;
  frame.clock.sourceTick = receipt.selectedPreview->scene.sourceTick;
}

}  // namespace iggy3d_creative_app
