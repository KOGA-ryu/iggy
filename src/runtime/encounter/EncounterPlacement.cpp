#include "runtime/encounter/EncounterPlacement.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

#include "core/math/Aabb3.hpp"
#include "core/math/Transform3.hpp"
#include "runtime/ai/InfluenceMap.hpp"
#include "runtime/ai/ReasoningRoute.hpp"

namespace iggy3d {
namespace {

float distanceMeters(Vec3 a, Vec3 b) { return std::sqrt(lengthSquared(b - a)); }

// Yaw (degrees) from `from` toward `to`: 0 = +Z, 90 = +X (atan2(dx, dz)).
float yawTowardDegrees(Vec3 from, Vec3 to) {
  const float dx = to.x - from.x;
  const float dz = to.z - from.z;
  if (std::abs(dx) < 1.0e-6F && std::abs(dz) < 1.0e-6F) {
    return 0.0F;
  }
  constexpr float kRadToDeg = 57.2957795F;
  return std::atan2(dx, dz) * kRadToDeg;
}

bool isObjectiveAnchorKind(const std::string& kind) {
  return kind == "treasure" || kind == "key" || kind == "pickup";
}

Vec3 spawnPosition(const RoomAsset& room) {
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.kind == "spawn") {
      return anchor.positionMeters;
    }
  }
  return Vec3{};
}

bool roomHasObjectiveAnchor(const RoomAsset& room) {
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (isObjectiveAnchorKind(anchor.kind)) {
      return true;
    }
  }
  return false;
}

// Tactical center = the first objective-item anchor position (stable order) if present, else spawn.
Vec3 tacticalCenterOf(const RoomAsset& room) {
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (isObjectiveAnchorKind(anchor.kind)) {
      return anchor.positionMeters;
    }
  }
  return spawnPosition(room);
}

std::string skipReasonCode(EncounterDealSkipReason reason) {
  switch (reason) {
    case EncounterDealSkipReason::None: return "none";
    case EncounterDealSkipReason::Budget: return "budget";
    case EncounterDealSkipReason::Requires: return "requires";
    case EncounterDealSkipReason::TacticAlreadyDealt: return "tactic_already_dealt";
    case EncounterDealSkipReason::Conflict: return "conflict";
  }
  return "unknown";
}

std::string validationReasonCode(EncounterValidationReason reason) {
  switch (reason) {
    case EncounterValidationReason::ObjectiveUnreachable: return "objective_unreachable";
    case EncounterValidationReason::TacticObjectiveAbsent: return "tactic_objective_absent";
    case EncounterValidationReason::EnemyAnchorMissing: return "enemy_anchor_missing";
    case EncounterValidationReason::EnemyRequiresUnmet: return "enemy_requires_unmet";
    case EncounterValidationReason::EnemyTooCloseToPlayerStart:
      return "enemy_too_close_to_player_start";
    case EncounterValidationReason::BudgetExceeded: return "budget_exceeded";
  }
  return "unknown";
}

std::string difficultyBandOf(int totalCost, int budgetPoints) {
  if (budgetPoints <= 0) {
    return totalCost > 0 ? "heavy" : "light";
  }
  const float ratio = static_cast<float>(totalCost) / static_cast<float>(budgetPoints);
  if (ratio <= 0.5F) {
    return "light";
  }
  if (ratio <= 0.85F) {
    return "standard";
  }
  return "heavy";
}

}  // namespace

BattlefieldFacts battlefieldFactsFromRoom(const RoomAsset& room) {
  BattlefieldFacts facts;
  for (const RoomAnchorAsset& anchor : room.anchors) {
    bool found = false;
    for (std::pair<std::string, int>& entry : facts.counts) {
      if (entry.first == anchor.kind) {
        ++entry.second;
        found = true;
        break;
      }
    }
    if (!found) {
      facts.counts.push_back({anchor.kind, 1});
    }
  }
  return facts;
}

EncounterPlacementResult placeEncounterHand(const std::vector<EncounterCard>& hand,
                                            const RoomAsset& room, const ReasoningGraph& graph,
                                            std::span<const PhysicsAabbCollider> colliders,
                                            const PlacementConfig& config) {
  static_cast<void>(graph);
  static_cast<void>(config);
  EncounterPlacementResult result;
  for (const EncounterCard& card : hand) {
    result.totalCost += card.cost;
  }

  for (const EncounterCard& card : hand) {
    if (card.type == EncounterCardType::Tactic) {
      result.appliedTactic = card.id;  // s1 guarantees at most one tactic per hand
      break;
    }
  }

  const Vec3 playerStart = spawnPosition(room);
  result.tacticalCenter = tacticalCenterOf(room);

  std::vector<Vec3> patrolPosts;
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.kind == "patrol_post") {
      patrolPosts.push_back(anchor.positionMeters);
    }
  }

  std::vector<bool> anchorUsed(room.anchors.size(), false);
  std::map<std::string, int> perCardCount;

  for (const EncounterCard& card : hand) {
    if (card.type != EncounterCardType::EnemyRole) {
      continue;  // Hazard/Tactic/Objective are reported, not placed as entities
    }
    const auto isEligible = [&card](const std::string& kind) {
      if (!card.requiredFacts.empty()) {
        return kind == card.requiredFacts.front().fact;  // requires-bearing -> its affordance
      }
      return kind == "npc" || kind == "monster";  // requires-free -> enemy-spawn anchors
    };

    int chosen = -1;
    if (result.appliedTactic == "ambush") {
      for (std::size_t i = 0; i < room.anchors.size(); ++i) {
        if (anchorUsed[i] || !isEligible(room.anchors[i].kind)) {
          continue;
        }
        if (reasoningSegmentBlocked(colliders, playerStart, room.anchors[i].positionMeters)) {
          chosen = static_cast<int>(i);  // concealed from spawn
          break;
        }
      }
      if (chosen < 0) {
        for (std::size_t i = 0; i < room.anchors.size(); ++i) {
          if (!anchorUsed[i] && isEligible(room.anchors[i].kind)) {
            chosen = static_cast<int>(i);
            break;
          }
        }
        if (chosen >= 0) {
          const std::string warning = "ambush=no_concealed_slots";
          if (std::find(result.warnings.begin(), result.warnings.end(), warning) ==
              result.warnings.end()) {
            result.warnings.push_back(warning);
          }
        }
      }
    } else if (result.appliedTactic == "defend_objective") {
      float best = std::numeric_limits<float>::infinity();
      for (std::size_t i = 0; i < room.anchors.size(); ++i) {
        if (anchorUsed[i] || !isEligible(room.anchors[i].kind)) {
          continue;
        }
        const float d = distanceMeters(room.anchors[i].positionMeters, result.tacticalCenter);
        if (chosen < 0 || d < best) {
          best = d;
          chosen = static_cast<int>(i);  // nearest the objective (lowest-index tie via strict <)
        }
      }
    } else {
      for (std::size_t i = 0; i < room.anchors.size(); ++i) {
        if (!anchorUsed[i] && isEligible(room.anchors[i].kind)) {
          chosen = static_cast<int>(i);  // neutral anchor order
          break;
        }
      }
    }

    PlacementRecord record;
    record.cardId = card.id;
    if (chosen < 0) {
      record.placed = false;
      record.note = "no_slot:" + card.id;
      result.warnings.push_back("no_slot:" + card.id);
      result.placements.push_back(std::move(record));
      continue;
    }

    anchorUsed[static_cast<std::size_t>(chosen)] = true;
    const RoomAnchorAsset& anchor = room.anchors[static_cast<std::size_t>(chosen)];
    const int index = perCardCount[card.id]++;
    const std::string stableName = "enemy_" + card.id + "_" + std::to_string(index);
    record.anchorKind = anchor.kind;
    record.stableName = stableName;
    record.position = anchor.positionMeters;
    record.placed = true;
    result.placements.push_back(std::move(record));

    // Paired seeds (joined by stableName; entity carries position, aiActor carries profile/route).
    ScenarioEntitySeed entity;
    entity.stableName = stableName;
    entity.kind = EntityKind::Npc;
    entity.transform = identityTransform3();
    entity.transform.position = anchor.positionMeters;
    entity.localBounds = makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.2F, 0.25F});
    entity.active = true;
    entity.persistent = true;
    entity.targeting.targetable = true;
    entity.targeting.actions = {TargetAction::Attack, TargetAction::Inspect};
    entity.combatantEnabled = true;
    entity.combatant.factionId = 2;
    entity.combatant.hitPoints = 3;
    entity.combatant.maxHitPoints = 3;
    result.enemyEntities.push_back(std::move(entity));

    ScenarioAiActorSeed aiActor;
    aiActor.actorStableName = stableName;
    aiActor.behaviorProfileId = card.profileId.empty() ? "default" : card.profileId;
    aiActor.hasFacing = true;
    aiActor.facingDegrees = yawTowardDegrees(anchor.positionMeters, result.tacticalCenter);
    aiActor.patrolMode = PatrolMode::Loop;
    if (result.appliedTactic == "patrol") {
      aiActor.patrolWaypoints = patrolPosts;  // guards actually patrol (no dealt card changes nothing)
    }
    result.enemyAiActors.push_back(std::move(aiActor));
  }

  return result;
}

EncounterValidationResult validateEncounter(const std::vector<EncounterCard>& hand,
                                            const EncounterPlacementResult& placement,
                                            const RoomAsset& room, const ReasoningGraph& graph,
                                            std::span<const PhysicsAabbCollider> colliders,
                                            EncounterBudget budget, const PlacementConfig& config) {
  EncounterValidationResult result;
  const BattlefieldFacts facts = battlefieldFactsFromRoom(room);
  const std::vector<DerivedFactRule> derived = encounterDerivedFactRules();
  const Vec3 playerStart = spawnPosition(room);
  const bool hasObjective = roomHasObjectiveAnchor(room);

  // 1. Objective reachable from player start (A4 route over the graph + colliders).
  if (hasObjective) {
    const PlannedRoute route = planRoute(graph, colliders, playerStart, placement.tacticalCenter, {});
    if (route.nodeIds.empty()) {
      result.reasons.push_back(EncounterValidationReason::ObjectiveUnreachable);
    }
  }
  // 2. DefendObjective needs an objective anchor.
  if (placement.appliedTactic == "defend_objective" && !hasObjective) {
    result.reasons.push_back(EncounterValidationReason::TacticObjectiveAbsent);
  }
  // 3. Each placed enemy: its anchor kind present + its card's requires satisfied.
  for (const PlacementRecord& record : placement.placements) {
    if (!record.placed) {
      continue;
    }
    if (availableFactCount(facts, record.anchorKind, derived) < 1) {
      result.reasons.push_back(EncounterValidationReason::EnemyAnchorMissing);
    }
    for (const EncounterCard& card : hand) {
      if (card.id != record.cardId) {
        continue;
      }
      for (const FactRequirement& requirement : card.requiredFacts) {
        if (availableFactCount(facts, requirement.fact, derived) < requirement.minCount) {
          result.reasons.push_back(EncounterValidationReason::EnemyRequiresUnmet);
          break;
        }
      }
      break;
    }
  }
  // 4. No placed enemy inside the clear radius WITH line of sight from spawn.
  for (const PlacementRecord& record : placement.placements) {
    if (!record.placed) {
      continue;
    }
    if (distanceMeters(record.position, playerStart) < config.playerStartClearRadiusMeters &&
        !reasoningSegmentBlocked(colliders, playerStart, record.position)) {
      result.reasons.push_back(EncounterValidationReason::EnemyTooCloseToPlayerStart);
    }
  }
  // 5. Cost within budget.
  if (placement.totalCost > budget.points) {
    result.reasons.push_back(EncounterValidationReason::BudgetExceeded);
  }

  result.accepted = result.reasons.empty();
  return result;
}

FixtureScenarioSeed composeEncounterScenarioSeed(const RoomAsset& room,
                                                 const EncounterPlacementResult& placement,
                                                 const PlacementConfig& config,
                                                 const RuntimeConfig& runtimeConfig) {
  static_cast<void>(config);
  FixtureScenarioSeed seed;
  seed.scenarioId = "encounter_" + room.id;
  seed.config = runtimeConfig;
  seed.players.push_back(ScenarioPlayerSeed{0, PlayerSlotKind::Local, "player"});

  // Player FIRST => EntityId{1} (the Session::create law).
  ScenarioEntitySeed player;
  player.stableName = "player";
  player.kind = EntityKind::Player;
  player.transform = identityTransform3();
  player.transform.position = spawnPosition(room);
  player.localBounds = makeAabb3({-0.25F, 0.0F, -0.25F}, {0.25F, 1.8F, 0.25F});
  player.active = true;
  player.persistent = true;
  player.targeting.targetable = true;
  player.targeting.actions = {TargetAction::Attack, TargetAction::Inspect};
  player.combatantEnabled = true;
  player.combatant.factionId = 1;
  player.combatant.hitPoints = 10;
  player.combatant.maxHitPoints = 10;
  seed.entities.push_back(std::move(player));
  for (const ScenarioEntitySeed& enemy : placement.enemyEntities) {
    seed.entities.push_back(enemy);
  }
  seed.aiActors = placement.enemyAiActors;

  // One non-completing objective (condition None) so the session is well-formed and stays Playing.
  std::string target = "objective";
  for (const RoomAnchorAsset& anchor : room.anchors) {
    if (anchor.kind == "exit") {
      target = "exit";
      break;
    }
  }
  ScenarioObjectiveSeed objective;
  objective.id = "reach_" + target;
  objective.initialStatus = ObjectiveStatusSeed::Active;
  objective.condition = "None";
  objective.playerSlot = 0;
  seed.objectives.push_back(std::move(objective));

  return seed;
}

std::string_view encounterInfluenceWarningCode(EncounterInfluenceWarning warning) {
  switch (warning) {
    case EncounterInfluenceWarning::ObjectiveLowCoverage: return "objective_low_coverage";
    case EncounterInfluenceWarning::UnwatchedEscapeRoute: return "unwatched_escape_route";
  }
  return "unknown";
}

std::vector<InfluenceGuardSample> encounterGuardSamples(const EncounterPlacementResult& placement) {
  std::vector<InfluenceGuardSample> samples;
  const auto pushDedup = [&samples](Vec3 pos) {
    for (const InfluenceGuardSample& existing : samples) {
      if (nearlyEqual(existing.positionMeters, pos, 1.0e-3F)) {
        return;  // shared post / duplicate -> first wins
      }
    }
    samples.push_back(InfluenceGuardSample{pos});
  };
  for (const ScenarioAiActorSeed& aiActor : placement.enemyAiActors) {
    for (const ScenarioEntitySeed& entity : placement.enemyEntities) {
      if (entity.stableName == aiActor.actorStableName) {
        pushDedup(entity.transform.position);  // the guard's placed position
        break;
      }
    }
    for (const Vec3& waypoint : aiActor.patrolWaypoints) {
      pushDedup(waypoint);  // a patroller covers its route
    }
  }
  return samples;
}

std::vector<std::string> deriveEncounterInfluenceWarnings(const ReasoningGraph& graph,
                                                          std::span<const PhysicsAabbCollider> colliders,
                                                          const EncounterPlacementResult& placement,
                                                          const InfluenceWarningConfig& config) {
  std::vector<std::string> warnings;
  const std::vector<InfluenceGuardSample> samples = encounterGuardSamples(placement);
  const InfluenceMap map = buildInfluenceMap(graph, colliders, samples, config.influence);

  // Rule order = enum order. Each rule pushes its code AT MOST ONCE.
  // ObjectiveLowCoverage: an objective node exists AND max guardInfluence over objective nodes is
  // below the named threshold. (No objective node -> not a warning; that is TacticObjectiveAbsent's
  // concern in validateEncounter.)
  bool hasObjective = false;
  float maxObjectiveGuardInfluence = 0.0F;
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    if (graph.nodes[i].kind != ReasoningNodeKind::objective) {
      continue;
    }
    const float guardInfluence = influenceValue(map, i, InfluenceChannel::guardInfluence);
    if (!hasObjective || guardInfluence > maxObjectiveGuardInfluence) {
      maxObjectiveGuardInfluence = guardInfluence;
    }
    hasObjective = true;
  }
  if (hasObjective && maxObjectiveGuardInfluence < config.objectiveCoverageThreshold) {
    warnings.emplace_back(
        encounterInfluenceWarningCode(EncounterInfluenceWarning::ObjectiveLowCoverage));
  }

  // UnwatchedEscapeRoute: some exit node has zero visibilityCoverage from the placed guards.
  bool unwatchedExit = false;
  for (std::size_t i = 0; i < graph.nodes.size(); ++i) {
    if (graph.nodes[i].kind == ReasoningNodeKind::exit &&
        influenceValue(map, i, InfluenceChannel::visibilityCoverage) == 0.0F) {
      unwatchedExit = true;
      break;
    }
  }
  if (unwatchedExit) {
    warnings.emplace_back(
        encounterInfluenceWarningCode(EncounterInfluenceWarning::UnwatchedEscapeRoute));
  }

  return warnings;
}

EncounterBattleReport buildEncounterBattleReport(const std::string& battlefieldId,
                                                 const EncounterDealReceipt& dealReceipt,
                                                 const EncounterPlacementResult& placement,
                                                 const EncounterValidationResult& validation,
                                                 EncounterBudget budget,
                                                 const std::vector<std::string>& influenceWarnings) {
  EncounterBattleReport report;
  report.battlefieldId = battlefieldId;
  report.seed = dealReceipt.seed;
  report.budgetPoints = budget.points;
  report.totalCost = dealReceipt.totalCost;
  report.draws = dealReceipt.draws;
  report.placements = placement.placements;
  report.validation = validation;
  report.appliedTactic = placement.appliedTactic;
  report.warnings = placement.warnings;
  for (const std::string& warning : influenceWarnings) {
    report.warnings.push_back(warning);  // append derived influence warnings after placement's
  }
  report.difficultyBand = difficultyBandOf(dealReceipt.totalCost, budget.points);
  return report;
}

std::string renderEncounterBattleReport(const EncounterBattleReport& report) {
  std::string out;
  out += "battlefield=" + report.battlefieldId + "\n";
  out += "seed=" + std::to_string(report.seed) + "\n";
  out += "budget=" + std::to_string(report.budgetPoints) + "\n";
  out += "cost=" + std::to_string(report.totalCost) + "\n";
  out += "difficulty=" + report.difficultyBand + "\n";
  out += "tactic=" + report.appliedTactic + "\n";

  std::string dealt;
  std::string skipped;
  for (const EncounterDealEntry& entry : report.draws) {
    if (entry.kept) {
      dealt += (dealt.empty() ? "" : ",") + entry.cardId;
    } else {
      const std::string token = entry.cardId + ":" + skipReasonCode(entry.reason);
      skipped += (skipped.empty() ? "" : ",") + token;
    }
  }
  out += "dealt=" + dealt + "\n";
  out += "skipped=" + skipped + "\n";

  int placementIndex = 0;
  for (const PlacementRecord& record : report.placements) {
    if (!record.placed) {
      continue;
    }
    out += "placement." + std::to_string(placementIndex) + "=" + record.cardId + "@" +
           record.anchorKind + "#" + record.stableName + "\n";
    ++placementIndex;
  }

  out += std::string("validation=") + (report.validation.accepted ? "accepted" : "rejected") + "\n";
  for (std::size_t i = 0; i < report.validation.reasons.size(); ++i) {
    out += "reason." + std::to_string(i) + "=" + validationReasonCode(report.validation.reasons[i]) +
           "\n";
  }
  for (std::size_t i = 0; i < report.warnings.size(); ++i) {
    out += "warning." + std::to_string(i) + "=" + report.warnings[i] + "\n";
  }
  return out;
}

}  // namespace iggy3d
