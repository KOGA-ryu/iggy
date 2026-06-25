#include "runtime/ai/NpcBehaviorProfile.hpp"

#include <cctype>

namespace iggy3d {

std::string_view npcBehaviorProfileResolveStatusName(
    NpcBehaviorProfileResolveStatus status) {
  switch (status) {
    case NpcBehaviorProfileResolveStatus::Resolved:
      return "profile_resolved";
    case NpcBehaviorProfileResolveStatus::Missing:
      return "profile_missing";
    case NpcBehaviorProfileResolveStatus::InvalidId:
      return "profile_invalid_id";
    case NpcBehaviorProfileResolveStatus::InvalidConfig:
      return "profile_invalid_config";
    case NpcBehaviorProfileResolveStatus::CatalogEmpty:
      return "profile_catalog_empty";
  }
  return "profile_invalid_id";
}

bool isValidNpcBehaviorProfileId(std::string_view profileId) {
  if (profileId.empty()) {
    return false;
  }
  for (const char c : profileId) {
    const auto uc = static_cast<unsigned char>(c);
    if (!(std::islower(uc) || std::isdigit(uc) || c == '_')) {
      return false;
    }
  }
  return true;
}

NpcBehaviorConfig configFromNpcBehaviorProfile(const NpcBehaviorProfile& profile) {
  NpcBehaviorConfig config;
  config.engagementPolicy = profile.engagementPolicy;
  config.perceptionRadiusMeters = profile.perceptionRadiusMeters;
  config.chaseStopDistanceMeters = profile.chaseStopDistanceMeters;
  config.attackRangeMeters = profile.attackRangeMeters;
  config.chaseStepMeters = profile.chaseStepMeters;
  config.attackDamage = profile.attackDamage;
  config.decisionIntervalTicks = profile.decisionIntervalTicks;
  config.attackCooldownTicks = profile.attackCooldownTicks;
  return config;
}

NpcBehaviorProfileCatalog makeBuiltInNpcBehaviorProfileCatalog() {
  NpcBehaviorProfileCatalog catalog;

  NpcBehaviorProfile defaultProfile;
  defaultProfile.id.value = "default";
  catalog.profiles.push_back(defaultProfile);

  NpcBehaviorProfile meleeTraining;
  meleeTraining.id.value = "melee_training";
  catalog.profiles.push_back(meleeTraining);

  NpcBehaviorProfile passive;
  passive.id.value = "passive";
  passive.engagementPolicy = NpcEngagementPolicy::Passive;
  catalog.profiles.push_back(passive);

  return catalog;
}

NpcBehaviorProfileResolveResult resolveNpcBehaviorProfile(
    const NpcBehaviorProfileResolveRequest& request) {
  NpcBehaviorProfileResolveResult result;

  if (!isValidNpcBehaviorProfileId(request.profileId)) {
    result.status = NpcBehaviorProfileResolveStatus::InvalidId;
    result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
    return result;
  }
  if (request.catalog == nullptr || request.catalog->profiles.empty()) {
    result.status = NpcBehaviorProfileResolveStatus::CatalogEmpty;
    result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
    return result;
  }

  for (const NpcBehaviorProfile& profile : request.catalog->profiles) {
    if (profile.id.value != request.profileId) {
      continue;
    }
    result.profile = profile;
    result.config = configFromNpcBehaviorProfile(profile);
    if (!isValidNpcBehaviorConfig(result.config)) {
      result.status = NpcBehaviorProfileResolveStatus::InvalidConfig;
      result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
      return result;
    }
    result.ok = true;
    result.status = NpcBehaviorProfileResolveStatus::Resolved;
    result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
    return result;
  }

  result.status = NpcBehaviorProfileResolveStatus::Missing;
  result.reasonCode = npcBehaviorProfileResolveStatusName(result.status);
  return result;
}

}  // namespace iggy3d
