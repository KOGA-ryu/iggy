#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/RuntimeConfig.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"

namespace iggy3d {

using ScenarioPlayerSlotId = std::uint32_t;
inline constexpr ScenarioPlayerSlotId kInvalidScenarioPlayerSlotId = UINT32_MAX;

enum class ScenarioPlayerSlotKind : std::uint8_t {
  Unknown,
  Local,
  Remote,
  Ai,
  Observer,
};

enum class ScenarioClockMode : std::uint8_t {
  Normal,
  Slow,
  Paused,
};

enum class ScenarioCameraMode : std::uint8_t {
  FirstPerson,
  ThirdPerson,
  TacticalOverhead,
};

enum class ScenarioPatrolMode : std::uint8_t {
  Loop,
  PingPong,
};

enum class ScenarioEntityKind : std::uint8_t {
  Unknown,
  Player,
  Pickup,
  Door,
  Marker,
  Npc,
};

enum class ScenarioTargetAction : std::uint8_t {
  Interact,
  Inspect,
  Attack,
  Move,
};

struct ScenarioTargetingSeed {
  bool targetable = false;
  std::vector<ScenarioTargetAction> actions;
};

inline bool isScenarioTargetActionSupported(const ScenarioTargetingSeed& targeting,
                                            ScenarioTargetAction action) {
  if (!targeting.targetable) {
    return false;
  }
  for (const ScenarioTargetAction candidate : targeting.actions) {
    if (candidate == action) {
      return true;
    }
  }
  return false;
}

enum class ScenarioInteractionKind : std::uint8_t {
  None,
  Pickup,
  Activate,
  OpenDoor,
  Inspect,
  ObjectiveTrigger,
};

enum class ScenarioInteractionEffectKind : std::uint8_t {
  None,
  AddItemToInventory,
  DeactivateTarget,
  CompleteObjective,
  EmitEventOnly,
};

struct ScenarioInteractionSeed {
  ScenarioInteractionKind kind = ScenarioInteractionKind::None;
  ScenarioInteractionEffectKind primaryEffect = ScenarioInteractionEffectKind::None;
  std::string itemId;
  std::uint32_t itemCount = 0;
  std::string objectiveId;
  std::string requiredItemId;
  std::uint32_t requiredItemCount = 0;
  bool repeatable = false;
  bool deactivateTargetOnSuccess = false;
};

struct ScenarioCombatantSeed {
  std::uint32_t factionId = 0;
  std::int32_t hitPoints = 0;
  std::int32_t maxHitPoints = 0;
};

struct ScenarioPlayerSeed {
  ScenarioPlayerSlotId slot = kInvalidScenarioPlayerSlotId;
  ScenarioPlayerSlotKind kind = ScenarioPlayerSlotKind::Unknown;
  std::string actorStableName;
};

struct ScenarioEntitySeed {
  std::string stableName;
  ScenarioEntityKind kind = ScenarioEntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  ScenarioTargetingSeed targeting;
  ScenarioInteractionSeed interaction;
  bool combatantEnabled = false;
  ScenarioCombatantSeed combatant;
};

enum class ObjectiveStatusSeed : std::uint8_t {
  Active,
  Complete,
  Failed,
};

struct ScenarioObjectiveSeed {
  std::string id;
  ObjectiveStatusSeed initialStatus = ObjectiveStatusSeed::Active;
  std::string condition = "InventoryContains";
  ScenarioPlayerSlotId playerSlot = kInvalidScenarioPlayerSlotId;
  std::string itemId;
  std::uint32_t itemCount = 0;
  ObjectiveStatusSeed completeStatus = ObjectiveStatusSeed::Complete;
};

struct ScenarioAiActorSeed {
  std::string actorStableName;
  std::string behaviorProfileId = "default";
  bool hasFacing = false;
  float facingDegrees = 0.0F;
  std::vector<Vec3> patrolWaypoints{};
  ScenarioPatrolMode patrolMode = ScenarioPatrolMode::Loop;
};

struct ScenarioAiGuardAnchorSeed {
  std::string actorStableName;
  std::string anchorStableName;
  float leashRadiusMeters = 0.0F;
  float returnRadiusMeters = 0.0F;
  float homeToleranceMeters = 0.0F;
};

struct FixtureScenarioSeed {
  std::string scenarioId;
  RuntimeConfig config;
  ScenarioClockMode initialClockMode = ScenarioClockMode::Normal;
  ScenarioCameraMode defaultRealtimeCamera = ScenarioCameraMode::ThirdPerson;
  ScenarioCameraMode defaultTacticalCamera = ScenarioCameraMode::TacticalOverhead;
  std::vector<ScenarioPlayerSeed> players;
  std::vector<ScenarioEntitySeed> entities;
  std::vector<ScenarioObjectiveSeed> objectives;
  std::vector<ScenarioAiActorSeed> aiActors;
  std::vector<ScenarioAiGuardAnchorSeed> aiGuardAnchors;
};

}  // namespace iggy3d
