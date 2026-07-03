#include "app/iggy3d/Operations.hpp"

#include <filesystem>
#include <string>

#include "app/PackageRuntimeLookup.hpp"
#include "app/frontend/SaveBrowser.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Package.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/world/Creation.hpp"
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
  lookupConfig.packageMode = PackageMode::BuildTreeProduct;
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

ProductAsciiRoomAuthoringRequest productAsciiRoomAuthoringRequestFromWorldSetup(
    const WorldSetupDraft& draft) {
  return productWorldSetupAuthoringRequest(draft);
}

bool createProductSessionFromPackage(const PackageLoadResult& package,
                                     std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
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
  // MA1: initialize the window's mutable tuning copy from the session's resolved dimension profile
  // (earth_standard maps byte-identical to today's default; giant_lowgrav tunes it up).
  applyMovementDimensionProfileToTuning(activeSession->state().movementProfile,
                                        window.gameplayMovementTuning);
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

bool createProductSession(const ProductAppOptions& options,
                          std::optional<Session>& activeSession,
                          ProductAppWindowState& window) {
  const std::filesystem::path packagePath = defaultProductPackagePath(options);
  const PackageLoadResult package = loadPackage({packagePath.generic_string()});
  return createProductSessionFromPackage(package, activeSession, window);
}

ProductWorldCreationResult prepareProductWorldCreationFromDraft(
    const ProductAppOptions& options,
    const ProductWorldTemplate& world,
    const WorldSetupDraft& draft,
    ProductAppWindowState& window) {
  window.worldSetupTitle = draft.worldName;
  window.worldSetupDungeonTitle = draft.worldName;
  window.worldSetupDungeonCount = productBuiltinDungeonCatalog().size();
  const std::size_t dungeonIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  window.worldSetupDungeonIndex = 0;
  // branch-gate: BG-1137
  if (dungeonIndex < productBuiltinDungeonCatalog().size()) {
    window.worldSetupDungeonIndex =
        static_cast<std::uint64_t>(dungeonIndex + 1U);
  }
  window.worldSetupAsciiRoomEnabled = draft.asciiRoomEnabled;
  window.worldSetupAsciiRoomTextPresent = !draft.asciiRoomText.empty();
  window.worldSetupAsciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  window.worldSetupAsciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
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

  // createdAtUtc/savedAtUtc seed for the initial save: a real UTC timestamp so
  // the catalog's newest-first ordering and Continue policy reflect real
  // creation/save recency across worlds. The world id is minted unique per
  // world (the first world in an empty save root is world_0001) and preserved
  // across re-saves by carry-forward in the durable writer.
  ProductWorldCreationResult creation = prepareProductWorldCreation(
      makeProductWorldCreationInput(setup.createRequest,
                                    world,
                                    options.saveRoot,
                                    productSaveTimestampNowUtc(),
                                    nextProductWorldId(options.saveRoot)));
  window.worldCreationStatus = std::string(creation.status);
  window.worldCreationReasonCode = std::string(creation.reasonCode);
  window.worldCreationWorldId =
      creation.request.worldId.empty() ? "none" : creation.request.worldId;
  window.worldCreationWorldTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.worldCreationAsciiRoomRequested = creation.request.asciiRoomRequested;
  window.worldCreationAsciiRoomId =
      creation.request.asciiRoomId.empty() ? "none" : creation.request.asciiRoomId;
  window.worldCreationAsciiRoomSourceName =
      creation.request.asciiRoomSourceName.empty()
          ? "none"
          : creation.request.asciiRoomSourceName;
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
  window.worldCreationAsciiRoomRequested =
      initialSave.creation.request.asciiRoomRequested;
  window.worldCreationAsciiRoomId =
      initialSave.creation.request.asciiRoomId.empty()
          ? "none"
          : initialSave.creation.request.asciiRoomId;
  window.worldCreationAsciiRoomSourceName =
      initialSave.creation.request.asciiRoomSourceName.empty()
          ? "none"
          : initialSave.creation.request.asciiRoomSourceName;
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

void recordProductSaveLoadResult(const ProductSaveLoadResult& loaded,
                                 ProductAppWindowState& window) {
  window.productSaveLoadResult = loaded;
  // Normalize an empty save id to "none" at the producer so ReceiptBuilder
  // stays a straight read of the typed result (preserves the prior receipt).
  if (window.productSaveLoadResult.record.id.empty()) {
    window.productSaveLoadResult.record.id = "none";
  }
  if (loaded.ok && !loaded.record.id.empty()) {
    window.activeProductSaveId = loaded.record.id;
  }
}

void recordSavedRoomMarkerBindingResult(
    const ProductSavedRoomMarkerBindingResult& bound,
    ProductAppWindowState& window) {
  window.savedMarkerBind = bound;
  // Normalize an empty room id to "none" at the producer so ReceiptBuilder
  // stays a straight read of the typed result (preserves the prior receipt).
  if (window.savedMarkerBind.roomId.empty()) {
    window.savedMarkerBind.roomId = "none";
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
  const SaveSlotRingModel ring =
      // branch-gate: BG-1020
      buildSaveSlotRingModel(slots, slot == nullptr ? "none" : slot->id);
  window.saveSlotRingCount = static_cast<std::uint64_t>(ring.items.size());
  window.saveSlotRingSelectedIndex = ring.selectedIndex;
  window.saveSlotRingSelectedId = ring.selectedSlotId;
  window.saveSlotRingSelectedStatus = ring.selectedStatus;

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

void recordProductSaveSlotAction(ProductAppWindowState& window,
                                 const SaveSlotActionSpec& action,
                                 std::string_view status) {
  window.saveSlotActionCommand = std::string(saveSlotCommandName(action.command));
  window.saveSlotActionEnabled = action.enabled;
  window.saveSlotActionConfirmationRequired = action.confirmationRequired;
  window.saveSlotActionStatus = std::string(status);
}

void recordProductSaveFlowRequest(const ProductSaveFlowRequest& request,
                                  ProductAppWindowState& window) {
  window.saveFlowOperation =
      std::string(productSaveFlowOperationName(request.operation));
  // branch-gate: BG-1020
  window.saveFlowSourceSurface = request.sourceSurface.empty()
                                     ? "none"
                                     : request.sourceSurface;
  window.saveFlowAffectedSlotId =
      // branch-gate: BG-1020
      request.slotId.empty() ? "none" : request.slotId;
}

void recordProductSaveFlowResult(ProductSaveFlowOperation operation,
                                 std::string_view sourceSurface,
                                 const ProductSaveFlowResult& result,
                                 ProductAppWindowState& window) {
  window.saveFlowOperation =
      std::string(productSaveFlowOperationName(operation));
  // branch-gate: BG-1020
  window.saveFlowSourceSurface =
      sourceSurface.empty() ? "none" : std::string(sourceSurface);
  window.saveFlowStatus = result.status;
  window.saveFlowReasonCode = result.reason;
  window.saveFlowAffectedSlotId =
      // branch-gate: BG-1020
      result.affectedSlotId.empty() ? "none" : result.affectedSlotId;
  window.saveFlowActiveCountBefore = result.activeCountBefore;
  window.saveFlowActiveCountAfter = result.activeCountAfter;
  window.saveFlowDeletedCountAfter = result.deletedCountAfter;
  window.saveFlowSelectedSlotAfter =
      // branch-gate: BG-1020
      result.selectedSlotAfter.empty() ? "none" : result.selectedSlotAfter;
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

std::string_view productSaveFlowOperationName(ProductSaveFlowOperation operation) {
  // branch-gate: BG-1020
  switch (operation) {
    case ProductSaveFlowOperation::None:
      return "none";
    case ProductSaveFlowOperation::Delete:
      return "delete";
  }
  return "none";
}

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
  if (window.activeRoom.hasAuthoredRoom) {
    request.authoredRoom = &window.activeRoom.authoredRoom;
  }
  // A pause/progress save is a manual save and advances the save time. The
  // remaining identity (worldId/worldTitle/saveTitle/createdAtUtc) is left
  // empty on purpose: writeProductSessionSaveDurably carries it forward from
  // the existing save on disk so re-saving never wipes the world identity.
  request.saveType = "manual";
  request.savedAtUtc = productSaveTimestampNowUtc();
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

  const bool previous = action == InputAction::MenuUp;
  const std::string selectedId =
      nextSaveSlotRingSelection(slots, window.selectedProductSaveId, previous);
  const SaveSlotPreview* selected = saveSlotById(slots, selectedId);
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
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Load;
  window.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
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

  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  const ProductSaveMutationResult mutation = recoverProductSaveAndRefresh({
      options.saveRoot,
      recoverId,
      world.packageId,
      world.scenarioId,
  });
  const ProductSaveRecoverResult& recovered = mutation.recover;
  window.saveRecoverStatus = recovered.status;
  window.saveRecoverReasonCode = recovered.reasonCode;
  window.saveRecoverExecuted = recovered.ok;
  window.saveRecoverSaveId = recovered.saveId.empty() ? "none" : recovered.saveId;
  window.saveRecoverSnapshotRecovered = recovered.snapshotRecovered;
  window.saveRecoverSnapshotMissing = recovered.snapshotMissing;

  recordDeletedProductSaveSlots(mutation.deletedSaves, window);
  if (recovered.ok) {
    window.deletedSaveBrowserOpen = false;
    recordSelectedDeletedProductSaveSlot(mutation.deletedSaves.slots, nullptr, window);
    selectProductSaveSlotById(mutation.activeSaves.slots, recovered.saveId, window);
  }

  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.selectedAction = FrontendAction::LoadSave;
  frontend.status = recovered.ok ? "save_recover_recovered" : "save_recover_failed";
}

void openProductSaveDeleteConfirmation(const SaveSlotList& slots,
                                       ProductAppWindowState& window,
                                       FrontendState& frontend) {
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  const SaveSlotPreview* slot =
      window.selectedProductSaveId == "none"
          ? nullptr
          : saveSlotById(slots, window.selectedProductSaveId);
  if (slot == nullptr) {
    recordSelectedProductSaveSlot(slots, nullptr, window);
    recordProductSaveSlotAction(
        window,
        SaveSlotActionSpec{FrontendAction::Delete,
                           SaveSlotCommand::Delete,
                           "DELETE SELECTED",
                           false,
                           true,
                           "save_delete_unavailable"},
        "save_slot_action_disabled");
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
  recordProductSaveSlotAction(
      window,
      SaveSlotActionSpec{FrontendAction::Delete,
                         SaveSlotCommand::Delete,
                         "DELETE SELECTED",
                         true,
                         true,
                         "none"},
      "save_slot_action_confirm_requested");
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
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = "save_delete_cancelled";
}

ProductSaveFlowResult executeProductSaveSoftDelete(
    const ProductAppOptions& options,
    ProductSaveBridgeResult& saves,
    ProductAppWindowState& window,
    FrontendState& frontend) {
  ProductSaveFlowResult flow;
  flow.activeCountBefore = static_cast<std::uint64_t>(saves.slots.slots.size());
  ProductSaveFlowRequest request;
  request.operation = ProductSaveFlowOperation::Delete;
  request.slotId = window.saveDeleteCandidateId;
  request.sourceSurface = "delete_world_browser";
  request.confirmationToken = window.saveDeleteConfirmationOpen
                                  ? "delete_confirm_open"
                                  : "delete_confirm_missing";
  recordProductSaveFlowRequest(request, window);
  window.saveDeleteConfirmationOpen = false;
  window.saveDeleteType = "soft";
  window.saveDeleteRecoverable = false;
  if (window.saveDeleteCandidateId == "none" ||
      window.saveDeleteCandidateId.empty()) {
    window.saveDeleteStatus = "product_save_delete_id_missing";
    window.saveDeleteReasonCode = "product_save_delete_id_missing";
    window.saveDeleteExecuted = false;
    frontend.childScreen = FrontendScreen::LoadSave;
    frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
    window.saveSlotBrowserMode =
        std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
    frontend.status = "save_delete_failed";
    flow.status = window.saveDeleteStatus;
    flow.reason = window.saveDeleteReasonCode;
    flow.affectedSlotId = "none";
    flow.activeCountAfter = flow.activeCountBefore;
    flow.deletedCountAfter = window.deletedSaveCount;
    flow.selectedSlotAfter = window.selectedProductSaveId;
    recordProductSaveFlowResult(ProductSaveFlowOperation::Delete,
                                "delete_world_browser",
                                flow,
                                window);
    return flow;
  }

  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  const ProductSaveMutationResult mutation = softDeleteProductSaveAndRefresh({
      options.saveRoot,
      window.saveDeleteCandidateId,
      world.packageId,
      world.scenarioId,
  });
  const ProductSaveSoftDeleteResult& deleted = mutation.softDelete;
  window.saveDeleteStatus = deleted.status;
  window.saveDeleteReasonCode = deleted.reasonCode;
  window.saveDeleteExecuted = deleted.ok;
  window.saveDeleteRecoverable = deleted.ok;
  flow.ok = deleted.ok;
  flow.status = deleted.status;
  flow.reason = deleted.reasonCode;
  // branch-gate: BG-1020
  flow.affectedSlotId = deleted.saveId.empty() ? "none" : deleted.saveId;
  if (deleted.ok) {
    window.selectedProductSaveEnabled = false;
    window.selectedProductSaveStatus = "missing";
    saves = mutation.activeSaves;
    initializeSelectedProductSaveSlot(saves.slots, window);
  }
  recordDeletedProductSaveSlots(mutation.deletedSaves, window);
  flow.activeCountAfter = static_cast<std::uint64_t>(saves.slots.slots.size());
  flow.deletedCountAfter =
      static_cast<std::uint64_t>(mutation.deletedSaves.slots.slots.size());
  flow.selectedSlotAfter = window.selectedProductSaveId;
  frontend.childScreen = FrontendScreen::LoadSave;
  frontend.saveBrowserMode = FrontendSaveBrowserMode::Delete;
  window.saveSlotBrowserMode =
      std::string(frontendSaveBrowserModeName(frontend.saveBrowserMode));
  frontend.status = deleted.ok ? "save_delete_soft_deleted" : "save_delete_failed";
  recordProductSaveFlowResult(ProductSaveFlowOperation::Delete,
                              "delete_world_browser",
                              flow,
                              window);
  return flow;
}

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.launchAction = "create_and_enter";
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductWorldCreationResult creation =
      prepareProductWorldCreationFromDraft(options, world, worldSetupDraft, window);
  if (!creation.accepted) {
    window.launchStatus = std::string(creation.reasonCode);
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  ProductAsciiRoomAuthoringRequest asciiRequest;
  ProductAsciiRoomAuthoringResult asciiRoom;
  const SaveAuthoredRoomSection* initialSaveAuthoredRoom = nullptr;
  if (worldSetupDraft.asciiRoomEnabled) {
    asciiRequest = productAsciiRoomAuthoringRequestFromWorldSetup(worldSetupDraft);
    asciiRoom = buildProductAsciiRoomAuthoring(asciiRequest);
    recordProductAsciiRoomPreview(asciiRequest.sourceName,
                                  asciiRequest.roomId,
                                  asciiRoom,
                                  window);
    if (!asciiRoom.ok) {
      window.launchStatus = asciiRoom.reasonCode;
      frontend.status = "opening_menu_new_world_failed";
      return;
    }

    const PackageLoadResult asciiPackage =
        makeProductAsciiRoomPackage(asciiRoom.roomAsset.room,
                                    world.packageId,
                                    world.scenarioId);
    if (!createProductSessionFromPackage(asciiPackage, activeSession, window)) {
      frontend.status = "opening_menu_new_world_failed";
      return;
    }
    window.activeRoom = buildProductActiveRoomFromAsciiAuthoring(asciiRequest, asciiRoom);
    window.activeRoomCollision =
        buildProductActiveRoomCollision(window.activeRoom, activeSession->state());
    initialSaveAuthoredRoom = &asciiRoom.authoredRoom.authoredRoom;
  } else if (!createProductSession(options, activeSession, window)) {
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  ProductWorldInitialSaveRequest initialSaveRequest;
  initialSaveRequest.creation = creation;
  initialSaveRequest.state = &activeSession->state();
  initialSaveRequest.authoredRoom = initialSaveAuthoredRoom;
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
    window.productSaveLoadResult.status = "no_compatible_save";
    window.productSaveLoadResult.reasonCode = "no_compatible_save";
    frontend.status = source == "load_save_selector"
                          ? "load_save_action_disabled"
                          : "opening_menu_action_disabled";
    return;
  }
  if (!slot->enabled || slot->compatibility != SaveSlotCompatibility::Compatible) {
    const std::string reason =
        slot->reason.empty() ? "save_slot_disabled" : slot->reason;
    window.launchStatus = reason;
    window.productSaveLoadResult.status = reason;
    window.productSaveLoadResult.reasonCode = reason;
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

  if (loaded.authoredRoomPresent) {
    window.activeRoom =
        buildProductActiveRoomFromSavedAuthoredRoom(loaded.authoredRoom);
    const ProductSavedRoomMarkerBindingResult bound =
        bindSavedRoomMarkersToSession(window.activeRoom, *activeSession);
    recordSavedRoomMarkerBindingResult(bound, window);
    if (!bound.ok) {
      window.launchStatus = bound.reasonCode;
      clearProductGameplayLaunchState(activeSession, window);
      frontend.status = source == "load_save_selector"
                            ? "load_save_launch_failed"
                            : "opening_menu_continue_failed";
      return;
    }
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
