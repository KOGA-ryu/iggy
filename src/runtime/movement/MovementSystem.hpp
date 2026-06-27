#pragma once

#include "config/RuntimeConfig.hpp"
#include "runtime/collision/SpatialSurfaceSet.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/movement/MovementCommand.hpp"
#include "runtime/movement/MovementPolicy.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

struct MovementSystemContext {
  WorldState* world = nullptr;
  const RuntimeConfig* config = nullptr;
  const SpatialSurfaceSet* collisionSurfaces = nullptr;
  bool usePhysicsMovePlanner = false;
};

MovementResult executeMovement(MovementSystemContext& context, const MovementRequest& request);
MovementResult executeKinematicMovement(MovementSystemContext& context,
                                        const KinematicMovementRequest& request);

MovementRequest movementRequestFromAcceptedCommand(
    const CommandRecord& command,
    MovementMode mode,
    const RuntimeConfig& config);

MovementBlockedReason validateMovementRequest(
    const MovementSystemContext& context,
    const MovementRequest& request);

float movementDistanceMeters(const Vec3& start, const Vec3& destination);
float movementLimitMeters(const MovementRequest& request, const RuntimeConfig& config);
const char* movementBlockedReasonName(MovementBlockedReason reason);

}  // namespace iggy3d
