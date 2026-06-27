#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "app/iggy3d/ProductAppOptions.hpp"
#include "app/iggy3d/gameplay/ProductGameplayTape.hpp"
#include "app/iggy3d/ReceiptBuilder.hpp"
#include "runtime/session/SessionState.hpp"

namespace iggy3d {

struct ProductActiveRoomCollisionState;
struct ProductActiveRoomState;
class Session;
class SpatialSurfaceSet;

struct ProductGameplayTapeRunRequest {
  Session* session = nullptr;
  const ProductGameplayTape* tape = nullptr;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  const ProductActiveRoomState* activeRoom = nullptr;
  ProductActiveRoomCollisionState* activeRoomCollision = nullptr;
};

struct ProductGameplayTapeRunResult {
  bool ok = false;
  std::string status = "not_requested";
  std::string reasonCode = "not_requested";
  std::uint64_t stepCount = 0;
  std::uint64_t executedStepCount = 0;
  std::uint64_t expectedRejectedStepCount = 0;
  std::uint64_t expectedBlockedStepCount = 0;
  std::uint64_t failedStepIndex = 0;
  std::uint64_t failedSourceLine = 0;
  std::string failedAction = "none";
  std::string failedTarget = "none";
  std::string failedRejection = "none";
  std::string failedMovementBlock = "none";
  std::string lastAction = "none";
  std::string lastTarget = "none";
  std::string lastMovementBlock = "none";
  bool keyCollected = false;
  bool secretDoorOpened = false;
  bool treasureCollected = false;
  bool npcTargetable = false;
  bool npcDefeated = false;
  bool exitObjectiveComplete = false;
  bool loopComplete = false;
  bool aiCommandLogged = false;
  bool aiAttackLogged = false;
  bool aiWaitLogged = false;
  bool aiPlayerDamaged = false;
  std::int32_t aiPlayerHpBefore = 0;
  std::int32_t aiPlayerHpAfter = 0;
  std::string aiActorId = "none";
  std::string aiTargetId = "none";
  std::string aiBehavior = "none";
  std::string aiIntent = "none";
  std::string sessionOutcome = "None";
  std::uint64_t runtimeStateHash = 0;
};

struct ProductGameplayTapeOptionsRunRequest {
  const ProductAppOptions& options;
  std::optional<Session>& activeSession;
  ProductAppWindowState& window;
};

std::string_view productGameplayTapeSessionOutcomeName(SessionOutcome outcome);

ProductGameplayTapeRunResult runProductGameplayTape(
    const ProductGameplayTapeRunRequest& request);

void runProductGameplayTapeFromOptions(
    const ProductGameplayTapeOptionsRunRequest& request);

}  // namespace iggy3d
