#include "app/iggy3d/Operations.hpp"

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
         expect(frontend.childScreen == iggy3d::FrontendScreen::Gameplay,
                "creative launch frontend gameplay") &&
         expect(window.launchStatus == "product_creative_world_launched",
                "creative launch window status") &&
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
      secondOpenClearsOldFacadeStateAndInstallsRestoredDocument();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
