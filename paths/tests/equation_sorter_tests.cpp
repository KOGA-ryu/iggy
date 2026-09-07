#include "runtime/sorter/EquationSorterSession.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>

using namespace paths;
void expect(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::vector<SorterEquation> fixture() {
  std::vector<SorterEquation> result;
  for (std::size_t home = 0; home < 100; ++home)
    result.push_back({static_cast<unsigned>(1001 + home * 37 % 100), home, "x + " + std::to_string(home) + " = 100"});
  std::reverse(result.begin(), result.end());
  return result;
}
SorterResult send(EquationSorterSession& s, SorterActionKind kind, SorterEquationId id = 0, SorterBucket b = SorterBucket::A) {
  return s.dispatch({kind, b, id, s.view().revision});
}
void clickTwice(EquationSorterSession& s, SorterEquationId id) {
  expect(send(s, SorterActionKind::ActivateEquation, id).accepted, "first activation accepted");
  expect(send(s, SorterActionKind::ActivateEquation, id).accepted, "second activation accepted");
}
void invariants(const EquationSorterSession& s) {
  auto v = s.view();
  expect(s.content().size() == 100 && std::accumulate(v.counts.begin(), v.counts.end(), 0U) == 100, "all 100 remain owned");
  expect(!v.inventory || v.activeBucket.has_value(), "inventory has destination");
  std::set<SorterEquationId> slots;
  for (std::size_t i = 0; i < v.slotCount; ++i) expect(slots.insert(v.inventorySlots[i]).second, "unique slots");
  expect(!v.nextStep.empty() && !v.hint.empty(), "every sorter state exposes guidance");
  expect(v.autoSortCount + v.unclassifiedCount == v.counts[0], "automatic eligibility accounts for every remaining card");
  std::array<std::size_t, sorterBucketCount + 1> counts{};
  for (std::size_t i = 0; i < s.content().size(); ++i) {
    auto id = s.content()[i].id;
    expect(s.content()[i].homeIndex == i, "immutable sorted home mapping");
    ++counts[v.owners[i] ? static_cast<int>(*v.owners[i]) + 1 : 0];
    if (v.inventory && v.owners[i] == v.activeBucket) expect(slots.contains(id), "all inventory members have slots");
    if (v.inspected == id) expect(v.inventory ? v.owners[i] == v.activeBucket : !v.owners[i], "inspected card available");
  }
  expect(counts == v.counts, "counts derived from owners");
}
void basic() {
  EquationSorterSession s(fixture());
  const auto id = s.content()[0].id, second = s.content()[1].id;
  expect(!s.view().activeBucket && !s.view().undoDepth, "clean initial state");
  clickTwice(s, id);
  expect(s.view().inspected == id && s.view().counts[0] == 100, "inspection without destination cannot assign");
  (void)send(s, SorterActionKind::SelectBucket);
  expect(!s.view().inspected && !s.view().inventory, "select destination clears inspection and stays grid");
  const SorterAction click{SorterActionKind::ActivateEquation, SorterBucket::A, id, s.view().revision};
  expect(s.dispatch(click).accepted && s.view().counts[0] == 100, "first activation only inspects");
  expect(!s.dispatch(click).accepted && s.view().inspected == id, "stale activation cannot commit");
  (void)send(s, SorterActionKind::ActivateEquation, second);
  expect(s.view().inspected == second && s.view().counts[0] == 100, "different card changes inspection");
  (void)send(s, SorterActionKind::ActivateEquation, second);
  expect(s.view().owners[1] == SorterBucket::A && s.view().undoDepth == 1, "second activation commits once");
  for (int i = 0; i < 5; ++i) expect(!send(s, SorterActionKind::ActivateEquation, second).accepted, "vacated slot rejects repeats");
  (void)send(s, SorterActionKind::SelectBucket);
  expect(s.view().inventory && s.view().slotCount == 1, "active bucket opens inventory");
  clickTwice(s, second);
  expect(!s.view().owners[1] && s.view().inventorySlots[0] == second, "return retains slot reservation");
  (void)send(s, SorterActionKind::Undo);
  expect(s.view().owners[1] == SorterBucket::A && s.view().inventorySlots[0] == second, "Undo refills reserved slot");
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::Dump);
  expect(s.view().inventory && s.view().activeBucket == SorterBucket::Dump && s.view().slotCount == 0, "inventory switches directly to empty Dump");
  (void)send(s, SorterActionKind::Undo);
  expect(s.view().activeBucket == SorterBucket::Dump && s.view().inventory && s.view().counts[0] == 100, "cross-view Undo preserves view");
  invariants(s);
}
void reservedUndoAndBulk() {
  EquationSorterSession s(fixture());
  const auto e1 = s.content()[0].id, e2 = s.content()[1].id, e3 = s.content()[2].id;
  (void)send(s, SorterActionKind::SelectBucket);
  clickTwice(s, e1); clickTwice(s, e2);
  (void)send(s, SorterActionKind::SelectBucket);
  clickTwice(s, e1);
  (void)send(s, SorterActionKind::BackToGrid);
  clickTwice(s, e3);
  (void)send(s, SorterActionKind::SelectBucket);
  (void)send(s, SorterActionKind::Undo);
  (void)send(s, SorterActionKind::Undo);
  auto v = s.view();
  expect(v.slotCount == 3 && v.inventorySlots[0] == e2 && v.inventorySlots[1] == e3 && v.inventorySlots[2] == e1,
      "absent-at-open restoration appends without filling another card's hole");
  (void)send(s, SorterActionKind::SelectBucket);
  expect(s.view().inventorySlots == v.inventorySlots, "same open bucket never compacts");
  (void)send(s, SorterActionKind::RequestEmpty);
  expect(s.view().pendingEmpty && s.view().owners == v.owners, "empty request changes no membership");
  (void)send(s, SorterActionKind::CancelEmpty);
  expect(!s.view().pendingEmpty && s.view().undoDepth == v.undoDepth, "cancel changes no history");
  (void)send(s, SorterActionKind::RequestEmpty);
  (void)send(s, SorterActionKind::ActivateEquation, e2);
  expect(!s.view().pendingEmpty && s.view().inspected == e2 && s.view().owners == v.owners, "card cancels confirmation and only inspects");
  (void)send(s, SorterActionKind::RequestEmpty);
  (void)send(s, SorterActionKind::ConfirmEmpty);
  expect(s.view().counts[0] == 100 && s.view().undoDepth == v.undoDepth + 1, "bulk empty is one transaction");
  (void)send(s, SorterActionKind::Undo);
  expect(s.view().owners == v.owners && s.view().inventorySlots == v.inventorySlots, "one Undo restores exact bulk and positions");
  (void)send(s, SorterActionKind::RequestEmpty);
  const auto stale = SorterAction{SorterActionKind::ConfirmEmpty, SorterBucket::A, 0, s.view().revision};
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::B);
  expect(!s.dispatch(stale).accepted && !s.view().pendingEmpty, "navigation invalidates confirmation");
  invariants(s);
}
void fullAndMixed() {
  auto content = fixture();
  for (auto& e : content) e.subject = static_cast<SorterSubject>(e.homeIndex % sorterSubjects.size());
  EquationSorterSession s(content);
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::Dump);
  for (const auto& e : s.content()) clickTwice(s, e.id);
  expect(s.view().counts[0] == 0 && s.view().counts[sorterBucketCount] == 100, "all assigned remains a recoverable state");
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::Dump);
  (void)send(s, SorterActionKind::RequestEmpty);
  (void)send(s, SorterActionKind::ConfirmEmpty);
  (void)send(s, SorterActionKind::Undo);
  expect(s.view().counts[sorterBucketCount] == 100, "bulk restores all 100");
  for (int i = 0; i < 100; ++i) { (void)send(s, SorterActionKind::Undo); invariants(s); }
  expect(!s.view().undoDepth && s.view().counts[0] == 100, "history fully exhausts");
  expect(!send(s, SorterActionKind::Undo).changed, "exhausted Undo is a no-op");
  std::mt19937 rng(2037);
  std::vector<decltype(s.view().owners)> history;
  for (int i = 0; i < 5000; ++i) {
    const auto before = s.view();
    const auto kind = static_cast<SorterActionKind>(rng() % 11);
    const auto result = send(s, kind, 1001 + rng() % 103, static_cast<SorterBucket>(rng() % (sorterBucketCount + 1)));
    const auto after = s.view();
    if (!result.accepted) expect(after.owners == before.owners && after.revision == before.revision, "rejection preserves state");
    if (kind == SorterActionKind::Undo && before.undoDepth) {
      expect(!history.empty() && after.owners == history.back(), "mixed Undo matches independent ownership snapshots");
      history.pop_back();
    } else if (before.owners != after.owners) history.push_back(before.owners);
    expect(after.undoDepth == history.size(), "only committed ownership changes enter history");
    invariants(s);
  }
}
void autoSortAndHints() {
  auto content = fixture();
  for (auto& e : content) {
    e.subject = static_cast<SorterSubject>(e.homeIndex % sorterSubjects.size());
    e.hint = "Prepared hint for " + e.text;
  }
  content[0].subject.reset();
  EquationSorterSession s(content);
  const auto manual = s.content()[0].id;
  const auto beforeHint = s.view();
  (void)send(s, SorterActionKind::ShowHint);
  expect(s.view().hintVisible && s.view().owners == beforeHint.owners && !s.view().undoDepth && !s.view().inspected,
      "asking for a hint never assigns or silently inspects a card");
  (void)send(s, SorterActionKind::CloseHint);
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::Dump);
  clickTwice(s, manual);
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::E);
  (void)send(s, SorterActionKind::SelectBucket, 0, SorterBucket::E);
  const auto before = s.view();
  const SorterAction stale{SorterActionKind::AutoSort, SorterBucket::A, 0, before.revision};
  (void)send(s, SorterActionKind::ShowHint);
  expect(!s.dispatch(stale).accepted && s.view().owners == before.owners, "stale Auto sort is rejected atomically");
  (void)send(s, SorterActionKind::CloseHint);
  expect(send(s, SorterActionKind::AutoSort).changed, "Auto sort groups eligible remaining cards");
  const auto sorted = s.view();
  expect(sorted.undoDepth == before.undoDepth + 1 && sorted.counts[0] == 1 && !sorted.autoSortCount,
      "one automatic transaction leaves unclassified cards available");
  for (const auto& e : s.content()) {
    const SorterOwner expected = e.id == manual ? SorterOwner{SorterBucket::Dump} : e.subject ?
        SorterOwner{sorterSubjects[static_cast<unsigned>(*e.subject)].bucket} : SorterOwner{};
    expect(sorted.owners[e.homeIndex] == expected, "prepared subjects determine new groups and preserve manual choices");
  }
  expect(sorted.inventory && sorted.activeBucket == before.activeBucket, "Auto sort preserves the active inventory");
  expect(!send(s, SorterActionKind::AutoSort).changed && s.view().undoDepth == sorted.undoDepth, "repeat Auto sort creates no empty Undo");
  (void)send(s, SorterActionKind::Undo);
  expect(s.view().owners == before.owners && s.view().undoDepth == before.undoDepth && s.view().inventorySlots == sorted.inventorySlots,
      "one Undo restores exact prior owners and keeps inventory reservations");
  (void)send(s, SorterActionKind::BackToGrid);
  (void)send(s, SorterActionKind::ActivateEquation, s.content()[1].id);
  expect(s.view().hint.find(s.content()[1].hint) != std::string::npos && s.view().hint.find("Trigonometry") != std::string::npos,
      "inspected-card hint comes from prepared content and subject");
  invariants(s);
}
int main() {
  try { basic(); reservedUndoAndBulk(); fullAndMixed(); autoSortAndHints(); std::cout << "Sorter state, Auto sort, hints, recovery and 5000 mixed actions passed\n"; }
  catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
