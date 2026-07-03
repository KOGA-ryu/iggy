#include "app/iggy3d/Operations.hpp"
#include "app/iggy3d/menu/ActionHandlers.hpp"
#include "app/iggy3d/menu/FrontendRouter.hpp"

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
    cr::Facade& facade) {
  return iggy3d::launchProductCreativeNewWorld(options,
                                               request,
                                               frontend,
                                               activeSession,
                                               window,
                                               facade);
}

iggy3d::ProductCreativeOpenWorldLaunchResult openCreativeWorld(
    const iggy3d::ProductAppOptions& options,
    std::string_view saveId,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::Facade& facade) {
  iggy3d::ProductCreativeOpenWorldLaunchRequest request;
  request.saveId = std::string{saveId};
  return iggy3d::launchProductCreativeOpenWorld(options,
                                                request,
                                                frontend,
                                                activeSession,
                                                window,
                                                facade);
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

iggy3d::ProductMenuActionResult confirmPauseAction(
    const iggy3d::ProductAppOptions& options,
    iggy3d::FrontendAction action,
    iggy3d::FrontendState& frontend,
    std::optional<iggy3d::Session>& activeSession,
    iggy3d::ProductAppWindowState& window,
    cr::Facade* facade) {
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
       closeRequested, settings, facade});
}

bool successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode() {
  const iggy3d::ProductAppOptions options = testOptions("success");
  iggy3d::FrontendState frontend;
  std::optional<iggy3d::Session> activeSession;
  iggy3d::ProductAppWindowState window;
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest(),
                          frontend,
                          activeSession,
                          window,
                          facade);
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
         expect(facade.selectionState().selectedTarget.value == cr::kInvalidId,
                "creative launch selection clear") &&
         expect(facade.inspectionState().inspectedTarget.value == cr::kInvalidId,
                "creative launch inspection clear") &&
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
                "creative launch continue status");
}

bool blankTitleOrTimestampRejectsBeforeSessionInstallAndModeSwitch() {
  {
    const iggy3d::ProductAppOptions options = testOptions("blank_title");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::Facade facade;

    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest(" ", "2026-07-03T12:00:00Z"),
                            frontend,
                            activeSession,
                            window,
                            facade);

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
    cr::Facade facade;

    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("No Time", ""),
                            frontend,
                            activeSession,
                            window,
                            facade);

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
  cr::Facade facade;
  iggy3d::ProductCreativeNewWorldLaunchRequest request = launchRequest();
  request.attemptToken = "attempt token with spaces";

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options, request, frontend, activeSession, window, facade);
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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult first =
      launchCreativeWorld(options,
                          launchRequest("First", "2026-07-03T12:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  static_cast<void>(facade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(createdObject.objectId))));
  static_cast<void>(facade.setActiveTool(cr::Tool::Inspect));
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
                          facade);

  return expect(first.accepted, "second setup first accepted") &&
         expect(createdObject.accepted, "second setup object created") &&
         expect(second.accepted, "second launch accepted") &&
         expect(second.documentId == 2U, "second launch document id advances") &&
         expect(second.objectCount == 0U, "second launch empty document") &&
         expect(second.installReceipt.previousObjectCount == 1U,
                "second launch previous facade object count") &&
         expect(second.installReceipt.selectionCleared,
                "second launch selection cleared") &&
         expect(second.installReceipt.inspectionCleared,
                "second launch inspection cleared") &&
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
         expect(facade.inspectionState().inspectedTarget.value == cr::kInvalidId,
                "second launch inspection invalid") &&
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
  cr::Facade createFacade;
  const iggy3d::ProductCreativeNewWorldLaunchResult created =
      launchCreativeWorld(options,
                          launchRequest("Open Source",
                                        "2026-07-03T13:00:00Z"),
                          createFrontend,
                          createSession,
                          createWindow,
                          createFacade);
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
  cr::Facade openFacade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        created.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openFacade);

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
         expect(openFacade.inspectionState().inspectedTarget.value ==
                    cr::kInvalidId,
                "open launch inspection clear") &&
         expect(!openFacade.measurementState().hasMeasurement,
                "open launch measurement empty") &&
         expect(!openFacade.ghostState().visible,
                "open launch ghost hidden") &&
         expect(openFacade.toolState().pointer.target.value == cr::kInvalidId,
                "open launch pointer clear");
}

bool openBlankOrInvalidSaveIdRejectsBeforeSessionInstallAndModeSwitch() {
  {
    const iggy3d::ProductAppOptions options = testOptions("open_blank_id");
    iggy3d::FrontendState frontend;
    std::optional<iggy3d::Session> activeSession;
    iggy3d::ProductAppWindowState window;
    cr::Facade facade;

    const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
        openCreativeWorld(options, " ", frontend, activeSession, window, facade);

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
    cr::Facade facade;

    const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
        openCreativeWorld(options,
                          "bad save id",
                          frontend,
                          activeSession,
                          window,
                          facade);

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
  cr::Facade openFacade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        productWindow.activeProductSaveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openFacade);

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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult created =
      launchCreativeWorld(options,
                          launchRequest("Creative Before Product",
                                        "2026-07-03T15:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Save Current Creative",
                                        "2026-07-03T16:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
  const cr::CreativeDocumentCreateReceipt createdObject =
      facade.createDocumentObject(cr::CreativeObjectKind::Room);
  const cr::CreativeObjectDirtyFlags dirtyBefore =
      facade.document().dirtyFlags();
  const std::uint64_t revisionBefore = facade.document().revision();

  const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
      iggy3d::saveProductCurrentCreativeWorld(options, facade, "unit", window);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::Facade openFacade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        launched.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openFacade);

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
    cr::Facade facade;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Missing Id",
                                          "2026-07-03T16:10:00Z"),
                            frontend,
                            activeSession,
                            window,
                            facade);
    const cr::CreativeDocumentCreateReceipt createdObject =
        facade.createDocumentObject(cr::CreativeObjectKind::Room);
    const cr::CreativeObjectDirtyFlags dirtyBefore =
        facade.document().dirtyFlags();
    window.activeCreativeSaveId = "none";

    const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
        iggy3d::saveProductCurrentCreativeWorld(options, facade, "unit", window);

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
    cr::Facade facade;
    const iggy3d::ProductCreativeNewWorldLaunchResult launched =
        launchCreativeWorld(options,
                            launchRequest("Inactive Save",
                                          "2026-07-03T16:20:00Z"),
                            frontend,
                            activeSession,
                            window,
                            facade);
    const cr::CreativeDocumentCreateReceipt createdObject =
        facade.createDocumentObject(cr::CreativeObjectKind::Room);
    const cr::CreativeObjectDirtyFlags dirtyBefore =
        facade.document().dirtyFlags();
    window.interactionMode = iggy3d::ProductInteractionMode::Player;

    const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
        iggy3d::saveProductCurrentCreativeWorld(options, facade, "unit", window);

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
  cr::Facade facade;

  const iggy3d::ProductCreativeCurrentWorldSaveResult saved =
      iggy3d::saveProductCurrentCreativeWorld(options, facade, "unit", window);

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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Save",
                                        "2026-07-03T17:00:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
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
                         &facade);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::Facade openFacade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        launched.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openFacade);

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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Null",
                                        "2026-07-03T17:10:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Exit",
                                        "2026-07-03T17:20:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
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
                         &facade);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::Facade openFacade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult opened =
      openCreativeWorld(options,
                        launched.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openFacade);

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
  cr::Facade facade;

  const iggy3d::ProductCreativeNewWorldLaunchResult launched =
      launchCreativeWorld(options,
                          launchRequest("Pause Creative Exit Missing",
                                        "2026-07-03T17:30:00Z"),
                          frontend,
                          activeSession,
                          window,
                          facade);
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
                         &facade);

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
  cr::Facade firstCreateFacade;
  const iggy3d::ProductCreativeNewWorldLaunchResult firstCreated =
      launchCreativeWorld(options,
                          launchRequest("First Open",
                                        "2026-07-03T14:00:00Z"),
                          firstCreateFrontend,
                          firstCreateSession,
                          firstCreateWindow,
                          firstCreateFacade);
  const cr::CreativeDocumentCreateReceipt firstObject =
      firstCreateFacade.createDocumentObject(cr::CreativeObjectKind::Room);
  cr::CreativeDocument firstSavedDocument = firstCreateFacade.document();
  const iggy3d::CreativeWorldSaveResult firstSaved =
      iggy3d::saveCreativeWorld(
          saveRequest(options, firstCreated.saveId, firstSavedDocument));

  iggy3d::FrontendState secondCreateFrontend;
  std::optional<iggy3d::Session> secondCreateSession;
  iggy3d::ProductAppWindowState secondCreateWindow;
  cr::Facade secondCreateFacade;
  const iggy3d::ProductCreativeNewWorldLaunchResult secondCreated =
      launchCreativeWorld(options,
                          launchRequest("Second Open",
                                        "2026-07-03T14:10:00Z"),
                          secondCreateFrontend,
                          secondCreateSession,
                          secondCreateWindow,
                          secondCreateFacade);

  iggy3d::FrontendState openFrontend;
  std::optional<iggy3d::Session> openSession;
  iggy3d::ProductAppWindowState openWindow;
  cr::Facade openFacade;
  const iggy3d::ProductCreativeOpenWorldLaunchResult firstOpened =
      openCreativeWorld(options,
                        firstCreated.saveId,
                        openFrontend,
                        openSession,
                        openWindow,
                        openFacade);
  static_cast<void>(openFacade.dispatchToolInput(
      pointerInput(cr::CreativeToolInputKind::PointerPress,
                   1.0,
                   2.0,
                   targetId(firstObject.objectId))));
  static_cast<void>(openFacade.setActiveTool(cr::Tool::Inspect));
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
                        openFacade);

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
         expect(secondOpened.installReceipt.inspectionCleared,
                "second open inspection cleared") &&
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
         expect(openFacade.inspectionState().inspectedTarget.value ==
                    cr::kInvalidId,
                "second open inspection invalid") &&
         expect(!openFacade.measurementState().hasMeasurement,
                "second open measurement cleared state") &&
         expect(!openFacade.ghostState().visible,
                "second open ghost hidden") &&
         expect(openFacade.toolState().pointer.target.value == cr::kInvalidId,
                "second open pointer target invalid");
}

}  // namespace

int main() {
  const bool ok =
      successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode() &&
      blankTitleOrTimestampRejectsBeforeSessionInstallAndModeSwitch() &&
      invalidAttemptTokenWritesNoCommittedSaveAndDoesNotEnterCreativeMode() &&
      secondLaunchClearsOldFacadeStateAndInstallsNewDocument() &&
      openLaunchRestoresSavedCreativeDocumentAndEntersCreativeMode() &&
      openBlankOrInvalidSaveIdRejectsBeforeSessionInstallAndModeSwitch() &&
      openProductSessionSaveRejectsAsMissingCreativeSection() &&
      productNewWorldLaunchClearsActiveCreativeIdentity() &&
      currentCreativeWorldSaveDrainsDirtyAndPersistsDocument() &&
      currentCreativeWorldSaveRejectsInvalidContextsWithoutDrain() &&
      pauseCreativeSaveWritesCreativeDocumentAndKeepsSession() &&
      pauseCreativeSaveNullFacadeFailsClosed() &&
      pauseCreativeSaveAndExitWritesReturnsTitleAndClearsIdentity() &&
      pauseCreativeSaveAndExitFailureKeepsSessionAndDirtyState() &&
      secondOpenClearsOldFacadeStateAndInstallsRestoredDocument();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
