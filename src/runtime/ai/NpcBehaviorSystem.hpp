#pragma once

#include <cstdint>
#include <string_view>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/AiState.hpp"
#include "runtime/command/Command.hpp"

namespace iggy3d {

struct CombatState;
class WorldState;

enum class NpcEngagementPolicy : std::uint8_t {
  Hostile,
  Passive,
};

std::string_view npcEngagementPolicyName(NpcEngagementPolicy policy);

struct NpcBehaviorConfig {
  NpcEngagementPolicy engagementPolicy = NpcEngagementPolicy::Hostile;
  float perceptionRadiusMeters = 6.0F;
  float chaseStopDistanceMeters = 1.25F;
  float attackRangeMeters = 1.5F;
  float chaseStepMeters = 1.0F;
  // Half-angle of the horizontal vision cone. The target is only perceived
  // when it lies within +/- this angle of the actor's facing direction.
  float visionHalfAngleDegrees = 60.0F;
  // Half-angle of the vertical vision cone used by the 3D perception path.
  float verticalHalfAngleDegrees = 30.0F;
  float guardEyeHeightMeters = 1.6F;
  float targetStandEyeHeightMeters = 1.6F;
  float targetSneakEyeHeightMeters = 0.9F;
  float occlusionMarginMeters = 0.05F;
  std::int32_t attackDamage = 1;
  std::uint32_t decisionIntervalTicks = 1;
  std::uint32_t attackCooldownTicks = 2;
};

enum class NpcPerceptionStatus : std::uint8_t {
  Ready,
  InvalidWorld,
  InvalidCombat,
  InvalidActor,
  InvalidTarget,
  InvalidConfig,
  ActorInactive,
  TargetInactive,
  ActorDefeated,
  TargetDefeated,
  TargetOutOfRange,
  TargetOutOfCone,
  TargetOccluded,
};

std::string_view npcPerceptionStatusName(NpcPerceptionStatus status);
bool isValidNpcBehaviorConfig(const NpcBehaviorConfig& config);

struct NpcPerceptionRequest {
  const WorldState* world = nullptr;
  const CombatState* combat = nullptr;
  EntityId actor;
  EntityId target;
  NpcBehaviorConfig config;
  // Actor's horizontal gaze direction. A zero/degenerate vector disables the
  // vision-cone gate (omnidirectional perception) so low-level callers that
  // don't model facing keep the pre-cone behavior.
  Vec3 actorFacingDirection;
  // Whether the actor has an unobstructed line of sight to the target. The
  // caller resolves this (e.g. via a physics raycast against world colliders);
  // the default true means "assume clear" when no occlusion test is available.
  bool targetHasLineOfSight = true;
};

struct NpcPerceptionResult {
  NpcPerceptionStatus status = NpcPerceptionStatus::InvalidWorld;
  EntityId actor;
  EntityId target;
  Vec3 actorPosition;
  Vec3 targetPosition;
  float distanceMeters = 0.0F;
  bool actorActive = false;
  bool targetActive = false;
  bool actorDefeated = false;
  bool targetDefeated = false;
  bool targetInPerceptionRadius = false;
  bool targetInAttackRange = false;
  bool targetInVisionCone = false;
  bool hasLineOfSight = false;
  float horizontalAngleDeg = 0.0F;
  float verticalAngleDeg = 0.0F;
  bool inVerticalCone = false;
  bool perceived = false;
  enum class Los : std::uint8_t { Clear, Blocked, Unknown };
  Los los = Los::Unknown;
};

NpcPerceptionResult queryNpcPerception(const NpcPerceptionRequest& request);

enum class NpcBehaviorDecisionStatus : std::uint8_t {
  Decided,
  Disabled,
  WaitingForDecisionTick,
  ActorDefeated,
  NoTarget,
  OnCooldown,
  InvalidConfig,
  InvalidGuard,
  InvalidPerception,
};

std::string_view npcBehaviorDecisionStatusName(NpcBehaviorDecisionStatus status);

struct NpcBehaviorDecisionRequest {
  const AiActorState* actorState = nullptr;
  NpcPerceptionResult perception;
  NpcBehaviorConfig config;
  std::uint64_t currentTick = 0;
};

struct NpcBehaviorDecision {
  NpcBehaviorDecisionStatus status = NpcBehaviorDecisionStatus::InvalidPerception;
  EntityId actor;
  AiBehaviorKind behavior = AiBehaviorKind::Idle;
  AiIntentKind intent = AiIntentKind::None;
  EntityId target;
  Vec3 homePosition;
  float returnStopDistanceMeters = 0.0F;
  std::uint64_t nextDecisionTick = 0;
  std::uint32_t cooldownTicksRemaining = 0;
};

NpcBehaviorDecision chooseNpcBehaviorIntent(const NpcBehaviorDecisionRequest& request);

enum class NpcBehaviorCommandStatus : std::uint8_t {
  Built,
  NoCommand,
  InvalidConfig,
  InvalidDecision,
  InvalidPerception,
  InvalidDestination,
};

std::string_view npcBehaviorCommandStatusName(NpcBehaviorCommandStatus status);

struct NpcBehaviorCommandRequest {
  NpcBehaviorDecision decision;
  NpcPerceptionResult perception;
  NpcBehaviorConfig config;
};

struct NpcBehaviorCommandResult {
  NpcBehaviorCommandStatus status = NpcBehaviorCommandStatus::NoCommand;
  bool hasCommand = false;
  CommandRecord command;
};

NpcBehaviorCommandResult buildNpcBehaviorCommand(
    const NpcBehaviorCommandRequest& request);

}  // namespace iggy3d
