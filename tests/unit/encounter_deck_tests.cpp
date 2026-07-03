// a8b-s1 — L8 encounter-deck kernel: pure, headless, STRUCTURAL tests (quarantine hygiene -- NO
// golden over the real 9-card table; a8b-s2 owns the one real-table bitwise receipt pin).
// Determinism (call-twice self-compare), budget, requires + derived facts, the one-tactic rule, the
// keep-both conflict rule, and the starter-table invariants.

#include "runtime/encounter/EncounterDeck.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

iggy3d::BattlefieldFacts facts(std::vector<std::pair<std::string, int>> counts) {
  iggy3d::BattlefieldFacts f;
  f.counts = std::move(counts);
  return f;
}

const iggy3d::EncounterDealEntry* findDraw(const iggy3d::EncounterDealReceipt& receipt,
                                           std::string_view cardId) {
  for (const iggy3d::EncounterDealEntry& e : receipt.draws) {
    if (e.cardId == cardId) {
      return &e;
    }
  }
  return nullptr;
}

bool handHas(const std::vector<iggy3d::EncounterCard>& hand, std::string_view id) {
  for (const iggy3d::EncounterCard& c : hand) {
    if (c.id == id) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> drawOrder(const iggy3d::EncounterDealReceipt& receipt) {
  std::vector<std::string> order;
  for (const iggy3d::EncounterDealEntry& e : receipt.draws) {
    order.push_back(e.cardId);
  }
  return order;
}

bool receiptsEqual(const iggy3d::EncounterDealReceipt& a, const iggy3d::EncounterDealReceipt& b) {
  if (a.seed != b.seed || a.budgetPoints != b.budgetPoints || a.totalCost != b.totalCost ||
      a.draws.size() != b.draws.size() || a.conflictNotes != b.conflictNotes) {
    return false;
  }
  for (std::size_t i = 0; i < a.draws.size(); ++i) {
    if (a.draws[i].cardId != b.draws[i].cardId || a.draws[i].kept != b.draws[i].kept ||
        a.draws[i].reason != b.draws[i].reason) {
      return false;
    }
  }
  return true;
}

iggy3d::EncounterCard tacticCard(std::string id) {
  iggy3d::EncounterCard c;
  c.id = std::move(id);
  c.type = iggy3d::EncounterCardType::Tactic;
  c.cost = 1;
  return c;
}

bool determinismSelfCompare() {
  const std::vector<iggy3d::EncounterCard> deck = iggy3d::starterEncounterDeck();
  const iggy3d::BattlefieldFacts f = facts({{"high_ground", 1}, {"chokepoint", 1}, {"treasure", 1}});
  const iggy3d::EncounterDealResult a =
      iggy3d::dealEncounterHand(deck, iggy3d::kMediumEncounterBudget, 12345ULL, f);
  const iggy3d::EncounterDealResult b =
      iggy3d::dealEncounterHand(deck, iggy3d::kMediumEncounterBudget, 12345ULL, f);

  std::vector<std::string> aHand;
  std::vector<std::string> bHand;
  for (const iggy3d::EncounterCard& c : a.hand) {
    aHand.push_back(c.id);
  }
  for (const iggy3d::EncounterCard& c : b.hand) {
    bHand.push_back(c.id);
  }
  bool ok = expect(aHand == bHand && receiptsEqual(a.receipt, b.receipt),
                   "same inputs -> bitwise-identical hand + receipt");

  // Two DIFFERENT seeds -> a different draw (shuffle) order (not a magic hand).
  const iggy3d::EncounterDealResult c =
      iggy3d::dealEncounterHand(deck, iggy3d::kMediumEncounterBudget, 999999ULL, f);
  ok = ok && expect(drawOrder(a.receipt) != drawOrder(c.receipt),
                    "different seeds -> different draw order");
  return ok;
}

bool budgetDiscipline() {
  const std::vector<iggy3d::EncounterCard> deck = iggy3d::starterEncounterDeck();
  const iggy3d::BattlefieldFacts rich =
      facts({{"high_ground", 1}, {"chokepoint", 1}, {"patrol_post", 2}, {"treasure", 1}});
  const iggy3d::EncounterDealResult easy =
      iggy3d::dealEncounterHand(deck, iggy3d::kEasyEncounterBudget, 7ULL, rich);
  bool ok = expect(easy.receipt.totalCost <= iggy3d::kEasyEncounterBudget.points,
                   "hand total cost stays within budget");
  bool anyBudgetSkip = false;
  for (const iggy3d::EncounterDealEntry& e : easy.receipt.draws) {
    if (!e.kept && e.reason == iggy3d::EncounterDealSkipReason::Budget) {
      anyBudgetSkip = true;
    }
  }
  ok = ok && expect(anyBudgetSkip, "a 5-point budget over the 9-card deck skips some cards on Budget");

  // Zero budget -> empty hand, every draw skipped Budget.
  const iggy3d::EncounterDealResult broke =
      iggy3d::dealEncounterHand(deck, iggy3d::EncounterBudget{0}, 7ULL, rich);
  bool allBudget = !broke.receipt.draws.empty();
  for (const iggy3d::EncounterDealEntry& e : broke.receipt.draws) {
    allBudget = allBudget && !e.kept && e.reason == iggy3d::EncounterDealSkipReason::Budget;
  }
  return ok && expect(broke.hand.empty() && broke.receipt.totalCost == 0 && allBudget,
                      "zero budget -> empty hand with a truthful all-Budget receipt");
}

bool requiresFiltering() {
  // Single-card decks isolate the requires check.
  std::vector<iggy3d::EncounterCard> archerDeck;
  for (const iggy3d::EncounterCard& c : iggy3d::starterEncounterDeck()) {
    if (c.id == "archer") {
      archerDeck.push_back(c);
    }
  }
  const iggy3d::EncounterDealResult without =
      iggy3d::dealEncounterHand(archerDeck, iggy3d::kHardEncounterBudget, 1ULL, facts({}));
  const iggy3d::EncounterDealEntry* w = findDraw(without.receipt, "archer");
  bool ok = expect(w != nullptr && !w->kept && w->reason == iggy3d::EncounterDealSkipReason::Requires,
                   "no high_ground -> archer skipped with reason Requires");
  const iggy3d::EncounterDealResult with = iggy3d::dealEncounterHand(
      archerDeck, iggy3d::kHardEncounterBudget, 1ULL, facts({{"high_ground", 1}}));
  ok = ok && expect(handHas(with.hand, "archer"), "with high_ground -> archer dealt");

  // Derived fact: defend_objective requires the DERIVED `objective`, satisfied by `treasure`.
  std::vector<iggy3d::EncounterCard> defendDeck;
  for (const iggy3d::EncounterCard& c : iggy3d::starterEncounterDeck()) {
    if (c.id == "defend_objective") {
      defendDeck.push_back(c);
    }
  }
  const iggy3d::EncounterDealResult derived = iggy3d::dealEncounterHand(
      defendDeck, iggy3d::kHardEncounterBudget, 1ULL, facts({{"treasure", 1}}));
  ok = ok && expect(handHas(derived.hand, "defend_objective"),
                    "derived fact: treasure satisfies defend_objective's objective requirement");

  // Patrol needs >=2 patrol_post.
  std::vector<iggy3d::EncounterCard> patrolDeck;
  for (const iggy3d::EncounterCard& c : iggy3d::starterEncounterDeck()) {
    if (c.id == "patrol") {
      patrolDeck.push_back(c);
    }
  }
  const iggy3d::EncounterDealResult one = iggy3d::dealEncounterHand(
      patrolDeck, iggy3d::kHardEncounterBudget, 1ULL, facts({{"patrol_post", 1}}));
  const iggy3d::EncounterDealResult two = iggy3d::dealEncounterHand(
      patrolDeck, iggy3d::kHardEncounterBudget, 1ULL, facts({{"patrol_post", 2}}));
  ok = ok && expect(!handHas(one.hand, "patrol"), "1 patrol_post -> patrol Requires-skipped") &&
       expect(handHas(two.hand, "patrol"), "2 patrol_post -> patrol eligible");
  return ok;
}

bool oneTacticRule() {
  const std::vector<iggy3d::EncounterCard> deck = {tacticCard("tactic_a"), tacticCard("tactic_b")};
  const iggy3d::EncounterDealResult r =
      iggy3d::dealEncounterHand(deck, iggy3d::kHardEncounterBudget, 3ULL, facts({}));
  int kept = 0;
  int alreadyDealt = 0;
  for (const iggy3d::EncounterDealEntry& e : r.receipt.draws) {
    if (e.kept) {
      ++kept;
    } else if (e.reason == iggy3d::EncounterDealSkipReason::TacticAlreadyDealt) {
      ++alreadyDealt;
    }
  }
  return expect(kept == 1 && alreadyDealt == 1,
                "two tactics -> exactly one kept, the second TacticAlreadyDealt");
}

bool conflictRule() {
  std::vector<iggy3d::EncounterCard> deck;
  for (const iggy3d::EncounterCard& c : iggy3d::starterEncounterDeck()) {
    if (c.id == "rain" || c.id == "fire_barrel") {
      deck.push_back(c);
    }
  }
  const iggy3d::EncounterDealResult r =
      iggy3d::dealEncounterHand(deck, iggy3d::kHardEncounterBudget, 42ULL, facts({}));
  const long notes = std::count(r.receipt.conflictNotes.begin(), r.receipt.conflictNotes.end(),
                                std::string("fire_spread=dampened"));
  return expect(handHas(r.hand, "rain") && handHas(r.hand, "fire_barrel"),
                "rain + fire_barrel are BOTH kept (keep-both conflict)") &&
         expect(notes == 1, "the dampened conflict note appears exactly once");
}

bool tableInvariants() {
  const std::vector<iggy3d::EncounterCard> deck = iggy3d::starterEncounterDeck();
  bool ok = expect(deck.size() == iggy3d::kStarterEncounterCardCount, "starter deck has 9 cards");

  std::set<std::string> ids;
  for (const iggy3d::EncounterCard& c : deck) {
    ids.insert(c.id);
    ok = ok && expect(c.cost > 0, "every card cost is positive");
    if (c.type == iggy3d::EncounterCardType::EnemyRole) {
      ok = ok && expect(c.profileId == "default", "EnemyRole cards use profileId default");
    } else {
      ok = ok && expect(c.profileId.empty(), "non-EnemyRole cards have no profileId");
    }
  }
  ok = ok && expect(ids.size() == deck.size(), "card ids are unique");

  // requiredFacts must be affordance wire strings OR derived-fact names.
  std::set<std::string> allowed = {"spawn",       "npc",       "monster",   "exit",
                                   "treasure",    "key",       "pickup",    "chokepoint",
                                   "high_ground", "hiding_spot", "cover",   "patrol_post",
                                   "trap",        "reset_zone"};
  for (const iggy3d::DerivedFactRule& rule : iggy3d::encounterDerivedFactRules()) {
    allowed.insert(rule.derived);
  }
  for (const iggy3d::EncounterCard& c : deck) {
    for (const iggy3d::FactRequirement& req : c.requiredFacts) {
      ok = ok && expect(allowed.count(req.fact) == 1,
                        "every requiredFact is a wire string or a derived fact name");
    }
  }
  return ok;
}

}  // namespace

int main() {
  const bool ok = determinismSelfCompare() && budgetDiscipline() && requiresFiltering() &&
                  oneTacticRule() && conflictRule() && tableInvariants();
  return ok ? 0 : 1;
}
