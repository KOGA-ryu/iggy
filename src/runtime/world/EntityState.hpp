#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"

namespace iggy3d {

enum class EntityKind : std::uint8_t {
  Unknown,
  Player,
  Pickup,
  Door,
  Marker,
  Npc,
};

enum class TargetAction : std::uint8_t {
  Interact,
  Inspect,
  Move,
};

struct EntityTargeting {
  bool targetable = false;
  std::vector<TargetAction> actions;
};

struct EntityState {
  EntityId id;
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  EntityTargeting targeting;
  InteractionDefinition interaction;
};

inline bool isTargetActionSupported(const EntityTargeting& targeting, TargetAction action) {
  if (!targeting.targetable) {
    return false;
  }
  for (TargetAction candidate : targeting.actions) {
    if (candidate == action) {
      return true;
    }
  }
  return false;
}

inline bool isValidEntityKind(EntityKind kind) {
  return kind == EntityKind::Player || kind == EntityKind::Pickup || kind == EntityKind::Door ||
         kind == EntityKind::Marker || kind == EntityKind::Npc;
}

inline bool hasValidEntityIdentity(const EntityState& entity) {
  return isValid(entity.id) && !entity.stableName.empty() && isValidEntityKind(entity.kind);
}

}  // namespace iggy3d
