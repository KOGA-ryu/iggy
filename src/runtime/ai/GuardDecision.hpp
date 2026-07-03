#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "core/ids/EntityId.hpp"
#include "core/math/Vec3.hpp"
#include "runtime/ai/NpcPersonalityWeights.hpp"
#include "runtime/ai/ReasoningGraph.hpp"

namespace iggy3d {

// L6 guard-decision kernel (A5, deck_driven_tactical_ai.md §8/§13/§18): a guard with heat but no
// live target CHOOSES a meaningful reasoning-graph node to check -- scored, deterministic, and
// EXPLAINED by a signed factor receipt. Pure functions + named config tables; ZERO behavior change
// (a5s2 wires the rung). 1-PLY only (map refusal: no deep search). Ally/objective terms are
// reserved-neutral columns so A9 lands without re-cutting.

// The guard's memory input -- used HOT OR COLD (a2 leaves lastKnownTargetPosition/Tick intact when
// clearMemory flips the flag). `hasMemorySample` is the CALLER's flag: true iff a sighting/sound was
// EVER recorded (a5s2 derives it as hasLastKnownTarget || lastKnownTargetTick != 0). A cold trail
// still scores, decayed by age; hasMemorySample=false => suspicion 0 everywhere (static tour).
struct GuardMemorySample {
  Vec3 lastKnownPosition{};
  std::uint64_t lastKnownTick = 0;
  bool hasMemorySample = false;
};

// One SIGNED contribution to a decision score (deck §18: no black boxes).
struct GuardDecisionFactor {
  std::string_view name;
  float value;
};

// The full, inspectable record of a decision. The chosen node's factors are, in order,
// {"suspicion", "strategic", "travel"(<=0), "ally"(0)}, each already multiplied by its personality
// weight so the receipt shows the ACTUAL contribution. hasChoice=false + empty factors when no
// candidate remains.
struct GuardDecisionReceipt {
  EntityId actor;
  std::uint64_t tick = 0;
  bool hasChoice = false;
  std::uint32_t chosenNodeId = 0;
  float totalScore = 0.0F;
  std::vector<GuardDecisionFactor> factors;
  bool hasExcluded = false;
  std::uint32_t excludedNodeId = 0;
};

// NAMED strategic base value per ReasoningNodeKind -- "how much a guard wants to check this kind of
// place." Exit/chokepoint HIGH, patrolPost/reference LOW. Reference-seeded; A10 retunes. Ordered to
// the enum (doorway..reference); the static_assert guards the count like A4's multiplier table.
inline constexpr std::array<float, kReasoningNodeKindCount> kDefaultStrategicValueByKind{
    12.0F,  // doorway
    6.0F,   // stair
    18.0F,  // chokepoint
    8.0F,   // hidingSpot
    8.0F,   // coverCluster
    15.0F,  // objective
    6.0F,   // window
    6.0F,   // ladder
    20.0F,  // exit
    2.0F,   // patrolPost
    10.0F,  // highGround
    6.0F,   // soundSource
    12.0F,  // lastKnownPosition
    2.0F,   // reference
};
static_assert(kDefaultStrategicValueByKind.size() == kReasoningNodeKindCount,
              "strategic table must list one base value per ReasoningNodeKind");

// NAMED scoring config (no magic numbers). Reference-seeded; A10 retunes.
struct GuardDecisionConfig {
  std::array<float, kReasoningNodeKindCount> strategicValueByKind = kDefaultStrategicValueByKind;
  float suspicionFalloffRadiusMeters = 8.0F;      // linear proximity: max(0, 1 - d/radius)
  float suspicionStalenessHalfLifeTicks = 100.0F;  // exponential decay: 0.5^(dt/halfLife)
};

struct GuardDecision {
  std::optional<std::uint32_t> nodeId;
  GuardDecisionReceipt receipt;
};

// Choose the best node to check. Pure, deterministic, NEVER bakes (colliders are input). Candidates
// = graph nodes with id != excludedNodeId, iterated in node-id order; a node whose direct segment is
// blocked AND has no route is UNREACHABLE and excluded from candidates. Score =
// suspicion*wSusp + strategic*wStrat - travel*wTravel + 0*wAlly; MAX wins, equal scores resolve to
// the lowest node id. No candidate => nodeId=nullopt, receipt.hasChoice=false.
GuardDecision chooseSearchNode(const ReasoningGraph& graph,
                               std::span<const PhysicsAabbCollider> colliders, Vec3 guardPosition,
                               const GuardMemorySample& memory, std::uint64_t tick, EntityId actor,
                               const NpcPersonalityWeights& weights, const GuardDecisionConfig& config,
                               std::optional<std::uint32_t> excludedNodeId = std::nullopt);

}  // namespace iggy3d
