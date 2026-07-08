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
#include "app/iggy3d/world/ProductSessionLaunch.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/view/CreativeFlyAnchorStore.hpp"
#include "app/iggy3d/world/Creation.hpp"
#include "content/PackageLoader.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "render/RenderDiagnostics.hpp"

namespace iggy3d {

namespace {

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

}  // namespace iggy3d
