#include "app/iggy3d/ProductAppOperations.hpp"

#include <filesystem>
#include <string>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/ProductActiveRoomCollision.hpp"
#include "app/iggy3d/ProductActiveRoomState.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductPackageSessionSeed.hpp"
#include "app/iggy3d/ProductWorldCreation.hpp"
#include "content/PackageLoader.hpp"
#include "render/RenderDiagnostics.hpp"

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

std::filesystem::path defaultProductPackagePath(const ProductAppOptions& options) {
  if (!options.devPackageOverride.empty()) {
    return options.devPackageOverride;
  }

  PackageLookupConfig lookupConfig;
  lookupConfig.packageMode = PackageMode::BuildTreeVisual;
  lookupConfig.requireGraphicsRuntime = false;
  lookupConfig.requireShaderRoot = false;
  const PackageLookupResult lookup = resolvePackageRuntimeLookup(lookupConfig);
  if (lookup.outcome == RenderOutcome::Ok && !lookup.lookup.resourceRoot.empty()) {
    return lookup.lookup.resourceRoot / "demos" / "first_room" /
           "package.iggy3d.toml";
  }

  return std::filesystem::path{"fixtures"} / "demos" / "first_room" /
         "package.iggy3d.toml";
}

bool createProductSession(const ProductAppOptions& options,
                          std::optional<Session>& activeSession,
                          ProductAppWindowState& window) {
  const std::filesystem::path packagePath = defaultProductPackagePath(options);
  const PackageLoadResult package = loadPackage({packagePath.generic_string()});
  window.packageLoadStatus = packageLoadStatusName(package.status);
  if (package.status != PackageLoadStatus::Ok) {
    window.launchStatus = "package_load_failed";
    return false;
  }

  const ProductPackageSessionSeedResult seed =
      buildProductPackageSessionSeed(package);
  if (!seed.ok) {
    window.launchStatus = seed.reasonCode;
    return false;
  }

  SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = seed.seed;
  create.config = seed.seed.config;

  Result<Session> session = Session::create(create);
  if (session.status != ResultStatus::Ok) {
    window.launchStatus =
        session.error.code.empty() ? "session_create_failed" : session.error.code;
    return false;
  }

  window.activeRoom = {};
  window.activeRoomCollision = {};
  activeSession = std::move(session.value);
  if (!package.rooms.empty()) {
    window.activeRoom = buildProductActiveRoomFromPackageRoom(
        package.rooms.front(), package.manifest.packageId, package.scenario.scenarioId);
    window.activeRoomCollision =
        buildProductActiveRoomCollision(window.activeRoom, activeSession->state());
  }
  window.runtimeSessionCreated = true;
  window.gameplayActive = true;
  window.runtimeStateHash = activeSession->stateHash();
  window.launchStatus = "runtime_session_created";
  return true;
}

ProductWorldCreationResult prepareProductWorldCreationFromDraft(
    const ProductAppOptions& options,
    const ProductWorldTemplate& world,
    const WorldSetupDraft& draft,
    ProductAppWindowState& window) {
  window.worldSetupTitle = draft.worldName;
  const WorldSetupRouteResult setup =
      routeWorldSetupAction(draft, FrontendAction::CreateAndEnter);
  if (!setup.accepted || !setup.createRequested) {
    window.worldSetupStatus = std::string(setup.reasonCode);
    window.worldCreationStatus = std::string(setup.status);
    window.worldCreationReasonCode = std::string(setup.reasonCode);
    return {};
  }
  window.worldSetupTitle = setup.createRequest.worldName;
  window.worldSetupStatus = std::string(setup.status);

  ProductWorldCreationResult creation = prepareProductWorldCreation(
      makeProductWorldCreationInput(setup.createRequest,
                                    world,
                                    options.saveRoot,
                                    "product_new_world_request_001",
                                    "world_0001"));
  window.worldCreationStatus = std::string(creation.status);
  window.worldCreationReasonCode = std::string(creation.reasonCode);
  window.worldCreationWorldId =
      creation.request.worldId.empty() ? "none" : creation.request.worldId;
  window.worldCreationWorldTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.worldCreationInitialSaveRequested = creation.initialSavePlan.requested;
  window.worldCreationInitialSaveWritten = creation.initialSaveWritten;
  window.worldCreationInitialSaveId =
      creation.initialSavePlan.saveId.empty() ? "none"
                                              : creation.initialSavePlan.saveId;
  window.worldCreationInitialSaveTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.worldCreationRouteAfterCreate = std::string(creation.routeAfterCreate);
  return creation;
}

void recordProductWorldInitialSaveResult(
    const ProductWorldInitialSaveResult& initialSave,
    ProductAppWindowState& window) {
  window.worldCreationStatus = initialSave.status;
  window.worldCreationReasonCode = initialSave.reasonCode;
  window.worldCreationWorldId = initialSave.creation.request.worldId.empty()
                                    ? "none"
                                    : initialSave.creation.request.worldId;
  window.worldCreationWorldTitle =
      initialSave.creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.worldTitle;
  window.worldCreationInitialSaveRequested =
      initialSave.creation.initialSavePlan.requested;
  window.worldCreationInitialSaveWritten =
      initialSave.creation.initialSaveWritten;
  window.worldCreationInitialSaveId =
      initialSave.creation.initialSavePlan.saveId.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.saveId;
  window.worldCreationInitialSaveTitle =
      initialSave.creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.worldTitle;
  window.worldCreationRouteAfterCreate =
      std::string(initialSave.creation.routeAfterCreate);
  window.productSaveStatus = initialSave.saveWrite.status;
  window.productSaveReasonCode = initialSave.saveWrite.reasonCode;
  window.productSaveDurableReason = initialSave.saveWrite.durableReason;
  window.productSaveSource = "initial_world";
  window.productSaveSaveId = initialSave.saveWrite.record.id.empty()
                                 ? "none"
                                 : initialSave.saveWrite.record.id;
  window.productSaveSessionSaved = initialSave.saveWrite.ok;
  if (initialSave.saveWrite.ok && !initialSave.saveWrite.record.id.empty()) {
    window.activeProductSaveId = initialSave.saveWrite.record.id;
  }
}

const SaveSlotPreview* newestCompatibleSaveSlot(const SaveSlotList& slots) {
  for (auto it = slots.slots.rbegin(); it != slots.slots.rend(); ++it) {
    if (it->enabled && it->compatibility == SaveSlotCompatibility::Compatible) {
      return &*it;
    }
  }
  return nullptr;
}

void recordProductSaveLoadResult(const ProductSaveLoadResult& loaded,
                                 ProductAppWindowState& window) {
  window.productSaveLoadStatus = loaded.status;
  window.productSaveLoadReasonCode = loaded.reasonCode;
  window.productSaveLoadSaveId =
      loaded.record.id.empty() ? "none" : loaded.record.id;
  window.productSaveLoadPreviousHash = loaded.previousHash;
  window.productSaveLoadLoadedHash = loaded.loadedHash;
  window.productSaveLoadSessionLoaded = loaded.sessionLoaded;
  if (loaded.ok && !loaded.record.id.empty()) {
    window.activeProductSaveId = loaded.record.id;
  }
}

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  window.gameplayActive = false;
  window.runtimeSessionCreated = false;
  window.runtimeStateHash = 0;
  window.activeRoom = {};
  window.activeRoomCollision = {};
  activeSession.reset();
}

void recordProductSaveWriteResult(std::string_view source,
                                  const ProductSaveWriteResult& written,
                                  ProductAppWindowState& window) {
  window.productSaveStatus = written.status;
  window.productSaveReasonCode = written.reasonCode;
  window.productSaveDurableReason = written.durableReason;
  window.productSaveSource = std::string(source);
  window.productSaveSaveId = written.record.id.empty() ? "none" : written.record.id;
  window.productSaveSessionSaved = written.ok;
  if (written.ok && !written.record.id.empty()) {
    window.activeProductSaveId = written.record.id;
  }
}

void recordProductSaveLoadSelection(std::string_view source,
                                    const SaveSlotPreview* slot,
                                    ProductAppWindowState& window) {
  window.productSaveLoadSource = std::string(source);
  window.productSaveLoadSelectedId =
      slot == nullptr || slot->id.empty() ? "none" : slot->id;
  window.productSaveLoadSelectedEnabled = slot != nullptr && slot->enabled;
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

const SaveSlotPreview* firstSelectableSaveSlot(const SaveSlotList& slots) {
  for (const SaveSlotPreview& slot : slots.slots) {
    if (slot.enabled) {
      return &slot;
    }
  }
  return slots.slots.empty() ? nullptr : &slots.slots.front();
}

void recordSelectedProductSaveSlot(const SaveSlotList& slots,
                                   const SaveSlotPreview* slot,
                                   ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    window.selectedProductSaveId = "none";
    window.selectedProductSaveEnabled = false;
    window.selectedProductSaveStatus = "empty";
    return;
  }
  if (slot == nullptr) {
    window.selectedProductSaveId = "none";
    window.selectedProductSaveEnabled = false;
    window.selectedProductSaveStatus = "missing";
    return;
  }
  window.selectedProductSaveId = slot->id.empty() ? "none" : slot->id;
  window.selectedProductSaveEnabled = slot->enabled;
  window.selectedProductSaveStatus = slot->enabled ? "selected" : "disabled";
}

void recordSelectedDeletedProductSaveSlot(const SaveSlotList& slots,
                                          const SaveSlotPreview* slot,
                                          ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    window.deletedSelectedSaveId = "none";
    window.deletedSelectedSaveEnabled = false;
    window.deletedSelectedSaveStatus = "empty";
    return;
  }
  if (slot == nullptr) {
    window.deletedSelectedSaveId = "none";
    window.deletedSelectedSaveEnabled = false;
    window.deletedSelectedSaveStatus = "missing";
    return;
  }
  window.deletedSelectedSaveId = slot->id.empty() ? "none" : slot->id;
  window.deletedSelectedSaveEnabled = slot->enabled;
  window.deletedSelectedSaveStatus = slot->enabled ? "selected" : "disabled";
}

const SaveSlotPreview* initializeSelectedDeletedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window) {
  const SaveSlotPreview* current =
      window.deletedSelectedSaveId == "none"
          ? nullptr
          : saveSlotById(slots, window.deletedSelectedSaveId);
  const SaveSlotPreview* selected =
      current == nullptr ? firstSelectableSaveSlot(slots) : current;
  recordSelectedDeletedProductSaveSlot(slots, selected, window);
  return selected;
}

}  // namespace

ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options) {
  ProductWorldTemplate world =
      options.devPackageOverride.empty()
          ? defaultProductWorldTemplate()
          : devOverrideProductWorldTemplate(
                options.devPackageOverride.generic_string(), options.devScenario);
  const PackageLoadResult package =
      loadPackage({defaultProductPackagePath(options).generic_string()});
  if (package.status == PackageLoadStatus::Ok) {
    world.packageId = package.manifest.packageId;
    world.scenarioId = package.scenario.scenarioId;
  }
  return world;
}

ProductSaveWriteResult writeProductCurrentSessionSave(
    const ProductAppOptions& options,
    const std::optional<Session>& activeSession,
    std::string_view source,
    ProductAppWindowState& window) {
  if (!activeSession.has_value()) {
    ProductSaveWriteResult missing;
    missing.status = "product_save_session_missing";
    missing.reasonCode = "product_save_session_missing";
    missing.durableReason = "not_requested";
    recordProductSaveWriteResult(source, missing, window);
    return missing;
  }

  ProductSaveWriteRequest request;
  request.saveRoot = options.saveRoot;
  request.saveIdHint = window.activeProductSaveId == "none"
                           ? std::string{}
                           : window.activeProductSaveId;
  request.attemptToken = "attempt_002";
  request.state = &activeSession->state();
  const ProductSaveWriteResult written = writeProductSessionSaveDurably(request);
  recordProductSaveWriteResult(source, written, window);
  return written;
}

const SaveSlotPreview* initializeSelectedProductSaveSlot(
    const SaveSlotList& slots,
    ProductAppWindowState& window) {
  const SaveSlotPreview* current =
      window.selectedProductSaveId == "none"
          ? nullptr
          : saveSlotById(slots, window.selectedProductSaveId);
  const SaveSlotPreview* selected =
      current == nullptr ? firstSelectableSaveSlot(slots) : current;
  recordSelectedProductSaveSlot(slots, selected, window);
  return selected;
}

const SaveSlotPreview* moveSelectedProductSaveSlot(const SaveSlotList& slots,
                                                   InputAction action,
                                                   ProductAppWindowState& window) {
  if (slots.slots.empty()) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    return nullptr;
  }

  std::size_t index = 0;
  for (std::size_t i = 0; i < slots.slots.size(); ++i) {
    if (slots.slots[i].id == window.selectedProductSaveId) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp && index > 0U) {
    --index;
  } else if (action == InputAction::MenuDown && index + 1U < slots.slots.size()) {
    ++index;
  }
  const SaveSlotPreview* selected = &slots.slots[index];
  recordSelectedProductSaveSlot(slots, selected, window);
  return selected;
}

bool selectProductSaveSlotById(const SaveSlotList& slots,
                               std::string_view selectedId,
                               ProductAppWindowState& window) {
  const SaveSlotPreview* slot = saveSlotById(slots, selectedId);
  recordSelectedProductSaveSlot(slots, slot, window);
  return slot != nullptr;
}

ProductSaveBridgeResult scanDeletedProductSavesForOptions(
    const ProductAppOptions& options) {
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  return scanDeletedProductSaves(options.saveRoot, world.packageId, world.scenarioId);
}

void recordDeletedProductSaveSlots(const ProductSaveBridgeResult& deletedSaves,
                                   ProductAppWindowState& window) {
  window.deletedSaveCount =
      static_cast<std::uint64_t>(deletedSaves.slots.slots.size());
  window.deletedCompatibleSaveCount = deletedSaves.slots.compatibleCount;
}

bool selectDeletedProductSaveSlotById(const SaveSlotList& slots,
                                      std::string_view selectedId,
                                      ProductAppWindowState& window) {
  const SaveSlotPreview* slot = saveSlotById(slots, selectedId);
  recordSelectedDeletedProductSaveSlot(slots, slot, window);
  return slot != nullptr;
}

void openDeletedProductSaveBrowser(const ProductAppOptions& options,
                                   ProductAppWindowState& window,
                                   FrontendState& frontend) {
  const ProductSaveBridgeResult deletedSaves =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedSaves, window);
  window.deletedSaveBrowserOpen = true;
  initializeSelectedDeletedProductSaveSlot(deletedSaves.slots, window);
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.status = "deleted_save_browser_open";
}

void executeProductSaveRecover(const ProductAppOptions& options,
                               ProductAppWindowState& window,
                               FrontendState& frontend) {
  const ProductSaveBridgeResult deletedBefore =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedBefore, window);
  const SaveSlotPreview* selected =
      window.deletedSelectedSaveId == "none"
          ? initializeSelectedDeletedProductSaveSlot(deletedBefore.slots, window)
          : saveSlotById(deletedBefore.slots, window.deletedSelectedSaveId);
  recordSelectedDeletedProductSaveSlot(deletedBefore.slots, selected, window);

  const std::string recoverId =
      selected == nullptr || selected->id.empty() ? "none" : selected->id;
  window.saveRecoverSaveId = recoverId;
  window.saveRecoverSnapshotRecovered = false;
  window.saveRecoverSnapshotMissing = false;
  if (recoverId == "none") {
    window.saveRecoverStatus = "product_save_recover_id_missing";
    window.saveRecoverReasonCode = "product_save_recover_id_missing";
    window.saveRecoverExecuted = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.status = "save_recover_failed";
    return;
  }

  const ProductSaveRecoverResult recovered =
      recoverProductSave({options.saveRoot, recoverId});
  window.saveRecoverStatus = recovered.status;
  window.saveRecoverReasonCode = recovered.reasonCode;
  window.saveRecoverExecuted = recovered.ok;
  window.saveRecoverSaveId = recovered.saveId.empty() ? "none" : recovered.saveId;
  window.saveRecoverSnapshotRecovered = recovered.snapshotRecovered;
  window.saveRecoverSnapshotMissing = recovered.snapshotMissing;

  const ProductSaveBridgeResult deletedAfter =
      scanDeletedProductSavesForOptions(options);
  recordDeletedProductSaveSlots(deletedAfter, window);
  if (recovered.ok) {
    window.deletedSaveBrowserOpen = false;
    recordSelectedDeletedProductSaveSlot(deletedAfter.slots, nullptr, window);
    const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
    const ProductSaveBridgeResult activeAfter =
        scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
    selectProductSaveSlotById(activeAfter.slots, recovered.saveId, window);
  }

  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.selectedAction = FrontendAction::LoadSave;
  frontend.status = recovered.ok ? "save_recover_recovered" : "save_recover_failed";
}

void openProductSaveDeleteConfirmation(const SaveSlotList& slots,
                                       ProductAppWindowState& window,
                                       FrontendState& frontend) {
  const SaveSlotPreview* slot =
      window.selectedProductSaveId == "none"
          ? nullptr
          : saveSlotById(slots, window.selectedProductSaveId);
  if (slot == nullptr) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    window.saveDeleteConfirmationOpen = false;
    window.saveDeleteCandidateId = "none";
    window.saveDeleteCandidateEnabled = false;
    window.saveDeleteStatus =
        slots.slots.empty() ? "save_delete_unavailable" : "save_delete_missing";
    window.saveDeleteReasonCode = window.saveDeleteStatus;
    window.saveDeleteType = "soft";
    window.saveDeleteRecoverable = false;
    window.saveDeleteExecuted = false;
    frontend.status = window.saveDeleteStatus;
    return;
  }

  recordSelectedProductSaveSlot(slots, slot, window);
  window.saveDeleteConfirmationOpen = true;
  window.saveDeleteCandidateId = slot->id.empty() ? "none" : slot->id;
  window.saveDeleteCandidateEnabled = slot->enabled;
  window.saveDeleteStatus = "confirm_open";
  window.saveDeleteReasonCode = "confirm_open";
  window.saveDeleteType = "soft";
  window.saveDeleteRecoverable = false;
  window.saveDeleteExecuted = false;
  frontend.childScreen = FrontendScreen::DeleteConfirm;
  frontend.selectedAction = FrontendAction::Delete;
  frontend.status = "save_delete_confirm_open";
}

void cancelProductSaveDeleteConfirmation(ProductAppWindowState& window,
                                         FrontendState& frontend) {
  window.saveDeleteConfirmationOpen = false;
  window.saveDeleteStatus = "cancelled";
  window.saveDeleteReasonCode = "cancelled";
  window.saveDeleteType = "soft";
  window.saveDeleteRecoverable = false;
  window.saveDeleteExecuted = false;
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.selectedAction = FrontendAction::Delete;
  frontend.status = "save_delete_cancelled";
}

void executeProductSaveSoftDelete(const ProductAppOptions& options,
                                  ProductAppWindowState& window,
                                  FrontendState& frontend) {
  window.saveDeleteConfirmationOpen = false;
  window.saveDeleteType = "soft";
  window.saveDeleteRecoverable = false;
  if (window.saveDeleteCandidateId == "none" ||
      window.saveDeleteCandidateId.empty()) {
    window.saveDeleteStatus = "product_save_delete_id_missing";
    window.saveDeleteReasonCode = "product_save_delete_id_missing";
    window.saveDeleteExecuted = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.selectedAction = FrontendAction::Delete;
    frontend.status = "save_delete_failed";
    return;
  }

  const ProductSaveSoftDeleteResult deleted =
      softDeleteProductSave({options.saveRoot, window.saveDeleteCandidateId});
  window.saveDeleteStatus = deleted.status;
  window.saveDeleteReasonCode = deleted.reasonCode;
  window.saveDeleteExecuted = deleted.ok;
  window.saveDeleteRecoverable = deleted.ok;
  if (deleted.ok) {
    window.selectedProductSaveEnabled = false;
    window.selectedProductSaveStatus = "missing";
  }
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.selectedAction = FrontendAction::Delete;
  frontend.status = deleted.ok ? "save_delete_soft_deleted" : "save_delete_failed";
}

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.launchAction = "create_and_enter";
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  if (!createProductSession(options, activeSession, window)) {
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  ProductWorldCreationResult creation =
      prepareProductWorldCreationFromDraft(options, world, worldSetupDraft, window);
  if (!creation.accepted) {
    window.launchStatus = std::string(creation.reasonCode);
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  ProductWorldInitialSaveRequest initialSaveRequest;
  initialSaveRequest.creation = creation;
  initialSaveRequest.state = &activeSession->state();
  initialSaveRequest.attemptToken = "attempt_001";
  const ProductWorldInitialSaveResult initialSave =
      writeProductWorldInitialSaveDurably(initialSaveRequest);
  recordProductWorldInitialSaveResult(initialSave, window);
  if (!initialSave.ok) {
    window.launchStatus = initialSave.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  window.launchStatus = initialSave.status;
  enterProductGameplayTransition(frontend, window, FrontendAction::CreateAndEnter);
}

void launchProductSaveSlot(const ProductAppOptions& options,
                           const ProductWorldTemplate& world,
                           const SaveSlotPreview* slot,
                           FrontendAction launchAction,
                           std::string_view source,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.launchAction = std::string(frontendActionName(launchAction));
  recordProductSaveLoadSelection(source, slot, window);
  if (slot == nullptr) {
    window.launchStatus = "no_compatible_save";
    window.productSaveLoadStatus = "no_compatible_save";
    window.productSaveLoadReasonCode = "no_compatible_save";
    frontend.status = source == "load_save_selector"
                          ? "load_save_action_disabled"
                          : "opening_menu_action_disabled";
    return;
  }
  if (!slot->enabled || slot->compatibility != SaveSlotCompatibility::Compatible) {
    const std::string reason =
        slot->reason.empty() ? "save_slot_disabled" : slot->reason;
    window.launchStatus = reason;
    window.productSaveLoadStatus = reason;
    window.productSaveLoadReasonCode = reason;
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
    window.launchStatus = loaded.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = source == "load_save_selector"
                          ? "load_save_launch_failed"
                          : "opening_menu_continue_failed";
    return;
  }

  if (window.activeRoom.loaded) {
    window.activeRoomCollision =
        buildProductActiveRoomCollision(window.activeRoom, activeSession->state());
  }
  window.launchStatus = loaded.status;
  window.runtimeStateHash = activeSession->stateHash();
  enterProductGameplayTransition(frontend, window, launchAction);
}

void launchProductContinueSave(const ProductAppOptions& options,
                               const ProductWorldTemplate& world,
                               const ProductSaveBridgeResult& saves,
                               FrontendState& frontend,
                               std::optional<Session>& activeSession,
                               ProductAppWindowState& window) {
  launchProductSaveSlot(options,
                        world,
                        newestCompatibleSaveSlot(saves.slots),
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
