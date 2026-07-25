#include "EditorAppFramePhases.hpp"

#include <string>

#include "EditorAssetLibrary.hpp"
#include "EditorAssetReplacement.hpp"
#include "EditorAssets.hpp"
#include "EditorCatalog.hpp"
#include "EditorControls.hpp"
#include "EditorDesktopCommands.hpp"
#include "EditorDesktopPanels.hpp"
#include "EditorFrame.hpp"
#include "EditorToolOptions.hpp"
#include "EditorTransformFrame.hpp"
#include "EditorWorldLayoutLifecycle.hpp"
#include "render/vulkan/VulkanBackend.hpp"

namespace iggy3d_creative_app {

namespace creative = iggy3d::creative;
creative::CreativeObjectId firstCreativeEditorFloorObjectId(
    const creative::CreativeDocument& document) noexcept {
  for (const creative::CreativeObject& object : document.objects()) {
    if (object.kind == creative::CreativeObjectKind::Floor) {
      return object.id;
    }
  }
  return creative::kInvalidObjectId;
}

CreativeEditorAppToolPhaseReceipt runCreativeEditorAppToolPhase(
    const CreativeEditorAppToolPhaseRequest& request) {
  CreativeEditorAppToolPhaseReceipt receipt;
  CreativeEditorAssetLibraryFrameResult assetLibraryFrame;
  assetLibraryFrame.remainingInput = request.frameInput.routedInput;
  assetLibraryFrame = processCreativeEditorAssetLibraryFrame(
      {request.window, request.appState, request.editor,
       request.frameInput.routedInput});
  if (assetLibraryFrame.activeDocumentChanged) {
    invalidateCreativeEditorSceneCache(request.sceneCache);
  }

  creative::CreativeAppState& activeAppState =
      activeCreativeEditorAppState(request.editor, request.appState);
  receipt.activeAppState = &activeAppState;
  if (synchronizeCreativeEditorTerrainGeneration(
          request.editor.terrainGeneration,
          activeAppState.facade.document())) {
    invalidateCreativeEditorGeneratedTerrainPreview(
        request.terrainPreviewCache);
  }

  if (request.editor.desktopUi.frameActive) {
    CreativeDesktopCommandFrame desktopCommands;
    buildCreativeEditorDesktopMenuBar(
        request.editor.desktopUi, activeAppState,
        &activeAppState == &request.appState ? &request.editor.worldLayout
                                             : nullptr,
        desktopCommands);
    buildCreativeEditorDesktopPanels(
        request.editor.desktopUi, request.editor, activeAppState,
        &request.assetCatalog, &request.playtestOwner.monitor(),
        assetLibraryFrame.remainingInput, desktopCommands);
    if (desktopCommands.count > 0U || desktopCommands.overflowed) {
      const CreativeDesktopCommandResult desktopResult =
          dispatchCreativeDesktopCommands(
              desktopCommands,
              {request.appState,
               request.editor,
               request.saveRoot,
               &request.saveId,
               &request.assetCatalog,
               &request.playtestOwner});
      if (!desktopResult.message.empty()) {
        request.editor.desktopUi.statusMessage = desktopResult.message;
      }
      if (creativeDesktopCommandRequiresSceneRefresh(desktopResult)) {
        invalidateCreativeEditorSceneCache(request.sceneCache);
        request.floorObjectId =
            firstCreativeEditorFloorObjectId(
                request.appState.facade.document());
      }
    }
    buildCreativeEditorDesktopStatusBar(request.editor.desktopUi,
                                        request.editor, activeAppState);
  }

  const creative::CreativeInputRouteResult& routedInput =
      assetLibraryFrame.remainingInput;
  const CreativeEditorControlsFrameResult controlsFrame =
      processCreativeEditorControlsFrame(
          {request.window,
           request.editor,
           routedInput,
           request.frameInput.inputFrame,
           request.controlsPath,
           request.toolWheelPath,
           request.frameInput.monotonicTimeNanoseconds,
           request.frameInput.extent.width,
           request.frameInput.extent.height});
  const CreativeEditorTransformFrameResult transformFrame =
      processCreativeEditorTransformFrame(
          {request.window,
           activeAppState,
           request.editor,
           routedInput,
           request.frameInput.toolWheelDirectionX,
           request.frameInput.toolWheelDirectionY,
           request.frameInput.transformNudgeWheelSteps,
           request.frameInput.transformFineNudge,
           request.frameInput.extent.width,
           request.frameInput.extent.height});
  const CreativeEditorCatalogFrameResult catalogFrame =
      processCreativeEditorCatalogFrame(
          {request.window,
           activeAppState,
           request.editor,
           routedInput,
           request.frameInput.worldActions,
           request.toolWheelPath,
           request.frameInput.toolWheelDirectionX,
           request.frameInput.toolWheelDirectionY,
           request.frameInput.extent.width,
           request.frameInput.extent.height});
  if (catalogFrame.authoredAssetLibraryRequested) {
    const bool opened = beginCreativeEditorAssetLibrary(
        request.editor, request.appState.facade.document(),
        catalogFrame.authoredAssetId);
    if (opened) {
      static_cast<void>(
          creative::setCreativeCatalogOpen(request.editor.catalog.model, false));
      static_cast<void>(request.window.setTextInputActive(true));
      static_cast<void>(request.window.setRelativeMouseMode(false));
    } else {
      request.editor.catalog.statusLabel = "AUTHORED ASSET LIBRARY UNAVAILABLE";
    }
  }
  if (catalogFrame.assetReplacementRequested) {
    const CreativeAssetReplacementBeginReceipt begun =
        beginCreativeEditorAssetReplacement(
            activeAppState, request.assetCatalog,
            catalogFrame.replacementObjectKind,
            catalogFrame.replacementAssetId,
            request.editor.assetReplacement);
    if (begun.accepted) {
      static_cast<void>(
          creative::setCreativeCatalogOpen(request.editor.catalog.model, false));
      static_cast<void>(request.window.setTextInputActive(false));
      static_cast<void>(request.window.setRelativeMouseMode(true));
    } else {
      request.editor.catalog.statusLabel =
          "REPLACE FAILED: " + std::string(begun.reasonCode);
    }
  }

  const CreativeEditorAssetReplacementFrameResult assetReplacementFrame =
      processCreativeEditorAssetReplacementFrame(
          {activeAppState, request.editor.assetReplacement, routedInput});
  if (assetReplacementFrame.finished &&
      !assetReplacementFrame.commitReceipt.accepted) {
    invalidateCreativeEditorSceneCache(request.sceneCache);
  }
  const CreativeEditorToolOptionsFrameResult toolOptionsFrame =
      processCreativeEditorToolOptionsFrame(
          {request.window,
           activeAppState,
           request.editor,
           routedInput,
           catalogFrame.openToolOptionsRequested,
           catalogFrame.toolOptionsEntry,
           request.frameInput.extent.width,
           request.frameInput.extent.height,
           &request.assetCatalog,
           &request.sceneCache.placementClearance});
  const bool layoutPreviewActive =
      creativeEditorWorldLayoutPreviewActive(request.editor.worldLayout);
  receipt.modalBlocksWorldActions =
      assetLibraryFrame.blockWorldActions ||
      controlsFrame.blockWorldActions || transformFrame.blockWorldActions ||
      catalogFrame.blockWorldActions ||
      assetReplacementFrame.blockWorldActions ||
      toolOptionsFrame.blockWorldActions || layoutPreviewActive ||
      request.editor.terrainGeneration.previewActive;
  if (receipt.modalBlocksWorldActions ||
      !request.frameInput.windowFocused) {
    finalizeCreativeEditorContinuousGestures(
        activeAppState, request.editor,
        request.frameInput.windowFocused
            ? "creative_continuous_gesture_modal"
            : "creative_continuous_gesture_focus_lost");
  }
  if (catalogFrame.assetReloadRequested) {
    static_cast<void>(reloadCreativeEditorAssets(
        {request.backend,
         activeAppState,
         request.editor,
         request.sceneCache,
         request.assetCatalog,
         request.assetRoot}));
  }
  if (!catalogFrame.deferredCommandInput.actionEvents().empty()) {
    applyCreativeEditorCommandInput(
        catalogFrame.deferredCommandInput, activeAppState, request.editor,
        request.saveRoot, request.saveId, &request.appState);
  }
  if (!receipt.modalBlocksWorldActions &&
      request.frameInput.windowFocused) {
    applyCreativeEditorCommandInput(
        routedInput, activeAppState, request.editor, request.saveRoot,
        request.saveId, &request.appState);
  }
  return receipt;
}

}  // namespace iggy3d_creative_app
