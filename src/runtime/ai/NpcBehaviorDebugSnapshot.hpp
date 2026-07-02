#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/ai/NpcBehaviorProfile.hpp"
#include "runtime/command/Command.hpp"

namespace iggy3d {

struct CombatState;
class WorldState;

enum class NpcBehaviorDebugSnapshotStatus : std::uint8_t {
  Ok,
  Disabled,
  MissingWorld,
  MissingAi,
  MissingCombat,
  MissingProfileCatalog,
};

std::string_view npcBehaviorDebugSnapshotStatusName(
    NpcBehaviorDebugSnapshotStatus status);

struct NpcBehaviorDebugActorRow {
  EntityId actor;
  std::string stableName = "none";
  bool active = false;
  bool isNpc = false;
  bool hasAiState = false;
  bool hasCombatant = false;
  bool combatantDefeated = false;
  std::string behaviorProfileId = "default";
  bool profileResolved = false;
  std::string profileStatus = "profile_missing";
  NpcEngagementPolicy engagementPolicy = NpcEngagementPolicy::Hostile;
  AiBehaviorKind behavior = AiBehaviorKind::None;
  AiIntentKind lastIntent = AiIntentKind::None;
  EntityId target;
  std::string targetStableName = "none";
  bool targetResolved = false;
  bool targetActive = false;
  bool targetCombatantDefeated = false;
  float targetDistanceMeters = 0.0F;
  bool targetInPerceptionRadius = false;
  bool targetInVisionCone = false;
  bool targetHasLineOfSight = false;
  std::uint32_t cooldownTicksRemaining = 0;
  CommandTick nextDecisionTick = 0;
};

struct NpcBehaviorDebugSnapshot {
  NpcBehaviorDebugSnapshotStatus status = NpcBehaviorDebugSnapshotStatus::Disabled;
  std::string_view reasonCode = "npc_behavior_debug_disabled";
  CommandTick sourceTick = kInvalidCommandTick;
  std::size_t npcWorldCount = 0;
  std::size_t aiActorCount = 0;
  std::size_t resolvedProfileCount = 0;
  std::size_t failedProfileCount = 0;
  std::size_t hostileCount = 0;
  std::size_t passiveCount = 0;
  std::size_t attackingCount = 0;
  std::size_t waitingCount = 0;
  std::vector<NpcBehaviorDebugActorRow> actors;
};

struct NpcBehaviorDebugSnapshotRequest {
  bool enabled = false;
  const WorldState* world = nullptr;
  const AiState* ai = nullptr;
  const CombatState* combat = nullptr;
  const NpcBehaviorProfileCatalog* profileCatalog = nullptr;
  CommandTick sourceTick = kInvalidCommandTick;
  std::size_t maxActors = 128;
};

NpcBehaviorDebugSnapshot buildNpcBehaviorDebugSnapshot(
    const NpcBehaviorDebugSnapshotRequest& request);

}  // namespace iggy3d
