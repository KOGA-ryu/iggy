#include "app/iggy3d/world/Launch.hpp"

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductAppWindowState.hpp"
#include "app/iggy3d/ascii_room/Authoring.hpp"
#include "app/iggy3d/ascii_room/Package.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
#include "app/iggy3d/input/InteractionMode.hpp"
#include "app/iggy3d/menu/Transitions.hpp"
#include "app/iggy3d/world/BuiltinDungeon.hpp"
#include "app/iggy3d/world/Creation.hpp"
#include "app/frontend/FrontendState.hpp"
#include "app/frontend/WorldSetupModel.hpp"
#include "app/iggy3d/Options.hpp"
#include "app/iggy3d/world/WorldTemplate.hpp"
#include "content/PackageLoader.hpp"

namespace iggy3d {

namespace {

ProductAsciiRoomAuthoringRequest productAsciiRoomAuthoringRequestFromWorldSetup(
    const WorldSetupDraft& draft) {
  return productWorldSetupAuthoringRequest(draft);
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

}  // namespace

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
