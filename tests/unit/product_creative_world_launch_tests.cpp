#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/ui/UiDrawList.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/RenderDiagnostics.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

std::filesystem::path testRoot(std::string_view name) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "iggy3d_creative_launch" /
      std::string{name};
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root, error);
  return root;
}

iggy3d::ProductAppOptions testOptions(std::string_view name) {
  iggy3d::ProductAppOptions options;
  options.saveRoot = testRoot(name);
  return options;
}

iggy3d::ProductCreativeNewWorldLaunchRequest launchRequest(
    std::string_view title = "Creative Direct",
    std::string_view requestedAtUtc = "2026-07-03T12:00:00Z") {
  iggy3d::ProductCreativeNewWorldLaunchRequest request;
  request.title = std::string{title};
  request.requestedAtUtc = std::string{requestedAtUtc};
  return request;
}

cr::CreativeToolInputPacket pointerInput(cr::CreativeToolInputKind kind,
                                         double x,
                                         double y,
                                         cr::Id targetId = cr::kInvalidId) {
  cr::CreativeToolInputPacket input;
  input.kind = kind;
  input.pointer.x = x;
  input.pointer.y = y;
  input.pointer.button = cr::CreativeToolPointerButton::Primary;
  input.pointer.target.value = targetId;
  return input;
}

cr::Id targetId(cr::CreativeObjectId objectId) {
  return static_cast<cr::Id>(objectId);
}

iggy3d::ProductCreativeNewWorldLaunchResult launchCreativeWorld(
    const iggy3d::ProductAppOptions& options,
    const iggy3d::ProductCreativeNewWorldLaunchRequest& request,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app) {
  return iggy3d::launchProductCreativeNewWorld(options,
                                               request,
                                               frontend,
                                               activeSession,
                                               window,
                                               app);
}

iggy3d::ProductCreativeOpenWorldLaunchResult openCreativeWorld(
    const iggy3d::ProductAppOptions& options,
    std::string_view saveId,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app) {
  iggy3d::ProductCreativeOpenWorldLaunchRequest request;
  request.saveId = std::string{saveId};
  return iggy3d::launchProductCreativeOpenWorld(options,
                                                request,
                                                frontend,
                                                activeSession,
                                                window,
                                                app);
}

iggy3d::CreativeWorldSaveRequest saveRequest(
    const iggy3d::ProductAppOptions& options,
    std::string_view saveId,
    cr::CreativeDocument& document) {
  iggy3d::CreativeWorldSaveRequest request;
  request.saveRoot = options.saveRoot;
  request.saveId = std::string{saveId};
  request.document = &document;
  return request;
}

cr::CreativeDocumentCreateReceipt createBoundsObject(
    cr::Facade& facade,
    cr::CreativeObjectKind kind,
    cr::CreativeBounds bounds) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.bounds = bounds;
  request.hasBoundsOverride = true;
  return facade.createDocumentObject(request);
}

cr::CreativeDocumentCreateReceipt createPointObject(
    cr::Facade& facade,
    cr::CreativeObjectKind kind,
    cr::CreativeVec3 position) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = kind;
  request.transform.position = position;
  request.hasTransformOverride = true;
  return facade.createDocumentObject(request);
}

cr::CreativeDocumentCreateReceipt createPatrolRoute(cr::Facade& facade) {
  cr::CreativeDocumentCreateRequest request;
  request.kind = cr::CreativeObjectKind::PatrolRoute;
  request.hasPathOverride = true;
  request.pathPoints = {
      cr::CreativePathPoint{{0.0, 0.0, 0.0}},
      cr::CreativePathPoint{{2.0, 0.0, 0.0}},
      cr::CreativePathPoint{{2.0, 0.0, 2.0}},
  };
  return facade.createDocumentObject(request);
}

std::size_t countProjectedRole(const iggy3d::SceneRoomProjection& room,
                               std::string_view role) {
  std::size_t count = 0;
  for (const iggy3d::SceneRoomMeshItem& mesh : room.meshes) {
    if (mesh.role == role) {
      ++count;
    }
  }
  return count;
}

iggy3d::ProductActiveRoomState sentinelActiveRoom() {
  iggy3d::ProductActiveRoomState activeRoom;
  activeRoom.loaded = true;
  activeRoom.status = "sentinel_active_room";
  activeRoom.reasonCode = "sentinel_active_room";
  activeRoom.roomId = "sentinel_room";
  activeRoom.staticMeshCount = 99;
  activeRoom.room.id = "sentinel_room";
  return activeRoom;
}

iggy3d::ProductActiveRoomCollisionState sentinelActiveRoomCollision() {
  iggy3d::ProductActiveRoomCollisionState collision;
  collision.ready = true;
  collision.status = "sentinel_collision";
  collision.reasonCode = "sentinel_collision";
  collision.roomId = "sentinel_room";
  collision.querySurfaceCount = 77;
  return collision;
}

bool sentinelRoomStatePreserved(const iggy3d::ProductAppWindowState& window) {
  return expect(window.activeRoom.status == "sentinel_active_room",
                "sentinel active room preserved") &&
         expect(window.activeRoom.staticMeshCount == 99U,
                "sentinel active room mesh count preserved") &&
         expect(window.activeRoom.room.id == "sentinel_room",
                "sentinel active room id preserved") &&
         expect(window.activeRoomCollision.status == "sentinel_collision",
                "sentinel collision preserved") &&
         expect(window.activeRoomCollision.querySurfaceCount == 77U,
                "sentinel collision query count preserved");
}

iggy3d::MouseClick clickAt(float x, float y) {
  iggy3d::MouseClick click;
  click.clicked = true;
  click.x = x;
  click.y = y;
  return click;
}

bool selectFacadeObject(cr::Facade& facade, cr::CreativeObjectId objectId) {
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  const auto receipt = facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(objectId)));
  return receipt.selectionChanged ||
         facade.selectionState().selectedTarget.value == targetId(objectId);
}

const iggy3d::UiHitRegion* findHitRegion(
    const iggy3d::ProductUiDrawList& drawList,
    std::string_view semanticId) {
  for (const iggy3d::UiHitRegion& hit : drawList.hitRegions) {
    if (hit.semanticId == semanticId) {
      return &hit;
    }
  }
  return nullptr;
}

iggy3d::ProductUiDrawList creativeUiDrawListForApp(cr::CreativeAppState& app) {
  iggy3d::ProductCreativeUiProjectionRequest request;
  request.creative = &app;
  const iggy3d::ProductCreativeUiProjection projection =
      iggy3d::buildProductCreativeUiProjection(request);
  return projection.drawList;
}

bool clickCreativeUiRowThroughInputFrame(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app,
    std::string_view semanticId) {
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  bool closeRequested = false;

  const iggy3d::ProductUiDrawList drawList =
      creativeUiDrawListForApp(app);
  const iggy3d::UiHitRegion* hit = findHitRegion(drawList, semanticId);
  if (hit == nullptr) {
    return false;
  }

  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  clickOverride.click = clickAt(hit->rect.x, hit->rect.y);

  iggy3d::processProductWindowInputFrame(iggy3d::ProductWindowInputFrameContext{
      frontend,
      saves,
      options,
      settingsTab,
      activeSession,
      worldSetupDraft,
      window,
      settings,
      inputFrame,
      closeRequested,
      nullptr,
      &app,
      &drawList,
      {},
      {},
      0,
      cr::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });
  return true;
}

bool clickCreativeRebuildRoomThroughInputFrame(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app) {
  return clickCreativeUiRowThroughInputFrame(options,
                                            frontend,
                                            activeSession,
                                            window,
                                            app,
                                            "creative.row.tools.rebuild_room");
}

bool clickCreativeDeleteSelectedThroughInputFrame(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app) {
  return clickCreativeUiRowThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app,
      "creative.row.selection.delete_selected");
}

bool clickCreativeUndoThroughInputFrame(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app) {
  return clickCreativeUiRowThroughInputFrame(options,
                                            frontend,
                                            activeSession,
                                            window,
                                            app,
                                            "creative.row.tools.undo");
}

void runCreativePointerLifecycleFrame(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app,
    iggy3d::ProductWindowInputFrameState& inputFrame,
    bool primaryButtonDown,
    float x,
    float y) {
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::FrontendSettings settings;
  bool closeRequested = false;

  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.pointerLifecycle.enabled = true;
  clickOverride.pointerLifecycle.primaryButtonDown = primaryButtonDown;
  clickOverride.pointerLifecycle.x = x;
  clickOverride.pointerLifecycle.y = y;

  iggy3d::creative::CreativeSpatialProjectionRequest projectionRequest;
  projectionRequest.gridSize = {10, 10, 4};
  projectionRequest.cellSize = 1.0;
  projectionRequest.clampToGrid = true;

  iggy3d::processProductWindowInputFrame(iggy3d::ProductWindowInputFrameContext{
      frontend,
      saves,
      options,
      settingsTab,
      activeSession,
      worldSetupDraft,
      window,
      settings,
      inputFrame,
      closeRequested,
      nullptr,
      &app,
      nullptr,
      {0.0F, 0.0F, 100.0F, 100.0F},
      projectionRequest,
      0,
      cr::CreativeViewportPickDepthMode::FixedZ,
      clickOverride,
  });
}

iggy3d::ProductMenuActionResult confirmPauseAction(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendAction action,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState* app) {
  bool closeRequested = false;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::None;
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettings settings;

  (void)iggy3d::applyProductSystemPauseMenuAction(
      iggy3d::InputAction::SystemPause, {frontend, window, closeRequested});
  frontend.selectedAction = action;
  return iggy3d::applyProductPauseMenuAction(
      iggy3d::InputAction::MenuConfirm,
      {frontend, options, saves, settingsTab, activeSession, window,
       closeRequested, settings, app});
}

bool successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode() {
  const iggy3d::ProductAppOptions options = testOptions("success");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest(),
                          frontend,
                          activeSession,
                          window,
                          app);
  const iggy3d::ProductSaveBridgeResult scanned =
      iggy3d::scanProductSaves(options.saveRoot,
                               "iggy3d.creative",
                               "creative.document");
  const bool hasEntry = scanned.catalog.catalog.entries.size() == 1U;
  const iggy3d::ProductSaveCatalogEntry entry =
      hasEntry ? scanned.catalog.catalog.entries.front()
               : iggy3d::ProductSaveCatalogEntry{};
  const iggy3d::ProductContinueSelectionResult continued =
      iggy3d::selectProductContinueSave(scanned.catalog.catalog);
  const iggy3d::RenderReceipt receipt =
      iggy3d::buildProductAppReceipt(options,
                                     iggy3d::ProductWorldTemplate{},
                                     frontend,
                                     iggy3d::FrontendSettings{},
                                     window,
                                     scanned);

  return expect(launched.accepted, "creative launch accepted") &&
         expect(launched.status == "product_creative_world_launched",
                "creative launch status") &&
         expect(launched.reasonCode == "product_creative_world_launched",
                "creative launch reason") &&
         expect(launched.sessionCreated, "creative launch session created") &&
         expect(launched.documentInstalled,
                "creative launch document installed") &&
         expect(launched.enteredGameplay, "creative launch entered gameplay") &&
         expect(launched.createResult.accepted,
                "creative launch create accepted") &&
         expect(launched.installReceipt.accepted,
                "creative launch install accepted") &&
         expect(launched.saveId == "save_001", "creative launch save id") &&
         expect(launched.path == options.saveRoot / "save_001.iggy3d.save",
                "creative launch save path") &&
         expect(launched.worldId == "world_0001",
                "creative launch world id") &&
         expect(launched.documentId == 1U,
                "creative launch document id") &&
         expect(launched.objectCount == 0U,
                "creative launch object count") &&
         expect(launched.nextObjectId == 1U,
                "creative launch next object id") &&
         expect(std::filesystem::exists(launched.path),
                "creative launch durable save exists") &&
         expect(activeSession.has_value(),
                "creative launch active session present") &&
         expect(window.runtimeSessionCreated,
                "creative launch window runtime session") &&
         expect(window.gameplayActive, "creative launch gameplay active") &&
         expect(window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "creative launch interaction mode") &&
         expect(!iggy3d::productMapMakerLiveForWindow(frontend, window),
                "creative launch does not activate map maker") &&
         expect(window.mapMakerStatus == "map_maker_inactive",
                "creative launch map maker inactive status") &&
         expect(frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
                "creative launch frontend gameplay") &&
         expect(window.launchStatus == "product_creative_world_launched",
                "creative launch window status") &&
         expect(window.startupPackageLookupMeasured,
                "creative launch package lookup measured") &&
         expect(window.startupPackageLookupStatus ==
                    "startup_package_lookup_resolved",
                "creative launch package lookup status") &&
         expect(window.startupPackageLoadMeasured,
                "creative launch package load measured") &&
         expect(window.startupPackageLoadStatus == "ok",
                "creative launch package load status") &&
         expect(window.startupRuntimeSessionCreateMeasured,
                "creative launch session create measured") &&
         expect(window.startupRuntimeSessionCreateStatus ==
                    "startup_runtime_session_created",
                "creative launch session create status") &&
         expect(window.startupCreativeWorldIdScanMeasured,
                "creative launch world id scan measured") &&
         expect(window.startupCreativeWorldIdScanStatus ==
                    "product_world_id_scan_ready",
                "creative launch world id scan status") &&
         expect(window.startupCreativeDocumentIdScanMeasured,
                "creative launch document id scan measured") &&
         expect(window.startupCreativeDocumentIdScanStatus ==
                    "creative_document_id_scan_ready",
                "creative launch document id scan status") &&
         expect(window.activeProductSaveId == "none",
                "creative launch does not set product save id") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "creative launch active creative save id") &&
         expect(window.activeCreativeSavePath == launched.path.generic_string(),
                "creative launch active creative save path") &&
         expect(window.activeCreativeWorldId == launched.worldId,
                "creative launch active creative world id") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "creative launch active creative document id") &&
         expect(window.activeCreativeObjectCount == launched.objectCount,
                "creative launch active creative object count") &&
         expect(window.activeCreativeNextObjectId == launched.nextObjectId,
                "creative launch active creative next id") &&
         expect(window.activeCreativeSaveStatus ==
                    "creative_world_save_not_requested",
                "creative launch active creative save status") &&
         expect(facade.document().id() == launched.documentId,
                "creative launch facade document id") &&
         expect(facade.document().name() == "Creative Direct",
                "creative launch facade document name") &&
         expect(facade.document().objectCount() == 0U,
                "creative launch facade object count") &&
         expect(facade.document().revision() == 0U,
                "creative launch facade revision clean") &&
         expect(facade.document().dirtyFlags() == 0U,
                "creative launch facade dirty clean") &&
         expect(facade.document().nextObjectId() == 1U,
                "creative launch facade next id") &&
         expect(facade.toolState().activeTool == cr::Tool::Select,
                "creative launch active tool select") &&
         expect(facade.state().tool == cr::Tool::Select,
                "creative launch old state tool select") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "creative launch selection clear") &&
         expect(!facade.measurementState().active,
                "creative launch measurement inactive") &&
         expect(!facade.measurementState().hasMeasurement,
                "creative launch measurement empty") &&
         expect(!facade.ghostState().visible,
                "creative launch ghost hidden") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "creative launch pointer clear") &&
         expect(hasEntry, "creative launch catalog entry") &&
         expect(entry.contentKind ==
                    iggy3d::ProductSaveContentKind::CreativeDocument,
                "creative launch catalog kind") &&
         expect(entry.creativeDocumentPresent,
                "creative launch catalog present") &&
         expect(entry.creativeDocumentId == launched.documentId,
                "creative launch catalog document id") &&
         expect(entry.creativeObjectCount == 0U,
                "creative launch catalog object count") &&
         expect(iggy3d::canOpenCreativeWorld(entry),
                "creative launch can open creative") &&
         expect(!iggy3d::canLoadProductSave(entry),
                "creative launch not product loadable") &&
         expect(!continued.selected,
                "creative launch continue ignores creative") &&
         expect(continued.status == "continue_no_compatible_saves",
                "creative launch continue status") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "startup_package_lookup_measured",
                    "true"),
                "creative launch receipt package lookup measured") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "startup_runtime_session_create_status",
                    "startup_runtime_session_created"),
                "creative launch receipt session create status") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "startup_save_catalog_scan_entry_count",
                    "1"),
                "creative launch receipt save scan count") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "startup_creative_world_id_scan_status",
                    "product_world_id_scan_ready"),
                "creative launch receipt world id scan") &&
         expect(iggy3d::hasReceiptField(
                    receipt,
                    "startup_creative_document_id_scan_status",
                    "creative_document_id_scan_ready"),
                "creative launch receipt document id scan");
}

bool blankTitleOrTimestampRejectsBeforeSessionInstallAndModeSwitch() {
  {
    const iggy3d::ProductAppOptions options = testOptions("blank_title");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest(" ", "2026-07-03T12:00:00Z"),
                            frontend,
                            activeSession,
                            window,
                            app);

    if (!expect(!launched.accepted, "blank title rejected") ||
        !expect(launched.status == "creative_world_title_missing",
                "blank title status") ||
        !expect(!launched.sessionCreated, "blank title no session") ||
        !expect(!launched.documentInstalled, "blank title no install") ||
        !expect(!launched.enteredGameplay, "blank title no gameplay") ||
        !expect(!activeSession.has_value(), "blank title no active session") ||
        !expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                "blank title player mode") ||
        !expect(facade.document().id() == cr::kInvalidDocumentId,
                "blank title facade unchanged")) {
      return false;
    }
  }

  {
    const iggy3d::ProductAppOptions options = testOptions("blank_timestamp");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("No Time", ""),
                            frontend,
                            activeSession,
                            window,
                            app);

    return expect(!launched.accepted, "blank timestamp rejected") &&
           expect(launched.status == "creative_world_timestamp_missing",
                  "blank timestamp status") &&
           expect(!launched.sessionCreated, "blank timestamp no session") &&
           expect(!launched.documentInstalled, "blank timestamp no install") &&
           expect(!launched.enteredGameplay, "blank timestamp no gameplay") &&
           expect(!activeSession.has_value(),
                  "blank timestamp no active session") &&
           expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                  "blank timestamp player mode") &&
           expect(facade.document().id() == cr::kInvalidDocumentId,
                  "blank timestamp facade unchanged");
  }
}

bool invalidAttemptTokenWritesNoCommittedSaveAndDoesNotEnterCreativeMode() {
  const iggy3d::ProductAppOptions options = testOptions("invalid_attempt");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  iggy3d::ProductCreativeNewWorldLaunchRequest request = launchRequest();
  request.attemptToken = "attempt token with spaces";

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options, request, frontend, activeSession, window, app);
  const iggy3d::ProductSaveBridgeResult scanned =
      iggy3d::scanProductSaves(options.saveRoot,
                               "iggy3d.creative",
                               "creative.document");

  return expect(!launched.accepted, "invalid attempt rejected") &&
         expect(launched.status == "durable_save_invalid_attempt_token",
                "invalid attempt status") &&
         expect(launched.createResult.documentCreated,
                "invalid attempt local document created") &&
         expect(!launched.createResult.initialSaveWritten,
                "invalid attempt no initial save") &&
         expect(!launched.sessionCreated, "invalid attempt no session") &&
         expect(!launched.documentInstalled, "invalid attempt no install") &&
         expect(!launched.enteredGameplay, "invalid attempt no gameplay") &&
         expect(!activeSession.has_value(),
                "invalid attempt active session absent") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                "invalid attempt player mode") &&
         expect(facade.document().id() == cr::kInvalidDocumentId,
                "invalid attempt facade unchanged") &&
         expect(scanned.catalog.catalog.entries.empty(),
                "invalid attempt no catalog entry");
}

bool secondLaunchClearsOldFacadeStateAndInstallsNewDocument() {
  const iggy3d::ProductAppOptions options = testOptions("second_launch");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult first =
      launchCreativeWorld(options,
                          launchRequest("First", "2026-07-03T12:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(createdObject.objectId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   3.0,
                   4.0,
                   targetId(createdObject.objectId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   5.0,
                   6.0,
                   targetId(createdObject.objectId))));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerRelease,
                   7.0,
                   8.0,
                   targetId(createdObject.objectId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Select));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   9.0,
                   10.0,
                   targetId(createdObject.objectId))));
  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  window.creativeUndoAvailable = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndoDepth = cr::creativeUndoDepth(app.undoStack);

  const iggy3d::ProductCreativeNewWorldLaunchResult second =
      launchCreativeWorld(options,
                          launchRequest("Second", "2026-07-03T12:10:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);

  return expect(first.accepted, "second setup first accepted") &&
         expect(createdObject.accepted, "second setup object created") &&
         expect(second.accepted, "second launch accepted") &&
         expect(second.documentId == 2U, "second launch document id advances") &&
         expect(second.objectCount == 0U, "second launch empty document") &&
         expect(second.installReceipt.previousObjectCount == 1U,
                "second launch previous facade object count") &&
         expect(second.installReceipt.selectionCleared,
                "second launch selection cleared") &&
         expect(second.installReceipt.measurementCleared,
                "second launch measurement cleared") &&
         expect(second.installReceipt.ghostCleared,
                "second launch ghost cleared") &&
         expect(second.installReceipt.toolPointerCleared,
                "second launch pointer cleared") &&
         expect(activeSession.has_value(),
                "second launch active session present") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "second launch creative mode") &&
         expect(facade.document().id() == 2U,
                "second launch facade document id") &&
         expect(facade.document().name() == "Second",
                "second launch facade name") &&
         expect(facade.document().objectCount() == 0U,
                "second launch no leaked objects") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "second launch undo stack cleared") &&
         expect(!window.creativeUndoAvailable,
                "second launch window undo unavailable") &&
         expect(window.creativeUndoDepth == 0U,
                "second launch window undo depth") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "second launch selection invalid") &&
         expect(!facade.measurementState().hasMeasurement,
                "second launch measurement cleared state") &&
         expect(!facade.ghostState().visible,
                "second launch ghost hidden") &&
         expect(facade.toolState().pointer.target.value == cr::kInvalidId,
                "second launch pointer target invalid");
}

bool openLaunchRestoresSavedCreativeDocumentAndEntersCreativeMode() {
  const iggy3d::ProductAppOptions options = testOptions("open_success");
  iggy3d::FrontendState createFrontend;
  std::optional<iggy3d::Session> createSession;
  iggy3d::ProductAppWindowState createWindow;
  cr::CreativeAppState createApp;
  [[maybe_unused]] cr::Facade& createFacade = createApp.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult created =
      launchCreativeWorld(options,
                          launchRequest("Open Source",
                                        "2026-07-03T13:00:00Z"),
                          createFrontend,
                          createSession,
                          createWindow,
                          createApp);
  const cr::CreativeDocumentCreateReceipt createdObject =
      createFacade.createDocumentObject(cr::CreativeObjectKind::Room);
  cr::CreativeDocument savedDocument = createFacade.document();
  const bool renamed = savedDocument.rename("Opened Name");
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(
          saveRequest(options, created.saveId, savedDocument));

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  [[maybe_unused]] cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        created.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);

  return expect(created.accepted, "open launch setup create accepted") &&
         expect(createdObject.accepted, "open launch setup object created") &&
         expect(renamed, "open launch setup renamed") &&
         expect(saved.accepted, "open launch setup saved") &&
         expect(savedDocument.dirtyFlags() == 0U,
                "open launch setup save drained dirty") &&
         expect(opened.accepted, "open launch accepted") &&
         expect(opened.status == "product_creative_world_opened",
                "open launch status") &&
         expect(opened.reasonCode == "product_creative_world_opened",
                "open launch reason") &&
         expect(opened.sessionCreated, "open launch session created") &&
         expect(opened.documentInstalled, "open launch document installed") &&
         expect(opened.enteredGameplay, "open launch entered gameplay") &&
         expect(opened.openResult.accepted, "open launch open accepted") &&
         expect(opened.installReceipt.accepted,
                "open launch install accepted") &&
         expect(opened.saveId == created.saveId, "open launch save id") &&
         expect(opened.path == created.path, "open launch path") &&
         expect(opened.worldId == created.worldId, "open launch world id") &&
         expect(opened.documentId == created.documentId,
                "open launch document id") &&
         expect(opened.objectCount == 1U, "open launch object count") &&
         expect(opened.nextObjectId == savedDocument.nextObjectId(),
                "open launch next object id") &&
         expect(openSession.has_value(), "open launch active session") &&
         expect(openWindow.runtimeSessionCreated,
                "open launch runtime session window") &&
         expect(openWindow.gameplayActive, "open launch gameplay active") &&
         expect(openWindow.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "open launch creative mode") &&
         expect(openFrontend.childScreen == iggy3d::FrontendScreen::Gameplay,
                "open launch frontend gameplay") &&
         expect(openWindow.launchStatus == "product_creative_world_opened",
                "open launch window status") &&
         expect(openWindow.activeProductSaveId == "none",
                "open launch does not set product save id") &&
         expect(openWindow.activeCreativeSaveId == opened.saveId,
                "open launch active creative save id") &&
         expect(openWindow.activeCreativeSavePath == opened.path.generic_string(),
                "open launch active creative save path") &&
         expect(openWindow.activeCreativeWorldId == opened.worldId,
                "open launch active creative world id") &&
         expect(openWindow.activeCreativeDocumentId == opened.documentId,
                "open launch active creative document id") &&
         expect(openWindow.activeCreativeObjectCount == opened.objectCount,
                "open launch active creative object count") &&
         expect(openWindow.activeCreativeNextObjectId == opened.nextObjectId,
                "open launch active creative next id") &&
         expect(openFacade.document().id() == created.documentId,
                "open launch facade document id") &&
         expect(openFacade.document().name() == "Opened Name",
                "open launch facade document name") &&
         expect(openFacade.document().objectCount() == 1U,
                "open launch facade object count") &&
         expect(openFacade.document().nextObjectId() ==
                    savedDocument.nextObjectId(),
                "open launch facade next id") &&
         expect(openFacade.document().revision() == 0U,
                "open launch facade revision clean") &&
         expect(openFacade.document().dirtyFlags() == 0U,
                "open launch facade dirty clean") &&
         expect(openFacade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "open launch selection clear") &&
         expect(!openFacade.measurementState().hasMeasurement,
                "open launch measurement empty") &&
         expect(!openFacade.ghostState().visible,
                "open launch ghost hidden") &&
         expect(openFacade.toolState().pointer.target.value == cr::kInvalidId,
                "open launch pointer clear");
}

bool openLaunchRefreshesBakedActiveRoomFromSavedCreativeDocument() {
  const iggy3d::ProductAppOptions options = testOptions("open_baked_room");
  iggy3d::FrontendState createFrontend;
  std::optional<iggy3d::Session> createSession;
  iggy3d::ProductAppWindowState createWindow;
  cr::CreativeAppState createApp;
  cr::Facade& createFacade = createApp.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult created =
      launchCreativeWorld(options,
                          launchRequest("Open Baked Source",
                                        "2026-07-05T12:00:00Z"),
                          createFrontend,
                          createSession,
                          createWindow,
                          createApp);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(createFacade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const cr::CreativeDocumentCreateReceipt wall =
      createBoundsObject(createFacade,
                         cr::CreativeObjectKind::Wall,
                         {{5.0, 0.0, 0.0}, {9.0, 2.5, 0.25}});
  const cr::CreativeDocumentCreateReceipt crate =
      createBoundsObject(createFacade,
                         cr::CreativeObjectKind::Crate,
                         {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
  const cr::CreativeDocumentCreateReceipt beam =
      createBoundsObject(createFacade,
                         cr::CreativeObjectKind::Beam,
                         {{0.0, 1.0, 0.0}, {4.0, 1.35, 0.35}});
  const cr::CreativeDocumentCreateReceipt point =
      createPointObject(createFacade,
                        cr::CreativeObjectKind::PointLight,
                        {6.25, 1.5, -2.75});
  const cr::CreativeDocumentCreateReceipt path = createPatrolRoute(createFacade);
  const iggy3d::CreativeWorldSaveResult saved =
      iggy3d::saveCreativeWorld(
          saveRequest(options,
                      created.saveId,
                      createFacade.documentForPersistence()));

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        created.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);
  const iggy3d::SceneProjectionResult projection =
      openSession.has_value()
          ? iggy3d::buildSceneProjection(openSession->state(),
                                         &openWindow.activeRoom.room)
          : iggy3d::SceneProjectionResult{};

  return expect(created.accepted, "open baked setup create accepted") &&
         expect(floor.accepted, "open baked setup floor created") &&
         expect(wall.accepted, "open baked setup wall created") &&
         expect(crate.accepted, "open baked setup crate created") &&
         expect(beam.accepted, "open baked setup beam created") &&
         expect(point.accepted, "open baked setup point created") &&
         expect(path.accepted, "open baked setup path created") &&
         expect(saved.accepted, "open baked setup saved") &&
         expect(createFacade.document().dirtyFlags() == 0U,
                "open baked setup save drained dirty") &&
         expect(opened.accepted, "open baked launch accepted") &&
         expect(opened.status == "product_creative_world_opened",
                "open baked launch status") &&
         expect(opened.reasonCode == "product_creative_world_opened",
                "open baked launch reason") &&
         expect(opened.bakedActiveRoomRefreshRequested,
                "open baked refresh requested") &&
         expect(opened.bakedActiveRoomRefreshAccepted,
                "open baked refresh accepted flag") &&
         expect(opened.bakedActiveRoomRefresh.accepted,
                "open baked refresh accepted") &&
         expect(opened.bakedActiveRoomRefresh.status ==
                    "product_creative_baked_room_refreshed",
                "open baked refresh status") &&
         expect(opened.bakedActiveRoomRefresh.reasonCode ==
                    "product_creative_baked_room_refreshed",
                "open baked refresh reason") &&
         expect(opened.bakedActiveRoomRefresh.bakeMeasured,
                "open baked refresh measured") &&
         expect(opened.bakedActiveRoomRefresh.bakedDocumentRevision ==
                    openFacade.document().revision(),
                "open baked refresh revision") &&
         expect(opened.bakedActiveRoomRefresh.bakeReceipt.accepted,
                "open baked receipt accepted") &&
         expect(opened.bakedActiveRoomRefresh.staticMeshCount == 4U,
                "open baked result static mesh count") &&
         expect(opened.bakedActiveRoomRefresh.anchorCount == 1U,
                "open baked result anchor count") &&
         expect(opened.bakedActiveRoomRefresh.spatialSurfaceCount == 7U,
                "open baked result surface count") &&
         expect(opened.bakedActiveRoomRefresh.staticMeshSourceCount == 4U,
                "open baked result source mesh count") &&
         expect(opened.bakedActiveRoomRefresh.anchorSourceCount == 1U,
                "open baked result source anchor count") &&
         expect(opened.bakedActiveRoomRefresh.spatialSurfaceSourceCount == 7U,
                "open baked result source surface count") &&
         expect(openWindow.activeRoom.loaded, "open baked active room loaded") &&
         expect(openWindow.activeRoom.staticMeshCount == 4U,
                "open baked active mesh count") &&
         expect(openWindow.activeRoom.anchorCount == 1U,
                "open baked active anchor count") &&
         expect(openWindow.activeRoom.spatialSurfaceCount == 7U,
                "open baked active surface count") &&
         expect(openWindow.activeRoomCollision.ready,
                "open baked collision ready") &&
         expect(openWindow.activeRoomCollision.querySurfaceCount == 7U,
                "open baked collision query count") &&
         expect(projection.room.loaded, "open baked projection loaded") &&
         expect(countProjectedRole(projection.room, "floor") == 1U,
                "open baked projection floor count") &&
         expect(countProjectedRole(projection.room, "wall") == 1U,
                "open baked projection wall count") &&
         expect(countProjectedRole(projection.room, "prop") == 2U,
                "open baked projection prop count") &&
         expect(openWindow.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "open baked creative mode") &&
         expect(openWindow.activeCreativeSaveId == opened.saveId,
                "open baked active creative save id") &&
         expect(openWindow.activeCreativeDocumentId == opened.documentId,
                "open baked active creative document id") &&
         expect(openWindow.activeCreativeObjectCount == opened.objectCount,
                "open baked active creative object count") &&
         expect(openWindow.activeProductSaveId == "none",
                "open baked active product save none") &&
         expect(openFacade.document().objectCount() == 6U,
                "open baked facade object count") &&
         expect(openFacade.document().revision() == 0U,
                "open baked facade clean revision") &&
         expect(openFacade.document().dirtyFlags() == 0U,
                "open baked facade clean dirty");
}

bool openBlankOrInvalidSaveIdRejectsBeforeSessionInstallAndModeSwitch() {
  {
    const iggy3d::ProductAppOptions options = testOptions("open_blank_id");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
        openCreativeWorld(options, " ", frontend, activeSession, window, app);

    if (!expect(!opened.accepted, "open blank id rejected") ||
        !expect(opened.status == "creative_world_save_id_missing",
                "open blank id status") ||
        !expect(!opened.sessionCreated, "open blank id no session") ||
        !expect(!opened.documentInstalled, "open blank id no install") ||
        !expect(!opened.enteredGameplay, "open blank id no gameplay") ||
        !expect(!activeSession.has_value(), "open blank id no active session") ||
        !expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                "open blank id player mode") ||
        !expect(facade.document().id() == cr::kInvalidDocumentId,
                "open blank id facade unchanged")) {
      return false;
    }
  }

  {
    const iggy3d::ProductAppOptions options = testOptions("open_invalid_id");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
        openCreativeWorld(options,
                          "bad save id",
                          frontend,
                          activeSession,
                          window,
                          app);

    return expect(!opened.accepted, "open invalid id rejected") &&
           expect(opened.status == "creative_world_save_id_invalid",
                  "open invalid id status") &&
           expect(!opened.sessionCreated, "open invalid id no session") &&
           expect(!opened.documentInstalled, "open invalid id no install") &&
           expect(!opened.enteredGameplay, "open invalid id no gameplay") &&
           expect(!activeSession.has_value(),
                  "open invalid id no active session") &&
           expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                  "open invalid id player mode") &&
           expect(facade.document().id() == cr::kInvalidDocumentId,
                  "open invalid id facade unchanged");
  }
}

bool openProductSessionSaveRejectsAsMissingCreativeSection() {
  const iggy3d::ProductAppOptions options = testOptions("open_product_save");
  iggy3d::FrontendState productFrontend;
  std::optional<iggy3d::Session> productSession;
  iggy3d::ProductAppWindowState productWindow;
  iggy3d::WorldSetupDraft draft =
      iggy3d::makeDefaultWorldSetupDraft("product_session_world");

  iggy3d::launchProductNewWorld(options,
                                draft,
                                productFrontend,
                                productSession,
                                productWindow);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  [[maybe_unused]] cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        productWindow.activeProductSaveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);

  return expect(productSession.has_value(),
                "open product setup session created") &&
         expect(productWindow.activeProductSaveId != "none",
                "open product setup save id") &&
         expect(!opened.accepted, "open product rejected") &&
         expect(opened.status == "missing_creative_document_section",
                "open product missing creative status") &&
         expect(!opened.sessionCreated, "open product no new session") &&
         expect(!opened.documentInstalled, "open product no install") &&
         expect(!opened.enteredGameplay, "open product no gameplay") &&
         expect(!openSession.has_value(), "open product active session absent") &&
         expect(openWindow.interactionMode == iggy3d::ProductInteractionMode::Player,
                "open product player mode") &&
         expect(openFacade.document().id() == cr::kInvalidDocumentId,
                "open product facade unchanged");
}

bool productNewWorldLaunchClearsActiveCreativeIdentity() {
  const iggy3d::ProductAppOptions options = testOptions("product_clears_creative");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult created =
      launchCreativeWorld(options,
                          launchRequest("Creative Before Product",
                                        "2026-07-03T15:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const std::string creativeSaveId = window.activeCreativeSaveId;

  iggy3d::WorldSetupDraft draft =
      iggy3d::makeDefaultWorldSetupDraft("product_after_creative");
  draft.worldName = "Product After Creative";
  iggy3d::launchProductNewWorld(options, draft, frontend, activeSession, window);

  return expect(created.accepted,
                "product clear setup creative accepted") &&
         expect(creativeSaveId != "none",
                "product clear setup creative id recorded") &&
         expect(activeSession.has_value(),
                "product clear runtime session present") &&
         expect(window.gameplayActive,
                "product clear gameplay active") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                "product clear player interaction mode") &&
         expect(window.activeProductSaveId != "none",
                "product clear active product save id") &&
         expect(window.activeProductSaveId != creativeSaveId,
                "product clear product id differs creative id") &&
         expect(window.activeCreativeSaveId == "none",
                "product clear active creative save id") &&
         expect(window.activeCreativeSavePath == "none",
                "product clear active creative save path") &&
         expect(window.activeCreativeWorldId == "none",
                "product clear active creative world id") &&
         expect(window.activeCreativeDocumentId == cr::kInvalidDocumentId,
                "product clear active creative document id") &&
         expect(window.activeCreativeObjectCount == 0U,
                "product clear active creative object count") &&
         expect(window.activeCreativeNextObjectId == cr::kInvalidObjectId,
                "product clear active creative next id") &&
         expect(window.activeCreativeSaveStatus ==
                    "creative_world_save_not_requested",
                "product clear active creative save status");
}

bool currentCreativeWorldSaveDrainsDirtyAndPersistsDocument() {
  const iggy3d::ProductAppOptions options = testOptions("current_save_success");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Save Current Creative",
                                        "2026-07-03T16:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  const std::uint64_t revisionBefore = facade.document().revision();
  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  window.creativeUndoAvailable = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndoDepth = cr::creativeUndoDepth(app.undoStack);

  const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
      iggy3d::saveProductCurrentCreativeWorld(options, app, "unit", window);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  [[maybe_unused]] cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        launched.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);

  return expect(launched.accepted, "current save setup launch accepted") &&
         expect(createdObject.accepted,
                "current save setup object created") &&
         expect(dirtyBefore != 0U, "current save setup dirty nonzero") &&
         expect(saved.accepted, "current save accepted") &&
         expect(saved.saved, "current save saved") &&
         expect(saved.status == "product_creative_world_saved",
                "current save status") &&
         expect(saved.reasonCode == "product_creative_world_saved",
                "current save reason") &&
         expect(saved.saveId == launched.saveId, "current save id") &&
         expect(saved.path == launched.path, "current save path") &&
         expect(saved.worldId == launched.worldId, "current save world id") &&
         expect(saved.documentId == launched.documentId,
                "current save document id") &&
         expect(saved.objectCount == 1U, "current save object count") &&
         expect(saved.nextObjectId == facade.document().nextObjectId(),
                "current save next object id") &&
         expect(saved.dirtyFlagsBefore == dirtyBefore,
                "current save dirty before") &&
         expect(saved.dirtyFlagsDrained == dirtyBefore,
                "current save dirty drained") &&
         expect(saved.dirtyFlagsAfter == 0U, "current save dirty after") &&
         expect(facade.document().dirtyFlags() == 0U,
                "current save facade dirty drained") &&
         expect(facade.document().revision() == revisionBefore,
                "current save revision preserved") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "current save undo stack cleared") &&
         expect(!window.creativeUndoAvailable,
                "current save window undo unavailable") &&
         expect(window.creativeUndoDepth == 0U,
                "current save window undo depth") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "current save active id") &&
         expect(window.activeCreativeSavePath == launched.path.generic_string(),
                "current save active path") &&
         expect(window.activeCreativeWorldId == launched.worldId,
                "current save active world id") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "current save active document id") &&
         expect(window.activeCreativeObjectCount == 1U,
                "current save active object count") &&
         expect(window.activeCreativeNextObjectId ==
                    facade.document().nextObjectId(),
                "current save active next object id") &&
         expect(window.activeCreativeSaveStatus ==
                    "product_creative_world_saved",
                "current save active status") &&
         expect(window.activeCreativeSaveReasonCode ==
                    "product_creative_world_saved",
                "current save active reason") &&
         expect(window.activeCreativeSaveDirtyFlagsBefore == dirtyBefore,
                "current save active dirty before") &&
         expect(window.activeCreativeSaveDirtyFlagsDrained == dirtyBefore,
                "current save active dirty drained") &&
         expect(window.activeCreativeSaveDirtyFlagsAfter == 0U,
                "current save active dirty after") &&
         expect(window.activeCreativeSaveSavedAtUtc != "none",
                "current save saved timestamp") &&
         expect(opened.accepted, "current save reopen accepted") &&
         expect(opened.objectCount == 1U, "current save reopen object count") &&
         expect(openFacade.document().findObject(createdObject.objectId) != nullptr,
                "current save reopen object findable") &&
         expect(openFacade.document().dirtyFlags() == 0U,
                "current save reopen dirty clean");
}

bool currentCreativeWorldSaveRejectsInvalidContextsWithoutDrain() {
  {
    const iggy3d::ProductAppOptions options = testOptions("current_save_missing_id");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Missing Id",
                                          "2026-07-03T16:10:00Z"),
                            frontend,
                            activeSession,
                            window,
                            app);
    const cr::CreativeDocumentCreateReceipt createdObject =
        facade.createDocumentObject(cr::CreativeObjectKind::Room);
    const cr::CreativeObjectDirtyFlags dirtyBefore =
        facade.document().dirtyFlags();
    window.activeCreativeSaveId = "none";

    const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
        iggy3d::saveProductCurrentCreativeWorld(options, app, "unit", window);

    if (!expect(launched.accepted, "missing id setup launch accepted") ||
        !expect(createdObject.accepted, "missing id setup object created") ||
        !expect(!saved.accepted, "missing id rejected") ||
        !expect(saved.status == "product_creative_save_id_missing",
                "missing id status") ||
        !expect(saved.dirtyFlagsBefore == dirtyBefore,
                "missing id dirty before") ||
        !expect(saved.dirtyFlagsAfter == dirtyBefore,
                "missing id dirty after") ||
        !expect(facade.document().dirtyFlags() == dirtyBefore,
                "missing id facade dirty preserved") ||
        !expect(window.activeCreativeSaveStatus ==
                    "product_creative_save_id_missing",
                "missing id window status")) {
      return false;
    }
  }

  {
    const iggy3d::ProductAppOptions options = testOptions("current_save_inactive");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    [[maybe_unused]] cr::Facade& facade = app.facade;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Inactive Save",
                                          "2026-07-03T16:20:00Z"),
                            frontend,
                            activeSession,
                            window,
                            app);
    const cr::CreativeDocumentCreateReceipt createdObject =
        facade.createDocumentObject(cr::CreativeObjectKind::Room);
    const cr::CreativeObjectDirtyFlags dirtyBefore =
        facade.document().dirtyFlags();
    window.interactionMode = iggy3d::ProductInteractionMode::Player;

    const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
        iggy3d::saveProductCurrentCreativeWorld(options, app, "unit", window);

    if (!expect(launched.accepted, "inactive setup launch accepted") ||
        !expect(createdObject.accepted, "inactive setup object created") ||
        !expect(!saved.accepted, "inactive rejected") ||
        !expect(saved.status == "product_creative_save_inactive",
                "inactive status") ||
        !expect(saved.dirtyFlagsAfter == dirtyBefore,
                "inactive dirty after") ||
        !expect(facade.document().dirtyFlags() == dirtyBefore,
                "inactive facade dirty preserved") ||
        !expect(window.activeCreativeSaveStatus ==
                    "product_creative_save_inactive",
                "inactive window status")) {
      return false;
    }
  }

  const iggy3d::ProductAppOptions options = testOptions("current_save_invalid_doc");
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreativeSaveId = "save_001";
  window.activeCreativeWorldId = "world_0001";
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
      iggy3d::saveProductCurrentCreativeWorld(options, app, "unit", window);

  return expect(!saved.accepted, "invalid doc rejected") &&
         expect(saved.status == "product_creative_save_document_id_missing",
                "invalid doc status") &&
         expect(saved.dirtyFlagsBefore == 0U, "invalid doc dirty before") &&
         expect(saved.dirtyFlagsAfter == 0U, "invalid doc dirty after") &&
         expect(window.activeCreativeSaveStatus ==
                    "product_creative_save_document_id_missing",
                "invalid doc window status");
}

bool pauseCreativeSaveWritesCreativeDocumentAndKeepsSession() {
  const iggy3d::ProductAppOptions options = testOptions("pause_creative_save");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Save",
                                        "2026-07-03T17:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  const iggy3d::ProductMenuActionResult saved =
      confirmPauseAction(options,
                         iggy3d::FrontendAction::Save,
                         frontend,
                         activeSession,
                         window,
                         &app);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  [[maybe_unused]] cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        launched.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);

  return expect(launched.accepted, "pause creative save launch accepted") &&
         expect(createdObject.accepted, "pause creative save object created") &&
         expect(dirtyBefore != 0U, "pause creative save dirty before") &&
         expect(saved.handled && saved.accepted,
                "pause creative save handled") &&
         expect(frontend.status == "pause_creative_save_written",
                "pause creative save frontend status") &&
         expect(window.launchStatus == "product_creative_world_saved",
                "pause creative save launch status") &&
         expect(window.activeCreativeSaveStatus ==
                    "product_creative_world_saved",
                "pause creative save active status") &&
         expect(window.activeCreativeSaveDirtyFlagsBefore == dirtyBefore,
                "pause creative save dirty before mirrored") &&
         expect(window.activeCreativeSaveDirtyFlagsDrained == dirtyBefore,
                "pause creative save dirty drained mirrored") &&
         expect(window.activeCreativeSaveDirtyFlagsAfter == 0U,
                "pause creative save dirty after mirrored") &&
         expect(facade.document().dirtyFlags() == 0U,
                "pause creative save facade dirty drained") &&
         expect(activeSession.has_value(),
                "pause creative save keeps active session") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "pause creative save remains creative") &&
         expect(window.activeProductSaveId == "none",
                "pause creative save does not set product save id") &&
         expect(opened.accepted, "pause creative save reopen accepted") &&
         expect(opened.objectCount == 1U, "pause creative save reopen object") &&
         expect(openFacade.document().findObject(createdObject.objectId) != nullptr,
                "pause creative save reopened object findable");
}

bool pauseCreativeSaveNullFacadeFailsClosed() {
  const iggy3d::ProductAppOptions options = testOptions("pause_creative_null");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Null",
                                        "2026-07-03T17:10:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  const iggy3d::ProductMenuActionResult saved =
      confirmPauseAction(options,
                         iggy3d::FrontendAction::Save,
                         frontend,
                         activeSession,
                         window,
                         nullptr);

  return expect(launched.accepted, "pause creative null launch accepted") &&
         expect(createdObject.accepted, "pause creative null object created") &&
         expect(saved.handled && saved.accepted,
                "pause creative null handled") &&
         expect(frontend.status == "pause_creative_save_failed",
                "pause creative null frontend status") &&
         expect(window.launchStatus == "product_creative_save_facade_missing",
                "pause creative null launch status") &&
         expect(window.activeCreativeSaveStatus ==
                    "product_creative_save_facade_missing",
                "pause creative null active status") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "pause creative null dirty preserved") &&
         expect(activeSession.has_value(),
                "pause creative null keeps active session") &&
         expect(window.activeProductSaveId == "none",
                "pause creative null does not set product save id");
}

bool pauseCreativeSaveAndExitWritesReturnsTitleAndClearsIdentity() {
  const iggy3d::ProductAppOptions options =
      testOptions("pause_creative_save_exit");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Exit",
                                        "2026-07-03T17:20:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  const iggy3d::ProductMenuActionResult saved =
      confirmPauseAction(options,
                         iggy3d::FrontendAction::SaveAndExit,
                         frontend,
                         activeSession,
                         window,
                         &app);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  [[maybe_unused]] cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        launched.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);

  return expect(launched.accepted,
                "pause creative save exit launch accepted") &&
         expect(createdObject.accepted,
                "pause creative save exit object created") &&
         expect(dirtyBefore != 0U, "pause creative save exit dirty before") &&
         expect(saved.handled && saved.accepted,
                "pause creative save exit handled") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Starter,
                "pause creative save exit returned starter") &&
         expect(frontend.status == "returned_to_title",
                "pause creative save exit frontend returned") &&
         expect(window.launchStatus == "product_creative_world_saved",
                "pause creative save exit launch status") &&
         expect(!activeSession.has_value(),
                "pause creative save exit resets session") &&
         expect(!window.gameplayActive,
                "pause creative save exit clears gameplay active") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Player,
                "pause creative save exit player mode") &&
         expect(window.activeCreativeSaveId == "none",
                "pause creative save exit clears active creative id") &&
         expect(window.activeCreativeSaveStatus ==
                    "creative_world_save_not_requested",
                "pause creative save exit clears save status") &&
         expect(facade.document().dirtyFlags() == 0U,
                "pause creative save exit dirty drained") &&
         expect(window.activeProductSaveId == "none",
                "pause creative save exit does not set product save id") &&
         expect(opened.accepted, "pause creative save exit reopen accepted") &&
         expect(opened.objectCount == 1U,
                "pause creative save exit reopen object") &&
         expect(openFacade.document().findObject(createdObject.objectId) != nullptr,
                "pause creative save exit reopened object findable");
}

bool pauseCreativeSaveAndExitFailureKeepsSessionAndDirtyState() {
  const iggy3d::ProductAppOptions options =
      testOptions("pause_creative_save_exit_missing_id");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Exit Missing",
                                        "2026-07-03T17:30:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  window.activeCreativeSaveId = "none";
  const iggy3d::ProductMenuActionResult saved =
      confirmPauseAction(options,
                         iggy3d::FrontendAction::SaveAndExit,
                         frontend,
                         activeSession,
                         window,
                         &app);

  return expect(launched.accepted,
                "pause creative save exit failure launch accepted") &&
         expect(createdObject.accepted,
                "pause creative save exit failure object created") &&
         expect(saved.handled && saved.accepted,
                "pause creative save exit failure handled") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Pause,
                "pause creative save exit failure stays paused") &&
         expect(frontend.status == "pause_creative_save_and_exit_failed",
                "pause creative save exit failure frontend status") &&
         expect(window.launchStatus == "product_creative_save_id_missing",
                "pause creative save exit failure launch status") &&
         expect(activeSession.has_value(),
                "pause creative save exit failure keeps session") &&
         expect(window.gameplayActive,
                "pause creative save exit failure gameplay active") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "pause creative save exit failure stays creative") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "pause creative save exit failure dirty preserved") &&
         expect(window.activeCreativeSaveStatus ==
                    "product_creative_save_id_missing",
                "pause creative save exit failure active status");
}

bool pauseCreativeReturnToTitleClearsUndoStack() {
  const iggy3d::ProductAppOptions options =
      testOptions("pause_creative_return_title_undo");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Return",
                                        "2026-07-05T14:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  window.creativeUndoAvailable = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndoDepth = cr::creativeUndoDepth(app.undoStack);

  const iggy3d::ProductMenuActionResult returned =
      confirmPauseAction(options,
                         iggy3d::FrontendAction::ReturnToTitle,
                         frontend,
                         activeSession,
                         window,
                         &app);

  return expect(launched.accepted, "pause return launch accepted") &&
         expect(createdObject.accepted, "pause return object created") &&
         expect(returned.handled && returned.accepted,
                "pause return handled") &&
         expect(frontend.screen == iggy3d::FrontendScreen::Starter,
                "pause return starter") &&
         expect(frontend.status == "returned_to_title",
                "pause return status") &&
         expect(!activeSession.has_value(), "pause return session reset") &&
         expect(!window.gameplayActive, "pause return gameplay inactive") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "pause return undo stack cleared") &&
         expect(!window.creativeUndoAvailable,
                "pause return window undo unavailable") &&
         expect(window.creativeUndoDepth == 0U,
                "pause return window undo depth");
}

bool secondOpenClearsOldFacadeStateAndInstallsRestoredDocument() {
  const iggy3d::ProductAppOptions options = testOptions("open_second");

  iggy3d::FrontendState firstCreateFrontend;
  std::optional<iggy3d::Session> firstCreateSession;
  iggy3d::ProductAppWindowState firstCreateWindow;
  cr::CreativeAppState firstCreateApp;
  [[maybe_unused]] cr::Facade& firstCreateFacade = firstCreateApp.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult firstCreated =
      launchCreativeWorld(options,
                          launchRequest("First Open",
                                        "2026-07-03T14:00:00Z"),
                          firstCreateFrontend,
                          firstCreateSession,
                          firstCreateWindow,
                          firstCreateApp);
  const cr::CreativeDocumentCreateReceipt firstObject =
      firstCreateFacade.createDocumentObject(cr::CreativeObjectKind::Room);
  cr::CreativeDocument firstSavedDocument = firstCreateFacade.document();
  const iggy3d::CreativeWorldSaveResult firstSaved =
      iggy3d::saveCreativeWorld(
          saveRequest(options, firstCreated.saveId, firstSavedDocument));

  iggy3d::FrontendState secondCreateFrontend;
  std::optional<iggy3d::Session> secondCreateSession;
  iggy3d::ProductAppWindowState secondCreateWindow;
  cr::CreativeAppState secondCreateApp;
  [[maybe_unused]] cr::Facade& secondCreateFacade = secondCreateApp.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult secondCreated =
      launchCreativeWorld(options,
                          launchRequest("Second Open",
                                        "2026-07-03T14:10:00Z"),
                          secondCreateFrontend,
                          secondCreateSession,
                          secondCreateWindow,
                          secondCreateApp);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::CreativeAppState openApp;
  [[maybe_unused]] cr::Facade& openFacade = openApp.facade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult firstOpened =
      openCreativeWorld(options,
                        firstCreated.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);
  static_cast<void>(openFacade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(firstObject.objectId))));
  static_cast<void>(openFacade.setActiveTool(cr::Tool::Move));
  static_cast<void>(openFacade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   3.0,
                   4.0,
                   targetId(firstObject.objectId))));
  static_cast<void>(openFacade.setActiveTool(cr::Tool::Measure));
  static_cast<void>(openFacade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   5.0,
                   6.0,
                   targetId(firstObject.objectId))));
  static_cast<void>(openFacade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerRelease,
                   7.0,
                   8.0,
                   targetId(firstObject.objectId))));
  static_cast<void>(openFacade.setActiveTool(cr::Tool::Select));
  static_cast<void>(openFacade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerMove,
                   9.0,
                   10.0,
                   targetId(firstObject.objectId))));
  cr::pushCreativeUndoSnapshot(openApp.undoStack, openFacade.document());
  openWindow.creativeUndoAvailable =
      cr::creativeUndoAvailable(openApp.undoStack);
  openWindow.creativeUndoDepth = cr::creativeUndoDepth(openApp.undoStack);

  const iggy3d::ProductCreativeOpenWorldLaunchResult secondOpened =
      openCreativeWorld(options,
                        secondCreated.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openApp);

  return expect(firstCreated.accepted,
                "second open setup first create accepted") &&
         expect(firstObject.accepted,
                "second open setup first object created") &&
         expect(firstSaved.accepted, "second open setup first saved") &&
         expect(secondCreated.accepted,
                "second open setup second create accepted") &&
         expect(firstOpened.accepted, "second open first open accepted") &&
         expect(secondOpened.accepted, "second open accepted") &&
         expect(secondOpened.documentId == secondCreated.documentId,
                "second open document id") &&
         expect(secondOpened.objectCount == 0U,
                "second open empty restored object count") &&
         expect(secondOpened.installReceipt.previousObjectCount == 1U,
                "second open previous object count") &&
         expect(secondOpened.installReceipt.selectionCleared,
                "second open selection cleared") &&
         expect(secondOpened.installReceipt.measurementCleared,
                "second open measurement cleared") &&
         expect(secondOpened.installReceipt.ghostCleared,
                "second open ghost cleared") &&
         expect(secondOpened.installReceipt.toolPointerCleared,
                "second open pointer cleared") &&
         expect(openSession.has_value(), "second open active session") &&
         expect(openWindow.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "second open creative mode") &&
         expect(openFacade.document().id() == secondCreated.documentId,
                "second open facade document id") &&
         expect(openFacade.document().name() == "Second Open",
                "second open facade document name") &&
         expect(openFacade.document().objectCount() == 0U,
                "second open no leaked objects") &&
         expect(openFacade.document().revision() == 0U,
                "second open revision clean") &&
         expect(!cr::creativeUndoAvailable(openApp.undoStack),
                "second open undo stack cleared") &&
         expect(!openWindow.creativeUndoAvailable,
                "second open window undo unavailable") &&
         expect(openWindow.creativeUndoDepth == 0U,
                "second open window undo depth") &&
         expect(openFacade.document().dirtyFlags() == 0U,
                "second open dirty clean") &&
         expect(openFacade.selectionState().selectedTarget.value ==
                    cr::kInvalidId,
                "second open selection invalid") &&
         expect(!openFacade.measurementState().hasMeasurement,
                "second open measurement cleared state") &&
         expect(!openFacade.ghostState().visible,
                "second open ghost hidden") &&
         expect(openFacade.toolState().pointer.target.value == cr::kInvalidId,
                "second open pointer target invalid");
}

bool refreshCreativeBakedActiveRoomBuildsRoomCollisionAndProjection() {
  const iggy3d::ProductAppOptions options = testOptions("baked_room_refresh");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Baked Active Room",
                                        "2026-07-05T11:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const cr::CreativeDocumentCreateReceipt wall =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Wall,
                         {{5.0, 0.0, 0.0}, {9.0, 2.5, 0.25}});
  const cr::CreativeDocumentCreateReceipt crate =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Crate,
                         {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
  const cr::CreativeDocumentCreateReceipt beam =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Beam,
                         {{0.0, 1.0, 0.0}, {4.0, 1.35, 0.35}});
  const cr::CreativeDocumentCreateReceipt point =
      createPointObject(facade,
                        cr::CreativeObjectKind::PointLight,
                        {6.25, 1.5, -2.75});
  const cr::CreativeDocumentCreateReceipt path = createPatrolRoute(facade);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();

  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshed =
      iggy3d::refreshProductCreativeBakedActiveRoom(
          {},
          activeSession,
          window,
          app);
  const iggy3d::SceneProjectionResult projection =
      activeSession.has_value()
          ? iggy3d::buildSceneProjection(activeSession->state(),
                                         &window.activeRoom.room)
          : iggy3d::SceneProjectionResult{};

  return expect(launched.accepted, "baked room setup launch accepted") &&
         expect(activeSession.has_value(), "baked room active session") &&
         expect(floor.accepted, "baked room floor created") &&
         expect(wall.accepted, "baked room wall created") &&
         expect(crate.accepted, "baked room crate created") &&
         expect(beam.accepted, "baked room beam created") &&
         expect(point.accepted, "baked room point created") &&
         expect(path.accepted, "baked room path created") &&
         expect(dirtyBefore != 0U, "baked room dirty before refresh") &&
         expect(refreshed.accepted, "baked room refresh accepted") &&
         expect(refreshed.status ==
                    "product_creative_baked_room_refreshed",
                "baked room refresh status") &&
         expect(refreshed.reasonCode ==
                    "product_creative_baked_room_refreshed",
                "baked room refresh reason") &&
         expect(refreshed.documentId == launched.documentId,
                "baked room refresh document id") &&
         expect(refreshed.objectCount == 6U,
                "baked room refresh object count") &&
         expect(refreshed.bakeMeasured, "baked room refresh measured") &&
         expect(refreshed.bakedDocumentRevision == facade.document().revision(),
                "baked room refresh revision") &&
         expect(refreshed.bakeReceipt.accepted,
                "baked room bake accepted") &&
         expect(refreshed.bakeReceipt.status ==
                    cr::CreativeRoomBakeStatus::Baked,
                "baked room bake status") &&
         expect(refreshed.bakeReceipt.objectCount == 6U,
                "baked room bake object count") &&
         expect(refreshed.bakeReceipt.consideredObjectCount == 6U,
                "baked room bake considered count") &&
         expect(refreshed.bakeReceipt.bakedStaticMeshCount == 4U,
                "baked room bake static mesh count") &&
         expect(refreshed.bakeReceipt.bakedAnchorCount == 1U,
                "baked room bake anchor count") &&
         expect(refreshed.bakeReceipt.bakedSpatialSurfaceCount == 7U,
                "baked room bake spatial surface count") &&
         expect(refreshed.bakeReceipt.skippedUnsupportedShapeCount == 1U,
                "baked room bake unsupported path count") &&
         expect(refreshed.staticMeshCount == 4U,
                "baked room static mesh count") &&
         expect(refreshed.anchorCount == 1U, "baked room anchor count") &&
         expect(refreshed.spatialSurfaceCount == 7U,
                "baked room spatial surface count") &&
         expect(refreshed.staticMeshSourceCount == 4U,
                "baked room mesh source count") &&
         expect(refreshed.anchorSourceCount == 1U,
                "baked room anchor source count") &&
         expect(refreshed.spatialSurfaceSourceCount == 7U,
                "baked room surface source count") &&
         expect(refreshed.activeRoomLoaded,
                "baked room active room loaded result") &&
         expect(refreshed.activeRoomStatus == "active_room_loaded",
                "baked room active room status result") &&
         expect(refreshed.collisionReady,
                "baked room collision ready result") &&
         expect(refreshed.collisionQuerySurfaceCount == 7U,
                "baked room collision query count result") &&
         expect(window.activeRoom.loaded, "baked room window active loaded") &&
         expect(window.activeRoom.roomId == "iggy3d_creative_baked_room",
                "baked room window active room id") &&
         expect(window.activeRoom.sourceName == "iggy3d.creative",
                "baked room window source name") &&
         expect(window.activeRoom.sourceSubset == "creative_document_bake",
                "baked room window source subset") &&
         expect(window.activeRoom.staticMeshCount == 4U,
                "baked room window mesh count") &&
         expect(window.activeRoom.anchorCount == 1U,
                "baked room window anchor count") &&
         expect(window.activeRoom.spatialSurfaceCount == 7U,
                "baked room window surface count") &&
         expect(window.activeRoom.walkableSurfaceCount == 1U,
                "baked room walkable count") &&
         expect(window.activeRoom.actorBlockerSurfaceCount == 3U,
                "baked room actor blocker count") &&
         expect(window.activeRoom.projectileBlockerSurfaceCount == 3U,
                "baked room projectile blocker count") &&
         expect(window.activeRoomCollision.ready,
                "baked room window collision ready") &&
         expect(window.activeRoomCollision.querySurfaceCount == 7U,
                "baked room window collision query count") &&
         expect(window.activeRoomCollision.surfaces.size() == 7U,
                "baked room window collision surface set count") &&
         expect(projection.room.loaded, "baked room projection loaded") &&
         expect(countProjectedRole(projection.room, "floor") == 1U,
                "baked room projection floor count") &&
         expect(countProjectedRole(projection.room, "wall") == 1U,
                "baked room projection wall count") &&
         expect(countProjectedRole(projection.room, "prop") == 2U,
                "baked room projection prop count") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "baked room interaction remains creative") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "baked room active creative save id preserved") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "baked room active creative document id preserved") &&
         expect(window.activeCreativeObjectCount == launched.objectCount,
                "baked room active creative object count unchanged") &&
         expect(window.activeProductSaveId == "none",
                "baked room active product save id unchanged") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "baked room dirty flags preserved");
}

bool manualRebuildRoomCommandRefreshesBakedActiveRoomThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("manual_baked_room_rebuild");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Manual Baked Active Room",
                                        "2026-07-05T12:10:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const cr::CreativeDocumentCreateReceipt wall =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Wall,
                         {{5.0, 0.0, 0.0}, {9.0, 2.5, 0.25}});
  const cr::CreativeDocumentCreateReceipt crate =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Crate,
                         {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
  const cr::CreativeDocumentCreateReceipt beam =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Beam,
                         {{0.0, 1.0, 0.0}, {4.0, 1.35, 0.35}});
  const cr::CreativeDocumentCreateReceipt point =
      createPointObject(facade,
                        cr::CreativeObjectKind::PointLight,
                        {6.25, 1.5, -2.75});
  const cr::CreativeDocumentCreateReceipt path = createPatrolRoute(facade);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  cr::pushCreativeUndoSnapshot(app.undoStack, facade.document());
  const std::uint64_t undoDepthBeforeCommand =
      cr::creativeUndoDepth(app.undoStack);
  window.creativeUndoAvailable = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndoDepth = undoDepthBeforeCommand;
  const bool activeRoomLoadedBeforeCommand = window.activeRoom.loaded;
  window.creativeBakedRoomStale = true;
  window.creativeBakedRoomStaleDocumentId = facade.document().id();
  window.creativeBakedRoomStaleRevision = facade.document().revision();
  window.creativeBakedRoomStaleStatus =
      "creative_baked_room_stale_document_changed";
  window.creativeBakedRoomStaleReasonCode =
      "creative_baked_room_stale_document_changed";

  const bool clicked = clickCreativeRebuildRoomThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);
  const iggy3d::SceneProjectionResult projection =
      activeSession.has_value()
          ? iggy3d::buildSceneProjection(activeSession->state(),
                                         &window.activeRoom.room)
          : iggy3d::SceneProjectionResult{};

  return expect(launched.accepted, "manual rebuild setup launch accepted") &&
         expect(activeSession.has_value(), "manual rebuild active session") &&
         expect(!launched.bakedActiveRoomRefreshAccepted,
                "manual rebuild launch starts blank") &&
         expect(!activeRoomLoadedBeforeCommand,
                "manual rebuild active room starts blank") &&
         expect(floor.accepted, "manual rebuild floor created") &&
         expect(wall.accepted, "manual rebuild wall created") &&
         expect(crate.accepted, "manual rebuild crate created") &&
         expect(beam.accepted, "manual rebuild beam created") &&
         expect(point.accepted, "manual rebuild point created") &&
         expect(path.accepted, "manual rebuild path created") &&
         expect(dirtyBefore != 0U, "manual rebuild dirty before command") &&
         expect(clicked, "manual rebuild row clicked through input frame") &&
         expect(window.creativeUiInputConsumed,
                "manual rebuild ui input consumed") &&
         expect(window.creativeUiInputSemanticId ==
                    "creative.row.tools.rebuild_room",
                "manual rebuild ui semantic") &&
         expect(window.creativeUiCommandAccepted,
                "manual rebuild command accepted") &&
         expect(!window.creativeUiCommandChanged,
                "manual rebuild command no document change") &&
         expect(window.creativeUiCommandKind == "rebuild_room",
                "manual rebuild command kind") &&
         expect(window.creativeUiCommandStatus ==
                    "product_creative_ui_command_rebuild_room_requested",
                "manual rebuild command status") &&
         expect(window.creativeUiCommandBakedRoomRefreshRequested,
                "manual rebuild refresh requested") &&
         expect(window.creativeUiCommandBakedRoomRefreshAccepted,
                "manual rebuild refresh accepted") &&
         expect(window.creativeUiCommandBakedRoomRefreshStatus ==
                    "product_creative_baked_room_refreshed",
                "manual rebuild refresh status") &&
         expect(window.creativeUiCommandBakedRoomBakeMeasured,
                "manual rebuild refresh measured") &&
         expect(window.creativeUiCommandBakedRoomBakedDocumentRevision ==
                    facade.document().revision(),
                "manual rebuild refresh revision") &&
         expect(window.creativeUiCommandBakedRoomStaticMeshCount == 4U,
                "manual rebuild refresh static mesh count") &&
         expect(window.creativeUiCommandBakedRoomAnchorCount == 1U,
                "manual rebuild refresh anchor count") &&
         expect(window.creativeUiCommandBakedRoomSpatialSurfaceCount == 7U,
                "manual rebuild refresh spatial surface count") &&
         expect(window.creativeUiCommandBakedRoomCollisionReady,
                "manual rebuild refresh collision ready") &&
         expect(window.creativeUiCommandBakedRoomCollisionQuerySurfaceCount == 7U,
                "manual rebuild refresh collision query count") &&
         expect(window.creativeDocumentRevisionObserved,
                "manual rebuild revision observed") &&
         expect(!window.creativeDocumentChangedThisFrame,
                "manual rebuild no document mutation") &&
         expect(cr::creativeUndoDepth(app.undoStack) == undoDepthBeforeCommand,
                "manual rebuild undo depth preserved") &&
         expect(window.creativeUndoAvailable,
                "manual rebuild window undo available preserved") &&
         expect(window.creativeUndoDepth == undoDepthBeforeCommand,
                "manual rebuild window undo depth preserved") &&
         expect(!window.creativeBakedRoomStale,
                "manual rebuild clears stale on accepted refresh") &&
         expect(window.creativeBakedRoomStaleDocumentId ==
                    facade.document().id(),
                "manual rebuild stale doc id fresh") &&
         expect(window.creativeBakedRoomStaleRevision ==
                    facade.document().revision(),
                "manual rebuild stale revision fresh") &&
         expect(window.creativeBakedRoomStaleStatus ==
                    "creative_baked_room_fresh",
                "manual rebuild stale status fresh") &&
         expect(window.activeRoom.loaded,
                "manual rebuild active room loaded") &&
         expect(window.activeRoom.staticMeshCount == 4U,
                "manual rebuild active mesh count") &&
         expect(window.activeRoom.anchorCount == 1U,
                "manual rebuild active anchor count") &&
         expect(window.activeRoom.spatialSurfaceCount == 7U,
                "manual rebuild active surface count") &&
         expect(window.activeRoomCollision.ready,
                "manual rebuild collision ready") &&
         expect(window.activeRoomCollision.querySurfaceCount == 7U,
                "manual rebuild collision query count") &&
         expect(projection.room.loaded, "manual rebuild projection loaded") &&
         expect(countProjectedRole(projection.room, "floor") == 1U,
                "manual rebuild projection floor count") &&
         expect(countProjectedRole(projection.room, "wall") == 1U,
                "manual rebuild projection wall count") &&
         expect(countProjectedRole(projection.room, "prop") == 2U,
                "manual rebuild projection prop count") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "manual rebuild interaction remains creative") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "manual rebuild active creative save preserved") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "manual rebuild active creative document preserved") &&
         expect(window.activeProductSaveId == "none",
                "manual rebuild active product save unchanged") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "manual rebuild dirty flags preserved");
}

bool manualRebuildRoomCommandClearsRoomStateOnNoRenderableDocument() {
  const iggy3d::ProductAppOptions options =
      testOptions("manual_baked_room_no_renderable");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Manual Empty Baked Room",
                                        "2026-07-05T12:20:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      app.facade.document().dirtyFlags();
  window.activeRoom = sentinelActiveRoom();
  window.activeRoomCollision = sentinelActiveRoomCollision();
  window.creativeBakedRoomStale = true;
  window.creativeBakedRoomStaleDocumentId = app.facade.document().id();
  window.creativeBakedRoomStaleRevision = app.facade.document().revision();
  window.creativeBakedRoomStaleStatus =
      "creative_baked_room_stale_document_changed";
  window.creativeBakedRoomStaleReasonCode =
      "creative_baked_room_stale_document_changed";

  const bool clicked = clickCreativeRebuildRoomThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);

  return expect(launched.accepted,
                "manual empty rebuild setup launch accepted") &&
         expect(clicked, "manual empty rebuild clicked") &&
         expect(window.creativeUiCommandKind == "rebuild_room",
                "manual empty rebuild command kind") &&
         expect(window.creativeUiCommandBakedRoomRefreshRequested,
                "manual empty rebuild refresh requested") &&
         expect(window.creativeUiCommandBakedRoomRefreshAccepted,
                "manual empty rebuild refresh accepted clear") &&
         expect(window.creativeUiCommandBakedRoomRefreshStatus ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild refresh status") &&
         expect(window.creativeUiCommandBakedRoomRefreshReasonCode ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild refresh reason") &&
         expect(window.creativeUiCommandBakedRoomBakeMeasured,
                "manual empty rebuild refresh measured") &&
         expect(window.creativeUiCommandBakedRoomBakedDocumentRevision ==
                    app.facade.document().revision(),
                "manual empty rebuild refresh revision") &&
         expect(window.creativeUiCommandBakedRoomStaticMeshCount == 0U,
                "manual empty rebuild static mesh count") &&
         expect(window.creativeUiCommandBakedRoomAnchorCount == 0U,
                "manual empty rebuild anchor count") &&
         expect(window.creativeUiCommandBakedRoomSpatialSurfaceCount == 0U,
                "manual empty rebuild spatial surface count") &&
         expect(!window.creativeUiCommandBakedRoomCollisionReady,
                "manual empty rebuild collision not ready") &&
         expect(window.creativeUiCommandBakedRoomCollisionQuerySurfaceCount == 0U,
                "manual empty rebuild collision query count") &&
         expect(window.creativeDocumentRevisionObserved,
                "manual empty rebuild revision observed") &&
         expect(!window.creativeDocumentChangedThisFrame,
                "manual empty rebuild no document mutation") &&
         expect(!window.creativeBakedRoomStale,
                "manual empty rebuild clears stale after clear") &&
         expect(window.creativeBakedRoomStaleDocumentId ==
                    app.facade.document().id(),
                "manual empty rebuild fresh stale doc id") &&
         expect(window.creativeBakedRoomStaleRevision ==
                    app.facade.document().revision(),
                "manual empty rebuild fresh stale revision") &&
         expect(window.creativeBakedRoomStaleStatus ==
                    "creative_baked_room_fresh",
                "manual empty rebuild stale status fresh") &&
         expect(!window.activeRoom.loaded,
                "manual empty rebuild active room unloaded") &&
         expect(window.activeRoom.status ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild active room clear status") &&
         expect(window.activeRoom.reasonCode ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild active room clear reason") &&
         expect(window.activeRoom.source == "creative_room_bake",
                "manual empty rebuild active room clear source") &&
         expect(window.activeRoom.roomId == "iggy3d_creative_baked_room",
                "manual empty rebuild active room clear room id") &&
         expect(window.activeRoom.sourceName == "iggy3d.creative",
                "manual empty rebuild active room clear source name") &&
         expect(window.activeRoom.sourceSubset == "creative_document_bake",
                "manual empty rebuild active room clear source subset") &&
         expect(window.activeRoom.staticMeshCount == 0U,
                "manual empty rebuild active room clear mesh count") &&
         expect(window.activeRoom.anchorCount == 0U,
                "manual empty rebuild active room clear anchor count") &&
         expect(window.activeRoom.spatialSurfaceCount == 0U,
                "manual empty rebuild active room clear surface count") &&
         expect(window.activeRoom.room.staticMeshes.empty(),
                "manual empty rebuild room static meshes empty") &&
         expect(window.activeRoom.room.anchors.empty(),
                "manual empty rebuild room anchors empty") &&
         expect(window.activeRoom.room.spatialSurfaces.empty(),
                "manual empty rebuild room surfaces empty") &&
         expect(!window.activeRoomCollision.ready,
                "manual empty rebuild collision unavailable") &&
         expect(window.activeRoomCollision.status ==
                    "active_room_collision_unavailable",
                "manual empty rebuild collision clear status") &&
         expect(window.activeRoomCollision.reasonCode ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild collision clear reason") &&
         expect(window.activeRoomCollision.querySurfaceCount == 0U,
                "manual empty rebuild collision clear query count") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "manual empty rebuild active creative save preserved") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "manual empty rebuild active creative document preserved") &&
         expect(window.activeProductSaveId == "none",
                "manual empty rebuild active product save unchanged") &&
         expect(app.facade.document().dirtyFlags() == dirtyBefore,
                "manual empty rebuild dirty flags preserved");
}

bool disabledUndoRowDoesNotRouteThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("undo_disabled_no_history");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Undo Disabled",
                                        "2026-07-05T13:05:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const std::uint64_t revisionBefore = facade.document().revision();
  const std::uint64_t objectCountBefore = facade.document().objectCount();

  const bool undoClicked = clickCreativeUndoThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);

  return expect(launched.accepted, "disabled undo launch accepted") &&
         expect(undoClicked, "disabled undo row clicked") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "disabled undo no app stack") &&
         expect(!window.creativeUndoAvailable,
                "disabled undo window unavailable") &&
         expect(window.creativeUndoDepth == 0U,
                "disabled undo window depth") &&
         expect(facade.document().revision() == revisionBefore,
                "disabled undo revision unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "disabled undo object count unchanged") &&
         expect(window.creativeUiInputSemanticId == "creative.row.tools.undo",
                "disabled undo semantic") &&
         expect(window.creativeUiInputHit,
                "disabled undo input hit") &&
         expect(!window.creativeUiInputConsumed,
                "disabled undo input not consumed") &&
         expect(!window.creativeUiInputEnabled,
                "disabled undo input disabled") &&
         expect(window.creativeUiInputStatus ==
                    "product_creative_ui_input_hit_disabled",
                "disabled undo input status") &&
         expect(window.creativeUiCommandKind == "none",
                "disabled undo no command kind") &&
         expect(!window.creativeUiCommandAccepted,
                "disabled undo command not accepted") &&
         expect(!window.creativeDocumentChangedThisFrame,
                "disabled undo no document change") &&
         expect(!window.creativeBakedRoomAutoRefreshRequested,
                "disabled undo no auto refresh");
}

bool undoAfterCreateCrateRestoresEmptyDocumentThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("undo_create_crate");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Undo Create Crate",
                                        "2026-07-05T13:10:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);

  const bool createClicked = clickCreativeUiRowThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app,
      "creative.row.create.create_crate");
  const cr::CreativeObjectId createdObjectId =
      window.creativeUiCommandCreateObjectId;
  const bool undoAvailableAfterCreate =
      cr::creativeUndoAvailable(app.undoStack);
  const std::uint64_t undoDepthAfterCreate =
      cr::creativeUndoDepth(app.undoStack);
  const bool activeRoomLoadedAfterCreate = window.activeRoom.loaded;

  const bool undoClicked = clickCreativeUndoThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);

  return expect(launched.accepted, "undo create launch accepted") &&
         expect(createClicked, "undo create crate clicked") &&
         expect(createdObjectId != cr::kInvalidObjectId,
                "undo create object id") &&
         expect(facade.findObject(createdObjectId) == nullptr,
                "undo create object removed") &&
         expect(undoAvailableAfterCreate,
                "undo create undo available after create") &&
         expect(undoDepthAfterCreate == 1U,
                "undo create undo depth after create") &&
         expect(activeRoomLoadedAfterCreate,
                "undo create active room loaded after create") &&
         expect(undoClicked, "undo create undo clicked") &&
         expect(facade.document().objectCount() == 0U,
                "undo create object count restored") &&
         expect(facade.document().revision() == 0U,
                "undo create revision restored") &&
         expect(facade.document().dirtyFlags() == 0U,
                "undo create dirty flags restored") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "undo create selection clear") &&
         expect(window.creativeUiInputSemanticId == "creative.row.tools.undo",
                "undo create semantic") &&
         expect(window.creativeUiCommandKind == "undo_last_document_change",
                "undo create command kind") &&
         expect(window.creativeUiCommandAccepted,
                "undo create command accepted") &&
         expect(window.creativeUiCommandChanged,
                "undo create command changed") &&
         expect(window.creativeUiCommandUndoRequested,
                "undo create requested") &&
         expect(window.creativeUiCommandUndoAccepted,
                "undo create accepted") &&
         expect(window.creativeUiCommandUndoChanged,
                "undo create changed") &&
         expect(window.creativeUiCommandUndoHadSnapshot,
                "undo create had snapshot") &&
         expect(window.creativeUiCommandUndoDocumentId == launched.documentId,
                "undo create document id") &&
         expect(window.creativeUiCommandUndoRevisionBefore == 1U,
                "undo create revision before") &&
         expect(window.creativeUiCommandUndoRevisionAfter == 0U,
                "undo create revision after") &&
         expect(window.creativeUiCommandUndoObjectCountBefore == 1U,
                "undo create object count before") &&
         expect(window.creativeUiCommandUndoObjectCountAfter == 0U,
                "undo create object count after") &&
         expect(window.creativeUiCommandUndoDepthBefore == 1U,
                "undo create depth before") &&
         expect(window.creativeUiCommandUndoDepthAfter == 0U,
                "undo create depth after") &&
         expect(window.creativeUiCommandUndoStatus == "creative_undo_applied",
                "undo create undo status") &&
         expect(window.creativeDocumentChangedThisFrame,
                "undo create document changed") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "undo create auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "undo create auto refresh accepted") &&
         expect(window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "undo create active room cleared") &&
         expect(!window.activeRoom.loaded,
                "undo create active room unloaded") &&
         expect(!window.activeRoomCollision.ready,
                "undo create collision unavailable") &&
         expect(!window.creativeBakedRoomStale,
                "undo create stale fresh") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "undo create no redo stack") &&
         expect(!window.creativeUndoAvailable,
                "undo create window undo unavailable") &&
         expect(window.creativeUndoDepth == 0U,
                "undo create window undo depth");
}

bool undoAfterDeleteSelectedRestoresRenderableThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("undo_delete_selected");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Undo Delete Selected",
                                        "2026-07-05T13:20:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh =
      iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                    activeSession,
                                                    window,
                                                    app);
  const bool selected = selectFacadeObject(facade, floor.objectId);
  const bool deleteClicked = clickCreativeDeleteSelectedThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);
  const bool undoAvailableAfterDelete =
      cr::creativeUndoAvailable(app.undoStack);
  const std::uint64_t undoDepthAfterDelete =
      cr::creativeUndoDepth(app.undoStack);

  const bool undoClicked = clickCreativeUndoThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);
  const cr::CreativeObject* restored = facade.findObject(floor.objectId);

  return expect(launched.accepted, "undo delete launch accepted") &&
         expect(floor.accepted, "undo delete floor created") &&
         expect(initialRefresh.accepted, "undo delete initial refresh") &&
         expect(selected, "undo delete floor selected") &&
         expect(deleteClicked, "undo delete clicked") &&
         expect(undoAvailableAfterDelete,
                "undo delete undo available after delete") &&
         expect(undoDepthAfterDelete == 1U,
                "undo delete undo depth after delete") &&
         expect(undoClicked, "undo delete undo clicked") &&
         expect(restored != nullptr, "undo delete floor restored") &&
         expect(restored != nullptr && restored->kind == cr::CreativeObjectKind::Floor,
                "undo delete restored floor kind") &&
         expect(facade.document().objectCount() == 1U,
                "undo delete object count restored") &&
         expect(facade.document().revision() == 1U,
                "undo delete revision restored") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "undo delete selection clear") &&
         expect(window.creativeUiCommandKind == "undo_last_document_change",
                "undo delete command kind") &&
         expect(window.creativeUiCommandUndoAccepted,
                "undo delete accepted") &&
         expect(window.creativeUiCommandUndoObjectCountBefore == 0U,
                "undo delete object count before") &&
         expect(window.creativeUiCommandUndoObjectCountAfter == 1U,
                "undo delete object count after") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "undo delete auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "undo delete auto refresh accepted") &&
         expect(!window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "undo delete active room not cleared") &&
         expect(window.activeRoom.loaded,
                "undo delete active room loaded") &&
         expect(window.activeRoom.staticMeshCount == 1U,
                "undo delete active mesh count") &&
         expect(window.activeRoom.spatialSurfaceCount == 1U,
                "undo delete active surface count") &&
         expect(window.activeRoomCollision.ready,
                "undo delete collision ready") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "undo delete collision query count") &&
         expect(!window.creativeBakedRoomStale,
                "undo delete stale fresh") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "undo delete no redo stack") &&
         expect(window.creativeUndoDepth == 0U,
                "undo delete window undo depth");
}

bool undoAfterVisibilityToggleRestoresBakedRoomThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("undo_visibility_toggle");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Undo Visibility Toggle",
                                        "2026-07-05T13:30:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh =
      iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                    activeSession,
                                                    window,
                                                    app);
  const bool selected = selectFacadeObject(facade, floor.objectId);
  const bool hideClicked = clickCreativeUiRowThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app,
      "creative.row.selection.inspector_visible");
  const cr::CreativeObject* hidden = facade.findObject(floor.objectId);
  const bool undoAvailableAfterHide =
      cr::creativeUndoAvailable(app.undoStack);

  const bool undoClicked = clickCreativeUndoThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);
  const cr::CreativeObject* restored = facade.findObject(floor.objectId);

  return expect(launched.accepted, "undo visibility launch accepted") &&
         expect(floor.accepted, "undo visibility floor created") &&
         expect(initialRefresh.accepted, "undo visibility initial refresh") &&
         expect(selected, "undo visibility floor selected") &&
         expect(hideClicked, "undo visibility hide clicked") &&
         expect(hidden != nullptr && !hidden->visible,
                "undo visibility floor hidden") &&
         expect(undoAvailableAfterHide,
                "undo visibility undo available after hide") &&
         expect(undoClicked, "undo visibility undo clicked") &&
         expect(restored != nullptr && restored->visible,
                "undo visibility floor visible restored") &&
         expect(facade.document().objectCount() == 1U,
                "undo visibility object count") &&
         expect(facade.document().revision() == 1U,
                "undo visibility revision restored") &&
         expect(window.creativeUiCommandKind == "undo_last_document_change",
                "undo visibility command kind") &&
         expect(window.creativeUiCommandUndoAccepted,
                "undo visibility accepted") &&
         expect(window.creativeUiCommandUndoObjectCountBefore == 1U,
                "undo visibility object count before") &&
         expect(window.creativeUiCommandUndoObjectCountAfter == 1U,
                "undo visibility object count after") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "undo visibility auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "undo visibility auto refresh accepted") &&
         expect(!window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "undo visibility active room not cleared") &&
         expect(window.activeRoom.loaded,
                "undo visibility active room loaded") &&
         expect(window.activeRoom.staticMeshCount == 1U,
                "undo visibility active mesh count") &&
         expect(window.activeRoomCollision.ready,
                "undo visibility collision ready") &&
         expect(!window.creativeBakedRoomStale,
                "undo visibility stale fresh") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "undo visibility no redo stack") &&
         expect(window.creativeUndoDepth == 0U,
                "undo visibility window undo depth");
}

bool autoRefreshVisibilityToggleClearsAndRestoresBakedRoomThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("auto_visibility_baked_room");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Auto Visibility Baked Room",
                                        "2026-07-05T12:30:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const bool selected = selectFacadeObject(facade, floor.objectId);
  const cr::CreativeObjectDirtyFlags dirtyAfterCreate =
      facade.document().dirtyFlags();
  window.activeRoom = sentinelActiveRoom();
  window.activeRoomCollision = sentinelActiveRoomCollision();
  window.creativeBakedRoomStale = true;
  window.creativeBakedRoomStaleDocumentId = facade.document().id();
  window.creativeBakedRoomStaleRevision = facade.document().revision();
  window.creativeBakedRoomStaleStatus =
      "creative_baked_room_stale_document_changed";
  window.creativeBakedRoomStaleReasonCode =
      "creative_baked_room_stale_document_changed";

  bool ok = expect(launched.accepted, "auto visibility launch accepted") &&
            expect(floor.accepted, "auto visibility floor created") &&
            expect(selected, "auto visibility floor selected") &&
            expect(dirtyAfterCreate != 0U,
                   "auto visibility dirty after create");

  const bool hideClicked = clickCreativeUiRowThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app,
      "creative.row.selection.inspector_visible");
  const cr::CreativeObject* hidden = facade.findObject(floor.objectId);
  const cr::CreativeObjectDirtyFlags dirtyAfterHide =
      facade.document().dirtyFlags();
  ok &= expect(hideClicked, "auto visibility hide clicked") &&
        expect(hidden != nullptr && !hidden->visible,
               "auto visibility floor hidden") &&
        expect(window.creativeUiCommandKind ==
                   "toggle_selected_object_visibility",
               "auto visibility command kind hide") &&
        expect(window.creativeDocumentChangedThisFrame,
               "auto visibility hide document changed") &&
        expect(window.creativeBakedRoomAutoRefreshRequested,
               "auto visibility hide auto requested") &&
        expect(window.creativeBakedRoomAutoRefreshAccepted,
               "auto visibility hide auto accepted") &&
        expect(window.creativeBakedRoomAutoRefreshClearedActiveRoom,
               "auto visibility hide cleared") &&
        expect(window.creativeBakedRoomAutoRefreshStatus ==
                   "product_creative_baked_room_cleared_no_renderable_objects",
               "auto visibility hide clear status") &&
        expect(!window.activeRoom.loaded,
               "auto visibility hide active room unloaded") &&
        expect(window.activeRoom.status ==
                   "product_creative_baked_room_cleared_no_renderable_objects",
               "auto visibility hide active room status") &&
        expect(!window.activeRoomCollision.ready,
               "auto visibility hide collision unavailable") &&
        expect(!window.creativeBakedRoomStale,
               "auto visibility hide stale cleared") &&
        expect(window.creativeBakedRoomStaleStatus ==
                   "creative_baked_room_fresh",
               "auto visibility hide stale fresh") &&
        expect(facade.document().dirtyFlags() == dirtyAfterHide,
               "auto visibility hide dirty flags preserved");

  const bool showClicked = clickCreativeUiRowThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app,
      "creative.row.selection.inspector_visible");
  const cr::CreativeObject* shown = facade.findObject(floor.objectId);
  ok &= expect(showClicked, "auto visibility show clicked") &&
        expect(shown != nullptr && shown->visible,
               "auto visibility floor visible again") &&
        expect(window.creativeDocumentChangedThisFrame,
               "auto visibility show document changed") &&
        expect(window.creativeBakedRoomAutoRefreshRequested,
               "auto visibility show auto requested") &&
        expect(window.creativeBakedRoomAutoRefreshAccepted,
               "auto visibility show auto accepted") &&
        expect(!window.creativeBakedRoomAutoRefreshClearedActiveRoom,
               "auto visibility show not cleared") &&
        expect(window.creativeBakedRoomAutoRefreshStatus ==
                   "product_creative_baked_room_refreshed",
               "auto visibility show refresh status") &&
        expect(window.creativeBakedRoomAutoRefreshStaticMeshCount == 1U,
               "auto visibility show mesh count") &&
        expect(window.creativeBakedRoomAutoRefreshSpatialSurfaceCount == 1U,
               "auto visibility show surface count") &&
        expect(window.creativeBakedRoomAutoRefreshCollisionReady,
               "auto visibility show collision ready") &&
        expect(window.activeRoom.loaded,
               "auto visibility show active room loaded") &&
        expect(window.activeRoom.staticMeshCount == 1U,
               "auto visibility show active mesh count") &&
        expect(window.activeRoom.spatialSurfaceCount == 1U,
               "auto visibility show active surface count") &&
        expect(window.activeRoomCollision.ready,
               "auto visibility show active collision ready") &&
        expect(window.activeRoomCollision.querySurfaceCount == 1U,
               "auto visibility show collision query count") &&
        expect(!window.creativeBakedRoomStale,
               "auto visibility show stale cleared") &&
        expect(window.activeCreativeSaveId == launched.saveId,
               "auto visibility active creative save preserved") &&
        expect(window.activeProductSaveId == "none",
               "auto visibility active product save unchanged") &&
        expect(facade.document().dirtyFlags() != 0U,
               "auto visibility dirty flags not drained");

  return ok;
}

bool deleteSelectedRenderableClearsBakedRoomThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("delete_selected_clears_baked_room");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Delete Selected Clears",
                                        "2026-07-05T12:50:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh =
      iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                    activeSession,
                                                    window,
                                                    app);
  const bool activeRoomLoadedBeforeDelete = window.activeRoom.loaded;
  const bool selected = selectFacadeObject(facade, floor.objectId);
  const cr::CreativeObjectDirtyFlags dirtyBeforeDelete =
      facade.document().dirtyFlags();
  const std::uint64_t revisionBeforeDelete = facade.document().revision();

  const bool clicked = clickCreativeDeleteSelectedThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);

  return expect(launched.accepted, "delete clear launch accepted") &&
         expect(floor.accepted, "delete clear floor created") &&
         expect(initialRefresh.accepted, "delete clear initial refresh") &&
         expect(activeRoomLoadedBeforeDelete,
                "delete clear initial active room") &&
         expect(selected, "delete clear floor selected") &&
         expect(clicked, "delete clear row clicked") &&
         expect(facade.findObject(floor.objectId) == nullptr,
                "delete clear floor gone") &&
         expect(facade.document().objectCount() == 0U,
                "delete clear object count") &&
         expect(facade.document().revision() == revisionBeforeDelete + 1U,
                "delete clear revision advanced") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "delete clear selection cleared") &&
         expect(window.creativeUiInputConsumed,
                "delete clear ui input consumed") &&
         expect(window.creativeUiInputSemanticId ==
                    "creative.row.selection.delete_selected",
                "delete clear ui semantic") &&
         expect(window.creativeUiCommandKind == "delete_selected_object",
                "delete clear command kind") &&
         expect(window.creativeUiCommandAccepted,
                "delete clear command accepted") &&
         expect(window.creativeUiCommandChanged,
                "delete clear command changed") &&
         expect(window.creativeUiCommandDeleteRequested,
                "delete clear requested") &&
         expect(window.creativeUiCommandDeleteAccepted,
                "delete clear delete accepted") &&
         expect(window.creativeUiCommandDeleteChanged,
                "delete clear delete changed") &&
         expect(window.creativeUiCommandDeleteRemoved,
                "delete clear delete removed") &&
         expect(window.creativeUiCommandDeleteObjectId == floor.objectId,
                "delete clear object id") &&
         expect(window.creativeUiCommandDeleteObjectKind == "Floor",
                "delete clear object kind") &&
         expect(window.creativeUiCommandDeleteRevisionBefore ==
                    revisionBeforeDelete,
                "delete clear revision before") &&
         expect(window.creativeUiCommandDeleteRevisionAfter ==
                    revisionBeforeDelete + 1U,
                "delete clear revision after") &&
         expect(window.creativeUiCommandDeleteStatus == "Removed",
                "delete clear delete status") &&
         expect(window.creativeDocumentChangedThisFrame,
                "delete clear document changed") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "delete clear auto requested") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "delete clear auto accepted") &&
         expect(window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "delete clear auto cleared") &&
         expect(window.creativeBakedRoomAutoRefreshStatus ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "delete clear auto status") &&
         expect(!window.activeRoom.loaded,
                "delete clear active room unloaded") &&
         expect(window.activeRoom.staticMeshCount == 0U,
                "delete clear active mesh count") &&
         expect(!window.activeRoomCollision.ready,
                "delete clear collision unavailable") &&
         expect(window.activeRoomCollision.querySurfaceCount == 0U,
                "delete clear collision query count") &&
         expect(!window.creativeBakedRoomStale,
                "delete clear stale fresh") &&
         expect(window.creativeBakedRoomStaleStatus ==
                    "creative_baked_room_fresh",
                "delete clear stale status") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "delete clear active creative save preserved") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "delete clear active creative document preserved") &&
         expect(window.activeProductSaveId == "none",
                "delete clear active product save unchanged") &&
         expect(dirtyBeforeDelete != 0U, "delete clear dirty before") &&
         expect(facade.document().dirtyFlags() != 0U,
                "delete clear dirty flags not drained");
}

bool deleteOneOfTwoRenderablesRebuildsRemainingBakedRoomThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("delete_selected_rebuilds_remaining_baked_room");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Delete Selected Rebuilds",
                                        "2026-07-05T13:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const cr::CreativeDocumentCreateReceipt crate =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Crate,
                         {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh =
      iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                    activeSession,
                                                    window,
                                                    app);
  const bool selected = selectFacadeObject(facade, crate.objectId);
  const cr::CreativeObjectDirtyFlags dirtyBeforeDelete =
      facade.document().dirtyFlags();
  const std::uint64_t revisionBeforeDelete = facade.document().revision();

  const bool clicked = clickCreativeDeleteSelectedThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);

  return expect(launched.accepted, "delete rebuild launch accepted") &&
         expect(floor.accepted, "delete rebuild floor created") &&
         expect(crate.accepted, "delete rebuild crate created") &&
         expect(initialRefresh.accepted, "delete rebuild initial refresh") &&
         expect(initialRefresh.staticMeshCount == 2U,
                "delete rebuild initial mesh count") &&
         expect(selected, "delete rebuild crate selected") &&
         expect(clicked, "delete rebuild row clicked") &&
         expect(facade.findObject(crate.objectId) == nullptr,
                "delete rebuild crate gone") &&
         expect(facade.findObject(floor.objectId) != nullptr,
                "delete rebuild floor remains") &&
         expect(facade.document().objectCount() == 1U,
                "delete rebuild object count") &&
         expect(facade.document().revision() == revisionBeforeDelete + 1U,
                "delete rebuild revision advanced") &&
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "delete rebuild selection cleared") &&
         expect(window.creativeUiCommandKind == "delete_selected_object",
                "delete rebuild command kind") &&
         expect(window.creativeUiCommandDeleteObjectId == crate.objectId,
                "delete rebuild object id") &&
         expect(window.creativeUiCommandDeleteObjectKind == "Crate",
                "delete rebuild object kind") &&
         expect(window.creativeDocumentChangedThisFrame,
                "delete rebuild document changed") &&
         expect(window.creativeBakedRoomAutoRefreshRequested,
                "delete rebuild auto requested") &&
         expect(window.creativeBakedRoomAutoRefreshAccepted,
                "delete rebuild auto accepted") &&
         expect(!window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                "delete rebuild auto not cleared") &&
         expect(window.creativeBakedRoomAutoRefreshStatus ==
                    "product_creative_baked_room_refreshed",
                "delete rebuild auto status") &&
         expect(window.creativeBakedRoomAutoRefreshStaticMeshCount == 1U,
                "delete rebuild auto mesh count") &&
         expect(window.creativeBakedRoomAutoRefreshSpatialSurfaceCount == 1U,
                "delete rebuild auto surface count") &&
         expect(window.creativeBakedRoomAutoRefreshCollisionReady,
                "delete rebuild auto collision ready") &&
         expect(window.activeRoom.loaded,
                "delete rebuild active room loaded") &&
         expect(window.activeRoom.staticMeshCount == 1U,
                "delete rebuild active mesh count") &&
         expect(window.activeRoom.spatialSurfaceCount == 1U,
                "delete rebuild active surface count") &&
         expect(window.activeRoomCollision.ready,
                "delete rebuild active collision ready") &&
         expect(window.activeRoomCollision.querySurfaceCount == 1U,
                "delete rebuild collision query count") &&
         expect(!window.creativeBakedRoomStale,
                "delete rebuild stale fresh") &&
         expect(window.activeCreativeSaveId == launched.saveId,
                "delete rebuild active creative save preserved") &&
         expect(window.activeCreativeDocumentId == launched.documentId,
                "delete rebuild active creative document preserved") &&
         expect(window.activeProductSaveId == "none",
                "delete rebuild active product save unchanged") &&
         expect(dirtyBeforeDelete != 0U, "delete rebuild dirty before") &&
         expect(facade.document().dirtyFlags() != 0U,
                "delete rebuild dirty flags not drained");
}

bool autoRefreshMoveCommitAndIgnoresNoChangeReleaseThroughInputFrame() {
  const iggy3d::ProductAppOptions options =
      testOptions("auto_move_baked_room");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  cr::Facade& facade = app.facade;
  iggy3d::ProductWindowInputFrameState inputFrame;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Auto Move Baked Room",
                                        "2026-07-05T12:40:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  const cr::CreativeDocumentCreateReceipt floor =
      createBoundsObject(facade,
                         cr::CreativeObjectKind::Floor,
                         {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh =
      iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                    activeSession,
                                                    window,
                                                    app);
  const float initialCenterX =
      window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : window.activeRoom.room.staticMeshes.front().positionMeters.x;
  const float initialCenterY =
      window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : window.activeRoom.room.staticMeshes.front().positionMeters.y;
  const std::uint64_t revisionBeforeMove = facade.document().revision();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   0.0,
                   0.0,
                   targetId(floor.objectId))));
  inputFrame.creativePointerLifecycle.primaryButtonHeld = true;
  inputFrame.creativePointerLifecycle.lastPointerX = 0.0F;
  inputFrame.creativePointerLifecycle.lastPointerY = 0.0F;

  runCreativePointerLifecycleFrame(options,
                                   frontend,
                                   activeSession,
                                   window,
                                   app,
                                   inputFrame,
                                   false,
                                   60.0F,
                                   60.0F);

  const float movedCenterX =
      window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : window.activeRoom.room.staticMeshes.front().positionMeters.x;
  const float movedCenterY =
      window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : window.activeRoom.room.staticMeshes.front().positionMeters.y;

  bool ok = expect(launched.accepted, "auto move launch accepted") &&
            expect(floor.accepted, "auto move floor created") &&
            expect(initialRefresh.accepted, "auto move initial refresh") &&
            expect(window.activeRoom.loaded,
                   "auto move active room loaded after move") &&
            expect(facade.document().revision() == revisionBeforeMove + 1U,
                   "auto move revision advanced") &&
            expect(window.creativeDocumentChangedThisFrame,
                   "auto move document changed") &&
            expect(window.creativeBakedRoomAutoRefreshRequested,
                   "auto move auto requested") &&
            expect(window.creativeBakedRoomAutoRefreshAccepted,
                   "auto move auto accepted") &&
            expect(!window.creativeBakedRoomAutoRefreshClearedActiveRoom,
                   "auto move not cleared") &&
            expect(window.creativeBakedRoomAutoRefreshStatus ==
                       "product_creative_baked_room_refreshed",
                   "auto move refresh status") &&
            expect(window.activeRoom.staticMeshCount == 1U,
                   "auto move mesh count") &&
            expect(window.activeRoomCollision.ready,
                   "auto move collision ready") &&
            expect(window.activeRoomCollision.querySurfaceCount == 1U,
                   "auto move collision query count") &&
            expect(movedCenterX != initialCenterX || movedCenterY != initialCenterY,
                   "auto move baked mesh center changed") &&
            expect(cr::creativeUndoDepth(app.undoStack) == 1U,
                   "auto move undo depth after move") &&
            expect(window.creativeUndoAvailable,
                   "auto move window undo available after move") &&
            expect(window.creativeUndoDepth == 1U,
                   "auto move window undo depth after move") &&
            expect(!window.creativeBakedRoomStale,
                   "auto move stale cleared");

  const std::uint64_t revisionBeforeNoChange = facade.document().revision();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   6.0,
                   6.0,
                   targetId(floor.objectId))));
  inputFrame.creativePointerLifecycle.primaryButtonHeld = true;
  inputFrame.creativePointerLifecycle.lastPointerX = 60.0F;
  inputFrame.creativePointerLifecycle.lastPointerY = 60.0F;

  runCreativePointerLifecycleFrame(options,
                                   frontend,
                                   activeSession,
                                   window,
                                   app,
                                   inputFrame,
                                   false,
                                   60.0F,
                                   60.0F);

  ok &= expect(facade.document().revision() == revisionBeforeNoChange,
               "auto move no-change revision unchanged") &&
        expect(window.creativeDocumentRevisionObserved,
               "auto move no-change revision observed") &&
        expect(!window.creativeDocumentChangedThisFrame,
               "auto move no-change not changed") &&
        expect(!window.creativeBakedRoomAutoRefreshRequested,
               "auto move no-change no auto refresh") &&
        expect(cr::creativeUndoDepth(app.undoStack) == 1U,
               "auto move no-change undo depth unchanged") &&
        expect(window.creativeUndoDepth == 1U,
               "auto move no-change window undo depth unchanged") &&
        expect(window.activeRoom.loaded,
               "auto move no-change active room remains loaded") &&
        expect(!window.creativeBakedRoomStale,
               "auto move no-change remains fresh") &&
        expect(window.activeCreativeSaveId == launched.saveId,
               "auto move active creative save preserved") &&
        expect(window.activeProductSaveId == "none",
               "auto move active product save unchanged") &&
        expect(facade.document().dirtyFlags() != 0U,
               "auto move dirty flags not drained");

  const bool undoClicked = clickCreativeUndoThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);
  const float undoCenterX =
      window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : window.activeRoom.room.staticMeshes.front().positionMeters.x;
  const float undoCenterY =
      window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : window.activeRoom.room.staticMeshes.front().positionMeters.y;
  ok &= expect(undoClicked, "auto move undo clicked") &&
        expect(window.creativeUiCommandKind == "undo_last_document_change",
               "auto move undo command kind") &&
        expect(window.creativeUiCommandUndoAccepted,
               "auto move undo accepted") &&
        expect(window.creativeUiCommandUndoChanged,
               "auto move undo changed") &&
        expect(window.creativeUiCommandUndoDepthBefore == 1U,
               "auto move undo depth before") &&
        expect(window.creativeUiCommandUndoDepthAfter == 0U,
               "auto move undo depth after") &&
        expect(window.creativeDocumentChangedThisFrame,
               "auto move undo document changed") &&
        expect(window.creativeBakedRoomAutoRefreshRequested,
               "auto move undo auto refresh requested") &&
        expect(window.creativeBakedRoomAutoRefreshAccepted,
               "auto move undo auto refresh accepted") &&
        expect(window.activeRoom.loaded,
               "auto move undo active room loaded") &&
        expect(window.activeRoom.staticMeshCount == 1U,
               "auto move undo mesh count") &&
        expect(window.activeRoomCollision.ready,
               "auto move undo collision ready") &&
        expect(undoCenterX == initialCenterX && undoCenterY == initialCenterY,
               "auto move undo mesh center restored") &&
        expect(!cr::creativeUndoAvailable(app.undoStack),
               "auto move undo no redo stack") &&
        expect(window.creativeUndoDepth == 0U,
               "auto move undo window undo depth");

  return ok;
}

bool refreshCreativeBakedActiveRoomFailuresPreserveExistingRoomState() {
  {
    const iggy3d::ProductAppOptions options = testOptions("baked_room_inactive");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Baked Inactive",
                                          "2026-07-05T11:10:00Z"),
                            frontend,
                            activeSession,
                            window,
                            app);
    window.activeRoom = sentinelActiveRoom();
    window.activeRoomCollision = sentinelActiveRoomCollision();
    window.interactionMode = iggy3d::ProductInteractionMode::Player;

    const iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshed =
        iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                      activeSession,
                                                      window,
                                                      app);
    if (!expect(launched.accepted, "inactive bake setup launch accepted") ||
        !expect(!refreshed.accepted, "inactive bake rejected") ||
        !expect(refreshed.status == "product_creative_baked_room_inactive",
                "inactive bake status") ||
        !expect(!refreshed.bakeMeasured, "inactive bake not measured") ||
        !sentinelRoomStatePreserved(window)) {
      return false;
    }
  }

  {
    const iggy3d::ProductAppOptions options = testOptions("baked_room_no_session");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Baked Missing Session",
                                          "2026-07-05T11:20:00Z"),
                            frontend,
                            activeSession,
                            window,
                            app);
    window.activeRoom = sentinelActiveRoom();
    window.activeRoomCollision = sentinelActiveRoomCollision();
    activeSession.reset();

    const iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshed =
        iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                      activeSession,
                                                      window,
                                                      app);
    if (!expect(launched.accepted, "missing session setup launch accepted") ||
        !expect(!refreshed.accepted, "missing session bake rejected") ||
        !expect(refreshed.status ==
                    "product_creative_baked_room_session_missing",
                "missing session bake status") ||
        !expect(!refreshed.bakeMeasured, "missing session bake not measured") ||
        !sentinelRoomStatePreserved(window)) {
      return false;
    }
  }

  {
    const iggy3d::ProductAppOptions options = testOptions("baked_room_invalid_doc");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::CreativeAppState app;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Baked Invalid Document",
                                          "2026-07-05T11:30:00Z"),
                            frontend,
                            activeSession,
                            window,
                            app);
    window.activeRoom = sentinelActiveRoom();
    window.activeRoomCollision = sentinelActiveRoomCollision();
    app.facade.reset();

    const iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshed =
        iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                      activeSession,
                                                      window,
                                                      app);
    if (!expect(launched.accepted, "invalid doc setup launch accepted") ||
        !expect(app.facade.document().id() == cr::kInvalidDocumentId,
                "invalid doc setup facade reset") ||
        !expect(!refreshed.accepted, "invalid doc bake rejected") ||
        !expect(refreshed.status ==
                    "product_creative_baked_room_document_invalid",
                "invalid doc bake status") ||
        !expect(!refreshed.bakeMeasured, "invalid doc bake not measured") ||
        !sentinelRoomStatePreserved(window)) {
      return false;
    }
  }

  const iggy3d::ProductAppOptions options = testOptions("baked_room_empty_doc");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Baked Empty Document",
                                        "2026-07-05T11:40:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);
  window.activeRoom = sentinelActiveRoom();
  window.activeRoomCollision = sentinelActiveRoomCollision();

  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshed =
      iggy3d::refreshProductCreativeBakedActiveRoom({},
                                                    activeSession,
                                                    window,
                                                    app);

  return expect(launched.accepted, "empty bake setup launch accepted") &&
         expect(!refreshed.accepted, "empty bake rejected") &&
         expect(!refreshed.clearedActiveRoom,
                "empty bake default did not clear active room") &&
         expect(refreshed.status == "creative_room_bake_no_renderable_objects",
                "empty bake status mirrors RoomBake") &&
         expect(refreshed.bakeMeasured, "empty bake measured") &&
         expect(refreshed.bakedDocumentRevision == app.facade.document().revision(),
                "empty bake revision") &&
         expect(refreshed.bakeReceipt.requested, "empty bake requested") &&
         expect(!refreshed.bakeReceipt.accepted, "empty bake receipt rejected") &&
         expect(refreshed.bakeReceipt.objectCount == 0U,
                "empty bake object count") &&
         sentinelRoomStatePreserved(window);
}

// F0 (blank stage): the creative launch must NOT install the first_room demo
// room; window.activeRoom stays empty for a creative world.
bool creativeLaunchStandsOnBlankStageWithoutFirstRoomDemo() {
  const iggy3d::ProductAppOptions options = testOptions("blank_stage");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Blank Stage",
                                        "2026-07-05T09:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);

  return expect(launched.accepted, "blank stage launch accepted") &&
         expect(activeSession.has_value(), "blank stage session present") &&
         expect(window.gameplayActive, "blank stage gameplay active") &&
         expect(!window.activeRoom.loaded,
                "blank stage active room not loaded") &&
         expect(launched.bakedActiveRoomRefreshRequested,
                "blank stage refresh requested") &&
         expect(!launched.bakedActiveRoomRefreshAccepted,
                "blank stage refresh not accepted") &&
         expect(!launched.bakedActiveRoomRefresh.accepted,
                "blank stage refresh result rejected") &&
         expect(launched.bakedActiveRoomRefresh.status ==
                    "creative_room_bake_no_renderable_objects",
                "blank stage refresh status") &&
         expect(launched.bakedActiveRoomRefresh.reasonCode ==
                    "creative_room_bake_no_renderable_objects",
                "blank stage refresh reason") &&
         expect(launched.bakedActiveRoomRefresh.bakeMeasured,
                "blank stage refresh measured") &&
         expect(launched.bakedActiveRoomRefresh.bakedDocumentRevision ==
                    facade.document().revision(),
                "blank stage refresh revision") &&
         expect(launched.bakedActiveRoomRefresh.bakeReceipt.requested,
                "blank stage bake requested") &&
         expect(!launched.bakedActiveRoomRefresh.bakeReceipt.accepted,
                "blank stage bake rejected") &&
         expect(launched.bakedActiveRoomRefresh.bakeReceipt.objectCount == 0U,
                "blank stage bake object count") &&
         expect(!window.activeRoom.hasAuthoredRoom,
                "blank stage no authored demo room") &&
         expect(window.activeRoom.room.staticMeshes.empty(),
                "blank stage room has no demo meshes") &&
         expect(launched.objectCount == 0U, "blank stage empty document") &&
         expect(facade.document().objectCount() == 0U,
                "blank stage facade empty document") &&
         expect(window.startupPackageLoadStatus == "ok",
                "blank stage package load status ok") &&
         expect(window.startupRuntimeSessionCreateStatus ==
                    "startup_runtime_session_created",
                "blank stage session create status");
}

// F0: creative entry frames the fly camera on the world origin (elevated,
// pulled back, pitched down) so the origin ground grid is in view.
bool creativeLaunchFramesCameraOnOrigin() {
  const iggy3d::ProductAppOptions options = testOptions("origin_camera");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Origin Camera",
                                        "2026-07-05T09:10:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);

  return expect(launched.accepted, "origin camera launch accepted") &&
         expect(window.viewport.creativeFlyAnchorValid,
                "origin camera fly anchor valid") &&
         expect(window.viewport.creativeFlyPositionMeters.x == 0.0F,
                "origin camera anchor x at origin") &&
         expect(window.viewport.creativeFlyPositionMeters.y > 0.0F,
                "origin camera anchor elevated") &&
         expect(window.viewport.creativeFlyPositionMeters.z > 0.0F,
                "origin camera anchor pulled back") &&
         expect(window.viewport.cameraYawDegrees == 0.0F,
                "origin camera yaw faces origin") &&
         expect(window.viewport.cameraPitchDegrees < 0.0F,
                "origin camera pitched down toward ground");
}

// F0: the ground grid is present in the projected frame for a creative world.
bool creativeFrameShowsGroundGrid() {
  const iggy3d::ProductAppOptions options = testOptions("ground_grid");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Ground Grid",
                                        "2026-07-05T09:20:00Z"),
                          frontend,
                          activeSession,
                          window,
                          app);

  iggy3d::FrontendState gameplayFrontend;
  gameplayFrontend.screen = iggy3d::FrontendScreen::Gameplay;
  gameplayFrontend.childScreen = iggy3d::FrontendScreen::Gameplay;
  gameplayFrontend.status = "gameplay_active";
  iggy3d::refreshProductGameplayProjectionMetrics(
      iggy3d::ProductGameplayProjectionRefreshRequest{
          activeSession, window, false, false,
          iggy3d::ProductRendererRequest::Vulkan, gameplayFrontend});

  return expect(launched.accepted, "ground grid launch accepted") &&
         expect(iggy3d::productCreativeDocumentEditorActiveForWindow(window),
                "ground grid creative document active") &&
         expect(window.mapMakerGridVisible,
                "ground grid map maker grid visible") &&
         expect(window.mapMakerGridDotCount > 0U,
                "ground grid dot count nonzero") &&
         expect(window.viewport.productDrawMapMakerGridVisible,
                "ground grid draw list grid visible") &&
         expect(window.viewport.productDrawMapMakerGridDotCount > 0U,
                "ground grid draw list dot count nonzero") &&
         expect(!iggy3d::productMapMakerLiveForWindow(gameplayFrontend, window),
                "ground grid not legacy map maker surface");
}

}  // namespace

int main() {
  const bool ok =
      successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode() &&
      blankTitleOrTimestampRejectsBeforeSessionInstallAndModeSwitch() &&
      invalidAttemptTokenWritesNoCommittedSaveAndDoesNotEnterCreativeMode() &&
      secondLaunchClearsOldFacadeStateAndInstallsNewDocument() &&
      openLaunchRestoresSavedCreativeDocumentAndEntersCreativeMode() &&
      openLaunchRefreshesBakedActiveRoomFromSavedCreativeDocument() &&
      openBlankOrInvalidSaveIdRejectsBeforeSessionInstallAndModeSwitch() &&
      openProductSessionSaveRejectsAsMissingCreativeSection() &&
      productNewWorldLaunchClearsActiveCreativeIdentity() &&
      currentCreativeWorldSaveDrainsDirtyAndPersistsDocument() &&
      currentCreativeWorldSaveRejectsInvalidContextsWithoutDrain() &&
      pauseCreativeSaveWritesCreativeDocumentAndKeepsSession() &&
      pauseCreativeSaveNullFacadeFailsClosed() &&
      pauseCreativeSaveAndExitWritesReturnsTitleAndClearsIdentity() &&
      pauseCreativeSaveAndExitFailureKeepsSessionAndDirtyState() &&
      pauseCreativeReturnToTitleClearsUndoStack() &&
      secondOpenClearsOldFacadeStateAndInstallsRestoredDocument() &&
      refreshCreativeBakedActiveRoomBuildsRoomCollisionAndProjection() &&
      manualRebuildRoomCommandRefreshesBakedActiveRoomThroughInputFrame() &&
      manualRebuildRoomCommandClearsRoomStateOnNoRenderableDocument() &&
      disabledUndoRowDoesNotRouteThroughInputFrame() &&
      undoAfterCreateCrateRestoresEmptyDocumentThroughInputFrame() &&
      undoAfterDeleteSelectedRestoresRenderableThroughInputFrame() &&
      undoAfterVisibilityToggleRestoresBakedRoomThroughInputFrame() &&
      autoRefreshVisibilityToggleClearsAndRestoresBakedRoomThroughInputFrame() &&
      deleteSelectedRenderableClearsBakedRoomThroughInputFrame() &&
      deleteOneOfTwoRenderablesRebuildsRemainingBakedRoomThroughInputFrame() &&
      autoRefreshMoveCommitAndIgnoresNoChangeReleaseThroughInputFrame() &&
      refreshCreativeBakedActiveRoomFailuresPreserveExistingRoomState() &&
      creativeLaunchStandsOnBlankStageWithoutFirstRoomDemo() &&
      creativeLaunchFramesCameraOnOrigin() &&
      creativeFrameShowsGroundGrid();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
