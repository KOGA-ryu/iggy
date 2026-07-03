#include "runtime/encounter/EncounterDeck.hpp"

#include <array>
#include <utility>

namespace iggy3d {
namespace {

// The descriptor discipline (palette-descriptor precedent): the starter set is a constexpr POD
// table, so the count / unique-id / positive-cost invariants are compile-time static_asserts.
struct CardDescriptor {
  std::string_view id;
  EncounterCardType type;
  int cost;
  std::string_view requiredFact;  // empty => no requirement
  int requiredMinCount;
  std::string_view provides;      // empty => none (v1 rows carry a single descriptive tag)
  std::string_view profileId;     // "" for non-enemy cards
};

inline constexpr std::array<CardDescriptor, kStarterEncounterCardCount> kStarterDescriptors{{
    {"brute", EncounterCardType::EnemyRole, 1, "", 0, "melee_body", "default"},
    {"archer", EncounterCardType::EnemyRole, 2, "high_ground", 1, "ranged_body", "default"},
    {"trapper", EncounterCardType::EnemyRole, 2, "chokepoint", 1, "trap_body", "default"},
    {"rain", EncounterCardType::Hazard, 1, "", 0, "weather", ""},
    {"fog", EncounterCardType::Hazard, 2, "", 0, "concealment", ""},
    {"fire_barrel", EncounterCardType::Hazard, 1, "", 0, "fire_hazard", ""},
    {"patrol", EncounterCardType::Tactic, 1, "patrol_post", 2, "patrol_policy", ""},
    {"ambush", EncounterCardType::Tactic, 2, "", 0, "ambush_policy", ""},
    {"defend_objective", EncounterCardType::Tactic, 2, "objective", 1, "defend_policy", ""},
}};

constexpr bool descriptorsHaveUniqueIds() {
  for (std::size_t i = 0; i < kStarterDescriptors.size(); ++i) {
    for (std::size_t j = i + 1; j < kStarterDescriptors.size(); ++j) {
      if (kStarterDescriptors[i].id == kStarterDescriptors[j].id) {
        return false;
      }
    }
  }
  return true;
}

constexpr bool descriptorsHavePositiveCosts() {
  for (const CardDescriptor& descriptor : kStarterDescriptors) {
    if (descriptor.cost <= 0) {
      return false;
    }
  }
  return true;
}

static_assert(kStarterDescriptors.size() == kStarterEncounterCardCount,
              "starter deck must hold exactly kStarterEncounterCardCount cards");
static_assert(descriptorsHaveUniqueIds(), "starter card ids must be unique");
static_assert(descriptorsHavePositiveCosts(), "starter card costs must be positive");

// Deterministic, cross-platform PRNG (splitmix64 -- NOT <random> distributions, which are
// implementation-defined across stdlibs). Documented so the shuffle is reproducible everywhere.
struct Splitmix64 {
  std::uint64_t state;
  std::uint64_t next() {
    state += 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
  }
};

bool requiresSatisfied(const EncounterCard& card, const BattlefieldFacts& facts,
                       const std::vector<DerivedFactRule>& derived) {
  for (const FactRequirement& requirement : card.requiredFacts) {
    if (availableFactCount(facts, requirement.fact, derived) < requirement.minCount) {
      return false;
    }
  }
  return true;
}

}  // namespace

std::vector<EncounterCard> starterEncounterDeck() {
  std::vector<EncounterCard> deck;
  deck.reserve(kStarterDescriptors.size());
  for (const CardDescriptor& descriptor : kStarterDescriptors) {
    EncounterCard card;
    card.id = std::string(descriptor.id);
    card.type = descriptor.type;
    card.cost = descriptor.cost;
    if (!descriptor.requiredFact.empty()) {
      card.requiredFacts.push_back(
          FactRequirement{std::string(descriptor.requiredFact), descriptor.requiredMinCount});
    }
    if (!descriptor.provides.empty()) {
      card.provides.push_back(std::string(descriptor.provides));
    }
    card.profileId = std::string(descriptor.profileId);
    deck.push_back(std::move(card));
  }
  return deck;
}

std::vector<DerivedFactRule> encounterDerivedFactRules() {
  // One row: an objective fact is derived from any objective-item anchor.
  return {DerivedFactRule{"objective", {"treasure", "key", "pickup"}}};
}

std::vector<ConflictRule> encounterConflictRules() {
  // One row (§6 pattern): rain + fire_barrel are BOTH kept, the receipt notes the dampening.
  return {ConflictRule{"rain", "fire_barrel", /*keepBoth=*/true, "fire_spread=dampened"}};
}

int availableFactCount(const BattlefieldFacts& facts, const std::string& fact,
                       const std::vector<DerivedFactRule>& derived) {
  for (const DerivedFactRule& rule : derived) {
    if (rule.derived == fact) {
      int sum = 0;
      for (const std::string& source : rule.fromAny) {
        for (const std::pair<std::string, int>& entry : facts.counts) {
          if (entry.first == source) {
            sum += entry.second;
          }
        }
      }
      return sum;
    }
  }
  int raw = 0;
  for (const std::pair<std::string, int>& entry : facts.counts) {
    if (entry.first == fact) {
      raw += entry.second;
    }
  }
  return raw;
}

EncounterDealResult dealEncounterHand(const std::vector<EncounterCard>& deck, EncounterBudget budget,
                                      std::uint64_t seed, const BattlefieldFacts& facts) {
  EncounterDealResult result;
  result.receipt.seed = seed;
  result.receipt.budgetPoints = budget.points;

  // Seeded Fisher-Yates over a copy. `% (n - i)` is fully deterministic + cross-platform;
  // unbiased-bounded selection is not required here (documented in the header).
  std::vector<EncounterCard> shuffled = deck;
  Splitmix64 rng{seed};
  const std::size_t n = shuffled.size();
  for (std::size_t i = 0; i + 1 < n; ++i) {
    const std::size_t j = i + static_cast<std::size_t>(rng.next() % (n - i));
    std::swap(shuffled[i], shuffled[j]);
  }

  const std::vector<DerivedFactRule> derived = encounterDerivedFactRules();
  const std::vector<ConflictRule> conflicts = encounterConflictRules();

  int spent = 0;
  bool tacticDealt = false;
  for (const EncounterCard& card : shuffled) {
    EncounterDealEntry entry;
    entry.cardId = card.id;
    // Skip precedence (first failure wins): Budget -> Requires -> TacticAlreadyDealt.
    if (spent + card.cost > budget.points) {
      entry.reason = EncounterDealSkipReason::Budget;
    } else if (!requiresSatisfied(card, facts, derived)) {
      entry.reason = EncounterDealSkipReason::Requires;
    } else if (card.type == EncounterCardType::Tactic && tacticDealt) {
      entry.reason = EncounterDealSkipReason::TacticAlreadyDealt;
    } else {
      // Conflict is keep-both in v1: annotate (once) when the other card of a rule is already kept.
      for (const ConflictRule& rule : conflicts) {
        if (rule.cardA != card.id && rule.cardB != card.id) {
          continue;
        }
        const std::string& other = (rule.cardA == card.id) ? rule.cardB : rule.cardA;
        bool otherKept = false;
        for (const EncounterCard& kept : result.hand) {
          if (kept.id == other) {
            otherKept = true;
            break;
          }
        }
        if (otherKept && rule.keepBoth) {
          bool notepresent = false;
          for (const std::string& note : result.receipt.conflictNotes) {
            if (note == rule.note) {
              notepresent = true;
              break;
            }
          }
          if (!notepresent) {
            result.receipt.conflictNotes.push_back(rule.note);
          }
        }
      }
      result.hand.push_back(card);
      spent += card.cost;
      if (card.type == EncounterCardType::Tactic) {
        tacticDealt = true;
      }
      entry.kept = true;
      entry.reason = EncounterDealSkipReason::None;
    }
    result.receipt.draws.push_back(std::move(entry));
  }
  result.receipt.totalCost = spent;
  return result;
}

}  // namespace iggy3d
