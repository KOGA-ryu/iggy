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

}  // namespace

int main() {
  const bool ok =
      successfulLaunchCreatesSaveSessionInstallsDocumentAndEntersCreativeMode() &&
      blankTitleOrTimestampRejectsBeforeSessionInstallAndModeSwitch() &&
      invalidAttemptTokenWritesNoCommittedSaveAndDoesNotEnterCreativeMode() &&
      secondLaunchClearsOldFacadeStateAndInstallsNewDocument();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
