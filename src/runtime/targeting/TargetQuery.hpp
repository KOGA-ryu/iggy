#pragma once

#include <cstdint>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/command/Command.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

class WorldState;

enum class TargetQueryStatus : std::uint8_t {
  Found,
  NotFound,
  InvalidWorld,
  InvalidActor,
  InvalidOrigin,
};

struct TargetQueryRequest {
  const WorldState* world = nullptr;
  EntityId actor;
  bool hasOrigin = false;
  Vec3 origin;
  CommandKind commandKind = CommandKind::None;
  float maxDistanceMeters = 0.0F;
  bool allowSelf = false;
  bool requireActive = true;
};

struct TargetQueryResult {
  TargetQueryStatus status = TargetQueryStatus::NotFound;
  EntityId target;
  Vec3 origin;
  Vec3 targetPoint;
  float distanceMeters = 0.0F;
  bool targetActive = false;
  bool targetSupportsCommand = false;
};

TargetQueryResult queryTarget(const TargetQueryRequest& request);
bool targetSupportsCommandKind(const EntityState& entity, CommandKind kind);

}  // namespace iggy3d
