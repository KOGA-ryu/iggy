#include "app/iggy3d/ascii_room/Activation.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "app/iggy3d/gameplay/ActiveRoomState.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollision.hpp"
#include "app/iggy3d/gameplay/ActiveRoomCollisionFreshnessStore.hpp"
#include "app/iggy3d/gameplay/ProductRoomStore.hpp"
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
  window.asciiRoomActivation.status = result.status;
  window.asciiRoomActivation.reasonCode = result.reasonCode;
  window.asciiRoomActivation.roomId = result.roomId;
  window.asciiRoomActivation.packageId = result.packageId;
  window.asciiRoomActivation.scenarioId = result.scenarioId;
  window.asciiRoomActivation.sessionCreated = result.sessionCreated;
  window.asciiRoomActivation.playerSpawned = result.playerSpawned;
  window.asciiRoomActivation.playerCount = sizeReceiptValue(result.playerCount);
  window.asciiRoomActivation.entityCount = sizeReceiptValue(result.entityCount);
  window.asciiRoomActivation.npcCount = sizeReceiptValue(result.npcCount);
  window.asciiRoomActivation.pickupCount = sizeReceiptValue(result.pickupCount);
  window.asciiRoomActivation.doorCount = sizeReceiptValue(result.doorCount);
  window.asciiRoomActivation.markerEntityCount =
      sizeReceiptValue(result.markerEntityCount);
  window.asciiRoomActivation.objectiveCount =
      sizeReceiptValue(result.objectiveCount);
  window.asciiRoomActivation.wallCount = sizeReceiptValue(result.wallCount);
  window.asciiRoomActivation.markerCount = sizeReceiptValue(result.markerCount);
  window.asciiRoomActivation.runtimeHash = result.runtimeStateHash;
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
  result.roomId = window.asciiRoomDraft.roomId.empty() ? "none" : window.asciiRoomDraft.roomId;
  result.packageId = std::string(kAsciiRoomPackageId);
  result.scenarioId = productAsciiRoomScenarioIdForRoom(window.asciiRoomDraft.roomId);

  const ProductAsciiRoomAuthoringRequest request =
      productAsciiRoomAuthoringRequestFromDraft(window);
  const ProductAsciiRoomAuthoringResult preview =
      buildProductAsciiRoomAuthoring(request);
  recordProductAsciiRoomPreview(request.sourceName, request.roomId, preview, window);
  activeRoom(window) = buildProductActiveRoomFromAsciiAuthoring(request, preview);
  bumpActiveRoomRevision(window);
  (void)ensureActiveRoomCollisionFresh(window, nullptr);
  result.wallCount = preview.wallCount;
  result.markerCount = preview.markerCount;
  if (!preview.ok) {
    result = failedActivation(result, preview.status, preview.reasonCode);
    recordActivationResult(result, window);
    return result;
  }

  const PackageLoadResult package =
      makeProductAsciiRoomPackage(activeRoom(window).room,
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
  bumpActiveRoomRevision(window);
  (void)ensureActiveRoomCollisionFresh(window, &*activeSession);
  result.ok = true;
  result.status = "ascii_room_activated";
  result.reasonCode = "ascii_room_activated";
  result.sessionCreated = true;
  result.runtimeStateHash = activeSession->stateHash();

  window.gameplay.runtimeSessionCreated = true;
  window.gameplay.gameplayActive = true;
  window.launchAction = "ascii_room_activate";
  window.launchStatus = result.status;
  window.packageLoadStatus = "ascii_room_authoring";
  window.runtimeStateHash = result.runtimeStateHash;
  recordActivationResult(result, window);
  return result;
}

}  // namespace iggy3d
