#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy3d {

// L8 encounter-deck kernel (A8b, deck_driven_tactical_ai.md §5-§7 + §19). The card data model, the
// 9-card starter set, the difficulty budgets, and a SEEDED deterministic deal producing a hand + a
// reasoning receipt. Pure functions over plain data; ZERO behavior change / INERT -- nothing
// consumes this (a8b-s2 places + proves). Style mirrors runtime/objective/ObjectiveOutcome:
// plain structs, data tables as build-functions, no session/save deps.
//
// The affordance `requiredFacts` travel as plain wire strings from docs/affordance_vocabulary_v0_1.md
// (same decoupling law as a7s1 -- no compile dependency on the vocab header). NOTE: `requires` is a
// C++20 reserved keyword, so the field is `requiredFacts`.

enum class EncounterCardType : std::uint8_t {
  EnemyRole,
  Hazard,
  Tactic,
  Objective,
};

enum class EncounterDealSkipReason : std::uint8_t {
  None,
  Budget,
  Requires,
  TacticAlreadyDealt,
  Conflict,
};

// One required affordance fact + how many are needed (minCount handles Patrol's >=2 patrol_post).
struct FactRequirement {
  std::string fact;
  int minCount = 1;
};

struct EncounterCard {
  std::string id;
  EncounterCardType type = EncounterCardType::EnemyRole;
  int cost = 0;
  std::vector<FactRequirement> requiredFacts;  // `requires` is a keyword -> requiredFacts
  std::vector<std::string> provides;           // descriptive tags (not test-load-bearing in s1)
  std::string profileId;                       // EnemyRole -> "default"; others ""
};

struct EncounterBudget {
  int points = 0;
};
inline constexpr EncounterBudget kEasyEncounterBudget{5};
inline constexpr EncounterBudget kMediumEncounterBudget{10};
inline constexpr EncounterBudget kHardEncounterBudget{16};

// The affordance wire-string kinds PRESENT on the battlefield and how many of each (counts, because
// Patrol needs >=2 patrol_post). Caller-supplied (a8b-s2 derives it; s1 tests build it by hand).
struct BattlefieldFacts {
  std::vector<std::pair<std::string, int>> counts;
};

// A derived fact: `derived` is available when ANY of `fromAny` anchor kinds is present (sum of
// their counts). "objective" is a DERIVED fact, never an anchor wire string.
struct DerivedFactRule {
  std::string derived;
  std::vector<std::string> fromAny;
};

// A conflict between two card ids. v1: keepBoth=true keeps both and appends `note` (a future
// reject-rule with keepBoth=false would skip with reason Conflict -- that path exists, unused in v1).
struct ConflictRule {
  std::string cardA;
  std::string cardB;
  bool keepBoth = true;
  std::string note;
};

struct EncounterDealEntry {
  std::string cardId;
  bool kept = false;
  EncounterDealSkipReason reason = EncounterDealSkipReason::None;
};

// Receipts never lie: EVERY draw is recorded (in shuffle order) with a truthful kept/reason.
struct EncounterDealReceipt {
  std::uint64_t seed = 0;
  int budgetPoints = 0;
  int totalCost = 0;
  std::vector<EncounterDealEntry> draws;
  std::vector<std::string> conflictNotes;
};

struct EncounterDealResult {
  std::vector<EncounterCard> hand;
  EncounterDealReceipt receipt;
};

inline constexpr std::size_t kStarterEncounterCardCount = 9;

// Data tables as build-functions (mirror buildObjectiveOutcomeTable; future rules stay data).
std::vector<EncounterCard> starterEncounterDeck();
std::vector<DerivedFactRule> encounterDerivedFactRules();
std::vector<ConflictRule> encounterConflictRules();

// Available count of `fact`: if `fact` names a derived rule, the SUM of its `fromAny` members'
// counts; else the raw count of `fact` in `facts` (0 if absent).
int availableFactCount(const BattlefieldFacts& facts, const std::string& fact,
                       const std::vector<DerivedFactRule>& derived);

// Seeded deterministic deal: splitmix64 Fisher-Yates over a copy, then a single pass skipping
// illegal cards (budget -> requires -> tactic-already-dealt precedence; conflict is keep-both in v1).
// Pure function of its four explicit inputs + the two internal data tables.
EncounterDealResult dealEncounterHand(const std::vector<EncounterCard>& deck, EncounterBudget budget,
                                      std::uint64_t seed, const BattlefieldFacts& facts);

}  // namespace iggy3d
