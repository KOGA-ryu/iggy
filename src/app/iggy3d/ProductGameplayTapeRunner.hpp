#pragma once

#include <cstdint>
#include <string>

#include "app/iggy3d/ProductGameplayTape.hpp"
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
  bool exitObjectiveComplete = false;
  bool loopComplete = false;
  std::string sessionOutcome = "None";
  std::uint64_t runtimeStateHash = 0;
};

std::string_view productGameplayTapeSessionOutcomeName(SessionOutcome outcome);

ProductGameplayTapeRunResult runProductGameplayTape(
    const ProductGameplayTapeRunRequest& request);

}  // namespace iggy3d
