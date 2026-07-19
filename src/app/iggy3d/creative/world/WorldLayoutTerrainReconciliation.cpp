#include "app/iggy3d/creative/world/WorldLayoutTerrainReconciliation.hpp"

#include <algorithm>
#include <map>
#include <string_view>
#include <tuple>
#include <vector>

namespace iggy3d::creative {
namespace {

struct DesiredSource {
  CreativeWorldLayoutTable table = CreativeWorldLayoutTable::None;
  std::size_t index = kInvalidCreativeWorldLayoutIndex;
};

using DesiredSourceLookup = std::map<std::string, DesiredSource, std::less<>>;
using DecisionIdentity =
    std::tuple<CreativeWorldLayoutTable, std::size_t, std::string>;
using DecisionLookup = std::map<DecisionIdentity, std::size_t>;

[[nodiscard]] bool terrainTable(CreativeWorldLayoutTable table) noexcept {
  return table == CreativeWorldLayoutTable::TerrainProfile ||
         table == CreativeWorldLayoutTable::TerrainPath;
}

[[nodiscard]] bool appendDesiredSources(const CreativeWorldLayout& layout,
                                        DesiredSourceLookup& output) {
  const auto append = [&output](CreativeWorldLayoutTable table,
                                const auto& sources) {
    for (std::size_t index = 0U; index < sources.size(); ++index) {
      const std::string& key = sources[index].stableKey;
      if (!validCreativeWorldLayoutStableKey(key) ||
          !output.emplace(key, DesiredSource{table, index}).second) {
        return false;
      }
    }
    return true;
  };
  return append(CreativeWorldLayoutTable::TerrainProfile,
                layout.terrainProfiles) &&
         append(CreativeWorldLayoutTable::TerrainPath, layout.terrainPaths);
}

void reject(CreativeWorldLayoutTerrainReconciliationResult& result,
            CreativeWorldLayoutTerrainReconciliationStatus status,
            std::string_view reasonCode) {
  result.accepted = false;
  result.blocked = false;
  result.status = status;
  result.reasonCode = reasonCode;
}

}  // namespace

std::string_view toString(
    CreativeWorldLayoutTerrainConflictResolution resolution) noexcept {
  switch (resolution) {
    case CreativeWorldLayoutTerrainConflictResolution::Block:
      return "Block";
    case CreativeWorldLayoutTerrainConflictResolution::Regenerate:
      return "Regenerate";
    case CreativeWorldLayoutTerrainConflictResolution::Count:
      break;
  }
  return "Invalid";
}

std::string_view toString(
    CreativeWorldLayoutTerrainReconciliationStatus status) noexcept {
  switch (status) {
    case CreativeWorldLayoutTerrainReconciliationStatus::NotRequested:
      return "NotRequested";
    case CreativeWorldLayoutTerrainReconciliationStatus::InvalidRequest:
      return "InvalidRequest";
    case CreativeWorldLayoutTerrainReconciliationStatus::ImpactRejected:
      return "ImpactRejected";
    case CreativeWorldLayoutTerrainReconciliationStatus::DecisionInvalid:
      return "DecisionInvalid";
    case CreativeWorldLayoutTerrainReconciliationStatus::Conflict:
      return "Conflict";
    case CreativeWorldLayoutTerrainReconciliationStatus::Ready:
      return "Ready";
  }
  return "Invalid";
}

CreativeWorldLayoutTerrainReconciliationResult
reconcileCreativeWorldLayoutTerrain(
    const CreativeWorldLayoutTerrainReconciliationRequest& request) {
  CreativeWorldLayoutTerrainReconciliationResult result;
  result.requested = true;
  if (request.document == nullptr || request.generatedLayout == nullptr ||
      request.desiredLayout == nullptr || !request.document->isValid()) {
    reject(result,
           CreativeWorldLayoutTerrainReconciliationStatus::InvalidRequest,
           "creative_world_layout_terrain_reconciliation_invalid");
    return result;
  }

  DesiredSourceLookup desiredSources;
  if (!appendDesiredSources(*request.desiredLayout, desiredSources)) {
    reject(result,
           CreativeWorldLayoutTerrainReconciliationStatus::InvalidRequest,
           "creative_world_layout_terrain_reconciliation_desired_invalid");
    return result;
  }
  DecisionLookup decisionLookup;
  for (std::size_t index = 0U; index < request.decisions.size(); ++index) {
    const CreativeWorldLayoutTerrainConflictDecision& decision =
        request.decisions[index];
    if (!terrainTable(decision.table) ||
        decision.generatedIndex == kInvalidCreativeWorldLayoutIndex ||
        !validCreativeWorldLayoutStableKey(decision.stableKey) ||
        decision.resolution >=
            CreativeWorldLayoutTerrainConflictResolution::Count) {
      reject(result,
             CreativeWorldLayoutTerrainReconciliationStatus::DecisionInvalid,
             "creative_world_layout_terrain_decision_invalid");
      return result;
    }
    if (!decisionLookup
             .emplace(DecisionIdentity{decision.table, decision.generatedIndex,
                                       decision.stableKey},
                      index)
             .second) {
      reject(result,
             CreativeWorldLayoutTerrainReconciliationStatus::DecisionInvalid,
             "creative_world_layout_terrain_decision_duplicate");
      return result;
    }
  }

  const CreativeWorldLayoutTerrainImpactPlan impactPlan =
      buildCreativeWorldLayoutTerrainImpactPlan(*request.document,
                                                *request.generatedLayout);
  result.impactReasonCode = impactPlan.reasonCode;
  if (!impactPlan.accepted) {
    reject(result,
           CreativeWorldLayoutTerrainReconciliationStatus::ImpactRejected,
           "creative_world_layout_terrain_reconciliation_impact_rejected");
    return result;
  }

  for (const CreativeWorldLayoutTerrainSourceImpact& impact :
       impactPlan.sources) {
    if (impact.status != CreativeWorldLayoutTerrainImpactStatus::Drifted) {
      continue;
    }
    const auto desired = desiredSources.find(impact.stableKey);
    const bool sameSource =
        desired != desiredSources.end() && desired->second.table == impact.table;
    if (desired == desiredSources.end() &&
        request.desiredLayout->terrainOwnership ==
            CreativeWorldLayoutTerrainOwnership::PreserveExisting) {
      ++result.detachedSourceCount;
      continue;
    }

    CreativeWorldLayoutTerrainConflict conflict;
    conflict.generatedTable = impact.table;
    conflict.generatedIndex = impact.index;
    conflict.stableKey = impact.stableKey;
    conflict.impactStatus = impact.status;
    conflict.desiredSourcePresent = desired != desiredSources.end();
    if (conflict.desiredSourcePresent) {
      conflict.desiredTable = desired->second.table;
      conflict.desiredIndex = desired->second.index;
    }
    conflict.canDetachAndKeep3D =
        sameSource && request.desiredLayout->terrainOwnership ==
                          CreativeWorldLayoutTerrainOwnership::PreserveExisting;
    result.conflicts.push_back(std::move(conflict));
  }

  std::vector<bool> consumed(request.decisions.size(), false);
  bool unresolved = false;
  for (const CreativeWorldLayoutTerrainConflict& conflict : result.conflicts) {
    const auto found = decisionLookup.find(
        {conflict.generatedTable, conflict.generatedIndex, conflict.stableKey});
    if (found == decisionLookup.end() ||
        request.decisions[found->second].resolution ==
            CreativeWorldLayoutTerrainConflictResolution::Block) {
      unresolved = true;
      continue;
    }
    consumed[found->second] = true;
  }
  if (std::find(consumed.begin(), consumed.end(), false) != consumed.end()) {
    reject(result,
           CreativeWorldLayoutTerrainReconciliationStatus::DecisionInvalid,
           "creative_world_layout_terrain_decision_stale");
    return result;
  }
  if (unresolved) {
    result.blocked = true;
    result.status = CreativeWorldLayoutTerrainReconciliationStatus::Conflict;
    result.reasonCode = "creative_world_layout_terrain_refinement_conflict";
    return result;
  }

  result.accepted = true;
  result.status = CreativeWorldLayoutTerrainReconciliationStatus::Ready;
  result.reasonCode = "creative_world_layout_terrain_reconciliation_ready";
  return result;
}

}  // namespace iggy3d::creative
