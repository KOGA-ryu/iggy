#include "app/iggy3d/ascii_room/Activation.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/ascii_room/Package.hpp"
#include "app/iggy3d/ascii_room/Preview.hpp"
#include "app/iggy3d/world/PackageSessionSeed.hpp"

namespace iggy3d {
namespace {

constexpr std::string_view kAsciiRoomPackageId = "iggy3d.ascii_room_authoring";

std::uint64_t sizeReceiptValue(std::size_t value) {
  return static_cast<std::uint64_t>(value);
}

void recordActivationResult(const ProductAsciiRoomActivationResult& result,
                            ProductAppWindowState& window) {
  window.asciiRoomActivationStatus = result.status;
  window.asciiRoomActivationReasonCode = result.reasonCode;
  window.asciiRoomActivationRoomId = result.roomId;
  window.asciiRoomActivationPackageId = result.packageId;
  window.asciiRoomActivationScenarioId = result.scenarioId;
  window.asciiRoomActivationSessionCreated = result.sessionCreated;
  window.asciiRoomActivationPlayerSpawned = result.playerSpawned;
  window.asciiRoomActivationPlayerCount = sizeReceiptValue(result.playerCount);
  window.asciiRoomActivationEntityCount = sizeReceiptValue(result.entityCount);
  window.asciiRoomActivationNpcCount = sizeReceiptValue(result.npcCount);
  window.asciiRoomActivationPickupCount = sizeReceiptValue(result.pickupCount);
  window.asciiRoomActivationDoorCount = sizeReceiptValue(result.doorCount);
  window.asciiRoomActivationMarkerEntityCount =
      sizeReceiptValue(result.markerEntityCount);
  window.asciiRoomActivationObjectiveCount =
      sizeReceiptValue(result.objectiveCount);
  window.asciiRoomActivationWallCount = sizeReceiptValue(result.wallCount);
  window.asciiRoomActivationMarkerCount = sizeReceiptValue(result.markerCount);
  window.asciiRoomActivationRuntimeHash = result.runtimeStateHash;
}

ProductAsciiRoomActivationResult failedActivation(
    ProductAsciiRoomActivationResult result,
    std::string status,
    std::string reason) {
  result.ok = false;
  result.status = std::move(status);
  result.reasonCode = std::move(reason);
  return result;
}

}  // namespace

ProductAsciiRoomActivationResult activateProductAsciiRoomPreview(
    std::optional<Session>& activeSession,
    ProductAppWindowState& window) {
  ProductAsciiRoomActivationResult result;
  result.roomId = window.asciiRoomDraftRoomId.empty() ? "none" : window.asciiRoomDraftRoomId;
  result.packageId = std::string(kAsciiRoomPackageId);
  result.scenarioId = productAsciiRoomScenarioIdForRoom(window.asciiRoomDraftRoomId);

  const ProductAsciiRoomAuthoringRequest request =
      productAsciiRoomAuthoringRequestFromDraft(window);
  const ProductAsciiRoomAuthoringResult preview =
      buildProductAsciiRoomAuthoring(request);
  recordProductAsciiRoomPreview(request.sourceName, request.roomId, preview, window);
  window.activeRoom = buildProductActiveRoomFromAsciiAuthoring(request, preview);
  window.activeRoomCollision = buildProductActiveRoomCollision(window.activeRoom);
  result.wallCount = preview.wallCount;
  result.markerCount = preview.markerCount;
  if (!preview.ok) {
    result = failedActivation(result, preview.status, preview.reasonCode);
    recordActivationResult(result, window);
    return result;
  }

  const PackageLoadResult package =
      makeProductAsciiRoomPackage(window.activeRoom.room,
                                  result.packageId,
                                  result.scenarioId);

  const ProductPackageSessionSeedResult seed =
      buildProductPackageSessionSeed(package);
  result.playerCount = seed.playerCount;
  result.entityCount = seed.entityCount;
  result.npcCount = seed.npcCount;
  result.pickupCount = seed.pickupCount;
  result.doorCount = seed.doorCount;
  result.markerEntityCount = seed.markerEntityCount;
  result.objectiveCount = seed.objectiveCount;
  result.playerSpawned = seed.playerCount > 0;
  if (!seed.ok) {
    result = failedActivation(result, seed.status, seed.reasonCode);
    recordActivationResult(result, window);
    return result;
  }

  SessionCreateRequest create;
  create.packageId = package.manifest.packageId;
  create.seed = seed.seed;
  create.config = seed.seed.config;

  Result<Session> session = Session::create(create);
  if (session.status != ResultStatus::Ok) {
    const std::string reason =
        session.error.code.empty() ? "session_create_failed" : session.error.code;
    result = failedActivation(result, reason, reason);
    recordActivationResult(result, window);
    return result;
  }

  activeSession = std::move(session.value);
  // MA1: initialize the window's mutable tuning copy from the session's resolved dimension profile.
  applyMovementDimensionProfileToTuning(activeSession->state().movementProfile,
                                        window.gameplayMovementTuning);
  window.activeRoomCollision =
      buildProductActiveRoomCollision(window.activeRoom, activeSession->state());
  result.ok = true;
  result.status = "ascii_room_activated";
  result.reasonCode = "ascii_room_activated";
  result.sessionCreated = true;
  result.runtimeStateHash = activeSession->stateHash();

  window.runtimeSessionCreated = true;
  window.gameplayActive = true;
  window.launchAction = "ascii_room_activate";
  window.launchStatus = result.status;
  window.packageLoadStatus = "ascii_room_authoring";
  window.runtimeStateHash = result.runtimeStateHash;
  recordActivationResult(result, window);
  return result;
}

}  // namespace iggy3d
