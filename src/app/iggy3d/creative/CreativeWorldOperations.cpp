#include "app/iggy3d/creative/CreativeWorldOperations.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "app/frontend/FrontendState.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"
#include "app/iggy3d/creative/CreativeBlankStageSession.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/world/Launch.hpp"

namespace iggy3d {
namespace {

void setCreativeNewWorldLaunchStatus(
    ProductCreativeNewWorldLaunchResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

void setCreativeOpenWorldLaunchStatus(
    ProductCreativeOpenWorldLaunchResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

void setCurrentCreativeSaveStatus(
    ProductCreativeCurrentWorldSaveResult& result,
    std::string reason) {
  result.status = std::move(reason);
  result.reasonCode = result.status;
}

std::string idOrNone(std::string_view value) {
  return value.empty() ? "none" : std::string(value);
}

std::string pathOrNone(const std::filesystem::path& path) {
  return path.empty() ? "none" : path.generic_string();
}

bool missingWindowIdentity(std::string_view value) {
  return value.empty() || value == "none";
}

void recordActiveCreativeSaveIdentity(
    creative::CreativeActiveIdentity& identity,
    std::string_view saveId,
    const std::filesystem::path& path,
    std::string_view worldId,
    creative::CreativeDocumentId documentId,
    std::uint64_t objectCount,
    creative::CreativeObjectId nextObjectId) {
  identity.saveId = idOrNone(saveId);
  identity.savePath = pathOrNone(path);
  identity.worldId = idOrNone(worldId);
  identity.documentId = documentId;
  identity.objectCount = objectCount;
  identity.nextObjectId = nextObjectId;
  identity.saveStatus = "creative_world_save_not_requested";
  identity.saveReasonCode = "creative_world_save_not_requested";
  identity.saveDirtyFlagsBefore = 0;
  identity.saveDirtyFlagsDrained = 0;
  identity.saveDirtyFlagsAfter = 0;
  identity.saveSavedAtUtc = "none";
}

void recordActiveCreativeSaveResult(
    creative::CreativeActiveIdentity& identity,
    const ProductCreativeCurrentWorldSaveResult& result) {
  identity.saveStatus = result.status;
  identity.saveReasonCode = result.reasonCode;
  identity.saveDirtyFlagsBefore = result.dirtyFlagsBefore;
  identity.saveDirtyFlagsDrained = result.dirtyFlagsDrained;
  identity.saveDirtyFlagsAfter = result.dirtyFlagsAfter;
  identity.saveSavedAtUtc =
      result.saveResult.savedAtUtc.empty() ? "none" : result.saveResult.savedAtUtc;
  if (result.accepted && result.saved) {
    identity.saveId = idOrNone(result.saveId);
    identity.savePath = pathOrNone(result.path);
    identity.worldId = idOrNone(result.worldId);
    identity.documentId = result.documentId;
    identity.objectCount = result.objectCount;
    identity.nextObjectId = result.nextObjectId;
    identity.saveStatus = result.status;
    identity.saveReasonCode = result.reasonCode;
    identity.saveDirtyFlagsBefore = result.dirtyFlagsBefore;
    identity.saveDirtyFlagsDrained = result.dirtyFlagsDrained;
    identity.saveDirtyFlagsAfter = result.dirtyFlagsAfter;
    identity.saveSavedAtUtc =
        result.saveResult.savedAtUtc.empty() ? "none" : result.saveResult.savedAtUtc;
  }
}

void mirrorCreativeWorldCreateResult(
    ProductCreativeNewWorldLaunchResult& result,
    const CreativeWorldCreateResult& create) {
  result.createResult = create;
  result.saveId = create.saveId.empty() ? "none" : create.saveId;
  result.path = create.path;
  result.worldId = create.worldId;
  result.documentId = create.documentId;
  result.objectCount = create.document.objectCount();
  result.nextObjectId = create.document.nextObjectId();
}

void mirrorCreativeWorldOpenResult(
    ProductCreativeOpenWorldLaunchResult& result,
    const CreativeWorldOpenResult& open) {
  result.openResult = open;
  result.saveId = open.saveId.empty() ? "none" : open.saveId;
  result.path = open.path;
  result.worldId = open.worldId;
  result.documentId = open.documentId;
  result.objectCount = open.objectCount;
  result.nextObjectId = open.nextObjectId;
}

void mirrorCreativeDocumentInstallResult(
    ProductCreativeNewWorldLaunchResult& result,
    const creative::CreativeFacadeDocumentInstallReceipt& install) {
  result.installReceipt = install;
  result.documentInstalled = install.accepted;
  if (install.accepted) {
    result.documentId = install.nextDocumentId;
    result.objectCount = install.nextObjectCount;
    result.nextObjectId = result.createResult.document.nextObjectId();
  }
}

void mirrorCreativeDocumentInstallResult(
    ProductCreativeOpenWorldLaunchResult& result,
    const creative::CreativeFacadeDocumentInstallReceipt& install) {
  result.installReceipt = install;
  result.documentInstalled = install.accepted;
  if (install.accepted) {
    result.documentId = install.nextDocumentId;
    result.objectCount = install.nextObjectCount;
    result.nextObjectId = result.openResult.document.nextObjectId();
  }
}

void mirrorCreativeBakedActiveRoomRefreshResult(
    ProductCreativeNewWorldLaunchResult& result,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  result.bakedActiveRoomRefreshRequested = true;
  result.bakedActiveRoomRefreshAccepted = refresh.accepted;
  result.bakedActiveRoomRefresh = refresh;
}

void mirrorCreativeBakedActiveRoomRefreshResult(
    ProductCreativeOpenWorldLaunchResult& result,
    const ProductCreativeBakedActiveRoomRefreshResult& refresh) {
  result.bakedActiveRoomRefreshRequested = true;
  result.bakedActiveRoomRefreshAccepted = refresh.accepted;
  result.bakedActiveRoomRefresh = refresh;
}

}  // namespace

ProductCreativeNewWorldLaunchResult launchProductCreativeNewWorld(
    const ProductAppOptions& options,
    const ProductCreativeNewWorldLaunchRequest& request,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState& creativeApp) {
  creative::Facade& facade = creativeApp.facade;
  ProductCreativeNewWorldLaunchResult result;
  window.frontendShell.launchAction = "creative_create_and_enter";

  CreativeWorldCreateRequest createRequest;
  createRequest.saveRoot = options.saveRoot;
  createRequest.title = request.title;
  createRequest.templateId = request.templateId;
  createRequest.requestedAtUtc = request.requestedAtUtc;
  createRequest.attemptToken = request.attemptToken;
  createRequest.packageId = request.packageId;
  createRequest.scenarioId = request.scenarioId;

  const CreativeWorldCreateResult create = createCreativeWorld(createRequest);
  mirrorCreativeWorldCreateResult(result, create);
  window.frontendShell.startup.creativeWorldIdScanMeasured = create.worldIdScanMeasured;
  window.frontendShell.startup.creativeWorldIdScanMicroseconds =
      create.worldIdScanMicroseconds;
  window.frontendShell.startup.creativeWorldIdScanEntryCount =
      create.worldIdScanEntryCount;
  window.frontendShell.startup.creativeWorldIdScanStatus = create.worldIdScanStatus;
  window.frontendShell.startup.creativeDocumentIdScanMeasured =
      create.documentIdScanMeasured;
  window.frontendShell.startup.creativeDocumentIdScanMicroseconds =
      create.documentIdScanMicroseconds;
  window.frontendShell.startup.creativeDocumentIdScanEntryCount =
      create.documentIdScanEntryCount;
  window.frontendShell.startup.creativeDocumentIdScanStatus = create.documentIdScanStatus;
  if (!create.accepted) {
    setCreativeNewWorldLaunchStatus(result, create.reasonCode);
    window.frontendShell.launchStatus = result.reasonCode;
    return result;
  }

  // F0: enter a blank creative stage (empty ground grid at origin), NOT the
  // first_room demo. Product New World keeps createProductSession.
  if (!createCreativeBlankSession(activeSession, window)) {
    setCreativeNewWorldLaunchStatus(result, window.frontendShell.launchStatus);
    return result;
  }
  frameCreativeStageCameraOnOrigin(window);
  result.sessionCreated = activeSession.has_value();

  creative::CreativeDocument documentToInstall = create.document;
  const creative::CreativeFacadeDocumentInstallReceipt install =
      facade.installDocument(std::move(documentToInstall));
  mirrorCreativeDocumentInstallResult(result, install);
  if (!install.accepted) {
    setCreativeNewWorldLaunchStatus(result, std::string{install.reasonCode});
    window.frontendShell.launchStatus = result.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    return result;
  }
  creative::clearCreativeUndoStack(creativeApp.undoStack);
  window.creativeAuthoring.creativeUndo.available = false;
  window.creativeAuthoring.creativeUndo.depth = 0;

  enterProductGameplayTransition(frontend, window, FrontendAction::CreateAndEnter);
  window.inputDevice.interactionMode = ProductInteractionMode::Creative;
  result.enteredGameplay = true;
  result.accepted = true;
  setCreativeNewWorldLaunchStatus(result, "product_creative_world_launched");
  window.frontendShell.launchStatus = result.reasonCode;
  recordActiveCreativeSaveIdentity(creativeApp.identity,
                                   result.saveId,
                                   result.path,
                                   result.worldId,
                                   result.documentId,
                                   result.objectCount,
                                   result.nextObjectId);
  const ProductCreativeBakedActiveRoomRefreshResult bakedActiveRoom =
      refreshProductCreativeBakedActiveRoom(
          ProductCreativeBakedActiveRoomRefreshRequest{},
          activeSession,
          window,
          creativeApp);
  mirrorCreativeBakedActiveRoomRefreshResult(result, bakedActiveRoom);
  return result;
}

ProductCreativeOpenWorldLaunchResult launchProductCreativeOpenWorld(
    const ProductAppOptions& options,
    const ProductCreativeOpenWorldLaunchRequest& request,
    FrontendState& frontend,
    std::optional<Session>& activeSession,
    ProductAppWindowState& window,
    creative::CreativeAppState& creativeApp) {
  creative::Facade& facade = creativeApp.facade;
  ProductCreativeOpenWorldLaunchResult result;
  window.frontendShell.launchAction = "creative_open_and_enter";

  CreativeWorldOpenRequest openRequest;
  openRequest.saveRoot = options.saveRoot;
  openRequest.saveId = request.saveId;

  const CreativeWorldOpenResult open = openCreativeWorld(openRequest);
  mirrorCreativeWorldOpenResult(result, open);
  if (!open.accepted) {
    setCreativeOpenWorldLaunchStatus(result, open.reasonCode);
    window.frontendShell.launchStatus = result.reasonCode;
    return result;
  }

  // F0: opening a creative world also stands on the blank stage, not first_room.
  if (!createCreativeBlankSession(activeSession, window)) {
    setCreativeOpenWorldLaunchStatus(result, window.frontendShell.launchStatus);
    return result;
  }
  frameCreativeStageCameraOnOrigin(window);
  result.sessionCreated = activeSession.has_value();

  creative::CreativeDocument documentToInstall = open.document;
  const creative::CreativeFacadeDocumentInstallReceipt install =
      facade.installDocument(std::move(documentToInstall));
  mirrorCreativeDocumentInstallResult(result, install);
  if (!install.accepted) {
    setCreativeOpenWorldLaunchStatus(result, std::string{install.reasonCode});
    window.frontendShell.launchStatus = result.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    return result;
  }
  creative::clearCreativeUndoStack(creativeApp.undoStack);
  window.creativeAuthoring.creativeUndo.available = false;
  window.creativeAuthoring.creativeUndo.depth = 0;

  enterProductGameplayTransition(frontend, window, FrontendAction::Load);
  window.inputDevice.interactionMode = ProductInteractionMode::Creative;
  result.enteredGameplay = true;
  result.accepted = true;
  setCreativeOpenWorldLaunchStatus(result, "product_creative_world_opened");
  window.frontendShell.launchStatus = result.reasonCode;
  recordActiveCreativeSaveIdentity(creativeApp.identity,
                                   result.saveId,
                                   result.path,
                                   result.worldId,
                                   result.documentId,
                                   result.objectCount,
                                   result.nextObjectId);
  const ProductCreativeBakedActiveRoomRefreshResult bakedActiveRoom =
      refreshProductCreativeBakedActiveRoom(
          ProductCreativeBakedActiveRoomRefreshRequest{},
          activeSession,
          window,
          creativeApp);
  mirrorCreativeBakedActiveRoomRefreshResult(result, bakedActiveRoom);
  return result;
}

ProductCreativeCurrentWorldSaveResult saveProductCurrentCreativeWorld(
    const ProductAppOptions& options,
    creative::CreativeAppState& creativeApp,
    std::string_view source,
    ProductAppWindowState& window) {
  (void)source;
  creative::Facade& facade = creativeApp.facade;
  const creative::CreativeActiveIdentity& identity = creativeApp.identity;

  ProductCreativeCurrentWorldSaveResult result;
  result.saveId = identity.saveId;
  result.path =
      missingWindowIdentity(identity.savePath)
          ? std::filesystem::path{}
          : std::filesystem::path{identity.savePath};
  result.worldId = identity.worldId;
  result.documentId = facade.document().id();
  result.objectCount = facade.document().objectCount();
  result.nextObjectId = facade.document().nextObjectId();
  result.dirtyFlagsBefore = facade.document().dirtyFlags();
  result.dirtyFlagsAfter = result.dirtyFlagsBefore;

  if (window.inputDevice.interactionMode != ProductInteractionMode::Creative) {
    setCurrentCreativeSaveStatus(result, "product_creative_save_inactive");
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }
  if (missingWindowIdentity(identity.saveId)) {
    setCurrentCreativeSaveStatus(result, "product_creative_save_id_missing");
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }
  if (facade.document().id() == creative::kInvalidDocumentId) {
    setCurrentCreativeSaveStatus(result,
                                 "product_creative_save_document_id_missing");
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }

  CreativeWorldSaveRequest request;
  request.saveRoot = options.saveRoot;
  request.saveId = identity.saveId;
  request.attemptToken = "attempt_002";
  request.document = &facade.documentForPersistence();
  if (!missingWindowIdentity(identity.worldId)) {
    request.worldId = identity.worldId;
  }
  request.savedAtUtc = productSaveTimestampNowUtc();

  const CreativeWorldSaveResult saved = saveCreativeWorld(request);
  result.saveResult = saved;
  result.saveId = saved.saveId.empty() ? result.saveId : saved.saveId;
  result.path = saved.path.empty() ? result.path : saved.path;
  result.worldId = saved.worldId.empty() ? result.worldId : saved.worldId;
  result.documentId = saved.documentId;
  result.objectCount = saved.objectCount;
  result.nextObjectId = saved.nextObjectId;
  result.dirtyFlagsBefore = saved.dirtyFlagsBefore;
  result.dirtyFlagsDrained = saved.dirtyFlagsDrained;
  result.dirtyFlagsAfter = saved.dirtyFlagsAfter;
  result.saved = saved.saved;
  if (!saved.accepted || !saved.saved) {
    setCurrentCreativeSaveStatus(result, saved.reasonCode);
    recordActiveCreativeSaveResult(creativeApp.identity, result);
    return result;
  }

  result.accepted = true;
  setCurrentCreativeSaveStatus(result, "product_creative_world_saved");
  creative::clearCreativeUndoStack(creativeApp.undoStack);
  window.creativeAuthoring.creativeUndo.available = false;
  window.creativeAuthoring.creativeUndo.depth = 0;
  recordActiveCreativeSaveResult(creativeApp.identity, result);
  return result;
}

}  // namespace iggy3d
