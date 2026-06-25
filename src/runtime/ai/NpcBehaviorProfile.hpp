#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/ai/NpcBehaviorSystem.hpp"

namespace iggy3d {

struct NpcBehaviorProfileId {
  std::string value;
};

struct NpcBehaviorProfile {
  NpcBehaviorProfileId id;
  NpcEngagementPolicy engagementPolicy = NpcEngagementPolicy::Hostile;
  float perceptionRadiusMeters = 6.0F;
  float chaseStopDistanceMeters = 1.25F;
  float attackRangeMeters = 1.5F;
  float chaseStepMeters = 1.0F;
  std::int32_t attackDamage = 1;
  std::uint32_t decisionIntervalTicks = 1;
  std::uint32_t attackCooldownTicks = 2;
};

struct NpcBehaviorProfileCatalog {
  std::vector<NpcBehaviorProfile> profiles;
};

enum class NpcBehaviorProfileResolveStatus : std::uint8_t {
  Resolved,
  Missing,
  InvalidId,
  InvalidConfig,
  CatalogEmpty,
};

std::string_view npcBehaviorProfileResolveStatusName(
    NpcBehaviorProfileResolveStatus status);

struct NpcBehaviorProfileResolveRequest {
  const NpcBehaviorProfileCatalog* catalog = nullptr;
  std::string_view profileId;
};

struct NpcBehaviorProfileResolveResult {
  bool ok = false;
  NpcBehaviorProfileResolveStatus status = NpcBehaviorProfileResolveStatus::Missing;
  std::string_view reasonCode = "profile_missing";
  NpcBehaviorProfile profile;
  NpcBehaviorConfig config;
};

bool isValidNpcBehaviorProfileId(std::string_view profileId);
NpcBehaviorConfig configFromNpcBehaviorProfile(const NpcBehaviorProfile& profile);
NpcBehaviorProfileCatalog makeBuiltInNpcBehaviorProfileCatalog();
NpcBehaviorProfileResolveResult resolveNpcBehaviorProfile(
    const NpcBehaviorProfileResolveRequest& request);

}  // namespace iggy3d
