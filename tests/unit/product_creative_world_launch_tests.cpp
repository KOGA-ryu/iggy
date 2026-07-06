#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/ui/UiDrawList.hpp"
#include "app/iggy3d/creative/ui/UiProjection.hpp"
#include "app/iggy3d/gameplay/ProjectionRefresh.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"
#include "app/iggy3d/save/Flow.hpp"
#include "app/iggy3d/window/InputFrame.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/RenderDiagnostics.hpp"
#include "runtime/ai/ReasoningGraph.hpp"

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

bool expectActiveCreativeIdentityMirrorsWindow(
    const cr::CreativeActiveIdentity& identity,
    const iggy3d::ProductAppWindowState& window,
    std::string_view label) {
  const std::string prefix{label};
  bool ok = true;
  ok &= expect(identity.saveId == window.activeCreative.saveId,
               prefix + " identity save id mirrors window");
  ok &= expect(identity.savePath == window.activeCreative.savePath,
               prefix + " identity save path mirrors window");
  ok &= expect(identity.worldId == window.activeCreative.worldId,
               prefix + " identity world id mirrors window");
  ok &= expect(identity.documentId == window.activeCreative.documentId,
               prefix + " identity document id mirrors window");
  ok &= expect(identity.objectCount == window.activeCreative.objectCount,
               prefix + " identity object count mirrors window");
  ok &= expect(identity.nextObjectId == window.activeCreative.nextObjectId,
               prefix + " identity next object id mirrors window");
  ok &= expect(identity.saveStatus == window.activeCreative.saveStatus,
               prefix + " identity save status mirrors window");
  ok &= expect(identity.saveReasonCode ==
                   window.activeCreative.saveReasonCode,
               prefix + " identity save reason mirrors window");
  ok &= expect(identity.saveDirtyFlagsBefore ==
                   window.activeCreative.saveDirtyFlagsBefore,
               prefix + " identity dirty before mirrors window");
  ok &= expect(identity.saveDirtyFlagsDrained ==
                   window.activeCreative.saveDirtyFlagsDrained,
               prefix + " identity dirty drained mirrors window");
  ok &= expect(identity.saveDirtyFlagsAfter ==
                   window.activeCreative.saveDirtyFlagsAfter,
               prefix + " identity dirty after mirrors window");
  ok &= expect(identity.saveSavedAtUtc ==
                   window.activeCreative.saveSavedAtUtc,
               prefix + " identity saved timestamp mirrors window");
  return ok;
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

cr::CreativeDocumentCreateReceipt createDefaultFloor(cr::Facade& facade) {
  return createBoundsObject(facade,
                            cr::CreativeObjectKind::Floor,
                            {{0.0, 0.0, 0.0}, {4.0, 0.25, 4.0}});
}

cr::CreativeDocumentCreateReceipt createDefaultWall(cr::Facade& facade) {
  return createBoundsObject(facade,
                            cr::CreativeObjectKind::Wall,
                            {{5.0, 0.0, 0.0}, {9.0, 2.5, 0.25}});
}

cr::CreativeDocumentCreateReceipt createDefaultCrate(cr::Facade& facade) {
  return createBoundsObject(facade,
                            cr::CreativeObjectKind::Crate,
                            {{1.0, 0.0, 5.0}, {2.0, 1.0, 6.0}});
}

cr::CreativeDocumentCreateReceipt createDefaultBeam(cr::Facade& facade) {
  return createBoundsObject(facade,
                            cr::CreativeObjectKind::Beam,
                            {{0.0, 1.0, 0.0}, {4.0, 1.35, 0.35}});
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

cr::CreativeDocumentCreateReceipt createDefaultPointLight(cr::Facade& facade) {
  return createPointObject(facade,
                           cr::CreativeObjectKind::PointLight,
                           {6.25, 1.5, -2.75});
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

struct BakedCreativeProofObjects {
  cr::CreativeDocumentCreateReceipt floor;
  cr::CreativeDocumentCreateReceipt wall;
  cr::CreativeDocumentCreateReceipt crate;
  cr::CreativeDocumentCreateReceipt beam;
  cr::CreativeDocumentCreateReceipt point;
  cr::CreativeDocumentCreateReceipt path;
};

BakedCreativeProofObjects createBakedCreativeProofObjects(
    cr::Facade& facade) {
  BakedCreativeProofObjects objects;
  objects.floor = createDefaultFloor(facade);
  objects.wall = createDefaultWall(facade);
  objects.crate = createDefaultCrate(facade);
  objects.beam = createDefaultBeam(facade);
  objects.point = createDefaultPointLight(facade);
  objects.path = createPatrolRoute(facade);
  return objects;
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

void installSentinelRoomState(iggy3d::ProductAppWindowState& window) {
  window.activeRoom = sentinelActiveRoom();
  window.activeRoomCollision = sentinelActiveRoomCollision();
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

void markCreativeBakedRoomStale(iggy3d::ProductAppWindowState& window,
                                const cr::CreativeDocument& document) {
  window.creativeBakedRoomStale = true;
  window.creativeBakedRoomStaleDocumentId = document.id();
  window.creativeBakedRoomStaleRevision = document.revision();
  window.creativeBakedRoomStaleStatus =
      "creative_baked_room_stale_document_changed";
  window.creativeBakedRoomStaleReasonCode =
      "creative_baked_room_stale_document_changed";
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
  clickOverride.click = clickAt(hit->rect.x + hit->rect.width * 0.5F,
                                hit->rect.y + hit->rect.height * 0.5F);

  iggy3d::processProductWindowInputFrame(iggy3d::ProductWindowInputFrameContext{
      .frontend = frontend,
      .saves = saves,
      .options = options,
      .settingsTab = settingsTab,
      .activeSession = activeSession,
      .worldSetupDraft = worldSetupDraft,
      .window = window,
      .settings = settings,
      .inputFrame = inputFrame,
      .closeRequested = closeRequested,
      .sdlWindow = nullptr,
      .creativeApp = &app,
      .creativeUiDrawList = &drawList,
      .creativeViewportPickViewport = {},
      .creativeViewportPickProjectionRequest = {},
      .creativeViewportPickZ = 0,
      .creativeViewportPickDepthMode =
          cr::CreativeViewportPickDepthMode::FixedZ,
      .clickOverride = clickOverride,
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

bool clickCreativeGenerateRoomShellThroughInputFrame(
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
      "creative.row.selection.generate_room_shell");
}

bool clickCreativeRemoveRoomShellThroughInputFrame(
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
      "creative.row.selection.remove_room_shell");
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
         expect(window.startup.packageLookupMeasured,
                "creative launch package lookup measured") &&
         expect(window.startup.packageLookupStatus ==
                    "startup_package_lookup_resolved",
                "creative launch package lookup status") &&
         expect(window.startup.packageLoadMeasured,
                "creative launch package load measured") &&
         expect(window.startup.packageLoadStatus == "ok",
                "creative launch package load status") &&
         expect(window.startup.runtimeSessionCreateMeasured,
                "creative launch session create measured") &&
         expect(window.startup.runtimeSessionCreateStatus ==
                    "startup_runtime_session_created",
                "creative launch session create status") &&
         expect(window.startup.creativeWorldIdScanMeasured,
                "creative launch world id scan measured") &&
         expect(window.startup.creativeWorldIdScanStatus ==
                    "product_world_id_scan_ready",
                "creative launch world id scan status") &&
         expect(window.startup.creativeDocumentIdScanMeasured,
                "creative launch document id scan measured") &&
         expect(window.startup.creativeDocumentIdScanStatus ==
                    "creative_document_id_scan_ready",
                "creative launch document id scan status") &&
         expect(window.activeProductSaveId == "none",
                "creative launch does not set product save id") &&
         expect(window.activeCreative.saveId == launched.saveId,
                "creative launch active creative save id") &&
         expect(window.activeCreative.savePath == launched.path.generic_string(),
                "creative launch active creative save path") &&
         expect(window.activeCreative.worldId == launched.worldId,
                "creative launch active creative world id") &&
         expect(window.activeCreative.documentId == launched.documentId,
                "creative launch active creative document id") &&
         expect(window.activeCreative.objectCount == launched.objectCount,
                "creative launch active creative object count") &&
         expect(window.activeCreative.nextObjectId == launched.nextObjectId,
                "creative launch active creative next id") &&
         expect(window.activeCreative.saveStatus ==
                    "creative_world_save_not_requested",
                "creative launch active creative save status") &&
         expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "creative launch") &&
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
  window.creativeUndo.available = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndo.depth = cr::creativeUndoDepth(app.undoStack);

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
         expect(!window.creativeUndo.available,
                "second launch window undo unavailable") &&
         expect(window.creativeUndo.depth == 0U,
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
         expect(openWindow.activeCreative.saveId == opened.saveId,
                "open launch active creative save id") &&
         expect(openWindow.activeCreative.savePath == opened.path.generic_string(),
                "open launch active creative save path") &&
         expect(openWindow.activeCreative.worldId == opened.worldId,
                "open launch active creative world id") &&
         expect(openWindow.activeCreative.documentId == opened.documentId,
                "open launch active creative document id") &&
         expect(openWindow.activeCreative.objectCount == opened.objectCount,
                "open launch active creative object count") &&
         expect(openWindow.activeCreative.nextObjectId == opened.nextObjectId,
                "open launch active creative next id") &&
         expectActiveCreativeIdentityMirrorsWindow(openApp.identity,
                                                   openWindow,
                                                   "open launch") &&
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
  const BakedCreativeProofObjects objects =
      createBakedCreativeProofObjects(createFacade);
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
         expect(objects.floor.accepted, "open baked setup floor created") &&
         expect(objects.wall.accepted, "open baked setup wall created") &&
         expect(objects.crate.accepted, "open baked setup crate created") &&
         expect(objects.beam.accepted, "open baked setup beam created") &&
         expect(objects.point.accepted, "open baked setup point created") &&
         expect(objects.path.accepted, "open baked setup path created") &&
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
         expect(openWindow.activeCreative.saveId == opened.saveId,
                "open baked active creative save id") &&
         expect(openWindow.activeCreative.documentId == opened.documentId,
                "open baked active creative document id") &&
         expect(openWindow.activeCreative.objectCount == opened.objectCount,
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
  const std::string creativeSaveId = window.activeCreative.saveId;

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
         expect(window.activeCreative.saveId == "none",
                "product clear active creative save id") &&
         expect(window.activeCreative.savePath == "none",
                "product clear active creative save path") &&
         expect(window.activeCreative.worldId == "none",
                "product clear active creative world id") &&
         expect(window.activeCreative.documentId == cr::kInvalidDocumentId,
                "product clear active creative document id") &&
         expect(window.activeCreative.objectCount == 0U,
                "product clear active creative object count") &&
         expect(window.activeCreative.nextObjectId == cr::kInvalidObjectId,
                "product clear active creative next id") &&
         expect(window.activeCreative.saveStatus ==
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
  window.creativeUndo.available = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndo.depth = cr::creativeUndoDepth(app.undoStack);

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
         expect(!window.creativeUndo.available,
                "current save window undo unavailable") &&
         expect(window.creativeUndo.depth == 0U,
                "current save window undo depth") &&
         expect(window.activeCreative.saveId == launched.saveId,
                "current save active id") &&
         expect(window.activeCreative.savePath == launched.path.generic_string(),
                "current save active path") &&
         expect(window.activeCreative.worldId == launched.worldId,
                "current save active world id") &&
         expect(window.activeCreative.documentId == launched.documentId,
                "current save active document id") &&
         expect(window.activeCreative.objectCount == 1U,
                "current save active object count") &&
         expect(window.activeCreative.nextObjectId ==
                    facade.document().nextObjectId(),
                "current save active next object id") &&
         expect(window.activeCreative.saveStatus ==
                    "product_creative_world_saved",
                "current save active status") &&
         expect(window.activeCreative.saveReasonCode ==
                    "product_creative_world_saved",
                "current save active reason") &&
         expect(window.activeCreative.saveDirtyFlagsBefore == dirtyBefore,
                "current save active dirty before") &&
         expect(window.activeCreative.saveDirtyFlagsDrained == dirtyBefore,
                "current save active dirty drained") &&
         expect(window.activeCreative.saveDirtyFlagsAfter == 0U,
                "current save active dirty after") &&
         expect(window.activeCreative.saveSavedAtUtc != "none",
                "current save saved timestamp") &&
         expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "current save") &&
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
    app.identity.saveId = "none";
    iggy3d::mirrorProductActiveCreativeIdentity(app.identity, window);

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
        !expect(window.activeCreative.saveStatus ==
                    "product_creative_save_id_missing",
                "missing id window status") ||
        !expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "missing id save")) {
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
        !expect(window.activeCreative.saveStatus ==
                    "product_creative_save_inactive",
                "inactive window status") ||
        !expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "inactive save")) {
      return false;
    }
  }

  const iggy3d::ProductAppOptions options = testOptions("current_save_invalid_doc");
  iggy3d::ProductAppWindowState window;
  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  cr::CreativeAppState app;
  app.identity.saveId = "save_001";
  app.identity.worldId = "world_0001";
  iggy3d::mirrorProductActiveCreativeIdentity(app.identity, window);
  [[maybe_unused]] cr::Facade& facade = app.facade;
  const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
      iggy3d::saveProductCurrentCreativeWorld(options, app, "unit", window);

  return expect(!saved.accepted, "invalid doc rejected") &&
         expect(saved.status == "product_creative_save_document_id_missing",
                "invalid doc status") &&
         expect(saved.dirtyFlagsBefore == 0U, "invalid doc dirty before") &&
         expect(saved.dirtyFlagsAfter == 0U, "invalid doc dirty after") &&
         expect(window.activeCreative.saveStatus ==
                    "product_creative_save_document_id_missing",
                "invalid doc window status") &&
         expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "invalid doc save");
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
         expect(window.activeCreative.saveStatus ==
                    "product_creative_world_saved",
                "pause creative save active status") &&
         expect(window.activeCreative.saveDirtyFlagsBefore == dirtyBefore,
                "pause creative save dirty before mirrored") &&
         expect(window.activeCreative.saveDirtyFlagsDrained == dirtyBefore,
                "pause creative save dirty drained mirrored") &&
         expect(window.activeCreative.saveDirtyFlagsAfter == 0U,
                "pause creative save dirty after mirrored") &&
         expect(facade.document().dirtyFlags() == 0U,
                "pause creative save facade dirty drained") &&
         expect(activeSession.has_value(),
                "pause creative save keeps active session") &&
         expect(window.interactionMode == iggy3d::ProductInteractionMode::Creative,
                "pause creative save remains creative") &&
         expect(window.activeProductSaveId == "none",
                "pause creative save does not set product save id") &&
         expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "pause creative save") &&
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
         expect(window.activeCreative.saveStatus ==
                    "product_creative_save_facade_missing",
                "pause creative null active status") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "pause creative null dirty preserved") &&
         expect(activeSession.has_value(),
                "pause creative null keeps active session") &&
         expect(window.activeProductSaveId == "none",
                "pause creative null does not set product save id");
}

bool pauseSaveUsesCreativeIdentityInsteadOfStaleWindowMirror() {
  const iggy3d::ProductAppOptions options =
      testOptions("pause_stale_creative_identity");
  iggy3d::FrontendState frontend;
  iggy3d::FrontendSettings settings;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;

  window.interactionMode = iggy3d::ProductInteractionMode::Creative;
  window.activeCreative.saveId = "stale_save";
  window.activeCreative.worldId = "stale_world";
  window.activeCreative.documentId = 42U;
  window.activeCreative.objectCount = 3U;
  window.activeCreative.nextObjectId = 7U;

  const iggy3d::ProductPauseSaveFlowResult saved =
      iggy3d::executeProductPauseSaveFlow(
          iggy3d::ProductPauseSaveFlowKind::Save,
          options,
          frontend,
          activeSession,
          window,
          settings,
          &app);

  return expect(!app.identity.worldActive(),
                "stale mirror setup has inactive source identity") &&
         expect(!saved.creativeSaveRequested,
                "stale mirror does not request creative save") &&
         expect(saved.write.status == "product_save_session_missing",
                "stale mirror routes to product save fallback") &&
         expect(saved.launchStatus == "product_save_session_missing",
                "stale mirror launch status from product save") &&
         expect(window.activeCreative.saveStatus ==
                    "creative_world_save_not_requested",
                "stale mirror leaves creative save mirror unchanged") &&
         expect(window.activeCreative.saveId == "stale_save",
                "stale mirror compatibility field preserved");
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
         expect(window.activeCreative.saveId == "none",
                "pause creative save exit clears active creative id") &&
         expect(window.activeCreative.saveStatus ==
                    "creative_world_save_not_requested",
                "pause creative save exit clears save status") &&
         expect(facade.document().dirtyFlags() == 0U,
                "pause creative save exit dirty drained") &&
         expect(window.activeProductSaveId == "none",
                "pause creative save exit does not set product save id") &&
         expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "pause creative save exit") &&
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
  app.identity.saveId = "none";
  iggy3d::mirrorProductActiveCreativeIdentity(app.identity, window);
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
         expect(window.activeCreative.saveStatus ==
                    "product_creative_save_id_missing",
                "pause creative save exit failure active status") &&
         expectActiveCreativeIdentityMirrorsWindow(
             app.identity,
             window,
             "pause creative save exit failure");
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
  window.creativeUndo.available = cr::creativeUndoAvailable(app.undoStack);
  window.creativeUndo.depth = cr::creativeUndoDepth(app.undoStack);

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
         expect(!window.creativeUndo.available,
                "pause return window undo unavailable") &&
         expect(window.creativeUndo.depth == 0U,
                "pause return window undo depth") &&
         expect(window.activeCreative.saveId == "none",
                "pause return active creative save id cleared") &&
         expect(window.activeCreative.documentId == cr::kInvalidDocumentId,
                "pause return active creative document id cleared") &&
         expectActiveCreativeIdentityMirrorsWindow(app.identity,
                                                   window,
                                                   "pause return");
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
  openWindow.creativeUndo.available =
      cr::creativeUndoAvailable(openApp.undoStack);
  openWindow.creativeUndo.depth = cr::creativeUndoDepth(openApp.undoStack);

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
         expect(!openWindow.creativeUndo.available,
                "second open window undo unavailable") &&
         expect(openWindow.creativeUndo.depth == 0U,
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
  const BakedCreativeProofObjects objects =
      createBakedCreativeProofObjects(facade);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  bool activationHookCalled = false;
  std::uint64_t activationHookDocumentId = 0;
  std::size_t activationHookStaticMeshCount = 0;
  iggy3d::ProductCreativeBakedActiveRoomRefreshRequest refreshRequest;
  refreshRequest.activationHook =
      [&](iggy3d::Session& session,
          const iggy3d::RoomAsset& room,
          const cr::CreativeDocument& document) {
        activationHookCalled = true;
        activationHookDocumentId = document.id();
        activationHookStaticMeshCount = room.staticMeshes.size();
        iggy3d::ReasoningGraph graph;
        graph.nodes.push_back({7U,
                               iggy3d::ReasoningNodeKind::reference,
                               {1.0F, 2.0F, 3.0F},
                               "e97_activation_hook"});
        session.setReasoningGraph(graph);
      };

  const iggy3d::ProductCreativeBakedActiveRoomRefreshResult refreshed =
      iggy3d::refreshProductCreativeBakedActiveRoom(refreshRequest,
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
         expect(objects.floor.accepted, "baked room floor created") &&
         expect(objects.wall.accepted, "baked room wall created") &&
         expect(objects.crate.accepted, "baked room crate created") &&
         expect(objects.beam.accepted, "baked room beam created") &&
         expect(objects.point.accepted, "baked room point created") &&
         expect(objects.path.accepted, "baked room path created") &&
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
         expect(activationHookCalled, "baked room activation hook called") &&
         expect(activationHookDocumentId == launched.documentId,
                "baked room activation hook document") &&
         expect(activationHookStaticMeshCount == 4U,
                "baked room activation hook mesh count") &&
         expect(activeSession.has_value() &&
                    activeSession->state().reasoningGraph.nodes.size() == 1U,
                "baked room activation hook graph node count") &&
         expect(activeSession.has_value() &&
                    activeSession->state().reasoningGraph.nodes[0].id == 7U,
                "baked room activation hook graph node id") &&
         expect(activeSession.has_value() &&
                    activeSession->state().reasoningGraph.nodes[0].sourceLabel ==
                        "e97_activation_hook",
                "baked room activation hook graph source") &&
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
         expect(window.activeCreative.saveId == launched.saveId,
                "baked room active creative save id preserved") &&
         expect(window.activeCreative.documentId == launched.documentId,
                "baked room active creative document id preserved") &&
         expect(window.activeCreative.objectCount == launched.objectCount,
                "baked room active creative object count unchanged") &&
         expect(window.activeProductSaveId == "none",
                "baked room active product save id unchanged") &&
         expect(facade.document().dirtyFlags() == dirtyBefore,
                "baked room dirty flags preserved");
}

struct ManualRebuildRoomScenario {
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  iggy3d::ProductCreativeNewWorldLaunchResult launched;
  BakedCreativeProofObjects objects;
  cr::CreativeObjectDirtyFlags dirtyBefore = 0;
  std::uint64_t undoDepthBeforeCommand = 0;
  bool activeRoomLoadedBeforeCommand = false;
  bool clicked = false;
  iggy3d::SceneProjectionResult projection;
};

ManualRebuildRoomScenario runManualRebuildRoomScenario(std::string_view name) {
  ManualRebuildRoomScenario scenario;
  scenario.options = testOptions(name);
  scenario.launched =
      launchCreativeWorld(scenario.options,
                          launchRequest("Manual Baked Active Room",
                                        "2026-07-05T12:10:00Z"),
                          scenario.frontend,
                          scenario.activeSession,
                          scenario.window,
                          scenario.app);
  cr::Facade& facade = scenario.app.facade;
  scenario.objects = createBakedCreativeProofObjects(facade);
  scenario.dirtyBefore = facade.document().dirtyFlags();
  cr::pushCreativeUndoSnapshot(scenario.app.undoStack, facade.document());
  scenario.undoDepthBeforeCommand =
      cr::creativeUndoDepth(scenario.app.undoStack);
  scenario.window.creativeUndo.available =
      cr::creativeUndoAvailable(scenario.app.undoStack);
  scenario.window.creativeUndo.depth = scenario.undoDepthBeforeCommand;
  scenario.activeRoomLoadedBeforeCommand = scenario.window.activeRoom.loaded;
  markCreativeBakedRoomStale(scenario.window, facade.document());

  scenario.clicked = clickCreativeRebuildRoomThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app);
  scenario.projection =
      scenario.activeSession.has_value()
          ? iggy3d::buildSceneProjection(scenario.activeSession->state(),
                                         &scenario.window.activeRoom.room)
          : iggy3d::SceneProjectionResult{};
  return scenario;
}

bool manualRebuildRoomCommandReportsRefreshThroughInputFrame() {
  ManualRebuildRoomScenario scenario =
      runManualRebuildRoomScenario("manual_baked_room_rebuild_command");
  cr::Facade& facade = scenario.app.facade;

  return expect(scenario.launched.accepted,
                "manual rebuild setup launch accepted") &&
         expect(scenario.activeSession.has_value(),
                "manual rebuild active session") &&
         expect(!scenario.launched.bakedActiveRoomRefreshAccepted,
                "manual rebuild launch starts blank") &&
         expect(!scenario.activeRoomLoadedBeforeCommand,
                "manual rebuild active room starts blank") &&
         expect(scenario.objects.floor.accepted,
                "manual rebuild floor created") &&
         expect(scenario.objects.wall.accepted,
                "manual rebuild wall created") &&
         expect(scenario.objects.crate.accepted,
                "manual rebuild crate created") &&
         expect(scenario.objects.beam.accepted,
                "manual rebuild beam created") &&
         expect(scenario.objects.point.accepted,
                "manual rebuild point created") &&
         expect(scenario.objects.path.accepted,
                "manual rebuild path created") &&
         expect(scenario.dirtyBefore != 0U,
                "manual rebuild dirty before command") &&
         expect(scenario.clicked,
                "manual rebuild row clicked through input frame") &&
         expect(scenario.window.creativeUiInput.consumed,
                "manual rebuild ui input consumed") &&
         expect(scenario.window.creativeUiInput.semanticId ==
                    "creative.row.tools.rebuild_room",
                "manual rebuild ui semantic") &&
         expect(scenario.window.creativeUiCommand.accepted,
                "manual rebuild command accepted") &&
         expect(!scenario.window.creativeUiCommand.changed,
                "manual rebuild command no document change") &&
         expect(scenario.window.creativeUiCommand.kind == "rebuild_room",
                "manual rebuild command kind") &&
         expect(scenario.window.creativeUiCommand.status ==
                    "product_creative_ui_command_rebuild_room_requested",
                "manual rebuild command status") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.requested,
                "manual rebuild refresh requested") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.accepted,
                "manual rebuild refresh accepted") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.status ==
                    "product_creative_baked_room_refreshed",
                "manual rebuild refresh status") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.bakeMeasured,
                "manual rebuild refresh measured") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.bakedDocumentRevision ==
                    facade.document().revision(),
                "manual rebuild refresh revision") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.staticMeshCount == 4U,
                "manual rebuild refresh static mesh count") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.anchorCount == 1U,
                "manual rebuild refresh anchor count") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.spatialSurfaceCount == 7U,
                "manual rebuild refresh spatial surface count") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.collisionReady,
                "manual rebuild refresh collision ready") &&
         expect(scenario.window.creativeUiCommand.bakedRoomRefresh.collisionQuerySurfaceCount == 7U,
                "manual rebuild refresh collision query count") &&
         expect(scenario.window.creativeDocumentRevision.observed,
                "manual rebuild revision observed") &&
         expect(!scenario.window.creativeDocumentChangedThisFrame,
                "manual rebuild no document mutation") &&
         expect(cr::creativeUndoDepth(scenario.app.undoStack) ==
                    scenario.undoDepthBeforeCommand,
                "manual rebuild undo depth preserved") &&
         expect(scenario.window.creativeUndo.available,
                "manual rebuild window undo available preserved") &&
         expect(scenario.window.creativeUndo.depth ==
                    scenario.undoDepthBeforeCommand,
                "manual rebuild window undo depth preserved") &&
         expect(!scenario.window.creativeBakedRoomStale,
                "manual rebuild clears stale on accepted refresh") &&
         expect(scenario.window.creativeBakedRoomStaleDocumentId ==
                    facade.document().id(),
                "manual rebuild stale doc id fresh") &&
         expect(scenario.window.creativeBakedRoomStaleRevision ==
                    facade.document().revision(),
                "manual rebuild stale revision fresh") &&
         expect(scenario.window.creativeBakedRoomStaleStatus ==
                    "creative_baked_room_fresh",
                "manual rebuild stale status fresh");
}

bool manualRebuildRoomCommandLoadsActiveRoomThroughInputFrame() {
  ManualRebuildRoomScenario scenario =
      runManualRebuildRoomScenario("manual_baked_room_rebuild_active_room");
  cr::Facade& facade = scenario.app.facade;

  return expect(scenario.launched.accepted,
                "manual rebuild active setup launch accepted") &&
         expect(scenario.clicked,
                "manual rebuild active row clicked") &&
         expect(scenario.window.activeRoom.loaded,
                "manual rebuild active room loaded") &&
         expect(scenario.window.activeRoom.staticMeshCount == 4U,
                "manual rebuild active mesh count") &&
         expect(scenario.window.activeRoom.anchorCount == 1U,
                "manual rebuild active anchor count") &&
         expect(scenario.window.activeRoom.spatialSurfaceCount == 7U,
                "manual rebuild active surface count") &&
         expect(scenario.window.activeRoomCollision.ready,
                "manual rebuild collision ready") &&
         expect(scenario.window.activeRoomCollision.querySurfaceCount == 7U,
                "manual rebuild collision query count") &&
         expect(scenario.projection.room.loaded,
                "manual rebuild projection loaded") &&
         expect(countProjectedRole(scenario.projection.room, "floor") == 1U,
                "manual rebuild projection floor count") &&
         expect(countProjectedRole(scenario.projection.room, "wall") == 1U,
                "manual rebuild projection wall count") &&
         expect(countProjectedRole(scenario.projection.room, "prop") == 2U,
                "manual rebuild projection prop count") &&
         expect(scenario.window.interactionMode ==
                    iggy3d::ProductInteractionMode::Creative,
                "manual rebuild interaction remains creative") &&
         expect(scenario.window.activeCreative.saveId == scenario.launched.saveId,
                "manual rebuild active creative save preserved") &&
         expect(scenario.window.activeCreative.documentId ==
                    scenario.launched.documentId,
                "manual rebuild active creative document preserved") &&
         expect(scenario.window.activeProductSaveId == "none",
                "manual rebuild active product save unchanged") &&
         expect(facade.document().dirtyFlags() == scenario.dirtyBefore,
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
  installSentinelRoomState(window);
  markCreativeBakedRoomStale(window, app.facade.document());

  const bool clicked = clickCreativeRebuildRoomThroughInputFrame(
      options,
      frontend,
      activeSession,
      window,
      app);

  return expect(launched.accepted,
                "manual empty rebuild setup launch accepted") &&
         expect(clicked, "manual empty rebuild clicked") &&
         expect(window.creativeUiCommand.kind == "rebuild_room",
                "manual empty rebuild command kind") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.requested,
                "manual empty rebuild refresh requested") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.accepted,
                "manual empty rebuild refresh accepted clear") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.status ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild refresh status") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.reasonCode ==
                    "product_creative_baked_room_cleared_no_renderable_objects",
                "manual empty rebuild refresh reason") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.bakeMeasured,
                "manual empty rebuild refresh measured") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.bakedDocumentRevision ==
                    app.facade.document().revision(),
                "manual empty rebuild refresh revision") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.staticMeshCount == 0U,
                "manual empty rebuild static mesh count") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.anchorCount == 0U,
                "manual empty rebuild anchor count") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.spatialSurfaceCount == 0U,
                "manual empty rebuild spatial surface count") &&
         expect(!window.creativeUiCommand.bakedRoomRefresh.collisionReady,
                "manual empty rebuild collision not ready") &&
         expect(window.creativeUiCommand.bakedRoomRefresh.collisionQuerySurfaceCount == 0U,
                "manual empty rebuild collision query count") &&
         expect(window.creativeDocumentRevision.observed,
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
         expect(window.activeCreative.saveId == launched.saveId,
                "manual empty rebuild active creative save preserved") &&
         expect(window.activeCreative.documentId == launched.documentId,
                "manual empty rebuild active creative document preserved") &&
         expect(window.activeProductSaveId == "none",
                "manual empty rebuild active product save unchanged") &&
         expect(app.facade.document().dirtyFlags() == dirtyBefore,
                "manual empty rebuild dirty flags preserved");
}

struct GeneratedRoomShellScenario {
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  iggy3d::ProductCreativeNewWorldLaunchResult launched;
  cr::CreativeObjectId roomId = cr::kInvalidObjectId;
  bool createRoomClicked = false;
  bool metadataRoomOnlyAfterCreate = false;
  std::uint64_t undoDepthAfterCreate = 0;
  bool selected = false;
  bool shellClicked = false;
  std::uint64_t generatedFloorCount = 0;
  std::uint64_t generatedWallCount = 0;
  std::uint64_t bakedFloorRoleCount = 0;
  std::uint64_t bakedWallRoleCount = 0;
  std::string shellInputSemantic;
  std::string shellCommandKind;
  bool shellCommandAccepted = false;
  bool shellCommandChanged = false;
  bool shellReceiptRequested = false;
  bool shellReceiptAccepted = false;
  bool shellReceiptChanged = false;
  cr::CreativeObjectId shellReceiptRoomObjectId = cr::kInvalidObjectId;
  std::uint64_t shellGeneratedObjectCount = 0;
  std::uint64_t shellFloorCount = 0;
  std::uint64_t shellWallCount = 0;
  std::string shellReasonCode;
  std::uint64_t shellDocumentObjectCount = 0;
  bool shellSelectionCleared = false;
  bool shellDocumentChanged = false;
  bool shellAutoRefreshRequested = false;
  bool shellAutoRefreshAccepted = false;
  bool shellAutoRefreshCleared = false;
  std::uint64_t shellAutoRefreshStaticMeshCount = 0;
  std::uint64_t shellAutoRefreshSpatialSurfaceCount = 0;
  bool shellAutoRefreshCollisionReady = false;
  bool shellActiveRoomLoaded = false;
  std::uint64_t shellActiveRoomStaticMeshCount = 0;
  std::uint64_t shellActiveRoomSpatialSurfaceCount = 0;
  bool shellActiveRoomCollisionReady = false;
  std::uint64_t shellActiveRoomCollisionQuerySurfaceCount = 0;
  bool shellBakedRoomStale = false;
  std::uint64_t shellWindowUndoDepth = 0;
  std::uint64_t undoDepthAfterShell = 0;
};

GeneratedRoomShellScenario generateRoomShellScenario(std::string_view name) {
  GeneratedRoomShellScenario scenario;
  scenario.options = testOptions(name);
  scenario.launched =
      launchCreativeWorld(scenario.options,
                          launchRequest("Generate Room Shell",
                                        "2026-07-05T13:25:00Z"),
                          scenario.frontend,
                          scenario.activeSession,
                          scenario.window,
                          scenario.app);

  cr::Facade& facade = scenario.app.facade;
  scenario.createRoomClicked = clickCreativeUiRowThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app,
      "creative.row.create.create_room");
  scenario.roomId = scenario.window.creativeUiCommand.create.objectId;
  scenario.metadataRoomOnlyAfterCreate =
      facade.document().objectCount() == 1U &&
      facade.findObject(scenario.roomId) != nullptr &&
      !scenario.window.activeRoom.loaded &&
      scenario.window.creativeBakedRoomAutoRefresh.accepted &&
      scenario.window.creativeBakedRoomAutoRefresh.clearedActiveRoom;
  scenario.undoDepthAfterCreate =
      cr::creativeUndoDepth(scenario.app.undoStack);
  scenario.selected = selectFacadeObject(facade, scenario.roomId);

  scenario.shellClicked = clickCreativeGenerateRoomShellThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app);

  for (const cr::CreativeObject& object : facade.document().objects()) {
    if (object.parentId.has_value() &&
        object.parentId.value() == scenario.roomId) {
      scenario.generatedFloorCount +=
          object.kind == cr::CreativeObjectKind::Floor ? 1U : 0U;
      scenario.generatedWallCount +=
          object.kind == cr::CreativeObjectKind::Wall ? 1U : 0U;
    }
  }
  for (const iggy3d::RoomStaticMeshAsset& mesh :
       scenario.window.activeRoom.room.staticMeshes) {
    scenario.bakedFloorRoleCount += mesh.role == "floor" ? 1U : 0U;
    scenario.bakedWallRoleCount += mesh.role == "wall" ? 1U : 0U;
  }

  scenario.shellInputSemantic = scenario.window.creativeUiInput.semanticId;
  scenario.shellCommandKind = scenario.window.creativeUiCommand.kind;
  scenario.shellCommandAccepted = scenario.window.creativeUiCommand.accepted;
  scenario.shellCommandChanged = scenario.window.creativeUiCommand.changed;
  scenario.shellReceiptRequested =
      scenario.window.creativeUiCommand.shell.requested;
  scenario.shellReceiptAccepted =
      scenario.window.creativeUiCommand.shell.accepted;
  scenario.shellReceiptChanged =
      scenario.window.creativeUiCommand.shell.changed;
  scenario.shellReceiptRoomObjectId =
      scenario.window.creativeUiCommand.shell.roomObjectId;
  scenario.shellGeneratedObjectCount =
      scenario.window.creativeUiCommand.shell.generatedObjectCount;
  scenario.shellFloorCount =
      scenario.window.creativeUiCommand.shell.floorCount;
  scenario.shellWallCount =
      scenario.window.creativeUiCommand.shell.wallCount;
  scenario.shellReasonCode =
      scenario.window.creativeUiCommand.shell.reasonCode;
  scenario.shellDocumentObjectCount = facade.document().objectCount();
  scenario.shellSelectionCleared =
      facade.selectionState().selectedTarget.value == cr::kInvalidId;
  scenario.shellDocumentChanged =
      scenario.window.creativeDocumentChangedThisFrame;
  scenario.shellAutoRefreshRequested =
      scenario.window.creativeBakedRoomAutoRefresh.requested;
  scenario.shellAutoRefreshAccepted =
      scenario.window.creativeBakedRoomAutoRefresh.accepted;
  scenario.shellAutoRefreshCleared =
      scenario.window.creativeBakedRoomAutoRefresh.clearedActiveRoom;
  scenario.shellAutoRefreshStaticMeshCount =
      scenario.window.creativeBakedRoomAutoRefresh.staticMeshCount;
  scenario.shellAutoRefreshSpatialSurfaceCount =
      scenario.window.creativeBakedRoomAutoRefresh.spatialSurfaceCount;
  scenario.shellAutoRefreshCollisionReady =
      scenario.window.creativeBakedRoomAutoRefresh.collisionReady;
  scenario.shellActiveRoomLoaded = scenario.window.activeRoom.loaded;
  scenario.shellActiveRoomStaticMeshCount =
      scenario.window.activeRoom.staticMeshCount;
  scenario.shellActiveRoomSpatialSurfaceCount =
      scenario.window.activeRoom.spatialSurfaceCount;
  scenario.shellActiveRoomCollisionReady =
      scenario.window.activeRoomCollision.ready;
  scenario.shellActiveRoomCollisionQuerySurfaceCount =
      scenario.window.activeRoomCollision.querySurfaceCount;
  scenario.shellBakedRoomStale = scenario.window.creativeBakedRoomStale;
  scenario.shellWindowUndoDepth = scenario.window.creativeUndo.depth;
  scenario.undoDepthAfterShell =
      cr::creativeUndoDepth(scenario.app.undoStack);
  return scenario;
}

bool generateRoomShellFromSelectedRoomBakesThroughInputFrame() {
  GeneratedRoomShellScenario scenario =
      generateRoomShellScenario("generate_room_shell");

  return expect(scenario.launched.accepted, "shell launch accepted") &&
         expect(scenario.createRoomClicked, "shell create room clicked") &&
         expect(scenario.roomId != cr::kInvalidObjectId,
                "shell room created id") &&
         expect(scenario.metadataRoomOnlyAfterCreate,
                "create room stays metadata-only no-renderable") &&
         expect(scenario.undoDepthAfterCreate == 1U,
                "shell undo depth after create room") &&
         expect(scenario.selected, "shell room selected") &&
         expect(scenario.shellClicked, "shell row clicked") &&
         expect(scenario.shellInputSemantic ==
                    "creative.row.selection.generate_room_shell",
                "shell input semantic") &&
         expect(scenario.shellCommandKind ==
                    "generate_selected_room_shell",
                "shell command kind") &&
         expect(scenario.shellCommandAccepted, "shell command accepted") &&
         expect(scenario.shellCommandChanged, "shell command changed") &&
         expect(scenario.shellReceiptRequested, "shell receipt requested") &&
         expect(scenario.shellReceiptAccepted, "shell receipt accepted") &&
         expect(scenario.shellReceiptChanged, "shell receipt changed") &&
         expect(scenario.shellReceiptRoomObjectId == scenario.roomId,
                "shell receipt room id") &&
         expect(scenario.shellGeneratedObjectCount == 5U,
                "shell generated object count") &&
         expect(scenario.shellFloorCount == 1U, "shell floor count") &&
         expect(scenario.shellWallCount == 4U, "shell wall count") &&
         expect(scenario.shellReasonCode == "creative_room_shell_generated",
                "shell reason") &&
         expect(scenario.shellDocumentObjectCount == 6U,
                "shell object count") &&
         expect(scenario.generatedFloorCount == 1U,
                "shell generated floor") &&
         expect(scenario.generatedWallCount == 4U,
                "shell generated walls") &&
         expect(scenario.shellSelectionCleared,
                "shell install clears selection") &&
         expect(scenario.shellDocumentChanged, "shell document changed") &&
         expect(scenario.shellAutoRefreshRequested,
                "shell auto refresh requested") &&
         expect(scenario.shellAutoRefreshAccepted,
                "shell auto refresh accepted") &&
         expect(!scenario.shellAutoRefreshCleared,
                "shell auto refresh not cleared") &&
         expect(scenario.shellAutoRefreshStaticMeshCount == 5U,
                "shell auto mesh count") &&
         expect(scenario.shellAutoRefreshSpatialSurfaceCount == 9U,
                "shell auto surface count") &&
         expect(scenario.shellAutoRefreshCollisionReady,
                "shell auto collision ready") &&
         expect(scenario.shellActiveRoomLoaded, "shell active room loaded") &&
         expect(scenario.shellActiveRoomStaticMeshCount == 5U,
                "shell active mesh count") &&
         expect(scenario.shellActiveRoomSpatialSurfaceCount == 9U,
                "shell active surface count") &&
         expect(scenario.bakedFloorRoleCount == 1U,
                "shell baked floor role") &&
         expect(scenario.bakedWallRoleCount == 4U,
                "shell baked wall role") &&
         expect(scenario.shellActiveRoomCollisionReady,
                "shell collision ready") &&
         expect(scenario.shellActiveRoomCollisionQuerySurfaceCount == 9U,
                "shell collision query count") &&
         expect(scenario.undoDepthAfterShell == 2U,
                "shell undo depth after shell") &&
         expect(scenario.shellWindowUndoDepth == 2U,
                "shell window undo depth") &&
         expect(!scenario.shellBakedRoomStale, "shell stale fresh");
}

bool generatedRoomShellParentDeleteRejectsThroughInputFrame() {
  GeneratedRoomShellScenario scenario =
      generateRoomShellScenario("generate_room_shell_parent_delete");
  cr::Facade& facade = scenario.app.facade;

  const bool reselectedForParentDelete =
      selectFacadeObject(facade, scenario.roomId);
  const std::uint64_t revisionBeforeParentDelete =
      facade.document().revision();
  const std::uint64_t objectCountBeforeParentDelete =
      facade.document().objectCount();
  const std::uint64_t undoDepthBeforeParentDelete =
      cr::creativeUndoDepth(scenario.app.undoStack);
  const bool activeRoomLoadedBeforeParentDelete =
      scenario.window.activeRoom.loaded;
  const std::uint64_t activeMeshCountBeforeParentDelete =
      scenario.window.activeRoom.staticMeshCount;
  const std::uint64_t activeSurfaceCountBeforeParentDelete =
      scenario.window.activeRoom.spatialSurfaceCount;

  const bool parentDeleteClicked = clickCreativeDeleteSelectedThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app);
  const std::string parentDeleteCommandKind =
      scenario.window.creativeUiCommand.kind;
  const bool parentDeleteCommandAccepted =
      scenario.window.creativeUiCommand.accepted;
  const bool parentDeleteCommandChanged =
      scenario.window.creativeUiCommand.changed;
  const bool parentDeleteRequested =
      scenario.window.creativeUiCommand.deleteObject.requested;
  const bool parentDeleteAccepted =
      scenario.window.creativeUiCommand.deleteObject.accepted;
  const bool parentDeleteChanged =
      scenario.window.creativeUiCommand.deleteObject.changed;
  const bool parentDeleteRemoved =
      scenario.window.creativeUiCommand.deleteObject.removed;
  const cr::CreativeObjectId parentDeleteObjectId =
      scenario.window.creativeUiCommand.deleteObject.objectId;
  const std::string parentDeleteObjectKind =
      scenario.window.creativeUiCommand.deleteObject.objectKind;
  const std::uint64_t parentDeleteRevisionBefore =
      scenario.window.creativeUiCommand.deleteObject.revisionBefore;
  const std::uint64_t parentDeleteRevisionAfter =
      scenario.window.creativeUiCommand.deleteObject.revisionAfter;
  const std::string parentDeleteStatus =
      scenario.window.creativeUiCommand.deleteObject.status;
  const std::string parentDeleteReasonCode =
      scenario.window.creativeUiCommand.deleteObject.reasonCode;
  const bool parentDeleteDocumentChanged =
      scenario.window.creativeDocumentChangedThisFrame;
  const bool parentDeleteAutoRefreshRequested =
      scenario.window.creativeBakedRoomAutoRefresh.requested;
  const bool parentDeleteActiveRoomLoaded = scenario.window.activeRoom.loaded;
  const std::uint64_t parentDeleteActiveRoomStaticMeshCount =
      scenario.window.activeRoom.staticMeshCount;
  const std::uint64_t parentDeleteActiveRoomSpatialSurfaceCount =
      scenario.window.activeRoom.spatialSurfaceCount;
  const std::uint64_t undoDepthAfterParentDelete =
      cr::creativeUndoDepth(scenario.app.undoStack);
  const std::uint64_t objectCountAfterParentDelete =
      facade.document().objectCount();
  const std::uint64_t revisionAfterParentDelete =
      facade.document().revision();
  const bool parentDeleteSelectionStillRoom =
      facade.selectionState().selectedTarget.value == scenario.roomId;

  return expect(scenario.launched.accepted,
                "shell parent delete launch accepted") &&
         expect(scenario.shellClicked, "shell parent delete setup shell") &&
         expect(reselectedForParentDelete,
                "shell parent delete room reselected") &&
         expect(parentDeleteClicked, "shell parent delete clicked") &&
         expect(parentDeleteCommandKind == "delete_selected_object",
                "shell parent delete command kind") &&
         expect(!parentDeleteCommandAccepted,
                "shell parent delete command rejected") &&
         expect(!parentDeleteCommandChanged,
                "shell parent delete command unchanged") &&
         expect(parentDeleteRequested,
                "shell parent delete requested") &&
         expect(!parentDeleteAccepted,
                "shell parent delete receipt rejected") &&
         expect(!parentDeleteChanged,
                "shell parent delete receipt unchanged") &&
         expect(!parentDeleteRemoved,
                "shell parent delete not removed") &&
         expect(parentDeleteObjectId == scenario.roomId,
                "shell parent delete object id") &&
         expect(parentDeleteObjectKind == "Room",
                "shell parent delete object kind") &&
         expect(parentDeleteRevisionBefore == revisionBeforeParentDelete,
                "shell parent delete revision before") &&
         expect(parentDeleteRevisionAfter == revisionBeforeParentDelete,
                "shell parent delete revision after") &&
         expect(parentDeleteStatus == "ParentHasChildren",
                "shell parent delete status") &&
         expect(parentDeleteReasonCode == "parent_has_children",
                "shell parent delete reason") &&
         expect(objectCountAfterParentDelete == objectCountBeforeParentDelete,
                "shell parent delete object count unchanged") &&
         expect(revisionAfterParentDelete == revisionBeforeParentDelete,
                "shell parent delete revision unchanged") &&
         expect(!parentDeleteDocumentChanged,
                "shell parent delete no document change") &&
         expect(!parentDeleteAutoRefreshRequested,
                "shell parent delete no auto refresh") &&
         expect(parentDeleteActiveRoomLoaded ==
                    activeRoomLoadedBeforeParentDelete,
                "shell parent delete active room loaded unchanged") &&
         expect(parentDeleteActiveRoomStaticMeshCount ==
                    activeMeshCountBeforeParentDelete,
                "shell parent delete active mesh count unchanged") &&
         expect(parentDeleteActiveRoomSpatialSurfaceCount ==
                    activeSurfaceCountBeforeParentDelete,
                "shell parent delete active surface count unchanged") &&
         expect(undoDepthAfterParentDelete == undoDepthBeforeParentDelete,
                "shell parent delete undo depth unchanged") &&
         expect(parentDeleteSelectionStillRoom,
                "shell parent delete selection remains");
}

bool removeGeneratedRoomShellAndUndoRestoresThroughInputFrame() {
  GeneratedRoomShellScenario scenario =
      generateRoomShellScenario("generate_room_shell_remove_undo");
  cr::Facade& facade = scenario.app.facade;
  const bool reselectedForRemove = selectFacadeObject(facade, scenario.roomId);
  const bool removeShellClicked = clickCreativeRemoveRoomShellThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app);
  const std::string removeShellInputSemantic =
      scenario.window.creativeUiInput.semanticId;
  const std::string removeShellCommandKind =
      scenario.window.creativeUiCommand.kind;
  const bool removeShellCommandAccepted =
      scenario.window.creativeUiCommand.accepted;
  const bool removeShellCommandChanged =
      scenario.window.creativeUiCommand.changed;
  const bool removeShellRequested =
      scenario.window.creativeUiCommand.shell.requested;
  const bool removeShellAccepted =
      scenario.window.creativeUiCommand.shell.accepted;
  const bool removeShellChanged =
      scenario.window.creativeUiCommand.shell.changed;
  const std::uint64_t removeShellRemovedObjectCount =
      scenario.window.creativeUiCommand.shell.removedObjectCount;
  const std::uint64_t removeShellFloorCount =
      scenario.window.creativeUiCommand.shell.floorCount;
  const std::uint64_t removeShellWallCount =
      scenario.window.creativeUiCommand.shell.wallCount;
  const std::string removeShellStatus =
      scenario.window.creativeUiCommand.shell.status;
  const std::string removeShellReasonCode =
      scenario.window.creativeUiCommand.shell.reasonCode;
  const bool removeShellDocumentChanged =
      scenario.window.creativeDocumentChangedThisFrame;
  const bool removeShellAutoRefreshRequested =
      scenario.window.creativeBakedRoomAutoRefresh.requested;
  const bool removeShellAutoRefreshAccepted =
      scenario.window.creativeBakedRoomAutoRefresh.accepted;
  const bool removeShellAutoRefreshCleared =
      scenario.window.creativeBakedRoomAutoRefresh.clearedActiveRoom;
  const std::uint64_t removeShellDocumentObjectCount =
      facade.document().objectCount();
  const bool removeShellActiveRoomLoaded = scenario.window.activeRoom.loaded;
  const std::uint64_t removeShellActiveRoomStaticMeshCount =
      scenario.window.activeRoom.staticMeshCount;
  const bool removeShellActiveRoomCollisionReady =
      scenario.window.activeRoomCollision.ready;
  const std::uint64_t removeShellActiveRoomCollisionQuerySurfaceCount =
      scenario.window.activeRoomCollision.querySurfaceCount;
  const bool removeShellStale = scenario.window.creativeBakedRoomStale;
  const std::uint64_t undoDepthAfterRemoveShell =
      cr::creativeUndoDepth(scenario.app.undoStack);

  const bool undoClicked = clickCreativeUndoThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app);

  return expect(scenario.launched.accepted, "shell remove launch accepted") &&
         expect(scenario.shellClicked, "shell remove setup shell") &&
         expect(reselectedForRemove, "shell remove room reselected") &&
         expect(removeShellClicked, "shell remove row clicked") &&
         expect(removeShellInputSemantic ==
                    "creative.row.selection.remove_room_shell",
                "shell remove input semantic") &&
         expect(removeShellCommandKind == "remove_selected_room_shell",
                "shell remove command kind") &&
         expect(removeShellCommandAccepted,
                "shell remove command accepted") &&
         expect(removeShellCommandChanged, "shell remove command changed") &&
         expect(removeShellRequested, "shell remove requested") &&
         expect(removeShellAccepted, "shell remove accepted") &&
         expect(removeShellChanged, "shell remove changed") &&
         expect(removeShellRemovedObjectCount == 5U,
                "shell remove removed count") &&
         expect(removeShellFloorCount == 1U, "shell remove floor count") &&
         expect(removeShellWallCount == 4U, "shell remove wall count") &&
         expect(removeShellStatus == "Removed",
                "shell remove receipt status") &&
         expect(removeShellReasonCode == "creative_room_shell_removed",
                "shell remove receipt reason") &&
         expect(removeShellDocumentChanged, "shell remove document changed") &&
         expect(removeShellAutoRefreshRequested,
                "shell remove auto refresh requested") &&
         expect(removeShellAutoRefreshAccepted,
                "shell remove auto refresh accepted") &&
         expect(removeShellAutoRefreshCleared,
                "shell remove auto refresh cleared") &&
         expect(removeShellDocumentObjectCount == 1U,
                "shell remove leaves metadata room") &&
         expect(!removeShellActiveRoomLoaded,
                "shell remove active room unloaded") &&
         expect(removeShellActiveRoomStaticMeshCount == 0U,
                "shell remove active mesh count") &&
         expect(!removeShellActiveRoomCollisionReady,
                "shell remove collision unavailable") &&
         expect(removeShellActiveRoomCollisionQuerySurfaceCount == 0U,
                "shell remove collision count") &&
         expect(!removeShellStale, "shell remove stale fresh") &&
         expect(undoDepthAfterRemoveShell == 3U,
                "shell remove undo depth") &&
         expect(undoClicked, "shell undo clicked") &&
         expect(scenario.window.creativeUiCommand.kind ==
                    "undo_last_document_change",
                "shell undo command kind") &&
         expect(scenario.window.creativeUiCommand.undo.accepted,
                "shell undo accepted") &&
         expect(scenario.window.creativeUiCommand.undo.objectCountBefore == 1U,
                "shell undo object count before") &&
         expect(scenario.window.creativeUiCommand.undo.objectCountAfter == 6U,
                "shell undo object count after") &&
         expect(facade.document().objectCount() == 6U,
                "shell undo object count restored") &&
         expect(facade.findObject(scenario.roomId) != nullptr,
                "shell undo room remains") &&
         expect(scenario.window.creativeBakedRoomAutoRefresh.requested,
                "shell undo auto refresh requested") &&
         expect(scenario.window.creativeBakedRoomAutoRefresh.accepted,
                "shell undo auto refresh accepted") &&
         expect(!scenario.window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
                "shell undo reloaded active room") &&
         expect(scenario.window.activeRoom.loaded,
                "shell undo active room loaded") &&
         expect(scenario.window.activeRoom.staticMeshCount == 5U,
                "shell undo active mesh count") &&
         expect(scenario.window.activeRoom.spatialSurfaceCount == 9U,
                "shell undo active surface count") &&
         expect(scenario.window.activeRoomCollision.ready,
                "shell undo collision ready") &&
         expect(scenario.window.activeRoomCollision.querySurfaceCount == 9U,
                "shell undo collision count") &&
         expect(cr::creativeUndoDepth(scenario.app.undoStack) == 2U,
                "shell undo leaves earlier snapshots") &&
         expect(scenario.window.creativeUndo.depth == 2U,
                "shell undo window depth after undo") &&
         expect(!scenario.window.creativeBakedRoomStale,
                "shell undo stale fresh") &&
         expect(scenario.window.activeCreative.saveId == scenario.launched.saveId,
                "shell active creative save preserved") &&
         expect(scenario.window.activeCreative.documentId ==
                    scenario.launched.documentId,
                "shell active creative document preserved") &&
         expect(scenario.window.activeProductSaveId == "none",
                "shell active product save unchanged") &&
         expect(facade.document().dirtyFlags() != 0U,
                "shell dirty flags not drained");
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
         expect(!window.creativeUndo.available,
                "disabled undo window unavailable") &&
         expect(window.creativeUndo.depth == 0U,
                "disabled undo window depth") &&
         expect(facade.document().revision() == revisionBefore,
                "disabled undo revision unchanged") &&
         expect(facade.document().objectCount() == objectCountBefore,
                "disabled undo object count unchanged") &&
         expect(window.creativeUiInput.semanticId == "creative.row.tools.undo",
                "disabled undo semantic") &&
         expect(window.creativeUiInput.hit,
                "disabled undo input hit") &&
         expect(!window.creativeUiInput.consumed,
                "disabled undo input not consumed") &&
         expect(!window.creativeUiInput.enabled,
                "disabled undo input disabled") &&
         expect(window.creativeUiInput.status ==
                    "product_creative_ui_input_hit_disabled",
                "disabled undo input status") &&
         expect(window.creativeUiCommand.kind == "none",
                "disabled undo no command kind") &&
         expect(!window.creativeUiCommand.accepted,
                "disabled undo command not accepted") &&
         expect(!window.creativeDocumentChangedThisFrame,
                "disabled undo no document change") &&
         expect(!window.creativeBakedRoomAutoRefresh.requested,
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
      window.creativeUiCommand.create.objectId;
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
         expect(window.creativeUiInput.semanticId == "creative.row.tools.undo",
                "undo create semantic") &&
         expect(window.creativeUiCommand.kind == "undo_last_document_change",
                "undo create command kind") &&
         expect(window.creativeUiCommand.accepted,
                "undo create command accepted") &&
         expect(window.creativeUiCommand.changed,
                "undo create command changed") &&
         expect(window.creativeUiCommand.undo.requested,
                "undo create requested") &&
         expect(window.creativeUiCommand.undo.accepted,
                "undo create accepted") &&
         expect(window.creativeUiCommand.undo.changed,
                "undo create changed") &&
         expect(window.creativeUiCommand.undo.hadSnapshot,
                "undo create had snapshot") &&
         expect(window.creativeUiCommand.undo.documentId == launched.documentId,
                "undo create document id") &&
         expect(window.creativeUiCommand.undo.revisionBefore == 1U,
                "undo create revision before") &&
         expect(window.creativeUiCommand.undo.revisionAfter == 0U,
                "undo create revision after") &&
         expect(window.creativeUiCommand.undo.objectCountBefore == 1U,
                "undo create object count before") &&
         expect(window.creativeUiCommand.undo.objectCountAfter == 0U,
                "undo create object count after") &&
         expect(window.creativeUiCommand.undo.depthBefore == 1U,
                "undo create depth before") &&
         expect(window.creativeUiCommand.undo.depthAfter == 0U,
                "undo create depth after") &&
         expect(window.creativeUiCommand.undo.status == "creative_undo_applied",
                "undo create undo status") &&
         expect(window.creativeDocumentChangedThisFrame,
                "undo create document changed") &&
         expect(window.creativeBakedRoomAutoRefresh.requested,
                "undo create auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefresh.accepted,
                "undo create auto refresh accepted") &&
         expect(window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
                "undo create active room cleared") &&
         expect(!window.activeRoom.loaded,
                "undo create active room unloaded") &&
         expect(!window.activeRoomCollision.ready,
                "undo create collision unavailable") &&
         expect(!window.creativeBakedRoomStale,
                "undo create stale fresh") &&
         expect(!cr::creativeUndoAvailable(app.undoStack),
                "undo create no redo stack") &&
         expect(!window.creativeUndo.available,
                "undo create window undo unavailable") &&
         expect(window.creativeUndo.depth == 0U,
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
  const cr::CreativeDocumentCreateReceipt floor = createDefaultFloor(facade);
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
         expect(window.creativeUiCommand.kind == "undo_last_document_change",
                "undo delete command kind") &&
         expect(window.creativeUiCommand.undo.accepted,
                "undo delete accepted") &&
         expect(window.creativeUiCommand.undo.objectCountBefore == 0U,
                "undo delete object count before") &&
         expect(window.creativeUiCommand.undo.objectCountAfter == 1U,
                "undo delete object count after") &&
         expect(window.creativeBakedRoomAutoRefresh.requested,
                "undo delete auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefresh.accepted,
                "undo delete auto refresh accepted") &&
         expect(!window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
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
         expect(window.creativeUndo.depth == 0U,
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
  const cr::CreativeDocumentCreateReceipt floor = createDefaultFloor(facade);
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
         expect(window.creativeUiCommand.kind == "undo_last_document_change",
                "undo visibility command kind") &&
         expect(window.creativeUiCommand.undo.accepted,
                "undo visibility accepted") &&
         expect(window.creativeUiCommand.undo.objectCountBefore == 1U,
                "undo visibility object count before") &&
         expect(window.creativeUiCommand.undo.objectCountAfter == 1U,
                "undo visibility object count after") &&
         expect(window.creativeBakedRoomAutoRefresh.requested,
                "undo visibility auto refresh requested") &&
         expect(window.creativeBakedRoomAutoRefresh.accepted,
                "undo visibility auto refresh accepted") &&
         expect(!window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
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
         expect(window.creativeUndo.depth == 0U,
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
  const cr::CreativeDocumentCreateReceipt floor = createDefaultFloor(facade);
  const bool selected = selectFacadeObject(facade, floor.objectId);
  const cr::CreativeObjectDirtyFlags dirtyAfterCreate =
      facade.document().dirtyFlags();
  installSentinelRoomState(window);
  markCreativeBakedRoomStale(window, facade.document());

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
        expect(window.creativeUiCommand.kind ==
                   "toggle_selected_object_visibility",
               "auto visibility command kind hide") &&
        expect(window.creativeDocumentChangedThisFrame,
               "auto visibility hide document changed") &&
        expect(window.creativeBakedRoomAutoRefresh.requested,
               "auto visibility hide auto requested") &&
        expect(window.creativeBakedRoomAutoRefresh.accepted,
               "auto visibility hide auto accepted") &&
        expect(window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
               "auto visibility hide cleared") &&
        expect(window.creativeBakedRoomAutoRefresh.status ==
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
        expect(window.creativeBakedRoomAutoRefresh.requested,
               "auto visibility show auto requested") &&
        expect(window.creativeBakedRoomAutoRefresh.accepted,
               "auto visibility show auto accepted") &&
        expect(!window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
               "auto visibility show not cleared") &&
        expect(window.creativeBakedRoomAutoRefresh.status ==
                   "product_creative_baked_room_refreshed",
               "auto visibility show refresh status") &&
        expect(window.creativeBakedRoomAutoRefresh.staticMeshCount == 1U,
               "auto visibility show mesh count") &&
        expect(window.creativeBakedRoomAutoRefresh.spatialSurfaceCount == 1U,
               "auto visibility show surface count") &&
        expect(window.creativeBakedRoomAutoRefresh.collisionReady,
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
        expect(window.activeCreative.saveId == launched.saveId,
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
  const cr::CreativeDocumentCreateReceipt floor = createDefaultFloor(facade);
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
         expect(window.creativeUiInput.consumed,
                "delete clear ui input consumed") &&
         expect(window.creativeUiInput.semanticId ==
                    "creative.row.selection.delete_selected",
                "delete clear ui semantic") &&
         expect(window.creativeUiCommand.kind == "delete_selected_object",
                "delete clear command kind") &&
         expect(window.creativeUiCommand.accepted,
                "delete clear command accepted") &&
         expect(window.creativeUiCommand.changed,
                "delete clear command changed") &&
         expect(window.creativeUiCommand.deleteObject.requested,
                "delete clear requested") &&
         expect(window.creativeUiCommand.deleteObject.accepted,
                "delete clear delete accepted") &&
         expect(window.creativeUiCommand.deleteObject.changed,
                "delete clear delete changed") &&
         expect(window.creativeUiCommand.deleteObject.removed,
                "delete clear delete removed") &&
         expect(window.creativeUiCommand.deleteObject.objectId == floor.objectId,
                "delete clear object id") &&
         expect(window.creativeUiCommand.deleteObject.objectKind == "Floor",
                "delete clear object kind") &&
         expect(window.creativeUiCommand.deleteObject.revisionBefore ==
                    revisionBeforeDelete,
                "delete clear revision before") &&
         expect(window.creativeUiCommand.deleteObject.revisionAfter ==
                    revisionBeforeDelete + 1U,
                "delete clear revision after") &&
         expect(window.creativeUiCommand.deleteObject.status == "Removed",
                "delete clear delete status") &&
         expect(window.creativeDocumentChangedThisFrame,
                "delete clear document changed") &&
         expect(window.creativeBakedRoomAutoRefresh.requested,
                "delete clear auto requested") &&
         expect(window.creativeBakedRoomAutoRefresh.accepted,
                "delete clear auto accepted") &&
         expect(window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
                "delete clear auto cleared") &&
         expect(window.creativeBakedRoomAutoRefresh.status ==
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
         expect(window.activeCreative.saveId == launched.saveId,
                "delete clear active creative save preserved") &&
         expect(window.activeCreative.documentId == launched.documentId,
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
  const cr::CreativeDocumentCreateReceipt floor = createDefaultFloor(facade);
  const cr::CreativeDocumentCreateReceipt crate = createDefaultCrate(facade);
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
         expect(window.creativeUiCommand.kind == "delete_selected_object",
                "delete rebuild command kind") &&
         expect(window.creativeUiCommand.deleteObject.objectId == crate.objectId,
                "delete rebuild object id") &&
         expect(window.creativeUiCommand.deleteObject.objectKind == "Crate",
                "delete rebuild object kind") &&
         expect(window.creativeDocumentChangedThisFrame,
                "delete rebuild document changed") &&
         expect(window.creativeBakedRoomAutoRefresh.requested,
                "delete rebuild auto requested") &&
         expect(window.creativeBakedRoomAutoRefresh.accepted,
                "delete rebuild auto accepted") &&
         expect(!window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
                "delete rebuild auto not cleared") &&
         expect(window.creativeBakedRoomAutoRefresh.status ==
                    "product_creative_baked_room_refreshed",
                "delete rebuild auto status") &&
         expect(window.creativeBakedRoomAutoRefresh.staticMeshCount == 1U,
                "delete rebuild auto mesh count") &&
         expect(window.creativeBakedRoomAutoRefresh.spatialSurfaceCount == 1U,
                "delete rebuild auto surface count") &&
         expect(window.creativeBakedRoomAutoRefresh.collisionReady,
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
         expect(window.activeCreative.saveId == launched.saveId,
                "delete rebuild active creative save preserved") &&
         expect(window.activeCreative.documentId == launched.documentId,
                "delete rebuild active creative document preserved") &&
         expect(window.activeProductSaveId == "none",
                "delete rebuild active product save unchanged") &&
         expect(dirtyBeforeDelete != 0U, "delete rebuild dirty before") &&
         expect(facade.document().dirtyFlags() != 0U,
                "delete rebuild dirty flags not drained");
}

struct AutoMoveScenario {
  iggy3d::ProductAppOptions options;
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::CreativeAppState app;
  iggy3d::ProductWindowInputFrameState inputFrame;
  iggy3d::ProductCreativeNewWorldLaunchResult launched;
  cr::CreativeDocumentCreateReceipt floor;
  iggy3d::ProductCreativeBakedActiveRoomRefreshResult initialRefresh;
  float initialCenterX = 0.0F;
  float initialCenterY = 0.0F;
  std::uint64_t revisionBeforeMove = 0;
  float movedCenterX = 0.0F;
  float movedCenterY = 0.0F;
  std::uint64_t revisionBeforeNoChange = 0;
  float undoCenterX = 0.0F;
  float undoCenterY = 0.0F;
  bool undoClicked = false;
};

AutoMoveScenario makeAutoMoveScenario(std::string_view name) {
  AutoMoveScenario scenario;
  scenario.options = testOptions(name);
  scenario.launched =
      launchCreativeWorld(scenario.options,
                          launchRequest("Auto Move Baked Room",
                                        "2026-07-05T12:40:00Z"),
                          scenario.frontend,
                          scenario.activeSession,
                          scenario.window,
                          scenario.app);
  cr::Facade& facade = scenario.app.facade;
  scenario.floor = createDefaultFloor(facade);
  scenario.initialRefresh =
      iggy3d::refreshProductCreativeBakedActiveRoom(
          {},
          scenario.activeSession,
          scenario.window,
          scenario.app);
  scenario.initialCenterX =
      scenario.window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : scenario.window.activeRoom.room.staticMeshes.front().positionMeters.x;
  scenario.initialCenterY =
      scenario.window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : scenario.window.activeRoom.room.staticMeshes.front().positionMeters.y;
  return scenario;
}

void runAutoMoveCommit(AutoMoveScenario& scenario) {
  cr::Facade& facade = scenario.app.facade;
  scenario.revisionBeforeMove = facade.document().revision();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   0.0,
                   0.0,
                   targetId(scenario.floor.objectId))));
  scenario.inputFrame.creativePointerLifecycle.primaryButtonHeld = true;
  scenario.inputFrame.creativePointerLifecycle.lastPointerX = 0.0F;
  scenario.inputFrame.creativePointerLifecycle.lastPointerY = 0.0F;

  runCreativePointerLifecycleFrame(scenario.options,
                                   scenario.frontend,
                                   scenario.activeSession,
                                   scenario.window,
                                   scenario.app,
                                   scenario.inputFrame,
                                   false,
                                   60.0F,
                                   60.0F);

  scenario.movedCenterX =
      scenario.window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : scenario.window.activeRoom.room.staticMeshes.front().positionMeters.x;
  scenario.movedCenterY =
      scenario.window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : scenario.window.activeRoom.room.staticMeshes.front().positionMeters.y;
}

void runAutoMoveNoChangeRelease(AutoMoveScenario& scenario) {
  cr::Facade& facade = scenario.app.facade;
  scenario.revisionBeforeNoChange = facade.document().revision();
  static_cast<void>(facade.setActiveTool(cr::Tool::Move));
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   6.0,
                   6.0,
                   targetId(scenario.floor.objectId))));
  scenario.inputFrame.creativePointerLifecycle.primaryButtonHeld = true;
  scenario.inputFrame.creativePointerLifecycle.lastPointerX = 60.0F;
  scenario.inputFrame.creativePointerLifecycle.lastPointerY = 60.0F;

  runCreativePointerLifecycleFrame(scenario.options,
                                   scenario.frontend,
                                   scenario.activeSession,
                                   scenario.window,
                                   scenario.app,
                                   scenario.inputFrame,
                                   false,
                                   60.0F,
                                   60.0F);
}

void runAutoMoveUndo(AutoMoveScenario& scenario) {
  scenario.undoClicked = clickCreativeUndoThroughInputFrame(
      scenario.options,
      scenario.frontend,
      scenario.activeSession,
      scenario.window,
      scenario.app);
  scenario.undoCenterX =
      scenario.window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : scenario.window.activeRoom.room.staticMeshes.front().positionMeters.x;
  scenario.undoCenterY =
      scenario.window.activeRoom.room.staticMeshes.empty()
          ? 0.0F
          : scenario.window.activeRoom.room.staticMeshes.front().positionMeters.y;
}

bool autoRefreshMoveCommitRefreshesBakedRoomThroughInputFrame() {
  AutoMoveScenario scenario = makeAutoMoveScenario("auto_move_baked_room");
  runAutoMoveCommit(scenario);
  cr::Facade& facade = scenario.app.facade;

  return expect(scenario.launched.accepted, "auto move launch accepted") &&
         expect(scenario.floor.accepted, "auto move floor created") &&
         expect(scenario.initialRefresh.accepted, "auto move initial refresh") &&
         expect(scenario.window.activeRoom.loaded,
                   "auto move active room loaded after move") &&
         expect(facade.document().revision() == scenario.revisionBeforeMove + 1U,
                   "auto move revision advanced") &&
         expect(scenario.window.creativeDocumentChangedThisFrame,
                   "auto move document changed") &&
         expect(scenario.window.creativeBakedRoomAutoRefresh.requested,
                   "auto move auto requested") &&
         expect(scenario.window.creativeBakedRoomAutoRefresh.accepted,
                   "auto move auto accepted") &&
         expect(!scenario.window.creativeBakedRoomAutoRefresh.clearedActiveRoom,
                   "auto move not cleared") &&
         expect(scenario.window.creativeBakedRoomAutoRefresh.status ==
                       "product_creative_baked_room_refreshed",
                   "auto move refresh status") &&
         expect(scenario.window.activeRoom.staticMeshCount == 1U,
                   "auto move mesh count") &&
         expect(scenario.window.activeRoomCollision.ready,
                   "auto move collision ready") &&
         expect(scenario.window.activeRoomCollision.querySurfaceCount == 1U,
                   "auto move collision query count") &&
         expect(scenario.movedCenterX != scenario.initialCenterX ||
                    scenario.movedCenterY != scenario.initialCenterY,
                   "auto move baked mesh center changed") &&
         expect(cr::creativeUndoDepth(scenario.app.undoStack) == 1U,
                   "auto move undo depth after move") &&
         expect(scenario.window.creativeUndo.available,
                   "auto move window undo available after move") &&
         expect(scenario.window.creativeUndo.depth == 1U,
                   "auto move window undo depth after move") &&
         expect(!scenario.window.creativeBakedRoomStale,
                   "auto move stale cleared");
}

bool autoRefreshNoChangeMoveReleaseDoesNotRefreshThroughInputFrame() {
  AutoMoveScenario scenario = makeAutoMoveScenario("auto_move_no_change");
  runAutoMoveCommit(scenario);
  runAutoMoveNoChangeRelease(scenario);
  cr::Facade& facade = scenario.app.facade;

  return expect(scenario.launched.accepted,
                "auto move no-change launch accepted") &&
         expect(scenario.floor.accepted,
                "auto move no-change floor created") &&
         expect(scenario.initialRefresh.accepted,
                "auto move no-change initial refresh") &&
         expect(facade.document().revision() == scenario.revisionBeforeNoChange,
               "auto move no-change revision unchanged") &&
        expect(scenario.window.creativeDocumentRevision.observed,
               "auto move no-change revision observed") &&
        expect(!scenario.window.creativeDocumentChangedThisFrame,
               "auto move no-change not changed") &&
        expect(!scenario.window.creativeBakedRoomAutoRefresh.requested,
               "auto move no-change no auto refresh") &&
        expect(cr::creativeUndoDepth(scenario.app.undoStack) == 1U,
               "auto move no-change undo depth unchanged") &&
        expect(scenario.window.creativeUndo.depth == 1U,
               "auto move no-change window undo depth unchanged") &&
        expect(scenario.window.activeRoom.loaded,
               "auto move no-change active room remains loaded") &&
        expect(!scenario.window.creativeBakedRoomStale,
               "auto move no-change remains fresh") &&
        expect(scenario.window.activeCreative.saveId == scenario.launched.saveId,
               "auto move active creative save preserved") &&
        expect(scenario.window.activeProductSaveId == "none",
               "auto move active product save unchanged") &&
        expect(facade.document().dirtyFlags() != 0U,
               "auto move dirty flags not drained");
}

bool undoAfterMoveCommitRestoresBakedRoomThroughInputFrame() {
  AutoMoveScenario scenario = makeAutoMoveScenario("auto_move_undo");
  runAutoMoveCommit(scenario);
  runAutoMoveNoChangeRelease(scenario);
  runAutoMoveUndo(scenario);

  return expect(scenario.launched.accepted, "auto move undo launch accepted") &&
        expect(scenario.floor.accepted, "auto move undo floor created") &&
        expect(scenario.initialRefresh.accepted,
               "auto move undo initial refresh") &&
        expect(scenario.undoClicked, "auto move undo clicked") &&
        expect(scenario.window.creativeUiCommand.kind ==
                   "undo_last_document_change",
               "auto move undo command kind") &&
        expect(scenario.window.creativeUiCommand.undo.accepted,
               "auto move undo accepted") &&
        expect(scenario.window.creativeUiCommand.undo.changed,
               "auto move undo changed") &&
        expect(scenario.window.creativeUiCommand.undo.depthBefore == 1U,
               "auto move undo depth before") &&
        expect(scenario.window.creativeUiCommand.undo.depthAfter == 0U,
               "auto move undo depth after") &&
        expect(scenario.window.creativeDocumentChangedThisFrame,
               "auto move undo document changed") &&
        expect(scenario.window.creativeBakedRoomAutoRefresh.requested,
               "auto move undo auto refresh requested") &&
        expect(scenario.window.creativeBakedRoomAutoRefresh.accepted,
               "auto move undo auto refresh accepted") &&
        expect(scenario.window.activeRoom.loaded,
               "auto move undo active room loaded") &&
        expect(scenario.window.activeRoom.staticMeshCount == 1U,
               "auto move undo mesh count") &&
        expect(scenario.window.activeRoomCollision.ready,
               "auto move undo collision ready") &&
        expect(scenario.undoCenterX == scenario.initialCenterX &&
                   scenario.undoCenterY == scenario.initialCenterY,
               "auto move undo mesh center restored") &&
        expect(!cr::creativeUndoAvailable(scenario.app.undoStack),
               "auto move undo no redo stack") &&
        expect(scenario.window.creativeUndo.depth == 0U,
               "auto move undo window undo depth");
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
    installSentinelRoomState(window);
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
    installSentinelRoomState(window);
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
    installSentinelRoomState(window);
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
  installSentinelRoomState(window);

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
         expect(window.startup.packageLoadStatus == "ok",
                "blank stage package load status ok") &&
         expect(window.startup.runtimeSessionCreateStatus ==
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
      pauseSaveUsesCreativeIdentityInsteadOfStaleWindowMirror() &&
      pauseCreativeSaveAndExitWritesReturnsTitleAndClearsIdentity() &&
      pauseCreativeSaveAndExitFailureKeepsSessionAndDirtyState() &&
      pauseCreativeReturnToTitleClearsUndoStack() &&
      secondOpenClearsOldFacadeStateAndInstallsRestoredDocument() &&
      refreshCreativeBakedActiveRoomBuildsRoomCollisionAndProjection() &&
      manualRebuildRoomCommandReportsRefreshThroughInputFrame() &&
      manualRebuildRoomCommandLoadsActiveRoomThroughInputFrame() &&
      manualRebuildRoomCommandClearsRoomStateOnNoRenderableDocument() &&
      generateRoomShellFromSelectedRoomBakesThroughInputFrame() &&
      generatedRoomShellParentDeleteRejectsThroughInputFrame() &&
      removeGeneratedRoomShellAndUndoRestoresThroughInputFrame() &&
      disabledUndoRowDoesNotRouteThroughInputFrame() &&
      undoAfterCreateCrateRestoresEmptyDocumentThroughInputFrame() &&
      undoAfterDeleteSelectedRestoresRenderableThroughInputFrame() &&
      undoAfterVisibilityToggleRestoresBakedRoomThroughInputFrame() &&
      autoRefreshVisibilityToggleClearsAndRestoresBakedRoomThroughInputFrame() &&
      deleteSelectedRenderableClearsBakedRoomThroughInputFrame() &&
      deleteOneOfTwoRenderablesRebuildsRemainingBakedRoomThroughInputFrame() &&
      autoRefreshMoveCommitRefreshesBakedRoomThroughInputFrame() &&
      autoRefreshNoChangeMoveReleaseDoesNotRefreshThroughInputFrame() &&
      undoAfterMoveCommitRestoresBakedRoomThroughInputFrame() &&
      refreshCreativeBakedActiveRoomFailuresPreserveExistingRoomState() &&
      creativeLaunchStandsOnBlankStageWithoutFirstRoomDemo() &&
      creativeLaunchFramesCameraOnOrigin() &&
      creativeFrameShowsGroundGrid();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
