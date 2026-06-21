#pragma once

#include "config/RuntimeConfig.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementCommand.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

struct MovementSystemContext {
  WorldState* world = nullptr;
  const RuntimeConfig* config = nullptr;
};

MovementResult executeMovement(MovementSystemContext& context, const MovementRequest& request);

MovementRequest movementRequestFromAcceptedCommand(
    const CommandRecord& command,
    MovementMode mode,
    const RuntimeConfig& config);

MovementBlockedReason validateMovementRequest(
    const MovementSystemContext& context,
    const MovementRequest& request);

float movementDistanceMeters(const Vec3& start, const Vec3& destination);
float movementLimitMeters(const MovementRequest& request, const RuntimeConfig& config);

}  // namespace iggy3d
