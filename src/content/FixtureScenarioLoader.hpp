#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config/MovementDimensionProfile.hpp"
#include "config/RuntimeConfig.hpp"
#include "core/diagnostics/Diagnostic.hpp"
#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/camera/CameraState.hpp"
#include "runtime/clock/ClockState.hpp"
#include "runtime/combat/CombatState.hpp"
#include "runtime/player/PlayerSlot.hpp"
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
  PlayerSlotId slot = kInvalidPlayerSlotId;
  PlayerSlotKind kind = PlayerSlotKind::Unknown;
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
  PlayerSlotId playerSlot = kInvalidPlayerSlotId;
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
  PatrolMode patrolMode = PatrolMode::Loop;
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
  // Selected movement dimension profile (MA1). Default earth_standard; the loader resolves the
  // scenario's `movement_profile` key against the static rows and applies precedence (an explicit
  // movement_distance_meters overrides the profile's limit).
  MovementDimensionProfile movementProfile;
  ClockMode initialClockMode = ClockMode::Normal;
  CameraMode defaultRealtimeCamera = CameraMode::ThirdPerson;
  CameraMode defaultTacticalCamera = CameraMode::TacticalOverhead;
  std::vector<ScenarioPlayerSeed> players;
  std::vector<ScenarioEntitySeed> entities;
  std::vector<ScenarioObjectiveSeed> objectives;
  std::vector<ScenarioAiActorSeed> aiActors;
  std::vector<ScenarioAiGuardAnchorSeed> aiGuardAnchors;
  // Scenario-authored behavior profiles (A9). Added on top of the three built-ins by
  // buildNpcBehaviorProfileCatalog; empty => the catalog is byte-identical to the built-ins.
  std::vector<NpcBehaviorProfile> behaviorProfiles;
};

struct ScenarioLoadResult {
  ScenarioLoadStatus status = ScenarioLoadStatus::Ok;
  FixtureScenarioSeed seed;
  std::vector<Diagnostic> diagnostics;
};

ScenarioLoadResult parseScenarioText(const std::string& scenarioText);

}  // namespace iggy3d
