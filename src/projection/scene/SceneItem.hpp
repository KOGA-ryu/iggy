#pragma once

#include <cstdint>
#include <string>

#include "core/ids/EntityId.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/interaction/InteractionDefinition.hpp"
#include "runtime/player/PlayerSlot.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

enum class SceneItemKind : std::uint8_t {
  Player,
  Pickup,
  Interactable,
  ObjectiveMarker,
  TacticalMarker,
  DebugOnly,
};

struct SceneItem {
  EntityId entityId;
  std::string stableName;
  SceneItemKind kind = SceneItemKind::DebugOnly;
  EntityKind entityKind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 worldBounds;
  bool active = false;
  bool visible = false;
  bool targetable = false;
  bool interactable = false;
  bool tactical = false;
  std::string assetRef;
  std::string itemId;
  std::string objectiveId;
  InteractionKind interactionKind = InteractionKind::None;
  PlayerSlotId owningPlayerSlot = kInvalidPlayerSlotId;
};

}  // namespace iggy3d
