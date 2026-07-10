#include "app/iggy3d/world/Launch.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SaveBrowser.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/save/Catalog.hpp"
#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/save/SaveBridge.hpp"
#include "app/iggy3d/save/SaveSlotOperations.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"
#include "content/PackageLoader.hpp"

namespace iggy3d {

namespace {

std::string packageLoadStatusName(PackageLoadStatus status) {
  switch (status) {
    case PackageLoadStatus::Ok:
      return "ok";
    case PackageLoadStatus::MissingPackageFile:
      return "missing_package_file";
    case PackageLoadStatus::PackageReadFailed:
      return "package_read_failed";
    case PackageLoadStatus::ScenarioReadFailed:
      return "scenario_read_failed";
    case PackageLoadStatus::ParseError:
      return "parse_error";
    case PackageLoadStatus::UnsupportedKey:
      return "unsupported_key";
    case PackageLoadStatus::MissingRequiredKey:
      return "missing_required_key";
    case PackageLoadStatus::MissingScenarioId:
      return "missing_scenario_id";
    case PackageLoadStatus::InvalidNumber:
      return "invalid_number";
    case PackageLoadStatus::InvalidEnum:
      return "invalid_enum";
    case PackageLoadStatus::InvalidPath:
      return "invalid_path";
  }
  return "unknown";
}

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

void recordProductSaveLoadResult(const ProductSaveLoadResult& loaded,
                                 ProductAppWindowState& window) {
  window.saveSession.productSaveLoadResult = loaded;
  // Normalize an empty save id to "none" at the producer so ReceiptBuilder
  // stays a straight read of the typed result (preserves the prior receipt).
  if (window.saveSession.productSaveLoadResult.record.id.empty()) {
    window.saveSession.productSaveLoadResult.record.id = "none";
  }
  if (loaded.ok && !loaded.record.id.empty()) {
    window.saveSession.activeProductSaveId = loaded.record.id;
  }
}

void recordSavedRoomMarkerBindingResult(
    const ProductSavedRoomMarkerBindingResult& bound,
    ProductAppWindowState& window) {
  window.saveSession.savedMarkerBind = bound;
  // Normalize an empty room id to "none" at the producer so ReceiptBuilder
  // stays a straight read of the typed result (preserves the prior receipt).
  if (window.saveSession.savedMarkerBind.roomId.empty()) {
    window.saveSession.savedMarkerBind.roomId = "none";
  }
}

void recordProductSaveLoadSelection(std::string_view source,
                                    const SaveSlotPreview* slot,
                                    ProductAppWindowState& window) {
  window.saveSession.productSaveLoadSource = std::string(source);
  window.saveSession.productSaveLoadSelectedId =
      slot == nullptr || slot->id.empty() ? "none" : slot->id;
  window.saveSession.productSaveLoadSelectedEnabled = slot != nullptr && slot->enabled;
}

const SaveSlotPreview* saveSlotById(const SaveSlotList& slots,
                                    std::string_view selectedId) {
  for (const SaveSlotPreview& slot : slots.slots) {
    if (slot.id == selectedId) {
      return &slot;
    }
  }
  return nullptr;
}

void launchProductSaveSlot(const ProductAppOptions& options,
                           const ProductWorldTemplate& world,
                           const SaveSlotPreview* slot,
                           FrontendAction launchAction,
                           std::string_view source,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.frontendShell.launchAction = std::string(frontendActionName(launchAction));
  recordProductSaveLoadSelection(source, slot, window);
  if (slot == nullptr) {
    window.frontendShell.launchStatus = "no_compatible_save";
    window.saveSession.productSaveLoadResult.status = "no_compatible_save";
    window.saveSession.productSaveLoadResult.reasonCode = "no_compatible_save";
    frontend.status = source == "load_save_selector"
                          ? "load_save_action_disabled"
                          : "opening_menu_action_disabled";
    return;
  }
  if (!slot->enabled || slot->compatibility != SaveSlotCompatibility::Compatible) {
    const std::string reason =
        slot->reason.empty() ? "save_slot_disabled" : slot->reason;
    window.frontendShell.launchStatus = reason;
    window.saveSession.productSaveLoadResult.status = reason;
    window.saveSession.productSaveLoadResult.reasonCode = reason;
    frontend.status = "load_save_action_disabled";
    return;
  }

  if (!createProductSession(options, activeSession, window)) {
    frontend.status = source == "load_save_selector"
                          ? "load_save_launch_failed"
                          : "opening_menu_continue_failed";
    return;
  }

  ProductSaveLoadRequest loadRequest;
  loadRequest.path = slot->path;
  loadRequest.session = &*activeSession;
  loadRequest.expectedPackageId = world.packageId;
  loadRequest.expectedScenarioId = world.scenarioId;
  const ProductSaveLoadResult loaded = loadProductSessionSave(loadRequest);
  recordProductSaveLoadResult(loaded, window);
  if (!loaded.ok) {
    window.frontendShell.launchStatus = loaded.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = source == "load_save_selector"
                          ? "load_save_launch_failed"
                          : "opening_menu_continue_failed";
    return;
  }

  if (loaded.authoredRoomPresent) {
    activeRoom(window) =
        buildProductActiveRoomFromSavedAuthoredRoom(loaded.authoredRoom);
    bumpActiveRoomRevision(window);
    const ProductSavedRoomMarkerBindingResult bound =
        bindSavedRoomMarkersToSession(activeRoom(window), *activeSession);
    recordSavedRoomMarkerBindingResult(bound, window);
    if (!bound.ok) {
      window.frontendShell.launchStatus = bound.reasonCode;
      clearProductGameplayLaunchState(activeSession, window);
      frontend.status = source == "load_save_selector"
                            ? "load_save_launch_failed"
                            : "opening_menu_continue_failed";
      return;
    }
  }
  (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
  window.frontendShell.launchStatus = loaded.status;
  window.inputDevice.interactionMode = ProductInteractionMode::Player;
  enterProductGameplayTransition(frontend, window, launchAction);
}

}  // namespace

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  window.gameplay.gameplayActive = false;
  window.gameplay.runtimeSessionCreated = false;
  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession.reset();
}

bool createProductSessionFromPackage(const PackageLoadResult& package,
                                     std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  window.frontendShell.packageLoadStatus = packageLoadStatusName(package.status);
  if (package.status != PackageLoadStatus::Ok) {
    window.frontendShell.startup.runtimeSessionCreateMeasured = false;
    window.frontendShell.startup.runtimeSessionCreateMicroseconds = 0;
    window.frontendShell.startup.runtimeSessionCreateStatus = "package_load_failed";
    window.frontendShell.launchStatus = "package_load_failed";
    return false;
  }

  const auto sessionStarted = std::chrono::steady_clock::now();
  const ProductPackageSessionSeedResult seed =
      buildProductPackageSessionSeed(package);
  if (!seed.ok) {
    window.frontendShell.startup.runtimeSessionCreateMeasured = true;
    window.frontendShell.startup.runtimeSessionCreateMicroseconds =
        elapsedMicroseconds(sessionStarted);
    window.frontendShell.startup.runtimeSessionCreateStatus = seed.reasonCode;
    window.frontendShell.launchStatus = seed.reasonCode;
    return false;
  }

  SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = seed.seed;
  create.config = seed.seed.config;

  Result<Session> session = Session::create(create);
  if (session.status != ResultStatus::Ok) {
    window.frontendShell.launchStatus =
        session.error.code.empty() ? "session_create_failed" : session.error.code;
    window.frontendShell.startup.runtimeSessionCreateMeasured = true;
    window.frontendShell.startup.runtimeSessionCreateMicroseconds =
        elapsedMicroseconds(sessionStarted);
    window.frontendShell.startup.runtimeSessionCreateStatus = window.frontendShell.launchStatus;
    return false;
  }
  window.frontendShell.startup.runtimeSessionCreateMeasured = true;
  window.frontendShell.startup.runtimeSessionCreateMicroseconds =
      elapsedMicroseconds(sessionStarted);
  window.frontendShell.startup.runtimeSessionCreateStatus =
      "startup_runtime_session_created";

  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  activeSession = std::move(session.value);
  if (!package.rooms.empty()) {
    activeRoom(window) = buildProductActiveRoomFromPackageRoom(
        package.rooms.front(), package.manifest.packageId, package.scenario.scenarioId);
  }
  bumpActiveRoomRevision(window);
  (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
  window.gameplay.runtimeSessionCreated = true;
  window.gameplay.gameplayActive = true;
  window.frontendShell.launchStatus = "runtime_session_created";
  return true;
}

bool createProductSession(const ProductAppOptions& options,
                          std::optional<Session>& activeSession,
                          ProductAppWindowState& window) {
  const auto lookupStarted = std::chrono::steady_clock::now();
  const std::filesystem::path packagePath = productPackagePathFromOptions(options);
  window.frontendShell.startup.packagePath =
      packagePath.empty() ? "none" : packagePath.generic_string();
  window.frontendShell.startup.packageLookupMeasured = true;
  window.frontendShell.startup.packageLookupMicroseconds =
      elapsedMicroseconds(lookupStarted);
  window.frontendShell.startup.packageLookupStatus = "startup_package_lookup_resolved";

  const auto loadStarted = std::chrono::steady_clock::now();
  const PackageLoadResult package = loadPackage({packagePath.generic_string()});
  window.frontendShell.startup.packageLoadMeasured = true;
  window.frontendShell.startup.packageLoadMicroseconds = elapsedMicroseconds(loadStarted);
  window.frontendShell.startup.packageLoadStatus = packageLoadStatusName(package.status);
  return createProductSessionFromPackage(package, activeSession, window);
}

void launchProductContinueSave(const ProductAppOptions& options,
                               const ProductWorldTemplate& world,
                               const ProductSaveBridgeResult& saves,
                               FrontendState& frontend,
                               std::optional<Session>& activeSession,
                               ProductAppWindowState& window) {
  // Continue selects the newest compatible active save via the catalog policy
  // (selectProductContinueSave), then resolves the matching projected slot to
  // load. The catalog is sorted newest-first and the policy is unit-tested in
  // product_save_catalog_tests; this replaces an ad-hoc reverse scan that
  // returned the OLDEST compatible save.
  const ProductContinueSelectionResult continueSelection =
      selectProductContinueSave(saves.catalog.catalog);
  const SaveSlotPreview* slot =
      continueSelection.selected
          ? saveSlotById(saves.slots, continueSelection.selectedSaveId)
          : nullptr;
  launchProductSaveSlot(options,
                        world,
                        slot,
                        FrontendAction::Continue,
                        "continue",
                        frontend,
                        activeSession,
                        window);
}

void launchProductLoadSaveSelection(const ProductAppOptions& options,
                                    const ProductWorldTemplate& world,
                                    const ProductSaveBridgeResult& saves,
                                    FrontendState& frontend,
                                    std::optional<Session>& activeSession,
                                    ProductAppWindowState& window) {
  frontend.selectedAction = FrontendAction::Load;
  const SaveSlotPreview* selected =
      initializeSelectedProductSaveSlot(saves.slots, window);
  launchProductSaveSlot(options,
                        world,
                        selected,
                        FrontendAction::Load,
                        "load_save_selector",
                        frontend,
                        activeSession,
                        window);
}

}  // namespace iggy3d
