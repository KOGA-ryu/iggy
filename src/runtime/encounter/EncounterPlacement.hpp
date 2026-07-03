#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <string_view>

#include "config/RuntimeConfig.hpp"
#include "content/FixtureScenarioLoader.hpp"
#include "content/assets/RoomAsset.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/InfluenceMap.hpp"
#include "runtime/ai/ReasoningGraph.hpp"
#include "runtime/encounter/EncounterDeck.hpp"
#include "runtime/physics/PhysicsAabbCollider.hpp"

namespace iggy3d {

// L8 deck kernel, slice 2 (A8b, deck doc §16-§19) -- THE FULL-DNA MILESTONE half: place a dealt hand
// onto a battlefield's affordances, validate the silhouette (rejects, never repairs), compose the
// paired seeds the existing Session pipeline consumes, and render the battle report. Pure functions
// over plain data; the module has ZERO Session dependency (Session appears only in the milestone
// test). Wire strings are the only cross-lane contract (a7s1 decoupling law).

// Named placement/validation tuning (no magic numbers).
struct PlacementConfig {
  float playerStartClearRadiusMeters = 3.0F;  // enemies within this of spawn WITH LOS are rejected
};

// One slot-by-slot placement outcome (report + debug).
struct PlacementRecord {
  std::string cardId;
  std::string anchorKind;
  std::string stableName;
  Vec3 position;
  bool placed = false;
  std::string note;
};

struct EncounterPlacementResult {
  std::vector<PlacementRecord> placements;
  std::vector<ScenarioEntitySeed> enemyEntities;    // paired by stableName with enemyAiActors
  std::vector<ScenarioAiActorSeed> enemyAiActors;
  std::vector<std::string> warnings;
  std::string appliedTactic = "none";
  Vec3 tacticalCenter;
  int totalCost = 0;  // sum of the dealt hand's card costs (== deal receipt totalCost)
};

enum class EncounterValidationReason : std::uint8_t {
  ObjectiveUnreachable,
  TacticObjectiveAbsent,
  EnemyAnchorMissing,
  EnemyRequiresUnmet,
  EnemyTooCloseToPlayerStart,
  BudgetExceeded,
};

struct EncounterValidationResult {
  bool accepted = true;
  std::vector<EncounterValidationReason> reasons;
};

struct EncounterBattleReport {
  std::string battlefieldId;
  std::uint64_t seed = 0;
  int budgetPoints = 0;
  int totalCost = 0;
  std::vector<EncounterDealEntry> draws;
  std::vector<PlacementRecord> placements;
  EncounterValidationResult validation;
  std::string difficultyBand;
  std::string appliedTactic;
  std::vector<std::string> warnings;
};

// Tally the room's anchors by wire-string kind into the a8b-s1 facts (what dealEncounterHand reads).
// `treasure`/`key`/`pickup` stay raw -- `objective` is DERIVED by s1's table, never an anchor.
BattlefieldFacts battlefieldFactsFromRoom(const RoomAsset& room);

// Place a dealt hand (EnemyRole cards -> paired entity+aiActor seeds at eligible anchors). Tactics
// are placement policies (patrol -> waypoints; defend_objective -> nearest-to-objective; ambush ->
// prefer concealed-from-spawn). Requires-bearing roles place at their affordance anchor; requires-
// free roles at enemy-spawn anchors (npc/monster). Pure/deterministic.
EncounterPlacementResult placeEncounterHand(const std::vector<EncounterCard>& hand,
                                            const RoomAsset& room, const ReasoningGraph& graph,
                                            std::span<const PhysicsAabbCollider> colliders,
                                            const PlacementConfig& config);

// Validate the placement against the silhouette. REJECTS (never repairs): appends every failing
// reason; accepted == reasons.empty(). Needs the dealt hand to re-check per-enemy requires.
EncounterValidationResult validateEncounter(const std::vector<EncounterCard>& hand,
                                            const EncounterPlacementResult& placement,
                                            const RoomAsset& room, const ReasoningGraph& graph,
                                            std::span<const PhysicsAabbCollider> colliders,
                                            EncounterBudget budget, const PlacementConfig& config);

// Compose the FixtureScenarioSeed the existing Session::create consumes: player entity FIRST (slot 0
// => EntityId{1}) at the spawn anchor, then the placed enemy entities; aiActors = the enemy actors;
// one non-completing objective so the session is well-formed and stays Playing. Pure (no Session).
FixtureScenarioSeed composeEncounterScenarioSeed(const RoomAsset& room,
                                                 const EncounterPlacementResult& placement,
                                                 const PlacementConfig& config,
                                                 const RuntimeConfig& runtimeConfig);

// Influence-derived warnings (A6 slice 2): the battle report's warnings section learns to warn from
// the L7 influence map computed over the PLACED encounter. Rules are DATA (evaluated in enum order),
// codes only (the renderer's no-floats law). NOT wired into Session/scoring (a9s4 territory).
enum class EncounterInfluenceWarning : std::uint8_t {
  ObjectiveLowCoverage,
  UnwatchedEscapeRoute,
};
std::string_view encounterInfluenceWarningCode(EncounterInfluenceWarning warning);

struct InfluenceWarningConfig {
  float objectiveCoverageThreshold = 1.0F;  // NAMED: guardInfluence at an objective node below this warns
  InfluenceMapConfig influence;             // the a6s1 kernel config (default)
};

// Guard samples for the influence map = each placed guard's ENTITY position UNION its patrol
// waypoints, DEDUP'd by position (first-wins, deterministic order). The union is honest: a patroller
// covers its route, so spawn-only sampling would fire `unwatched_escape_route` about a route a guard
// demonstrably walks. Dedup collapses the shared-post artifact (the Patrol tactic hands every guard
// the SAME post list) while genuine distinct-position multi-guard sums are preserved.
std::vector<InfluenceGuardSample> encounterGuardSamples(const EncounterPlacementResult& placement);

// Derive the influence warning codes over the placed encounter. Pure; borrows the already-baked
// graph+colliders (NEVER bakes). ObjectiveLowCoverage: an objective node exists AND the max
// guardInfluence over objective nodes < threshold. UnwatchedEscapeRoute: some exit node has zero
// visibilityCoverage. (Potential visibility may only OVER-cover vs a real cone, so this warning may
// only UNDER-fire -- it NEVER over-claims a route unwatched.)
std::vector<std::string> deriveEncounterInfluenceWarnings(const ReasoningGraph& graph,
                                                          std::span<const PhysicsAabbCollider> colliders,
                                                          const EncounterPlacementResult& placement,
                                                          const InfluenceWarningConfig& config);

EncounterBattleReport buildEncounterBattleReport(const std::string& battlefieldId,
                                                 const EncounterDealReceipt& dealReceipt,
                                                 const EncounterPlacementResult& placement,
                                                 const EncounterValidationResult& validation,
                                                 EncounterBudget budget,
                                                 const std::vector<std::string>& influenceWarnings = {});

// Deterministic key=value text (s8-dashboard discipline): FIXED line order, NO raw floats / NO
// positions, so the bitwise pin is robust to geometry retunes.
std::string renderEncounterBattleReport(const EncounterBattleReport& report);

}  // namespace iggy3d
