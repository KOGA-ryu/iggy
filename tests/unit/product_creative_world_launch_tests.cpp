#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/ui/UiDrawList.hpp"
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

iggy3d::ProductUiDrawList creativeUiDrawListForFacade(cr::Facade& facade) {
  const cr::CreativeUiBuildReceipt ui = facade.buildUiModel();
  iggy3d::ProductCreativeUiDrawListRequest request;
  request.model = &ui.model;
  return iggy3d::buildProductCreativeUiDrawList(request);
}

bool clickCreativeRebuildRoomThroughInputFrame(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::CreativeAppState& app) {
  iggy3d::ProductSaveBridgeResult saves;
  iggy3d::FrontendSettingsTab settingsTab = iggy3d::FrontendSettingsTab::Input;
  iggy3d::WorldSetupDraft worldSetupDraft;
  iggy3d::FrontendSettings settings;
  iggy3d::ProductWindowInputFrameState inputFrame;
  bool closeRequested = false;

  const iggy3d::ProductUiDrawList drawList =
      creativeUiDrawListForFacade(app.facade);
  const iggy3d::UiHitRegion* rebuildHit =
      findHitRegion(drawList, "creative.row.tools.rebuild_room");
  if (rebuildHit == nullptr) {
    return false;
  }

  iggy3d::ProductWindowInputClickOverride clickOverride;
  clickOverride.enabled = true;
  clickOverride.click = clickAt(rebuildHit->rect.x, rebuildHit->rect.y);

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
  const bool activeRoomLoadedBeforeCommand = window.activeRoom.loaded;

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

bool manualRebuildRoomCommandPreservesRoomStateOnNoRenderableDocument() {
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
  window.activeRoom = sentinelActiveRoom();
  window.activeRoomCollision = sentinelActiveRoomCollision();

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
         expect(!window.creativeUiCommandBakedRoomRefreshAccepted,
                "manual empty rebuild refresh rejected") &&
         expect(window.creativeUiCommandBakedRoomRefreshStatus ==
                    "creative_room_bake_no_renderable_objects",
                "manual empty rebuild refresh status") &&
         expect(window.creativeUiCommandBakedRoomRefreshReasonCode ==
                    "creative_room_bake_no_renderable_objects",
                "manual empty rebuild refresh reason") &&
         expect(window.creativeUiCommandBakedRoomStaticMeshCount == 0U,
                "manual empty rebuild static mesh count") &&
         expect(window.creativeUiCommandBakedRoomAnchorCount == 0U,
                "manual empty rebuild anchor count") &&
         expect(window.creativeUiCommandBakedRoomSpatialSurfaceCount == 0U,
                "manual empty rebuild spatial surface count") &&
         expect(!window.creativeUiCommandBakedRoomCollisionReady,
                "manual empty rebuild collision not ready") &&
         sentinelRoomStatePreserved(window);
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
         expect(refreshed.status == "creative_room_bake_no_renderable_objects",
                "empty bake status mirrors RoomBake") &&
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
      secondOpenClearsOldFacadeStateAndInstallsRestoredDocument() &&
      refreshCreativeBakedActiveRoomBuildsRoomCollisionAndProjection() &&
      manualRebuildRoomCommandRefreshesBakedActiveRoomThroughInputFrame() &&
      manualRebuildRoomCommandPreservesRoomStateOnNoRenderableDocument() &&
      refreshCreativeBakedActiveRoomFailuresPreserveExistingRoomState() &&
      creativeLaunchStandsOnBlankStageWithoutFirstRoomDemo() &&
      creativeLaunchFramesCameraOnOrigin() &&
      creativeFrameShowsGroundGrid();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
