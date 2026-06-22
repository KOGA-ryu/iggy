#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/world/WorldState.hpp"

namespace iggy3d {

enum class EntityHitStatus : std::uint8_t {
  Hit,
  NoHit,
  InvalidInput,
  MissingWorld,
};

struct EntityHitQueryRequest {
  const WorldState* world = nullptr;
  Vec3 startMeters;
  Vec3 endMeters;
  EntityId ignoredEntity;
  float radiusMeters = 0.0F;
  bool requireAttackTarget = true;
};

struct EntityHitQueryResult {
  EntityHitStatus status = EntityHitStatus::NoHit;
  EntityId entity;
  std::string stableName = "none";
  Aabb3 worldBounds;
  Vec3 pointMeters;
  Vec3 normal;
  float distanceMeters = 0.0F;
  float timeOfImpact = 0.0F;
  std::size_t checkedEntityCount = 0;
  std::size_t candidateEntityCount = 0;
  std::string reasonCode = "entity_hit_no_hit";
};

std::string_view entityHitStatusName(EntityHitStatus status);
EntityHitQueryResult queryFirstEntityHit(const EntityHitQueryRequest& request);

}  // namespace iggy3d
