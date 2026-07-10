#pragma once

#include <string>
#include <utility>

#include "core/math/Vec3.hpp"
#include "runtime/session/Session.hpp"

namespace iggy3d {

struct PhysicsSpatialSurfaceColliderBakeResult;

namespace session_detail {

inline ErrorInfo error(std::string code, std::string message) {
  return {std::move(code), std::move(message)};
}

inline StatusResult statusError(std::string code, std::string message) {
  return {ResultStatus::Error, error(std::move(code), std::move(message))};
}

inline StatusResult statusOk() {
  return {ResultStatus::Ok, {}};
}

void clearTransient(SessionState& state);
Vec3 initialNpcFacing(const WorldState& world, Vec3 actorPosition);
SessionCommandResult appendCommandThroughAdmission(Session& session,
                                                   SessionState& state,
                                                   const CommandRecord& command);
void enqueueNpcBehaviorCommands(
    Session& session,
    SessionState& state,
    const PhysicsSpatialSurfaceColliderBakeResult& occlusionBake);

}  // namespace session_detail

}  // namespace iggy3d
