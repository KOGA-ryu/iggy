#include "app/iggy3d/AppShell.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/SettingsMenu.hpp"
#include "app/iggy3d/DefaultWorldTemplate.hpp"
#include "app/iggy3d/ProductCameraController.hpp"
#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/ProductGameplayController.hpp"
#include "app/iggy3d/ProductGameplayFeedback.hpp"
#include "app/iggy3d/ProductMenuTransitions.hpp"
#include "app/iggy3d/ProductPackageSessionSeed.hpp"
#include "app/iggy3d/ProductPrimitiveDrawList.hpp"
#include "app/iggy3d/ProductRenderBridge.hpp"
#include "app/iggy3d/ProductScriptedGameplayDriver.hpp"
#include "app/iggy3d/ProductViewportFraming.hpp"
#include "app/iggy3d/ProductWorldCreation.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "app/iggy3d/SaveBridge.hpp"
#include "content/PackageLoader.hpp"
#include "app/input/ActionState.hpp"
#include "app/input/InputRouter.hpp"
#include "app/input/GamepadInput.hpp"
#include "app/input/KeyboardInput.hpp"
#include "app/input/MouseInput.hpp"
#include "projection/debug/DebugProjection.hpp"
#include "projection/scene/SceneProjection.hpp"
#include "render/RenderDiagnostics.hpp"
#include "runtime/session/Session.hpp"

#if defined(IGGY3D_HAS_SDL3)
#include <chrono>
#include <thread>

#include <SDL3/SDL.h>

#include "app/iggy3d/OpeningMenuView.hpp"
#include "app/platform/SdlWindow.hpp"
#endif

namespace iggy3d {

namespace {

FrontendInputBackend settingsInputBackendFromOptions(ProductInputBackend backend) {
  switch (backend) {
    case ProductInputBackend::Keyboard:
      return FrontendInputBackend::Keyboard;
    case ProductInputBackend::Gamepad:
      return FrontendInputBackend::Gamepad;
    case ProductInputBackend::Auto:
      return FrontendInputBackend::Auto;
  }
  return FrontendInputBackend::Auto;
}

FrontendRendererChoice settingsRendererFromOptions(ProductRendererRequest renderer) {
  switch (renderer) {
    case ProductRendererRequest::Null:
      return FrontendRendererChoice::Null;
    case ProductRendererRequest::Vulkan:
      return FrontendRendererChoice::Vulkan;
  }
  return FrontendRendererChoice::Null;
}

FrontendWindowMode settingsWindowModeFromOptions(ProductWindowMode mode) {
  switch (mode) {
    case ProductWindowMode::NoWindow:
      return FrontendWindowMode::NoWindow;
    case ProductWindowMode::Window:
      return FrontendWindowMode::Window;
  }
  return FrontendWindowMode::NoWindow;
}

FrontendSettings productFrontendSettingsFromOptions(const ProductAppOptions& options) {
  FrontendSettings settings = defaultFrontendSettings();
  settings.inputBackend = settingsInputBackendFromOptions(options.inputBackend);
  settings.renderer = settingsRendererFromOptions(options.renderer);
  settings.windowMode = settingsWindowModeFromOptions(options.windowMode);
  settings.cameraMode = FrontendCameraMode::FirstPerson;
  settings.devToolsEnabled = true;
  settings.debugOverlayEnabled = true;
  return settings;
}

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
    return lookup.lookup.resourceRoot / "demos" / "first_room" / "package.iggy3d.toml";
  }

  return std::filesystem::path{"fixtures"} / "demos" / "first_room" / "package.iggy3d.toml";
}

ProductWorldTemplate productWorldTemplateFromOptions(const ProductAppOptions& options) {
  ProductWorldTemplate world =
      options.devPackageOverride.empty()
          ? defaultProductWorldTemplate()
          : devOverrideProductWorldTemplate(options.devPackageOverride.generic_string(),
                                            options.devScenario);
  const PackageLoadResult package =
      loadPackage({defaultProductPackagePath(options).generic_string()});
  if (package.status == PackageLoadStatus::Ok) {
    world.packageId = package.manifest.packageId;
    world.scenarioId = package.scenario.scenarioId;
  }
  return world;
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
    window.launchStatus = session.error.code.empty() ? "session_create_failed"
                                                     : session.error.code;
    return false;
  }

  activeSession = std::move(session.value);
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
      creation.initialSavePlan.saveId.empty() ? "none" : creation.initialSavePlan.saveId;
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
  window.worldCreationInitialSaveWritten = initialSave.creation.initialSaveWritten;
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
  window.productSaveSaveId =
      initialSave.saveWrite.record.id.empty() ? "none" : initialSave.saveWrite.record.id;
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
  window.productSaveLoadSaveId = loaded.record.id.empty() ? "none" : loaded.record.id;
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
  activeSession.reset();
}

void recordProductSaveWriteResult(std::string_view source,
                                  const ProductSaveWriteResult& written,
                                  ProductAppWindowState& window) {
  window.productSaveStatus = written.status;
  window.productSaveReasonCode = written.reasonCode;
  window.productSaveDurableReason = written.durableReason;
  window.productSaveSource = std::string(source);
  window.productSaveSaveId =
      written.record.id.empty() ? "none" : written.record.id;
  window.productSaveSessionSaved = written.ok;
  if (written.ok && !written.record.id.empty()) {
    window.activeProductSaveId = written.record.id;
  }
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
  request.saveIdHint = window.activeProductSaveId == "none" ? std::string{}
                                                            : window.activeProductSaveId;
  request.attemptToken = "attempt_002";
  request.state = &activeSession->state();
  const ProductSaveWriteResult written = writeProductSessionSaveDurably(request);
  recordProductSaveWriteResult(source, written, window);
  return written;
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

ProductSaveBridgeResult scanDeletedProductSavesForOptions(
    const ProductAppOptions& options) {
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  return scanDeletedProductSaves(options.saveRoot, world.packageId, world.scenarioId);
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

void recordDeletedProductSaveSlots(const ProductSaveBridgeResult& deletedSaves,
                                   ProductAppWindowState& window) {
  window.deletedSaveCount =
      static_cast<std::uint64_t>(deletedSaves.slots.slots.size());
  window.deletedCompatibleSaveCount = deletedSaves.slots.compatibleCount;
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
  if (window.saveDeleteCandidateId == "none" || window.saveDeleteCandidateId.empty()) {
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
    const std::string reason = slot->reason.empty() ? "save_slot_disabled"
                                                    : slot->reason;
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

void applyGameplayProjectionMetrics(ProductAppWindowState& window,
                                    const SceneProjectionResult* scene,
                                    const DebugProjectionResult* debug,
                                    const ProductPrimitiveDrawList* drawList,
                                    const ProductViewportFrame* frame,
                                    const ProductRenderBridgeFrame* bridge,
                                    bool viewVisible) {
  if (!window.gameplayActive || scene == nullptr) {
    window.viewport.gameplayViewVisible = false;
    window.viewport.productDrawGridVisible = false;
    window.viewport.productDrawPlayerVisible = false;
    window.viewport.productDrawRoomVisible = false;
    window.viewport.productDrawObjectiveVisible = false;
    window.viewport.productDrawTargetIndicatorVisible = false;
    window.viewport.productDrawItemCount = 0;
    window.viewport.productDrawDebugMarkerCount = 0;
    window.viewport.productViewProjection = "primitive_first_person";
    window.viewport.productViewYawApplied = false;
    window.viewport.productViewPitchApplied = false;
    window.viewport.productViewPlayerAnchorFound = false;
    window.viewport.productRenderBridgeReady = false;
    window.viewport.productViewFrameReady = false;
    window.viewport.productViewFrameItemCount = 0;
    window.viewport.productViewFrameOnScreenItemCount = 0;
    window.viewport.productViewFrameTargetItemCount = 0;
    window.viewport.productFeedbackBridgeReady = false;
    window.viewport.productFeedbackBridgeLineCount = 0;
    window.sceneItemCount = 0;
    window.debugItemCount = 0;
    window.playerVisible = false;
    window.roomVisible = false;
    window.objectiveVisible = false;
    window.rendererMutatedRuntime = false;
    return;
  }

  window.viewport.gameplayViewVisible = viewVisible;
  window.sceneItemCount = static_cast<std::uint64_t>(scene->items.size());
  window.debugItemCount =
      debug == nullptr ? 0U : static_cast<std::uint64_t>(debug->items.size());
  window.playerVisible = scene->playerCount > 0;
  window.roomVisible = true;
  window.objectiveVisible =
      scene->pickupCount > 0 || scene->interactableCount > 0 || scene->markerCount > 0 ||
      scene->room.loaded;
  window.rendererMutatedRuntime = false;
  if (drawList != nullptr) {
    window.viewport.productDrawGridVisible = drawList->gridVisible;
    window.viewport.productDrawPlayerVisible = drawList->playerVisible;
    window.viewport.productDrawRoomVisible = drawList->roomVisible;
    window.viewport.productDrawObjectiveVisible = drawList->objectiveVisible;
    window.viewport.productDrawTargetIndicatorVisible =
        drawList->playerFocusIndicatorVisible;
    window.viewport.productDrawItemCount = drawList->itemCount;
    window.viewport.productDrawDebugMarkerCount = drawList->debugMarkerCount;
  }
  if (frame != nullptr) {
    window.viewport.productViewProjection = frame->projectionMode;
    window.viewport.productViewYawApplied = frame->yawApplied;
    window.viewport.productViewPitchApplied = frame->pitchApplied;
    window.viewport.productViewPlayerAnchorFound = frame->playerAnchorFound;
  }
  if (bridge != nullptr) {
    window.viewport.productRenderBridgeReady = bridge->ready;
    window.viewport.productViewFrameReady = bridge->viewFrameReady;
    window.viewport.productViewFrameItemCount = bridge->frameItemCount;
    window.viewport.productViewFrameOnScreenItemCount = bridge->onScreenItemCount;
    window.viewport.productViewFrameTargetItemCount = bridge->targetItemCount;
    window.viewport.productFeedbackBridgeReady = bridge->feedbackReady;
    window.viewport.productFeedbackBridgeLineCount = bridge->feedbackLineCount;
  }
}

void refreshGameplayProjectionMetrics(const std::optional<Session>& activeSession,
                                      ProductAppWindowState& window) {
  if (!window.gameplayActive || !activeSession.has_value()) {
    applyGameplayProjectionMetrics(window, nullptr, nullptr, nullptr, nullptr, nullptr,
                                   false);
    return;
  }

  const SceneProjectionResult scene = buildSceneProjection(activeSession->state());
  const DebugProjectionResult debug = buildDebugProjection(activeSession->state());
  const ProductPrimitiveDrawList drawList = buildProductPrimitiveDrawList(&scene, &debug);
  const ProductViewportFrame frame = buildProductViewportFrame(
      drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                           window.viewport.cameraPitchDegrees});
  const ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
  const ProductRenderBridgeFrame bridge =
      buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
  window.runtimeStateHash = activeSession->stateHash();
  applyGameplayProjectionMetrics(window, &scene, &debug, &drawList, &frame, &bridge,
                                 true);
}

FrontendAction nextStarterSelection(FrontendAction current, InputAction action) {
  const std::array<FrontendAction, 6> actions = {
      FrontendAction::Continue,
      FrontendAction::NewWorld,
      FrontendAction::LoadSave,
      FrontendAction::Settings,
      FrontendAction::DevTools,
      FrontendAction::Exit,
  };

  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }

  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

FrontendDevToolsCategory nextDevToolsSelection(FrontendDevToolsCategory current,
                                               InputAction action) {
  const auto& categories = devToolsCategoryOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < categories.size(); ++i) {
    if (categories[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? categories.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % categories.size();
  }
  return categories[index];
}

FrontendSettingsTab nextSettingsSelection(FrontendSettingsTab current, InputAction action) {
  const auto& tabs = settingsTabOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < tabs.size(); ++i) {
    if (tabs[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? tabs.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % tabs.size();
  }
  return tabs[index];
}

FrontendAction nextPauseSelection(FrontendAction current, InputAction action) {
  const auto& actions = pauseActionOrder();
  std::size_t index = 0;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i] == current) {
      index = i;
      break;
    }
  }
  if (action == InputAction::MenuUp) {
    index = index == 0 ? actions.size() - 1 : index - 1;
  } else if (action == InputAction::MenuDown) {
    index = (index + 1) % actions.size();
  }
  return actions[index];
}

MenuOwner productInputOwnerFor(const FrontendState& frontend,
                               const ProductAppWindowState& window) {
  if (frontend.screen == FrontendScreen::Settings ||
      frontend.childScreen == FrontendScreen::Settings) {
    return MenuOwner::Settings;
  }
  if (frontend.screen == FrontendScreen::DevOverlay ||
      frontend.childScreen == FrontendScreen::StarterDevTools) {
    return MenuOwner::DevTools;
  }
  if (frontend.screen == FrontendScreen::Starter) {
    return MenuOwner::Starter;
  }
  if (frontend.screen == FrontendScreen::Pause) {
    return MenuOwner::Pause;
  }
  if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
    return MenuOwner::Gameplay;
  }
  return MenuOwner::None;
}

void applyOpeningMenuAction(FrontendState& frontend,
                            const ProductSaveBridgeResult& saves,
                            const ProductAppOptions& options,
                            FrontendSettingsTab& settingsTab,
                            std::optional<Session>& activeSession,
                            WorldSetupDraft& worldSetupDraft,
                            ProductAppWindowState& window,
                            InputAction action,
                            bool& closeRequested) {
  if (action == InputAction::SystemPause) {
    if (frontend.screen == FrontendScreen::Gameplay && window.gameplayActive) {
      openProductPauseTransition(frontend, window, FrontendAction::Resume);
      frontend.status = "pause_opened_from_gameplay";
      return;
    }
    if (frontend.screen == FrontendScreen::Pause) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.screen == FrontendScreen::DevOverlay) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.screen == FrontendScreen::Settings &&
        frontend.childScreen == FrontendScreen::Pause) {
      openProductPauseTransition(frontend, window, FrontendAction::Settings);
      return;
    }
    frontend.status = "opening_menu_pause_back_requested";
    closeRequested = true;
    return;
  }

  if (frontend.screen == FrontendScreen::Pause) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.selectedAction = nextPauseSelection(frontend.selectedAction, action);
      frontend.status = "pause_menu_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (action != InputAction::MenuConfirm) {
      return;
    }
    if (frontend.selectedAction == FrontendAction::Resume) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (frontend.selectedAction == FrontendAction::Settings) {
      openProductPauseSettingsTransition(frontend, window, settingsTab);
      return;
    }
    if (frontend.selectedAction == FrontendAction::DevTools) {
      openProductPauseDevToolsTransition(frontend, window,
                                         FrontendDevToolsCategory::Session);
      return;
    }
    if (frontend.selectedAction == FrontendAction::Save) {
      const ProductSaveWriteResult written =
          writeProductCurrentSessionSave(options, activeSession, "pause_save", window);
      frontend.status = written.ok ? "pause_save_written" : "pause_save_failed";
      window.launchStatus = written.ok ? "pause_save_written" : written.reasonCode;
      return;
    }
    if (frontend.selectedAction == FrontendAction::SaveAndExit) {
      const ProductSaveWriteResult written = writeProductCurrentSessionSave(
          options, activeSession, "pause_save_and_exit", window);
      frontend.status = written.ok ? "pause_save_and_exit_written"
                                   : "pause_save_and_exit_failed";
      window.launchStatus =
          written.ok ? "pause_save_and_exit_written" : written.reasonCode;
      if (written.ok) {
        returnProductToTitleTransition(frontend, window);
        activeSession.reset();
      }
      return;
    }
    if (frontend.selectedAction == FrontendAction::ReturnToTitle) {
      returnProductToTitleTransition(frontend, window);
      activeSession.reset();
      return;
    }
    if (frontend.selectedAction == FrontendAction::ExitGame) {
      frontend.status = "pause_exit_game_requested";
      closeRequested = true;
      return;
    }
    frontend.status = "pause_action_selected";
    return;
  }

  if (frontend.screen == FrontendScreen::DevOverlay) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_overlay_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      closeProductOverlayToGameplayTransition(frontend, window);
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_overlay_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::StarterDevTools) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      frontend.devToolsCategory = nextDevToolsSelection(frontend.devToolsCategory, action);
      frontend.status = "dev_tools_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.devToolsOpen = false;
      frontend.status = "dev_tools_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "dev_tools_category_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::Settings) {
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      settingsTab = nextSettingsSelection(settingsTab, action);
      frontend.status = "settings_selection_changed";
      return;
    }
    if (action == InputAction::MenuBack) {
      if (frontend.screen == FrontendScreen::Settings &&
          frontend.childScreen == FrontendScreen::Pause) {
        openProductPauseTransition(frontend, window, FrontendAction::Settings);
      } else {
        frontend.childScreen = FrontendScreen::Gameplay;
      }
      frontend.status = "settings_closed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      frontend.status = "settings_tab_selected";
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::DeleteConfirm &&
      window.saveDeleteConfirmationOpen) {
    if (action == InputAction::MenuBack) {
      cancelProductSaveDeleteConfirmation(window, frontend);
      return;
    }
    if (action == InputAction::MenuConfirm) {
      executeProductSaveSoftDelete(options, window, frontend);
      return;
    }
  }

  if (frontend.childScreen == FrontendScreen::NewWorld) {
    window.worldSetupTitle = worldSetupDraft.worldName;
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.selectedAction = FrontendAction::NewWorld;
      frontend.status = "new_world_closed";
      window.worldSetupStatus = "world_setup_back";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
      return;
    }
    frontend.status = "new_world_input_ignored";
    return;
  }

  if (frontend.childScreen == FrontendScreen::LoadSave) {
    if (action == InputAction::MenuBack) {
      frontend.childScreen = FrontendScreen::Gameplay;
      frontend.status = "load_save_closed";
      return;
    }
    if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
      moveSelectedProductSaveSlot(saves.slots, action, window);
      frontend.status = "load_save_selection_changed";
      return;
    }
    if (action == InputAction::MenuConfirm) {
      if (frontend.selectedAction == FrontendAction::Delete) {
        openProductSaveDeleteConfirmation(saves.slots, window, frontend);
      } else {
        launchProductLoadSaveSelection(options,
                                       productWorldTemplateFromOptions(options),
                                       saves,
                                       frontend,
                                       activeSession,
                                       window);
      }
      return;
    }
  }

  if (action == InputAction::MenuUp || action == InputAction::MenuDown) {
    frontend.selectedAction = nextStarterSelection(frontend.selectedAction, action);
    frontend.status = "opening_menu_selection_changed";
    return;
  }
  if (action == InputAction::MenuBack) {
    frontend.status = "opening_menu_back_requested";
    closeRequested = true;
    return;
  }
  if (action != InputAction::MenuConfirm) {
    return;
  }

  if (frontend.selectedAction == FrontendAction::Continue) {
    if (saves.slots.compatibleCount == 0) {
      frontend.status = "opening_menu_action_disabled";
      return;
    }
    launchProductContinueSave(options, productWorldTemplateFromOptions(options), saves,
                              frontend, activeSession, window);
    return;
  }
  if (frontend.selectedAction == FrontendAction::Exit) {
    frontend.status = "opening_menu_exit_requested";
    closeRequested = true;
    return;
  }
  if (frontend.selectedAction == FrontendAction::NewWorld) {
    frontend.childScreen = FrontendScreen::NewWorld;
    frontend.selectedAction = FrontendAction::CreateAndEnter;
    frontend.status = "opening_menu_new_world_selected";
    window.worldSetupTitle = worldSetupDraft.worldName;
    window.worldSetupStatus = "world_setup_open";
    return;
  }
  if (frontend.selectedAction == FrontendAction::LoadSave) {
    frontend.childScreen = FrontendScreen::LoadSave;
    initializeSelectedProductSaveSlot(saves.slots, window);
    frontend.status = "opening_menu_load_save_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::Settings) {
    frontend.childScreen = FrontendScreen::Settings;
    settingsTab = FrontendSettingsTab::Input;
    frontend.status = "opening_menu_settings_selected";
    return;
  }
  if (frontend.selectedAction == FrontendAction::DevTools) {
    frontend.childScreen = FrontendScreen::StarterDevTools;
    frontend.devToolsOpen = true;
    frontend.devToolsCategory = FrontendDevToolsCategory::Session;
    frontend.status = "opening_menu_dev_tools_selected";
    return;
  }
  frontend.status = "opening_menu_action_selected";
}

void routeOpeningMenuInput(FrontendState& frontend,
                           const ProductSaveBridgeResult& saves,
                           const ProductAppOptions& options,
                           FrontendSettingsTab& settingsTab,
                           std::optional<Session>& activeSession,
                           WorldSetupDraft& worldSetupDraft,
                           ActionState& actionState,
                           InputAction inputAction,
                           ProductAppWindowState& window,
                           bool& closeRequested) {
  if (inputAction == InputAction::None) {
    return;
  }

  if (frontend.screen == FrontendScreen::Gameplay &&
      inputAction == InputAction::MenuBack) {
    inputAction = InputAction::SystemPause;
  }

  recordAction(actionState, inputAction, true, true, false, 1.0F);

  InputRoutingContext routingContext;
  routingContext.owners.starter = frontend.screen == FrontendScreen::Starter;
  routingContext.owners.pause = frontend.screen == FrontendScreen::Pause;
  routingContext.owners.settings = frontend.screen == FrontendScreen::Settings ||
                                   frontend.childScreen == FrontendScreen::Settings;
  routingContext.owners.devTools = frontend.screen == FrontendScreen::DevOverlay ||
                                   frontend.childScreen == FrontendScreen::StarterDevTools;
  routingContext.owners.gameplay = frontend.screen == FrontendScreen::Gameplay &&
                                   window.gameplayActive;
  const InputRoutingResult routed = routeInputAction(routingContext, inputAction);
  window.inputOwner = routed.owner;
  window.lastInputAction = routed.action;
  window.lastInputAccepted = routed.accepted;
  window.gameplayInputSuppressed = routed.gameplaySuppressed;
  if (routed.accepted) {
    applyOpeningMenuAction(frontend, saves, options, settingsTab, activeSession,
                           worldSetupDraft, window, routed.action, closeRequested);
  }
}

struct ProductAutomationCommand {
  std::string key;
  std::string value;
};

bool parseAutomationBool(std::string_view value, bool& out) {
  if (value == "true" || value == "1" || value == "yes") {
    out = true;
    return true;
  }
  if (value == "false" || value == "0" || value == "no") {
    out = false;
    return true;
  }
  return false;
}

bool parseAutomationInputAction(std::string_view value, InputAction& out) {
  if (value == "up") {
    out = InputAction::MenuUp;
  } else if (value == "down") {
    out = InputAction::MenuDown;
  } else if (value == "left") {
    out = InputAction::MenuLeft;
  } else if (value == "right") {
    out = InputAction::MenuRight;
  } else if (value == "confirm") {
    out = InputAction::MenuConfirm;
  } else if (value == "back") {
    out = InputAction::MenuBack;
  } else if (value == "next_tab") {
    out = InputAction::MenuNextTab;
  } else if (value == "previous_tab") {
    out = InputAction::MenuPreviousTab;
  } else if (value == "none") {
    out = InputAction::None;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationFrontendAction(std::string_view value, FrontendAction& out) {
  if (value == "continue") {
    out = FrontendAction::Continue;
  } else if (value == "new_world") {
    out = FrontendAction::NewWorld;
  } else if (value == "load_save") {
    out = FrontendAction::LoadSave;
  } else if (value == "settings") {
    out = FrontendAction::Settings;
  } else if (value == "dev_tools") {
    out = FrontendAction::DevTools;
  } else if (value == "exit") {
    out = FrontendAction::Exit;
  } else if (value == "resume") {
    out = FrontendAction::Resume;
  } else if (value == "save") {
    out = FrontendAction::Save;
  } else if (value == "save_and_exit") {
    out = FrontendAction::SaveAndExit;
  } else if (value == "return_to_title") {
    out = FrontendAction::ReturnToTitle;
  } else if (value == "exit_game") {
    out = FrontendAction::ExitGame;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationSettingsTab(std::string_view value, FrontendSettingsTab& out) {
  if (value == "input") {
    out = FrontendSettingsTab::Input;
  } else if (value == "controls") {
    out = FrontendSettingsTab::Controls;
  } else if (value == "camera") {
    out = FrontendSettingsTab::Camera;
  } else if (value == "gameplay") {
    out = FrontendSettingsTab::Gameplay;
  } else if (value == "video_display") {
    out = FrontendSettingsTab::VideoDisplay;
  } else if (value == "audio") {
    out = FrontendSettingsTab::Audio;
  } else if (value == "accessibility") {
    out = FrontendSettingsTab::Accessibility;
  } else if (value == "developer") {
    out = FrontendSettingsTab::Developer;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationDevToolsCategory(std::string_view value,
                                     FrontendDevToolsCategory& out) {
  if (value == "session") {
    out = FrontendDevToolsCategory::Session;
  } else if (value == "input") {
    out = FrontendDevToolsCategory::Input;
  } else if (value == "player") {
    out = FrontendDevToolsCategory::Player;
  } else if (value == "movement") {
    out = FrontendDevToolsCategory::Movement;
  } else if (value == "world_editor") {
    out = FrontendDevToolsCategory::WorldEditor;
  } else if (value == "collision") {
    out = FrontendDevToolsCategory::Collision;
  } else if (value == "spells") {
    out = FrontendDevToolsCategory::Spells;
  } else if (value == "camera") {
    out = FrontendDevToolsCategory::Camera;
  } else if (value == "renderer") {
    out = FrontendDevToolsCategory::Renderer;
  } else if (value == "performance") {
    out = FrontendDevToolsCategory::Performance;
  } else {
    return false;
  }
  return true;
}

bool parseAutomationOwner(std::string_view value, MenuOwner& out) {
  if (value == "starter") {
    out = MenuOwner::Starter;
  } else if (value == "pause") {
    out = MenuOwner::Pause;
  } else if (value == "settings") {
    out = MenuOwner::Settings;
  } else if (value == "dev_tools") {
    out = MenuOwner::DevTools;
  } else {
    return false;
  }
  return true;
}

bool hasAutomationKey(const std::vector<ProductAutomationCommand>& commands,
                      const std::string& key) {
  for (const ProductAutomationCommand& command : commands) {
    if (command.key == key) {
      return true;
    }
  }
  return false;
}

bool readProductAutomationCommands(const std::filesystem::path& path,
                                   ProductAppWindowState& window,
                                   std::vector<ProductAutomationCommand>& commands) {
  window.automationControlRequested = !path.empty();
  window.automationControlPath = path.empty() ? "" : path.generic_string();
  if (path.empty()) {
    return false;
  }
  window.automationControlScope = "frontend_menu";
  std::ifstream input(path);
  if (!input) {
    window.automationControlStatus = "read_failed";
    window.automationControlLastResult = "failed";
    return false;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    ++window.automationControlLineCount;
    const std::size_t equals = line.find('=');
    if (equals == std::string::npos || equals == 0U) {
      window.automationControlStatus = "parse_error";
      window.automationControlLastKey = "none";
      window.automationControlLastResult = "failed";
      return false;
    }
    ProductAutomationCommand command{line.substr(0, equals), line.substr(equals + 1U)};
    if (hasAutomationKey(commands, command.key)) {
      window.automationControlStatus = "duplicate_key";
      window.automationControlLastKey = command.key;
      window.automationControlLastResult = "failed";
      return false;
    }
    commands.push_back(std::move(command));
  }
  window.automationControlLoaded = true;
  window.automationControlStatus = "loaded";
  return true;
}

void markAutomationApplied(ProductAppWindowState& window,
                           const ProductAutomationCommand& command,
                           std::string_view action,
                           MenuOwner owner,
                           std::string_view result) {
  window.automationControlLastKey = command.key;
  window.automationControlLastAction = std::string(action);
  window.automationControlLastOwner = owner;
  window.automationControlLastResult = std::string(result);
  if (result == "applied") {
    ++window.automationControlAppliedCount;
    window.automationControlStatus = "applied";
  }
}

bool automationFailurePreservesLoaded(std::string_view status) {
  return status == "applied" || status == "loaded";
}

bool routeAutomationInput(FrontendState& frontend,
                          const ProductSaveBridgeResult& saves,
                          const ProductAppOptions& options,
                          FrontendSettingsTab& settingsTab,
                          std::optional<Session>& activeSession,
                          WorldSetupDraft& worldSetupDraft,
                          ProductAppWindowState& window,
                          InputAction action,
                          bool& closeRequested) {
  ActionState actionState;
  routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                        worldSetupDraft, actionState, action, window, closeRequested);
  window.automationControlLastOwner = productInputOwnerFor(frontend, window);
  return window.lastInputAccepted || action == InputAction::None;
}

bool applyProductAutomationCommand(const ProductAutomationCommand& command,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   const ProductAppOptions& options,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  const std::string_view key{command.key};
  const std::string_view value{command.value};
  bool boolValue = false;
  InputAction inputAction = InputAction::None;

  if (key == "automation.owner") {
    MenuOwner expectedOwner = MenuOwner::None;
    if (!parseAutomationOwner(value, expectedOwner)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const MenuOwner currentOwner = productInputOwnerFor(frontend, window);
    if (currentOwner != expectedOwner) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, menuOwnerName(expectedOwner), currentOwner,
                            "failed");
      return false;
    }
    markAutomationApplied(window, command, menuOwnerName(expectedOwner), currentOwner,
                          "applied");
    return true;
  }

  if (key == "menu.input") {
    if (!parseAutomationInputAction(value, inputAction)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            inputAction, closeRequested);
    markAutomationApplied(window, command, inputActionName(inputAction),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  const bool menuBoolKey =
      key == "menu.up" || key == "menu.down" || key == "menu.left" ||
      key == "menu.right" || key == "menu.confirm" || key == "menu.back" ||
      key == "menu.next_tab" || key == "menu.previous_tab";
  if (menuBoolKey) {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    if (key == "menu.up") {
      inputAction = InputAction::MenuUp;
    } else if (key == "menu.down") {
      inputAction = InputAction::MenuDown;
    } else if (key == "menu.left") {
      inputAction = InputAction::MenuLeft;
    } else if (key == "menu.right") {
      inputAction = InputAction::MenuRight;
    } else if (key == "menu.confirm") {
      inputAction = InputAction::MenuConfirm;
    } else if (key == "menu.back") {
      inputAction = InputAction::MenuBack;
    } else if (key == "menu.next_tab") {
      inputAction = InputAction::MenuNextTab;
    } else {
      inputAction = InputAction::MenuPreviousTab;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            inputAction, closeRequested);
    markAutomationApplied(window, command, inputActionName(inputAction),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "frontend.select" || key == "pause.select") {
    FrontendAction action = FrontendAction::None;
    if (!parseAutomationFrontendAction(value, action)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    frontend.selectedAction = action;
    markAutomationApplied(window, command, frontendActionName(action),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.title" || key == "world_setup.title") {
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.title",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    worldSetupDraft.worldName = std::string(value);
    window.worldSetupTitle = worldSetupDraft.worldName;
    window.worldSetupStatus = "world_setup_title_updated";
    markAutomationApplied(window, command, "world.title",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "world.create" || key == "world_setup.create") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "world.create",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::NewWorld) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "world.create",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuConfirm, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuConfirm),
                          window.automationControlLastOwner,
                          routed ? "applied" : "failed");
    return routed;
  }

  if (key == "save.select" || key == "frontend.save_select") {
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.select",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const bool selected = selectProductSaveSlotById(saves.slots, value, window);
    markAutomationApplied(window,
                          command,
                          window.selectedProductSaveId,
                          productInputOwnerFor(frontend, window),
                          selected ? "applied" : "ignored");
    return selected;
  }

  if (key == "save.delete" || key == "frontend.save_delete") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.delete",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.delete",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    openProductSaveDeleteConfirmation(saves.slots, window, frontend);
    markAutomationApplied(window, command, "save.delete",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "save.show_deleted" || key == "frontend.show_deleted_saves") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.show_deleted",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.show_deleted",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    openDeletedProductSaveBrowser(options, window, frontend);
    markAutomationApplied(window, command, "save.show_deleted",
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "save.deleted_select" || key == "frontend.deleted_save_select") {
    if (frontend.childScreen != FrontendScreen::LoadSave ||
        !window.deletedSaveBrowserOpen) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.deleted_select",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    const ProductSaveBridgeResult deletedSaves =
        scanDeletedProductSavesForOptions(options);
    recordDeletedProductSaveSlots(deletedSaves, window);
    const bool selected =
        selectDeletedProductSaveSlotById(deletedSaves.slots, value, window);
    markAutomationApplied(window,
                          command,
                          window.deletedSelectedSaveId,
                          productInputOwnerFor(frontend, window),
                          selected ? "applied" : "ignored");
    return selected;
  }

  if (key == "save.recover" || key == "frontend.save_recover") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "save.recover",
                            productInputOwnerFor(frontend, window), "ignored");
      return true;
    }
    if (frontend.childScreen != FrontendScreen::LoadSave ||
        !window.deletedSaveBrowserOpen) {
      window.automationControlStatus = "owner_unavailable";
      markAutomationApplied(window, command, "save.recover",
                            productInputOwnerFor(frontend, window), "failed");
      return false;
    }
    executeProductSaveRecover(options, window, frontend);
    markAutomationApplied(window, command, "save.recover",
                          productInputOwnerFor(frontend, window),
                          window.saveRecoverExecuted ? "applied" : "failed");
    return window.saveRecoverExecuted;
  }

  if (key == "settings.tab") {
    if (!parseAutomationSettingsTab(value, settingsTab)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    window.selectedSettingsTab = settingsTab;
    markAutomationApplied(window, command, frontendSettingsTabName(settingsTab),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "dev_tools.category") {
    FrontendDevToolsCategory category = FrontendDevToolsCategory::None;
    if (!parseAutomationDevToolsCategory(value, category)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    frontend.devToolsCategory = category;
    markAutomationApplied(window, command, frontendDevToolsCategoryName(category),
                          productInputOwnerFor(frontend, window), "applied");
    return true;
  }

  if (key == "frontend.execute" || key == "pause.execute" ||
      key == "dev_tools.execute") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuConfirm, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuConfirm),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "settings.apply" || key == "settings.restore_defaults") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      frontend.status = key == "settings.apply" ? "settings_applied"
                                                : "settings_defaults_restored";
    }
    markAutomationApplied(window, command, key == "settings.apply" ? "settings.apply"
                                                                   : "settings.restore_defaults",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  if (key == "settings.back" || key == "system.back") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::MenuBack, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::MenuBack),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "frontend.return_to_title") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      returnProductToTitleTransition(frontend, window);
      activeSession.reset();
    }
    markAutomationApplied(window, command, "frontend.return_to_title",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  if (key == "system.pause") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (!boolValue) {
      markAutomationApplied(window, command, "none", productInputOwnerFor(frontend, window),
                            "ignored");
      return true;
    }
    const bool routed = routeAutomationInput(frontend, saves, options, settingsTab,
                                            activeSession, worldSetupDraft, window,
                                            InputAction::SystemPause, closeRequested);
    markAutomationApplied(window, command, inputActionName(InputAction::SystemPause),
                          window.automationControlLastOwner,
                          routed ? "applied" : "ignored");
    return routed;
  }

  if (key == "system.quit") {
    if (!parseAutomationBool(value, boolValue)) {
      window.automationControlStatus = "invalid_value";
      return false;
    }
    if (boolValue) {
      closeRequested = true;
    }
    markAutomationApplied(window, command, "system.quit",
                          productInputOwnerFor(frontend, window),
                          boolValue ? "applied" : "ignored");
    return true;
  }

  window.automationControlStatus = "unknown_key";
  return false;
}

void applyProductAutomationControl(const ProductAppOptions& options,
                                   FrontendState& frontend,
                                   const ProductSaveBridgeResult& saves,
                                   FrontendSettingsTab& settingsTab,
                                   std::optional<Session>& activeSession,
                                   WorldSetupDraft& worldSetupDraft,
                                   ProductAppWindowState& window,
                                   bool& closeRequested) {
  std::vector<ProductAutomationCommand> commands;
  if (!readProductAutomationCommands(options.automationControlPath, window, commands)) {
    return;
  }
  for (const ProductAutomationCommand& command : commands) {
    if (!applyProductAutomationCommand(command, frontend, saves, options, settingsTab,
                                       activeSession, worldSetupDraft, window,
                                       closeRequested)) {
      if (window.automationControlLastKey == "none") {
        window.automationControlLastKey = command.key;
      }
      if (window.automationControlLastAction == "none") {
        window.automationControlLastAction = command.value.empty() ? "none" : command.value;
      }
      window.automationControlLastOwner = productInputOwnerFor(frontend, window);
      window.automationControlLastResult = "failed";
      if (automationFailurePreservesLoaded(window.automationControlStatus)) {
        window.automationControlStatus = "command_failed";
      } else {
        window.automationControlLoaded = false;
      }
      return;
    }
  }
  if (window.automationControlStatus == "loaded" && commands.empty()) {
    window.automationControlLastResult = "none";
  }
  window.selectedSettingsTab = settingsTab;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed = frontendBlocksGameplayInput(frontend);
}

ProductAppWindowState runOpeningMenuWindow(const ProductAppOptions& options,
                                           const ProductWorldTemplate& world,
                                           FrontendState& frontend,
                                           std::optional<Session>& activeSession,
                                           WorldSetupDraft& worldSetupDraft,
                                           ProductAppWindowState window,
                                           const FrontendSettings& settings,
                                           const ProductSaveBridgeResult& saves) {
  window.requested = options.windowMode == ProductWindowMode::Window;
  window.inputOwner = productInputOwnerFor(frontend, window);
  window.gameplayInputSuppressed = frontendBlocksGameplayInput(frontend);
  if (!window.requested) {
    return window;
  }

#if defined(IGGY3D_HAS_SDL3)
  window.sdlAvailable = true;

  SdlWindowCreateInfo createInfo;
  createInfo.title = "iggy3d - Opening Menu";
  createInfo.width = 1280;
  createInfo.height = 720;
  createInfo.resizable = true;
  createInfo.highDpi = true;
  createInfo.vulkan = false;

  SdlWindow sdlWindow(createInfo);
  window.created = sdlWindow.nativeWindow() != nullptr;
  window.drawable = sdlWindow.isDrawable();
  window.openingMenuVisible = window.created && frontend.screen == FrontendScreen::Starter;
  if (!window.created) {
    window.status = "window_create_failed";
    return window;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(sdlWindow.nativeWindow(), nullptr);
  if (renderer == nullptr) {
    window.status = "renderer_create_failed";
    return window;
  }

  sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");
  const auto start = std::chrono::steady_clock::now();
  KeyboardInputState keyboard;
  MouseInputState mouse;
  GamepadMenuState gamepad;
  initializeGamepadMenuState(gamepad);
  window.gamepadAvailable = gamepad.gamepadAvailable;
  window.gamepadName = gamepad.gamepadName;
  window.gamepadMapping = gamepad.gamepadAvailable ? "sdl_gamepad" : "unavailable";
  bool closeRequested = false;
  FrontendSettingsTab settingsTab = FrontendSettingsTab::Input;
  while (sdlWindow.isOpen()) {
    ActionState actionState;
    sdlWindow.pollEvents();
    ++window.eventPollCount;
    window.drawable = sdlWindow.isDrawable();
    sdlWindow.setTitle(window.gameplayActive ? "iggy3d - Gameplay" : "iggy3d - Opening Menu");

    routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                          worldSetupDraft, actionState, pollKeyboardMenuAction(keyboard),
                          window, closeRequested);

    const InputAction gamepadAction = pollGamepadMenuAction(gamepad);
    if (gamepadAction != InputAction::None) {
      window.gamepadMenuSelectUsed = true;
      routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                            worldSetupDraft, actionState, gamepadAction, window,
                            closeRequested);
    }

    const MouseClick click = pollMouseClick(mouse);
    if (click.clicked) {
      const OpeningMenuHitTestResult hit = openingMenuActionAt(frontend, click.x, click.y);
      if (hit.hit) {
        window.mouseMenuSelectUsed = true;
        if (hit.area == OpeningMenuHitArea::StarterAction) {
          frontend.selectedAction = hit.action;
          routeOpeningMenuInput(frontend, saves, options, settingsTab, activeSession,
                                worldSetupDraft, actionState, mouseClickAction(click),
                                window, closeRequested);
        } else if (hit.area == OpeningMenuHitArea::DevToolsCategory) {
          frontend.devToolsCategory = hit.devToolsCategory;
          frontend.status = "dev_tools_category_selected";
        } else if (hit.area == OpeningMenuHitArea::SettingsTab) {
          settingsTab = hit.settingsTab;
          frontend.status = "settings_tab_selected";
        }
      }
    }

    if (window.gameplayActive && activeSession.has_value() &&
        !frontendBlocksGameplayInput(frontend)) {
      ActionState gameplayActions;
      pollKeyboardGameplayActions(keyboard, gameplayActions);
      pollGamepadGameplayActions(gamepad, gameplayActions);
      pollMouseGameplayActions(mouse, gameplayActions);

      ActionState acceptedGameplayActions;
      InputRoutingContext routingContext;
      routingContext.owners.gameplay = true;
      for (const ActionStateEntry& entry : gameplayActions.entries) {
        const InputRoutingResult routed = routeInputAction(routingContext, entry.action);
        window.inputOwner = routed.owner;
        window.lastInputAction = routed.action;
        window.lastInputAccepted = routed.accepted;
        window.gameplayInputSuppressed = routed.gameplaySuppressed;
        if (routed.accepted) {
          recordAction(acceptedGameplayActions, entry.action, entry.down, entry.pressed,
                       entry.released, entry.value);
        }
      }
      applyProductCameraActions(acceptedGameplayActions, window.viewport, settings,
                                "action_map");
      applyProductGameplayActions(*activeSession, acceptedGameplayActions, window,
                                  "action_map");
    }

    SceneProjectionResult scene;
    DebugProjectionResult debug;
    ProductPrimitiveDrawList drawList;
    ProductViewportFrame frame;
    std::size_t sceneItemCount = 0;
    const SceneProjectionResult* scenePtr = nullptr;
    const DebugProjectionResult* debugPtr = nullptr;
    const ProductPrimitiveDrawList* drawListPtr = nullptr;
    const ProductViewportFrame* framePtr = nullptr;
    const ProductRenderBridgeFrame* bridgePtr = nullptr;
    ProductGameplayFeedback feedback = buildProductGameplayFeedback(window);
    ProductRenderBridgeFrame bridge;
    if (window.gameplayActive && activeSession.has_value()) {
      scene = buildSceneProjection(activeSession->state());
      debug = buildDebugProjection(activeSession->state());
      drawList = buildProductPrimitiveDrawList(&scene, &debug);
      frame = buildProductViewportFrame(
          drawList, ProductViewportFrameConfig{window.viewport.cameraYawDegrees,
                                               window.viewport.cameraPitchDegrees});
      scenePtr = &scene;
      debugPtr = &debug;
      drawListPtr = &drawList;
      framePtr = &frame;
      sceneItemCount = scene.items.size();
      window.runtimeStateHash = activeSession->stateHash();
      feedback = buildProductGameplayFeedback(window);
      bridge = buildProductRenderBridgeFrame(&drawList, &frame, &feedback);
      bridgePtr = &bridge;
    }

    if (window.drawable) {
      const OpeningMenuViewState view =
          drawOpeningMenuView(*renderer, options, world, frontend, settingsTab,
                              window.gameplayActive, window.runtimeStateHash, framePtr,
                              &feedback, sceneItemCount, debugPtr,
                              window.viewport.cameraYawDegrees,
                              window.viewport.cameraPitchDegrees, saves);
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr,
                                     window.gameplayActive && scenePtr != nullptr);
      window.viewport.cameraHeadingVisible =
          window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
      window.menuTextDrawn = window.menuTextDrawn || view.textDrawn;
      window.selectedRowDrawn = window.selectedRowDrawn || view.selectedRowDrawn;
      window.menuRowCount = view.rowCount;
    } else {
      applyGameplayProjectionMetrics(window, scenePtr, debugPtr, drawListPtr, framePtr,
                                     bridgePtr, false);
    }
    ++window.framesPresented;

    if (options.frames > 0 && window.framesPresented >= options.frames) {
      break;
    }
    if (closeRequested) {
      break;
    }
    if (options.holdSeconds > 0) {
      const auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed >= std::chrono::seconds(options.holdSeconds)) {
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  shutdownGamepadMenuState(gamepad);
  SDL_DestroyRenderer(renderer);
  window.selectedSettingsTab = settingsTab;
  window.status = window.menuTextDrawn ? "opening_menu_text_ready"
                                       : "opening_menu_window_ready";
  return window;
#else
  (void)world;
  (void)frontend;
  (void)worldSetupDraft;
  (void)saves;
  window.sdlAvailable = false;
  window.created = false;
  window.drawable = false;
  window.openingMenuVisible = false;
  window.status = "sdl3_unavailable";
  return window;
#endif
}

}  // namespace

int runProductApp(int argc, char** argv) {
  const ProductAppOptionsParseResult parsed = parseProductAppOptions(argc, argv);
  if (parsed.status == ProductAppOptionStatus::Help) {
    std::cout << productAppHelpText();
    return 0;
  }
  if (parsed.status != ProductAppOptionStatus::Ok) {
    RenderReceipt receipt;
    appendReceiptField(receipt, "app", "iggy3d");
    appendReceiptField(receipt, "result", "fail");
    appendReceiptField(receipt, "reason_code", productAppOptionStatusReason(parsed.status));
    appendReceiptField(receipt, "option", parsed.option);
    std::cout << formatRenderReceipt(receipt);
    return 2;
  }

  const ProductAppOptions& options = parsed.options;
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductSaveBridgeResult saves =
      scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  const FrontendSettings settings = productFrontendSettingsFromOptions(options);
  std::optional<Session> activeSession;
  WorldSetupDraft worldSetupDraft = makeDefaultWorldSetupDraft();
  ProductAppWindowState window;
  window.worldSetupTitle = worldSetupDraft.worldName;

  FrontendState frontend;
  initializeProductStarterTransition(frontend, window, saves.slots.compatibleCount > 0);

  if (options.autoNewWorld) {
    launchProductNewWorld(options, worldSetupDraft, frontend, activeSession, window);
  }
  if (options.scriptedGameplaySmoke) {
    runScriptedProductGameplaySmoke(activeSession, window);
    ActionState scriptedLook;
    recordAction(scriptedLook, InputAction::PlayerLookX, true, false, false, 1.0F);
    recordAction(scriptedLook, InputAction::PlayerLookY, true, false, false, 0.5F);
    applyProductCameraActions(scriptedLook, window.viewport, settings, "scripted");
  }

  FrontendSettingsTab automationSettingsTab = FrontendSettingsTab::None;
  bool automationCloseRequested = false;
  applyProductAutomationControl(options, frontend, saves, automationSettingsTab,
                                activeSession, worldSetupDraft, window,
                                automationCloseRequested);
  saves = scanProductSaves(options.saveRoot, world.packageId, world.scenarioId);
  if (automationCloseRequested) {
    window.status = "automation_close_requested";
  }

  window =
      runOpeningMenuWindow(options, world, frontend, activeSession, worldSetupDraft,
                           window, settings, saves);
  refreshGameplayProjectionMetrics(activeSession, window);

  if (options.printRenderReceipt) {
    std::cout << formatRenderReceipt(
        buildProductAppReceipt(options, world, frontend, settings, window, saves));
  }

  return window.requested && !window.created ? 77 : 0;
}

}  // namespace iggy3d
