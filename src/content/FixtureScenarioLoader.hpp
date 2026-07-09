#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/RuntimeConfig.hpp"
#include "core/diagnostics/Diagnostic.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "content/ScenarioSeed.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/world/EntityState.hpp"

namespace iggy3d {

enum class ScenarioLoadStatus : std::uint8_t {
  Ok,
  ParseError,
  UnsupportedKey,
  MissingRequiredKey,
  MissingScenarioId,
  InvalidNumber,
  InvalidEnum,
  InvalidPath,
};

struct ScenarioPlayerSeed {
  ScenarioPlayerSlotId slot = kInvalidScenarioPlayerSlotId;
  ScenarioPlayerSlotKind kind = ScenarioPlayerSlotKind::Unknown;
  std::string actorStableName;
};

struct ScenarioEntitySeed {
  std::string stableName;
  EntityKind kind = EntityKind::Unknown;
  Transform3 transform;
  Aabb3 localBounds;
  bool active = true;
  bool persistent = true;
  EntityTargeting targeting;
  InteractionDefinition interaction;
  bool combatantEnabled = false;
  CombatantState combatant;
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
  // Optional authored spawn facing (yaw degrees; 0 = +Z, clockwise from above
  // so 90 = +X). When unset the NPC faces the player at spawn.
  bool hasFacing = false;
  float facingDegrees = 0.0F;
  // Optional authored patrol route (slice 6). Repeated `waypoint` keys append in order;
  // an empty route means the NPC does not patrol (back-compat). Default member
  // initializers keep positional aggregate-init sites warning-free.
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

struct ScenarioLoadResult {
  ScenarioLoadStatus status = ScenarioLoadStatus::Ok;
  FixtureScenarioSeed seed;
  std::vector<Diagnostic> diagnostics;
};

ScenarioLoadResult parseScenarioText(const std::string& scenarioText);

}  // namespace iggy3d
