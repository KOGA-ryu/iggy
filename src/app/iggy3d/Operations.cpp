#include "app/iggy3d/Operations.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>

#include "app/frontend/SaveBrowser.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Package.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/creative/BakedActiveRoomRefresh.hpp"
#include "app/iggy3d/creative/CreativeWorldOperations.hpp"
#include "app/iggy3d/world/DefaultWorldTemplate.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/ProductWorldTemplateOperations.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"
#include "app/iggy3d/save/RoomMarkerBinding.hpp"
#include "app/iggy3d/save/SaveSlotOperations.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/world/Creation.hpp"
#include "content/PackageLoader.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
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

std::uint64_t elapsedMicroseconds(
    std::chrono::steady_clock::time_point started) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - started)
          .count());
}

ProductAsciiRoomAuthoringRequest productAsciiRoomAuthoringRequestFromWorldSetup(
    const WorldSetupDraft& draft) {
  return productWorldSetupAuthoringRequest(draft);
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

// F0 (blank stage): a CREATIVE world must stand on its own empty canvas, not
// the first_room demo ("Loop Keep") that createProductSession installs. Build a
// minimal session that seeds ONLY a local player at the world origin and leaves
// active-room state empty, so no demo room geometry projects. The map_maker grid
// (surfaced for the creative-document surface in ProjectionRefresh) draws the
// visible ground grid around the origin. Product / LegacyMapMaker launches
// keep calling createProductSession untouched.
bool createCreativeBlankSessionImpl(std::optional<Session>& activeSession,
                                    ProductAppWindowState& window) {
  const auto lookupStarted = std::chrono::steady_clock::now();
  window.frontendShell.startup.packagePath = "creative_blank_stage";
  window.frontendShell.startup.packageLookupMeasured = true;
  window.frontendShell.startup.packageLookupMicroseconds = elapsedMicroseconds(lookupStarted);
  window.frontendShell.startup.packageLookupStatus = "startup_package_lookup_resolved";

  const auto loadStarted = std::chrono::steady_clock::now();
  window.frontendShell.packageLoadStatus = "ok";
  window.frontendShell.startup.packageLoadMeasured = true;
  window.frontendShell.startup.packageLoadMicroseconds = elapsedMicroseconds(loadStarted);
  window.frontendShell.startup.packageLoadStatus = "ok";

  const auto sessionStarted = std::chrono::steady_clock::now();
  FixtureScenarioSeed seed;
  seed.scenarioId = "creative_blank";
  ScenarioEntitySeed player;
  player.stableName = "player";
  player.kind = EntityKind::Player;
  player.transform = identityTransform3();
  player.localBounds = makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  player.active = true;
  player.persistent = true;
  seed.players.push_back({0, PlayerSlotKind::Local, "player"});
  seed.entities.push_back(std::move(player));
  // Session::create requires at least one objective. A creative stage has no
  // gameplay goal, so seed a single inert objective: condition "None" +
  // initialStatus Active never self-completes (only an ObjectiveTrigger would),
  // so the session stays Playing and never finalizes an outcome.
  ScenarioObjectiveSeed stageObjective;
  stageObjective.id = "creative_blank_stage";
  stageObjective.initialStatus = ObjectiveStatusSeed::Active;
  stageObjective.condition = "None";
  stageObjective.playerSlot = 0;
  seed.objectives.push_back(std::move(stageObjective));

  SessionCreateRequest create;
  create.packageId = "iggy3d.creative_blank";
  create.seed = seed;
  create.config = seed.config;

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
  window.frontendShell.startup.runtimeSessionCreateStatus = "startup_runtime_session_created";

  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession = std::move(session.value);
  window.gameplay.runtimeSessionCreated = true;
  window.gameplay.gameplayActive = true;
  window.frontendShell.launchStatus = "runtime_session_created";
  return true;
}

// F0: place the creative fly camera on the world origin and pitch it down so
// the origin ground grid (where objects will be created) is framed on entry.
void frameCreativeStageCameraOnOriginImpl(ProductAppWindowState& window) {
  bumpCreativeWorldEpoch(window);
  seedCreativeFlyAnchorFromOrigin(window);
  window.viewport.cameraYawDegrees = 0.0F;
  window.viewport.cameraPitchDegrees = -30.0F;
}

ProductWorldCreationResult prepareProductWorldCreationFromDraft(
    const ProductAppOptions& options,
    const ProductWorldTemplate& world,
    const WorldSetupDraft& draft,
    ProductAppWindowState& window) {
  window.creativeAuthoring.worldSetup.title = draft.worldName;
  window.creativeAuthoring.worldSetup.dungeonTitle = draft.worldName;
  window.creativeAuthoring.worldSetup.dungeonCount = productBuiltinDungeonCatalog().size();
  const std::size_t dungeonIndex =
      productBuiltinDungeonIndexForRoomId(draft.asciiRoomId);
  window.creativeAuthoring.worldSetup.dungeonIndex = 0;
  // branch-gate: BG-1137
  if (dungeonIndex < productBuiltinDungeonCatalog().size()) {
    window.creativeAuthoring.worldSetup.dungeonIndex =
        static_cast<std::uint64_t>(dungeonIndex + 1U);
  }
  window.creativeAuthoring.worldSetup.asciiRoomEnabled = draft.asciiRoomEnabled;
  window.creativeAuthoring.worldSetup.asciiRoomTextPresent = !draft.asciiRoomText.empty();
  window.creativeAuthoring.worldSetup.asciiRoomId =
      draft.asciiRoomId.empty() ? "none" : draft.asciiRoomId;
  window.creativeAuthoring.worldSetup.asciiRoomSourceName =
      draft.asciiRoomSourceName.empty() ? "none" : draft.asciiRoomSourceName;
  const WorldSetupRouteResult setup =
      routeWorldSetupAction(draft, FrontendAction::CreateAndEnter);
  if (!setup.accepted || !setup.createRequested) {
    window.creativeAuthoring.worldSetup.status = std::string(setup.reasonCode);
    window.creativeAuthoring.worldCreation.status = std::string(setup.status);
    window.creativeAuthoring.worldCreation.reasonCode = std::string(setup.reasonCode);
    return {};
  }
  window.creativeAuthoring.worldSetup.title = setup.createRequest.worldName;
  window.creativeAuthoring.worldSetup.status = std::string(setup.status);

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
  window.creativeAuthoring.worldCreation.status = std::string(creation.status);
  window.creativeAuthoring.worldCreation.reasonCode = std::string(creation.reasonCode);
  window.creativeAuthoring.worldCreation.worldId =
      creation.request.worldId.empty() ? "none" : creation.request.worldId;
  window.creativeAuthoring.worldCreation.worldTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.creativeAuthoring.worldCreation.asciiRoomRequested = creation.request.asciiRoomRequested;
  window.creativeAuthoring.worldCreation.asciiRoomId =
      creation.request.asciiRoomId.empty() ? "none" : creation.request.asciiRoomId;
  window.creativeAuthoring.worldCreation.asciiRoomSourceName =
      creation.request.asciiRoomSourceName.empty()
          ? "none"
          : creation.request.asciiRoomSourceName;
  window.creativeAuthoring.worldCreation.initialSaveRequested = creation.initialSavePlan.requested;
  window.creativeAuthoring.worldCreation.initialSaveWritten = creation.initialSaveWritten;
  window.creativeAuthoring.worldCreation.initialSaveId =
      creation.initialSavePlan.saveId.empty() ? "none"
                                              : creation.initialSavePlan.saveId;
  window.creativeAuthoring.worldCreation.initialSaveTitle =
      creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : creation.initialSavePlan.worldTitle;
  window.creativeAuthoring.worldCreation.routeAfterCreate = std::string(creation.routeAfterCreate);
  return creation;
}

void recordProductWorldInitialSaveResult(
    const ProductWorldInitialSaveResult& initialSave,
    ProductAppWindowState& window) {
  window.creativeAuthoring.worldCreation.status = initialSave.status;
  window.creativeAuthoring.worldCreation.reasonCode = initialSave.reasonCode;
  window.creativeAuthoring.worldCreation.worldId = initialSave.creation.request.worldId.empty()
                                    ? "none"
                                    : initialSave.creation.request.worldId;
  window.creativeAuthoring.worldCreation.worldTitle =
      initialSave.creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.worldTitle;
  window.creativeAuthoring.worldCreation.asciiRoomRequested =
      initialSave.creation.request.asciiRoomRequested;
  window.creativeAuthoring.worldCreation.asciiRoomId =
      initialSave.creation.request.asciiRoomId.empty()
          ? "none"
          : initialSave.creation.request.asciiRoomId;
  window.creativeAuthoring.worldCreation.asciiRoomSourceName =
      initialSave.creation.request.asciiRoomSourceName.empty()
          ? "none"
          : initialSave.creation.request.asciiRoomSourceName;
  window.creativeAuthoring.worldCreation.initialSaveRequested =
      initialSave.creation.initialSavePlan.requested;
  window.creativeAuthoring.worldCreation.initialSaveWritten =
      initialSave.creation.initialSaveWritten;
  window.creativeAuthoring.worldCreation.initialSaveId =
      initialSave.creation.initialSavePlan.saveId.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.saveId;
  window.creativeAuthoring.worldCreation.initialSaveTitle =
      initialSave.creation.initialSavePlan.worldTitle.empty()
          ? "none"
          : initialSave.creation.initialSavePlan.worldTitle;
  window.creativeAuthoring.worldCreation.routeAfterCreate =
      std::string(initialSave.creation.routeAfterCreate);
  window.saveSession.productSaveStatus = initialSave.saveWrite.status;
  window.saveSession.productSaveReasonCode = initialSave.saveWrite.reasonCode;
  window.saveSession.productSaveDurableReason = initialSave.saveWrite.durableReason;
  window.saveSession.productSaveSource = "initial_world";
  window.saveSession.productSaveSaveId = initialSave.saveWrite.record.id.empty()
                                 ? "none"
                                 : initialSave.saveWrite.record.id;
  window.saveSession.productSaveSessionSaved = initialSave.saveWrite.ok;
  if (initialSave.saveWrite.ok && !initialSave.saveWrite.record.id.empty()) {
    window.saveSession.activeProductSaveId = initialSave.saveWrite.record.id;
  }
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

void clearProductGameplayLaunchStateImpl(std::optional<Session>& activeSession,
                                         ProductAppWindowState& window) {
  window.gameplay.gameplayActive = false;
  window.gameplay.runtimeSessionCreated = false;
  activeRoom(window) = {};
  activeRoomCollision(window) = {};
  bumpActiveRoomRevision(window);
  activeSession.reset();
}

void recordProductSaveWriteResult(std::string_view source,
                                  const ProductSaveWriteResult& written,
                                  ProductAppWindowState& window) {
  window.saveSession.productSaveStatus = written.status;
  window.saveSession.productSaveReasonCode = written.reasonCode;
  window.saveSession.productSaveDurableReason = written.durableReason;
  window.saveSession.productSaveSource = std::string(source);
  window.saveSession.productSaveSaveId = written.record.id.empty() ? "none" : written.record.id;
  window.saveSession.productSaveSessionSaved = written.ok;
  if (written.ok && !written.record.id.empty()) {
    window.saveSession.activeProductSaveId = written.record.id;
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

}  // namespace

bool createCreativeBlankSession(std::optional<Session>& activeSession,
                                ProductAppWindowState& window) {
  return createCreativeBlankSessionImpl(activeSession, window);
}

void frameCreativeStageCameraOnOrigin(ProductAppWindowState& window) {
  frameCreativeStageCameraOnOriginImpl(window);
}

void clearProductGameplayLaunchState(std::optional<Session>& activeSession,
                                     ProductAppWindowState& window) {
  clearProductGameplayLaunchStateImpl(activeSession, window);
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
  request.saveIdHint = window.saveSession.activeProductSaveId == "none"
                           ? std::string{}
                           : window.saveSession.activeProductSaveId;
  request.attemptToken = "attempt_002";
  request.state = &activeSession->state();
  if (activeRoom(window).hasAuthoredRoom) {
    request.authoredRoom = &activeRoom(window).authoredRoom;
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

void launchProductNewWorld(const ProductAppOptions& options,
                           const WorldSetupDraft& worldSetupDraft,
                           FrontendState& frontend,
                           std::optional<Session>& activeSession,
                           ProductAppWindowState& window) {
  window.frontendShell.launchAction = "create_and_enter";
  const ProductWorldTemplate world = productWorldTemplateFromOptions(options);
  ProductWorldCreationResult creation =
      prepareProductWorldCreationFromDraft(options, world, worldSetupDraft, window);
  if (!creation.accepted) {
    window.frontendShell.launchStatus = std::string(creation.reasonCode);
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
      window.frontendShell.launchStatus = asciiRoom.reasonCode;
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
    activeRoom(window) =
        buildProductActiveRoomFromAsciiAuthoring(asciiRequest, asciiRoom);
    bumpActiveRoomRevision(window);
    (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
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
    window.frontendShell.launchStatus = initialSave.reasonCode;
    clearProductGameplayLaunchState(activeSession, window);
    frontend.status = "opening_menu_new_world_failed";
    return;
  }

  window.frontendShell.launchStatus = initialSave.status;
  window.inputDevice.interactionMode = ProductInteractionMode::Player;
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
